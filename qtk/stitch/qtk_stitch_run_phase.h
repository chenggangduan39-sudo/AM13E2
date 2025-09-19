#ifndef __QTK_STITCH_RUN_PHASE_H__
#define __QTK_STITCH_RUN_PHASE_H__

#include "qtk_stitch_run_cfg.h"
#include "qtk_stitch_camera_estimator.h"
#include "qtk_stitch_seamfinder.h"
#include "wtk/core/wtk_queue.h"
#include "wtk/os/wtk_lockhoard.h"
#include "qtk_stitch_warper.h"
#include "qtk_stitch_cropper.h"
#include "qtk_stitch_exposure_compensator.h"
#include "qtk_stitch_blender.h"
#include "wtk/os/wtk_thread.h"
#include "wtk/os/wtk_blockqueue.h"
#include "wtk/os/wtk_sem.h"
#include "qtk_stitch_humanseg.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void(*qtk_stitch_run_phase_notify_f)(void *user_data,void *data,int w,int h,int channel);

typedef struct {
    qtk_stitch_run_cfg_t *cfg;
    void *reseam_masks; //std::vector<cv::UMat>
    // void *reseam_masks_no; //std::vector<cv::UMat>
    void *reseam_masks_low; // std::vector<cv::UMat> 低分变率下的seam_masks
    void *reseam_masks_src_low; // std::vector<cv::UMat> 原始的低分变率下的seam_masks 一般指的是标定好的
    qtk_stitch_camera_estimator_t *camera_estimator;
    qtk_stitch_warper_t *warper;
    qtk_stitch_cropper_t *cropper;
    qtk_stitch_exposure_compensator_t *compensator;
    qtk_stitch_blender_t *blender;
    qtk_stitch_warper_t *low_warper;
    qtk_stitch_cropper_t *low_cropper;
    qtk_stitch_seamfinder_t *seamfinder;
    qtk_stitch_humanseg_t *humanseg;

    wtk_queue_t img_queue;
    wtk_queue_t low_img_queue;
    // wtk_lockhoard_t img_hoard;

    int *size_w;
    int *size_h;
    int *low_seammask_w;
    int *low_seammask_h;
    float camera_aspect;
    float lir_aspect;

    wtk_thread_t paper_thread;
    wtk_thread_t *the_threads;
    int paper_thread_run;
    int threads_run;
    wtk_blockqueue_t paper_bq;
    wtk_blockqueue_t the_bqs;

    wtk_sem_t the_threads_sem;
    wtk_sem_t the_paper_sem;
    void *blend_mem;
    int *seammask_points; //这个是低分辨率seammask的各个点的坐标
    int *seammask_points_src; //这个是原始低分辨率seammask的各个点的坐标
    qtk_stitch_run_phase_notify_f notify;
    void *userdata;
    float *seammask_score;
    // float *seammask_score_src; //各个点的得分
    int first;
    unsigned int update_count;
} qtk_stitch_run_phase_t;

qtk_stitch_run_phase_t* qtk_stitch_run_phase_new(qtk_stitch_run_cfg_t *cfg);
int qtk_stitch_run_phase_delete(qtk_stitch_run_phase_t *phase);
void qtk_stitch_run_phase_push_image_file(qtk_stitch_run_phase_t *phase, const char* filename);
void qtk_stitch_run_phase_push_data(qtk_stitch_run_phase_t *phase, float channel, void *img);
void qtk_stitch_run_phase_run(qtk_stitch_run_phase_t *phase);
void* qtk_stitch_run_phase_get_end(qtk_stitch_run_phase_t *phase,int *row,int *col);
void qtk_stitch_run_phase_estimate_esposure(qtk_stitch_run_phase_t *phase);
void qtk_stitch_run_phase_img_clean(qtk_stitch_run_phase_t* phase);
void qtk_stitch_run_phase_low_img_clean(qtk_stitch_run_phase_t* phase);
int qtk_stitch_run_phase_start(qtk_stitch_run_phase_t *phase);
int qtk_stitch_run_phase_stop(qtk_stitch_run_phase_t *phase);
void qtk_stitch_run_phase_set_blend_mem(qtk_stitch_run_phase_t *phase,void *ptr);
void qtk_stitch_run_phase_get_rect(qtk_stitch_run_phase_t *phase,int *w, int *h);
void qtk_stitch_run_phase_set_rgb(qtk_stitch_run_phase_t *phase,int idx, char alpha,char beta,char gamma,
                                    float alpha_x,float beta_x,float gamma_x);
void qtk_stitch_run_phase_set_blender_smooth(qtk_stitch_run_phase_t *phase,float smooth);
void qtk_stitch_run_phase_push_low_data(qtk_stitch_run_phase_t *phase, void *img);
void qtk_stitch_run_phase_get_low_size(qtk_stitch_run_phase_t *phase,int *w, int *h);
// void qtk_stitch_run_phase_run2(qtk_stitch_run_phase_t *phase);
void qtk_stitch_run_phase_set_seammask_region(qtk_stitch_run_phase_t *phase, int x,int y,int w,int h, int idx);
int qtk_stitch_run_phase_set_notify(qtk_stitch_run_phase_t *phase, void *userdata, qtk_stitch_run_phase_notify_f notify);
int qtk_stitch_run_phase_run_reseammask(qtk_stitch_run_phase_t *phase);
void qtk_stitch_run_phase_run_no_reseammask(qtk_stitch_run_phase_t *phase);
void qtk_stitch_run_phase_push_low_data_rgba(qtk_stitch_run_phase_t *phase, void *img);
void qtk_stitch_run_phase_run_save_overlap(qtk_stitch_run_phase_t *phase,char *outpath);

#ifdef __cplusplus
};
#endif

#endif