#include "qtk/ult/qtk_ult_track.h"
#include "qtk/cv/qtk_cv_bbox.h"
#include "qtk/cv/tracking/qtk_mot_sort.h"
#include "qtk/math/qtk_matrix.h"
#include "qtk/mdl/qtk_sonicnet.h"
#include "qtk/ult/fmcw/qtk_ult_fmcw.h"
#include "qtk/ult/qtk_ult_msc2d.h"
#include "qtk/ult/qtk_ult_ofdm.h"
#include "qtk/ult/qtk_ult_perception.h"
#include "qtk/ult/qtk_ult_track_type.h"
#include "qtk/ult/qtk_ultm2.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/core/wtk_queue.h"
#include "wtk/core/wtk_robin.h"
#include "wtk/core/wtk_sort.h"
#include "wtk/core/wtk_type.h"
#ifdef QTK_WITH_TRACY
#include "qtk/tracy/qtk_tracy.h"
#endif

/* a,b is signed int*/
#define is_a_ge_zero_and_lt_b(a, b) (unsigned int)(a) < (unsigned int)(b)

typedef struct {
    wtk_queue_node_t node;
    wtk_complex_t *cir;
} cir_item_t;

typedef struct {
    float r;
    float theta;
    float range_prob;
    float angle_prob;
} obj_info_t;

typedef struct {
    qtk_mot_sort_tracklet_t *tl;
    obj_info_t *obj_info;
    qtk_ult_track_result_t pre_result;
    obj_info_t last_det;
} tracklet_info_t;

static void tracklet_info_reset_(tracklet_info_t *trk_info) {
    trk_info->obj_info = NULL;
    trk_info->tl = NULL;
    trk_info->pre_result.id = -1;
}

static cir_item_t *cir_item_new_(qtk_ult_track_t *s) {
    cir_item_t *item =
        wtk_malloc(sizeof(cir_item_t) +
                   sizeof(wtk_complex_t) * s->n * s->cfg->ultm2.channel);
    item->cir = (wtk_complex_t *)(item + 1);
    return item;
}

static void cir_item_delete_(qtk_ult_track_t *s, cir_item_t *item) {
    wtk_free(item);
}

static void float_upsamp_(float *s, float *d, int len, int upsamp) {
    int upsamp_len = len * upsamp;
    int ctx_len = upsamp * 2 - 1;
    int offset = (ctx_len - 1) / 2;
    for (int k = 0; k < upsamp_len; k++) {
        float sum = 0;
        for (int ci = 0; ci < ctx_len; ci++) {
            int pesudo_idx = (k + offset - ctx_len + 1 + ci) / upsamp;
            sum += is_a_ge_zero_and_lt_b(pesudo_idx, len) ? s[pesudo_idx] : 0;
        }
        d[k] = sum / ctx_len;
    }
}

static float *range_upsamp_(qtk_ult_track_t *s, int nframe, float *prob) {
    int upsamp_len = s->sonicnet->cfg->D * s->cfg->upsamp;
    float *range_prob =
        wtk_heap_malloc(s->heap, sizeof(float) * nframe * upsamp_len);
    for (int i = 0; i < nframe; i++) {
        float *cur_frame_prob = prob + i * s->sonicnet->cfg->D;
        float *cur_range_prob = range_prob + i * upsamp_len;
        float_upsamp_(cur_frame_prob, cur_range_prob, s->sonicnet->cfg->D,
                      s->cfg->upsamp);
    }
    return range_prob;
}

static void float_alpha_update_(float *d, float *new, int len, float alpha) {
    float beta = 1 - alpha;
    for (int i = 0; i < len; i++) {
        d[i] = alpha * d[i] + beta * new[i];
    }
}

static int search_range_tgt_(qtk_ult_track_t *s, float *range_prob, int D,
                             int *idx_hint, int *n_hint, int tgt_cap) {
    int pre_i = -2;
    int nobj = -1;
    float thresh = s->cfg->nn_prob;
    for (int i = 0; i < D; i++) {
        if (range_prob[i] < thresh) {
            continue;
        }
        if (i - pre_i == 1) {
            if (range_prob[idx_hint[nobj]] < range_prob[i]) {
                idx_hint[nobj] = i;
            }
            n_hint[nobj]++;
        } else {
            if (nobj == tgt_cap - 1) {
                break;
            }
            nobj++;
            idx_hint[nobj] = i;
            n_hint[nobj] = 1;
        }
        pre_i = i;
    }
    nobj++;
    return nobj;
}

