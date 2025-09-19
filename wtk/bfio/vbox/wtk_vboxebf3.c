#include "wtk_vboxebf3.h"
void wtk_vboxebf3_aec_on_gainnet(wtk_vboxebf3_edra_t *vdr, float *gain, int len,
                                 int is_end);
void wtk_vboxebf3_denoise_on_gainnet(wtk_vboxebf3_edra_t *vdr, float *gain,
                                     int len, int is_end);
void wtk_vboxebf3_agc_on_gainnet(wtk_vboxebf3_edra_t *vdr, float *gain, int len,
                                 int is_end);
void wtk_vboxebf3_on_ssl2(wtk_vboxebf3_t *vboxebf3, wtk_ssl2_extp_t *nbest_extp,
                          int nbest, int ts, int te);

void wtk_vboxebf3_set_3A_level(wtk_vboxebf3_t *vboxebf3) {
    int n_agc_level = vboxebf3->cfg->n_agc_level;
    float *agc_model_level = vboxebf3->cfg->agc_model_level;
    float *qmmse_agc_level = vboxebf3->cfg->qmmse_agc_level;
    float *gc_gain_level = vboxebf3->cfg->gc_gain_level;
    int i;
    float min_dis = 0;
    int idx = 0;

    if (vboxebf3->agc_on) {
        min_dis = fabs(vboxebf3->cfg->agc_a2 - agc_model_level[0]);
        for (i = 1; i < n_agc_level; ++i) {
            if (fabs(vboxebf3->cfg->agc_a2 - agc_model_level[i]) < min_dis) {
                min_dis = fabs(vboxebf3->cfg->agc_a2 - agc_model_level[i]);
                idx = i;
            }
        }
    } else if (vboxebf3->qmmse2) {
        min_dis = fabs(vboxebf3->qmmse2->cfg->agc_level - qmmse_agc_level[0]);
        for (i = 1; i < n_agc_level; ++i) {
            if (fabs(vboxebf3->qmmse2->cfg->agc_level - qmmse_agc_level[i]) <
                min_dis) {
                min_dis =
                    fabs(vboxebf3->qmmse2->cfg->agc_level - qmmse_agc_level[i]);
                idx = i;
            }
        }
    } else if (vboxebf3->gc) {
        min_dis = fabs(vboxebf3->gc->kalman.Z_k - gc_gain_level[0]);
        for (i = 1; i < n_agc_level; ++i) {
            if (fabs(vboxebf3->gc->kalman.Z_k - gc_gain_level[i]) < min_dis) {
                min_dis = fabs(vboxebf3->gc->kalman.Z_k - gc_gain_level[i]);
                idx = i;
            }
        }
    }
    vboxebf3->agc_level = idx + 1;
    vboxebf3->aec_level = vboxebf3->cfg->n_aec_level;
    vboxebf3->ans_level = vboxebf3->cfg->n_ans_level;
}

void wtk_vboxebf3_edra_init(wtk_vboxebf3_edra_t *vdr, wtk_vboxebf3_cfg_t *cfg) {
    vdr->cfg = cfg;
    vdr->nbin = cfg->wins / 2 + 1;

    vdr->bank_mic = wtk_bankfeat_new(&(cfg->bankfeat));
    vdr->bank_sp = wtk_bankfeat_new(&(cfg->bankfeat));

    vdr->g = wtk_malloc(sizeof(float) * cfg->bankfeat.nb_bands);
    vdr->lastg = wtk_malloc(sizeof(float) * cfg->bankfeat.nb_bands);
    vdr->gf = wtk_malloc(sizeof(float) * vdr->nbin);
    vdr->ssl_gf = wtk_malloc(sizeof(float) * vdr->nbin);
    vdr->g2 = wtk_malloc(sizeof(float) * cfg->bankfeat.nb_bands);
    vdr->lastg2 = wtk_malloc(sizeof(float) * cfg->bankfeat.nb_bands);
    vdr->gf2 = wtk_malloc(sizeof(float) * vdr->nbin);

    vdr->feature_sp = NULL;
    if (cfg->featm_lm + cfg->featsp_lm > 2) {
        vdr->feature_sp = wtk_malloc(sizeof(float) * cfg->bankfeat.nb_features *
                                     (cfg->featm_lm + cfg->featsp_lm - 1));
    }

    vdr->gainnet2 = wtk_gainnet2_new(cfg->gainnet2);
    wtk_gainnet2_set_notify(vdr->gainnet2, vdr,
                            (wtk_gainnet2_notify_f)wtk_vboxebf3_aec_on_gainnet);

    vdr->gainnet7 = NULL;
    if (cfg->gainnet7) {
        vdr->gainnet7 = wtk_gainnet7_new(cfg->gainnet7);
        wtk_gainnet7_set_notify(
            vdr->gainnet7, vdr,
            (wtk_gainnet7_notify_f)wtk_vboxebf3_denoise_on_gainnet);
    }
    vdr->agc_gainnet = NULL;
    if (cfg->agc_gainnet) {
        vdr->agc_gainnet = wtk_gainnet_new(cfg->agc_gainnet);
        wtk_gainnet_set_notify(
            vdr->agc_gainnet, vdr,
            (wtk_gainnet_notify_f)wtk_vboxebf3_agc_on_gainnet);
    }

    vdr->qmmse = NULL;
    if (cfg->use_qmmse) {
        vdr->qmmse = wtk_qmmse_new(&(cfg->qmmse));
    }
}

void wtk_vboxebf3_edra_clean(wtk_vboxebf3_edra_t *vdr) {
    wtk_bankfeat_delete(vdr->bank_mic);
    wtk_bankfeat_delete(vdr->bank_sp);

    if (vdr->feature_sp) {
        wtk_free(vdr->feature_sp);
    }

    wtk_free(vdr->g);
    wtk_free(vdr->lastg);
    wtk_free(vdr->gf);
    wtk_free(vdr->ssl_gf);
    wtk_free(vdr->g2);
    wtk_free(vdr->lastg2);
    wtk_free(vdr->gf2);

    if (vdr->qmmse) {
        wtk_qmmse_delete(vdr->qmmse);
    }

    wtk_gainnet2_delete(vdr->gainnet2);
    if (vdr->gainnet7) {
        wtk_gainnet7_delete(vdr->gainnet7);
    }
    if (vdr->agc_gainnet) {
        wtk_gainnet_delete(vdr->agc_gainnet);
    }
}

void wtk_vboxebf3_edra_reset(wtk_vboxebf3_edra_t *vdr) {
    wtk_bankfeat_reset(vdr->bank_mic);
    wtk_bankfeat_reset(vdr->bank_sp);

    if (vdr->feature_sp) {
        memset(vdr->feature_sp, 0,
               sizeof(float) * vdr->bank_sp->cfg->nb_features *
                   (vdr->cfg->featm_lm + vdr->cfg->featsp_lm - 1));
    }

    memset(vdr->g, 0, sizeof(float) * vdr->cfg->bankfeat.nb_bands);
    memset(vdr->lastg, 0, sizeof(float) * vdr->cfg->bankfeat.nb_bands);
    memset(vdr->gf, 0, sizeof(float) * vdr->nbin);
    memset(vdr->ssl_gf, 0, sizeof(float) * vdr->nbin);

    memset(vdr->g2, 0, sizeof(float) * vdr->cfg->bankfeat.nb_bands);
    memset(vdr->lastg2, 0, sizeof(float) * vdr->cfg->bankfeat.nb_bands);
    memset(vdr->gf2, 0, sizeof(float) * vdr->nbin);

    wtk_gainnet2_reset(vdr->gainnet2);
    if (vdr->gainnet7) {
        wtk_gainnet7_reset(vdr->gainnet7);
    }

    if (vdr->agc_gainnet) {
        wtk_gainnet_reset(vdr->agc_gainnet);
    }

    if (vdr->qmmse) {
        wtk_qmmse_reset(vdr->qmmse);
    }
}

