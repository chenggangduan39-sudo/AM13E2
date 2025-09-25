#include "wtk_kalman_cfg.h"

int wtk_kalman_cfg_init(wtk_kalman_cfg_t *cfg) {
    cfg->main_cfg = NULL;
    cfg->mbin_cfg = NULL;

    cfg->channel = 0;
    cfg->nmicchannel = 0;
    cfg->nspchannel = 0;
    cfg->rate = 16000;
    cfg->wins = 1024;
    cfg->nbin = 513;
    cfg->B = 4;
    cfg->L = 512;
    cfg->A = 0.999;
    cfg->P_init = 0.1;
    cfg->alpha = 0.8;
    cfg->beta = 0.5;
    cfg->update_thresh = 1e-4;
    cfg->clip_thresh = 1e-4;
    cfg->phi_delta_init = 1e-3;
    cfg->phi_ss_init = 0.05;

    cfg->use_sec_iter = 0;
    cfg->use_res = 0;

    return 0;
}

int wtk_kalman_cfg_clean(wtk_kalman_cfg_t *cfg) { return 0; }

int wtk_kalman_cfg_update_local(wtk_kalman_cfg_t *cfg, wtk_local_cfg_t *m) {
    wtk_string_t *v;
    wtk_local_cfg_t *lc;

    lc = m;
    wtk_local_cfg_update_cfg_i(lc, cfg, channel, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, nmicchannel, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, nspchannel, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, rate, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, wins, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, nbin, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, B, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, L, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, A, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, P_init, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, alpha, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, beta, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, update_thresh, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, clip_thresh, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, phi_delta_init, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, phi_ss_init, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_sec_iter, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_res, v);

    return 0;
}

int wtk_kalman_cfg_update(wtk_kalman_cfg_t *cfg) {
    cfg->nbin = cfg->wins / 2 + 1;
    cfg->L = cfg->wins / 2;
    cfg->nl = cfg->B * cfg->nmicchannel;
    if (cfg->channel < cfg->nmicchannel + cfg->nspchannel) {
        cfg->channel = cfg->nmicchannel + cfg->nspchannel;
    }
    return 0;
}

int wtk_kalman_cfg_update2(wtk_kalman_cfg_t *cfg, wtk_source_loader_t *sl) {
    wtk_kalman_cfg_update(cfg);
    return 0;
}

wtk_kalman_cfg_t *wtk_kalman_cfg_new(char *fn) {
    wtk_main_cfg_t *main_cfg;
    wtk_kalman_cfg_t *cfg;

    main_cfg = wtk_main_cfg_new_type(wtk_kalman_cfg, fn);
    if (!main_cfg) {
        return NULL;
    }
    cfg = (wtk_kalman_cfg_t *)main_cfg->cfg;
    cfg->main_cfg = main_cfg;
    return cfg;
}

void wtk_kalman_cfg_delete(wtk_kalman_cfg_t *cfg) {
    wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_kalman_cfg_t *wtk_kalman_cfg_new_bin(char *fn) {
    wtk_mbin_cfg_t *mbin_cfg;
    wtk_kalman_cfg_t *cfg;

    mbin_cfg = wtk_mbin_cfg_new_type(wtk_kalman_cfg, fn, "./cfg");
    if (!mbin_cfg) {
        return NULL;
    }
    cfg = (wtk_kalman_cfg_t *)mbin_cfg->cfg;
    cfg->mbin_cfg = mbin_cfg;
    return cfg;
}

void wtk_kalman_cfg_delete_bin(wtk_kalman_cfg_t *cfg) {
    wtk_mbin_cfg_delete(cfg->mbin_cfg);
}