static void fuse_pos_decision_(qtk_ult_track_t *s, int *idx_hint, int *n_hint,
                               int nobj, float *range_prob, float *angle_prob) {
    float gamma = s->cfg->gamma;
    wtk_strbuf_reset(s->tmp_buf);
    int D = s->cfg->sonicnet.D * s->cfg->upsamp;
    double *aptd_up = wtk_heap_malloc(s->heap, sizeof(double) * D);
    float angle_unit = 180.0 / s->sonicnet->cfg->K;
    for (int i = 0; i < D; i++) {
        aptd_up[i] = pow(s->smoothed_cir_amp[i], gamma);
    }
    for (int i = 0; i < nobj; i++) {
        obj_info_t cur_obj;
        int tgt_st = max(0, idx_hint[i] - 5 * s->cfg->upsamp);
        int tgt_et = min(D, idx_hint[i] + 3 * s->cfg->upsamp);
        double sum = 0, sum1 = 0;
        for (int j = tgt_st; j < tgt_et; j++) {
            sum += aptd_up[j];
            sum1 += j * aptd_up[j];
        }
        int pos_idx = sum1 / sum;
        int angle_idx = 18;
        if (angle_prob) {
            float *cur_dis_angle_prob =
                angle_prob + pos_idx / s->cfg->upsamp * s->sonicnet->cfg->K;
            angle_idx = 0;
            cur_obj.angle_prob = cur_dis_angle_prob[0];
            for (int j = 1; j < s->sonicnet->cfg->K; j++) {
                if (cur_dis_angle_prob[j] > cur_obj.angle_prob) {
                    angle_idx = j;
                    cur_obj.angle_prob = cur_dis_angle_prob[j];
                }
            }
        }
        cur_obj.range_prob = range_prob[pos_idx];
        cur_obj.r = pos_idx * s->dis_unit;
        cur_obj.theta = angle_idx * angle_unit;
        wtk_strbuf_push(s->tmp_buf, (char *)&cur_obj, sizeof(cur_obj));
    }
}

static void rtheta2bbox_(float r, float theta, qtk_cv_bbox_t *bbox) {
    float fake_half_h = 0.25;
    float fake_half_w = 0.25;
    theta = theta * PI / 180;
    float x = r * sin(theta);
    float y = r * cos(theta);
    bbox->x1 = x - fake_half_w;
    bbox->x2 = x + fake_half_w;
    bbox->y1 = y - fake_half_h;
    bbox->y2 = y + fake_half_h;
}

static void bbox2rtheta_(qtk_cv_bbox_t *bbox, float *r, float *theta) {
    float x = (bbox->x1 + bbox->x2) / 2;
    float y = (bbox->y1 + bbox->y2) / 2;
    *r = sqrt(x * x + y * y);
    *theta = acos(y / *r) * 180 / PI;
}

static float tracklet_cmp_(void *arg, void *a, void *b) {
    tracklet_info_t *x = *(tracklet_info_t **)a;
    tracklet_info_t *y = *(tracklet_info_t **)b;
    float scale = 0.1;
    float fused_prob_x, fused_prob_y;
    fused_prob_x =
        scale * log(x->tl->filter.P2 * (x->tl->age > 1 ? x->tl->filter.P1 : 1));
    fused_prob_y =
        scale * log(y->tl->filter.P2 * (y->tl->age > 1 ? y->tl->filter.P1 : 1));
    fused_prob_x += x->obj_info ? log(x->obj_info->range_prob) : 0;
    fused_prob_y += y->obj_info ? log(y->obj_info->range_prob) : 0;
    return fused_prob_x > fused_prob_y ? -1 : 1;
}

static void track_result_update_height_(qtk_ult_track_t *s,
                                        qtk_ult_track_result_t *res,
                                        qtk_ult_track_result_t *pre_res,
                                        cir_item_t *cir_item) {
    int r_idx = res->r / s->dis_unit;
    int idx_st = max(0, r_idx - s->cfg->height_search_width * s->cfg->upsamp);
    int idx_et = min(s->sonicnet->cfg->D * s->cfg->upsamp,
                     r_idx + s->cfg->height_search_width * s->cfg->upsamp);
    int K = s->sonicnet->cfg->K + 1;
    int D = idx_et - idx_st;
    int i;
    float *prob = wtk_heap_malloc(s->heap, sizeof(float) * K * D);
    float sum = 0, sum1 = 0;
    float angle_unit = 180.0 / s->sonicnet->cfg->K;
    qtk_ult_msc2d_feed(s->y_msc2d, cir_item->cir + s->cfg->ofdm.nsymbols,
                       idx_st, D, 0, K, prob);
    for (i = 0; i < D; i++) {
        int max_idx;
        float *cur_prob = prob + i * (s->sonicnet->cfg->K + 1);
        max_idx = wtk_float_argmax(cur_prob, s->sonicnet->cfg->K + 1);
        sum1 += s->smoothed_cir_amp[i + idx_st] * max_idx;
        sum += s->smoothed_cir_amp[i + idx_st];
    }
    res->y_theta = sum1 / sum * angle_unit - 90;
}