wtk_vboxebf3_t *wtk_vboxebf3_new(wtk_vboxebf3_cfg_t *cfg) {
    wtk_vboxebf3_t *vboxebf3;
    int i;

    vboxebf3 = (wtk_vboxebf3_t *)wtk_malloc(sizeof(wtk_vboxebf3_t));
    vboxebf3->cfg = cfg;
    vboxebf3->ths = NULL;
    vboxebf3->notify = NULL;
    vboxebf3->ssl_ths = NULL;
    vboxebf3->notify_ssl = NULL;
    vboxebf3->eng_ths = NULL;
    vboxebf3->notify_eng = NULL;

    vboxebf3->mic = wtk_strbufs_new(vboxebf3->cfg->nmicchannel);
    vboxebf3->mic2 = NULL;
    if (vboxebf3->cfg->mic_channel2) {
        vboxebf3->mic2 = wtk_strbufs_new(vboxebf3->cfg->nmicchannel);
    }
    vboxebf3->sp = wtk_strbufs_new(vboxebf3->cfg->nspchannel);

    vboxebf3->nbin = cfg->wins / 2 + 1;
    vboxebf3->analysis_window = wtk_malloc(sizeof(float) * cfg->wins);  /// 2);
    vboxebf3->synthesis_window = wtk_malloc(sizeof(float) * cfg->wins); /// 2);
    vboxebf3->analysis_mem =
        wtk_float_new_p2(cfg->nmicchannel, vboxebf3->nbin - 1);
    vboxebf3->analysis_mem_sp =
        wtk_float_new_p2(cfg->nspchannel, vboxebf3->nbin - 1);
    vboxebf3->synthesis_mem = wtk_malloc(sizeof(float) * (vboxebf3->nbin - 1));
    vboxebf3->mul_synthesis_mem = NULL;
    if (cfg->use_mul_out) {
        vboxebf3->mul_synthesis_mem =
            wtk_float_new_p2(cfg->nmicchannel, vboxebf3->nbin - 1);
    }
    vboxebf3->rfft = wtk_drft_new(cfg->wins);
    vboxebf3->rfft_in = (float *)wtk_malloc(sizeof(float) * (cfg->wins));

    vboxebf3->fft = wtk_complex_new_p2(cfg->nmicchannel, vboxebf3->nbin);
    vboxebf3->fft_sp = wtk_complex_new_p2(cfg->nspchannel, vboxebf3->nbin);

    vboxebf3->erls = NULL;
    vboxebf3->enlms = NULL;
    vboxebf3->ekalman = NULL;
    if (cfg->use_nlms) {
        vboxebf3->enlms = wtk_malloc(sizeof(wtk_nlms_t) * (vboxebf3->nbin));
        for (i = 0; i < vboxebf3->nbin; ++i) {
            wtk_nlms_init(vboxebf3->enlms + i, &(cfg->echo_nlms));
        }
    } else if (cfg->use_rls) {
        vboxebf3->erls = wtk_malloc(sizeof(wtk_rls_t) * (vboxebf3->nbin));
        for (i = 0; i < vboxebf3->nbin; ++i) {
            wtk_rls_init(vboxebf3->erls + i, &(cfg->echo_rls));
        }
    } else if (cfg->use_kalman) {
        vboxebf3->ekalman =
            qtk_kalman_new(&(cfg->echo_kalman), 1, vboxebf3->nbin);
    }
    vboxebf3->fftx =
        (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * vboxebf3->nbin);
    vboxebf3->ffty =
        (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * vboxebf3->nbin);

    vboxebf3->agc_on = 0;
    if (cfg->agc_gainnet) {
        vboxebf3->agc_on = 1;
    }

    vboxebf3->vdr =
        (wtk_vboxebf3_edra_t *)wtk_malloc(sizeof(wtk_vboxebf3_edra_t));
    wtk_vboxebf3_edra_init(vboxebf3->vdr, cfg);

    vboxebf3->covm = NULL;
    vboxebf3->echo_covm = NULL;
    if (!cfg->use_fixtheta) {
        vboxebf3->covm =
            wtk_covm_new(&(cfg->covm), vboxebf3->nbin, cfg->nbfchannel);
        vboxebf3->echo_covm = NULL;
        if (cfg->use_echocovm) {
            vboxebf3->echo_covm = wtk_covm_new(&(cfg->echo_covm),
                                               vboxebf3->nbin, cfg->nbfchannel);
        }
    }
    vboxebf3->bf = wtk_bf_new(&(cfg->bf), vboxebf3->cfg->wins);

    vboxebf3->sim_scov = NULL;
    vboxebf3->sim_ncov = NULL;
    vboxebf3->sim_echo_scov = NULL;
    vboxebf3->sim_echo_ncov = NULL;
    vboxebf3->sim_cnt_sum = NULL;
    vboxebf3->sim_echo_cnt_sum = NULL;
    if (cfg->use_simple_bf) {
        vboxebf3->sim_scov =
            (float *)wtk_malloc(sizeof(float) * vboxebf3->nbin);
        vboxebf3->sim_ncov =
            (float *)wtk_malloc(sizeof(float) * vboxebf3->nbin);
        vboxebf3->sim_cnt_sum = (int *)wtk_malloc(sizeof(int) * vboxebf3->nbin);
        if (cfg->use_echocovm) {
            vboxebf3->sim_echo_scov =
                (float *)wtk_malloc(sizeof(float) * vboxebf3->nbin);
            vboxebf3->sim_echo_ncov =
                (float *)wtk_malloc(sizeof(float) * vboxebf3->nbin);
            vboxebf3->sim_echo_cnt_sum =
                (int *)wtk_malloc(sizeof(int) * vboxebf3->nbin);
        }
    }

    vboxebf3->mul_scov = NULL;
    vboxebf3->mul_ncov = NULL;
    vboxebf3->mul_echo_scov = NULL;
    vboxebf3->mul_echo_ncov = NULL;
    vboxebf3->mul_cnt_sum = NULL;
    vboxebf3->mul_echo_cnt_sum = NULL;
    if (cfg->use_mul_bf) {
        vboxebf3->mul_scov = wtk_float_new_p2(cfg->nmicchannel, vboxebf3->nbin);
        vboxebf3->mul_ncov = wtk_float_new_p2(cfg->nmicchannel, vboxebf3->nbin);
        vboxebf3->mul_cnt_sum = (int *)wtk_malloc(sizeof(int) * vboxebf3->nbin);
    }
    if (cfg->use_mul_echo_bf) {
        vboxebf3->mul_echo_scov =
            wtk_float_new_p2(cfg->nmicchannel, vboxebf3->nbin);
        vboxebf3->mul_echo_ncov =
            wtk_float_new_p2(cfg->nmicchannel, vboxebf3->nbin);
        vboxebf3->mul_echo_cnt_sum =
            (int *)wtk_malloc(sizeof(int) * vboxebf3->nbin);
    }
    vboxebf3->mask_mu = NULL;
    vboxebf3->mask_mu2 = NULL;
    if (cfg->n_mask_mu > 0) {
        vboxebf3->mask_mu = (float *)wtk_malloc(sizeof(float) * vboxebf3->nbin *
                                                cfg->n_mask_mu);
        vboxebf3->mask_mu2 = (float *)wtk_malloc(
            sizeof(float) * vboxebf3->nbin * cfg->n_mask_mu);
    }

    vboxebf3->entropy_E = (float *)wtk_malloc(sizeof(float) * vboxebf3->nbin);
    vboxebf3->entropy_Eb = (float *)wtk_malloc(sizeof(float) * cfg->wins);

    vboxebf3->eq = NULL;
    if (cfg->use_eq) {
        vboxebf3->eq = wtk_equalizer_new(&(cfg->eq));
    }

    vboxebf3->maskssl = NULL;
    vboxebf3->maskssl2 = NULL;
    if (cfg->use_maskssl && !cfg->use_ssl_delay) {
        vboxebf3->maskssl = wtk_maskssl_new(&(cfg->maskssl));
        wtk_maskssl_set_notify(vboxebf3->maskssl, vboxebf3,
                               (wtk_maskssl_notify_f)wtk_vboxebf3_on_ssl2);
    } else if (cfg->use_maskssl2 && !cfg->use_ssl_delay) {
        vboxebf3->maskssl2 = wtk_maskssl2_new(&(cfg->maskssl2));
        wtk_maskssl2_set_notify(vboxebf3->maskssl2, vboxebf3,
                                (wtk_maskssl2_notify_f)wtk_vboxebf3_on_ssl2);
    }

    vboxebf3->mul_out = NULL;
    if (cfg->use_mul_out) {
        vboxebf3->mul_out =
            wtk_malloc(sizeof(float) * (vboxebf3->nbin - 1) * cfg->nmicchannel);
        vboxebf3->out =
            wtk_malloc(sizeof(float) * (vboxebf3->nbin - 1) * cfg->nmicchannel);
    } else {
        vboxebf3->out = wtk_malloc(sizeof(float) * (vboxebf3->nbin - 1));
    }

    vboxebf3->qmmse2 = NULL;
    if (cfg->use_qmmse2) {
        vboxebf3->qmmse2 = wtk_qmmse_new(&(cfg->qmmse2));
    }
    vboxebf3->gc = NULL;
    if (cfg->use_gc) {
        vboxebf3->gc = qtk_gain_controller_new(&(cfg->gc));
        qtk_gain_controller_set_mode(vboxebf3->gc, 0);
        vboxebf3->gc->kalman.Z_k = cfg->gc_gain;
    }

    vboxebf3->bs_win = NULL;
    if (cfg->use_bs_win) {
        vboxebf3->bs_win = wtk_math_create_hanning_window2(cfg->wins / 2);
    }

    vboxebf3->limiter = NULL;
    if (cfg->use_limiter) {
        vboxebf3->limiter = wtk_limiter_new(&(cfg->limiter));
    }

    wtk_vboxebf3_reset(vboxebf3);

    return vboxebf3;
}

void wtk_vboxebf3_delete(wtk_vboxebf3_t *vboxebf3) {
    int i;

    wtk_strbufs_delete(vboxebf3->mic, vboxebf3->cfg->nmicchannel);
    if (vboxebf3->mic2) {
        wtk_strbufs_delete(vboxebf3->mic2, vboxebf3->cfg->nmicchannel);
    }
    wtk_strbufs_delete(vboxebf3->sp, vboxebf3->cfg->nspchannel);

    wtk_free(vboxebf3->analysis_window);
    wtk_free(vboxebf3->synthesis_window);
    wtk_float_delete_p2(vboxebf3->analysis_mem, vboxebf3->cfg->nmicchannel);
    wtk_float_delete_p2(vboxebf3->analysis_mem_sp, vboxebf3->cfg->nspchannel);
    wtk_free(vboxebf3->synthesis_mem);
    if (vboxebf3->mul_synthesis_mem) {
        wtk_float_delete_p2(vboxebf3->mul_synthesis_mem,
                            vboxebf3->cfg->nmicchannel);
    }
    wtk_free(vboxebf3->rfft_in);
    wtk_drft_delete(vboxebf3->rfft);
    wtk_complex_delete_p2(vboxebf3->fft, vboxebf3->cfg->nmicchannel);
    wtk_complex_delete_p2(vboxebf3->fft_sp, vboxebf3->cfg->nspchannel);

    if (vboxebf3->covm) {
        wtk_covm_delete(vboxebf3->covm);
    }
    if (vboxebf3->echo_covm) {
        wtk_covm_delete(vboxebf3->echo_covm);
    }
    wtk_bf_delete(vboxebf3->bf);
    if (vboxebf3->sim_scov) {
        wtk_free(vboxebf3->sim_scov);
        wtk_free(vboxebf3->sim_ncov);
        wtk_free(vboxebf3->sim_cnt_sum);
    }
    if (vboxebf3->sim_echo_scov) {
        wtk_free(vboxebf3->sim_echo_scov);
        wtk_free(vboxebf3->sim_echo_ncov);
        wtk_free(vboxebf3->sim_echo_cnt_sum);
    }
    if (vboxebf3->mul_scov) {
        wtk_float_delete_p2(vboxebf3->mul_scov, vboxebf3->cfg->nmicchannel);
        wtk_float_delete_p2(vboxebf3->mul_ncov, vboxebf3->cfg->nmicchannel);
        wtk_free(vboxebf3->mul_cnt_sum);
    }
    if (vboxebf3->mul_echo_scov) {
        wtk_float_delete_p2(vboxebf3->mul_echo_scov,
                            vboxebf3->cfg->nmicchannel);
        wtk_float_delete_p2(vboxebf3->mul_echo_ncov,
                            vboxebf3->cfg->nmicchannel);
        wtk_free(vboxebf3->mul_echo_cnt_sum);
    }
    if (vboxebf3->mask_mu) {
        wtk_free(vboxebf3->mask_mu);
        wtk_free(vboxebf3->mask_mu2);
    }
    wtk_free(vboxebf3->entropy_E);
    wtk_free(vboxebf3->entropy_Eb);

    if (vboxebf3->erls) {
        for (i = 0; i < vboxebf3->nbin; ++i) {
            wtk_rls_clean(vboxebf3->erls + i);
        }
        wtk_free(vboxebf3->erls);
    }
    if (vboxebf3->enlms) {
        for (i = 0; i < vboxebf3->nbin; ++i) {
            wtk_nlms_clean(vboxebf3->enlms + i);
        }
        wtk_free(vboxebf3->enlms);
    }
    if (vboxebf3->ekalman) {
        qtk_kalman_delete(vboxebf3->ekalman);
    }

    if (vboxebf3->eq) {
        wtk_equalizer_delete(vboxebf3->eq);
    }

    wtk_free(vboxebf3->fftx);
    wtk_free(vboxebf3->ffty);

    wtk_vboxebf3_edra_clean(vboxebf3->vdr);
    wtk_free(vboxebf3->vdr);

    if (vboxebf3->maskssl) {
        wtk_maskssl_delete(vboxebf3->maskssl);
    }
    if (vboxebf3->maskssl2) {
        wtk_maskssl2_delete(vboxebf3->maskssl2);
    }
    if (vboxebf3->mul_out) {
        wtk_free(vboxebf3->mul_out);
    }
    wtk_free(vboxebf3->out);

    if (vboxebf3->qmmse2) {
        wtk_qmmse_delete(vboxebf3->qmmse2);
    }
    if (vboxebf3->gc) {
        qtk_gain_controller_delete(vboxebf3->gc);
    }
    if (vboxebf3->bs_win) {
        wtk_free(vboxebf3->bs_win);
    }
    if (vboxebf3->limiter) {
        wtk_limiter_delete(vboxebf3->limiter);
    }

    wtk_free(vboxebf3);
}

void wtk_vboxebf3_start(wtk_vboxebf3_t *vboxebf3) {
    wtk_bf_update_ovec(vboxebf3->bf, vboxebf3->cfg->theta, vboxebf3->cfg->phi);
    wtk_bf_init_w(vboxebf3->bf);
    int i;
    if (vboxebf3->mic_delay_samples > 0) {
        for (i = 0; i < vboxebf3->cfg->nmicchannel; ++i) {
            wtk_strbuf_push(vboxebf3->mic[i], NULL,
                            vboxebf3->mic_delay_samples * sizeof(short));
        }
        // wtk_strbufs_push_short(vboxebf3->mic, vboxebf3->cfg->nmicchannel,
        // NULL, vboxebf3->mic_delay_samples);
    }
    if (vboxebf3->sp_delay_samples > 0) {
        for (i = 0; i < vboxebf3->cfg->nspchannel; ++i) {
            wtk_strbuf_push(vboxebf3->sp[i], NULL,
                            vboxebf3->sp_delay_samples * sizeof(short));
        }
        // wtk_strbufs_push_short(vboxebf3->sp, vboxebf3->cfg->nspchannel, NULL,
        // vboxebf3->sp_delay_samples);
    }
}

