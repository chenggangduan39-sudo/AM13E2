#include "wtk/bfio/maskbfnet/wtk_mask_bf_net_cfg.h"

int wtk_mask_bf_net_cfg_init(wtk_mask_bf_net_cfg_t *cfg) {

    cfg->main_cfg = NULL;
    cfg->mbin_cfg = NULL;

    wtk_rls3_cfg_init(&(cfg->echo_rls3));
    wtk_covm_cfg_init(&(cfg->covm));
    wtk_covm_cfg_init(&(cfg->echo_covm));
    wtk_bf_cfg_init(&(cfg->bf));
    wtk_bankfeat_cfg_init(&(cfg->bankfeat));
    wtk_bankfeat_cfg_init(&(cfg->bankfeat2));
    wtk_qmmse_cfg_init(&(cfg->qmmse));
    wtk_qmmse_cfg_init(&(cfg->qmmse2));
    cfg->aecmdl_fn = NULL;
    cfg->aecmdl_fn2 = NULL;
    cfg->gainnet2 = NULL;
    cfg->gainnet2_2 = NULL;
    qtk_ahs_gain_controller_cfg_init(&(cfg->gc));
    wtk_equalizer_cfg_init(&(cfg->eq));
    wtk_limiter_cfg_init(&(cfg->limiter));
    qtk_nnrt_cfg_init(&cfg->stage1_rt);
    qtk_nnrt_cfg_init(&cfg->stage2_rt);
    qtk_nnrt_cfg_init(&cfg->dr_stage2_rt);
    cfg->qnn1_fn = NULL;
    cfg->qnn2_fn = NULL;
    cfg->qnn3_fn = NULL;
    cfg->qnn1_buf = NULL;
    cfg->qnn2_buf = NULL;
    cfg->qnn3_buf = NULL;

    cfg->wins = 1024;
    cfg->rate = 16000;
    cfg->sv = 334;

    cfg->channel = 0;
    cfg->nmicchannel = 0;
    cfg->mic_channel = NULL;
    cfg->nspchannel = 0;
    cfg->sp_channel = NULL;
    cfg->nbfchannel = 0;
    cfg->sp_main_chn = 0;

    cfg->mic_scale = 1.0;
    cfg->sp_scale = 1.0;

    cfg->num_frame = 1;
    cfg->rss_iter = 5;
    cfg->update_w_cnt = 1;

    cfg->update_w_freq = NULL;

    cfg->spenr_thresh = 100;
    cfg->spenr_cnt = 10;

    cfg->micenr_thresh = 300;
    cfg->micenr_cnt = 10;

    cfg->clip_s = 0;
    cfg->clip_e = 8000;
    cfg->pre_pow_ratio = 2.0;
    cfg->pre_mul_ratio = 2.0;
    cfg->pre_clip_s = 4000;
    cfg->pre_clip_e = 8000;

    cfg->entropy_thresh = -1;
    cfg->entropy_sp_thresh = -1;
    cfg->entropy_in_cnt = 2;
    cfg->entropy_cnt = 20;

    cfg->bfmu = 1;
    cfg->bfmu2 = 1;
    cfg->echo_bfmu = 1;
    cfg->echo_bfmu2 = 1;
    cfg->bf_clip_s = 0;
    cfg->bf_clip_e = 8000;

    cfg->featm_lm = 1;
    cfg->featsp_lm = 1;
    cfg->featm_lm2 = 1;
    cfg->featsp_lm2 = 1;

    cfg->cnon_clip_s = 7000;
    cfg->cnon_clip_e = 8000;

    cfg->sym = 1e-2;

    cfg->scov_alpha = 0.6;
    cfg->ncov_alpha = 0.01;
    cfg->init_covnf = 100;
    cfg->mu_entropy_thresh = 20;

    cfg->gc_gain = 50000.0;
    cfg->gc_min_thresh = 0.2;
    cfg->gc_cnt = 10;
    cfg->out_agc_level = -1;
    cfg->max_out = 32000;

    cfg->max_mask = 1.0;
    cfg->mask_peak = 0;
    cfg->sp_max_mask = 1.0;
    cfg->sp_mask_peak = 0;

    cfg->all_scov_alpha = NULL;
    cfg->eta_thresh = NULL;
    cfg->scov_alpha_idx = NULL;
    cfg->scov_alpha_n = 0;

    cfg->qmmse2_agc_level = NULL;
    cfg->qmmse2_max_gain = NULL;
    cfg->gc_gain_level = NULL;
    cfg->n_agc_level = 0;

    cfg->qmmse2_noise_suppress = NULL;
    cfg->n_ans_level = 0;

    cfg->min_mask = 0.5;
    cfg->eta_mean_thresh = 0.6;
    cfg->scov_entropy_thresh = 20.0;
    cfg->echo_scov_entropy_thresh = 18.5;
    cfg->epsi_thresh = 0.5;
    cfg->eta_clip_s = 0;
    cfg->eta_clip_e = 8000;

    cfg->ntheta = 8;
    cfg->sdb_alpha = 0;
    cfg->n_mask_mu = 4;

    cfg->mu_t_alpha = 0.5;
    cfg->mu_f_alpha = 0.8;
    cfg->mu_mask_s = 0;
    cfg->mu_mask_e = 1000;
    cfg->bf2_alpha = 1.0;
    cfg->echo_bf2_alpha = 1.0;
    cfg->bf_change_frame = 20;

    cfg->qmmse2_mask_thresh = 0.1;

    cfg->bf_init_frame = 5;
    cfg->vec_init = 1.0;

    cfg->beta = 0.999;
    cfg->gamma = 1.2;
    cfg->zeta = 0.97;
    cfg->bin_speech_thresh = 0.5;
    cfg->bin_noise_thresh = 0.7;
    cfg->frame_speech_thresh = 0.1;
    cfg->frame_noise_thresh = 0.7;
    cfg->scnt_thresh = 0.1;
    cfg->ncnt_thresh = 0.5;
    cfg->post_clip = 0.5;

    cfg->model1_scale = 1.0;
    cfg->model1_sp_scale = 1.0;
    cfg->model2_scale = 1.0;
    cfg->model2_sp_scale = 1.0;

    cfg->use_pffft = 1;
    cfg->use_rls3 = 0;
    cfg->use_stage1_rt = 0;
    cfg->use_edr_stage1_rt = 0;
    cfg->use_freq_preemph = 0;
    cfg->use_stage2_rt = 0;
    cfg->use_dr_stage2_rt = 0;
    cfg->use_mask_model = 0;
    cfg->use_qmmse = 1;
    cfg->use_qmmse2 = 0;
    cfg->use_gainnet2 = 0;
    cfg->use_gainnet2_2 = 0;
    cfg->use_ccm = 0;
    cfg->use_bf = 0;
    cfg->use_bf_v2 = 0;
    cfg->use_echo_bf = 0;
    cfg->use_echocovm = 1;
    cfg->use_gc = 0;
    cfg->use_cnon = 0;
    cfg->use_bs_win = 0;
    cfg->use_eq = 0;
    cfg->use_rbin_res = 0;
    cfg->use_median_chn = 0;
    cfg->use_phase_corr = 0;
    cfg->use_ds = 0;
    cfg->use_debug = 0;
    cfg->use_debug_echo_model = 0;
    cfg->use_limiter = 1;
    cfg->use_raw_add = 0;

    cfg->use_qnn = 0;
    return 0;
}

