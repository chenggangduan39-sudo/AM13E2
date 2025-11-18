#include "wtk/asr/fextra/pitch/audio/qtk_resample_cfg.h"

int qtk_linear_resample_cfg_init(qtk_linear_resample_cfg_t *cfg) {
    cfg->samp_rate_in_hz = 16000;
    cfg->samp_rate_out_hz = 4000;
    cfg->filter_cutoff_hz = 1000;
    cfg->num_zeros = 1;

    return 0;
}

int qtk_linear_resample_cfg_clean(qtk_linear_resample_cfg_t *cfg) { return 0; }

int qtk_linear_resample_cfg_update(qtk_linear_resample_cfg_t *cfg) {
    cfg->window_width = cfg->num_zeros / (2.0 * cfg->filter_cutoff_hz);
    return 0;
}

int qtk_arbitrary_resample_cfg_init(qtk_arbitrary_resample_cfg_t *cfg) {
    return 0;
}

int qtk_arbitrary_resample_cfg_clean(qtk_arbitrary_resample_cfg_t *cfg) {
    return 0;
}

int qtk_arbitrary_resample_cfg_update(qtk_arbitrary_resample_cfg_t *cfg) {
    return 0;
}