void wtk_vboxebf3_reset(wtk_vboxebf3_t *vboxebf3) {
    int wins = vboxebf3->cfg->wins;
    int i, nbin = vboxebf3->nbin;

    wtk_strbufs_reset(vboxebf3->mic, vboxebf3->cfg->nmicchannel);
    if (vboxebf3->mic2) {
        wtk_strbufs_reset(vboxebf3->mic2, vboxebf3->cfg->nmicchannel);
    }
    wtk_strbufs_reset(vboxebf3->sp, vboxebf3->cfg->nspchannel);

    for (i = 0; i < wins; ++i) {
        vboxebf3->analysis_window[i] = sin((0.5 + i) * PI / (wins));
    }
    wtk_drft_init_synthesis_window(vboxebf3->synthesis_window,
                                   vboxebf3->analysis_window, wins);

    wtk_float_zero_p2(vboxebf3->analysis_mem, vboxebf3->cfg->nmicchannel,
                      (vboxebf3->nbin - 1));
    wtk_float_zero_p2(vboxebf3->analysis_mem_sp, vboxebf3->cfg->nspchannel,
                      (vboxebf3->nbin - 1));
    memset(vboxebf3->synthesis_mem, 0, sizeof(float) * (vboxebf3->nbin - 1));
    if (vboxebf3->mul_synthesis_mem) {
        wtk_float_zero_p2(vboxebf3->mul_synthesis_mem,
                          vboxebf3->cfg->nmicchannel, (vboxebf3->nbin - 1));
    }

    wtk_complex_zero_p2(vboxebf3->fft, vboxebf3->cfg->nmicchannel,
                        vboxebf3->nbin);
    wtk_complex_zero_p2(vboxebf3->fft_sp, vboxebf3->cfg->nspchannel,
                        vboxebf3->nbin);

    if (vboxebf3->covm) {
        wtk_covm_reset(vboxebf3->covm);
    }
    if (vboxebf3->echo_covm) {
        wtk_covm_reset(vboxebf3->echo_covm);
    }

    wtk_bf_reset(vboxebf3->bf);
    if (vboxebf3->sim_scov) {
        memset(vboxebf3->sim_scov, 0, sizeof(float) * vboxebf3->nbin);
        memset(vboxebf3->sim_ncov, 0, sizeof(float) * vboxebf3->nbin);
        memset(vboxebf3->sim_cnt_sum, 0, sizeof(int) * vboxebf3->nbin);
    }
    if (vboxebf3->sim_echo_scov) {
        memset(vboxebf3->sim_echo_scov, 0, sizeof(float) * vboxebf3->nbin);
        memset(vboxebf3->sim_echo_ncov, 0, sizeof(float) * vboxebf3->nbin);
        memset(vboxebf3->sim_echo_cnt_sum, 0, sizeof(int) * vboxebf3->nbin);
    }
    if (vboxebf3->mul_scov) {
        wtk_float_zero_p2(vboxebf3->mul_scov, vboxebf3->cfg->nmicchannel,
                          vboxebf3->nbin);
        wtk_float_zero_p2(vboxebf3->mul_ncov, vboxebf3->cfg->nmicchannel,
                          vboxebf3->nbin);
        memset(vboxebf3->mul_cnt_sum, 0, sizeof(int) * vboxebf3->nbin);
    }
    if (vboxebf3->mul_echo_scov) {
        wtk_float_zero_p2(vboxebf3->mul_echo_scov, vboxebf3->cfg->nmicchannel,
                          vboxebf3->nbin);
        wtk_float_zero_p2(vboxebf3->mul_echo_ncov, vboxebf3->cfg->nmicchannel,
                          vboxebf3->nbin);
        memset(vboxebf3->mul_echo_cnt_sum, 0, sizeof(int) * vboxebf3->nbin);
    }
    if (vboxebf3->mask_mu) {
        memset(vboxebf3->mask_mu, 0,
               sizeof(float) * vboxebf3->nbin * vboxebf3->cfg->n_mask_mu);
        memset(vboxebf3->mask_mu2, 0,
               sizeof(float) * vboxebf3->nbin * vboxebf3->cfg->n_mask_mu);
    }
    memset(vboxebf3->entropy_E, 0, sizeof(float) * nbin);
    memset(vboxebf3->entropy_Eb, 0, sizeof(float) * wins);

    if (vboxebf3->erls) {
        for (i = 0; i < nbin; ++i) {
            wtk_rls_reset(vboxebf3->erls + i);
        }
    }
    if (vboxebf3->enlms) {
        for (i = 0; i < nbin; ++i) {
            wtk_nlms_reset(vboxebf3->enlms + i);
        }
    }
    if (vboxebf3->ekalman) {
        qtk_kalman_reset(vboxebf3->ekalman);
    }

    memset(vboxebf3->ffty, 0, sizeof(wtk_complex_t) * (vboxebf3->nbin));
    memset(vboxebf3->fftx, 0, sizeof(wtk_complex_t) * (vboxebf3->nbin));

    wtk_vboxebf3_edra_reset(vboxebf3->vdr);

    vboxebf3->sp_silcnt = 0;
    vboxebf3->sp_sil = 1;

    vboxebf3->mic_silcnt = 0;
    vboxebf3->mic_sil = 1;

    vboxebf3->bfflushnf = vboxebf3->cfg->bfflush_cnt;

    vboxebf3->ssl_enable = 0;
    if (vboxebf3->maskssl) {
        wtk_maskssl_reset(vboxebf3->maskssl);
        vboxebf3->ssl_enable = 1;
    }
    if (vboxebf3->maskssl2) {
        wtk_maskssl2_reset(vboxebf3->maskssl2);
        vboxebf3->ssl_enable = 1;
    }

    if (vboxebf3->qmmse2) {
        wtk_qmmse_reset(vboxebf3->qmmse2);
    }
    if (vboxebf3->gc) {
        qtk_gain_controller_reset(vboxebf3->gc);
    }
    if (vboxebf3->limiter) {
        wtk_limiter_reset(vboxebf3->limiter);
    }

    wtk_vboxebf3_set_3A_level(vboxebf3);

    vboxebf3->inmic_scale = 1.0;
    vboxebf3->agc_enable = 1;
    vboxebf3->echo_enable = 1;
    vboxebf3->denoise_enable = 1;
    vboxebf3->eq_enable = 1;
    vboxebf3->bs_scale = 1.0;
    vboxebf3->bs_last_scale = 1.0;
    vboxebf3->bs_real_scale = 1.0;
    vboxebf3->bs_max_cnt = 0;
    vboxebf3->eng_cnt = vboxebf3->cfg->eng_cnt;
    vboxebf3->gf_mask_cnt = 0;
    vboxebf3->gc_cnt = 0;

    vboxebf3->mic_delay = 0;
    vboxebf3->sp_delay = 0;
    vboxebf3->mic_delay_samples = 0;
    vboxebf3->sp_delay_samples = 0;

    vboxebf3->mu_entropy_cnt = 0;
}

void wtk_vboxebf3_set_notify(wtk_vboxebf3_t *vboxebf3, void *ths,
                             wtk_vboxebf3_notify_f notify) {
    vboxebf3->notify = notify;
    vboxebf3->ths = ths;
}

void wtk_vboxebf3_set_ssl_notify(wtk_vboxebf3_t *vboxebf3, void *ths,
                                 wtk_vboxebf3_notify_ssl_f notify) {
    vboxebf3->notify_ssl = notify;
    vboxebf3->ssl_ths = ths;
}

void wtk_vboxebf3_set_eng_notify(wtk_vboxebf3_t *vboxebf3, void *ths,
                                 wtk_vboxebf3_notify_eng_f notify) {
    vboxebf3->notify_eng = notify;
    vboxebf3->eng_ths = ths;
}

static float wtk_vboxebf3_sp_energy(short *p, int n) {
    float f, f2;
    int i;

    f = 0;
    for (i = 0; i < n; ++i) {
        f += p[i];
    }
    f /= n;

    f2 = 0;
    for (i = 0; i < n; ++i) {
        f2 += (p[i] - f) * (p[i] - f);
    }
    f2 /= n;

    return f2;
}

static float wtk_vboxebf3_fft_energy(wtk_complex_t *fftx, int nbin) {
    float f;
    int i;

    f = 0;
    for (i = 1; i < nbin - 1; ++i) {
        f += fftx[i].a * fftx[i].a + fftx[i].b * fftx[i].b;
    }

    return f;
}

void wtk_vboxebf3_denoise_on_gainnet(wtk_vboxebf3_edra_t *vdr, float *gain,
                                     int len, int is_end) {
    memcpy(vdr->g, gain, sizeof(float) * vdr->cfg->bankfeat.nb_bands);
}

void wtk_vboxebf3_aec_on_gainnet(wtk_vboxebf3_edra_t *vdr, float *gain, int len,
                                 int is_end) {
    memcpy(vdr->g, gain, sizeof(float) * vdr->cfg->bankfeat.nb_bands);
}

void wtk_vboxebf3_agc_on_gainnet(wtk_vboxebf3_edra_t *vdr, float *gain, int len,
                                 int is_end) {
    memcpy(vdr->g2, gain, sizeof(float) * vdr->cfg->bankfeat.nb_bands);
}

static void _qtk_vector_cpx_mag_squared(wtk_complex_t *a, float *d, int numSamples) {
    int i;
    for (i = 0; i < numSamples; i++) {
        d[i] = a[i].a * a[i].a + a[i].b * a[i].b;
    }
}

float wtk_vboxebf3_entropy(wtk_vboxebf3_t *vboxebf3, wtk_complex_t *fftx) {
    int rate = vboxebf3->cfg->rate;
    int wins = vboxebf3->cfg->wins;
    int nbin = vboxebf3->nbin;
    int i;
    int km = floor(wins * 1.0 / 8);
    float K = 0.5;
    float *E = vboxebf3->entropy_E;
    float P1;
    float *Eb = vboxebf3->entropy_Eb;
    float sum;
    float prob;
    float Hb;
    int fx1 = 200;
    int fx2 = 3500;
    fx1 = (fx1 * 1.0 * wins) / rate;
    fx2 = (fx2 * 1.0 * wins) / rate;

    memset(E, 0, sizeof(float) * nbin);
    memset(Eb, 0, sizeof(float) * wins);
    _qtk_vector_cpx_mag_squared(fftx + fx1, E + fx1, fx2 - fx1);
    sum = 1e-10;
    for (i = fx1; i < fx2; ++i) {
        sum += E[i];
    }
    for (i = fx1; i < fx2; ++i) {
        P1 = E[i] / sum;
        if (P1 >= 0.9) {
            E[i] = 0;
        }
    }
    sum = 0;
    for (i = 0; i < km; ++i) {
        Eb[i] = K;
        Eb[i] += E[i * 4 + 1] + E[i * 4 + 2] + E[i * 4 + 3] + E[i * 4 + 4];
        sum += Eb[i];
    }
    Hb = 0;
    for (i = 0; i < nbin; ++i) {
        prob = (E[i] + K) / sum;
        Hb += -prob * logf(prob + 1e-12);
    }
    // printf("%f\n", Hb);

    return Hb;
}

