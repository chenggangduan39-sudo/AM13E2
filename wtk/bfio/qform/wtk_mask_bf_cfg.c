#include "wtk/bfio/qform/wtk_mask_bf_cfg.h"

int wtk_mask_bf_cfg_init(wtk_mask_bf_cfg_t *cfg) {
    cfg->main_cfg = NULL;
    cfg->mbin_cfg = NULL;

    cfg->channel = 0;
    cfg->channelx2 = 0;

    cfg->wins = 1024;
    cfg->nbin = 513;
    cfg->rate = 16000;

    cfg->clip_s = 0;
    cfg->clip_e = 8000;
    cfg->low_bin = 200;
    cfg->high_bin = 6000;
    cfg->en_low_bin = 250;
    cfg->en_high_bin = 3500;
    cfg->post_clip = 1.0;
    cfg->post_clip_e = 1.0;
    cfg->init_post_clip = 0;
    cfg->init_cnt = 100;

    cfg->zeta = 0.97;
    cfg->s_power_thresh = 0.1;
    cfg->n_power_thresh = 0.5;
    cfg->s_frame_thresh = 0.1;
    cfg->s_bin_thresh = 0.1;
    cfg->n_bin_thresh = 0.6;
    cfg->En_thresh = 19.2;
    cfg->scov_alpha = 0.01;
    cfg->ncov_alpha = 0.01;
    cfg->s_frame_thresh_e = 0.15;
    cfg->s_bin_thresh_e = 0.25;
    cfg->n_bin_thresh_e = 0.65;
    cfg->En_thresh_e = 18.9;
    cfg->scov_alpha_e = 0.01;
    cfg->ncov_alpha_e = 0.01;
    cfg->snr_thresh = 1.05;
    cfg->gamma = 1.5;
    cfg->eye = 5e-3;
    cfg->rss_iter = 1;
    cfg->update_w_cnt = 1;
    cfg->update_w_freq = NULL;

    cfg->snr_acc_tuple = NULL;
    cfg->scov_alpha_tuple = NULL;
    cfg->s_frame_thresh_tuple = NULL;
    cfg->s_bin_thresh_tuple = NULL;
    cfg->n_bin_thresh_tuple = NULL;
    cfg->En_thresh_tuple = NULL;
    cfg->snr_acc_tuple_n = 0;

    cfg->use_ref_change = 0;

    return 0;
}

int wtk_mask_bf_cfg_clean(wtk_mask_bf_cfg_t *cfg) {
    if (cfg->update_w_freq) {
        wtk_free(cfg->update_w_freq);
    }
    if (cfg->snr_acc_tuple) {
        wtk_free(cfg->snr_acc_tuple);
    }
    if (cfg->scov_alpha_tuple) {
        wtk_free(cfg->scov_alpha_tuple);
    }
    if (cfg->s_frame_thresh_tuple) {
        wtk_free(cfg->s_frame_thresh_tuple);
    }
    if (cfg->s_bin_thresh_tuple) {
        wtk_free(cfg->s_bin_thresh_tuple);
    }
    if (cfg->n_bin_thresh_tuple) {
        wtk_free(cfg->n_bin_thresh_tuple);
    }
    if (cfg->En_thresh_tuple) {
        wtk_free(cfg->En_thresh_tuple);
    }
    return 0;
}

