#include "qtk/cv/detection/qtk_objdect.h"
#include "wtk/core/wtk_alloc.h"
#include "qtk/core/qtk_type.h"
#include "qtk/math/qtk_math.h"

static int _bbox_conf_cmp(qtk_cv_bbox_conf_t *a, qtk_cv_bbox_conf_t *b) {
    if (a->conf < b->conf) {
        return 1;
    }
    if (a->conf > b->conf) {
        return -1;
    }
    return 0;
}

// result shape = (feat_height, feat_width, num_sz * 4)
void qtk_objdect_priorbox_gen(int feat_width, int feat_height, int img_width,
                              int img_height, int step, int num_sz,
                              float *pwinszs, float *result) {
    for (int r = 0; r < feat_height; r++) {
        for (int c = 0; c < feat_width; c++) {
            for (int s = 0; s < num_sz; s++) {
                float min_sz = pwinszs[s];
                float center_x = (c + 0.5f) * step;
                float center_y = (r + 0.5f) * step;
                *result++ = (center_x - min_sz / 2.0) / img_width;
                *result++ = (center_y - min_sz / 2.0) / img_height;
                *result++ = (center_x + min_sz / 2.0) / img_width;
                *result++ = (center_y + min_sz / 2.0) / img_height;
            }
        }
    }
}

int qtk_objdect_post_init(qtk_objdect_post_t *odp, int nbox, float *priorbox,
                          float *loc, float *conf, float *iou, int keep_topk) {
    odp->nbox = nbox;
    odp->priorbox = priorbox;
    odp->loc = loc;
    odp->conf = conf;
    odp->iou = iou;
    odp->keep_topk = keep_topk;
    odp->loc_stride = 14;
    odp->iou_stride = 1;
    odp->conf_stride = 2;
    odp->result = wtk_calloc(sizeof(qtk_cv_bbox_conf_t), keep_topk);
    qtk_min_heap_init2(&odp->mheap, sizeof(qtk_cv_bbox_conf_t), 10,
                       cast(qtk_min_heap_cmp_f, _bbox_conf_cmp));
    return 0;
}

void qtk_objdect_post_set_stride(qtk_objdect_post_t *odp, int loc_stride,
                                 int conf_stride, int iou_stride) {
    odp->loc_stride = loc_stride;
    odp->conf_stride = conf_stride;
    odp->iou_stride = iou_stride;
}

int qtk_objdect_post_clean(qtk_objdect_post_t *odp) {
    wtk_free(odp->result);
    qtk_min_heap_clean2(&odp->mheap);
    return 0;
}

// return ndetect
int qtk_objdect_post_process(qtk_objdect_post_t *odp, float overlap_threshold,
                             float conf_threshold) {
    float prior_variance[4] = {0.1, 0.1, 0.2, 0.2};
    float *priorbox = odp->priorbox;
    float *loc = odp->loc;
    float *iou = odp->iou;
    float *conf = odp->conf;
    int idx;
    int nbox = odp->nbox;
    int n;
    int keep_topk = odp->keep_topk;
    int ndetect = 0;

    // clamp iou
    for (int i = 0; i < nbox; i++) {
        idx = i * odp->iou_stride + 0;
        if (iou[idx] < 0.0) {
            iou[idx] = 0;
        }
        if (iou[idx] > 1.0) {
            iou[idx] = 1;
        }
    }

    for (int i = 0; i < nbox; i++) {
        float cls_score = conf[(i * odp->conf_stride) + 1];
        float iou_score = iou[i * odp->iou_stride + 0];
        qtk_cv_bbox_t fbox;
        float cur_conf = qtk_sqrtf(cls_score * iou_score);
        float locx1, locy1, locx2, locy2;
        float prior_width, prior_height, prior_center_x, prior_center_y;
        float box_centerx, box_centery, box_width, box_height;

        if (cur_conf <= conf_threshold) {
            continue;
        }

        fbox.x1 = priorbox[i << 2];
        fbox.y1 = priorbox[(i << 2) + 1];
        fbox.x2 = priorbox[(i << 2) + 2];
        fbox.y2 = priorbox[(i << 2) + 3];

        locx1 = loc[i * odp->loc_stride]; // TODO remove landmark releated
        locy1 = loc[i * odp->loc_stride + 1];
        locx2 = loc[i * odp->loc_stride + 2];
        locy2 = loc[i * odp->loc_stride + 3];

        prior_width = fbox.x2 - fbox.x1;
        prior_height = fbox.y2 - fbox.y1;
        prior_center_x = (fbox.x1 + fbox.x2) / 2;
        prior_center_y = (fbox.y1 + fbox.y2) / 2;

        box_centerx = prior_variance[0] * locx1 * prior_width + prior_center_x;
        box_centery = prior_variance[1] * locy1 * prior_height + prior_center_y;
        box_width = qtk_expf(prior_variance[2] * locx2) * prior_width;
        box_height = qtk_expf(prior_variance[3] * locy2) * prior_height;

        fbox.x1 = max(0.0, box_centerx - box_width / 2.0);
        fbox.y1 = max(0.0, box_centery - box_height / 2.0);
        fbox.x2 = min(1.0, box_centerx + box_width / 2.0);
        fbox.y2 = min(1.0, box_centery + box_height / 2.0);

        qtk_min_heap_push(&odp->mheap, &(qtk_cv_bbox_conf_t){cur_conf, fbox});
    }

    n = odp->mheap.nelem;

    for (int i = 0; i < n; i++) {
        int keep = 1;
        qtk_cv_bbox_conf_t cur;
        qtk_min_heap_pop(&odp->mheap, &cur);
        for (int j = 0; j < ndetect; j++) {
            float overlap =
                qtk_cv_bbox_jaccard_overlap(&cur.box, &odp->result[j].box);
            if (overlap > overlap_threshold) {
                keep = 0;
                break;
            }
        }
        if (keep) {
            odp->result[ndetect] = cur;
            if (++ndetect == keep_topk) {
                break;
            }
        }
    }

    return ndetect;
}
