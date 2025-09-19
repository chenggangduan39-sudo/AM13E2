#ifndef __QTK_STITCH_H__
#define __QTK_STITCH_H__

/** 
项目来源
https://github.com/OpenStitching/stitching.git
*/

#include "qtk_stitch_def.h"
#include "qtk_stitch_cfg.h"
#include "qtk_stitch_feature_detector.h"
#include "qtk_stitch_feature_matcher.h"
#include "qtk_stitch_subsetter.h"
#include "qtk_stitch_camera_estimator.h"
#include "qtk_stitch_camera_adjuster.h"
#include "qtk_stitch_wave_corrector.h"
#include "qtk_stitch_warper.h"
#include "qtk_stitch_cropper.h"
#include "qtk_stitch_exposure_compensator.h"
#include "qtk_stitch_seamfinder.h"
#include "qtk_stitch_blender.h"
#include "qtk_stitch_timelapser.h"
#include "wtk/core/wtk_queue.h"
#include "wtk/os/wtk_lockhoard.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_stitch{
    qtk_stitch_cfg_t *cfg;

    qtk_stitch_feature_detector_t *detector;
    qtk_stitch_feature_matcher_t *matcher;
    qtk_stitch_subsetter_t *subsetter;
    qtk_stitch_camera_estimator_t *camera_estimator; //作为运行camera
    qtk_stitch_camera_estimator_t *camera_estimator2; //暂时作为new camera使用
    qtk_stitch_camera_estimator_t *camera_estimator_tmp; //暂时作为tmp使用
    qtk_stitch_camera_adjuster_t *camera_adjuster;
    qtk_stitch_wave_corrector_t *wave_corrector;
    qtk_stitch_warper_t *warper;
    qtk_stitch_cropper_t *cropper;
    qtk_stitch_exposure_compensator_t *compensator;
    qtk_stitch_seamfinder_t *seam_finder;
    qtk_stitch_blender_t *blender;
    qtk_stitch_timelapser_t *timelapser;
    void *reseam_masks; //std::vector<cv::UMat>
    
    wtk_queue_t img_queue;
    wtk_lockhoard_t img_hoard;
    int *size_w;
    int *size_h;

    float match_conf;
    int try_use_gpu;
    int touch_camera; //作为照片变换的依据
    int camera_update; //更新了相机的相机参数
}qtk_stitch_t;

qtk_stitch_t* qtk_stitch_new(qtk_stitch_cfg_t *cfg);
void qtk_stitch_push_image_file(qtk_stitch_t* stitch, const char* filename);
void* qtk_stitch_stitch(qtk_stitch_t* stitch);
void* qtk_stitch_stitch2(qtk_stitch_t* stitch);
void qtk_stitch_delete(qtk_stitch_t* stitch);
void* qtk_stitch_get_end(qtk_stitch_t *stitch,int *row,int *col);
void qtk_stitch_queue_clean(qtk_stitch_t* stitch);

#ifdef __cplusplus
}
#endif

#endif