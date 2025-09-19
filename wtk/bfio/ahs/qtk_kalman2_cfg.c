#include "qtk_kalman2_cfg.h"
int qtk_ahs_kalman2_cfg_init(qtk_ahs_kalman2_cfg_t *cfg){
    cfg->B = 32;
    cfg->nbin = 129;
    cfg->alpha = 0.999;
    cfg->keep_m_gate = 0.5;
    cfg->p_initial = 1;
    cfg->use_res = 1;
    return 0;
}

int qtk_ahs_kalman2_cfg_clean(qtk_ahs_kalman2_cfg_t *cfg){
    return 0;
}

int qtk_ahs_kalman2_cfg_update_local(qtk_ahs_kalman2_cfg_t *cfg, wtk_local_cfg_t *lc){
    wtk_string_t *v;

    wtk_local_cfg_update_cfg_i(lc, cfg, B, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, nbin, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_res, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, alpha, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, keep_m_gate, v);
    return 0;
}

int qtk_ahs_kalman2_cfg_update(qtk_ahs_kalman2_cfg_t *cfg){
    return 0;
}

int qtk_ahs_kalman2_cfg_update2(qtk_ahs_kalman2_cfg_t *cfg, wtk_source_loader_t *sl){
    return 0;
}