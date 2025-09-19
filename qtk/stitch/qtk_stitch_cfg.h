#ifndef __QTK_STITCH_CFG_H__
#define __QTK_STITCH_CFG_H__

#include "wtk/core/cfg/wtk_local_cfg.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct qtk_stitch_cfg{
    float medium_megapix;
    int detector;
    int nfeatures;
    int matcher_type;
    int range_width;
    int try_use_gpu;
    float match_conf;
    float confidence_threshold;
    wtk_string_t matches_graph_dot_file;
    int estimator;
    int adjuster;
    wtk_string_t refinement_mask;
    int wave_correct_kind;
    int warper_type;
    float low_megapix;
    int crop;
    int compensator;
    int nr_feeds;
    int block_size;
    int finder;
    float final_megapix;
    int blender_type;
    float blend_strength;
    int timelapse;
    wtk_string_t timelapse_prefix;
    int num_camera;
}qtk_stitch_cfg_t;

int qtk_stitch_cfg_init(qtk_stitch_cfg_t *cfg);
int qtk_stitch_cfg_clean(qtk_stitch_cfg_t *cfg);
int qtk_stitch_cfg_update_local(qtk_stitch_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_stitch_cfg_update(qtk_stitch_cfg_t *cfg);

#ifdef __cplusplus
}
#endif

#endif