void wtk_vboxebf3_edra_feed(wtk_vboxebf3_t *vboxebf3, wtk_complex_t *fftx,
                            wtk_complex_t *ffty, int sp_sil, float gbias,
                            int usecohv) {
    wtk_vboxebf3_edra_t *vdr = vboxebf3->vdr;
    int i;
    int nbin = vdr->nbin;
    float *g = vdr->g, *gf = vdr->gf, *g2 = vdr->g2, *gf2 = vdr->gf2;
    float *ssl_gf = vdr->ssl_gf;
    float *lastg = vdr->lastg, *lastg2 = vdr->lastg2;
    float ralpha = vdr->cfg->ralpha, ralpha2 = vdr->cfg->ralpha2;
    float echo_ralpha = vdr->cfg->ralpha, echo_ralpha2 = vdr->cfg->echo_ralpha2;
    float agc_a, agc_b;
    float agc_a2;
    float g2_min = vdr->cfg->g2_min;
    float g2_max = vdr->cfg->g2_max;
    float g_a = vdr->cfg->g_a;
    float g_b = vdr->cfg->g_b;
    float g_min = vdr->cfg->g_min;
    float g_max = vdr->cfg->g_max;
    float g_minthresh = vdr->cfg->g_minthresh;
    wtk_gainnet2_t *gainnet2 = vdr->gainnet2;
    wtk_gainnet7_t *gainnet7 = vdr->gainnet7;
    wtk_gainnet_t *agc_gainnet = vdr->agc_gainnet;
    wtk_bankfeat_t *bank_mic = vdr->bank_mic;
    wtk_bankfeat_t *bank_sp = vdr->bank_sp;
    int featsp_lm = vdr->cfg->featsp_lm;
    int featm_lm = vdr->cfg->featm_lm;
    float *feature_sp = vdr->feature_sp;
    int nb_bands = bank_mic->cfg->nb_bands;
    int nb_features = bank_mic->cfg->nb_features;
    wtk_qmmse_t *qmmse = vdr->qmmse;
    float *qmmse_gain;
    wtk_complex_t *fftytmp, sed, *fftxtmp;
    float ef, yf;
    float leak;

    wtk_bankfeat_flush_frame_features(bank_mic, fftx);
    if (usecohv) {
        fftytmp = ffty;
        fftxtmp = fftx;
        for (i = 0; i < nbin; ++i, ++fftxtmp, ++fftytmp) {
            ef = fftxtmp->a * fftxtmp->a + fftxtmp->b * fftxtmp->b;
            yf = fftytmp->a * fftytmp->a + fftytmp->b * fftytmp->b;
            sed.a = fftytmp->a * fftxtmp->a + fftytmp->b * fftxtmp->b;
            sed.b = -fftytmp->a * fftxtmp->b + fftytmp->b * fftxtmp->a;
            leak = (sed.a * sed.a + sed.b * sed.b) / (max(ef, yf) * yf + 1e-9);
            leak = sqrtf(leak);
            fftytmp->a *= leak;
            fftytmp->b *= leak;
            leak = (sed.a * sed.a + sed.b * sed.b) / (ef * yf + 1e-9);
            gf[i] = leak * yf;
        }
    }
    wtk_bankfeat_flush_frame_features(bank_sp, ffty);
    if (feature_sp && featsp_lm > 1) {
        memmove(feature_sp + nb_features * featm_lm,
                feature_sp + nb_features * (featm_lm - 1),
                sizeof(float) * nb_features * (featsp_lm - 1));
        memcpy(feature_sp + nb_features * (featm_lm - 1), bank_sp->features,
               sizeof(float) * nb_features);
    }

    if (qmmse && usecohv) {
        wtk_qmmse_flush_mask(qmmse, fftx, gf);
    }
    if (1) {
        if (sp_sil) {
            if (gainnet7) {
                wtk_gainnet7_feed(gainnet7, bank_mic->features, nb_features, 0);
            } else {
                if (feature_sp) {
                    wtk_gainnet2_feed2(
                        gainnet2, bank_mic->features, nb_features, feature_sp,
                        nb_features * (featm_lm + featsp_lm - 1), 0);
                } else {
                    wtk_gainnet2_feed2(gainnet2, bank_mic->features,
                                       nb_features, bank_sp->features,
                                       nb_features, 0);
                }
            }
        } else {
            if (feature_sp) {
                wtk_gainnet2_feed2(gainnet2, bank_mic->features, nb_features,
                                   feature_sp,
                                   nb_features * (featm_lm + featsp_lm - 1), 0);
            } else {
                wtk_gainnet2_feed2(gainnet2, bank_mic->features, nb_features,
                                   bank_sp->features, nb_features, 0);
            }
        }
        if (vdr->cfg->use_gsigmoid) {
            for (i = 0; i < nb_bands; ++i) {
                g[i] = max(g_min, g[i]);
                g[i] = min(g_max, g[i]);
                g[i] = -1 / g_a * (logf(1 / g[i] - 1) - g_b);
                if (g[i] < 0) {
                    g[i] = 0;
                };
                if (g[i] > 1) {
                    g[i] = 1;
                };
            }
        }
        if (agc_gainnet) {
            wtk_gainnet_feed2(agc_gainnet, bank_mic->features, nb_features, g,
                              nb_bands, 0);
            if (sp_sil) {
                agc_a = vdr->cfg->agc_a;
                agc_a2 = vdr->cfg->agc_a2;
                agc_b = vdr->cfg->agc_b;
            } else {
                agc_a = vdr->cfg->eagc_a;
                agc_a2 = vdr->cfg->agc_a2;
                agc_b = vdr->cfg->eagc_b;
            }
            for (i = 0; i < nb_bands; ++i) {
                g2[i] = max(g2_min, g2[i]);
                g2[i] = min(g2_max, g2[i]);
                g2[i] = -1 / agc_a * agc_a2 * (logf(1 / g2[i] - 1) - agc_b);
                if (g2[i] < 0) {
                    g2[i] = g[i];
                };
            }
            if (sp_sil) {
                for (i = 0; i < nb_bands; ++i) {
                    g2[i] = max(g2[i], lastg2[i] * ralpha2);
                    lastg2[i] = g2[i];
                }
            } else {
                for (i = 0; i < nb_bands; ++i) {
                    g2[i] = max(g2[i], lastg2[i] * echo_ralpha2);
                    lastg2[i] = g2[i];
                }
            }
            wtk_bankfeat_interp_band_gain(bank_mic, nbin, gf2, g2);
        }
        if (sp_sil) {
            for (i = 0; i < nb_bands; ++i) {
                g[i] = max(g[i], lastg[i] * ralpha);
                lastg[i] = g[i];
            }
        } else {
            for (i = 0; i < nb_bands; ++i) {
                g[i] = max(g[i], lastg[i] * echo_ralpha);
                lastg[i] = g[i];
            }
        }
        wtk_bankfeat_interp_band_gain(bank_mic, nbin, gf, g);
        memcpy(ssl_gf, gf, sizeof(float) * nbin);
        if (vboxebf3->cfg->gf_mask_thresh >= 0) {
            if (wtk_float_mean(g, nb_bands) >= vboxebf3->cfg->gf_mask_thresh) {
                vboxebf3->gf_mask_cnt = vboxebf3->cfg->gf_mask_cnt;
            } else if (vboxebf3->gf_mask_cnt > 0) {
                --vboxebf3->gf_mask_cnt;
            }
            if (vboxebf3->gf_mask_cnt > 0) {
                gbias = vboxebf3->cfg->gbias2;
            }
        }
        if (sp_sil && gbias > 0) {
            for (i = 1; i < nbin - 1; ++i) {
                gf[i] = min(gf[i] + gbias, 1);
            }
            if (agc_gainnet) {
                for (i = 1; i < nbin - 1; ++i) {
                    gf2[i] = gf2[i] / (gf[i] + 1e-6f);
                    gf2[i] = max(1, gf2[i]);
                }
            }
        } else {
            if (qmmse && agc_gainnet && usecohv) {
                qmmse_gain = qmmse->gain;
                for (i = 1; i < nbin - 1; ++i) {
                    if (gf[i] > g_minthresh) {
                        gf2[i] = gf2[i] / gf[i];
                    } else {
                        gf2[i] = 1;
                    }
                    if (gf[i] > qmmse_gain[i]) {
                        gf2[i] *= qmmse_gain[i] / gf[i];
                        gf[i] = qmmse_gain[i];
                    }
                    gf2[i] = max(1, gf2[i]);
                }
            } else if (qmmse && usecohv) {
                qmmse_gain = qmmse->gain;
                for (i = 1; i < nbin - 1; ++i) {
                    if (gf[i] > qmmse_gain[i]) {
                        gf[i] = qmmse_gain[i];
                    }
                }
            } else if (agc_gainnet) {
                for (i = 1; i < nbin - 1; ++i) {
                    if (gf[i] > g_minthresh) {
                        gf2[i] = gf2[i] / gf[i];
                    } else {
                        gf2[i] = 1;
                    }
                    gf2[i] = max(1, gf2[i]);
                }
            }
        }
    }
    if (feature_sp && featm_lm > 1) {
        memmove(feature_sp + nb_features, feature_sp,
                sizeof(float) * nb_features * (featm_lm - 2));
        memcpy(feature_sp, bank_mic->features, sizeof(float) * nb_features);
    }

    if (vboxebf3->mask_mu) {
        float *mask_mu = vboxebf3->mask_mu;
        float *mask_mu2 = vboxebf3->mask_mu2;
        int n_mask_mu = vboxebf3->cfg->n_mask_mu;
        float alpha = vboxebf3->cfg->mu_t_alpha;
        float alpha_1 = 1.0 - alpha;
        float alpha2 = vboxebf3->cfg->mu_f_alpha;
        float alpha2_1 = 1.0 - alpha2;
        int base_fs = vboxebf3->cfg->mu_mask_s;
        int base_fe = vboxebf3->cfg->mu_mask_e;
        int idx, idx2, idx3;
        int j, k;

        memcpy(mask_mu, mask_mu + nbin, sizeof(float) * nbin * (n_mask_mu - 1));
        memcpy(mask_mu + nbin * (n_mask_mu - 1), vboxebf3->vdr->gf,
               sizeof(float) * nbin);

        for (k = 0; k < nbin; ++k) {
            for (i = n_mask_mu - 1; i < n_mask_mu; ++i) {
                idx = i * nbin + k;
                mask_mu2[idx] = mask_mu[idx];
            }
        }
        for (k = base_fs; k < base_fe; ++k) {
            for (i = n_mask_mu - 1; i < n_mask_mu; ++i) {
                idx = i * nbin + k;
                idx3 = idx;
                mask_mu2[idx] = mask_mu[idx];
                for (j = i - 1; j > i - n_mask_mu; --j) {
                    idx2 = j * nbin + k;
                    if (mask_mu[idx3] < mask_mu[idx2]) {
                        mask_mu2[idx] =
                            mask_mu[idx2] * alpha + mask_mu[idx] * alpha_1;
                        idx3 = idx2;
                    }
                }
            }
        }
        for (i = n_mask_mu - 1; i < n_mask_mu; ++i) {
            for (k = base_fe; k < nbin; ++k) {
                idx = i * nbin + k;
                idx3 = idx;
                mask_mu2[idx] = mask_mu[idx];
                for (j = k; j > 0; --j) {
                    idx2 = i * nbin + j;
                    if (mask_mu2[idx3] < mask_mu2[idx2]) {
                        mask_mu2[idx] =
                            mask_mu2[idx2] * alpha2 + mask_mu[idx] * alpha2_1;
                        idx3 = idx2;
                    }
                }
            }
        }
    }
}

void wtk_vboxebf3_feed_edra(wtk_vboxebf3_t *vboxebf3, wtk_complex_t **fft,
                            wtk_complex_t **fft_sp) {
    int nmicchannel = vboxebf3->cfg->nmicchannel;
    int nspchannel = vboxebf3->cfg->nspchannel;
    int i, j, k;
    int nbin = vboxebf3->nbin;
    wtk_rls_t *erls = vboxebf3->erls, *erlstmp;
    wtk_nlms_t *enlms = vboxebf3->enlms, *enlmstmp;
    qtk_ahs_kalman_t *ekalman = vboxebf3->ekalman;
    wtk_complex_t *fftx = vboxebf3->fftx;
    wtk_complex_t ffttmp[64] = {0};
    wtk_complex_t fftsp2[10] = {0};
    wtk_complex_t *ffty = vboxebf3->ffty;
    int usecohv = 1;

    if (erls) {
        erlstmp = erls;
        for (k = 0; k < nbin; ++k, ++erlstmp) {
            if (vboxebf3->cfg->use_erlssingle && vboxebf3->cfg->use_firstds) {
                ffttmp[0].a = ffttmp[0].b = 0;
                for (i = 0; i < nmicchannel; ++i) {
                    ffttmp[0].a += fft[i][k].a;
                    ffttmp[0].b += fft[i][k].b;
                }
                ffttmp[0].a /= nmicchannel;
                ffttmp[0].b /= nmicchannel;
            } else {
                for (i = 0; i < nmicchannel; ++i) {
                    ffttmp[i] = fft[i][k];
                }
            }
            for (j = 0; j < nspchannel; ++j) {
                fftsp2[j] = fft_sp[j][k];
            }
            wtk_rls_feed3(erlstmp, ffttmp, fftsp2,
                          vboxebf3->sp_sil == 0 && vboxebf3->echo_enable);
            if (vboxebf3->sp_sil == 0 && vboxebf3->echo_enable) {
                ffty[k] = erlstmp->lsty[0];
                fftx[k] = erlstmp->out[0];
                if (vboxebf3->cfg->use_erlssingle) {
                    for (i = 0; i < nmicchannel; ++i) {
                        fft[i][k].a -= erlstmp->lsty[0].a;
                        fft[i][k].b -= erlstmp->lsty[0].b;
                    }
                } else {
                    for (i = 0; i < nmicchannel; ++i) {
                        fft[i][k] = erlstmp->out[i];
                    }
                }
            } else {
                fftx[k] = ffttmp[0];
                ffty[k].a = ffty[k].b = 0;
            }
        }
    } else if (enlms) {
        enlmstmp = enlms;
        for (k = 0; k < nbin; ++k, ++enlmstmp) {
            if (vboxebf3->cfg->use_erlssingle && vboxebf3->cfg->use_firstds) {
                ffttmp[0].a = ffttmp[0].b = 0;
                for (i = 0; i < nmicchannel; ++i) {
                    ffttmp[0].a += fft[i][k].a;
                    ffttmp[0].b += fft[i][k].b;
                }
                ffttmp[0].a /= nmicchannel;
                ffttmp[0].b /= nmicchannel;
            } else {
                for (i = 0; i < nmicchannel; ++i) {
                    ffttmp[i] = fft[i][k];
                }
            }
            for (j = 0; j < nspchannel; ++j) {
                fftsp2[j] = fft_sp[j][k];
            }
            wtk_nlms_feed3(enlmstmp, ffttmp, fftsp2,
                           vboxebf3->sp_sil == 0 && vboxebf3->echo_enable);
            if (vboxebf3->sp_sil == 0 && vboxebf3->echo_enable) {
                ffty[k] = enlmstmp->lsty[0];
                fftx[k] = enlmstmp->out[0];
                if (vboxebf3->cfg->use_erlssingle) {
                    for (i = 0; i < nmicchannel; ++i) {
                        fft[i][k].a -= enlmstmp->lsty[0].a;
                        fft[i][k].b -= enlmstmp->lsty[0].b;
                    }
                } else {
                    for (i = 0; i < nmicchannel; ++i) {
                        fft[i][k] = enlmstmp->out[i];
                    }
                }
            } else {
                fftx[k] = ffttmp[0];
                ffty[k].a = ffty[k].b = 0;
            }
        }
    } else if (ekalman) {
        qtk_kalman_update(ekalman, fft[0], vboxebf3->nbin, 1, fft_sp[0]);
        if (vboxebf3->sp_sil == 0) {
            for (k = 0; k < nbin; ++k) {
                fftx[k].a = ekalman->s_hat[k].a;
                fftx[k].b = ekalman->s_hat[k].b;
                ffty[k].a = ekalman->lsty_hat[k].a;
                ffty[k].b = ekalman->lsty_hat[k].b;
            }
        } else {
            for (k = 0; k < nbin; ++k) {
                fftx[k].a = fft[0][k].a;
                fftx[k].b = fft[0][k].b;
                ffty[k].a = 0;
                ffty[k].b = 0;
            }
        }
    } else {
        usecohv = 0;
        for (k = 0; k < nbin; ++k) {
            fftx[k] = fft[0][k];
            ffty[k] = fft_sp[0][k];
        }
    }

    wtk_vboxebf3_edra_feed(
        vboxebf3, fftx, ffty, vboxebf3->echo_enable ? vboxebf3->sp_sil : 1,
        vboxebf3->denoise_enable ? vboxebf3->cfg->gbias : 1.0, usecohv);
}

