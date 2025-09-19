#ifndef __QTK_STITCH_CAMERA_ADJUSTER_H__
#define __QTK_STITCH_CAMERA_ADJUSTER_H__

#include "wtk/core/wtk_queue.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct qtk_stitch_camera_adjuster{
    int type;
    char *refinement_mask;
    float confidence_threshold;
    void *adjuster;
}qtk_stitch_camera_adjuster_t;

qtk_stitch_camera_adjuster_t* qtk_stitch_camera_adjuster_new(int type,
                                                        char* refinement_mask, 
                                                        float confidence_threshold);
void qtk_stitch_camera_adjuster_adjust(qtk_stitch_camera_adjuster_t* adjuster,
                                        wtk_queue_t *queue,void *pairwise_matchesp,
                                        void *estimated_cameras);
void qtk_stitch_camera_adjuster_delete(qtk_stitch_camera_adjuster_t* adjuster);

#ifdef __cplusplus
}
#endif

#endif