static void sort_post_(qtk_ult_track_t *s, int fake_det, cir_item_t *cir_item) {
    wtk_queue_node_t *node;
    int i;
    tracklet_info_t *tl_info;
    int ntracklet = 0;
    tracklet_info_t **tl_infos =
        wtk_heap_malloc(s->heap, sizeof(void *) * s->sort.trackers.length);
    obj_info_t *obj_data = fake_det ? NULL : (obj_info_t *)s->tmp_buf->data;
    for (node = s->sort.trackers.pop; node; node = node->next) {
        qtk_mot_sort_tracklet_t *tl =
            data_offset2(node, qtk_mot_sort_tracklet_t, q_n);
        tl_info = (tracklet_info_t *)tl->upval;
        if (tl->det_id >= 0 && obj_data) {
            tl_info->obj_info = obj_data + tl->det_id;
            tl_info->last_det = obj_data[tl->det_id];
        } else {
            tl_info->obj_info = NULL;
        }
        tl_info->tl = tl;
        if (!tl->ongoing) {
            continue;
        }
        tl_infos[ntracklet++] = tl_info;
    }
    if (ntracklet > s->cfg->max_tracklet) {
        tracklet_info_t *tmp_info;
        wtk_qsort(tl_infos, tl_infos + ntracklet - 1, sizeof(tracklet_info_t *),
                  tracklet_cmp_, s, &tmp_info);
        ntracklet = s->cfg->max_tracklet;
    }
    wtk_strbuf_reset(s->tmp_buf);
    wtk_strbuf_reset(s->obj);
    for (i = 0; i < ntracklet; i++) {
        qtk_ult_track_result_t res;
        tl_info = tl_infos[i];
        res.id = tl_info->tl->id;
        if (tl_info->tl->age > 1) {
            bbox2rtheta_(&tl_info->tl->predict, &res.r, &res.theta);
        } else {
            res.r = tl_info->last_det.r;
            res.theta = tl_info->last_det.theta;
        }
        if (tl_info->pre_result.id >= 0) {
            res.theta = s->cfg->hist_alpha * tl_info->pre_result.theta +
                        (1 - s->cfg->hist_alpha) * res.theta;
        }
        if (s->cfg->with_height) {
            track_result_update_height_(s, &res, &tl_info->pre_result,
                                        cir_item);
        } else {
            res.y_theta = 0;
        }
        tl_info->pre_result = res;
        wtk_strbuf_push(s->tmp_buf, (char *)&res, sizeof(res));
        wtk_strbuf_push(s->obj, (char *)&tl_info, sizeof(void *));
    }
}

static void track_sort_(qtk_ult_track_t *s, cir_item_t *cir_item) {
    int fake_det = 0;
    int nobj = s->tmp_buf->pos / sizeof(obj_info_t);
    obj_info_t *obj_data = (obj_info_t *)s->tmp_buf->data;
    qtk_cv_bbox_t *dets =
        wtk_heap_malloc(s->heap, sizeof(qtk_cv_bbox_t) * nobj);
    for (int i = 0; i < nobj; i++) {
        rtheta2bbox_(obj_data[i].r, obj_data[i].theta, dets + i);
    }
    if (nobj == 0 && s->cfg->use_perception &&
        ((s->perception_res.state_1m || s->perception_res.state_5m) &&
         s->perception_res.trusted)) {
        tracklet_info_t **tl = (tracklet_info_t **)s->obj->data;
        nobj = s->obj->pos / sizeof(void *);
        dets = wtk_heap_malloc(s->heap, sizeof(qtk_cv_bbox_t) * nobj);
        fake_det = 1;
        for (int i = 0; i < nobj; i++) {
            rtheta2bbox_(tl[i]->last_det.r, tl[i]->last_det.theta, dets + i);
        }
    }
    qtk_mot_sort_update(&s->sort, nobj, dets);
    sort_post_(s, fake_det, cir_item);
}

static void update_perception_(qtk_ult_track_t *s) {
    uint32_t chunk_s, chunk_e;
    float prob;
    qtk_ult_perception_input_t in;
    qtk_ult_track_result_t *trk = (qtk_ult_track_result_t *)s->tmp_buf->data;
    int ntrk = s->tmp_buf->pos / sizeof(qtk_ult_track_result_t);
    in.nobj = ntrk;
    in.trk = trk;
    if (!qtk_sonicnet_get_vad_result(s->sonicnet, &chunk_s, &chunk_e, &prob) &&
        chunk_s != s->vad_pre_chunk_s) {
        s->vad_pre_chunk_s = chunk_s;
        in.vad_prob = prob;
    } else {
        in.vad_prob = -1;
    }
    qtk_ult_perception_feed(s->perception, &in, &s->perception_res);
    if (s->perception_res.state_1m || s->perception->state_5m) {
        wtk_strbuf_reset(s->tmp_buf);
    }
}