void wtk_vboxebf3_feed_bf(wtk_vboxebf3_t *vboxebf3, wtk_complex_t **fft) {
    wtk_complex_t *fftx = vboxebf3->fftx;
    int k, nbin = vboxebf3->nbin;
    int i;
    // int nmicchannel=vboxebf3->cfg->nmicchannel;
    int nbfchannel = vboxebf3->cfg->nbfchannel;
    wtk_bf_t *bf = vboxebf3->bf;
    wtk_covm_t *covm;
    int b;
    wtk_vboxebf3_edra_t *vdr = vboxebf3->vdr;
    float gf;
    wtk_complex_t fft2[64];
    wtk_complex_t ffts[64];
    wtk_complex_t ffty[64];
    int clip_s = vboxebf3->cfg->clip_s;
    int clip_e = vboxebf3->cfg->clip_e;
    int bf_clip_s = vboxebf3->cfg->bf_clip_s;
    int bf_clip_e = vboxebf3->cfg->bf_clip_e;
    int bfflush_cnt = vboxebf3->cfg->bfflush_cnt;

    float *mask_mu = vboxebf3->mask_mu;
    float *mask_mu2 = vboxebf3->mask_mu2;
    int n_mask_mu = vboxebf3->cfg->n_mask_mu;
    float bf_alpha = vboxebf3->cfg->bf_alpha;
    float bf_alpha_1 = 1.0 - bf_alpha;
    float mu_entropy_thresh = vboxebf3->cfg->mu_entropy_thresh;

    float entropy = 0;

    for (k = 0; k < nbin; ++k) {
        gf = vdr->gf[k] * vboxebf3->cfg->wins / 32768.0;
        fftx[k].a = fft[0][k].a * gf;
        fftx[k].b = fft[0][k].b * gf;
    }
    entropy = wtk_vboxebf3_entropy(vboxebf3, fftx);
    // printf("%f\n", entropy);
    if (entropy < mu_entropy_thresh && vboxebf3->sp_sil == 1) {
        vboxebf3->mu_entropy_cnt = vboxebf3->cfg->mu_entropy_cnt;
    } else {
        --vboxebf3->mu_entropy_cnt;
    }

    fftx[0].a = fftx[0].b = 0;
    fftx[nbin - 1].a = fftx[nbin - 1].b = 0;
    if (vboxebf3->cfg->use_fixtheta) {
        for (k = 1; k < nbin - 1; ++k) {
            gf = vdr->gf[k];
            for (i = 0; i < nbfchannel; ++i) {
                ffts[i].a = fft[i][k].a * gf;
                ffts[i].b = fft[i][k].b * gf;
            }
            wtk_bf_output_fft_k(bf, ffts, fftx + k, k);
        }
    } else if (vboxebf3->cfg->use_simple_bf) {
        float *scov;
        float *ncov;
        int *cnt_sum;
        float scov_alpha;
        float ncov_alpha;
        float scov_alpha_1;
        float ncov_alpha_1;
        int init_covnf;
        float w;
        float mu;
        if (vboxebf3->cfg->use_echocovm && vboxebf3->sp_sil == 0 &&
            vboxebf3->echo_enable) {
            scov = vboxebf3->sim_echo_scov;
            ncov = vboxebf3->sim_echo_ncov;
            cnt_sum = vboxebf3->sim_echo_cnt_sum;
            scov_alpha = vboxebf3->echo_covm->cfg->scov_alpha;
            ncov_alpha = vboxebf3->echo_covm->cfg->ncov_alpha;
            init_covnf = vboxebf3->echo_covm->cfg->init_scovnf;
        } else {
            scov = vboxebf3->sim_scov;
            ncov = vboxebf3->sim_ncov;
            cnt_sum = vboxebf3->sim_cnt_sum;
            scov_alpha = vboxebf3->covm->cfg->scov_alpha;
            ncov_alpha = vboxebf3->covm->cfg->ncov_alpha;
            init_covnf = vboxebf3->covm->cfg->init_scovnf;
        }
        scov_alpha_1 = 1.0 - scov_alpha;
        ncov_alpha_1 = 1.0 - ncov_alpha;
        for (k = clip_s + 1; k < clip_e; ++k) {
            gf = vdr->gf[k];
            if (vboxebf3->cfg->use_echocovm && vboxebf3->sp_sil == 0 &&
                vboxebf3->echo_enable) {
                if (vboxebf3->cfg->gf_bfmu_thresh >= 0) {
                    if (k >= bf_clip_s && k < bf_clip_e &&
                        gf > vboxebf3->cfg->gf_bfmu_thresh) {
                        mu = vboxebf3->cfg->echo_bfmu;
                    } else {
                        mu = vboxebf3->cfg->echo_bfmu2;
                    }
                } else {
                    if (k >= bf_clip_s && k < bf_clip_e) {
                        mu = vboxebf3->cfg->echo_bfmu;
                    } else {
                        mu = vboxebf3->cfg->echo_bfmu2;
                    }
                }
            } else {
                if (k >= bf_clip_s && k < bf_clip_e) {
                    mu = vboxebf3->cfg->bfmu;
                } else {
                    mu = vboxebf3->cfg->bfmu2;
                }
            }
            if (mask_mu && vboxebf3->mu_entropy_cnt > 0) {
                int idx = (n_mask_mu - 1) * nbin + k;
                mu = (1.0 - mask_mu2[idx]) * mu;
            }
            ffts[0].a = fft[0][k].a * gf;
            ffts[0].b = fft[0][k].b * gf;
            ffty[0].a = fft[0][k].a * (1 - gf);
            ffty[0].b = fft[0][k].b * (1 - gf);

            if (cnt_sum[k] < init_covnf) {
                scov[k] += ffts[0].a * ffts[0].a + ffts[0].b * ffts[0].b;
            } else if (cnt_sum[k] == init_covnf) {
                scov[k] /= cnt_sum[k];
            } else {
                scov[k] = scov_alpha_1 * scov[k] +
                          scov_alpha *
                              (ffts[0].a * ffts[0].a + ffts[0].b * ffts[0].b);
            }
            if (cnt_sum[k] < init_covnf) {
                ncov[k] += ffty[0].a * ffty[0].a + ffty[0].b * ffty[0].b;
            } else if (cnt_sum[k] == init_covnf) {
                ncov[k] /= cnt_sum[k];
            } else {
                ncov[k] = ncov_alpha_1 * ncov[k] +
                          ncov_alpha *
                              (ffty[0].a * ffty[0].a + ffty[0].b * ffty[0].b);
            }
            cnt_sum[k]++;
            if (ncov[k] < 1e-10) {
                ncov[k] = 1e-10;
            }
            w = scov[k] / (ncov[k]);
            w = w / (mu + w);
            if (vboxebf3->mu_entropy_cnt > 0) {
                w = w * bf_alpha + bf_alpha_1;
            }
            fftx[k].a = ffts[0].a * w;
            fftx[k].b = ffts[0].b * w;
        }
    } else {
        for (k = clip_s + 1; k < clip_e; ++k) {
            gf = vdr->gf[k];
            if (vboxebf3->sp_sil == 0 && vboxebf3->echo_enable) {
                if (vboxebf3->cfg->gf_bfmu_thresh >= 0) {
                    if (k >= bf_clip_s && k < bf_clip_e &&
                        gf > vboxebf3->cfg->gf_bfmu_thresh) {
                        bf->cfg->mu = vboxebf3->cfg->echo_bfmu;
                    } else {
                        bf->cfg->mu = vboxebf3->cfg->echo_bfmu2;
                    }
                } else {
                    if (k >= bf_clip_s && k < bf_clip_e) {
                        bf->cfg->mu = vboxebf3->cfg->echo_bfmu;
                    } else {
                        bf->cfg->mu = vboxebf3->cfg->echo_bfmu2;
                    }
                }
                covm = vboxebf3->echo_covm == NULL ? vboxebf3->covm
                                                   : vboxebf3->echo_covm;
            } else {
                if (k >= bf_clip_s && k < bf_clip_e) {
                    bf->cfg->mu = vboxebf3->cfg->bfmu;
                } else {
                    bf->cfg->mu = vboxebf3->cfg->bfmu2;
                }
                covm = vboxebf3->covm;
            }
            if (mask_mu && vboxebf3->mu_entropy_cnt > 0) {
                int idx = (n_mask_mu - 1) * nbin + k;
                bf->cfg->mu = (1.0 - mask_mu2[idx]) * bf->cfg->mu;
            }
            for (i = 0; i < nbfchannel; ++i) {
                ffts[i].a = fft[i][k].a * gf;
                ffts[i].b = fft[i][k].b * gf;

                ffty[i].a = fft[i][k].a * (1 - gf);
                ffty[i].b = fft[i][k].b * (1 - gf);
            }
            if ((vboxebf3->sp_sil == 1 && vboxebf3->cfg->use_fftsbf) ||
                (vboxebf3->sp_sil == 0 && vboxebf3->cfg->use_efftsbf)) {
                for (i = 0; i < nbfchannel; ++i) {
                    fft2[i] = ffts[i];
                }
            } else {
                for (i = 0; i < nbfchannel; ++i) {
                    fft2[i] = fft[i][k];
                }
            }

            b = 0;
            b = wtk_covm_feed_fft3(covm, ffty, k, 1);
            if (b == 1) {
                wtk_bf_update_ncov(bf, covm->ncov, k);
            }
            if (covm->scov) {
                b = wtk_covm_feed_fft3(covm, ffts, k, 0);
                if (b == 1) {
                    wtk_bf_update_scov(bf, covm->scov, k);
                }
            }
            if (b == 1 && vboxebf3->bfflushnf == bfflush_cnt) {
                wtk_bf_update_w(bf, k);
            }
            wtk_bf_output_fft_k(bf, fft2, fftx + k, k);
            // fftx[k]=fft[0][k];
        }
        --vboxebf3->bfflushnf;
        if (vboxebf3->bfflushnf == 0) {
            vboxebf3->bfflushnf = bfflush_cnt;
        }
    }
    for (k = 0; k <= clip_s; ++k) {
        fftx[k].a = fftx[k].b = 0;
    }
    for (k = clip_e; k < nbin; ++k) {
        fftx[k].a = fftx[k].b = 0;
    }
}

