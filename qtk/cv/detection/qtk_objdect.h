#ifndef __QBL_OBJECT_DETECTION_QBL_OBJDECT_H__
#define __QBL_OBJECT_DETECTION_QBL_OBJDECT_H__
#pragma once
#include "qtk/core/qtk_min_heap.h"
#include "qtk/cv/qtk_cv_bbox.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_objdect_post qtk_objdect_post_t;

struct qtk_objdect_post {
    int nbox;
    float *priorbox;
    float *loc;
    float *conf;
    float *iou;
    int loc_stride;
    int conf_stride;
    int iou_stride;
    int keep_topk;
    qtk_cv_bbox_conf_t *result;
    qtk_min_heap_t mheap;
};

int qtk_objdect_post_init(qtk_objdect_post_t *odp, int nbox, float *priorbox,
                          float *loc, float *conf, float *iou, int keep_topk);
void qtk_objdect_post_set_stride(qtk_objdect_post_t *odp, int loc_stride,
                                 int conf_stride, int iou_stride);
int qtk_objdect_post_clean(qtk_objdect_post_t *odp);
int qtk_objdect_post_process(qtk_objdect_post_t *odp, float overlap_threshold,
                             float conf_threshold);

// result shape = (feat_height, feat_width, num_sz * 4)
void qtk_objdect_priorbox_gen(int feat_width, int feat_height, int img_width,
                              int img_height, int step, int num_sz,
                              float *pwinszs, float *result);

#ifdef __cplusplus
};
#endif
#endif
