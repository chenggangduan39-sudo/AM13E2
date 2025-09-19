#include "wtk_vboxebf6_cfg.h"

int wtk_vboxebf6_cfg_init(wtk_vboxebf6_cfg_t *cfg) {
    cfg->channel = 0;
    cfg->nmicchannel = 0;
    cfg->mic_channel = NULL;
    cfg->nmixchannel = 0;
    cfg->mix_channel = NULL;
    cfg->mic_channel2 = NULL;
    cfg->nspchannel = 0;
    cfg->sp_channel = NULL;
    cfg->nbfchannel = 0;

    cfg->wins = 1024;

    wtk_bankfeat_cfg_init(&(cfg->bankfeat));

    cfg->mdl_fn = NULL;
    cfg->gainnet7 = NULL;
    cfg->aecmdl_fn = NULL;
    cfg->gainnet2 = NULL;
    cfg->use_rbin_res = 0;

    wtk_covm_cfg_init(&(cfg->covm));
    wtk_covm_cfg_init(&(cfg->echo_covm));
    cfg->use_echocovm = 0;
    cfg->bfflush_cnt = 1;

    cfg->bfmu = 1;
    cfg->echo_bfmu = 1;
    cfg->echo_bfmu2 = 1;
    wtk_bf_cfg_init(&(cfg->bf));
    cfg->theta = 180;
    cfg->phi = 0;

    cfg->use_rls = 1;
    wtk_rls_cfg_init(&(cfg->echo_rls));
    cfg->use_nlms = 0;
    wtk_nlms_cfg_init(&(cfg->echo_nlms));

    wtk_qmmse_cfg_init(&(cfg->qmmse));
    wtk_qmmse_cfg_init(&(cfg->qmmse2));
    wtk_qmmse_cfg_init(&(cfg->qmmse3));
    wtk_qmmse_cfg_init(&(cfg->qmmse4));
    wtk_qmmse_cfg_init(&(cfg->qmmse5));
    qtk_ahs_gain_controller_cfg_init(&(cfg->gc));

    cfg->spenr_thresh = 100;
    cfg->spenr_thresh2 = 10000;
    cfg->spenr_cnt = 10;
    cfg->spenr_cnt2 = 1000;

    cfg->micenr_thresh = 300;
    cfg->micenr_cnt = 10;

    cfg->entropy_thresh = 3.5;
    cfg->entropy_cnt = 40;
    cfg->entropy_in_cnt = 40;
    cfg->entropy_scale = 1.0;
    cfg->entropy_in_scale = 1.0;

    cfg->main_cfg = NULL;
    cfg->mbin_cfg = NULL;

    cfg->use_erlssingle = 1;
    cfg->use_firstds = 0;

    cfg->rate = 16000;

    wtk_equalizer_cfg_init(&(cfg->eq));
    cfg->use_eq = 0;
    wtk_limiter_cfg_init(&(cfg->limiter));
    cfg->use_limiter = 1;

    cfg->agc_a = 0.69;
    cfg->agc_a2 = 1.0;
    cfg->agc_b = 6.9;
    cfg->eagc_a = 0.69;
    cfg->eagc_b = 6.9;

    cfg->use_maskssl = 0;
    wtk_maskssl_cfg_init(&(cfg->maskssl));
    cfg->use_maskssl2 = 0;
    wtk_maskssl2_cfg_init(&(cfg->maskssl2));

    cfg->use_fftsbf = 0;
    cfg->use_efftsbf = 1;

    cfg->clip_s = 0;
    cfg->clip_e = 8000;

    cfg->denoise_clip_s = 0;
    cfg->denoise_clip_e = 8000;

    cfg->aec_clip_s = 0;
    cfg->aec_clip_e = 8000;

    cfg->ralpha = 0;
    cfg->ralpha2 = 0;
    cfg->echo_ralpha = 0;
    cfg->echo_ralpha2 = 0;

    cfg->use_qmmse = 1;
    cfg->use_qmmse2 = 1;
    cfg->use_qmmse3 = 1;
    cfg->use_qmmse4 = 1;
    cfg->use_qmmse5 = 1;
    cfg->use_gc = 0;

    cfg->agcmdl_fn = NULL;
    cfg->agc_gainnet = NULL;

    cfg->use_cnon = 0;
    cfg->sym = 1e-2;
    cfg->gbias = 0;

    cfg->use_agcmean = 0;

    cfg->featm_lm = 1;
    cfg->featsp_lm = 1;

    cfg->use_nsgainnet = 1;

    cfg->use_freq_mask = 0;
    cfg->freq_mask = NULL;
    cfg->freq_count = 0;

    cfg->agce_thresh = 0.1;
    cfg->g2_min = 0.05;
    cfg->g2_max = 0.95;

    cfg->g_min = 0.05;
    cfg->g_max = 0.95;
    cfg->g_a = 5.88;
    cfg->g_b = 2.94;

    cfg->g_minthresh = 1e-6;
    cfg->use_ssl_delay = 0;

    cfg->use_gsigmoid = 0;

    cfg->agcaddg = 1.0;

    cfg->use_fixtheta = 0;

    cfg->gf_mask_thresh = -1;

    cfg->max_1 = 32000;
    cfg->min_1 = -32000;
    cfg->max_2 = 30000;
    cfg->min_2 = -30000;
    cfg->f_win = 128;
    cfg->scale = 1;
    cfg->leak_scale = 1.5;
    cfg->denoise_mdl_scale = 1.0;
    cfg->denoise_alg_scale = 1.0;
    cfg->denoise_alg_scale2 = 1.0;
    cfg->denoise_alg_scale3 = 1.0;
    cfg->aec_mdl_scale = 1.0;
    cfg->aec_alg_scale = 1.0;
    cfg->aec_alg_scale2 = 1.0;
    cfg->aec_alg_scale3 = 1.0;
    cfg->qmmse_mask_scale = 1.0;

    cfg->denoise_alg_alpha = 0.1;
    cfg->aec_alg_alpha = 0.1;
    cfg->alg_alpha = -1;

    cfg->use_denoise_mdl = 1;
    cfg->use_denoise_alg = 1;
    cfg->use_aec_mdl = 1;
    cfg->use_aec_alg = 1;
    cfg->use_qmmse_mask = 1;
    cfg->use_qmmse_state = 1;

    cfg->e_th = 0;

    cfg->leak_frame = 1;
    cfg->use_frame_leak = 0;
    cfg->max_out = 32700.0;
    cfg->use_bs_win = 0;
    cfg->b_agc_scale = 1.0;
    cfg->gc_gain = 50000.0;
    cfg->gc_min_thresh = 0;
    cfg->gc_cnt = 10;
    cfg->out_agc_level = -1;

    cfg->agc_model_level = NULL;
    cfg->gc_gain_level = NULL;
    cfg->qmmse_agc_level = NULL;
    cfg->qmmse_max_gain = NULL;
    cfg->n_agc_level = 0;

    cfg->qmmse_noise_suppress = NULL;
    cfg->gbias_level = NULL;
    cfg->n_ans_level = 0;

    cfg->aec_leak_scale = NULL;
    cfg->n_aec_level = 0;

    return 0;
}

