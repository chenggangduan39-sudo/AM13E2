#ifndef __QTK_STITCH_WARPER_H__
#define __QTK_STITCH_WARPER_H__

#include "qtk_stitch_def.h"
#include "wtk/core/wtk_queue.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_stitch_warper{
    int type;
    float scale;
    void *warper_imgs; //cv::Mat[]
    void *warper_mask; //cv::Mat[]
    void *roi_corners; //std::vector<cv::Point>
    void *roi_sizes; //std::vector<cv::Size>
    void *dst_roi; //std::vector<cv::Rect>
    void *uxmap; //std::vector<cv::UMat>
    void *uymap; //std::vector<cv::UMat>
    int num_camera;
} qtk_stitch_warper_t;

qtk_stitch_warper_t* qtk_stitch_warper_new(int type,int num_camera);
void qtk_stitch_warper_set_scale(qtk_stitch_warper_t* warpe, void *estimated_cameras);
void qtk_stitch_warper_delete(qtk_stitch_warper_t* warpe);
void qtk_stitch_warper_warp_images(qtk_stitch_warper_t* warpe,wtk_queue_t* queue,
                void *estimated_cameras,float aspect,int type);
void qtk_stitch_warper_create_and_warp_mask(qtk_stitch_warper_t* warpe,
                                int *size_w, int *size_h, 
                                void *estimated_cameras,float aspect);
void qtk_stitch_warper_warp_rois(qtk_stitch_warper_t* warpe,
                                int *size_w, int *size_h, 
                                void *estimated_cameras,float aspect);
void qtk_stitch_warper_warp_buildmaps(qtk_stitch_warper_t *warpe,int w, int h,
                                    void *estimated_cameras,float aspect);
void qtk_stitch_warper_warp_clearmaps(qtk_stitch_warper_t *warpe);
void qtk_stitch_warper_warp_images2(qtk_stitch_warper_t* warpe,wtk_queue_t* queue);
void qtk_stitch_warper_warp_image(qtk_stitch_warper_t* warpe,wtk_queue_t* queue,int index);

#ifdef __cplusplus
}
#endif

#endif