int wtk_mask_bf_net_cfg_clean(wtk_mask_bf_net_cfg_t *cfg) {
    if (cfg->mic_channel) {
        wtk_free(cfg->mic_channel);
    }
    if (cfg->sp_channel) {
        wtk_free(cfg->sp_channel);
    }
    if (cfg->update_w_freq) {
        wtk_free(cfg->update_w_freq);
    }
    if (cfg->all_scov_alpha) {
        wtk_free(cfg->all_scov_alpha);
    }
    if (cfg->eta_thresh) {
        wtk_free(cfg->eta_thresh);
    }
    if (cfg->scov_alpha_idx) {
        wtk_free(cfg->scov_alpha_idx);
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
    if (cfg->gainnet2) {
        if (cfg->use_rbin_res) {
            wtk_gainnet2_cfg_delete_bin3(cfg->gainnet2);
        } else {
            wtk_gainnet2_cfg_delete_bin2(cfg->gainnet2);
        }
    }
    if (cfg->gainnet2_2) {
        if (cfg->use_rbin_res) {
            wtk_gainnet2_cfg_delete_bin3(cfg->gainnet2_2);
        } else {
            wtk_gainnet2_cfg_delete_bin2(cfg->gainnet2_2);
        }
    }
    wtk_rls3_cfg_clean(&(cfg->echo_rls3));
    wtk_covm_cfg_clean(&(cfg->covm));
    wtk_covm_cfg_clean(&(cfg->echo_covm));
    wtk_bf_cfg_clean(&cfg->bf);
    wtk_qmmse_cfg_clean(&(cfg->qmmse));
    wtk_qmmse_cfg_clean(&(cfg->qmmse2));
    wtk_bankfeat_cfg_clean(&(cfg->bankfeat));
    wtk_bankfeat_cfg_clean(&(cfg->bankfeat2));
    qtk_ahs_gain_controller_cfg_clean(&(cfg->gc));
    wtk_equalizer_cfg_clean(&(cfg->eq));
    wtk_limiter_cfg_clean(&(cfg->limiter));

    qtk_nnrt_cfg_clean(&cfg->stage1_rt);
    qtk_nnrt_cfg_clean(&cfg->stage2_rt);
    qtk_nnrt_cfg_clean(&cfg->dr_stage2_rt);

    cfg->qnn1_fn = NULL;

    if (cfg->qnn1_buf) {
        wtk_strbuf_delete(cfg->qnn1_buf);
    }
    if (cfg->qnn2_buf) {
        wtk_strbuf_delete(cfg->qnn2_buf);
    }
    if (cfg->qnn3_buf) {
        wtk_strbuf_delete(cfg->qnn3_buf);
    }

    return 0;
}

int wtk_mask_bf_net_cfg_load_qnn_res(wtk_strbuf_t *buf, wtk_source_t *src) {
    if (buf) {
        wtk_source_read_file2(src, buf);
        return 0;
    }
    return -1;
}

int wtk_mask_bf_net_cfg_update(wtk_mask_bf_net_cfg_t *cfg) {
    int ret;
    int i;

    if (cfg->channel < cfg->nmicchannel + cfg->nspchannel) {
        cfg->channel = cfg->nmicchannel + cfg->nspchannel;
    }
    if (cfg->nbfchannel == 0) {
        cfg->nbfchannel = cfg->nmicchannel;
    }

    if (cfg->use_gainnet2) {
        if (cfg->aecmdl_fn) {
            cfg->gainnet2 = wtk_gainnet2_cfg_new_bin2(cfg->aecmdl_fn);
            if (!cfg->gainnet2) {
                ret = -1;
                goto end;
            }
        }
    }
    if (cfg->use_gainnet2_2) {
        if (cfg->aecmdl_fn2) {
            cfg->gainnet2_2 = wtk_gainnet2_cfg_new_bin2(cfg->aecmdl_fn2);
            if (!cfg->gainnet2_2) {
                ret = -1;
                goto end;
            }
        }
    }
    if (cfg->use_rls3) {
        if (cfg->use_echo_bf) {
            cfg->echo_rls3.channel = cfg->nbfchannel;
        } else {
            cfg->echo_rls3.channel = 1;
        }
        cfg->echo_rls3.N = cfg->nspchannel;
        ret = wtk_rls3_cfg_update(&(cfg->echo_rls3));
        if (ret != 0) {
            goto end;
        }
    }
    ret = wtk_covm_cfg_update(&(cfg->covm));
    if (ret != 0) {
        goto end;
    }
    ret = wtk_covm_cfg_update(&(cfg->echo_covm));
    if (ret != 0) {
        goto end;
    }
    ret = wtk_bf_cfg_update(&(cfg->bf));
    if (ret != 0) {
        goto end;
    }
    ret = wtk_bankfeat_cfg_update(&(cfg->bankfeat));
    if (ret != 0) {
        goto end;
    }
    ret = wtk_bankfeat_cfg_update(&(cfg->bankfeat2));
    if (ret != 0) {
        goto end;
    }
    ret = wtk_qmmse_cfg_update(&(cfg->qmmse));
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
    cfg->bf.nmic = cfg->nbfchannel;

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
    cfg->vec_init = powf(1.0 / cfg->nmicchannel, 0.5);

    if (cfg->scov_alpha_idx) {
        for (i = 0; i < cfg->scov_alpha_n; ++i) {
            cfg->scov_alpha_idx[i] =
                (cfg->scov_alpha_idx[i] * 1.0 * cfg->wins) / cfg->rate;
        }
    }

    cfg->eta_mean_thresh = powf(cfg->eta_mean_thresh, cfg->nmicchannel);
    cfg->epsi_thresh = powf(cfg->epsi_thresh, cfg->nmicchannel);
    if (cfg->eta_thresh) {
        for (i = 0; i < cfg->scov_alpha_n; ++i) {
            cfg->eta_thresh[i] = pow(cfg->eta_thresh[i], cfg->nmicchannel);
        }
    }

    cfg->clip_s = (cfg->clip_s * 1.0 * cfg->wins) / cfg->rate;
    cfg->clip_e = (cfg->clip_e * 1.0 * cfg->wins) / cfg->rate;
    cfg->pre_clip_s = (cfg->pre_clip_s * 1.0 * cfg->wins) / cfg->rate;
    cfg->pre_clip_e = (cfg->pre_clip_e * 1.0 * cfg->wins) / cfg->rate;
    cfg->bf_clip_s = (cfg->bf_clip_s * 1.0 * cfg->wins) / cfg->rate;
    cfg->bf_clip_e = (cfg->bf_clip_e * 1.0 * cfg->wins) / cfg->rate;
    cfg->cnon_clip_s = (cfg->cnon_clip_s * 1.0 * cfg->wins) / cfg->rate;
    cfg->cnon_clip_e = (cfg->cnon_clip_e * 1.0 * cfg->wins) / cfg->rate;
    cfg->mu_mask_s = (cfg->mu_mask_s * 1.0 * cfg->wins) / cfg->rate;
    cfg->mu_mask_e = (cfg->mu_mask_e * 1.0 * cfg->wins) / cfg->rate;
    cfg->eta_clip_s = (cfg->eta_clip_s * 1.0 * cfg->wins) / cfg->rate;
    cfg->eta_clip_e = (cfg->eta_clip_e * 1.0 * cfg->wins) / cfg->rate;

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

    if (cfg->use_qnn) {
        wtk_source_loader_t sl;
        sl.hook = 0;
        sl.vf = wtk_source_load_file_v;
        if (cfg->qnn1_fn) {
            cfg->qnn1_buf = wtk_strbuf_new(1024, 1);
            ret = wtk_source_loader_load(
                &sl, cfg->qnn1_buf,
                (wtk_source_load_handler_t)wtk_mask_bf_net_cfg_load_qnn_res,
                cfg->qnn1_fn);
        }
        if (cfg->qnn2_fn) {
            cfg->qnn2_buf = wtk_strbuf_new(1024, 1);
            ret = wtk_source_loader_load(
                &sl, cfg->qnn2_buf,
                (wtk_source_load_handler_t)wtk_mask_bf_net_cfg_load_qnn_res,
                cfg->qnn2_fn);
        }
        if (cfg->qnn3_fn) {
            cfg->qnn3_buf = wtk_strbuf_new(1024, 1);
            ret = wtk_source_loader_load(
                &sl, cfg->qnn3_buf,
                (wtk_source_load_handler_t)wtk_mask_bf_net_cfg_load_qnn_res,
                cfg->qnn3_fn);
        }
    }

    qtk_nnrt_cfg_update(&cfg->stage1_rt);
    qtk_nnrt_cfg_update(&cfg->stage2_rt);
    qtk_nnrt_cfg_update(&cfg->dr_stage2_rt);

    ret = 0;
end:
    return ret;
}

int wtk_mask_bf_net_cfg_update2(wtk_mask_bf_net_cfg_t *cfg,
                                wtk_source_loader_t *sl) {
    wtk_rbin2_item_t *item;
    wtk_rbin2_t *rbin = (wtk_rbin2_t *)(sl->hook);
    int ret;
    int i;

    if (cfg->channel < cfg->nmicchannel + cfg->nspchannel) {
        cfg->channel = cfg->nmicchannel + cfg->nspchannel;
    }
    if (cfg->nbfchannel == 0) {
        cfg->nbfchannel = cfg->nmicchannel;
    }

    cfg->use_rbin_res = 1;
    if (cfg->use_gainnet2) {
        if (cfg->aecmdl_fn) {
            item = wtk_rbin2_get(rbin, cfg->aecmdl_fn, strlen(cfg->aecmdl_fn));
            if (!item) {
                ret = -1;
                goto end;
            }
            cfg->gainnet2 = wtk_gainnet2_cfg_new_bin3(rbin->fn, item->pos);
            if (!cfg->gainnet2) {
                ret = -1;
                goto end;
            }
        }
    }
    if (cfg->use_gainnet2_2) {
        if (cfg->aecmdl_fn2) {
            item =
                wtk_rbin2_get(rbin, cfg->aecmdl_fn2, strlen(cfg->aecmdl_fn2));
            if (!item) {
                ret = -1;
                goto end;
            }
            cfg->gainnet2_2 = wtk_gainnet2_cfg_new_bin3(rbin->fn, item->pos);
            if (!cfg->gainnet2_2) {
                ret = -1;
                goto end;
            }
        }
    }
    if (cfg->use_rls3) {
        if (cfg->use_echo_bf) {
            cfg->echo_rls3.channel = cfg->nbfchannel;
        } else {
            cfg->echo_rls3.channel = 1;
        }
        cfg->echo_rls3.N = cfg->nspchannel;
        ret = wtk_rls3_cfg_update(&(cfg->echo_rls3));
        if (ret != 0) {
            goto end;
        }
    }
    ret = wtk_covm_cfg_update(&(cfg->covm));
    if (ret != 0) {
        goto end;
    }
    ret = wtk_covm_cfg_update(&(cfg->echo_covm));
    if (ret != 0) {
        goto end;
    }
    ret = wtk_bf_cfg_update(&(cfg->bf));
    if (ret != 0) {
        goto end;
    }
    ret = wtk_bankfeat_cfg_update(&(cfg->bankfeat));
    if (ret != 0) {
        goto end;
    }
    ret = wtk_bankfeat_cfg_update(&(cfg->bankfeat2));
    if (ret != 0) {
        goto end;
    }
    ret = wtk_qmmse_cfg_update(&(cfg->qmmse));
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
                (wtk_source_load_handler_t)wtk_mask_bf_net_cfg_load_qnn_res,
                cfg->qnn1_fn);
        }
        if (cfg->qnn2_fn) {
            cfg->qnn2_buf = wtk_strbuf_new(1024, 1);
            ret = wtk_source_loader_load(
                sl, cfg->qnn2_buf,
                (wtk_source_load_handler_t)wtk_mask_bf_net_cfg_load_qnn_res,
                cfg->qnn2_fn);
        }
        if (cfg->qnn3_fn) {
            cfg->qnn3_buf = wtk_strbuf_new(1024, 1);
            ret = wtk_source_loader_load(
                sl, cfg->qnn3_buf,
                (wtk_source_load_handler_t)wtk_mask_bf_net_cfg_load_qnn_res,
                cfg->qnn3_fn);
        }
    }

    cfg->bf.nmic = cfg->nbfchannel;

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

    cfg->vec_init = powf(1.0 / cfg->nmicchannel, 0.5);

    if (cfg->scov_alpha_idx) {
        for (i = 0; i < cfg->scov_alpha_n; ++i) {
            cfg->scov_alpha_idx[i] =
                (cfg->scov_alpha_idx[i] * 1.0 * cfg->wins) / cfg->rate;
        }
    }

    cfg->eta_mean_thresh = powf(cfg->eta_mean_thresh, cfg->nmicchannel);
    cfg->epsi_thresh = powf(cfg->epsi_thresh, cfg->nmicchannel);
    if (cfg->eta_thresh) {
        for (i = 0; i < cfg->scov_alpha_n; ++i) {
            cfg->eta_thresh[i] = powf(cfg->eta_thresh[i], cfg->nmicchannel);
        }
    }

    if (cfg->use_stage1_rt) {
        qtk_nnrt_cfg_update2(&cfg->stage1_rt, sl);
    }
    if (cfg->use_stage2_rt) {
        qtk_nnrt_cfg_update2(&cfg->stage2_rt, sl);
    }
    if (cfg->use_dr_stage2_rt) {
        qtk_nnrt_cfg_update2(&cfg->dr_stage2_rt, sl);
    }

    cfg->clip_s = (cfg->clip_s * 1.0 * cfg->wins) / cfg->rate;
    cfg->clip_e = (cfg->clip_e * 1.0 * cfg->wins) / cfg->rate;
    cfg->pre_clip_s = (cfg->pre_clip_s * 1.0 * cfg->wins) / cfg->rate;
    cfg->pre_clip_e = (cfg->pre_clip_e * 1.0 * cfg->wins) / cfg->rate;
    cfg->bf_clip_s = (cfg->bf_clip_s * 1.0 * cfg->wins) / cfg->rate;
    cfg->bf_clip_e = (cfg->bf_clip_e * 1.0 * cfg->wins) / cfg->rate;
    cfg->cnon_clip_s = (cfg->cnon_clip_s * 1.0 * cfg->wins) / cfg->rate;
    cfg->cnon_clip_e = (cfg->cnon_clip_e * 1.0 * cfg->wins) / cfg->rate;
    cfg->mu_mask_s = (cfg->mu_mask_s * 1.0 * cfg->wins) / cfg->rate;
    cfg->mu_mask_e = (cfg->mu_mask_e * 1.0 * cfg->wins) / cfg->rate;
    cfg->eta_clip_s = (cfg->eta_clip_s * 1.0 * cfg->wins) / cfg->rate;
    cfg->eta_clip_e = (cfg->eta_clip_e * 1.0 * cfg->wins) / cfg->rate;

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

    ret = 0;