void wtk_vboxebf3_feed_agc2(wtk_vboxebf3_t *vboxebf3, wtk_complex_t *fft) {
    wtk_vboxebf3_edra_t *vdr = vboxebf3->vdr;
    int nbin = vdr->nbin;
    float *gf2 = vdr->gf2;
    float gf;
    int i, j;
    float *freq_mask = vboxebf3->cfg->freq_mask;
    int bands_step = (nbin - 1) / vboxebf3->cfg->bankfeat.nb_bands;
    wtk_bankfeat_t *bankfeat = vboxebf3->vdr->bank_mic;
    int nb_bands = bankfeat->cfg->nb_bands;
    float E = 0;
    float *Ex = bankfeat->Ex;
    float agcaddg = vboxebf3->cfg->agcaddg;

    wtk_bankfeat_compute_band_energy(bankfeat, Ex, fft);
    for (i = 0; i < nb_bands; ++i) {
        E += Ex[i];
    }

    if (vboxebf3->agc_enable && !vdr->bank_mic->silence &&
        E > vboxebf3->cfg->agce_thresh) {
        if (vboxebf3->cfg->use_agcmean) {
            gf = wtk_float_abs_mean(gf2 + 1, nbin - 2);
            if (vboxebf3->cfg->use_freq_mask) {
                for (i = 1; i < nbin - 1; ++i) {
                    j = floor(i / bands_step);
                    fft[i].a *= gf * freq_mask[j] * agcaddg;
                    fft[i].b *= gf * freq_mask[j] * agcaddg;
                }
            } else {
                for (i = 1; i < nbin - 1; ++i) {
                    fft[i].a *= gf * agcaddg;
                    fft[i].b *= gf * agcaddg;
                }
            }
        } else {
            if (vboxebf3->cfg->use_freq_mask) {
                for (i = 1; i < nbin - 1; ++i) {
                    j = floor(i / bands_step);
                    fft[i].a *= gf2[i] * freq_mask[j] * agcaddg;
                    fft[i].b *= gf2[i] * freq_mask[j] * agcaddg;
                }
            } else {
                for (i = 1; i < nbin - 1; ++i) {
                    fft[i].a *= gf2[i] * agcaddg;
                    fft[i].b *= gf2[i] * agcaddg;
                }
            }
        }
    }

    // wtk_complex_t fft2[1024];
    // memcpy(fft2,fft,sizeof(wtk_complex_t)*nbin);
    // for(i=3; i<nbin-3; ++i)
    // {
    // 	fft[i].a=0.075*fft2[i-2].a+0.1*fft2[i-1].a+0.65*fft2[i].a+0.1*fft2[i+1].a+0.075*fft2[i+2].a;
    // 	fft[i].b=0.075*fft2[i-2].b+0.1*fft2[i-1].b+0.65*fft2[i].b+0.1*fft2[i+1].b+0.075*fft2[i+2].b;
    // }
}

void wtk_vboxebf3_feed_cnon(wtk_vboxebf3_t *vboxebf3, wtk_complex_t *fft) {
    wtk_vboxebf3_edra_t *vdr = vboxebf3->vdr;
    int nbin = vdr->nbin;
    float *gf = vdr->gf;
    float sym = vboxebf3->cfg->sym;
    static float fx = 2.0f * PI / RAND_MAX;
    int cnon_clip_s = vboxebf3->cfg->cnon_clip_s;
    int cnon_clip_e = vboxebf3->cfg->cnon_clip_e;
    float f, f2;
    int i;

    for (i = max(1, cnon_clip_s); i < min(nbin - 1, cnon_clip_e); ++i) {
        f = rand() * fx;
        f2 = 1.f - gf[i] * gf[i];
        if (f2 > 0) {
            // f2=sqrtf(f2);
            fft[i].a += sym * cosf(f) * f2;
            fft[i].b += sym * sinf(f) * f2;
        }
    }
}

void wtk_vboxebf3_on_ssl2(wtk_vboxebf3_t *vboxebf3, wtk_ssl2_extp_t *nbest_extp,
                          int nbest, int ts, int te) {
    // printf("[%d %d] ==> %d %d %f\n", ts,te, nbest_extp[0].theta,
    // nbest_extp[0].phi, nbest_extp[0].nspecsum);
    if (vboxebf3->notify_ssl) {
        vboxebf3->notify_ssl(vboxebf3->ssl_ths, ts, te, nbest_extp, nbest);
    }
}

void wtk_vboxebf3_control_bs(wtk_vboxebf3_t *vboxebf3, float *out, int len) {
    float *bs_win = vboxebf3->bs_win;
    float max_out = vboxebf3->cfg->max_out;
    float out_max;
    int i;

    if (vboxebf3->mic_sil == 0) {
        out_max = wtk_float_abs_max(out, len);
        if (out_max > max_out) {
            vboxebf3->bs_scale = max_out / out_max;
            if (vboxebf3->bs_scale < vboxebf3->bs_last_scale) {
                vboxebf3->bs_last_scale = vboxebf3->bs_scale;
            } else {
                vboxebf3->bs_scale = vboxebf3->bs_last_scale;
            }
            vboxebf3->bs_max_cnt = 5;
        }
        if (bs_win) {
            for (i = 0; i < len / 2; ++i) {
                out[i] *= vboxebf3->bs_scale * bs_win[i] +
                          vboxebf3->bs_real_scale * (1.0 - bs_win[i]);
            }
            for (i = len / 2; i < len; ++i) {
                out[i] *= vboxebf3->bs_scale;
            }
            vboxebf3->bs_real_scale = vboxebf3->bs_scale;
        } else {
            for (i = 0; i < len; ++i) {
                out[i] *= vboxebf3->bs_scale;
            }
        }
        if (vboxebf3->bs_max_cnt > 0) {
            --vboxebf3->bs_max_cnt;
        }
        if (vboxebf3->bs_max_cnt <= 0 && vboxebf3->bs_scale < 1.0) {
            vboxebf3->bs_scale *= 1.1f;
            vboxebf3->bs_last_scale = vboxebf3->bs_scale;
            if (vboxebf3->bs_scale > 1.0) {
                vboxebf3->bs_scale = 1.0;
                vboxebf3->bs_last_scale = 1.0;
            }
        }
    } else {
        vboxebf3->bs_scale = 1.0;
        vboxebf3->bs_last_scale = 1.0;
        vboxebf3->bs_max_cnt = 0;
    }
}

void wtk_vboxebf3_feed_qmmse2(wtk_vboxebf3_t *vboxebf3, wtk_complex_t *fftx) {
    wtk_qmmse_t *qmmse2 = vboxebf3->qmmse2;
    float *gf = vboxebf3->vdr->gf;
    int nbin = vboxebf3->nbin;
    // wtk_qmmse_feed_mask(qmmse2, fftx, gf);
    float mean_mask = wtk_float_abs_mean(gf, nbin);
    if (mean_mask < 0.2 || vboxebf3->sp_sil == 0) {
        wtk_qmmse_feed_mask(qmmse2, fftx, gf);
    } else {
        wtk_qmmse_denoise(qmmse2, fftx);
    }
}

void wtk_vboxebf3_feed(wtk_vboxebf3_t *vboxebf3, short *data, int len,
                       int is_end) {
    int i, j;
    int nbin = vboxebf3->nbin;
    int nmicchannel = vboxebf3->cfg->nmicchannel;
    int *mic_channel = vboxebf3->cfg->mic_channel;
    int *mic_channel2 = vboxebf3->cfg->mic_channel2;
    int nspchannel = vboxebf3->cfg->nspchannel;
    int *sp_channel = vboxebf3->cfg->sp_channel;
    int channel = vboxebf3->cfg->channel;
    wtk_strbuf_t **mic = vboxebf3->mic;
    wtk_strbuf_t **mic2 = vboxebf3->mic2;
    wtk_strbuf_t **sp = vboxebf3->sp;
    int wins = vboxebf3->cfg->wins;
    int fsize = wins / 2;
    int length;
    float spenr;
    float spenr_thresh = vboxebf3->cfg->spenr_thresh;
    int spenr_cnt = vboxebf3->cfg->spenr_cnt;
    float micenr;
    float micenr_thresh = vboxebf3->cfg->micenr_thresh;
    int micenr_cnt = vboxebf3->cfg->micenr_cnt;
    wtk_drft_t *rfft = vboxebf3->rfft;
    float *rfft_in = vboxebf3->rfft_in;
    wtk_complex_t **fft = vboxebf3->fft;
    wtk_complex_t **fft_sp = vboxebf3->fft_sp;
    float **analysis_mem = vboxebf3->analysis_mem,
          **analysis_mem_sp = vboxebf3->analysis_mem_sp;
    float *synthesis_mem = vboxebf3->synthesis_mem;
    float *analysis_window = vboxebf3->analysis_window,
          *synthesis_window = vboxebf3->synthesis_window;
    wtk_complex_t *fftx = vboxebf3->fftx;
    float *out = vboxebf3->out;
    short *pv = (short *)out;
    float mic_energy = 0;
    float energy = 0;
    float snr = 0;

    float de_eng_sum = 0;
    int de_clip_s = vboxebf3->cfg->de_clip_s;
    int de_clip_e = vboxebf3->cfg->de_clip_e;
    float de_thresh = vboxebf3->cfg->de_thresh;
    float de_alpha = vboxebf3->cfg->de_alpha;
    float de_alpha2;
    short pv1;
    int t_mic_in_scale = vboxebf3->cfg->t_mic_in_scale;
    int t_sp_in_scale = vboxebf3->cfg->t_sp_in_scale;

    for (i = 0; i < len; ++i) {
        for (j = 0; j < nmicchannel; ++j) {
            pv1 = data[mic_channel[j]];
            pv1 *= t_mic_in_scale;
            wtk_strbuf_push(mic[j], (char *)&pv1, sizeof(short));
        }
        for (j = 0; j < nspchannel; ++j) {
            pv1 = data[sp_channel[j]];
            pv1 *= t_sp_in_scale;
            wtk_strbuf_push(sp[j], (char *)&pv1, sizeof(short));
        }
        if (mic2) {
            for (j = 0; j < nmicchannel; ++j) {
                pv1 = data[mic_channel2[j]];
                pv1 *= t_mic_in_scale;
                wtk_strbuf_push(mic2[j], (char *)&pv1, sizeof(short));
            }
        }
        data += channel;
    }
    length = mic[0]->pos / sizeof(short);
    while (length >= fsize) {
        if (vboxebf3->notify_eng) {
            mic_energy = sqrtf(wtk_vboxebf3_fft_energy(fft[0], nbin) * 2);
        }
        for (i = 0, j = nmicchannel; i < nspchannel; ++i, ++j) {
            wtk_drft_frame_analysis2(rfft, rfft_in, analysis_mem_sp[i],
                                     fft_sp[i], (short *)(sp[i]->data), wins,
                                     analysis_window);
        }
        if (nspchannel > 0) {
            spenr = wtk_vboxebf3_sp_energy((short *)sp[0]->data, fsize);
        } else {
            spenr = 0;
        }
        // static int cnt=0;
        // cnt++;
        if (spenr > spenr_thresh) {
            // if(vboxebf3->sp_sil==1)
            // {
            // 	printf("sp start %f %f
            // %f\n", 1.0/16000*cnt*(nbin-1),spenr,spenr_thresh);
            // }
            vboxebf3->sp_sil = 0;
            vboxebf3->sp_silcnt = spenr_cnt;
        } else if (vboxebf3->sp_sil == 0) {
            vboxebf3->sp_silcnt -= 1;
            if (vboxebf3->sp_silcnt <= 0) {
                // printf("sp end %f\n", 1.0/16000*cnt*(nbin-1));
                vboxebf3->sp_sil = 1;
            }
        }
        if (vboxebf3->sp_sil == 1) {
            for (i = 0; i < nmicchannel; ++i) {
                wtk_drft_frame_analysis2(rfft, rfft_in, analysis_mem[i], fft[i],
                                         (short *)(mic[i]->data), wins,
                                         analysis_window);
            }
        } else {
            if (mic2) {
                for (i = 0; i < nmicchannel; ++i) {
                    wtk_drft_frame_analysis2(rfft, rfft_in, analysis_mem[i],
                                             fft[i], (short *)(mic2[i]->data),
                                             wins, analysis_window);
                }
            } else {
                for (i = 0; i < nmicchannel; ++i) {
                    wtk_drft_frame_analysis2(rfft, rfft_in, analysis_mem[i],
                                             fft[i], (short *)(mic[i]->data),
                                             wins, analysis_window);
                }
            }
        }
        if (vboxebf3->agc_enable == 0 && vboxebf3->inmic_scale > 1.0) {
            for (i = 0; i < nmicchannel; ++i) {
                for (j = 0; j < nbin; ++j) {
                    fft[i][j].a *= vboxebf3->inmic_scale;
                    fft[i][j].b *= vboxebf3->inmic_scale;
                }
            }
        } else if (vboxebf3->cfg->in_scale != 1.0) {
            for (i = 0; i < nmicchannel; ++i) {
                for (j = 0; j < nbin; ++j) {
                    fft[i][j].a *= vboxebf3->cfg->in_scale;
                    fft[i][j].b *= vboxebf3->cfg->in_scale;
                }
            }
        }
        if (vboxebf3->cfg->use_in_eq && vboxebf3->sp_sil) {
            float *eq_gain = vboxebf3->cfg->eq_gain;
            for (i = 0; i < nbin; ++i) {
                for (j = 0; j < nmicchannel; ++j) {
                    fft[j][i].a *= eq_gain[i];
                    fft[j][i].b *= eq_gain[i];
                }
            }
        }

        if (vboxebf3->cfg->use_freq_preemph) {
            int pre_clip_s = vboxebf3->cfg->pre_clip_s;
            int pre_clip_e = vboxebf3->cfg->pre_clip_e;
            float pre_pow_ratio = vboxebf3->cfg->pre_pow_ratio;
            float pre_mul_ratio = vboxebf3->cfg->pre_mul_ratio;
            float alpha;
            for (i = pre_clip_s; i < pre_clip_e; ++i) {
                alpha = (pre_mul_ratio - 1) *
                            (pow(i - pre_clip_s, pre_pow_ratio)) /
                            pow((nbin - pre_clip_s), pre_pow_ratio) +
                        1;
                for (j = 0; j < nmicchannel; ++j) {
                    fft[j][i].a *= alpha;
                    fft[j][i].b *= alpha;
                }
            }
        }

        wtk_vboxebf3_feed_edra(vboxebf3, fft, fft_sp);
        wtk_vboxebf3_feed_bf(vboxebf3, fft);
        if (vboxebf3->sp_sil == 1) {
            if (vboxebf3->denoise_enable == 0) {
                memcpy(fftx, fft[0], sizeof(wtk_complex_t) * nbin);
            }
        }

        if (vboxebf3->notify_eng) {
            energy = sqrtf(wtk_vboxebf3_fft_energy(fftx, nbin) * 2);
            snr = 20 *
                  log10(max(
                      floor(energy / max(abs(mic_energy - energy), 1e-6) + 0.5),
                      1.0));
            energy =
                20 * log10(max(floor(energy + 0.5), 1.0)) - 20 * log10(32768.0);

            // wtk_debug("%f %f\n", energy, snr);
            vboxebf3->notify_eng(vboxebf3->eng_ths, energy, snr);
        }
        micenr = wtk_vboxebf3_fft_energy(fftx, nbin);
        if (vboxebf3->cfg->eng_scale < 1.0 ||
            vboxebf3->cfg->eng_sp_scale < 1.0) {
            float eng_scale;
            if (vboxebf3->sp_sil == 0) {
                eng_scale = vboxebf3->cfg->eng_sp_scale;
            } else {
                eng_scale = vboxebf3->cfg->eng_scale;
            }
            if (micenr > vboxebf3->cfg->eng_thresh) {
                vboxebf3->eng_cnt = vboxebf3->cfg->eng_cnt;
            } else {
                --vboxebf3->eng_cnt;
            }
            if (vboxebf3->eng_cnt <= 0) {
                for (i = 0; i < nbin; ++i) {
                    fftx[i].a *= eng_scale;
                    fftx[i].b *= eng_scale;
                }
            }
        }
        float gc_mask = wtk_float_abs_mean(vboxebf3->vdr->gf, nbin);
        float gc_min_thresh;
        if (vboxebf3->sp_sil) {
            gc_min_thresh = vboxebf3->cfg->gc_min_thresh;
        } else {
            gc_min_thresh = vboxebf3->cfg->echo_gc_min_thresh;
        }
        if (gc_mask > gc_min_thresh) {
            vboxebf3->gc_cnt = vboxebf3->cfg->gc_cnt;
        } else {
            --vboxebf3->gc_cnt;
        }

        if (vboxebf3->gc_cnt >= 0) {
            if (vboxebf3->agc_enable) {
                if (vboxebf3->qmmse2) {
                    wtk_vboxebf3_feed_qmmse2(vboxebf3, fftx);
                }
                if (vboxebf3->agc_on) {
                    wtk_vboxebf3_feed_agc2(vboxebf3, fftx);
                }
                if (vboxebf3->gc) {
                    qtk_gain_controller_run(vboxebf3->gc, fftx, fsize, NULL,
                                            gc_mask);
                }
            }
        } else {
            memset(fftx, 0, sizeof(wtk_complex_t) * nbin);
        }

        // static int cnt=0;
        // cnt++;
        micenr = wtk_vboxebf3_fft_energy(fftx, nbin);
        // printf("%f\n", micenr);
        if (micenr > micenr_thresh) {
            // if(vboxebf3->mic_sil==1)
            // {
            // 	printf("sp start %f %f
            // %f\n", 1.0/16000*cnt*(nbin-1),micenr,micenr_thresh);
            // }
            vboxebf3->mic_sil = 0;
            vboxebf3->mic_silcnt = micenr_cnt;
        } else if (vboxebf3->mic_sil == 0) {
            vboxebf3->mic_silcnt -= 1;
            if (vboxebf3->mic_silcnt <= 0) {
                // printf("sp end %f\n", 1.0/16000*cnt*(nbin-1));
                vboxebf3->mic_sil = 1;
            }
        }

        if (vboxebf3->maskssl2 && vboxebf3->ssl_enable) {
            wtk_maskssl2_feed_fft2(vboxebf3->maskssl2, fft,
                                   vboxebf3->vdr->ssl_gf, vboxebf3->mic_sil);
        } else if (vboxebf3->maskssl && vboxebf3->ssl_enable) {
            wtk_maskssl_feed_fft2(vboxebf3->maskssl, fft, vboxebf3->vdr->ssl_gf,
                                  vboxebf3->mic_sil);
        }

        if (vboxebf3->cfg->use_cnon) {
            wtk_vboxebf3_feed_cnon(vboxebf3, fftx);
        }

        wtk_strbufs_pop(mic, nmicchannel, fsize * sizeof(short));
        if (mic2) {
            wtk_strbufs_pop(mic2, nmicchannel, fsize * sizeof(short));
        }
        wtk_strbufs_pop(sp, nspchannel, fsize * sizeof(short));
        length = mic[0]->pos / sizeof(short);

        if (de_alpha != 1.0) {
            for (i = de_clip_s; i < de_clip_e; ++i) {
                de_eng_sum += fftx[i].a * fftx[i].a + fftx[i].b * fftx[i].b;
            }
            // printf("%f\n", logf(de_eng_sum+1e-9));
            // printf("%f\n", de_eng_sum);
            if (de_eng_sum > de_thresh) {
                de_alpha2 = de_alpha *
                            min(max(0.1, 5.0 / logf(de_eng_sum + 1e-9)), 1.0);
                for (i = de_clip_s; i < de_clip_e; ++i) {
                    fftx[i].a *= de_alpha2;
                    fftx[i].b *= de_alpha2;
                }
            }
        }
        if (vboxebf3->cfg->use_out_eq && vboxebf3->sp_sil) {
            float *eq_gain = vboxebf3->cfg->eq_gain;
            for (i = 0; i < nbin; ++i) {
                fftx[i].a *= eq_gain[i];
                fftx[i].b *= eq_gain[i];
            }
        }

        wtk_drft_frame_synthesis(rfft, rfft_in, synthesis_mem, fftx, out, wins,
                                 synthesis_window);
        if (vboxebf3->eq && vboxebf3->eq_enable) {
            wtk_equalizer_feed_float(vboxebf3->eq, out, fsize);
        }
        if (vboxebf3->limiter) {
            wtk_limiter_feed(vboxebf3->limiter, out, fsize);
        } else {
            wtk_vboxebf3_control_bs(vboxebf3, out, fsize);
        }
        for (i = 0; i < fsize; ++i) {
            pv[i] = floorf(out[i] + 0.5);
        }
        if (vboxebf3->notify) {
            vboxebf3->notify(vboxebf3->ths, pv, fsize);
        }
    }
    if (is_end && length > 0) {
        if (vboxebf3->notify) {
            pv = (short *)mic[0]->data;
            vboxebf3->notify(vboxebf3->ths, pv, length);
        }
    }
}

