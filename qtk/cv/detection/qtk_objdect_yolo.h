#ifndef G_1ON4SWXI_2KDS_FO1Y_X0D3_H5HHXKASQ2VP
#define G_1ON4SWXI_2KDS_FO1Y_X0D3_H5HHXKASQ2VP
#pragma once
#include "qtk/core/qtk_min_heap.h"
#include "qtk/cv/qtk_cv_bbox.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_objdetect_yolo_post qtk_objdetect_yolo_post_t;

struct qtk_objdetect_yolo_post {
    int *stride;
    int nstride;
    float *anchors;
    int nanchors;
    int nclass;
    int width;
    int height;

    qtk_min_heap_t min_heap;
    qtk_cv_bbox_result_t *result;
    int nresult;
    int result_cap;
};

int qtk_objdetect_yolo_post_init(qtk_objdetect_yolo_post_t *p, int *stride,
                                 int nstride, float *anchors, int nanchors,
                                 int nclass, int width, int height);
void qtk_objdetect_yolo_post_clean(qtk_objdetect_yolo_post_t *p);
void qtk_objdetect_yolo_post_process(qtk_objdetect_yolo_post_t *p,
                                     float *nn_out, float conf_thresh,
                                     float obj_thresh, float nms_thresh,
                                     int frame_width, int frame_height);

#ifdef __cplusplus
};
#endif
#endif /* G_1ON4SWXI_2KDS_FO1Y_X0D3_H5HHXKASQ2VP */
