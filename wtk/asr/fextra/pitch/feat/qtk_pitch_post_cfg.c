#include "wtk/asr/fextra/pitch/feat/qtk_pitch_post_cfg.h"

int qtk_pitch_post_cfg_init(qtk_pitch_post_cfg_t *cfg) {
    cfg->pitch_scale = 2.0;
    cfg->pov_scale = 2.0;
    cfg->pov_offset = 0.0;
    cfg->delta_pitch_scale = 10.0;
    cfg->delta_pitch_noise_stddev = 0.005;
    cfg->normalization_left_context = 75;
    cfg->normalization_right_context = 75;
    cfg->delta_window = 2;
    cfg->delay = 0;

    cfg->add_pov_feature = 1;
    cfg->add_normalized_log_pitch = 1;
    cfg->add_delta_pitch = 1;
    cfg->add_raw_log_pitch = 0;

    qtk_feature_delta_cfg_init(&cfg->delta);

    return 0;
}

int qtk_pitch_post_cfg_update(qtk_pitch_post_cfg_t *cfg) {
    cfg->delta.order = 1;
    cfg->delta.window = cfg->delta_window;
    qtk_feature_delta_cfg_update(&cfg->delta);
    return 0;
}

int qtk_pitch_post_cfg_clean(qtk_pitch_post_cfg_t *cfg) {
    qtk_feature_delta_cfg_clean(&cfg->delta);
    return 0;
}
