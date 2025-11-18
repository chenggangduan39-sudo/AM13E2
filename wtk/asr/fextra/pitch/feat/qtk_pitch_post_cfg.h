#ifndef __QTK_FEAT_QTK_PITCH_POST_CFG_H__
#define __QTK_FEAT_QTK_PITCH_POST_CFG_H__
#include "wtk/asr/fextra/pitch/feat/qtk_feature_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_pitch_post_cfg qtk_pitch_post_cfg_t;

struct qtk_pitch_post_cfg {
    float pitch_scale;
    float pov_scale;
    float pov_offset;
    float delta_pitch_scale;
    float delta_pitch_noise_stddev;
    int normalization_left_context;
    int normalization_right_context;
    int delta_window;
    int delay;

    unsigned add_pov_feature : 1;
    unsigned add_normalized_log_pitch : 1;
    unsigned add_delta_pitch : 1;
    unsigned add_raw_log_pitch : 1;

    qtk_feature_delta_cfg_t delta;
};

int qtk_pitch_post_cfg_init(qtk_pitch_post_cfg_t *cfg);
int qtk_pitch_post_cfg_update(qtk_pitch_post_cfg_t *cfg);
int qtk_pitch_post_cfg_clean(qtk_pitch_post_cfg_t *cfg);

#ifdef __cplusplus
};
#endif
#endif