int wtk_mask_bf_cfg_update_local(wtk_mask_bf_cfg_t *cfg, wtk_local_cfg_t *m) {
    wtk_string_t *v;
    wtk_local_cfg_t *lc;
    wtk_array_t *a;
    int ret;
    int i;

    lc = m;
    wtk_local_cfg_update_cfg_i(lc, cfg, wins, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, rate, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, channel, v);

    wtk_local_cfg_update_cfg_i(lc, cfg, clip_s, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, clip_e, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, low_bin, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, high_bin, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, en_low_bin, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, en_high_bin, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, post_clip, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, post_clip_e, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, init_post_clip, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, init_cnt, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, zeta, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, s_power_thresh, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, n_power_thresh, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, s_frame_thresh, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, s_bin_thresh, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, n_bin_thresh, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, En_thresh, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, scov_alpha, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, ncov_alpha, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, s_frame_thresh_e, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, s_bin_thresh_e, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, n_bin_thresh_e, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, En_thresh_e, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, scov_alpha_e, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, ncov_alpha_e, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, snr_thresh, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, gamma, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, eye, v);

    wtk_local_cfg_update_cfg_i(lc, cfg, rss_iter, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, update_w_cnt, v);

    wtk_local_cfg_update_cfg_b(lc, cfg, use_ref_change, v);

    a = wtk_local_cfg_find_array_s(m, "snr_acc_tuple");
    if (a) {
        cfg->snr_acc_tuple = (float *)wtk_malloc(sizeof(float) * a->nslot);
        cfg->snr_acc_tuple_n = a->nslot;
        for (i = 0; i < a->nslot; ++i) {
            v = ((wtk_string_t **)a->slot)[i];
            cfg->snr_acc_tuple[i] = wtk_str_atof(v->data, v->len);
        }
    }
    a = wtk_local_cfg_find_array_s(m, "scov_alpha_tuple");
    if (a) {
        cfg->scov_alpha_tuple = (float *)wtk_malloc(sizeof(float) * a->nslot);
        for (i = 0; i < a->nslot; ++i) {
            v = ((wtk_string_t **)a->slot)[i];
            cfg->scov_alpha_tuple[i] = wtk_str_atof(v->data, v->len);
        }
        if (cfg->snr_acc_tuple_n > 0) {
            if (cfg->snr_acc_tuple_n != a->nslot - 1) {
                wtk_debug("scov_alpha_tuple size not match snr_acc_tuple_n\n");
                ret = -1;
                goto end;
            }
        }
    }
    a = wtk_local_cfg_find_array_s(m, "s_frame_thresh_tuple");
    if (a) {
        cfg->s_frame_thresh_tuple =
            (float *)wtk_malloc(sizeof(float) * a->nslot);
        for (i = 0; i < a->nslot; ++i) {
            v = ((wtk_string_t **)a->slot)[i];
            cfg->s_frame_thresh_tuple[i] = wtk_str_atof(v->data, v->len);
        }
        if (cfg->snr_acc_tuple_n > 0) {
            if (cfg->snr_acc_tuple_n != a->nslot - 1) {
                wtk_debug(
                    "s_frame_thresh_tuple size not match snr_acc_tuple_n\n");
                ret = -1;
                goto end;
            }
        }
    }
    a = wtk_local_cfg_find_array_s(m, "s_bin_thresh_tuple");
    if (a) {
        cfg->s_bin_thresh_tuple = (float *)wtk_malloc(sizeof(float) * a->nslot);
        for (i = 0; i < a->nslot; ++i) {
            v = ((wtk_string_t **)a->slot)[i];
            cfg->s_bin_thresh_tuple[i] = wtk_str_atof(v->data, v->len);
        }
        if (cfg->snr_acc_tuple_n > 0) {
            if (cfg->snr_acc_tuple_n != a->nslot - 1) {
                wtk_debug(
                    "s_bin_thresh_tuple size not match snr_acc_tuple_n\n");
                ret = -1;
                goto end;
            }
        }
    }
    a = wtk_local_cfg_find_array_s(m, "n_bin_thresh_tuple");
    if (a) {
        cfg->n_bin_thresh_tuple = (float *)wtk_malloc(sizeof(float) * a->nslot);
        for (i = 0; i < a->nslot; ++i) {
            v = ((wtk_string_t **)a->slot)[i];
            cfg->n_bin_thresh_tuple[i] = wtk_str_atof(v->data, v->len);
        }
        if (cfg->snr_acc_tuple_n > 0) {
            if (cfg->snr_acc_tuple_n != a->nslot - 1) {
                wtk_debug(
                    "n_bin_thresh_tuple size not match snr_acc_tuple_n\n");
                ret = -1;
                goto end;
            }
        }
    }
    a = wtk_local_cfg_find_array_s(m, "En_thresh_tuple");
    if (a) {
        cfg->En_thresh_tuple = (float *)wtk_malloc(sizeof(float) * a->nslot);
        for (i = 0; i < a->nslot; ++i) {
            v = ((wtk_string_t **)a->slot)[i];
            cfg->En_thresh_tuple[i] = wtk_str_atof(v->data, v->len);
        }
        if (cfg->snr_acc_tuple_n > 0) {
            if (cfg->snr_acc_tuple_n != a->nslot - 1) {
                wtk_debug("En_thresh_tuple size not match snr_acc_tuple_n\n");
                ret = -1;
                goto end;
            }
        }
    }

    ret = 0;
end:
    return ret;
}