int wtk_vboxebf6_cfg_clean(wtk_vboxebf6_cfg_t *cfg) {
    if (cfg->mic_channel) {
        wtk_free(cfg->mic_channel);
    }
    if (cfg->mix_channel) {
        wtk_free(cfg->mix_channel);
    }
    if (cfg->mic_channel2) {
        wtk_free(cfg->mic_channel2);
    }
    if (cfg->sp_channel) {
        wtk_free(cfg->sp_channel);
    }
    if (cfg->freq_mask) {
        wtk_free(cfg->freq_mask);
    }
    if (cfg->agc_model_level) {
        wtk_free(cfg->agc_model_level);
    }
    if (cfg->gc_gain_level) {
        wtk_free(cfg->gc_gain_level);
    }
    if (cfg->qmmse_agc_level) {
        wtk_free(cfg->qmmse_agc_level);
    }
    if (cfg->qmmse_max_gain) {
        wtk_free(cfg->qmmse_max_gain);
    }
    if (cfg->qmmse_noise_suppress) {
        wtk_free(cfg->qmmse_noise_suppress);
    }
    if (cfg->gbias_level) {
        wtk_free(cfg->gbias_level);
    }
    if (cfg->aec_leak_scale) {
        wtk_free(cfg->aec_leak_scale);
    }
    if (cfg->gainnet7) {
        if (cfg->use_rbin_res) {
            wtk_gainnet7_cfg_delete_bin3(cfg->gainnet7);
        } else {
            wtk_gainnet7_cfg_delete_bin2(cfg->gainnet7);
        }
    }
    if (cfg->gainnet2) {
        if (cfg->use_rbin_res) {
            wtk_gainnet2_cfg_delete_bin3(cfg->gainnet2);
        } else {
            wtk_gainnet2_cfg_delete_bin2(cfg->gainnet2);
        }
    }
    if (cfg->agc_gainnet) {
        if (cfg->use_rbin_res) {
            wtk_gainnet_cfg_delete_bin3(cfg->agc_gainnet);
        } else {
            wtk_gainnet_cfg_delete_bin2(cfg->agc_gainnet);
        }
    }

    wtk_bankfeat_cfg_clean(&(cfg->bankfeat));
    wtk_rls_cfg_clean(&(cfg->echo_rls));
    wtk_bf_cfg_clean(&(cfg->bf));
    wtk_covm_cfg_clean(&(cfg->covm));
    wtk_qmmse_cfg_clean(&(cfg->qmmse));
    wtk_qmmse_cfg_clean(&(cfg->qmmse2));
    wtk_qmmse_cfg_clean(&(cfg->qmmse3));
    wtk_qmmse_cfg_clean(&(cfg->qmmse4));
    wtk_qmmse_cfg_clean(&(cfg->qmmse5));
    qtk_ahs_gain_controller_cfg_clean(&(cfg->gc));
    wtk_equalizer_cfg_clean(&(cfg->eq));
    wtk_limiter_cfg_clean(&(cfg->limiter));
    wtk_maskssl_cfg_clean(&(cfg->maskssl));
    wtk_maskssl2_cfg_clean(&(cfg->maskssl2));

    return 0;
}

