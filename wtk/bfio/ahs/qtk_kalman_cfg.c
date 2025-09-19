#include "qtk_kalman_cfg.h"
int qtk_ahs_kalman_cfg_init(qtk_ahs_kalman_cfg_t *cfg){
    cfg->kalman_thresh = 1e-5;
    cfg->kalman_order = 1;
    cfg->kalman_type = 0;
    cfg->L = 24;
    cfg->pvorg = 0.05;
    cfg->pworg = 0.5;
    cfg->kalman_a = 0.9;
    cfg->dg_col = 8;
    cfg->dg_row = 8;
    cfg->use_dg = 0;
    cfg->use_residual_cancellation = 0;
    cfg->gamma = 0.93;
    cfg->use_symmetric_ph = 0;
    cfg->Phi_SS_smooth_factor = 0.5;
    cfg->use_res = 0;
    return 0;
}

int qtk_ahs_kalman_cfg_clean(qtk_ahs_kalman_cfg_t *cfg){
    return 0;
}

int qtk_ahs_kalman_cfg_update_local(qtk_ahs_kalman_cfg_t *cfg, wtk_local_cfg_t *lc){
    wtk_string_t *v;

    wtk_local_cfg_update_cfg_i(lc, cfg, kalman_order, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, kalman_type, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, L, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_dg, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_residual_cancellation, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_symmetric_ph, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_res, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, Phi_SS_smooth_factor, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, gamma, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, kalman_thresh, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, pworg, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, pvorg, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, kalman_a, v);
    return 0;
}

int qtk_ahs_kalman_cfg_update(qtk_ahs_kalman_cfg_t *cfg){
    return 0;
}

int qtk_ahs_kalman_cfg_update2(qtk_ahs_kalman_cfg_t *cfg, wtk_source_loader_t *sl){
    return 0;
}