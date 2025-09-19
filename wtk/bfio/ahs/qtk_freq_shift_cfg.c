#include "qtk_freq_shift_cfg.h"
int qtk_ahs_freq_shift_cfg_init(qtk_ahs_freq_shift_cfg_t *cfg){
    cfg->N_tap = 65;
    cfg->freq_shift = 3;
    return 0;
}

int qtk_ahs_freq_shift_cfg_clean(qtk_ahs_freq_shift_cfg_t *cfg){
    return 0;
}

int qtk_ahs_freq_shift_cfg_update_local(qtk_ahs_freq_shift_cfg_t *cfg, wtk_local_cfg_t *lc){
    wtk_string_t *v;

    wtk_local_cfg_update_cfg_i(lc, cfg, N_tap, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, freq_shift, v);
    return 0;
}

int qtk_ahs_freq_shift_cfg_update(qtk_ahs_freq_shift_cfg_t *cfg){
    return 0;
}

int qtk_ahs_freq_shift_cfg_update2(qtk_ahs_freq_shift_cfg_t *cfg, wtk_source_loader_t *sl){
    return 0;
}