int wtk_vboxebf6_cfg_update_local(wtk_vboxebf6_cfg_t *cfg, wtk_local_cfg_t *m) {
    wtk_string_t *v;
    wtk_local_cfg_t *lc;
    int ret;
    wtk_array_t *a;
    int i;

    lc = m;
    wtk_local_cfg_update_cfg_i(lc, cfg, wins, v);

    wtk_local_cfg_update_cfg_str(lc, cfg, mdl_fn, v);
    wtk_local_cfg_update_cfg_str(lc, cfg, aecmdl_fn, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, theta, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, phi, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, spenr_thresh, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, spenr_thresh2, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, spenr_cnt, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, spenr_cnt2, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, micenr_thresh, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, micenr_cnt, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, entropy_thresh, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, entropy_cnt, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, entropy_in_cnt, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, entropy_scale, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, entropy_in_scale, v);

    wtk_local_cfg_update_cfg_b(lc, cfg, use_erlssingle, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_nlms, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_rls, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_firstds, v);

    wtk_local_cfg_update_cfg_i(lc, cfg, rate, v);

    wtk_local_cfg_update_cfg_b(lc, cfg, use_eq, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_limiter, v);

    wtk_local_cfg_update_cfg_str(lc, cfg, agcmdl_fn, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, agc_a, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, agc_a2, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, agc_b, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, eagc_a, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, eagc_b, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, agce_thresh, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, g2_min, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, g2_max, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, agcaddg, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, g_min, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, g_max, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, g_a, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, g_b, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, g_minthresh, v);

    wtk_local_cfg_update_cfg_b(lc, cfg, use_maskssl, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_maskssl2, v);

    wtk_local_cfg_update_cfg_b(lc, cfg, use_fftsbf, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_efftsbf, v);

    wtk_local_cfg_update_cfg_i(lc, cfg, clip_s, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, clip_e, v);

    wtk_local_cfg_update_cfg_i(lc, cfg, denoise_clip_s, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, denoise_clip_e, v);

    wtk_local_cfg_update_cfg_i(lc, cfg, aec_clip_s, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, aec_clip_e, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, ralpha, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, ralpha2, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, echo_ralpha, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, echo_ralpha2, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, bfmu, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, echo_bfmu, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, echo_bfmu2, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, gbias, v);

    wtk_local_cfg_update_cfg_b(lc, cfg, use_qmmse, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_qmmse2, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_qmmse3, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_qmmse4, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_qmmse5, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_gc, v);

    wtk_local_cfg_update_cfg_b(lc, cfg, use_echocovm, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, channel, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, nbfchannel, v);

    wtk_local_cfg_update_cfg_i(lc, cfg, bfflush_cnt, v);

    wtk_local_cfg_update_cfg_i(lc, cfg, use_cnon, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, sym, v);

    wtk_local_cfg_update_cfg_b(lc, cfg, use_agcmean, v);

    wtk_local_cfg_update_cfg_i(lc, cfg, featsp_lm, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, featm_lm, v);

    wtk_local_cfg_update_cfg_b(lc, cfg, use_nsgainnet, v);

    wtk_local_cfg_update_cfg_b(lc, cfg, use_gsigmoid, v);

    wtk_local_cfg_update_cfg_b(lc, cfg, use_fixtheta, v);

    wtk_local_cfg_update_cfg_b(lc, cfg, use_freq_mask, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_ssl_delay, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, gf_mask_thresh, v);

    wtk_local_cfg_update_cfg_i(lc, cfg, max_1, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, min_1, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, max_2, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, min_2, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, f_win, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, scale, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_denoise_mdl, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_denoise_alg, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_aec_mdl, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_aec_alg, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_qmmse_mask, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_qmmse_state, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, leak_scale, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, denoise_mdl_scale, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, denoise_alg_scale, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, denoise_alg_scale2, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, denoise_alg_scale3, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, aec_mdl_scale, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, aec_alg_scale, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, aec_alg_scale2, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, aec_alg_scale3, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, qmmse_mask_scale, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, denoise_alg_alpha, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, aec_alg_alpha, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, alg_alpha, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, e_th, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, leak_frame, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_frame_leak, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, max_out, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_bs_win, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, b_agc_scale, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, gc_gain, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, gc_min_thresh, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, gc_cnt, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, out_agc_level, v);

    a = wtk_local_cfg_find_array_s(lc, "mic_channel");
    if (a) {
        cfg->mic_channel = (int *)wtk_malloc(sizeof(int) * a->nslot);
        cfg->nmicchannel = a->nslot;
        for (i = 0; i < a->nslot; ++i) {
            v = ((wtk_string_t **)a->slot)[i];
            cfg->mic_channel[i] = wtk_str_atoi(v->data, v->len);
        }
    }
    a = wtk_local_cfg_find_array_s(lc, "mix_channel");
    if (a) {
        cfg->mix_channel = (int *)wtk_malloc(sizeof(int) * a->nslot);
        cfg->nmixchannel = a->nslot;
        for (i = 0; i < a->nslot; ++i) {
            v = ((wtk_string_t **)a->slot)[i];
            cfg->mix_channel[i] = wtk_str_atoi(v->data, v->len);
        }
    }
    a = wtk_local_cfg_find_array_s(lc, "mic_channel2");
    if (a) {
        cfg->mic_channel2 = (int *)wtk_malloc(sizeof(int) * a->nslot);
        for (i = 0; i < a->nslot; ++i) {
            v = ((wtk_string_t **)a->slot)[i];
            cfg->mic_channel2[i] = wtk_str_atoi(v->data, v->len);
        }
    }

    a = wtk_local_cfg_find_array_s(lc, "sp_channel");
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
    lc = wtk_local_cfg_find_lc_s(m, "echo_rls");
    if (lc) {
        ret = wtk_rls_cfg_update_local(&(cfg->echo_rls), lc);
        if (ret != 0) {
            goto end;
        }
    }
    lc = wtk_local_cfg_find_lc_s(m, "echo_nlms");
    if (lc) {
        ret = wtk_nlms_cfg_update_local(&(cfg->echo_nlms), lc);
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
    lc = wtk_local_cfg_find_lc_s(m, "qmmse3");
    if (lc) {
        ret = wtk_qmmse_cfg_update_local(&(cfg->qmmse3), lc);
        cfg->qmmse3.step = cfg->wins / 2;
        if (ret != 0) {
            goto end;
        }
    }
    lc = wtk_local_cfg_find_lc_s(m, "qmmse4");
    if (lc) {
        ret = wtk_qmmse_cfg_update_local(&(cfg->qmmse4), lc);
        cfg->qmmse4.step = cfg->wins / 2;
        if (ret != 0) {
            goto end;
        }
    }
    lc = wtk_local_cfg_find_lc_s(m, "qmmse5");
    if (lc) {
        ret = wtk_qmmse_cfg_update_local(&(cfg->qmmse5), lc);
        cfg->qmmse5.step = cfg->wins / 2;
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

    lc = wtk_local_cfg_find_lc_s(m, "maskssl");
    if (lc) {
        ret = wtk_maskssl_cfg_update_local(&(cfg->maskssl), lc);
        cfg->maskssl.wins = cfg->wins;
        if (ret != 0) {
            goto end;
        }
    }
    lc = wtk_local_cfg_find_lc_s(m, "maskssl2");
    if (lc) {
        ret = wtk_maskssl2_cfg_update_local(&(cfg->maskssl2), lc);
        cfg->maskssl2.wins = cfg->wins;
        if (ret != 0) {
            goto end;
        }
    }

    if (cfg->use_freq_mask) {
        a = wtk_local_cfg_find_array_s(lc, "freq_mask");
        if (a) {
            cfg->freq_count = a->nslot;
            if (cfg->freq_count != cfg->bankfeat.nb_bands) {
                wtk_debug("freq_mask and nb_bands must have the same length\n");
                ret = -1;
                goto end;
            }
            cfg->freq_mask =
                (float *)wtk_malloc(sizeof(float) * cfg->freq_count);
            for (i = 0; i < cfg->freq_count; ++i) {
                v = ((wtk_string_t **)a->slot)[i];
                cfg->freq_mask[i] = wtk_str_atof(v->data, v->len);
            }
        }
    }

    a = wtk_local_cfg_find_array_s(lc, "qmmse_agc_level");
    if (a) {
        cfg->qmmse_agc_level = (float *)wtk_malloc(sizeof(float) * a->nslot);
        for (i = 0; i < a->nslot; ++i) {
            v = ((wtk_string_t **)a->slot)[i];
            cfg->qmmse_agc_level[i] = wtk_str_atof(v->data, v->len);
        }
        if (cfg->n_agc_level > 0) {
            if (cfg->n_agc_level != a->nslot) {
                wtk_debug("qmmse_agc_level size not match n_agc_level\n");
                ret = -1;
                goto end;
            }
        } else {
            cfg->n_agc_level = a->nslot;
        }
    }
    a = wtk_local_cfg_find_array_s(lc, "qmmse_max_gain");
    if (a) {
        cfg->qmmse_max_gain = (float *)wtk_malloc(sizeof(float) * a->nslot);
        for (i = 0; i < a->nslot; ++i) {
            v = ((wtk_string_t **)a->slot)[i];
            cfg->qmmse_max_gain[i] = wtk_str_atof(v->data, v->len);
        }
        if (cfg->n_agc_level > 0) {
            if (cfg->n_agc_level != a->nslot) {
                wtk_debug("qmmse_max_gain size not match n_agc_level\n");
                ret = -1;
                goto end;
            }
        } else {
            cfg->n_agc_level = a->nslot;
        }
    }
    a = wtk_local_cfg_find_array_s(lc, "agc_model_level");
    if (a) {
        cfg->agc_model_level = (float *)wtk_malloc(sizeof(float) * a->nslot);
        for (i = 0; i < a->nslot; ++i) {
            v = ((wtk_string_t **)a->slot)[i];
            cfg->agc_model_level[i] = wtk_str_atof(v->data, v->len);
        }
        if (cfg->n_agc_level > 0) {
            if (cfg->n_agc_level != a->nslot) {
                wtk_debug("agc_model_level size not match n_agc_level\n");
                ret = -1;
                goto end;
            }
        } else {
            cfg->n_agc_level = a->nslot;
        }
    }
    a = wtk_local_cfg_find_array_s(lc, "gc_gain_level");
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

    a = wtk_local_cfg_find_array_s(lc, "qmmse_noise_suppress");
    if (a) {
        cfg->qmmse_noise_suppress =
            (float *)wtk_malloc(sizeof(float) * a->nslot);
        for (i = 0; i < a->nslot; ++i) {
            v = ((wtk_string_t **)a->slot)[i];
            cfg->qmmse_noise_suppress[i] = wtk_str_atof(v->data, v->len);
        }
        if (cfg->n_ans_level > 0) {
            if (cfg->n_ans_level != a->nslot) {
                wtk_debug("qmmse_noise_suppress size not match n_ans_level\n");
                ret = -1;
                goto end;
            }
        } else {
            cfg->n_ans_level = a->nslot;
        }
    }
    a = wtk_local_cfg_find_array_s(lc, "gbias_level");
    if (a) {
        cfg->gbias_level = (float *)wtk_malloc(sizeof(float) * a->nslot);
        for (i = 0; i < a->nslot; ++i) {
            v = ((wtk_string_t **)a->slot)[i];
            cfg->gbias_level[i] = wtk_str_atof(v->data, v->len);
        }
        if (cfg->n_ans_level > 0) {
            if (cfg->n_ans_level != a->nslot) {
                wtk_debug("gbias_level size not match n_ans_level\n");
                ret = -1;
                goto end;
            }
        } else {
            cfg->n_ans_level = a->nslot;
        }
    }

    a = wtk_local_cfg_find_array_s(lc, "aec_leak_scale");
    if (a) {
        cfg->aec_leak_scale = (float *)wtk_malloc(sizeof(float) * a->nslot);
        for (i = 0; i < a->nslot; ++i) {
            v = ((wtk_string_t **)a->slot)[i];
            cfg->aec_leak_scale[i] = wtk_str_atof(v->data, v->len);
        }
        if (cfg->n_aec_level > 0) {
            if (cfg->n_aec_level != a->nslot) {
                wtk_debug("aec_leak_scale size not match n_aec_level\n");
                ret = -1;
                goto end;
            }
        } else {
            cfg->n_aec_level = a->nslot;
        }
    }
    ret = 0;
end:
    return ret;
}

int wtk_vboxebf6_cfg_update(wtk_vboxebf6_cfg_t *cfg) {
    int ret;

    if (cfg->mdl_fn && cfg->use_nsgainnet) {
        cfg->gainnet7 = wtk_gainnet7_cfg_new_bin2(cfg->mdl_fn);
        if (!cfg->gainnet7) {
            ret = -1;
            goto end;
        }
    }
    if (cfg->aecmdl_fn) {
        cfg->gainnet2 = wtk_gainnet2_cfg_new_bin2(cfg->aecmdl_fn);
        if (!cfg->gainnet2) {
            ret = -1;
            goto end;
        }
    }
    if (cfg->agcmdl_fn) {
        cfg->agc_gainnet = wtk_gainnet_cfg_new_bin2(cfg->agcmdl_fn);
        if (!cfg->agc_gainnet) {
            ret = -1;
            goto end;
        }
    }

    ret = wtk_bankfeat_cfg_update(&(cfg->bankfeat));
    if (ret != 0) {
        goto end;
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

    if (cfg->use_nlms) {
        if (cfg->use_erlssingle) {
            cfg->echo_nlms.channel = 1;
        } else {
            cfg->echo_nlms.channel = cfg->nmicchannel;
            cfg->use_firstds = 0;
        }
        cfg->echo_nlms.N = cfg->nspchannel;
        ret = wtk_nlms_cfg_update(&(cfg->echo_nlms));
        if (ret != 0) {
            goto end;
        }
    } else {
        if (cfg->use_erlssingle) {
            cfg->echo_rls.channel = 1;
        } else {
            cfg->echo_rls.channel = cfg->nmicchannel;
            cfg->use_firstds = 0;
        }
        cfg->echo_rls.N = cfg->nspchannel;
        ret = wtk_rls_cfg_update(&(cfg->echo_rls));
        if (ret != 0) {
            goto end;
        }
    }
    ret = wtk_qmmse_cfg_update(&(cfg->qmmse));
    if (ret != 0) {
        goto end;
    }
    ret = wtk_qmmse_cfg_update(&(cfg->qmmse2));
    if (ret != 0) {
        goto end;
    }
    ret = wtk_qmmse_cfg_update(&(cfg->qmmse3));
    if (ret != 0) {
        goto end;
    }
    ret = wtk_qmmse_cfg_update(&(cfg->qmmse4));
    if (ret != 0) {
        goto end;
    }
    ret = wtk_qmmse_cfg_update(&(cfg->qmmse5));
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
    if (cfg->use_maskssl) {
        ret = wtk_maskssl_cfg_update(&(cfg->maskssl));
        if (ret != 0) {
            goto end;
        }
    }
    if (cfg->use_maskssl2) {
        ret = wtk_maskssl2_cfg_update(&(cfg->maskssl2));
        if (ret != 0) {
            goto end;
        }
    }
    if (cfg->channel <
        max(cfg->nmicchannel, cfg->nmixchannel) + cfg->nspchannel) {
        cfg->channel =
            max(cfg->nmicchannel, cfg->nmixchannel) + cfg->nspchannel;
    }
    if (cfg->nbfchannel == 0) {
        cfg->nbfchannel = cfg->nmicchannel;
    }
    cfg->bf.nmic = cfg->nbfchannel;
    cfg->clip_s = (cfg->clip_s * 1.0 * cfg->wins) / cfg->rate;
    cfg->clip_e = (cfg->clip_e * 1.0 * cfg->wins) / cfg->rate;

    cfg->denoise_clip_s = (cfg->denoise_clip_s * 1.0 * cfg->wins) / cfg->rate;
    cfg->denoise_clip_e = (cfg->denoise_clip_e * 1.0 * cfg->wins) / cfg->rate;

    cfg->aec_clip_s = (cfg->aec_clip_s * 1.0 * cfg->wins) / cfg->rate;
    cfg->aec_clip_e = (cfg->aec_clip_e * 1.0 * cfg->wins) / cfg->rate;

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
    if (cfg->agc_model_level == NULL) {
        cfg->agc_model_level =
            (float *)wtk_malloc(sizeof(float) * cfg->n_agc_level);
        float tmp_agc_model_level[6] = {0.51, 0.71, 1.0, 1.4, 1.96, 2.74};
        for (int i = 0; i < cfg->n_agc_level; ++i) {
            if (i < 6) {
                cfg->agc_model_level[i] = tmp_agc_model_level[i];
            } else {
                cfg->agc_model_level[i] = tmp_agc_model_level[5];
            }
        }
    }
    if (cfg->gc_gain_level == NULL) {
        cfg->gc_gain_level =
            (float *)wtk_malloc(sizeof(float) * cfg->n_agc_level);
        float tmp_gc_gain_level[6] = {6250,  12500,  25000,
                                      50000, 100000, 200000};
        for (int i = 0; i < cfg->n_agc_level; ++i) {
            if (i < 6) {
                cfg->gc_gain_level[i] = tmp_gc_gain_level[i];
            } else {
                cfg->gc_gain_level[i] = tmp_gc_gain_level[5];
            }
        }
    }
    if (cfg->qmmse_agc_level == NULL) {
        cfg->qmmse_agc_level =
            (float *)wtk_malloc(sizeof(float) * cfg->n_agc_level);
        float tmp_qmmse_agc_level[6] = {4200, 6000, 9000, 13500, 20000, 28000};
        for (int i = 0; i < cfg->n_agc_level; ++i) {
            if (i < 6) {
                cfg->qmmse_agc_level[i] = tmp_qmmse_agc_level[i];
            } else {
                cfg->qmmse_agc_level[i] = tmp_qmmse_agc_level[5];
            }
        }
    }
    if (cfg->qmmse_max_gain == NULL) {
        cfg->qmmse_max_gain =
            (float *)wtk_malloc(sizeof(float) * cfg->n_agc_level);
        float tmp_qmmse_max_gain[6] = {25, 25, 25, 35, 35, 45};
        for (int i = 0; i < cfg->n_agc_level; ++i) {
            if (i < 6) {
                cfg->qmmse_max_gain[i] = tmp_qmmse_max_gain[i];
            } else {
                cfg->qmmse_max_gain[i] = tmp_qmmse_max_gain[5];
            }
        }
    }

    if (cfg->n_ans_level == 0) {
        cfg->n_ans_level = 6;
    }
    if (cfg->qmmse_noise_suppress == NULL) {
        cfg->qmmse_noise_suppress =
            (float *)wtk_malloc(sizeof(float) * cfg->n_ans_level);
        float tmp_qmmse_noise_suppress[6] = {0.0,   -5.0,  -15.0,
                                             -25.0, -35.0, -100.0};
        for (int i = 0; i < cfg->n_ans_level; ++i) {
            if (i < 6) {
                cfg->qmmse_noise_suppress[i] = tmp_qmmse_noise_suppress[i];
            } else {
                cfg->qmmse_noise_suppress[i] = tmp_qmmse_noise_suppress[5];
            }
        }
    }
    if (cfg->gbias_level == NULL) {
        cfg->gbias_level =
            (float *)wtk_malloc(sizeof(float) * cfg->n_ans_level);
        // float tmp_gbias_level[6] = {1.0, 0.5, 0.2, 0.1, 0.05, 0.0};
        float tmp_gbias_level[6] = {0.0, 0.5, 0.8, 0.9, 0.95, 1.0};
        for (int i = 0; i < cfg->n_ans_level; ++i) {
            if (i < 6) {
                cfg->gbias_level[i] = tmp_gbias_level[i];
            } else {
                cfg->gbias_level[i] = tmp_gbias_level[5];
            }
        }
    }

    if (cfg->n_aec_level == 0) {
        cfg->n_aec_level = 6;
    }
    if (cfg->aec_leak_scale == NULL) {
        cfg->aec_leak_scale =
            (float *)wtk_malloc(sizeof(float) * cfg->n_aec_level);
        float tmp_aec_leak_scale[6] = {0.0, 0.5, 1.0, 1.5, 2.0, 2.5};
        for (int i = 0; i < cfg->n_aec_level; ++i) {
            if (i < 6) {
                cfg->aec_leak_scale[i] = tmp_aec_leak_scale[i];
            } else {
                cfg->aec_leak_scale[i] = tmp_aec_leak_scale[5];
            }
        }
    }
    ret = 0;
end:
    return ret;
}

int wtk_vboxebf6_cfg_update2(wtk_vboxebf6_cfg_t *cfg, wtk_source_loader_t *sl) {
    wtk_rbin2_item_t *item;
    wtk_rbin2_t *rbin = (wtk_rbin2_t *)(sl->hook);
    int ret;

    cfg->use_rbin_res = 1;
    if (cfg->mdl_fn && cfg->use_nsgainnet) {
        item = wtk_rbin2_get(rbin, cfg->mdl_fn, strlen(cfg->mdl_fn));
        if (!item) {
            ret = -1;
            goto end;
        }
        cfg->gainnet7 = wtk_gainnet7_cfg_new_bin3(rbin->fn, item->pos);
        if (!cfg->gainnet7) {
            ret = -1;
            goto end;
        }
    }
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
    if (cfg->agcmdl_fn) {
        item = wtk_rbin2_get(rbin, cfg->agcmdl_fn, strlen(cfg->agcmdl_fn));
        if (!item) {
            ret = -1;
            goto end;
        }
        cfg->agc_gainnet = wtk_gainnet_cfg_new_bin3(rbin->fn, item->pos);
        if (!cfg->agc_gainnet) {
            ret = -1;
            goto end;
        }
    }

    ret = wtk_bankfeat_cfg_update(&(cfg->bankfeat));
    if (ret != 0) {
        goto end;
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

    if (cfg->use_nlms) {
        if (cfg->use_erlssingle) {
            cfg->echo_nlms.channel = 1;
        } else {
            cfg->echo_nlms.channel = cfg->nmicchannel;
            cfg->use_firstds = 0;
        }
        cfg->echo_nlms.N = cfg->nspchannel;
        ret = wtk_nlms_cfg_update(&(cfg->echo_nlms));
        if (ret != 0) {
            goto end;
        }
    } else {
        if (cfg->use_erlssingle) {
            cfg->echo_rls.channel = 1;
        } else {
            cfg->echo_rls.channel = cfg->nmicchannel;
            cfg->use_firstds = 0;
        }
        cfg->echo_rls.N = cfg->nspchannel;
        ret = wtk_rls_cfg_update(&(cfg->echo_rls));
        if (ret != 0) {
            goto end;
        }
    }
    ret = wtk_qmmse_cfg_update(&(cfg->qmmse));
    if (ret != 0) {
        goto end;
    }
    ret = wtk_qmmse_cfg_update(&(cfg->qmmse2));
    if (ret != 0) {
        goto end;
    }
    ret = wtk_qmmse_cfg_update(&(cfg->qmmse3));
    if (ret != 0) {
        goto end;
    }
    ret = wtk_qmmse_cfg_update(&(cfg->qmmse4));
    if (ret != 0) {
        goto end;
    }
    ret = wtk_qmmse_cfg_update(&(cfg->qmmse5));
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
    if (cfg->use_maskssl) {
        ret = wtk_maskssl_cfg_update2(&(cfg->maskssl), sl);
        if (ret != 0) {
            goto end;
        }
    }
    if (cfg->use_maskssl2) {
        ret = wtk_maskssl2_cfg_update2(&(cfg->maskssl2), sl);
        if (ret != 0) {
            goto end;
        }
    }

    if (cfg->channel <
        max(cfg->nmicchannel, cfg->nmixchannel) + cfg->nspchannel) {
        cfg->channel =
            max(cfg->nmicchannel, cfg->nmixchannel) + cfg->nspchannel;
    }
    if (cfg->nbfchannel == 0) {
        cfg->nbfchannel = cfg->nmicchannel;
    }
    cfg->bf.nmic = cfg->nbfchannel;
    cfg->clip_s = (cfg->clip_s * 1.0 * cfg->wins) / cfg->rate;
    cfg->clip_e = (cfg->clip_e * 1.0 * cfg->wins) / cfg->rate;

    cfg->denoise_clip_s = (cfg->denoise_clip_s * 1.0 * cfg->wins) / cfg->rate;
    cfg->denoise_clip_e = (cfg->denoise_clip_e * 1.0 * cfg->wins) / cfg->rate;

    cfg->aec_clip_s = (cfg->aec_clip_s * 1.0 * cfg->wins) / cfg->rate;
    cfg->aec_clip_e = (cfg->aec_clip_e * 1.0 * cfg->wins) / cfg->rate;

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
    if (cfg->agc_model_level == NULL) {
        cfg->agc_model_level =
            (float *)wtk_malloc(sizeof(float) * cfg->n_agc_level);
        float tmp_agc_model_level[6] = {0.51, 0.71, 1.0, 1.4, 1.96, 2.74};
        for (int i = 0; i < cfg->n_agc_level; ++i) {
            if (i < 6) {
                cfg->agc_model_level[i] = tmp_agc_model_level[i];
            } else {
                cfg->agc_model_level[i] = tmp_agc_model_level[5];
            }
        }
    }
    if (cfg->gc_gain_level == NULL) {
        cfg->gc_gain_level =
            (float *)wtk_malloc(sizeof(float) * cfg->n_agc_level);
        float tmp_gc_gain_level[6] = {6250,  12500,  25000,
                                      50000, 100000, 200000};
        for (int i = 0; i < cfg->n_agc_level; ++i) {
            if (i < 6) {
                cfg->gc_gain_level[i] = tmp_gc_gain_level[i];
            } else {
                cfg->gc_gain_level[i] = tmp_gc_gain_level[5];
            }
        }
    }
    if (cfg->qmmse_agc_level == NULL) {
        cfg->qmmse_agc_level =
            (float *)wtk_malloc(sizeof(float) * cfg->n_agc_level);
        float tmp_qmmse_agc_level[6] = {4200, 6000, 9000, 13500, 20000, 28000};
        for (int i = 0; i < cfg->n_agc_level; ++i) {
            if (i < 6) {
                cfg->qmmse_agc_level[i] = tmp_qmmse_agc_level[i];
            } else {
                cfg->qmmse_agc_level[i] = tmp_qmmse_agc_level[5];
            }
        }
    }
    if (cfg->qmmse_max_gain == NULL) {
        cfg->qmmse_max_gain =
            (float *)wtk_malloc(sizeof(float) * cfg->n_agc_level);
        float tmp_qmmse_max_gain[6] = {25, 25, 25, 35, 35, 45};
        for (int i = 0; i < cfg->n_agc_level; ++i) {
            if (i < 6) {
                cfg->qmmse_max_gain[i] = tmp_qmmse_max_gain[i];
            } else {
                cfg->qmmse_max_gain[i] = tmp_qmmse_max_gain[5];
            }
        }
    }

    if (cfg->n_ans_level == 0) {
        cfg->n_ans_level = 6;
    }
    if (cfg->qmmse_noise_suppress == NULL) {
        cfg->qmmse_noise_suppress =
            (float *)wtk_malloc(sizeof(float) * cfg->n_ans_level);
        float tmp_qmmse_noise_suppress[6] = {0.0,   -5.0,  -15.0,
                                             -25.0, -35.0, -100.0};
        for (int i = 0; i < cfg->n_ans_level; ++i) {
            if (i < 6) {
                cfg->qmmse_noise_suppress[i] = tmp_qmmse_noise_suppress[i];
            } else {
                cfg->qmmse_noise_suppress[i] = tmp_qmmse_noise_suppress[5];
            }
        }
    }
    if (cfg->gbias_level == NULL) {
        cfg->gbias_level =
            (float *)wtk_malloc(sizeof(float) * cfg->n_ans_level);
        // float tmp_gbias_level[6] = {1.0, 0.5, 0.2, 0.1, 0.05, 0.0};
        float tmp_gbias_level[6] = {0.0, 0.5, 0.8, 0.9, 0.95, 1.0};
        for (int i = 0; i < cfg->n_ans_level; ++i) {
            if (i < 6) {
                cfg->gbias_level[i] = tmp_gbias_level[i];
            } else {
                cfg->gbias_level[i] = tmp_gbias_level[5];
            }
        }
    }

    if (cfg->n_aec_level == 0) {
        cfg->n_aec_level = 6;
    }
    if (cfg->aec_leak_scale == NULL) {
        cfg->aec_leak_scale =
            (float *)wtk_malloc(sizeof(float) * cfg->n_aec_level);
        float tmp_aec_leak_scale[6] = {0.0, 0.5, 1.0, 1.5, 2.0, 2.5};
        for (int i = 0; i < cfg->n_aec_level; ++i) {
            if (i < 6) {
                cfg->aec_leak_scale[i] = tmp_aec_leak_scale[i];
            } else {
                cfg->aec_leak_scale[i] = tmp_aec_leak_scale[5];
            }
        }
    }
    ret = 0;
end:
    return ret;
}

wtk_vboxebf6_cfg_t *wtk_vboxebf6_cfg_new(char *fn) {
    wtk_main_cfg_t *main_cfg;
    wtk_vboxebf6_cfg_t *cfg;

    main_cfg = wtk_main_cfg_new_type(wtk_vboxebf6_cfg, fn);
    if (!main_cfg) {
        return NULL;
    }
    cfg = (wtk_vboxebf6_cfg_t *)main_cfg->cfg;
    cfg->main_cfg = main_cfg;
    return cfg;
}

void wtk_vboxebf6_cfg_delete(wtk_vboxebf6_cfg_t *cfg) {
    wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_vboxebf6_cfg_t *wtk_vboxebf6_cfg_new_bin(char *fn) {
    wtk_mbin_cfg_t *mbin_cfg;
    wtk_vboxebf6_cfg_t *cfg;

    mbin_cfg = wtk_mbin_cfg_new_type(wtk_vboxebf6_cfg, fn, "./cfg");
    if (!mbin_cfg) {
        return NULL;
    }
    cfg = (wtk_vboxebf6_cfg_t *)mbin_cfg->cfg;
    cfg->mbin_cfg = mbin_cfg;
    return cfg;
}

void wtk_vboxebf6_cfg_delete_bin(wtk_vboxebf6_cfg_t *cfg) {
    wtk_mbin_cfg_delete(cfg->mbin_cfg);
}
