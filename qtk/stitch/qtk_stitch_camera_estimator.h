#ifndef __QTK_STITCH_CAMERA_ESTIMATOR_H__
#define __QTK_STITCH_CAMERA_ESTIMATOR_H__

#include "wtk/core/wtk_queue.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct qtk_stitch_camera_estimator{
    int type;
    void *estimator;
    void *cameras; //数据 list
}qtk_stitch_camera_estimator_t;

qtk_stitch_camera_estimator_t* qtk_stitch_camera_estimator_new(int type);
void qtk_stitch_camera_estimator_estimate(qtk_stitch_camera_estimator_t* estimator,
                                          wtk_queue_t *queue,void *pairwise_matchesp);
void qtk_stitch_camera_estimator_delete(qtk_stitch_camera_estimator_t* estimator);
void qtk_stitch_camera_estimator_camera_push(qtk_stitch_camera_estimator_t* estimator,float aspect,
                                float focal,float ppx,float ppy,float *R,float *t);
float qtk_stitch_camera_estimator_distance(qtk_stitch_camera_estimator_t* estimator1,qtk_stitch_camera_estimator_t* estimator2);
void qtk_stitch_camera_estimator_2to1(qtk_stitch_camera_estimator_t* estimator1,qtk_stitch_camera_estimator_t* estimator2);
void qtk_stitch_camera_estimator_2copy1(qtk_stitch_camera_estimator_t* estimator2,qtk_stitch_camera_estimator_t* estimator1);
#ifdef __cplusplus
}
#endif


#endif