static void raise_obj_(qtk_ult_track_t *s) {
    s->notifier(s->upval, s->nframe,
                s->tmp_buf->pos / sizeof(qtk_ult_track_result_t),
                (qtk_ult_track_result_t *)s->tmp_buf->data);
}

static float obj_cmp_(void *args, void *arga, void *argb) {
    obj_info_t *a = (obj_info_t *)arga;
    obj_info_t *b = (obj_info_t *)argb;
    return a->range_prob > b->range_prob ? -1 : 1;
}

static void update_obj_angle_via_nn_(qtk_ult_track_t *s, obj_info_t *obj_data,
                                     float *angle_prob) {
    float *best_angle_idx =
        wtk_heap_malloc(s->heap, sizeof(float) * s->sonicnet->cfg->D);
    float *best_angle_idx_upsamp = wtk_heap_malloc(
        s->heap, sizeof(float) * s->sonicnet->cfg->D * s->cfg->upsamp);
    float angle_unit = 180.0 / s->sonicnet->cfg->K;
    int pos_idx = obj_data[0].r / s->dis_unit;
    int idx_st = max(0, pos_idx - s->cfg->angle_search_width * s->cfg->upsamp);
    int idx_et = min(s->sonicnet->cfg->D * s->cfg->upsamp,
                     pos_idx + s->cfg->angle_search_width * s->cfg->upsamp);
    float sum = 0, sum1 = 0;
    for (int i = 0; i < s->sonicnet->cfg->D; i++) {
        int idx = wtk_float_argmax(angle_prob + i * s->sonicnet->cfg->K,
                                   s->sonicnet->cfg->K);
        best_angle_idx[i] = idx;
    }
    float_upsamp_(best_angle_idx, best_angle_idx_upsamp, s->sonicnet->cfg->D,
                  s->cfg->upsamp);
    for (int i = idx_st; i < idx_et; i++) {
        sum += s->smoothed_cir_amp[i];
        sum1 += s->smoothed_cir_amp[i] * best_angle_idx_upsamp[i];
    }
    obj_data[0].theta = sum1 / sum * angle_unit;
}

static void update_obj_angle_via_msc2d_(qtk_ult_track_t *s,
                                        obj_info_t *obj_data,
                                        cir_item_t *cir_item) {
    int pos_idx = obj_data[0].r / s->dis_unit;
    float *prob = s->msc2d_prob;
    float angle_unit = 180.0 / s->sonicnet->cfg->K;
    int idx_st = max(0, pos_idx - s->cfg->angle_search_width * s->cfg->upsamp);
    int idx_et = min(s->sonicnet->cfg->D * s->cfg->upsamp,
                     pos_idx + s->cfg->angle_search_width * s->cfg->upsamp);
    int i, idx_len = idx_et - idx_st;
    float sum = 0, sum1 = 0;
    qtk_ult_msc2d_feed(s->msc2d, cir_item->cir, idx_st, idx_len, 0,
                       s->sonicnet->cfg->K + 1, prob);
    for (i = 0; i < idx_len; i++) {
        int max_idx;
        float *cur_prob = prob + i * (s->sonicnet->cfg->K + 1);
        max_idx = wtk_float_argmax(cur_prob, s->sonicnet->cfg->K + 1);
        sum1 += s->smoothed_cir_amp[i + idx_st] * max_idx;
        sum += s->smoothed_cir_amp[i + idx_st];
    }
    obj_data[0].theta = sum1 / sum * angle_unit;
}

static void prune_obj_(qtk_ult_track_t *s, float *range_prob, float *angle_prob,
                       cir_item_t *cir_item) {
    obj_info_t tmp_obj;
    int nobj = s->tmp_buf->pos / sizeof(obj_info_t);
    if (nobj <= s->cfg->max_tgt && (s->cfg->max_tgt > 1 || nobj == 0)) {
        return;
    }
    obj_info_t *obj_data = (obj_info_t *)s->tmp_buf->data;
    wtk_qsort(obj_data, obj_data + nobj - 1, sizeof(obj_info_t), obj_cmp_, s,
              &tmp_obj);
    s->tmp_buf->pos = sizeof(obj_info_t) * s->cfg->max_tgt;
    if (s->sonicnet_with_angle && s->cfg->max_tgt == 1) {
        if (s->active && s->msc2d && 0) {
            update_obj_angle_via_msc2d_(s, obj_data, cir_item);
        } else {
            update_obj_angle_via_nn_(s, obj_data, angle_prob);
        }
    }
}