end:
    return ret;
}

int wtk_mask_bf_net_cfg_update_local(wtk_mask_bf_net_cfg_t *cfg,
                                     wtk_local_cfg_t *m) {
    wtk_string_t *v;
    wtk_local_cfg_t *lc;
    int ret;
    wtk_array_t *a;
    int i;

    lc = m;
    wtk_local_cfg_update_cfg_str(lc, cfg, aecmdl_fn, v);
    wtk_local_cfg_update_cfg_str(lc, cfg, aecmdl_fn2, v);

    wtk_local_cfg_update_cfg_i(lc, cfg, wins, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, rate, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, sv, v);

    wtk_local_cfg_update_cfg_i(lc, cfg, channel, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, nbfchannel, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, sp_main_chn, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, mic_scale, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, sp_scale, v);

    wtk_local_cfg_update_cfg_i(lc, cfg, num_frame, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, rss_iter, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, update_w_cnt, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, spenr_thresh, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, spenr_cnt, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, micenr_thresh, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, micenr_cnt, v);

    wtk_local_cfg_update_cfg_i(lc, cfg, clip_s, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, clip_e, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, pre_clip_s, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, pre_clip_e, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, pre_pow_ratio, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, pre_mul_ratio, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, entropy_thresh, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, entropy_sp_thresh, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, entropy_in_cnt, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, entropy_cnt, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, bfmu, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, bfmu2, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, echo_bfmu, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, echo_bfmu2, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, bf_clip_s, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, bf_clip_e, v);

    wtk_local_cfg_update_cfg_i(lc, cfg, featm_lm, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, featsp_lm, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, featm_lm2, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, featsp_lm2, v);

    wtk_local_cfg_update_cfg_i(lc, cfg, cnon_clip_s, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, cnon_clip_e, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, sym, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, scov_alpha, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, ncov_alpha, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, init_covnf, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, mu_entropy_thresh, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, gc_gain, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, gc_min_thresh, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, gc_cnt, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, out_agc_level, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, max_out, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, max_mask, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, mask_peak, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, sp_max_mask, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, sp_mask_peak, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, min_mask, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, eta_mean_thresh, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, scov_entropy_thresh, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, echo_scov_entropy_thresh, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, epsi_thresh, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, eta_clip_s, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, eta_clip_e, v);

    wtk_local_cfg_update_cfg_i(lc, cfg, ntheta, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, sdb_alpha, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, n_mask_mu, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, mu_t_alpha, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, mu_f_alpha, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, mu_mask_s, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, mu_mask_e, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, bf2_alpha, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, echo_bf2_alpha, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, bf_change_frame, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, qmmse2_mask_thresh, v);

    wtk_local_cfg_update_cfg_i(lc, cfg, bf_init_frame, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, beta, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, gamma, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, zeta, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, bin_speech_thresh, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, bin_noise_thresh, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, frame_speech_thresh, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, frame_noise_thresh, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, scnt_thresh, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, ncnt_thresh, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, post_clip, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, model1_scale, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, model1_sp_scale, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, model2_scale, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, model2_sp_scale, v);

    wtk_local_cfg_update_cfg_b(lc, cfg, use_pffft, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_rls3, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_stage1_rt, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_edr_stage1_rt, v);
    wtk_local_cfg_update_cfg_str(lc, cfg, qnn1_fn, v);
    wtk_local_cfg_update_cfg_str(lc, cfg, qnn2_fn, v);
    wtk_local_cfg_update_cfg_str(lc, cfg, qnn3_fn, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_freq_preemph, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_stage2_rt, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_dr_stage2_rt, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_mask_model, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_qmmse, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_qmmse2, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_gainnet2, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_gainnet2_2, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_ccm, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_bf, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_bf_v2, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_echo_bf, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_bf2, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_echocovm, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_gc, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, use_cnon, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_bs_win, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_eq, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_median_chn, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_phase_corr, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_ds, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_debug, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_debug_echo_model, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_limiter, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_raw_add, v);
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

    lc = wtk_local_cfg_find_lc_s(m, "bankfeat");
    if (lc) {
        ret = wtk_bankfeat_cfg_update_local(&(cfg->bankfeat), lc);
        if (ret != 0) {
            goto end;
        }
    }

    lc = wtk_local_cfg_find_lc_s(m, "bankfeat2");
    if (lc) {
        ret = wtk_bankfeat_cfg_update_local(&(cfg->bankfeat2), lc);
        if (ret != 0) {
            goto end;
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

    lc = wtk_local_cfg_find_lc_s(m, "qmmse2");
    if (lc) {
        ret = wtk_qmmse_cfg_update_local(&(cfg->qmmse2), lc);
        cfg->qmmse2.step = cfg->wins / 2;
        if (ret != 0) {
            goto end;
        }
    }

    lc = wtk_local_cfg_find_lc_s(m, "echo_rls3");
    if (lc) {
        ret = wtk_rls3_cfg_update_local(&(cfg->echo_rls3), lc);
        if (ret != 0) {
            goto end;
        }
    }
    lc = wtk_local_cfg_find_lc_s(m, "covm");
    if (lc) {
        ret = wtk_covm_cfg_update_local(&(cfg->covm), lc);
        if (ret != 0) {
            goto end;
        }
    }
    lc = wtk_local_cfg_find_lc_s(m, "echo_covm");
    if (lc) {
        ret = wtk_covm_cfg_update_local(&(cfg->echo_covm), lc);
        if (ret != 0) {
            goto end;
        }
    }
    lc = wtk_local_cfg_find_lc_s(m, "bf");
    if (lc) {
        ret = wtk_bf_cfg_update_local(&(cfg->bf), lc);
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

    a = wtk_local_cfg_find_array_s(m, "all_scov_alpha");
    if (a) {
        cfg->scov_alpha_n = a->nslot;
        cfg->all_scov_alpha =
            (float *)wtk_malloc(sizeof(float) * cfg->scov_alpha_n);
        for (i = 0; i < cfg->scov_alpha_n; ++i) {
            v = ((wtk_string_t **)a->slot)[i];
            cfg->all_scov_alpha[i] = wtk_str_atof(v->data, v->len);
        }
    }
    a = wtk_local_cfg_find_array_s(m, "scov_alpha_idx");
    if (a) {
        if (cfg->scov_alpha_n != a->nslot) {
            wtk_debug("scov_alpha_idx size not match scov_alpha_n\n");
            ret = -1;
            goto end;
        }
        cfg->scov_alpha_idx = (int *)wtk_malloc(sizeof(int) * a->nslot);
        for (i = 0; i < a->nslot; ++i) {
            v = ((wtk_string_t **)a->slot)[i];
            cfg->scov_alpha_idx[i] = wtk_str_atoi(v->data, v->len);
        }
    }
    a = wtk_local_cfg_find_array_s(m, "eta_thresh");
    if (a) {
        if (cfg->scov_alpha_n != a->nslot) {
            wtk_debug("eta_thresh size not match scov_alpha_n\n");
            ret = -1;
            goto end;
        }
        cfg->eta_thresh = (float *)wtk_malloc(sizeof(float) * a->nslot);
        for (i = 0; i < a->nslot; ++i) {
            v = ((wtk_string_t **)a->slot)[i];
            cfg->eta_thresh[i] = wtk_str_atof(v->data, v->len);
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
    lc = wtk_local_cfg_find_lc_s(m, "stage2_rt");
    if (lc) {
        ret = qtk_nnrt_cfg_update_local(&cfg->stage2_rt, lc);
        if (ret != 0) {
            goto end;
        }
    }
    lc = wtk_local_cfg_find_lc_s(m, "dr_stage2_rt");
    if (lc) {
        ret = qtk_nnrt_cfg_update_local(&cfg->dr_stage2_rt, lc);
        if (ret != 0) {
            goto end;
        }
    }
    ret = 0;
end:
    return ret;
}

wtk_mask_bf_net_cfg_t *wtk_mask_bf_net_cfg_new(char *fn) {
    wtk_main_cfg_t *main_cfg;
    wtk_mask_bf_net_cfg_t *cfg;

    main_cfg = wtk_main_cfg_new_type(wtk_mask_bf_net_cfg, fn);
    if (!main_cfg) {
        return NULL;
    }
    cfg = (wtk_mask_bf_net_cfg_t *)main_cfg->cfg;
    cfg->main_cfg = main_cfg;
    return cfg;
}

void wtk_mask_bf_net_cfg_delete(wtk_mask_bf_net_cfg_t *cfg) {
    wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_mask_bf_net_cfg_t *wtk_mask_bf_net_cfg_new_bin(char *fn) {
    wtk_mbin_cfg_t *mbin_cfg;
    wtk_mask_bf_net_cfg_t *cfg;

    mbin_cfg = wtk_mbin_cfg_new_type(wtk_mask_bf_net_cfg, fn, "./cfg");
    if (!mbin_cfg) {
        return NULL;
    }
    cfg = (wtk_mask_bf_net_cfg_t *)mbin_cfg->cfg;
    cfg->mbin_cfg = mbin_cfg;
    return cfg;
}

void wtk_mask_bf_net_cfg_delete_bin(wtk_mask_bf_net_cfg_t *cfg) {
    wtk_mbin_cfg_delete(cfg->mbin_cfg);
}
