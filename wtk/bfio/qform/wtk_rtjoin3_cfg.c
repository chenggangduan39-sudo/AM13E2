#include "wtk_rtjoin3_cfg.h"

int wtk_rtjoin3_cfg_init(wtk_rtjoin3_cfg_t *cfg) {
    cfg->channel = 0;
    cfg->nmicchannel = 0;
    cfg->mic_channel = NULL;
    cfg->wins = 512;
    cfg->nbin = 257;

    cfg->main_cfg = NULL;
    cfg->mbin_cfg = NULL;

    cfg->rate = 16000;

    cfg->clip_s = 0;
    cfg->clip_e = 8000;

    cfg->micenr_thresh = 100;
    cfg->micenr_cnt = 10;

    wtk_qmmse_cfg_init(&(cfg->qmmse));

    cfg->max_out = 32700.0;
    cfg->out_agc_level = -1;

    wtk_equalizer_cfg_init(&(cfg->eq));
    cfg->use_eq = 0;
    cfg->limiter_thresh = 0.9;

    cfg->use_control_bs = 0;

    cfg->use_mul_out = 0;
    cfg->use_bs_win = 0;
    cfg->use_qmmse = 0;
    cfg->use_limiter = 0;

    return 0;
}

int wtk_rtjoin3_cfg_clean(wtk_rtjoin3_cfg_t *cfg) {
    if (cfg->mic_channel) {
        wtk_free(cfg->mic_channel);
    }
    wtk_qmmse_cfg_clean(&(cfg->qmmse));
    wtk_equalizer_cfg_clean(&(cfg->eq));

    return 0;
}

int wtk_rtjoin3_cfg_update_local(wtk_rtjoin3_cfg_t *cfg, wtk_local_cfg_t *m) {
    wtk_string_t *v;
    wtk_local_cfg_t *lc;
    int ret;
    wtk_array_t *a;
    int i;

    lc = m;
    wtk_local_cfg_update_cfg_i(lc, cfg, wins, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, channel, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, rate, v);

    wtk_local_cfg_update_cfg_i(lc, cfg, clip_s, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, clip_e, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, micenr_thresh, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, micenr_cnt, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, max_out, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, limiter_thresh, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, out_agc_level, v);

    wtk_local_cfg_update_cfg_b(lc, cfg, use_eq, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_control_bs, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_mul_out, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_bs_win, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_qmmse, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_limiter, v);

    a = wtk_local_cfg_find_array_s(lc, "mic_channel");
    if (a) {
        cfg->mic_channel = (int *)wtk_malloc(sizeof(int) * a->nslot);
        cfg->nmicchannel = a->nslot;
        for (i = 0; i < a->nslot; ++i) {
            v = ((wtk_string_t **)a->slot)[i];
            cfg->mic_channel[i] = wtk_str_atoi(v->data, v->len);
        }
    }
    lc = wtk_local_cfg_find_lc_s(m, "qmmse");
    if (lc) {
        ret = wtk_qmmse_cfg_update_local(&(cfg->qmmse), lc);
        cfg->qmmse.step = cfg->wins / 2;
        if (ret != 0) {
            goto end;
        }
    }
    lc = wtk_local_cfg_find_lc_s(m, "eq");
    if (lc) {
        ret = wtk_equalizer_cfg_update_local(&(cfg->eq), lc);
        if (ret != 0) {
            goto end;
        }
    }
    ret = 0;
end:
    return ret;
}

int wtk_rtjoin3_cfg_update(wtk_rtjoin3_cfg_t *cfg) {
    int ret;

    cfg->nbin = cfg->wins / 2 + 1;
    if (cfg->channel < cfg->nmicchannel) {
        cfg->channel = cfg->nmicchannel;
    }
    cfg->clip_s = (cfg->clip_s * 1.0 * cfg->wins) / cfg->rate;
    cfg->clip_e = (cfg->clip_e * 1.0 * cfg->wins) / cfg->rate;

    ret = wtk_qmmse_cfg_update(&(cfg->qmmse));
    if (ret != 0) {
        goto end;
    }
    ret = wtk_equalizer_cfg_update(&(cfg->eq));
    if (ret != 0) {
        goto end;
    }

    if (cfg->out_agc_level == 1) {
        cfg->max_out = pow(10, 75.0 / 20.0);
    } else if (cfg->out_agc_level == 2) {
        cfg->max_out = pow(10, 78.0 / 20.0);
    } else if (cfg->out_agc_level == 3) {
        cfg->max_out = pow(10, 81.0 / 20.0);
    } else if (cfg->out_agc_level == 4) {
        cfg->max_out = pow(10, 84.0 / 20.0);
    } else if (cfg->out_agc_level == 5) {
        cfg->max_out = pow(10, 87.0 / 20.0);
    } else if (cfg->out_agc_level >= 6) {
        cfg->max_out = pow(10, 90.0 / 20.0);
    }
    ret = 0;
end:
    return ret;
}

int wtk_rtjoin3_cfg_update2(wtk_rtjoin3_cfg_t *cfg, wtk_source_loader_t *sl) {
    return wtk_rtjoin3_cfg_update(cfg);
}

wtk_rtjoin3_cfg_t *wtk_rtjoin3_cfg_new(char *fn) {
    wtk_main_cfg_t *main_cfg;
    wtk_rtjoin3_cfg_t *cfg;

    main_cfg = wtk_main_cfg_new_type(wtk_rtjoin3_cfg, fn);
    if (!main_cfg) {
        return NULL;
    }
    cfg = (wtk_rtjoin3_cfg_t *)main_cfg->cfg;
    cfg->main_cfg = main_cfg;
    return cfg;
}

void wtk_rtjoin3_cfg_delete(wtk_rtjoin3_cfg_t *cfg) {
    wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_rtjoin3_cfg_t *wtk_rtjoin3_cfg_new_bin(char *fn) {
    wtk_mbin_cfg_t *mbin_cfg;
    wtk_rtjoin3_cfg_t *cfg;

    mbin_cfg = wtk_mbin_cfg_new_type(wtk_rtjoin3_cfg, fn, "./cfg");
    if (!mbin_cfg) {
        return NULL;
    }
    cfg = (wtk_rtjoin3_cfg_t *)mbin_cfg->cfg;
    cfg->mbin_cfg = mbin_cfg;
    return cfg;
}

void wtk_rtjoin3_cfg_delete_bin(wtk_rtjoin3_cfg_t *cfg) {
    wtk_mbin_cfg_delete(cfg->mbin_cfg);
}