int wtk_mask_bf_cfg_update(wtk_mask_bf_cfg_t *cfg) {
    int ret;
    int i;

    cfg->nbin = cfg->wins / 2 + 1;
    cfg->clip_s = (cfg->clip_s * 1.0 * cfg->wins) / cfg->rate;
    cfg->clip_e = (cfg->clip_e * 1.0 * cfg->wins) / cfg->rate;
    cfg->low_bin = (cfg->low_bin * 1.0 * cfg->wins) / cfg->rate;
    cfg->high_bin = (cfg->high_bin * 1.0 * cfg->wins) / cfg->rate;
    cfg->en_low_bin = (cfg->en_low_bin * 1.0 * cfg->wins) / cfg->rate;
    cfg->en_high_bin = (cfg->en_high_bin * 1.0 * cfg->wins) / cfg->rate;
    cfg->channelx2 = cfg->channel * cfg->channel;

    cfg->update_w_freq = (int *)wtk_malloc((cfg->wins / 2 + 1) * sizeof(int));
    if (cfg->update_w_cnt == 1) {
        for (i = 0; i < cfg->wins / 2 + 1; ++i) {
            cfg->update_w_freq[i] = 1;
        }
    } else {
        int step = (cfg->wins / 2 + 1) / cfg->update_w_cnt;
        int cnt = 0;
        int idx = 1;
        for (i = 0; i < cfg->wins / 2 + 1; ++i, ++cnt) {
            if (cnt < step) {
                cfg->update_w_freq[i] = idx;
            } else if (cnt == step) {
                cfg->update_w_freq[i] = idx;
                cnt = 0;
                ++idx;
            }
        }
    }
    ret = 0;
end:
    return ret;
}

int wtk_mask_bf_cfg_update2(wtk_mask_bf_cfg_t *cfg, wtk_source_loader_t *sl) {
    int ret;

    ret = wtk_mask_bf_cfg_update(cfg);
    if (ret != 0) {
        goto end;
    }
    ret = 0;
end:
    return ret;
}

wtk_mask_bf_cfg_t *wtk_mask_bf_cfg_new(char *fn) {
    wtk_main_cfg_t *main_cfg;
    wtk_mask_bf_cfg_t *cfg;

    main_cfg = wtk_main_cfg_new_type(wtk_mask_bf_cfg, fn);
    if (!main_cfg) {
        return NULL;
    }
    cfg = (wtk_mask_bf_cfg_t *)main_cfg->cfg;
    cfg->main_cfg = main_cfg;
    return cfg;
}

void wtk_mask_bf_cfg_delete(wtk_mask_bf_cfg_t *cfg) {
    wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_mask_bf_cfg_t *wtk_mask_bf_cfg_new_bin(char *fn) {
    wtk_mbin_cfg_t *mbin_cfg;
    wtk_mask_bf_cfg_t *cfg;

    mbin_cfg = wtk_mbin_cfg_new_type(wtk_mask_bf_cfg, fn, "./cfg");
    if (!mbin_cfg) {
        return NULL;
    }
    cfg = (wtk_mask_bf_cfg_t *)mbin_cfg->cfg;
    cfg->mbin_cfg = mbin_cfg;
    return cfg;
}

void wtk_mask_bf_cfg_delete_bin(wtk_mask_bf_cfg_t *cfg) {
    wtk_mbin_cfg_delete(cfg->mbin_cfg);
}

void wtk_mask_bf_cfg_set_channel(wtk_mask_bf_cfg_t *cfg, int channel) {
    cfg->channel = channel;
    cfg->channelx2 = cfg->channel * cfg->channel;
}