static void update_active_(qtk_ult_track_t *s, cir_item_t *feat_item) {
    int N = s->cfg->ofdm.nsymbols * s->cfg->upsamp;
    float e = 0;
    int i;
    for (i = 0; i < N; i++) {
        e += s->smoothed_cir_amp[i];
    }
    s->active = e > s->cfg->act_cir_amp_e;
}

static void accumulate_cir_amp_upsamp_(qtk_ult_track_t *s,
                                       float *cir_amp_upsamp_acc,
                                       wtk_complex_t *cir) {
    int i;
    float *cir_amp = wtk_heap_malloc(s->heap, sizeof(float) * s->n);
    float *cir_amp_upsamp =
        wtk_heap_malloc(s->heap, sizeof(float) * s->n * s->cfg->upsamp);
    for (i = 0; i < s->n; i++) {
        cir_amp[i] = sqrt(cir[i].a * cir[i].a + cir[i].b * cir[i].b);
    }
    float_upsamp_(cir_amp, cir_amp_upsamp, s->n, s->cfg->upsamp);
    for (i = 0; i < s->n * s->cfg->upsamp; i++) {
        cir_amp_upsamp_acc[i] += cir_amp_upsamp[i];
    }
}

static void process_frame_(qtk_ult_track_t *s, float *range_prob,
                           float *angle_prob) {
#define TGT_CAP 128
    int D = s->cfg->sonicnet.D * s->cfg->upsamp;
    int idx_hint[TGT_CAP];
    int n_hint[TGT_CAP];
    int nobj;
    int channel = s->cfg->ultm2.channel;
    cir_item_t *cir_item, *stale_cir_item;
    wtk_queue_node_t *node;
    s->nframe++;

    node = wtk_queue_pop(&s->chan_resp);
    stale_cir_item = data_offset2(node, cir_item_t, node);
    cir_item = s->cfg->use_latest_cir
                   ? data_offset2(s->chan_resp.push, cir_item_t, node)
                   : stale_cir_item;
    float *cir_amp = wtk_heap_zalloc(s->heap, sizeof(float) * D);
    for (int c = 0; c < channel; c++) {
        wtk_complex_t *_cir = cir_item->cir + c * s->n;
        accumulate_cir_amp_upsamp_(s, cir_amp, _cir);
    }
    if (s->nframe == 1) {
        memcpy(s->smoothed_range_prob, range_prob, D * sizeof(float));
        memcpy(s->smoothed_cir_amp, cir_amp, sizeof(float) * D);
    } else {
        float_alpha_update_(s->smoothed_range_prob, range_prob, D,
                            s->cfg->range_prob_alpha);
        float_alpha_update_((float *)s->smoothed_cir_amp, cir_amp, D,
                            s->cfg->aptd_dynamic_alpha);
    }
    nobj = search_range_tgt_(s, s->smoothed_range_prob, D, idx_hint, n_hint,
                             TGT_CAP);
    fuse_pos_decision_(s, idx_hint, n_hint, nobj, s->smoothed_range_prob,
                       angle_prob);
    prune_obj_(s, range_prob, angle_prob, cir_item);
    track_sort_(s, cir_item);
    if (s->cfg->use_perception) {
        update_perception_(s);
    }
    raise_obj_(s);
    update_active_(s, cir_item);
    cir_item_delete_(s, stale_cir_item);
#undef TGT_CAP
}

static int post_(qtk_ult_track_t *s, int nframe, float *prob) {
    int D = s->sonicnet->cfg->D;
    int K = s->sonicnet->cfg->K;
#ifdef QTK_WITH_TRACY
    int shape[] = {nframe, D};
    qtk_tracy_log_tensor_s("range_prob", QTK_TRACY_TENSOR_TYPE_FP32, shape, 2,
                           prob);
#endif
    float *range_prob = range_upsamp_(s, nframe, prob);
    float *angle_prob = prob + nframe * D;
    float *angle_probT = NULL;
    if (s->sonicnet_with_angle) {
        angle_probT = wtk_heap_malloc(s->heap, sizeof(float) * nframe * D * K);
        qtk_matrix_transposef(angle_prob, angle_probT, K, nframe * D);
    }
    for (int i = 0; i < nframe; i++) {
        process_frame_(s, range_prob + i * D * s->cfg->upsamp,
                       angle_probT ? angle_probT + i * D * K : NULL);
    }
    return 0;
}

