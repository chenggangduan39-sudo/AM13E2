#include "qtk/cv/detection/qtk_objdect_yolo.h"
#include "wtk/core/wtk_alloc.h"
#include "qtk/core/qtk_type.h"
#include "qtk/math/qtk_math.h"

static int bbox_conf_cmp_(qtk_cv_bbox_result_t *a, qtk_cv_bbox_result_t *b) {
    if (a->conf < b->conf) {
        return 1;
    }
    if (a->conf > b->conf) {
        return -1;
    }
    return 0;
}

int qtk_objdetect_yolo_post_init(qtk_objdetect_yolo_post_t *p, int *stride,
                                 int nstride, float *anchors, int nanchors,
                                 int nclass, int width, int height) {
    int stride_bytes, anchors_bytes;

    stride_bytes = sizeof(p->stride[0]) * nstride;
    anchors_bytes = sizeof(p->anchors[0]) * nstride * nanchors * 2;

    p->nstride = nstride;
    p->nanchors = nanchors;
    p->nclass = nclass;
    p->height = height;
    p->width = width;

    p->stride = wtk_malloc(stride_bytes);
    p->anchors = wtk_malloc(anchors_bytes);

    memcpy(p->stride, stride, stride_bytes);
    memcpy(p->anchors, anchors, anchors_bytes);

    p->result = wtk_malloc(sizeof(qtk_cv_bbox_result_t) * 10);
    p->result_cap = 10;

    qtk_min_heap_init2(&p->min_heap, sizeof(qtk_cv_bbox_result_t), 10,
                       cast(qtk_min_heap_cmp_f, bbox_conf_cmp_));

    return 0;
}

void qtk_objdetect_yolo_post_clean(qtk_objdetect_yolo_post_t *p) {
    wtk_free(p->anchors);
    wtk_free(p->stride);
    wtk_free(p->result);
    qtk_min_heap_clean2(&p->min_heap);
}

static int expand_result_(qtk_objdetect_yolo_post_t *p) {
    qtk_cv_bbox_result_t *new_result =
        wtk_malloc(sizeof(qtk_cv_bbox_result_t) * p->result_cap * 2);
    if (p->nresult > 0) {
        memcpy(new_result, p->result,
               p->nresult * sizeof(qtk_cv_bbox_result_t));
    }
    wtk_free(p->result);
    p->result = new_result;
    p->result_cap = p->result_cap * 2;
    return 0;
}

static int argmax_(float *num, int n, float *max_val) {
    int res = 0;
    *max_val = num[0];

    for (int i = 1; i < n; i++) {
        if (num[i] > *max_val) {
            res = i;
            *max_val = num[i];
        }
    }

    return res;
}

void qtk_objdetect_yolo_post_process(qtk_objdetect_yolo_post_t *p,
                                     float *nn_out, float conf_thresh,
                                     float obj_thresh, float nms_thresh,
                                     int frame_width, int frame_height) {
    int nn_out_stride = p->nanchors * 4 + p->nanchors + 1;
    float ratioh = (float)frame_height / p->height;
    float ratiow = (float)frame_width / p->width;
    p->nresult = 0;

#define SQUARE(x) ((x) * (x))

    for (int i = 0; i < p->nstride; i++) {
        int grid_w = p->width / p->stride[i];
        int grid_h = p->height / p->stride[i];
        for (int gh = 0; gh < grid_h; gh++) {
            for (int gw = 0; gw < grid_w; gw++) {
                for (int j = 0; j < p->nanchors; j++) {
                    float class_score;
                    float obj_score = nn_out[4 * p->nanchors + j];
                    int class_id =
                        argmax_(nn_out + 4 * p->nanchors + p->nanchors,
                                p->nclass, &class_score);

                    if (class_score < conf_thresh || obj_score < obj_thresh ||
                        class_score * obj_score < conf_thresh) {
                        continue;
                    }

                    float cx = (nn_out[j * 4 + 0] * 2.0 - 0.5 + gw) *
                               p->stride[i] * ratiow;
                    float cy = (nn_out[j * 4 + 1] * 2.0 - 0.5 + gh) *
                               p->stride[i] * ratioh;
                    float w = SQUARE(nn_out[j * 4 + 2] * 2.0) *
                              p->anchors[i * p->nanchors * 2 + j * 2 + 0] *
                              ratiow;
                    float h = SQUARE(nn_out[j * 4 + 3] * 2.0) *
                              p->anchors[i * p->nanchors * 2 + j * 2 + 1] *
                              ratioh;

                    qtk_cv_bbox_t box;

                    box.x1 = cx - w / 2;
                    box.y1 = cy - h / 2;
                    box.x2 = box.x1 + w;
                    box.y2 = box.y1 + h;

                    qtk_min_heap_push(&p->min_heap, &(qtk_cv_bbox_result_t){
                                                        class_score * obj_score,
                                                        class_id, box});
                }
                nn_out += nn_out_stride;
            }
        }
    }

    {
        qtk_cv_bbox_result_t cur_box;
        while (0 == qtk_min_heap_pop(&p->min_heap, &cur_box)) {
            int keep = 1;
            for (int i = 0; i < p->nresult; i++) {
                float iou = qtk_cv_bbox_jaccard_overlap(&cur_box.box,
                                                        &p->result[i].box);
                if (iou > nms_thresh) {
                    keep = 0;
                }
            }
            if (keep) {
                if (p->nresult == p->result_cap) {
                    expand_result_(p);
                }
                p->result[p->nresult].box.x1 = qtk_roundf(cur_box.box.x1);
                p->result[p->nresult].box.x2 = qtk_roundf(cur_box.box.x2);
                p->result[p->nresult].box.y1 = qtk_roundf(cur_box.box.y1);
                p->result[p->nresult].box.y2 = qtk_roundf(cur_box.box.y2);

                p->result[p->nresult].conf = cur_box.conf;
                p->result[p->nresult].clsid = cur_box.clsid;
                p->nresult++;
            }
        }
    }

#undef SQUARE
}
