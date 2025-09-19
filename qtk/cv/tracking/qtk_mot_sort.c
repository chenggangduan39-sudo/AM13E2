#include "qtk/cv/tracking/qtk_mot_sort.h"
#include "wtk/core/wtk_alloc.h"
#include "wtk/core/wtk_hoard.h"
#include "qtk/math/qtk_math.h"
#include "qtk/sci/optimize/qtk_optimize.h"

static void x2bbox_(qtk_cv_bbox_t *bbox, float *x) {
    float w, h;
    w = qtk_sqrtf(x[2] * x[3]);
    h = x[2] / w;
    bbox->x1 = x[0] - w / 2;
    bbox->x2 = x[0] + w / 2;
    bbox->y1 = x[1] - h / 2;
    bbox->y2 = x[1] + h / 2;
}

static void bbox2x_(qtk_cv_bbox_t *bbox, float *x) {
    float w, h;
    w = bbox->x2 - bbox->x1;
    h = bbox->y2 - bbox->y1;
    x[0] = bbox->x1 + w / 2;
    x[1] = bbox->y1 + h / 2;
    x[2] = w * h;
    x[3] = w / h;
}

static qtk_mot_sort_tracklet_t *kalman_tracker_new_(qtk_mot_sort_t *s) {
    qtk_mot_sort_tracklet_t *kt = wtk_malloc(sizeof(qtk_mot_sort_tracklet_t));
    qtk_filter_kalman_init(&kt->filter, 7, 4);
    return kt;
}

static int kalman_tracker_predict_(qtk_mot_sort_tracklet_t *kt) {
    if (kt->filter.x[6] + kt->filter.x[2] <= 0) {
        kt->filter.x[6] *= 0.0;
    }
    qtk_filter_kalman_predict(&kt->filter);
    x2bbox_(&kt->predict, kt->filter.x);
    if (kt->time_since_update > 0) {
        kt->hit_streak = 0;
    }
    kt->age += 1;
    kt->time_since_update += 1;
    return 0;
}

static int kalman_tracker_update_(qtk_mot_sort_tracklet_t *kt,
                                  qtk_cv_bbox_t *bbox) {
    float z[4];
    kt->time_since_update = 0;
    kt->hits += 1;
    kt->hit_streak += 1;
    bbox2x_(bbox, z);
    qtk_filter_kalman_update(&kt->filter, z);
    return 0;
}

static int kalman_tracker_delete_(qtk_mot_sort_t *s,
                                  qtk_mot_sort_tracklet_t *kt) {
    qtk_filter_kalman_clean(&kt->filter);
    wtk_free(kt);
    return 0;
}

static qtk_mot_sort_tracklet_t *mot_sort_new_tracker_(qtk_mot_sort_t *s,
                                                      qtk_cv_bbox_t *bbox) {
    // col first
    float F[] = {1,
                 0,
                 0,
                 0,
                 0,
                 0,
                 0,
                 0,
                 1,
                 0,
                 0,
                 0,
                 0,
                 0,
                 0,
                 0,
                 1,
                 0,
                 0,
                 0,
                 0,
                 0,
                 0,
                 0,
                 1,
                 0,
                 0,
                 0,
                 1,
                 0,
                 0,
                 0,
                 s->cfg.delta_t,
                 0,
                 0,
                 0,
                 1,
                 0,
                 0,
                 0,
                 s->cfg.delta_t,
                 0,
                 0,
                 0,
                 1,
                 0,
                 0,
                 0,
                 1};

    float H[] = {
        1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0,
        0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    };

    qtk_mot_sort_tracklet_t *kt = wtk_hoard_pop(&s->trackers_hub);

    memcpy(kt->filter.F, F, sizeof(F));
    memcpy(kt->filter.H, H, sizeof(H));
    for (int i = 2; i < 4; i++) {
        for (int j = 2; j < 4; j++) {
            kt->filter.R[j * 4 + i] *= 10;
        }
    }
    for (int i = 4; i < 7; i++) {
        for (int j = 4; j < 7; j++) {
            kt->filter.P[j * 7 + i] *= 1000;
        }
    }
    for (int i = 0; i < 7 * 7; i++) {
        kt->filter.P[i] *= 10;
    }
    for (int i = 4; i < 7; i++) {
        for (int j = 4; j < 7; j++) {
            kt->filter.Q[j * 7 + i] *= s->cfg.speed_noise;
        }
    }
    kt->filter.Q[48] *= 0.01;

    bbox2x_(bbox, kt->filter.x);

    kt->assign_index = -1;
    kt->time_since_update = 0;
    kt->id = s->id++;
    kt->hits = 0;
    kt->hit_streak = 0;
    kt->age = 0;
    kt->det_id = -1;
    kt->ongoing = 0;
    wtk_queue_push(&s->trackers, &kt->q_n);
    return kt;
}