static void update_dyn_cir_(qtk_ult_track_t *s) {
    wtk_complex_t *cur_cfr;
    int N = s->cfg->ultm2.channel * s->n;
    cir_item_t *cir_item = cir_item_new_(s);
    memcpy(cir_item->cir, wtk_robin_at(s->cir_ctx, 0),
           sizeof(wtk_complex_t) * N);
    for (int i = 1; i < s->cir_ctx->nslot; i++) {
        cur_cfr = wtk_robin_at(s->cir_ctx, i);
        for (int j = 0; j < N; j++) {
            cir_item->cir[j].a += cur_cfr[j].a;
            cir_item->cir[j].b += cur_cfr[j].b;
        }
    }
    cur_cfr = wtk_robin_at(s->cir_ctx, s->cfg->left_ctx);
    for (int j = 0; j < N; j++) {
        cir_item->cir[j].a =
            cur_cfr[j].a - cir_item->cir[j].a / s->cir_ctx->nslot;
        cir_item->cir[j].b =
            cur_cfr[j].b - cir_item->cir[j].b / s->cir_ctx->nslot;
    }
    wtk_queue_push(&s->chan_resp, &cir_item->node);
}

static void msc2d_warmup_(qtk_ult_track_t *s, wtk_complex_t *cur_cir) {
    int d = s->n;
    int D = d * s->cfg->upsamp;
    int K = s->sonicnet->cfg->K + 1;
    float *cir_amp = wtk_heap_zalloc(s->heap, sizeof(float) * D);
    float *cur_prob, accum_prob;
    int c, i;
    int channel = s->cfg->ultm2.channel;
    int candi_pos, candi_angle;
    int start_idx, end_idx, nrange;
    float max_cir;
    obj_info_t obj;
    float angle_unit = 180.0 / s->sonicnet->cfg->K;
    for (c = 0; c < channel; c++) {
        accumulate_cir_amp_upsamp_(s, cir_amp, cur_cir + c * d);
    }
    candi_pos = wtk_float_argmax(cir_amp, D);
    max_cir = cir_amp[candi_pos];
    if (max_cir < s->cfg->warmup_cir_thresh) {
        return;
    }
    start_idx = max(0, candi_pos - 5 * s->cfg->upsamp);
    end_idx = min(D - 1, candi_pos + 5 * s->cfg->upsamp);
    nrange = end_idx - start_idx;
    qtk_ult_msc2d_feed(s->msc2d, cur_cir, start_idx, nrange, 0,
                       s->sonicnet->cfg->K + 1, s->msc2d_prob);
    float max_prob = 0;
    candi_pos = 0;
    for (i = 0; i < nrange; i++) {
        cur_prob = s->msc2d_prob + i * (s->sonicnet->cfg->K + 1);
        accum_prob = wtk_float_sum(cur_prob, s->sonicnet->cfg->K + 1);
        if (max_prob < accum_prob) {
            max_prob = accum_prob;
            candi_pos = i + start_idx;
        }
    }
    cur_prob =
        s->msc2d_prob + (candi_pos - start_idx) * (s->sonicnet->cfg->K + 1);
    candi_angle = wtk_float_argmax(cur_prob, K);
    wtk_strbuf_reset(s->tmp_buf);
    obj.angle_prob = 0;
    obj.range_prob = 0;
    obj.r = s->dis_unit * candi_pos;
    obj.theta = angle_unit * candi_angle;
    wtk_strbuf_push(s->tmp_buf, (char *)&obj, sizeof(obj));
}

static int on_ultm2_(qtk_ult_track_t *s, float **wav, int len) {
    int c = s->ultm2->cfg->channel;
    int nframe;
    float *prob;
    wtk_complex_t *feat;
    int i;

    wtk_complex_t *cur_cir = wtk_robin_next(s->cir_ctx);
    float dis_start = s->sonicnet->cfg->dis_start;
    float dis_end = s->sonicnet->cfg->dis_end;
    int N = s->ofdm->cfg->nsymbols;
    int gap = s->ofdm->cfg->gap_frame;
    wtk_complex_t *cur_cfr =
        wtk_heap_malloc(s->heap, sizeof(wtk_complex_t) * N * c);
    for (i = 0; i < c; i++) {
        wtk_complex_t *cfr, *cir;
        qtk_ult_ofdm_estimate_cfr(s->ofdm, wav[i], dis_start, dis_end, &cfr,
                                  &cir);
        for (int k = 0; k < N; k++) {
            cur_cir[i * N + k] = cir[k * gap];
            cur_cfr[i * N + k] = cfr[k * gap];
        }
    }
    feat = cur_cfr;

    if (wtk_robin_is_full(s->cir_ctx)) {
        update_dyn_cir_(s);
    }

    qtk_sonicnet_feed(s->sonicnet, feat, &nframe, &prob);
    if (nframe > 0) {
        post_(s, nframe, prob);
    } else {
        if (s->cfg->startup_warmup && s->nframe == 0 &&
            s->chan_resp.length > 0 && s->cfg->channel >= 3 && s->msc2d) {
            wtk_queue_node_t *n =
                wtk_queue_peek(&s->chan_resp, s->chan_resp.length - 1);
            cir_item_t *item = data_offset2(n, cir_item_t, node);
            msc2d_warmup_(s, item->cir);
            track_sort_(s, item);
            if (s->cfg->use_perception) {
                update_perception_(s);
            }
            raise_obj_(s);
        }
    }

    return 0;
}

