#include "wtk/bfio/qform/beamnet/wtk_beam_bf_net_cfg.h"

int wtk_beam_bf_net_cfg_init(wtk_beam_bf_net_cfg_t *cfg) {

    cfg->main_cfg = NULL;
    cfg->mbin_cfg = NULL;

    wtk_mask_bf_cfg_init(&(cfg->mask_bf));
    wtk_qmmse_cfg_init(&(cfg->qmmse2));
    qtk_ahs_gain_controller_cfg_init(&(cfg->gc));
    wtk_equalizer_cfg_init(&(cfg->eq));
    wtk_limiter_cfg_init(&(cfg->limiter));
    qtk_nnrt_cfg_init(&cfg->stage1_rt);
    cfg->qnn1_fn = NULL;
    cfg->qnn1_buf = NULL;

    cfg->wins = 1024;
    cfg->nbin = 513;
    cfg->rate = 16000;
    cfg->sv = 334;

    cfg->channel = 0;
    cfg->nmicchannel = 0;
    cfg->mic_channel = NULL;
    cfg->nspchannel = 0;
    cfg->sp_channel = NULL;
    cfg->nbfchannel = 0;
    cfg->sp_main_chn = 0;
    cfg->nmic = 0;
    cfg->mic_pos = NULL;

    cfg->mic_scale = 1.0;
    cfg->sp_scale = 1.0;

    cfg->num_frame = 1;

    cfg->spenr_thresh = 100;
    cfg->spenr_cnt = 10;

    cfg->micenr_thresh = 300;
    cfg->micenr_cnt = 10;

    cfg->clip_s = 0;
    cfg->clip_e = 8000;

    cfg->cnon_clip_s = 7000;
    cfg->cnon_clip_e = 8000;

    cfg->sym = 1e-2;

    cfg->gc_gain = 50000.0;
    cfg->gc_min_thresh = 0.2;
    cfg->gc_cnt = 10;
    cfg->out_agc_level = -1;
    cfg->max_out = 32000;

    cfg->qmmse2_agc_level = NULL;
    cfg->qmmse2_max_gain = NULL;
    cfg->gc_gain_level = NULL;
    cfg->n_agc_level = 0;

    cfg->qmmse2_noise_suppress = NULL;
    cfg->n_ans_level = 0;

    cfg->qmmse2_mask_thresh = 0.1;

    cfg->model1_scale = 1.0;
    cfg->model1_sp_scale = 1.0;

    cfg->use_pffft = 1;
    cfg->use_stage1_rt = 0;
    cfg->use_qmmse2 = 0;
    cfg->use_mask_bf = 0;
    cfg->use_gc = 0;
    cfg->use_cnon = 0;
    cfg->use_bs_win = 0;
    cfg->use_eq = 0;
    cfg->use_limiter = 1;

    cfg->use_qnn = 0;
    return 0;
}

int wtk_beam_bf_net_cfg_clean(wtk_beam_bf_net_cfg_t *cfg) {
    int i;
    if (cfg->mic_channel) {
        wtk_free(cfg->mic_channel);
    }
    if (cfg->sp_channel) {
        wtk_free(cfg->sp_channel);
    }
    if (cfg->mic_pos) {
        for (i = 0; i < cfg->nmic; ++i) {
            wtk_free(cfg->mic_pos[i]);
        }
        wtk_free(cfg->mic_pos);
    }
    if (cfg->qmmse2_agc_level) {
        wtk_free(cfg->qmmse2_agc_level);
    }
    if (cfg->qmmse2_max_gain) {
        wtk_free(cfg->qmmse2_max_gain);
    }
    if (cfg->gc_gain_level) {
        wtk_free(cfg->gc_gain_level);
    }
    if (cfg->qmmse2_noise_suppress) {
        wtk_free(cfg->qmmse2_noise_suppress);
    }
    wtk_mask_bf_cfg_clean(&cfg->mask_bf);
    wtk_qmmse_cfg_clean(&(cfg->qmmse2));
    qtk_ahs_gain_controller_cfg_clean(&(cfg->gc));
    wtk_equalizer_cfg_clean(&(cfg->eq));
    wtk_limiter_cfg_clean(&(cfg->limiter));

    qtk_nnrt_cfg_clean(&cfg->stage1_rt);

    cfg->qnn1_fn = NULL;

    if (cfg->qnn1_buf) {
        wtk_strbuf_delete(cfg->qnn1_buf);
    }

    return 0;
}

