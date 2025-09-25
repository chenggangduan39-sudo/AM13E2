#include "qtk_cv_detection_stand.h"
#include "qtk/cv/qtk_cv_bbox.h"
#include <stdio.h>

#if 1
typedef struct stand_res {
    float height[STAND_NUM];
    float cen_y[STAND_NUM];
    float cen_x[STAND_NUM];
    float width;
    int cnt;
    int num;
    int st;
} stand_res_t;
static stand_res_t box[100];

static void reset_clean(stand_res_t *box) {
    box->num = -1;
    box->cnt = 0;
    box->st = 0;
    box->width=0;
    for (int i = 0; i < STAND_NUM; i++) {
        box->height[i] = -1;
        box->cen_x[i] = -1;
        box->cen_y[i] = -1;
    }
}

typedef enum {
    TL_INIT,
    TL_STAND,
} tracklet_state_t;

typedef struct {
    hash_str_node_t node;
    qtk_mot_sort_tracklet_t *kt;
    char id[1024];
    tracklet_state_t state;
    int dur;
    float tot;
} tracklet_t;

static int on_sort_(qtk_cv_detection_stand_t *s, qtk_mot_sort_tracklet_t *kt,
                    int add) {
    if (add) {
        tracklet_t *tl = wtk_malloc(sizeof(tracklet_t));
        int len = snprintf(tl->id, sizeof(tl->id), "%d", kt->id);
        tl->kt = kt;
        tl->dur = 0;
        tl->state = TL_INIT;
        wtk_str_hash_add_node(s->tracklets, tl->id, len, tl, &tl->node);
    } else {
        char buf[1024];
        int len = snprintf(buf, sizeof(buf), "%d", kt->id);

        hash_str_node_t *node = wtk_str_hash_remove(s->tracklets, buf, len);
        tracklet_t *tl = node->value;
        wtk_free(tl);
    }
    return 0;
}

static float x_dither_thresh = 0.06;
static int stable_thresh = 3;
static int tl_walker_(qtk_cv_detection_stand_t *s, tracklet_t *tl) {
    float norm_f = sqrtf(tl->kt->filter.x[2]);
    float *x = tl->kt->filter.x;
    float w = sqrtf(x[2] * x[3]);
    float h = x[2] / w;
    float v_s = tl->kt->filter.x[6] / norm_f;
    float v_x = tl->kt->filter.x[4] / w;
    float v_y = tl->kt->filter.x[5] / h;
    if (!tl->kt->ongoing||tl->kt->det_id<0) {
        tl->dur = 0;
        tl->tot = 0;
        tl->state = TL_INIT;
        return 0;
    }
    float iou = qtk_cv_bbox_jaccard_overlap(&tl->kt->predict,
                                            s->sort_dets + tl->kt->det_id);
    if (iou < 0.1) {
        tl->dur = 0;
        tl->tot = 0;
        tl->state = TL_INIT;
        return 0;
    }
    switch (tl->state) {
    case TL_INIT:
        // printf(">>>>>>>%f\n",v_y);
        if (fabsf(v_x) < x_dither_thresh && v_y < -0.01 && v_s < 5) {
            tl->tot += v_y;
            if (++tl->dur > stable_thresh) {
                if (tl->tot < -0.10) {
                    tl->state = TL_STAND;
                    tl->tot = 0;
                    tl->dur = 0;
                }
            }
        } else {
            tl->dur = 0;
            tl->tot = 0;
        }
        break;
    case TL_STAND:
        // printf(">>>>>>>%f\n",v_s);
        s->body_res.box[s->body_res.cnt].roi.x1=(s->sort_dets + tl->kt->det_id)->x1;
        s->body_res.box[s->body_res.cnt].roi.x2=(s->sort_dets + tl->kt->det_id)->x2;
        s->body_res.box[s->body_res.cnt].roi.y1=(s->sort_dets + tl->kt->det_id)->y1;
        s->body_res.box[s->body_res.cnt].roi.y2=(s->sort_dets + tl->kt->det_id)->y2;
        ++s->body_res.cnt; 
        if (fabsf(v_x) > x_dither_thresh || v_y > 0.01 || v_s > 8) {
            if (++tl->dur > 2) {
                tl->state = TL_INIT;
                tl->dur = 0;
                tl->tot = 0;
            }
        } else {
            tl->dur = 0;
            tl->tot = 0;
        }
        break;
    default:;
    }
    return 0;
}

