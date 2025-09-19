#include "qtk_stitch_cfg.h"
#include "qtk_stitch_def.h"
int qtk_stitch_cfg_init(qtk_stitch_cfg_t *cfg)
{
    cfg->medium_megapix = 0.6;
    cfg->detector = QTK_STITCH_FEATURE_DETECTOR_ORB;
    cfg->nfeatures = 500;
    cfg->matcher_type = QTK_STITCH_FEATURE_MATCHER_HOMOGRAPHY;
    cfg->range_width = -1;
    cfg->try_use_gpu = 0;
    cfg->match_conf = 0.0f;
    cfg->confidence_threshold = 1;
    wtk_string_set_s(&cfg->matches_graph_dot_file,"");
    cfg->estimator = QTK_STITCH_CAMERA_ESTIMATOR_HOMOGRAPHY;
    cfg->adjuster = QTK_STITCH_CAMERA_ADJUSTER_RAY;
    wtk_string_set_s(&cfg->refinement_mask,"xxxxx");
    cfg->wave_correct_kind = QTK_STITCH_WAVE_CORRECT_HORIZ;
    cfg->warper_type = QTK_STITCH_WARPER_SPHERICAL;
    cfg->low_megapix = 0.1;
    cfg->crop = 1;
    cfg->compensator = QTK_STITCH_EXPOSURE_GAIN_BLOCK;
    cfg->nr_feeds = 1;
    cfg->block_size = 32;
    cfg->finder = QTK_STITCH_FINDER_DP_COLOR;
    cfg->final_megapix = -1;
    cfg->blender_type = QTK_STITCH_BLENDER_MULTIBAND;
    cfg->blend_strength = 5;
    cfg->timelapse = QTK_STITCH_TIMELAPSE_NO;
    wtk_string_set_s(&cfg->timelapse_prefix,"fixed_");

    cfg->num_camera = 3;

    return 0;
}

int qtk_stitch_cfg_clean(qtk_stitch_cfg_t *cfg)
{
    return 0;
}

int qtk_stitch_cfg_update_local(qtk_stitch_cfg_t *cfg,wtk_local_cfg_t *main_lc)
{
    wtk_string_t *v;
    wtk_local_cfg_update_cfg_i(main_lc,cfg,num_camera,v);
    wtk_local_cfg_update_cfg_f(main_lc,cfg,medium_megapix,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,detector,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,nfeatures,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,matcher_type,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,range_width,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,try_use_gpu,v);
    wtk_local_cfg_update_cfg_f(main_lc,cfg,match_conf,v);
    wtk_local_cfg_update_cfg_f(main_lc,cfg,confidence_threshold,v);
    wtk_local_cfg_update_cfg_string_v(main_lc,cfg,matches_graph_dot_file,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,estimator,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,adjuster,v);
    wtk_local_cfg_update_cfg_string_v(main_lc,cfg,refinement_mask,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,wave_correct_kind,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,warper_type,v);
    wtk_local_cfg_update_cfg_f(main_lc,cfg,low_megapix,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,crop,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,compensator,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,nr_feeds,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,block_size,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,finder,v);
    wtk_local_cfg_update_cfg_f(main_lc,cfg,final_megapix,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,blender_type,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,blend_strength,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,timelapse,v);
    wtk_local_cfg_update_cfg_string_v(main_lc,cfg,timelapse_prefix,v);
    return 0;
}

int qtk_stitch_cfg_update(qtk_stitch_cfg_t *cfg)
{
    return 0;
}