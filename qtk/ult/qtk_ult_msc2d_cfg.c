#include "qtk/ult/qtk_ult_msc2d_cfg.h"

int qtk_ult_msc2d_cfg_init(qtk_ult_msc2d_cfg_t *cfg) {
    cfg->lag = 2;
    cfg->nsensor = 32;
    cfg->nzc = 64;
    cfg->nmic = 0;
    cfg->xcorr_use_alpha = 1;
    cfg->xcorr_alpha = 0.8;
    cfg->num_tgt = 6;
    cfg->mic = NULL;
    cfg->use_fft = 1;
    return 0;
}

int qtk_ult_msc2d_cfg_clean(qtk_ult_msc2d_cfg_t *cfg) { return 0; }

int qtk_ult_msc2d_cfg_update(qtk_ult_msc2d_cfg_t *cfg) {
    if (cfg->nmic < 2) {
        wtk_debug("Error Mic Cfg\n");
        return -1;
    }
    return 0;
}

int qtk_ult_msc2d_cfg_update_local(qtk_ult_msc2d_cfg_t *cfg,
                                   wtk_local_cfg_t *lc) {
    wtk_string_t *v;
    wtk_array_t *mic_a;
    wtk_local_cfg_update_cfg_i(lc, cfg, lag, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, nsensor, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, nzc, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, num_tgt, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, xcorr_use_alpha, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, xcorr_alpha, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_fft, v);
    mic_a = wtk_local_cfg_find_float_array_s(lc, "mic");
    if (mic_a) {
        cfg->nmic = mic_a->nslot / 2;
        cfg->mic = (float *)mic_a->slot;
    }
    return 0;
}