static int sort_post_(qtk_cv_detection_stand_t *s) {
    s->body_res.cnt=0;
    wtk_str_hash_walk(s->tracklets, (wtk_walk_handler_t)tl_walker_, s);
    return 0;
}
#endif

qtk_cv_detection_stand_t *qtk_cv_detection_stand_new() {
    qtk_cv_detection_stand_t *hk = (qtk_cv_detection_stand_t *)wtk_malloc(
        sizeof(qtk_cv_detection_stand_t));
    hk->head = qtk_cv_detection_head_new();
    qtk_mot_sort_cfg_init(&hk->cfg);
    hk->cfg.speed_noise = 10;
    hk->cfg.max_age = 10;
    hk->cfg.min_hits = 1;
    hk->cfg.use_distance = 1;
    hk->cfg.iou_threshold = 0.3;
    hk->cfg.distance_threshold = 80.0;
    qtk_mot_sort_init(&hk->s, &hk->cfg);
    hk->head_res.cnt=0;
    hk->body_res.cnt=0;
    hk->tracklets = wtk_str_hash_new(10);
    qtk_mot_sort_set_notifier(&hk->s, hk, (qtk_mot_sort_notifier_t)on_sort_);
    for (int i = 0; i < 100; i++)
        reset_clean(&box[i]);
    return hk;
}

void qtk_cv_detection_stand_delete(qtk_cv_detection_stand_t *hk) {
    qtk_cv_detection_head_delete(hk->head);
    qtk_mot_sort_clean(&hk->s);
    wtk_str_hash_delete(hk->tracklets);
    wtk_free(hk);
    hk = NULL;
}