void wtk_vboxebf3_feed_mul_bf(wtk_vboxebf3_t *vboxebf3, wtk_complex_t **fft) {
    int i, k;
    int nbin = vboxebf3->nbin;
    int nmicchannel = vboxebf3->cfg->nmicchannel;
    int clip_s = vboxebf3->cfg->clip_s;
    int clip_e = vboxebf3->cfg->clip_e;
    int bf_clip_s = vboxebf3->cfg->bf_clip_s;
    int bf_clip_e = vboxebf3->cfg->bf_clip_e;
    wtk_complex_t ffts[64];
    wtk_complex_t ffty[64];
    float *scov;
    float *ncov;
    int *cnt_sum;
    float scov_alpha;
    float ncov_alpha;
    float scov_alpha_1;
    float ncov_alpha_1;
    int init_covnf;
    float w;
    float mu;
    float gf;

    if (vboxebf3->sp_sil) {
        if (vboxebf3->cfg->use_mul_bf) {
            for (i = 0; i < nmicchannel; ++i) {
                scov = vboxebf3->mul_scov[i];
                ncov = vboxebf3->mul_ncov[i];
                cnt_sum = vboxebf3->mul_cnt_sum;
                scov_alpha = vboxebf3->covm->cfg->scov_alpha;
                ncov_alpha = vboxebf3->covm->cfg->ncov_alpha;
                init_covnf = vboxebf3->covm->cfg->init_scovnf;
                scov_alpha_1 = 1.0 - scov_alpha;
                ncov_alpha_1 = 1.0 - ncov_alpha;
                for (k = clip_s + 1; k < clip_e; ++k) {
                    gf = vboxebf3->vdr->gf[k];
                    if (k >= bf_clip_s && k < bf_clip_e) {
                        mu = vboxebf3->cfg->bfmu;
                    } else {
                        mu = vboxebf3->cfg->bfmu2;
                    }
                    ffts[0].a = fft[i][k].a * gf;
                    ffts[0].b = fft[i][k].b * gf;
                    ffty[0].a = fft[i][k].a * (1 - gf);
                    ffty[0].b = fft[i][k].b * (1 - gf);

                    if (cnt_sum[k] < init_covnf) {
                        scov[k] +=
                            ffts[0].a * ffts[0].a + ffts[0].b * ffts[0].b;
                    } else if (cnt_sum[k] == init_covnf) {
                        scov[k] /= cnt_sum[k];
                    } else {
                        scov[k] = scov_alpha_1 * scov[k] +
                                  scov_alpha * (ffts[0].a * ffts[0].a +
                                                ffts[0].b * ffts[0].b);
                    }
                    if (cnt_sum[k] < init_covnf) {
                        ncov[k] +=
                            ffty[0].a * ffty[0].a + ffty[0].b * ffty[0].b;
                    } else if (cnt_sum[k] == init_covnf) {
                        ncov[k] /= cnt_sum[k];
                    } else {
                        ncov[k] = ncov_alpha_1 * ncov[k] +
                                  ncov_alpha * (ffty[0].a * ffty[0].a +
                                                ffty[0].b * ffty[0].b);
                    }
                    cnt_sum[k]++;
                    if (ncov[k] < 1e-10) {
                        ncov[k] = 1e-10;
                    }
                    w = scov[k] / (ncov[k]);
                    w = w / (mu + w);
                    fft[i][k].a = ffts[0].a * w;
                    fft[i][k].b = ffts[0].b * w;
                }
            }
        } else if (vboxebf3->cfg->use_mul_mask) {
            for (k = 1; k < nbin - 1; ++k) {
                gf = vboxebf3->vdr->gf[k];
                for (i = 0; i < nmicchannel; ++i) {
                    fft[i][k].a *= gf;
                    fft[i][k].b *= gf;
                }
            }
        }
    } else if (vboxebf3->sp_sil == 0) {
        if (vboxebf3->cfg->use_mul_echo_bf) {
            for (i = 0; i < nmicchannel; ++i) {
                scov = vboxebf3->mul_echo_scov[i];
                ncov = vboxebf3->mul_echo_ncov[i];
                cnt_sum = vboxebf3->mul_echo_cnt_sum;
                scov_alpha = vboxebf3->echo_covm->cfg->scov_alpha;
                ncov_alpha = vboxebf3->echo_covm->cfg->ncov_alpha;
                init_covnf = vboxebf3->echo_covm->cfg->init_scovnf;
                scov_alpha_1 = 1.0 - scov_alpha;
                ncov_alpha_1 = 1.0 - ncov_alpha;
                for (k = clip_s + 1; k < clip_e; ++k) {
                    gf = vboxebf3->vdr->gf[k];
                    if (k >= bf_clip_s && k < bf_clip_e) {
                        mu = vboxebf3->cfg->echo_bfmu;
                    } else {
                        mu = vboxebf3->cfg->echo_bfmu2;
                    }
                    ffts[0].a = fft[i][k].a * gf;
                    ffts[0].b = fft[i][k].b * gf;
                    ffty[0].a = fft[i][k].a * (1 - gf);
                    ffty[0].b = fft[i][k].b * (1 - gf);

                    if (cnt_sum[k] < init_covnf) {
                        scov[k] +=
                            ffts[0].a * ffts[0].a + ffts[0].b * ffts[0].b;
                    } else if (cnt_sum[k] == init_covnf) {
                        scov[k] /= cnt_sum[k];
                    } else {
                        scov[k] = scov_alpha_1 * scov[k] +
                                  scov_alpha * (ffts[0].a * ffts[0].a +
                                                ffts[0].b * ffts[0].b);
                    }
                    if (cnt_sum[k] < init_covnf) {
                        ncov[k] +=
                            ffty[0].a * ffty[0].a + ffty[0].b * ffty[0].b;
                    } else if (cnt_sum[k] == init_covnf) {
                        ncov[k] /= cnt_sum[k];
                    } else {
                        ncov[k] = ncov_alpha_1 * ncov[k] +
                                  ncov_alpha * (ffty[0].a * ffty[0].a +
                                                ffty[0].b * ffty[0].b);
                    }
                    cnt_sum[k]++;
                    if (ncov[k] < 1e-10) {
                        ncov[k] = 1e-10;
                    }
                    w = scov[k] / (ncov[k]);
                    w = w / (mu + w);
                    fft[i][k].a = ffts[0].a * w;
                    fft[i][k].b = ffts[0].b * w;
                }
            }
        } else if (vboxebf3->cfg->use_mul_echo_mask) {
            for (k = 1; k < nbin - 1; ++k) {
                gf = vboxebf3->vdr->gf[k];
                for (i = 0; i < nmicchannel; ++i) {
                    fft[i][k].a *= gf;
                    fft[i][k].b *= gf;
                }
            }
        }
    }
    if (vboxebf3->cfg->use_simple_bf) {
    }
}