static void mot_sort_delete_trakcer_(qtk_mot_sort_t *s,
                                     qtk_mot_sort_tracklet_t *kt) {
    qtk_filter_kalman_reset(&kt->filter);
    wtk_hoard_push(&s->trackers_hub, kt);
}

static int attach_dets_to_trackers_(qtk_mot_sort_t *s, int ndet,
                                    qtk_cv_bbox_t *dets,
                                    int nassign, int *row, int *col) {
    wtk_queue_node_t *node;
    qtk_mot_sort_tracklet_t *kt;
    float *cost_matrix = NULL;
    float cost_threshold;
    int index;

    cost_matrix =
        wtk_heap_malloc(s->heap, sizeof(float) * ndet * s->trackers.length);
    cost_threshold =
        s->cfg.use_distance ? s->cfg.distance_threshold : -s->cfg.iou_threshold;

    if (s->cost_f) {
        for (int i = 0; i < ndet; i++) {
            for (node = s->trackers.pop, index = 0; node;
                 node = node->next, index++) {
                kt = data_offset2(node, qtk_mot_sort_tracklet_t, q_n);
                kt->assign_index = index;
                cost_matrix[i * s->trackers.length + index] =
                    s->cost_f(s->cost_ths, dets + i, kt);
            }
        }
    } else {
        if (s->cfg.use_distance) {
            for (int i = 0; i < ndet; i++) {
                for (node = s->trackers.pop, index = 0; node;
                     node = node->next, index++) {
                    kt = data_offset2(node, qtk_mot_sort_tracklet_t, q_n);
                    kt->assign_index = index;
                    cost_matrix[i * s->trackers.length + index] =
                        qtk_cv_bbox_distance(dets + i, &kt->predict);
                }
            }
        } else {
            for (int i = 0; i < ndet; i++) {
                for (node = s->trackers.pop, index = 0; node;
                     node = node->next, index++) {
                    kt = data_offset2(node, qtk_mot_sort_tracklet_t, q_n);
                    kt->assign_index = index;
                    cost_matrix[i * s->trackers.length + index] =
                        -1 *
                        qtk_cv_bbox_jaccard_overlap(dets + i, &kt->predict);
                }
            }
        }
    }

    qtk_optimize_linear_sum_assignment(ndet, s->trackers.length, cost_matrix,
                                       row, col);

    for (int i = 0; i < nassign; i++) {
        float cost = cost_matrix[row[i] * s->trackers.length + col[i]];
        if (cost > cost_threshold) { // treat as unmatched
            row[i] = col[i] = -1;
        }
    }

    return 0;
}

int qtk_mot_sort_init(qtk_mot_sort_t *s, qtk_mot_sort_cfg_t *cfg) {
    s->cfg = *cfg;
    wtk_hoard_init2(&s->trackers_hub,
                    offsetof(qtk_mot_sort_tracklet_t, hoard_n), 10,
                    cast(qtk_new_handler_t, kalman_tracker_new_),
                    cast(qtk_delete_handler2_t, kalman_tracker_delete_), s);
    s->id = 0;
    s->result_cap = 10;
    s->frame_count = 0;
    s->notifier = NULL;
    s->cost_f = NULL;
    s->upval = NULL;
    s->heap = wtk_heap_new(4096);
    wtk_queue_init(&s->trackers);
    return 0;
}

