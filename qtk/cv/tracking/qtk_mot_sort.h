#ifndef QBL_CV_TRACKING_QBL_MOT_SORT_H
#define QBL_CV_TRACKING_QBL_MOT_SORT_H
#pragma once
#include "qtk/cv/qtk_cv_bbox.h"
#include "qtk/cv/tracking/qtk_mot_sort_cfg.h"
#include "qtk/sci/filter/qtk_filter_kalman.h"
#include "wtk/core/wtk_hoard.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_mot_sort qtk_mot_sort_t;

typedef struct {
    qtk_filter_kalman_t filter;
    wtk_queue_node_t hoard_n;
    wtk_queue_node_t q_n;
    qtk_cv_bbox_t predict;
    int assign_index;
    int id;
    int time_since_update;
    int hits;
    int hit_streak;
    int age;
    int det_id;
    void *upval;
    unsigned ongoing : 1;
} qtk_mot_sort_tracklet_t;

typedef int (*qtk_mot_sort_notifier_t)(void *upval, qtk_mot_sort_tracklet_t *kt,
                                       int add);
typedef float (*qtk_mot_sort_cost_f)(void *ths, qtk_cv_bbox_t *det,
                                     qtk_mot_sort_tracklet_t *kt);

struct qtk_mot_sort {
    qtk_mot_sort_cfg_t cfg;
    int id;
    int frame_count;

    int *result;
    int result_cap;

    wtk_hoard_t trackers_hub;
    wtk_queue_t trackers;
    wtk_heap_t *heap;

    void *upval;
    void *cost_ths;
    qtk_mot_sort_notifier_t notifier;
    qtk_mot_sort_cost_f cost_f;
};

int qtk_mot_sort_init(qtk_mot_sort_t *s, qtk_mot_sort_cfg_t *cfg);
void qtk_mot_sort_clean(qtk_mot_sort_t *s);
int qtk_mot_sort_update(qtk_mot_sort_t *s, int ndet, qtk_cv_bbox_t *dets);
void qtk_mot_sort_set_notifier(qtk_mot_sort_t *s, void *upval,
                               qtk_mot_sort_notifier_t notifier);
void qtk_mot_sort_set_cost_f(qtk_mot_sort_t *s, void *upval,
                             qtk_mot_sort_cost_f cost_f);

#ifdef __cplusplus
};
#endif
#endif
