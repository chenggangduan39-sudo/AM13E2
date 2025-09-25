#include "qtk_erb_cfg.h"
int qtk_ahs_erb_cfg_init(qtk_ahs_erb_cfg_t *cfg){
    cfg->erb_subband_1 = 33;
    cfg->erb_subband_2 = 32;
    cfg->nfft = 512;
    cfg->high_lim = 8000;
    cfg->fs = 16000;
    cfg->update = 0;
    return 0;
}

int qtk_ahs_erb_cfg_clean(qtk_ahs_erb_cfg_t *cfg){
    return 0;
}

int qtk_ahs_erb_cfg_update_local(qtk_ahs_erb_cfg_t *cfg, wtk_local_cfg_t *lc){
    wtk_string_t *v;

    wtk_local_cfg_update_cfg_i(lc, cfg, erb_subband_1, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, erb_subband_2, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, nfft, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, high_lim, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, fs, v);
    wtk_local_cfg_update_cfg_b(lc,cfg,update,v);    
    return 0;
}

int qtk_ahs_erb_cfg_update(qtk_ahs_erb_cfg_t *cfg){
    return 0;
}

int qtk_ahs_erb_cfg_update2(qtk_ahs_erb_cfg_t *cfg, wtk_source_loader_t *sl){
    return 0;
}