void qtk_cv_detection_stand_process(qtk_cv_detection_stand_t *hk,
                                    uint8_t *imagedata, invoke_t invoke) {
    hk->stand_res = 0;
    hk->down_res = 0;
    hk->head_res.cnt = 0;
    hk->head->res.cnt = 0;
    qtk_cv_detection_head_process(hk->head, imagedata, invoke);
    static qtk_cv_bbox_t roi[1000];
    for (int i = 0; i < hk->head->res.cnt; i++) {
        hk->head_res.box[i].roi.x1 = hk->head->res.box[i].roi.x1;
        hk->head_res.box[i].roi.x2 = hk->head->res.box[i].roi.x2;
        hk->head_res.box[i].roi.y1 = hk->head->res.box[i].roi.y1;
        hk->head_res.box[i].roi.y2 = hk->head->res.box[i].roi.y2;
        float w = hk->head->res.box[i].roi.x2 - hk->head->res.box[i].roi.x1;
        float h = hk->head->res.box[i].roi.y2 - hk->head->res.box[i].roi.y1;
        hk->head->res.box[i].roi.x1 =
            max(floor(hk->head->res.box[i].roi.x1 - w * hk->w_dio), 0);
        hk->head->res.box[i].roi.y1 =
            max(floor(hk->head->res.box[i].roi.y1 - h * hk->h_dio), 0);
        hk->head->res.box[i].roi.x2 = min(
            floor(hk->head->res.box[i].roi.x2 + w * hk->w_dio), hk->desc.width);
        hk->head->res.box[i].roi.y2 =
            min(floor(hk->head->res.box[i].roi.y2 + h * hk->h_dio),
                hk->desc.height);
        roi[i].x1 = hk->head->res.box[i].roi.x1;
        roi[i].x2 = hk->head->res.box[i].roi.x2;
        roi[i].y1 = hk->head->res.box[i].roi.y1;
        roi[i].y2 = hk->head->res.box[i].roi.y2;
    }
    hk->head_res.cnt = hk->head->res.cnt;
    if (qtk_mot_sort_update(&hk->s, hk->head->res.cnt, roi) != 0) {
        wtk_debug("sort error!\n");
    }
    hk->sort_dets = roi;
    sort_post_(hk);
    for (int i = 0; i < hk->head->res.cnt; i++) {
        hk->head->res.box[i].num = hk->s.result[i];
        hk->head_res.box[i].num = hk->s.result[i];
    }

#if 0
    for (int i = 0; i < hk->head->res.cnt; i++) {
        if (hk->head_res.box[i].num == -1)
            continue;
        int m = 0;
        for (int j = 0; j < 100; j++) {
            if (box[j].num == hk->head_res.box[i].num) {
                int k = box[j].cnt;
                box[j].width=
                    hk->head_res.box[i].roi.x2 - hk->head_res.box[i].roi.x1;
                box[j].height[k] =
                    hk->head_res.box[i].roi.y2 - hk->head_res.box[i].roi.y1;
                box[j].cen_y[k] =
                    (hk->head_res.box[i].roi.y2 + hk->head_res.box[i].roi.y1) /
                    2.0;
                box[j].cen_x[k]=
                    (hk->head_res.box[i].roi.x2 + hk->head_res.box[i].roi.x1) /
                    2.0;
                box[j].cnt++;
                if (box[j].cnt == STAND_NUM)
                    box[j].cnt = 0;
                break;
            } else
                m++;
        }
        if (m == 100) {
            for (int j = 0; j < 100; j++) {
                if (box[j].num == -1) {
                    box[j].num = hk->head_res.box[i].num;
                    int k = box[j].cnt;
                    box[j].width=
                        hk->head_res.box[i].roi.x2 - hk->head_res.box[i].roi.x1;
                    box[j].height[k] = floor(hk->head_res.box[i].roi.y2 -
                                             hk->head_res.box[i].roi.y1) +
                                       1;
                    box[j].cen_y[k] = (hk->head_res.box[i].roi.y2 +
                                       hk->head_res.box[i].roi.y1) /
                                      2.0;
                    box[j].cen_y[k] = (hk->head_res.box[i].roi.x2 +
                                       hk->head_res.box[i].roi.x1) /
                                      2.0;
                    box[j].cnt++;
                    if (box[j].cnt == STAND_NUM)
                        box[j].cnt = 0;
                    break;
                }
            }
        }
    }

    for (int i = 0; i < hk->body_res.cnt; i++) {
        for (int j = 0; j < hk->head->res.cnt; j++) {
            if (hk->body_res.box[i].num != hk->head->res.box[j].num)
                continue;
            float k = qtk_cv_bbox_jaccard_overlap(&hk->body_res.box[i].roi,
                                                  &hk->head->res.box[j].roi);
            if (k <= 0.6) {
                for (int h = i; h < hk->body_res.cnt; h++) {
                    hk->body_res.box[h].roi.x1 = hk->body_res.box[h + 1].roi.x1;
                    hk->body_res.box[h].roi.y1 = hk->body_res.box[h + 1].roi.y1;
                    hk->body_res.box[h].roi.x2 = hk->body_res.box[h + 1].roi.x2;
                    hk->body_res.box[h].roi.y2 = hk->body_res.box[h + 1].roi.y2;
                    hk->body_res.box[h].num = hk->body_res.box[h + 1].num;
                }
                hk->body_res.cnt--;
                i--;
                break;
            }
        }
    }

    for (int i = 0; i < 100; i++) {
        if (box[i].num == -1)
            continue;
        int m = 0;
        for (int j = 0; j < hk->head_res.cnt; j++) {
            if (hk->head_res.box[j].num == box[i].num)
                break;
            else
                m++;
        }
        if (m == hk->head_res.cnt)
            box[i].st++;
        if (box[i].st == STAND_NUM) {
            for (int j = 0; j < hk->body_res.cnt; j++) {
                if (hk->body_res.box[j].num == box[i].num) {
                    for (int x = j; x < hk->body_res.cnt; x++) {
                        hk->body_res.box[x].num = hk->body_res.box[x + 1].num;
                        hk->body_res.box[x].roi.x1 =
                            hk->body_res.box[x + 1].roi.x1;
                        hk->body_res.box[x].roi.x2 =
                            hk->body_res.box[x + 1].roi.x2;
                        hk->body_res.box[x].roi.y1 =
                            hk->body_res.box[x + 1].roi.y1;
                        hk->body_res.box[x].roi.y2 =
                            hk->body_res.box[x + 1].roi.y2;
                    }
                    hk->body_res.cnt--;
                    break;
                }
            }
            reset_clean(&box[i]);
        }
    }

    for (int i = 0; i < 100; i++) {
        if (box[i].num == -1)
            continue;
        float max_y = box[i].cen_y[0];
        float min_y = box[i].cen_y[0];
        float max_x = 0;
        float min_x = 0;
        int k0 = box[i].cnt - 1;
        if (k0 < 0)
            k0 = STAND_NUM - 1;
        int k2 = k0;
        float height = box[i].height[k0];
        for (int j = 0; j < STAND_NUM / 2; j++) {
            --k2;
            if (k2 < 0)
                k2 = STAND_NUM - 1;
        }
        for (int j = 0; j < STAND_NUM; j++) {
            if (box[i].cen_y[j] > 0) {
                max_y = max(max_y, box[i].cen_y[j]);
                min_y = min(min_y, box[i].cen_y[j]);
            }
            if (box[i].height[j] <= 0)
                continue;
            height = min(height, box[i].height[j]);
        }
        float score=0;
        if (height>30)
            score=hk->h_score_dio;
        else score=hk->score_dio;
        if (max_y - min_y > height * 0.8) {
            float y0 = 0;
            float y1 = 0;
            float max_height = 0;
            for (int m = 0; m < STAND_NUM / 2; m++) {    
                if (box[i].cen_y[k0] > 0)
                    y0 += box[i].cen_y[k0];
                if (box[i].cen_y[k2] > 0)
                    y1 += box[i].cen_y[k2];
                if (box[i].cen_x[k0]>0)
                    max_x+=box[i].cen_x[k0];
                if (box[i].cen_x[k2]>0)
                    min_x+=box[i].cen_x[k2];
                k0--;
                k2--;
                if (k0 < 0)
                    k0 = STAND_NUM - 1;
                if (k2 < 0)
                    k2 = STAND_NUM - 1;
            }
            max_x/=STAND_NUM / 2;
            min_x/=STAND_NUM / 2;
            for (int j = 0; j < STAND_NUM; j++) {
                if (box[i].height[j] <= 0)
                    continue;
                max_height = max(max_height, box[i].height[j]);
            }
            if (y0 - y1 > 0) {
                for (int j = 0; j < hk->body_res.cnt; j++) {
                    if (hk->body_res.box[j].num == box[i].num) {
                        for (int x = j; x < hk->body_res.cnt; x++) {
                            hk->body_res.box[x].num =
                                hk->body_res.box[x + 1].num;
                            hk->body_res.box[x].roi.x1 =
                                hk->body_res.box[x + 1].roi.x1;
                            hk->body_res.box[x].roi.x2 =
                                hk->body_res.box[x + 1].roi.x2;
                            hk->body_res.box[x].roi.y1 =
                                hk->body_res.box[x + 1].roi.y1;
                            hk->body_res.box[x].roi.y2 =
                                hk->body_res.box[x + 1].roi.y2;
                        }
                        hk->body_res.cnt--;
                        hk->down_res++;
                        break;
                    }
                }
            } else if (y0 - y1 < 0 &&
                       max_y - min_y >= score * max_height) {
                if (min_x > 30) {
                    if (fabs(max_x - min_x) > box[i].width * hk->front_x) {
                        // printf("30>>> max_x:%f min_x:%f\n",max_x,min_x);
                        continue;
                    }
                } else if (min_x > 20 && min_x <= 30) {
                    if (fabs(max_x - min_x) > box[i].width * hk->mim_x) {
                        // printf("20>>>  max_x:%f min_x:%f\n",max_x,min_x);
                        continue;
                    }
                } else {
                    if (fabs(max_x - min_x) > box[i].width * hk->heig_x) {
                        // printf("0>>> max_x:%f min_x:%f\n",max_x,min_x);
                        continue;
                    }
                }
                int k = hk->body_res.cnt;
                int m = 0;
                for (int j = 0; j < k; j++) {
                    if (hk->body_res.box[j].num == box[i].num)
                        break;
                    else
                        m++;
                }
                if (m == k) {
                    for (int h = 0; h < hk->head->res.cnt; h++) {
                        if (hk->head->res.box[h].num == box[i].num) {
                            hk->body_res.box[k].num = box[i].num;
                            hk->body_res.box[k].roi.x1 =
                                hk->head->res.box[h].roi.x1;
                            hk->body_res.box[k].roi.x2 =
                                hk->head->res.box[h].roi.x2;
                            hk->body_res.box[k].roi.y1 =
                                hk->head->res.box[h].roi.y1;
                            hk->body_res.box[k].roi.y2 =
                                hk->head->res.box[h].roi.y2;
                            ++hk->body_res.cnt;
                            ++hk->stand_res;
                            break;
                        }
                    }
                }
            }
        }
    }
#endif
}