static int on_sort_(qtk_ult_track_t *s, qtk_mot_sort_tracklet_t *kt, int add) {
    if (add) {
        if (s->cfg->inactive_halt && !s->active) {
            return 1;
        }
        tracklet_info_t *tl_info = wtk_vpool_pop(s->tl_info);
        tracklet_info_reset_(tl_info);
        kt->upval = tl_info;
    } else {
        tracklet_info_t *tl_info = (tracklet_info_t *)kt->upval;
        wtk_vpool_push(s->tl_info, tl_info);
    }
    return 0;
}

static int on_fmcw_(qtk_ult_track_t *s, wtk_complex_t *cir) {
    int nframe;
    float *prob;
    wtk_complex_t *cur_cir = wtk_robin_next(s->cir_ctx);
    memcpy(cur_cir, cir, s->n * sizeof(wtk_complex_t) * s->cfg->channel);
    if (wtk_robin_is_full(s->cir_ctx)) {
        update_dyn_cir_(s);
    }
    qtk_sonicnet_feed(s->sonicnet, cir, &nframe, &prob);
    if (nframe > 0) {
        post_(s, nframe, prob);
    } else {
        if (s->cfg->startup_warmup && s->nframe == 0 &&
            s->chan_resp.length > 0 && s->cfg->channel >= 3 && s->msc2d) {
            wtk_queue_node_t *n =
                wtk_queue_peek(&s->chan_resp, s->chan_resp.length - 1);
            cir_item_t *item = data_offset2(n, cir_item_t, node);
            msc2d_warmup_(s, item->cir);
            track_sort_(s, item);
            if (s->cfg->use_perception) {
                update_perception_(s);
            }
            raise_obj_(s);
        }
    }
    return 0;
}

qtk_ult_track_t *qtk_ult_track_new(qtk_ult_track_cfg_t *cfg) {
    qtk_ult_track_t *s = wtk_malloc(sizeof(qtk_ult_track_t));
    int K = cfg->ofdm.nsymbols * cfg->upsamp;
    int c = cfg->ultm2.channel;
    int i;
    float df;
    s->cfg = cfg;
    s->n = cfg->use_fmcw ? cfg->fmcw.nsamples : cfg->ofdm.nsymbols;
    K = s->n * cfg->upsamp;
    if (cfg->use_fmcw) {
        s->ultm2 = NULL;
        s->fmcw = qtk_ult_fmcw_new(&cfg->fmcw, s, (qtk_ult_fmcw_notifier_t )on_fmcw_);
    } else {
        s->ofdm = qtk_ult_ofdm_new(&cfg->ofdm);
        s->ultm2 = qtk_ultm2_new(&cfg->ultm2, cfg->ofdm.period,
                                 cfg->ofdm.gap_frame - 1);
        qtk_ultm2_set_notify(s->ultm2, (qtk_ultm2_notify_f)on_ultm2_, s);
    }
    s->sonicnet = qtk_sonicnet_new(&cfg->sonicnet);
    s->heap = wtk_heap_new(4096);
    s->smoothed_range_prob = wtk_malloc(sizeof(float) * K);
    s->cir_ctx = wtk_robin_new(1 + cfg->left_ctx + cfg->right_ctx);
    s->cir_ctx_data = wtk_malloc(sizeof(wtk_complex_t) * s->cir_ctx->nslot *
                                 cfg->ofdm.nsymbols * c);
    for (i = 0; i < s->cir_ctx->nslot; i++) {
        s->cir_ctx->r[i] = s->cir_ctx_data + i * cfg->ofdm.nsymbols * c;
    }
    s->smoothed_cir_amp = wtk_malloc(sizeof(float) * K);
    for (i = 0; i < K; i++) {
        s->smoothed_cir_amp[i] = 1.0;
    }
    s->tmp_buf = wtk_strbuf_new(1024, 1);
    s->obj = wtk_strbuf_new(1024, 1);
    df = (float)cfg->ofdm.sampling_rate / cfg->ofdm.period;
    s->dis_unit = 340.0 / (2.0 * df * cfg->ofdm.nsymbols * cfg->upsamp);
    s->nframe = 0;
    wtk_queue_init(&s->chan_resp);
    qtk_mot_sort_init(&s->sort, &cfg->sort);
    qtk_mot_sort_set_notifier(&s->sort, s, (qtk_mot_sort_notifier_t)on_sort_);

    if (s->cfg->channel >= 3) {
        float fc =
            cfg->use_fmcw ? cfg->fmcw.central_freq : cfg->ofdm.central_freq;
        s->msc2d = qtk_ult_msc2d_new(&cfg->msc2d);
        s->msc2d_prob = wtk_malloc(sizeof(float) * (s->sonicnet->cfg->K + 1) *
                                   (s->cfg->ofdm.nsymbols * s->cfg->upsamp));
        qtk_ult_msc2d_update_steer(
            s->msc2d, df, fc, -PI / 2, PI / 2, PI / 36, 0,
            cfg->ofdm.nsymbols * s->dis_unit * s->cfg->upsamp * 2,
            s->dis_unit * 2);
    } else {
        s->msc2d = NULL;
        s->msc2d_prob = NULL;
    }
    if (s->cfg->with_height) {
        float fc =
            cfg->use_fmcw ? cfg->fmcw.central_freq : cfg->ofdm.central_freq;
        s->y_msc2d = qtk_ult_msc2d_new(&cfg->y_msc2d);
        qtk_ult_msc2d_update_steer(
            s->y_msc2d, df, fc, -PI / 2, PI / 2, PI / 36, 0,
            cfg->ofdm.nsymbols * s->dis_unit * s->cfg->upsamp * 2,
            s->dis_unit * 2);
    } else {
        s->y_msc2d = NULL;
    }

    s->active = 0;
    s->sonicnet_with_angle = s->cfg->sonicnet.channel > 1;
    s->tl_info = wtk_vpool_new(sizeof(tracklet_info_t), 20);
    s->perception = qtk_ult_perception_new(&cfg->perception);
    s->vad_pre_chunk_s = 100;
    memset(&s->perception_res, 0, sizeof(s->perception_res));
    return s;
}