int wtk_beam_bf_net_cfg_update_local(wtk_beam_bf_net_cfg_t *cfg,
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
    wtk_local_cfg_update_cfg_i(lc, cfg, nbfchannel, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, sp_main_chn, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, mic_scale, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, sp_scale, v);

    wtk_local_cfg_update_cfg_i(lc, cfg, num_frame, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, spenr_thresh, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, spenr_cnt, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, micenr_thresh, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, micenr_cnt, v);

    wtk_local_cfg_update_cfg_i(lc, cfg, clip_s, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, clip_e, v);

    wtk_local_cfg_update_cfg_i(lc, cfg, cnon_clip_s, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, cnon_clip_e, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, sym, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, gc_gain, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, gc_min_thresh, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, gc_cnt, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, out_agc_level, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, max_out, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, qmmse2_mask_thresh, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, model1_scale, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, model1_sp_scale, v);

    wtk_local_cfg_update_cfg_b(lc, cfg, use_pffft, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_stage1_rt, v);
    wtk_local_cfg_update_cfg_str(lc, cfg, qnn1_fn, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_qmmse2, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_mask_bf, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_gc, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, use_cnon, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_bs_win, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_eq, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_limiter, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_qnn, v);

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

    lc = wtk_local_cfg_find_lc_s(m, "mic");
    if (lc) {
        wtk_queue_node_t *qn;
        wtk_cfg_item_t *item;
        int i;

        cfg->mic_pos =
            (float **)wtk_malloc(sizeof(float *) * lc->cfg->queue.length);
        cfg->nmic = 0;
        for (qn = lc->cfg->queue.pop; qn; qn = qn->next) {
            item = data_offset2(qn, wtk_cfg_item_t, n);
            if (item->type != WTK_CFG_ARRAY || item->value.array->nslot != 3) {
                continue;
            }
            cfg->mic_pos[cfg->nmic] = (float *)wtk_malloc(sizeof(float) * 3);
            for (i = 0; i < 3; ++i) {
                v = ((wtk_string_t **)item->value.array->slot)[i];
                cfg->mic_pos[cfg->nmic][i] = wtk_str_atof(v->data, v->len);
                // wtk_debug("v[%d][%d]=%f\n",cfg->nmic,i,cfg->mic_pos[cfg->nmic][i]);
            }
            ++cfg->nmic;
        }
        if (cfg->nmic != cfg->nmicchannel) {
            wtk_debug("error: nmic=%d!=nmicchannel=%d\n", cfg->nmic,
                      cfg->nmicchannel);
            exit(1);
        }
    }

    lc = wtk_local_cfg_find_lc_s(m, "qmmse2");
    if (lc) {
        ret = wtk_qmmse_cfg_update_local(&(cfg->qmmse2), lc);
        cfg->qmmse2.step = cfg->wins / 2;
        if (ret != 0) {
            goto end;
        }
    }

    lc = wtk_local_cfg_find_lc_s(m, "mask_bf");
    if (lc) {
        ret = wtk_mask_bf_cfg_update_local(&(cfg->mask_bf), lc);
        if (ret != 0) {
            goto end;
        }
    }
    lc = wtk_local_cfg_find_lc_s(m, "gc");
    if (lc) {
        ret = qtk_ahs_gain_controller_cfg_update_local(&(cfg->gc), lc);
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

    lc = wtk_local_cfg_find_lc_s(m, "limiter");
    if (lc) {
        ret = wtk_limiter_cfg_update_local(&(cfg->limiter), lc);
        if (ret != 0) {
            goto end;
        }
    }

    a = wtk_local_cfg_find_array_s(m, "qmmse2_agc_level");
    if (a) {
        cfg->qmmse2_agc_level = (float *)wtk_malloc(sizeof(float) * a->nslot);
        for (i = 0; i < a->nslot; ++i) {
            v = ((wtk_string_t **)a->slot)[i];
            cfg->qmmse2_agc_level[i] = wtk_str_atof(v->data, v->len);
        }
        if (cfg->n_agc_level > 0) {
            if (cfg->n_agc_level != a->nslot) {
                wtk_debug("qmmse2_agc_level size not match n_agc_level\n");
                ret = -1;
                goto end;
            }
        } else {
            cfg->n_agc_level = a->nslot;
        }
    }
    a = wtk_local_cfg_find_array_s(m, "qmmse2_max_gain");
    if (a) {
        cfg->qmmse2_max_gain = (float *)wtk_malloc(sizeof(float) * a->nslot);
        for (i = 0; i < a->nslot; ++i) {
            v = ((wtk_string_t **)a->slot)[i];
            cfg->qmmse2_max_gain[i] = wtk_str_atof(v->data, v->len);
        }
        if (cfg->n_agc_level > 0) {
            if (cfg->n_agc_level != a->nslot) {
                wtk_debug("qmmse2_max_gain size not match n_agc_level\n");
                ret = -1;
                goto end;
            }
        } else {
            cfg->n_agc_level = a->nslot;
        }
    }
    a = wtk_local_cfg_find_array_s(m, "gc_gain_level");
    if (a) {
        cfg->gc_gain_level = (float *)wtk_malloc(sizeof(float) * a->nslot);
        for (i = 0; i < a->nslot; ++i) {
            v = ((wtk_string_t **)a->slot)[i];
            cfg->gc_gain_level[i] = wtk_str_atof(v->data, v->len);
        }
        if (cfg->n_agc_level > 0) {
            if (cfg->n_agc_level != a->nslot) {
                wtk_debug("gc_gain_level size not match n_agc_level\n");
                ret = -1;
                goto end;
            }
        } else {
            cfg->n_agc_level = a->nslot;
        }
    }

    a = wtk_local_cfg_find_array_s(m, "qmmse2_noise_suppress");
    if (a) {
        cfg->qmmse2_noise_suppress =
            (float *)wtk_malloc(sizeof(float) * a->nslot);
        for (i = 0; i < a->nslot; ++i) {
            v = ((wtk_string_t **)a->slot)[i];
            cfg->qmmse2_noise_suppress[i] = wtk_str_atof(v->data, v->len);
        }
        if (cfg->n_ans_level > 0) {
            if (cfg->n_ans_level != a->nslot) {
                wtk_debug("qmmse2_noise_suppress size not match n_ans_level\n");
                ret = -1;
                goto end;
            }
        } else {
            cfg->n_ans_level = a->nslot;
        }
    }

    lc = wtk_local_cfg_find_lc_s(m, "stage1_rt");
    if (lc) {
        ret = qtk_nnrt_cfg_update_local(&cfg->stage1_rt, lc);
        if (ret != 0) {
            goto end;
        }
    }
    ret = 0;
end:
    return ret;
}

int wtk_beam_bf_net_cfg_load_qnn_res(wtk_strbuf_t *buf, wtk_source_t *src) {
    if (buf) {
        wtk_source_read_file2(src, buf);
        return 0;
    }
    return -1;
}

void wtk_beam_bf_net_cfg_set(wtk_beam_bf_net_cfg_t *cfg) {
    int i;

    cfg->nbin = cfg->wins / 2 + 1;
    if (cfg->channel < cfg->nmicchannel + cfg->nspchannel) {
        cfg->channel = cfg->nmicchannel + cfg->nspchannel;
    }
    if (cfg->nbfchannel == 0) {
        cfg->nbfchannel = cfg->nmicchannel;
    }
    cfg->out_channels = cfg->nmicchannel * (cfg->nmicchannel - 1) / 2;
    if (cfg->use_mask_bf) {
        wtk_mask_bf_cfg_set_channel(&(cfg->mask_bf), cfg->nbfchannel);
    }

    cfg->clip_s = (cfg->clip_s * 1.0 * cfg->wins) / cfg->rate;
    cfg->clip_e = (cfg->clip_e * 1.0 * cfg->wins) / cfg->rate;
    cfg->cnon_clip_s = (cfg->cnon_clip_s * 1.0 * cfg->wins) / cfg->rate;
    cfg->cnon_clip_e = (cfg->cnon_clip_e * 1.0 * cfg->wins) / cfg->rate;

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

    if (cfg->use_limiter) {
        cfg->limiter.max_amp = cfg->max_out;
    }

    if (cfg->n_agc_level == 0) {
        cfg->n_agc_level = 6;
    }
    if (cfg->gc_gain_level == NULL) {
        cfg->gc_gain_level =
            (float *)wtk_malloc(sizeof(float) * cfg->n_agc_level);
        float tmp_gc_gain_level[6] = {6250,  12500,  25000,
                                      50000, 100000, 200000};
        for (i = 0; i < cfg->n_agc_level; ++i) {
            if (i < 6) {
                cfg->gc_gain_level[i] = tmp_gc_gain_level[i];
            } else {
                cfg->gc_gain_level[i] = tmp_gc_gain_level[5];
            }
        }
    }
    if (cfg->qmmse2_agc_level == NULL) {
        cfg->qmmse2_agc_level =
            (float *)wtk_malloc(sizeof(float) * cfg->n_agc_level);
        float tmp_qmmse2_agc_level[6] = {4200, 6000, 9000, 13500, 20000, 28000};
        for (i = 0; i < cfg->n_agc_level; ++i) {
            if (i < 6) {
                cfg->qmmse2_agc_level[i] = tmp_qmmse2_agc_level[i];
            } else {
                cfg->qmmse2_agc_level[i] = tmp_qmmse2_agc_level[5];
            }
        }
    }
    if (cfg->qmmse2_max_gain == NULL) {
        cfg->qmmse2_max_gain =
            (float *)wtk_malloc(sizeof(float) * cfg->n_agc_level);
        float tmp_qmmse2_max_gain[6] = {25, 25, 25, 35, 35, 45};
        for (i = 0; i < cfg->n_agc_level; ++i) {
            if (i < 6) {
                cfg->qmmse2_max_gain[i] = tmp_qmmse2_max_gain[i];
            } else {
                cfg->qmmse2_max_gain[i] = tmp_qmmse2_max_gain[5];
            }
        }
    }

    if (cfg->n_ans_level == 0) {
        cfg->n_ans_level = 6;
    }
    if (cfg->qmmse2_noise_suppress == NULL) {
        cfg->qmmse2_noise_suppress =
            (float *)wtk_malloc(sizeof(float) * cfg->n_ans_level);
        float tmp_qmmse2_noise_suppress[6] = {0.0,   -5.0,  -15.0,
                                              -25.0, -35.0, -100.0};
        for (i = 0; i < cfg->n_ans_level; ++i) {
            if (i < 6) {
                cfg->qmmse2_noise_suppress[i] = tmp_qmmse2_noise_suppress[i];
            } else {
                cfg->qmmse2_noise_suppress[i] = tmp_qmmse2_noise_suppress[5];
            }
        }
    }
}

int wtk_beam_bf_net_cfg_update(wtk_beam_bf_net_cfg_t *cfg) {
    int ret;
    int i;

    wtk_beam_bf_net_cfg_set(cfg);
    ret = wtk_mask_bf_cfg_update(&(cfg->mask_bf));
    if (ret != 0) {
        goto end;
    }
    ret = wtk_qmmse_cfg_update(&(cfg->qmmse2));
    if (ret != 0) {
        goto end;
    }
    ret = qtk_ahs_gain_controller_cfg_update(&(cfg->gc));
    if (ret != 0) {
        goto end;
    }
    ret = wtk_equalizer_cfg_update(&(cfg->eq));
    if (ret != 0) {
        goto end;
    }

    if (cfg->use_qnn) {
        wtk_source_loader_t sl;
        sl.hook = 0;
        sl.vf = wtk_source_load_file_v;
        if (cfg->qnn1_fn) {
            cfg->qnn1_buf = wtk_strbuf_new(1024, 1);
            ret = wtk_source_loader_load(
                &sl, cfg->qnn1_buf,
                (wtk_source_load_handler_t)wtk_beam_bf_net_cfg_load_qnn_res,
                cfg->qnn1_fn);
        }
    }

    if (cfg->use_stage1_rt) {
        qtk_nnrt_cfg_update(&cfg->stage1_rt);
    }

    ret = 0;
end:
    return ret;
}

int wtk_beam_bf_net_cfg_update2(wtk_beam_bf_net_cfg_t *cfg,
                                wtk_source_loader_t *sl) {
    int ret;
    int i;

    wtk_beam_bf_net_cfg_set(cfg);
    ret = wtk_mask_bf_cfg_update(&(cfg->mask_bf));
    if (ret != 0) {
        goto end;
    }
    ret = wtk_qmmse_cfg_update(&(cfg->qmmse2));
    if (ret != 0) {
        goto end;
    }
    ret = qtk_ahs_gain_controller_cfg_update(&(cfg->gc));
    if (ret != 0) {
        goto end;
    }
    ret = wtk_equalizer_cfg_update(&(cfg->eq));
    if (ret != 0) {
        goto end;
    }

    if (cfg->use_qnn) {
        if (cfg->qnn1_fn) {
            cfg->qnn1_buf = wtk_strbuf_new(1024, 1);
            ret = wtk_source_loader_load(
                sl, cfg->qnn1_buf,
                (wtk_source_load_handler_t)wtk_beam_bf_net_cfg_load_qnn_res,
                cfg->qnn1_fn);
        }
    }

    if (cfg->use_stage1_rt) {
        qtk_nnrt_cfg_update2(&cfg->stage1_rt, sl);
    }

    ret = 0;
end:
    return ret;
}

wtk_beam_bf_net_cfg_t *wtk_beam_bf_net_cfg_new(char *fn) {
    wtk_main_cfg_t *main_cfg;
    wtk_beam_bf_net_cfg_t *cfg;

    main_cfg = wtk_main_cfg_new_type(wtk_beam_bf_net_cfg, fn);
    if (!main_cfg) {
        return NULL;
    }
    cfg = (wtk_beam_bf_net_cfg_t *)main_cfg->cfg;
    cfg->main_cfg = main_cfg;
    return cfg;
}

void wtk_beam_bf_net_cfg_delete(wtk_beam_bf_net_cfg_t *cfg) {
    wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_beam_bf_net_cfg_t *wtk_beam_bf_net_cfg_new_bin(char *fn) {
    wtk_mbin_cfg_t *mbin_cfg;
    wtk_beam_bf_net_cfg_t *cfg;

    mbin_cfg = wtk_mbin_cfg_new_type(wtk_beam_bf_net_cfg, fn, "./cfg");
    if (!mbin_cfg) {
        return NULL;
    }
    cfg = (wtk_beam_bf_net_cfg_t *)mbin_cfg->cfg;
    cfg->mbin_cfg = mbin_cfg;
    return cfg;
}

void wtk_beam_bf_net_cfg_delete_bin(wtk_beam_bf_net_cfg_t *cfg) {
    wtk_mbin_cfg_delete(cfg->mbin_cfg);
}