void qtk_mot_sort_clean(qtk_mot_sort_t *s) {
    wtk_queue_node_t *node;
    while ((node = wtk_queue_pop(&s->trackers))) {
        qtk_mot_sort_tracklet_t *kt = data_offset2(node, qtk_mot_sort_tracklet_t, q_n);
        if (s->notifier) {
            s->notifier(s->upval, kt, 0);
        }
        mot_sort_delete_trakcer_(s, kt);
    }
    wtk_hoard_clean(&s->trackers_hub);
    wtk_heap_delete(s->heap);
}

int qtk_mot_sort_update(qtk_mot_sort_t *s, int ndet, qtk_cv_bbox_t *dets) {
    wtk_queue_node_t *node, *next;
    qtk_mot_sort_tracklet_t *kt;
    int *row = NULL, *col = NULL;
    int nassign;

    s->frame_count++;
    wtk_heap_reset(s->heap);

    s->result = wtk_heap_malloc(s->heap, sizeof(s->result[0]) * ndet);
    for (int i = 0; i < ndet; i++) {
        s->result[i] = -1;
    }

    nassign = min(ndet, s->trackers.length);
    row = wtk_heap_malloc(s->heap, sizeof(row[0]) * nassign);
    col = wtk_heap_malloc(s->heap, sizeof(col[0]) * nassign);

    if (s->trackers.length == 0) {
        for (int i = 0; i < nassign; i++) {
            row[i] = -1;
            col[i] = -1;
        }
    } else {
        for (node = s->trackers.pop; node; node = node->next) {
            kt = data_offset2(node, qtk_mot_sort_tracklet_t, q_n);
            kalman_tracker_predict_(kt);
            kt->det_id = -1;
        }
        attach_dets_to_trackers_(s, ndet, dets, nassign, row, col);
    }

    // process matched
    for (int i = 0; i < nassign; i++) {
        int track_index = col[i];
        int det_index = row[i];
        if (det_index < 0) {
            continue;
        }
        for (node = s->trackers.pop; node; node = node->next) {
            kt = data_offset2(node, qtk_mot_sort_tracklet_t, q_n);
            if (kt->assign_index == track_index) {
                kalman_tracker_update_(kt, dets + det_index);
                kt->det_id = det_index;
            }
        }
    }

    // process unmatched detection
    for (int i = 0; i < ndet; i++) {
        int unmatched = 1;
        for (int j = 0; j < nassign; j++) {
            if (row[j] < 0) {
                continue;
            }
            if (row[j] == i) {
                unmatched = 0;
                break;
            }
        }
        if (unmatched) {
            kt = mot_sort_new_tracker_(s, dets + i);
            kt->det_id = i;
            if (s->notifier && s->notifier(s->upval, kt, 1)) {
                wtk_queue_remove(&s->trackers, &kt->q_n);
                mot_sort_delete_trakcer_(s, kt);
            }
        }
    }

    // process unmatched trackers
    for (node = s->trackers.pop; node; node = next) {
        next = node->next;
        kt = data_offset2(node, qtk_mot_sort_tracklet_t, q_n);
        if (kt->time_since_update < 1 && (kt->hit_streak >= s->cfg.min_hits ||
                                          s->frame_count <= s->cfg.min_hits)) {
            s->result[kt->det_id] = kt->id;
            kt->ongoing = 1;
        } else {
            if (kt->time_since_update > s->cfg.max_age) {
                int reserve = 0;
                if (s->notifier) {
                    reserve = s->notifier(s->upval, kt, 0) != 0;
                }
                if (!reserve) {
                    wtk_queue_remove(&s->trackers, &kt->q_n);
                    mot_sort_delete_trakcer_(s, kt);
                }
            }
        }
    }

    return 0;
}

void qtk_mot_sort_set_notifier(qtk_mot_sort_t *s, void *upval,
                               qtk_mot_sort_notifier_t notifier) {
    s->upval = upval;
    s->notifier = notifier;
}

void qtk_mot_sort_set_cost_f(qtk_mot_sort_t *s, void *upval,
                             qtk_mot_sort_cost_f cost_f) {
    s->cost_ths = upval;
    s->cost_f = cost_f;
}
