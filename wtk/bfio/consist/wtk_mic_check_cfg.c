#include "wtk/bfio/consist/wtk_mic_check_cfg.h"

int wtk_mic_check_cfg_init(wtk_mic_check_cfg_t *cfg) {

    cfg->main_cfg = NULL;
    cfg->mbin_cfg = NULL;

    cfg->wins = 1024;
    cfg->rate = 16000;
    cfg->sv = 334;

    cfg->channel = 0;
    cfg->nmicchannel = 0;
    cfg->mic_channel = NULL;
    cfg->nspchannel = 0;
    cfg->sp_channel = NULL;

    cfg->mic_scale = 1.0;
    cfg->sp_scale = 1.0;

    cfg->spenr_thresh = 100;
    cfg->spenr_cnt = 10;

    cfg->micenr_thresh = 300;
    cfg->micenr_cnt = 10;

    cfg->clip_s = 0;
    cfg->clip_e = 8000;

    cfg->alpha = 0.9;
    cfg->eng_thresh = 0.0;
    cfg->var_thresh = 0.0;
    cfg->sc_thresh = 0.0;
    cfg->eng_thresh2 = 0.0;
    cfg->var_thresh2 = 0.0;
    cfg->sc_thresh2 = 0.0;
    cfg->rcd_thresh = 0.9;
    cfg->play_thresh = 0.5;
    cfg->play_thresh2 = 0.9;

    cfg->type = WTK_MIC_CHECK_TYPE_NONE;

    cfg->use_pffft = 1;
    cfg->use_fft = 0;
    return 0;
}

int wtk_mic_check_cfg_clean(wtk_mic_check_cfg_t *cfg) {
    if (cfg->mic_channel) {
        wtk_free(cfg->mic_channel);
    }
    if (cfg->sp_channel) {
        wtk_free(cfg->sp_channel);
    }

    return 0;
}

int wtk_mic_check_cfg_update(wtk_mic_check_cfg_t *cfg) {
    int ret;

    if (cfg->channel < cfg->nmicchannel + cfg->nspchannel) {
        cfg->channel = cfg->nmicchannel + cfg->nspchannel;
    }

    cfg->clip_s = (cfg->clip_s * 1.0 * cfg->wins) / cfg->rate;
    cfg->clip_e = (cfg->clip_e * 1.0 * cfg->wins) / cfg->rate;

    ret = 0;
end:
    return ret;
}

int wtk_mic_check_cfg_update2(wtk_mic_check_cfg_t *cfg,
                                wtk_source_loader_t *sl) {
    int ret;

    if (cfg->channel < cfg->nmicchannel + cfg->nspchannel) {
        cfg->channel = cfg->nmicchannel + cfg->nspchannel;
    }

    cfg->clip_s = (cfg->clip_s * 1.0 * cfg->wins) / cfg->rate;
    cfg->clip_e = (cfg->clip_e * 1.0 * cfg->wins) / cfg->rate;

    ret = 0;
end:
    return ret;
}

int wtk_mic_check_cfg_update_local(wtk_mic_check_cfg_t *cfg,
                                     wtk_local_cfg_t *m) {
    wtk_string_t *v;
    wtk_local_cfg_t *lc;
    int ret;
    wtk_array_t *a;
    int i;

    lc = m;
    wtk_local_cfg_update_cfg_i(lc, cfg, wins, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, rate, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, sv, v);

    wtk_local_cfg_update_cfg_i(lc, cfg, channel, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, mic_scale, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, sp_scale, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, spenr_thresh, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, spenr_cnt, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, micenr_thresh, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, micenr_cnt, v);

    wtk_local_cfg_update_cfg_i(lc, cfg, clip_s, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, clip_e, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, alpha, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, eng_thresh, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, var_thresh, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, sc_thresh, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, eng_thresh2, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, var_thresh2, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, sc_thresh2, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, rcd_thresh, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, play_thresh, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, play_thresh2, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, type, v);

    wtk_local_cfg_update_cfg_b(lc, cfg, use_pffft, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_fft, v);

    a = wtk_local_cfg_find_array_s(m, "mic_channel");
    if (a) {
        cfg->mic_channel = (int *)wtk_malloc(sizeof(int) * a->nslot);
        cfg->nmicchannel = a->nslot;
        for (i = 0; i < a->nslot; ++i) {
            v = ((wtk_string_t **)a->slot)[i];
            cfg->mic_channel[i] = wtk_str_atoi(v->data, v->len);
        }
    }

    a = wtk_local_cfg_find_array_s(m, "sp_channel");
    if (a) {
        cfg->sp_channel = (int *)wtk_malloc(sizeof(int) * a->nslot);
        cfg->nspchannel = a->nslot;
        for (i = 0; i < a->nslot; ++i) {
            v = ((wtk_string_t **)a->slot)[i];
            cfg->sp_channel[i] = wtk_str_atoi(v->data, v->len);
        }
    }

    ret = 0;
end:
    return ret;
}

wtk_mic_check_cfg_t *wtk_mic_check_cfg_new(char *fn) {
    wtk_main_cfg_t *main_cfg;
    wtk_mic_check_cfg_t *cfg;

    main_cfg = wtk_main_cfg_new_type(wtk_mic_check_cfg, fn);
    if (!main_cfg) {
        return NULL;
    }
    cfg = (wtk_mic_check_cfg_t *)main_cfg->cfg;
    cfg->main_cfg = main_cfg;
    return cfg;
}

void wtk_mic_check_cfg_delete(wtk_mic_check_cfg_t *cfg) {
    wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_mic_check_cfg_t *wtk_mic_check_cfg_new_bin(char *fn) {
    wtk_mbin_cfg_t *mbin_cfg;
    wtk_mic_check_cfg_t *cfg;

    mbin_cfg = wtk_mbin_cfg_new_type(wtk_mic_check_cfg, fn, "./cfg");
    if (!mbin_cfg) {
        return NULL;
    }
    cfg = (wtk_mic_check_cfg_t *)mbin_cfg->cfg;
    cfg->mbin_cfg = mbin_cfg;
    return cfg;
}

void wtk_mic_check_cfg_delete_bin(wtk_mic_check_cfg_t *cfg) {
    wtk_mbin_cfg_delete(cfg->mbin_cfg);
}