void wtk_vboxebf3_feed_mul(wtk_vboxebf3_t *vboxebf3, short *data, int len,
                           int is_end) {
    int i, j, k;
    int nbin = vboxebf3->nbin;
    int nmicchannel = vboxebf3->cfg->nmicchannel;
    int *mic_channel = vboxebf3->cfg->mic_channel;
    int nspchannel = vboxebf3->cfg->nspchannel;
    int *sp_channel = vboxebf3->cfg->sp_channel;
    int channel = vboxebf3->cfg->channel;
    wtk_strbuf_t **mic = vboxebf3->mic;
    wtk_strbuf_t **sp = vboxebf3->sp;
    int wins = vboxebf3->cfg->wins;
    int fsize = wins / 2;
    int length;
    float spenr;
    float spenr_thresh = vboxebf3->cfg->spenr_thresh;
    int spenr_cnt = vboxebf3->cfg->spenr_cnt;
    wtk_drft_t *rfft = vboxebf3->rfft;
    float *rfft_in = vboxebf3->rfft_in;
    wtk_complex_t **fft = vboxebf3->fft;
    wtk_complex_t **fft_sp = vboxebf3->fft_sp;
    float **analysis_mem = vboxebf3->analysis_mem,
          **analysis_mem_sp = vboxebf3->analysis_mem_sp;
    float **mul_synthesis_mem = vboxebf3->mul_synthesis_mem;
    float *analysis_window = vboxebf3->analysis_window,
          *synthesis_window = vboxebf3->synthesis_window;
    float *mul_out = vboxebf3->mul_out;
    float *out = vboxebf3->out;
    short *pv = (short *)out;

    for (i = 0; i < len; ++i) {
        for (j = 0; j < nmicchannel; ++j) {
            wtk_strbuf_push(mic[j], (char *)(data + mic_channel[j]),
                            sizeof(short));
        }
        for (j = 0; j < nspchannel; ++j) {
            wtk_strbuf_push(sp[j], (char *)(data + sp_channel[j]),
                            sizeof(short));
        }
        data += channel;
    }
    length = mic[0]->pos / sizeof(short);
    if (nspchannel > 0) {
        length = min(length, sp[0]->pos / sizeof(short));
    }
    while (length >= fsize) {
        for (i = 0, j = nmicchannel; i < nspchannel; ++i, ++j) {
            wtk_drft_frame_analysis2(rfft, rfft_in, analysis_mem_sp[i],
                                     fft_sp[i], (short *)(sp[i]->data), wins,
                                     analysis_window);
        }
        if (nspchannel > 0) {
            spenr = wtk_vboxebf3_sp_energy((short *)sp[0]->data, fsize);
        } else {
            spenr = 0;
        }
        // static int cnt=0;
        // cnt++;
        if (spenr > spenr_thresh) {
            // if(vboxebf3->sp_sil==1)
            // {
            // 	printf("sp start %f %f
            // %f\n", 1.0/16000*cnt*(nbin-1),spenr,spenr_thresh);
            // }
            vboxebf3->sp_sil = 0;
            vboxebf3->sp_silcnt = spenr_cnt;
        } else if (vboxebf3->sp_sil == 0) {
            vboxebf3->sp_silcnt -= 1;
            if (vboxebf3->sp_silcnt <= 0) {
                // printf("sp end %f\n", 1.0/16000*cnt*(nbin-1));
                vboxebf3->sp_sil = 1;
            }
        }
        for (i = 0; i < nmicchannel; ++i) {
            wtk_drft_frame_analysis2(rfft, rfft_in, analysis_mem[i], fft[i],
                                     (short *)(mic[i]->data), wins,
                                     analysis_window);
        }
        if (vboxebf3->cfg->use_mic_scale) {
            float *mic_scale = vboxebf3->cfg->mic_scale;
            for (i = 0; i < nmicchannel; ++i) {
                for (k = 0; k < nbin; ++k) {
                    fft[i][k].a *= mic_scale[i];
                    fft[i][k].b *= mic_scale[i];
                }
            }
        }

        wtk_vboxebf3_feed_edra(vboxebf3, fft, fft_sp);
        wtk_vboxebf3_feed_mul_bf(vboxebf3, fft);

        wtk_strbufs_pop(mic, nmicchannel, fsize * sizeof(short));
        wtk_strbufs_pop(sp, nspchannel, fsize * sizeof(short));
        length = mic[0]->pos / sizeof(short);
        if (nspchannel > 0) {
            length = min(length, sp[0]->pos / sizeof(short));
        }

        for (i = 0; i < nmicchannel; ++i) {
            wtk_drft_frame_synthesis(rfft, rfft_in, mul_synthesis_mem[i],
                                     fft[i], mul_out, wins, synthesis_window);
            for (j = 0; j < fsize; ++j) {
                out[j * nmicchannel + i] = mul_out[j];
            }
        }

        for (i = 0; i < fsize * nmicchannel; ++i) {
            pv[i] = floorf(out[i] + 0.5);
        }
        if (vboxebf3->notify) {
            vboxebf3->notify(vboxebf3->ths, pv, fsize * nmicchannel);
        }
    }
    if (is_end && length > 0) {
        if (vboxebf3->notify) {
            pv = (short *)mic[0]->data;
            vboxebf3->notify(vboxebf3->ths, pv, length);
        }
    }
}

void wtk_vboxebf3_set_micvolume(wtk_vboxebf3_t *vboxebf3, float fscale) {
    vboxebf3->inmic_scale = fscale;
}

void wtk_vboxebf3_set_agcenable(wtk_vboxebf3_t *vboxebf3, int enable) {
    vboxebf3->agc_enable = enable;
}

void wtk_vboxebf3_set_agclevel(wtk_vboxebf3_t *vboxebf3, int level) {
    int n_agc_level = vboxebf3->cfg->n_agc_level;
    float *agc_model_level = vboxebf3->cfg->agc_model_level;
    float *qmmse_agc_level = vboxebf3->cfg->qmmse_agc_level;
    float *qmmse_max_gain = vboxebf3->cfg->qmmse_max_gain;
    float *gc_gain_level = vboxebf3->cfg->gc_gain_level;
    int i;

    level = max(min(level, n_agc_level), 1);
    vboxebf3->agc_level = level;

    if (vboxebf3->agc_on) {
        for (i = 0; i < n_agc_level; ++i) {
            if (level == i + 1) {
                vboxebf3->cfg->agc_a2 = agc_model_level[i];
                break;
            }
        }
    } else if (vboxebf3->qmmse2) {
        for (i = 0; i < n_agc_level; ++i) {
            if (level == i + 1) {
                vboxebf3->qmmse2->cfg->agc_level = qmmse_agc_level[i];
                vboxebf3->qmmse2->cfg->max_gain = qmmse_max_gain[i];
                break;
            }
        }
    } else if (vboxebf3->gc) {
        for (i = 0; i < n_agc_level; ++i) {
            if (level == i + 1) {
                vboxebf3->gc->kalman.Z_k = gc_gain_level[i];
                break;
            }
        }
    }
}

void wtk_vboxebf3_set_echoenable(wtk_vboxebf3_t *vboxebf3, int enable) {
    vboxebf3->echo_enable = enable;
}

void wtk_vboxebf3_set_denoiseenable(wtk_vboxebf3_t *vboxebf3, int enable) {
    vboxebf3->denoise_enable = enable;
    // if(enable==0)
    // {
    // 	wtk_vboxebf3_set_denoisesuppress(vboxebf3, 0);
    // }
}

void wtk_vboxebf3_set_echolevel(wtk_vboxebf3_t *vboxebf3, int level) {
    int n_aec_level = vboxebf3->cfg->n_aec_level;
    // float *aec_leak_scale = vboxebf3->cfg->aec_leak_scale;
    // int i;

    level = max(min(level, n_aec_level), 1);
    vboxebf3->aec_level = level;
    // for (i = 0; i < n_aec_level; ++i) {
    // 	if (level == i + 1) {
    // 		vboxebf3->cfg->leak_scale = aec_leak_scale[i];
    // 		break;
    // 	}
    // }
}

void wtk_vboxebf3_set_eqenable(wtk_vboxebf3_t *vboxebf3, int enable) {
    vboxebf3->eq_enable = enable;
}

void wtk_vboxebf3_set_denoisesuppress(wtk_vboxebf3_t *vboxebf3,
                                      float denoisesuppress) {
    if (denoisesuppress < 0.5) {
        vboxebf3->cfg->use_fftsbf = 0;
    } else {
        vboxebf3->cfg->use_fftsbf = 1;
    }
    // vboxebf3->cfg->bfmu=denoisesuppress;
    vboxebf3->cfg->gbias = 1 - denoisesuppress;
}

void wtk_vboxebf3_set_denoiselevel(wtk_vboxebf3_t *vboxebf3, int level) {
    int n_ans_level = vboxebf3->cfg->n_ans_level;
    float *qmmse_noise_suppress = vboxebf3->cfg->qmmse_noise_suppress;
    float *gbias_level = vboxebf3->cfg->gbias_level;
    int i;

    level = max(min(level, n_ans_level), 1);
    vboxebf3->ans_level = level;

    if (vboxebf3->qmmse2) {
        for (i = 0; i < n_ans_level; ++i) {
            if (level == i + 1) {
                vboxebf3->qmmse2->cfg->noise_suppress = qmmse_noise_suppress[i];
                break;
            }
        }
    }
    for (i = 0; i < n_ans_level; ++i) {
        if (level == i + 1) {
            wtk_vboxebf3_set_denoisesuppress(vboxebf3, gbias_level[i]);
            break;
        }
    }
}
void wtk_vboxebf3_ssl_delay_new(wtk_vboxebf3_t *vboxebf3) {
    if (vboxebf3->cfg->use_maskssl) {
        vboxebf3->maskssl = wtk_maskssl_new(&(vboxebf3->cfg->maskssl));
        wtk_maskssl_set_notify(vboxebf3->maskssl, vboxebf3,
                               (wtk_maskssl_notify_f)wtk_vboxebf3_on_ssl2);
    } else if (vboxebf3->cfg->use_maskssl2) {
        vboxebf3->maskssl2 = wtk_maskssl2_new(&(vboxebf3->cfg->maskssl2));
        wtk_maskssl2_set_notify(vboxebf3->maskssl2, vboxebf3,
                                (wtk_maskssl2_notify_f)wtk_vboxebf3_on_ssl2);
    }
    if (vboxebf3->maskssl) {
        wtk_maskssl_reset(vboxebf3->maskssl);
    }
    if (vboxebf3->maskssl2) {
        wtk_maskssl2_reset(vboxebf3->maskssl2);
    }
    vboxebf3->ssl_enable = 1;
}

void wtk_vboxebf3_set_sslenable(wtk_vboxebf3_t *vboxebf3, int enable) {
    vboxebf3->ssl_enable = enable;
    if (enable == 0) {
        if (vboxebf3->maskssl2) {
            wtk_maskssl2_feed_fft2(vboxebf3->maskssl2, NULL, NULL, 1);
        } else if (vboxebf3->maskssl) {
            wtk_maskssl_feed_fft2(vboxebf3->maskssl, NULL, NULL, 1);
        }
    }
}

void wtk_vboxebf3_set_delay(wtk_vboxebf3_t *vboxebf3, float mic_delay,
                            float sp_delay) {
    vboxebf3->mic_delay = mic_delay;
    vboxebf3->sp_delay = sp_delay;
    vboxebf3->mic_delay_samples = floor(mic_delay * vboxebf3->cfg->rate);
    vboxebf3->sp_delay_samples = floor(sp_delay * vboxebf3->cfg->rate);
}