int qtk_ult_track_feed(qtk_ult_track_t *s, short **wav, int len) {
    int c = s->cfg->channel;
    wtk_heap_reset(s->heap);
    if (s->cfg->use_fmcw) {
        qtk_ult_fmcw_feed(s->fmcw, wav, len);
    } else {
        qtk_ultm2_feed(s->ultm2, (char **)wav, len * 2, 0, c);
    }
    return 0;
}

int qtk_ult_track_feed_end(qtk_ult_track_t *s) {
    float *prob;
    int nframe;
    if (!s->cfg->use_fmcw) {
        qtk_ultm2_feed(s->ultm2, NULL, 0, 1, s->cfg->channel);
        qtk_sonicnet_feed_end(s->sonicnet, &nframe, &prob);
        if (nframe > 0) {
            post_(s, nframe, prob);
        }
        wtk_heap_reset(s->heap);
    }
    return 0;
}

void qtk_ult_track_delete(qtk_ult_track_t *s) {
    wtk_queue_node_t *nxt;
    for (wtk_queue_node_t *n = s->chan_resp.pop; n; n = nxt) {
        nxt = n->next;
        cir_item_t *item = data_offset2(n, cir_item_t, node);
        cir_item_delete_(s, item);
    }
    wtk_queue_init(&s->chan_resp);
    if (s->cfg->use_fmcw) {
        qtk_ult_fmcw_delete(s->fmcw);
    } else {
        qtk_ult_ofdm_delete(s->ofdm);
        qtk_ultm2_delete(s->ultm2);
    }
    wtk_free(s->smoothed_cir_amp);
    wtk_strbuf_delete(s->tmp_buf);
    wtk_strbuf_delete(s->obj);
    wtk_free(s->smoothed_range_prob);
    wtk_heap_delete(s->heap);
    wtk_robin_delete(s->cir_ctx);
    wtk_free(s->cir_ctx_data);
    qtk_sonicnet_delete(s->sonicnet);
    qtk_mot_sort_clean(&s->sort);
    if (s->msc2d) {
        qtk_ult_msc2d_delete(s->msc2d);
        wtk_free(s->msc2d_prob);
    }
    if (s->y_msc2d) {
        qtk_ult_msc2d_delete(s->y_msc2d);
    }
    wtk_vpool_delete(s->tl_info);
    qtk_ult_perception_delete(s->perception);
    wtk_free(s);
}

void qtk_ult_track_set_notifier(qtk_ult_track_t *s, void *upval,
                                qtk_ult_track_notifier_t notifier) {
    s->notifier = notifier;
    s->upval = upval;
}
