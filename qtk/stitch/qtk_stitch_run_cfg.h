#ifndef __QTK_STITCH_RUN_CFG_H__
#define __QTK_STITCH_RUN_CFG_H__

#include "qtk_stitch_run_camera_cfg.h"
#include "qtk_stitch_run_seammask_cfg.h"
#include "qtk_stitch_run_rectangle_cfg.h"
#include "qtk_stitch_def.h"
#include "qtk_stitch_humanseg_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int camera_num;
    qtk_stitch_run_camera_cfg_t *cameras_cfg;
    qtk_stitch_run_seammask_cfg_t *seammasks_cfg;
    qtk_stitch_run_rectangle_cfg_t *intersection;
    qtk_stitch_run_rectangle_cfg_t *overlapping;
    int estimator;
    int warper_type;
    int remap_type;
    int crop;
    int compensator;
    int nr_feeds;
    int block_size;
    int blender_type;
    float blend_strength;
    int finder;
    int use_gpu;
    float medium_megapix;
    float low_megapix;
    int *camera_alpha;
    float *camera_alpha_x;
    float seammask_threshold;
    int use_alpha;
    float smooth;
    int use_compensator;
    int use_compensator_on_single;

    int w;
    int h;
    int use_nthread;
    int use_all_img;
    int src_type;

    int use_reseammask;
    int reseammask_point_num;
    int reseammask_split_num;
    int use_shift_seam_position;
    int *shift_seam_offsets;

    int left_full;
    int right_full;
    
    int use_humanseg;
    qtk_stitch_humanseg_cfg_t humanseg_cfg;
} qtk_stitch_run_cfg_t;

int qtk_stitch_run_cfg_init(qtk_stitch_run_cfg_t *cfg);
int qtk_stitch_run_cfg_clean(qtk_stitch_run_cfg_t *cfg);
int qtk_stitch_run_cfg_update_local(qtk_stitch_run_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_stitch_run_cfg_update(qtk_stitch_run_cfg_t *cfg);

#ifdef __cplusplus
};
#endif

#endif