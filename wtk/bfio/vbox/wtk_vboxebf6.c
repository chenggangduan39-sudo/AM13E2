#include "wtk_vboxebf6.h"
void wtk_vboxebf6_aec_on_gainnet(wtk_vboxebf6_edra_t *vdr, float *gain, int len,
                                 int is_end);
void wtk_vboxebf6_denoise_on_gainnet(wtk_vboxebf6_edra_t *vdr, float *gain,
                                     int len, int is_end);
void wtk_vboxebf6_agc_on_gainnet(wtk_vboxebf6_edra_t *vdr, float *gain, int len,
                                 int is_end);
void wtk_vboxebf6_on_ssl2(wtk_vboxebf6_t *vboxebf6, wtk_ssl2_extp_t *nbest_extp,
                          int nbest, int ts, int te);

void wtk_vboxebf6_set_3A_level(wtk_vboxebf6_t *vboxebf6) {
    int n_agc_level = vboxebf6->cfg->n_agc_level;
    float *agc_model_level = vboxebf6->cfg->agc_model_level;
    float *qmmse_agc_level = vboxebf6->cfg->qmmse_agc_level;
    float *gc_gain_level = vboxebf6->cfg->gc_gain_level;
    int i;
    float min_dis = 0;
    int idx = 0;

    if (vboxebf6->agc_on) {
        min_dis = fabs(vboxebf6->cfg->agc_a2 - agc_model_level[0]);
        for (i = 1; i < n_agc_level; ++i) {
            if (fabs(vboxebf6->cfg->agc_a2 - agc_model_level[i]) < min_dis) {
                min_dis = fabs(vboxebf6->cfg->agc_a2 - agc_model_level[i]);
                idx = i;
            }
        }
    } else if (vboxebf6->qmmse2) {
        min_dis = fabs(vboxebf6->qmmse2->cfg->agc_level - qmmse_agc_level[0]);
        for (i = 1; i < n_agc_level; ++i) {
            if (fabs(vboxebf6->qmmse2->cfg->agc_level - qmmse_agc_level[i]) <
                min_dis) {
                min_dis =
                    fabs(vboxebf6->qmmse2->cfg->agc_level - qmmse_agc_level[i]);
                idx = i;
            }
        }
    } else if (vboxebf6->gc) {
        min_dis = fabs(vboxebf6->gc->kalman.Z_k - gc_gain_level[0]);
        for (i = 1; i < n_agc_level; ++i) {
            if (fabs(vboxebf6->gc->kalman.Z_k - gc_gain_level[i]) < min_dis) {
                min_dis = fabs(vboxebf6->gc->kalman.Z_k - gc_gain_level[i]);
                idx = i;
            }
        }
    }
    vboxebf6->agc_level = idx + 1;
    vboxebf6->aec_level = vboxebf6->cfg->n_aec_level;
    vboxebf6->ans_level = vboxebf6->cfg->n_ans_level;
}

void wtk_vboxebf6_edra_init(wtk_vboxebf6_edra_t *vdr, wtk_vboxebf6_cfg_t *cfg) {
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
                            (wtk_gainnet2_notify_f)wtk_vboxebf6_aec_on_gainnet);

    vdr->gainnet7 = NULL;
    if (cfg->gainnet7) {
        vdr->gainnet7 = wtk_gainnet7_new(cfg->gainnet7);
        wtk_gainnet7_set_notify(
            vdr->gainnet7, vdr,
            (wtk_gainnet7_notify_f)wtk_vboxebf6_denoise_on_gainnet);
    }
    vdr->agc_gainnet = NULL;
    if (cfg->agc_gainnet) {
        vdr->agc_gainnet = wtk_gainnet_new(cfg->agc_gainnet);
        wtk_gainnet_set_notify(
            vdr->agc_gainnet, vdr,
            (wtk_gainnet_notify_f)wtk_vboxebf6_agc_on_gainnet);
    }

    vdr->qmmse = NULL;
    if (cfg->use_qmmse) {
        vdr->qmmse = wtk_qmmse_new(&(cfg->qmmse));
    }
}

void wtk_vboxebf6_edra_clean(wtk_vboxebf6_edra_t *vdr) {
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

void wtk_vboxebf6_edra_reset(wtk_vboxebf6_edra_t *vdr) {
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

wtk_vboxebf6_t *wtk_vboxebf6_new(wtk_vboxebf6_cfg_t *cfg) {
    wtk_vboxebf6_t *vboxebf6;
    int i;

    vboxebf6 = (wtk_vboxebf6_t *)wtk_malloc(sizeof(wtk_vboxebf6_t));
    vboxebf6->cfg = cfg;
    vboxebf6->ths = NULL;
    vboxebf6->notify = NULL;
    vboxebf6->ssl_ths = NULL;
    vboxebf6->notify_ssl = NULL;
    vboxebf6->eng_ths = NULL;
    vboxebf6->notify_eng = NULL;

    vboxebf6->mic = wtk_strbufs_new(vboxebf6->cfg->nmicchannel);
    vboxebf6->mix = wtk_strbufs_new(vboxebf6->cfg->nmixchannel);
    vboxebf6->mic2 = NULL;
    if (vboxebf6->cfg->mic_channel2) {
        vboxebf6->mic2 = wtk_strbufs_new(vboxebf6->cfg->nmicchannel);
    }
    vboxebf6->sp = wtk_strbufs_new(vboxebf6->cfg->nspchannel);

    vboxebf6->nbin = cfg->wins / 2 + 1;
    vboxebf6->analysis_window = wtk_malloc(sizeof(float) * cfg->wins);  /// 2);
    vboxebf6->synthesis_window = wtk_malloc(sizeof(float) * cfg->wins); /// 2);
    vboxebf6->analysis_mem =
        wtk_float_new_p2(cfg->nmicchannel + 1, vboxebf6->nbin - 1);
    vboxebf6->analysis_mem_sp =
        wtk_float_new_p2(cfg->nspchannel, vboxebf6->nbin - 1);
    vboxebf6->synthesis_mem = wtk_malloc(sizeof(float) * (vboxebf6->nbin - 1));
    vboxebf6->rfft = wtk_drft_new(cfg->wins);
    vboxebf6->rfft_in = (float *)wtk_malloc(sizeof(float) * (cfg->wins));

    vboxebf6->fft = wtk_complex_new_p2(cfg->nmicchannel + 1, vboxebf6->nbin);
    vboxebf6->fft_sp = wtk_complex_new_p2(cfg->nspchannel, vboxebf6->nbin);

    vboxebf6->erls = NULL;
    vboxebf6->enlms = NULL;
    if (cfg->use_nlms) {
        vboxebf6->enlms = wtk_malloc(sizeof(wtk_nlms_t) * (vboxebf6->nbin));
        for (i = 0; i < vboxebf6->nbin; ++i) {
            wtk_nlms_init(vboxebf6->enlms + i, &(cfg->echo_nlms));
        }
    } else if (cfg->use_rls) {
        vboxebf6->erls = wtk_malloc(sizeof(wtk_rls_t) * (vboxebf6->nbin));
        for (i = 0; i < vboxebf6->nbin; ++i) {
            wtk_rls_init(vboxebf6->erls + i, &(cfg->echo_rls));
        }
    }
    vboxebf6->fftx =
        (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * vboxebf6->nbin);
    vboxebf6->ffty =
        (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * vboxebf6->nbin);
    vboxebf6->o_fft =
        (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * vboxebf6->nbin);
    vboxebf6->o_fft2 =
        (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * vboxebf6->nbin);

    vboxebf6->agc_on = 0;
    if (cfg->agc_gainnet) {
        vboxebf6->agc_on = 1;
    }

    vboxebf6->vdr =
        (wtk_vboxebf6_edra_t *)wtk_malloc(sizeof(wtk_vboxebf6_edra_t));
    wtk_vboxebf6_edra_init(vboxebf6->vdr, cfg);

    vboxebf6->covm = NULL;
    vboxebf6->echo_covm = NULL;
    if (!cfg->use_fixtheta) {
        vboxebf6->covm =
            wtk_covm_new(&(cfg->covm), vboxebf6->nbin, cfg->nbfchannel);
        vboxebf6->echo_covm = NULL;
        if (cfg->use_echocovm) {
            vboxebf6->echo_covm = wtk_covm_new(&(cfg->echo_covm),
                                               vboxebf6->nbin, cfg->nbfchannel);
        }
    }
    vboxebf6->bf = wtk_bf_new(&(cfg->bf), vboxebf6->cfg->wins);

    vboxebf6->eq = NULL;
    if (cfg->use_eq) {
        vboxebf6->eq = wtk_equalizer_new(&(cfg->eq));
    }

    vboxebf6->maskssl = NULL;
    vboxebf6->maskssl2 = NULL;
    if (cfg->use_maskssl && !cfg->use_ssl_delay) {
        vboxebf6->maskssl = wtk_maskssl_new(&(cfg->maskssl));
        wtk_maskssl_set_notify(vboxebf6->maskssl, vboxebf6,
                               (wtk_maskssl_notify_f)wtk_vboxebf6_on_ssl2);
    } else if (cfg->use_maskssl2 && !cfg->use_ssl_delay) {
        vboxebf6->maskssl2 = wtk_maskssl2_new(&(cfg->maskssl2));
        wtk_maskssl2_set_notify(vboxebf6->maskssl2, vboxebf6,
                                (wtk_maskssl2_notify_f)wtk_vboxebf6_on_ssl2);
    }

    vboxebf6->out = wtk_malloc(sizeof(float) * (vboxebf6->nbin - 1));
    vboxebf6->mix_out = wtk_malloc(sizeof(short) * (vboxebf6->nbin - 1));

    vboxebf6->qmmse2 = NULL;
    if (cfg->use_qmmse2) {
        vboxebf6->qmmse2 = wtk_qmmse_new(&(cfg->qmmse2));
    }
    vboxebf6->qmmse3 = NULL;
    if (cfg->use_qmmse3) {
        vboxebf6->qmmse3 = wtk_qmmse_new(&(cfg->qmmse3));
    }
    vboxebf6->qmmse4 = NULL;
    if (cfg->use_qmmse4) {
        vboxebf6->qmmse4 = wtk_qmmse_new(&(cfg->qmmse4));
    }
    vboxebf6->qmmse5 = NULL;
    if (cfg->use_qmmse5) {
        vboxebf6->qmmse5 = wtk_qmmse_new(&(cfg->qmmse5));
    }
    vboxebf6->gc = NULL;
    if (cfg->use_gc) {
        vboxebf6->gc = qtk_gain_controller_new(&(cfg->gc));
        qtk_gain_controller_set_mode(vboxebf6->gc, 0);
        vboxebf6->gc->kalman.Z_k = cfg->gc_gain;
    }

    vboxebf6->bs_win = NULL;
    if (cfg->use_bs_win) {
        vboxebf6->bs_win = wtk_math_create_hanning_window2(cfg->wins / 2);
    }

    vboxebf6->limiter = NULL;
    if (cfg->use_limiter) {
        vboxebf6->limiter = wtk_limiter_new(&(cfg->limiter));
    }

    vboxebf6->Yf = (float *)wtk_malloc(sizeof(float) * vboxebf6->nbin);

    vboxebf6->leak_fft = NULL;
    vboxebf6->leak_spec = NULL;
    vboxebf6->leak1 = NULL;
    vboxebf6->leak2 = NULL;
    vboxebf6->Ef = NULL;
    if (cfg->leak_frame > 0) {
        vboxebf6->leak_fft = (wtk_complex_t *)wtk_malloc(
            sizeof(wtk_complex_t) * vboxebf6->nbin * cfg->leak_frame);
        vboxebf6->leak_spec = (float *)wtk_malloc(
            sizeof(float) * vboxebf6->nbin * cfg->leak_frame);
        vboxebf6->leak1 = (float *)wtk_malloc(sizeof(float) * vboxebf6->nbin *
                                              (cfg->leak_frame + 1));
        vboxebf6->leak2 = (float *)wtk_malloc(sizeof(float) * vboxebf6->nbin *
                                              (cfg->leak_frame + 1));
        vboxebf6->Ef = (float *)wtk_malloc(sizeof(float) * vboxebf6->nbin);
    }

    wtk_vboxebf6_reset(vboxebf6);

    return vboxebf6;
}

void wtk_vboxebf6_delete(wtk_vboxebf6_t *vboxebf6) {
    int i;

    wtk_strbufs_delete(vboxebf6->mic, vboxebf6->cfg->nmicchannel);
    wtk_strbufs_delete(vboxebf6->mix, vboxebf6->cfg->nmixchannel);
    if (vboxebf6->mic2) {
        wtk_strbufs_delete(vboxebf6->mic2, vboxebf6->cfg->nmicchannel);
    }
    wtk_strbufs_delete(vboxebf6->sp, vboxebf6->cfg->nspchannel);

    wtk_free(vboxebf6->analysis_window);
    wtk_free(vboxebf6->synthesis_window);
    wtk_float_delete_p2(vboxebf6->analysis_mem, vboxebf6->cfg->nmicchannel + 1);
    wtk_float_delete_p2(vboxebf6->analysis_mem_sp, vboxebf6->cfg->nspchannel);
    wtk_free(vboxebf6->synthesis_mem);
    wtk_free(vboxebf6->rfft_in);
    wtk_drft_delete(vboxebf6->rfft);
    wtk_complex_delete_p2(vboxebf6->fft, vboxebf6->cfg->nmicchannel + 1);
    wtk_complex_delete_p2(vboxebf6->fft_sp, vboxebf6->cfg->nspchannel);

    if (vboxebf6->covm) {
        wtk_covm_delete(vboxebf6->covm);
    }
    if (vboxebf6->echo_covm) {
        wtk_covm_delete(vboxebf6->echo_covm);
    }
    wtk_bf_delete(vboxebf6->bf);

    if (vboxebf6->erls) {
        for (i = 0; i < vboxebf6->nbin; ++i) {
            wtk_rls_clean(vboxebf6->erls + i);
        }
        wtk_free(vboxebf6->erls);
    }
    if (vboxebf6->enlms) {
        for (i = 0; i < vboxebf6->nbin; ++i) {
            wtk_nlms_clean(vboxebf6->enlms + i);
        }
        wtk_free(vboxebf6->enlms);
    }

    if (vboxebf6->eq) {
        wtk_equalizer_delete(vboxebf6->eq);
    }

    wtk_free(vboxebf6->fftx);
    wtk_free(vboxebf6->ffty);
    wtk_free(vboxebf6->o_fft);
    wtk_free(vboxebf6->o_fft2);

    wtk_vboxebf6_edra_clean(vboxebf6->vdr);
    wtk_free(vboxebf6->vdr);

    if (vboxebf6->maskssl) {
        wtk_maskssl_delete(vboxebf6->maskssl);
    }
    if (vboxebf6->maskssl2) {
        wtk_maskssl2_delete(vboxebf6->maskssl2);
    }
    wtk_free(vboxebf6->out);
    wtk_free(vboxebf6->mix_out);
    if (vboxebf6->qmmse2) {
        wtk_qmmse_delete(vboxebf6->qmmse2);
    }
    if (vboxebf6->qmmse3) {
        wtk_qmmse_delete(vboxebf6->qmmse3);
    }
    if (vboxebf6->qmmse4) {
        wtk_qmmse_delete(vboxebf6->qmmse4);
    }
    if (vboxebf6->qmmse5) {
        wtk_qmmse_delete(vboxebf6->qmmse5);
    }
    if (vboxebf6->gc) {
        qtk_gain_controller_delete(vboxebf6->gc);
    }
    if (vboxebf6->bs_win) {
        wtk_free(vboxebf6->bs_win);
    }

    if (vboxebf6->limiter) {
        wtk_limiter_delete(vboxebf6->limiter);
    }

    if (vboxebf6->leak_fft) {
        wtk_free(vboxebf6->leak_fft);
    }
    if (vboxebf6->leak_spec) {
        wtk_free(vboxebf6->leak_spec);
    }
    if (vboxebf6->leak1) {
        wtk_free(vboxebf6->leak1);
    }
    if (vboxebf6->leak2) {
        wtk_free(vboxebf6->leak2);
    }
    if (vboxebf6->Ef) {
        wtk_free(vboxebf6->Ef);
    }
    wtk_free(vboxebf6->Yf);

    wtk_free(vboxebf6);
}

void wtk_vboxebf6_start(wtk_vboxebf6_t *vboxebf6) {
    wtk_bf_update_ovec(vboxebf6->bf, vboxebf6->cfg->theta, vboxebf6->cfg->phi);
    wtk_bf_init_w(vboxebf6->bf);
    int i;
    if (vboxebf6->mic_delay_samples > 0) {
        for (i = 0; i < vboxebf6->cfg->nmicchannel; ++i) {
            wtk_strbuf_push(vboxebf6->mic[i], NULL,
                            vboxebf6->mic_delay_samples * sizeof(short));
        }
        // wtk_strbufs_push_short(vboxebf6->mic, vboxebf6->cfg->nmicchannel,
        // NULL, vboxebf6->mic_delay_samples);
    }
    if (vboxebf6->sp_delay_samples > 0) {
        for (i = 0; i < vboxebf6->cfg->nspchannel; ++i) {
            wtk_strbuf_push(vboxebf6->sp[i], NULL,
                            vboxebf6->sp_delay_samples * sizeof(short));
        }
        // wtk_strbufs_push_short(vboxebf6->sp, vboxebf6->cfg->nspchannel, NULL,
        // vboxebf6->sp_delay_samples);
    }
}

void wtk_vboxebf6_reset(wtk_vboxebf6_t *vboxebf6) {
    int wins = vboxebf6->cfg->wins;
    int i, nbin = vboxebf6->nbin;

    wtk_strbufs_reset(vboxebf6->mic, vboxebf6->cfg->nmicchannel);
    wtk_strbufs_reset(vboxebf6->mix, vboxebf6->cfg->nmixchannel);
    if (vboxebf6->mic2) {
        wtk_strbufs_reset(vboxebf6->mic2, vboxebf6->cfg->nmicchannel);
    }
    wtk_strbufs_reset(vboxebf6->sp, vboxebf6->cfg->nspchannel);

    for (i = 0; i < wins; ++i) {
        vboxebf6->analysis_window[i] = sin((0.5 + i) * PI / (wins));
    }
    wtk_drft_init_synthesis_window(vboxebf6->synthesis_window,
                                   vboxebf6->analysis_window, wins);

    wtk_float_zero_p2(vboxebf6->analysis_mem, vboxebf6->cfg->nmicchannel + 1,
                      (vboxebf6->nbin - 1));
    wtk_float_zero_p2(vboxebf6->analysis_mem_sp, vboxebf6->cfg->nspchannel,
                      (vboxebf6->nbin - 1));
    memset(vboxebf6->synthesis_mem, 0, sizeof(float) * (vboxebf6->nbin - 1));

    wtk_complex_zero_p2(vboxebf6->fft, vboxebf6->cfg->nmicchannel + 1,
                        vboxebf6->nbin);
    wtk_complex_zero_p2(vboxebf6->fft_sp, vboxebf6->cfg->nspchannel,
                        vboxebf6->nbin);

    if (vboxebf6->covm) {
        wtk_covm_reset(vboxebf6->covm);
    }
    if (vboxebf6->echo_covm) {
        wtk_covm_reset(vboxebf6->echo_covm);
    }

    wtk_bf_reset(vboxebf6->bf);

    if (vboxebf6->erls) {
        for (i = 0; i < nbin; ++i) {
            wtk_rls_reset(vboxebf6->erls + i);
        }
    }
    if (vboxebf6->enlms) {
        for (i = 0; i < nbin; ++i) {
            wtk_nlms_reset(vboxebf6->enlms + i);
        }
    }

    memset(vboxebf6->ffty, 0, sizeof(wtk_complex_t) * (vboxebf6->nbin));
    memset(vboxebf6->fftx, 0, sizeof(wtk_complex_t) * (vboxebf6->nbin));
    memset(vboxebf6->o_fft, 0, sizeof(wtk_complex_t) * (vboxebf6->nbin));
    memset(vboxebf6->o_fft2, 0, sizeof(wtk_complex_t) * (vboxebf6->nbin));

    wtk_vboxebf6_edra_reset(vboxebf6->vdr);

    if (vboxebf6->qmmse2) {
        wtk_qmmse_reset(vboxebf6->qmmse2);
    }
    if (vboxebf6->qmmse3) {
        wtk_qmmse_reset(vboxebf6->qmmse3);
    }
    if (vboxebf6->qmmse4) {
        wtk_qmmse_reset(vboxebf6->qmmse4);
    }
    if (vboxebf6->qmmse5) {
        wtk_qmmse_reset(vboxebf6->qmmse5);
    }
    if (vboxebf6->gc) {
        qtk_gain_controller_reset(vboxebf6->gc);
    }
    if (vboxebf6->limiter) {
        wtk_limiter_reset(vboxebf6->limiter);
    }
    memset(vboxebf6->mix_out, 0, sizeof(short) * (vboxebf6->nbin - 1));
    memset(vboxebf6->Yf, 0, sizeof(float) * vboxebf6->nbin);

    if (vboxebf6->leak_fft) {
        memset(vboxebf6->leak_fft, 0,
               sizeof(wtk_complex_t) * vboxebf6->nbin *
                   vboxebf6->cfg->leak_frame);
        memset(vboxebf6->leak_spec, 0,
               sizeof(float) * vboxebf6->nbin * vboxebf6->cfg->leak_frame);
        memset(vboxebf6->leak1, 0,
               sizeof(float) * vboxebf6->nbin *
                   (vboxebf6->cfg->leak_frame + 1));
        memset(vboxebf6->leak2, 0,
               sizeof(float) * vboxebf6->nbin *
                   (vboxebf6->cfg->leak_frame + 1));
        memset(vboxebf6->Ef, 0, sizeof(float) * vboxebf6->nbin);
    }

    vboxebf6->sp_silcnt = 0;
    vboxebf6->sp_silcnt2 = 0;
    vboxebf6->sp_sil = 1;
    vboxebf6->sp_sil2 = 1;

    vboxebf6->mic_silcnt = 0;
    vboxebf6->mic_sil = 1;

    vboxebf6->bfflushnf = vboxebf6->cfg->bfflush_cnt;

    vboxebf6->ssl_enable = 0;
    if (vboxebf6->maskssl) {
        wtk_maskssl_reset(vboxebf6->maskssl);
        vboxebf6->ssl_enable = 1;
    }
    if (vboxebf6->maskssl2) {
        wtk_maskssl2_reset(vboxebf6->maskssl2);
        vboxebf6->ssl_enable = 1;
    }
    wtk_vboxebf6_set_3A_level(vboxebf6);

    vboxebf6->inmic_scale = 1.0;
    vboxebf6->agc_enable = 1;
    vboxebf6->echo_enable = 1;
    vboxebf6->denoise_enable = 1;
    vboxebf6->bs_scale = 1.0;
    vboxebf6->bs_last_scale = 1.0;
    vboxebf6->bs_real_scale = 1.0;
    vboxebf6->bs_max_cnt = 0;
    vboxebf6->gc_cnt = 0;

    vboxebf6->last_aec_alg_scale = 1.0;
    vboxebf6->last_denoise_alg_scale = 1.0;
    vboxebf6->last_alg_scale = 1.0;

    vboxebf6->entropy_in_cnt = 0;
    vboxebf6->entropy_out_cnt = 0;
    vboxebf6->entropy_state = 0;
    vboxebf6->entropy_state2 = 0;
    vboxebf6->sil_state = 0;

    vboxebf6->mic_delay = 0;
    vboxebf6->sp_delay = 0;
    vboxebf6->mic_delay_samples = 0;
    vboxebf6->sp_delay_samples = 0;
}

void wtk_vboxebf6_set_notify(wtk_vboxebf6_t *vboxebf6, void *ths,
                             wtk_vboxebf6_notify_f notify) {
    vboxebf6->notify = notify;
    vboxebf6->ths = ths;
}

void wtk_vboxebf6_set_ssl_notify(wtk_vboxebf6_t *vboxebf6, void *ths,
                                 wtk_vboxebf6_notify_ssl_f notify) {
    vboxebf6->notify_ssl = notify;
    vboxebf6->ssl_ths = ths;
}

void wtk_vboxebf6_set_eng_notify(wtk_vboxebf6_t *vboxebf6, void *ths,
                                 wtk_vboxebf6_notify_eng_f notify) {
    vboxebf6->notify_eng = notify;
    vboxebf6->eng_ths = ths;
}

static float wtk_vboxebf6_sp_energy(short *p, int n) {
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

static float wtk_vboxebf6_fft_energy(wtk_complex_t *fftx, int nbin) {
    float f;
    int i;

    f = 0;
    for (i = 1; i < nbin - 1; ++i) {
        f += fftx[i].a * fftx[i].a + fftx[i].b * fftx[i].b;
    }

    return f;
}

void wtk_vboxebf6_denoise_on_gainnet(wtk_vboxebf6_edra_t *vdr, float *gain,
                                     int len, int is_end) {
    memcpy(vdr->g, gain, sizeof(float) * vdr->cfg->bankfeat.nb_bands);
}

void wtk_vboxebf6_aec_on_gainnet(wtk_vboxebf6_edra_t *vdr, float *gain, int len,
                                 int is_end) {
    memcpy(vdr->g, gain, sizeof(float) * vdr->cfg->bankfeat.nb_bands);
}

void wtk_vboxebf6_agc_on_gainnet(wtk_vboxebf6_edra_t *vdr, float *gain, int len,
                                 int is_end) {
    memcpy(vdr->g2, gain, sizeof(float) * vdr->cfg->bankfeat.nb_bands);
}

void wtk_vboxebf6_edra_feed(wtk_vboxebf6_edra_t *vdr, wtk_complex_t *fftx,
                            wtk_complex_t *ffty, int sp_sil, float gbias,
                            int usecohv, float gf_mask_thresh, float *leak1,
                            float *leak2) {
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
    wtk_complex_t *fftytmp;
    // wtk_complex_t *fftytmp, sed, *fftxtmp;
    // float ef,yf;
    // float leak;

    wtk_bankfeat_flush_frame_features(bank_mic, fftx);
    if (usecohv) {
        fftytmp = ffty;
        for (i = 0; i < nbin; ++i, ++fftytmp) {
            fftytmp->a *= leak1[i];
            fftytmp->b *= leak1[i];

            gf[i] = leak2[i];
        }
        // for(i=0;i<nbin;++i,++fftxtmp,++fftytmp)
        // {
        // 	ef=fftxtmp->a*fftxtmp->a+fftxtmp->b*fftxtmp->b;
        // 	yf=fftytmp->a*fftytmp->a+fftytmp->b*fftytmp->b;
        // 	sed.a=fftytmp->a*fftxtmp->a+fftytmp->b*fftxtmp->b;
        // 	sed.b=-fftytmp->a*fftxtmp->b+fftytmp->b*fftxtmp->a;
        // 	leak=(sed.a*sed.a+sed.b*sed.b)/(max(ef,yf)*yf+1e-9);
        // 	leak=sqrtf(leak);
        // 	fftytmp->a*=leak;
        // 	fftytmp->b*=leak;
        // 	leak=(sed.a*sed.a+sed.b*sed.b)/(ef*yf+1e-9);
        // 	gf[i]=leak*yf;

        // }
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
        if (gf_mask_thresh >= 0) {
            if (wtk_float_mean(g, nb_bands) >= gf_mask_thresh) {
                gbias = 1;
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
}

void wtk_vboxebf6_aec_feed(wtk_vboxebf6_t *vboxebf6, wtk_complex_t *o_fft,
                           wtk_complex_t *ffty) {
    int i;
    int nbin = vboxebf6->nbin;
    wtk_qmmse_t *qmmse3 = vboxebf6->qmmse3;
    wtk_complex_t *fftytmp;
    float *Yf = vboxebf6->Yf;
    float leak_scale = vboxebf6->cfg->leak_scale;
    wtk_complex_t *o_fft2 = vboxebf6->o_fft2;

    if (vboxebf6->sp_sil == 1) {
        if (vboxebf6->qmmse2) {
            if (vboxebf6->cfg->use_qmmse_state && vboxebf6->qmmse3 &&
                vboxebf6->sil_state == 1) {
                memcpy(vboxebf6->qmmse2->noise, vboxebf6->qmmse3->noise,
                       sizeof(float) * vboxebf6->qmmse2->cfg->step);
                vboxebf6->sil_state = 0;
            }
            wtk_qmmse_denoise(vboxebf6->qmmse2, o_fft);
        } else if (qmmse3) {
            wtk_qmmse_denoise(qmmse3, o_fft);
        }
    } else {

        // fftytmp=ffty;
        // fftxtmp=o_fft;
        // for(i=0;i<nbin;++i,++fftxtmp,++fftytmp)
        // {
        // 	ef=fftxtmp->a*fftxtmp->a+fftxtmp->b*fftxtmp->b;
        // 	yf=fftytmp->a*fftytmp->a+fftytmp->b*fftytmp->b;
        // 	sed.a=fftytmp->a*fftxtmp->a+fftytmp->b*fftxtmp->b;
        // 	sed.b=-fftytmp->a*fftxtmp->b+fftytmp->b*fftxtmp->a;
        // 	leak=(sed.a*sed.a+sed.b*sed.b)/(max(ef,yf)*yf+1e-9);
        // 	leak=sqrtf(leak);
        // 	fftytmp->a*=leak;
        // 	fftytmp->b*=leak;
        // 	leak=(sed.a*sed.a+sed.b*sed.b)/(ef*yf+1e-9);
        // 	Yf[i]=leak*yf*leak_scale;
        // }

        float *leak1 = vboxebf6->leak1;
        float *leak2 = vboxebf6->leak2;
        fftytmp = ffty;
        for (i = 0; i < nbin; ++i, ++fftytmp) {
            fftytmp->a *= leak1[i];
            fftytmp->b *= leak1[i];

            Yf[i] = leak2[i] * leak_scale;
        }
        if (qmmse3) {
            if (vboxebf6->cfg->use_qmmse_state && vboxebf6->qmmse2 &&
                vboxebf6->sil_state == 0) {
                memcpy(vboxebf6->qmmse3->noise, vboxebf6->qmmse2->noise,
                       sizeof(float) * vboxebf6->qmmse3->cfg->step);
                vboxebf6->sil_state = 1;
            }
            wtk_qmmse_feed_echo_denoise(qmmse3, o_fft, Yf);
        }
        if (vboxebf6->qmmse5) {
            float amp1 = 0;
            float amp2 = 0;
            float scale = 0;
            memcpy(o_fft2, o_fft, sizeof(wtk_complex_t) * nbin);
            wtk_qmmse_denoise(vboxebf6->qmmse5, o_fft);

            for (i = 0; i < nbin; ++i) {
                amp1 = o_fft2[i].a * o_fft2[i].a + o_fft2[i].b * o_fft2[i].b;
                amp2 = o_fft[i].a * o_fft[i].a + o_fft[i].b * o_fft[i].b;
                scale = (amp1 - amp2) / (amp1 + 1e-9);
                scale = max(0.1, min(1.0, scale));
                o_fft[i].a = o_fft2[i].a * scale;
                o_fft[i].b = o_fft2[i].b * scale;
                // printf("%f\n", scale);
            }
        }
    }
}

void wtk_vboxebf6_feed_edra(wtk_vboxebf6_t *vboxebf6, wtk_complex_t **fft,
                            wtk_complex_t **fft_sp) {
    int nmicchannel = vboxebf6->cfg->nmicchannel;
    int nspchannel = vboxebf6->cfg->nspchannel;
    int i, j, k;
    int nbin = vboxebf6->nbin;
    wtk_rls_t *erls = vboxebf6->erls, *erlstmp;
    wtk_nlms_t *enlms = vboxebf6->enlms, *enlmstmp;
    wtk_complex_t *fftx = vboxebf6->fftx;
    wtk_complex_t ffttmp[64] = {0};
    wtk_complex_t fftsp2[10] = {0};
    wtk_complex_t *ffty = vboxebf6->ffty;
    wtk_vboxebf6_edra_t *vdr = vboxebf6->vdr;
    int usecohv = 1;
    wtk_complex_t *o_fft = vboxebf6->o_fft;

    int leak_frame = vboxebf6->cfg->leak_frame;
    wtk_complex_t *leak_fft = vboxebf6->leak_fft;
    float *leak_spec = vboxebf6->leak_spec;
    float *leak1 = vboxebf6->leak1;
    float *leak2 = vboxebf6->leak2;
    wtk_complex_t *fftytmp, sed, *fftxtmp;
    float *leak_tmp;
    float *Ef = vboxebf6->Ef;
    float ef, yf;
    float leak;

    if (vboxebf6->sp_sil == 1) {
        memset(o_fft, 0, sizeof(wtk_complex_t) * vboxebf6->nbin);
        for (i = 0; i < nbin; ++i) {
            for (j = 0; j < nmicchannel; ++j) {
                o_fft[i].a += fft[j][i].a;
                o_fft[i].b += fft[j][i].b;
            }
            o_fft[i].a *= 1.0 / nmicchannel;
            o_fft[i].b *= 1.0 / nmicchannel;
        }
        // memcpy(o_fft, fft[0], sizeof(wtk_complex_t)*vboxebf6->nbin);
    }
    if (erls) {
        erlstmp = erls;
        for (k = 0; k < nbin; ++k, ++erlstmp) {
            if (vboxebf6->cfg->use_erlssingle && vboxebf6->cfg->use_firstds) {
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
                          vboxebf6->sp_sil == 0 && vboxebf6->echo_enable);
            if (vboxebf6->sp_sil == 0 && vboxebf6->echo_enable) {
                ffty[k] = erlstmp->lsty[0];
                fftx[k] = erlstmp->out[0];
                if (vboxebf6->cfg->use_erlssingle) {
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
            if (vboxebf6->cfg->use_erlssingle && vboxebf6->cfg->use_firstds) {
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
                           vboxebf6->sp_sil == 0 && vboxebf6->echo_enable);
            if (vboxebf6->sp_sil == 0 && vboxebf6->echo_enable) {
                ffty[k] = enlmstmp->lsty[0];
                fftx[k] = enlmstmp->out[0];
                if (vboxebf6->cfg->use_erlssingle) {
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
    } else {
        usecohv = 0;
        for (k = 0; k < nbin; ++k) {
            fftx[k] = fft[0][k];
            ffty[k] = fft_sp[0][k];
        }
    }

    if (vboxebf6->sp_sil == 0) {
        memcpy(o_fft, fftx, sizeof(wtk_complex_t) * vboxebf6->nbin);

        if (leak_frame > 0) {
            memcpy(leak_fft, leak_fft + nbin,
                   sizeof(wtk_complex_t) * (leak_frame - 1) * nbin);
            memcpy(leak_fft + (leak_frame - 1) * nbin, ffty,
                   sizeof(wtk_complex_t) * nbin);
            memcpy(leak_spec, leak_spec + nbin,
                   sizeof(float) * (leak_frame - 1) * nbin);
        } else {
            memcpy(leak_fft, ffty, sizeof(wtk_complex_t) * nbin);
        }
        fftxtmp = fftx;
        for (i = 0; i < nbin; ++i, ++fftxtmp) {
            Ef[i] = fftxtmp->a * fftxtmp->a + fftxtmp->b * fftxtmp->b;
        }
        memset(leak1, 0, sizeof(float) * nbin * (leak_frame + 1));
        memset(leak2, 0, sizeof(float) * nbin * (leak_frame + 1));
        if (vboxebf6->cfg->use_frame_leak) {
            int idx = 0;
            float max_leak = 0;
            float sum_leak;
            for (j = 0; j < leak_frame; ++j) {
                fftytmp = leak_fft + j * nbin;
                fftxtmp = fftx;
                leak_tmp = leak_spec + j * nbin;
                sum_leak = 0;
                for (i = 0; i < nbin; ++i, ++fftxtmp, ++fftytmp) {
                    ef = Ef[i];
                    if (j < leak_frame - 1) {
                        yf = leak_tmp[i];
                    } else {
                        yf = fftytmp->a * fftytmp->a + fftytmp->b * fftytmp->b;
                        leak_tmp[i] = yf;
                    }
                    sed.a = fftytmp->a * fftxtmp->a + fftytmp->b * fftxtmp->b;
                    sed.b = -fftytmp->a * fftxtmp->b + fftytmp->b * fftxtmp->a;
                    leak = (sed.a * sed.a + sed.b * sed.b) /
                           (max(ef, yf) * yf + 1e-9);
                    leak = sqrtf(leak);
                    leak1[i + (j + 1) * nbin] = leak;
                    leak = (sed.a * sed.a + sed.b * sed.b) / (ef * yf + 1e-9);
                    leak2[i + (j + 1) * nbin] = leak * yf;
                    sum_leak +=
                        leak1[i + (j + 1) * nbin] + leak2[i + (j + 1) * nbin];
                }
                if (sum_leak > max_leak) {
                    max_leak = sum_leak;
                    idx = j;
                }
            }
            memcpy(leak1, leak1 + idx * nbin, sizeof(float) * nbin);
            memcpy(leak2, leak2 + idx * nbin, sizeof(float) * nbin);
        } else {
            for (j = 0; j < leak_frame; ++j) {
                fftytmp = leak_fft + j * nbin;
                fftxtmp = fftx;
                leak_tmp = leak_spec + j * nbin;
                for (i = 0; i < nbin; ++i, ++fftxtmp, ++fftytmp) {
                    ef = Ef[i];
                    if (j < leak_frame - 1) {
                        yf = leak_tmp[i];
                    } else {
                        yf = fftytmp->a * fftytmp->a + fftytmp->b * fftytmp->b;
                        leak_tmp[i] = yf;
                    }
                    sed.a = fftytmp->a * fftxtmp->a + fftytmp->b * fftxtmp->b;
                    sed.b = -fftytmp->a * fftxtmp->b + fftytmp->b * fftxtmp->a;
                    leak = (sed.a * sed.a + sed.b * sed.b) /
                           (max(ef, yf) * yf + 1e-9);
                    leak = sqrtf(leak);
                    leak1[i] = max(leak1[i], leak);
                    leak = (sed.a * sed.a + sed.b * sed.b) / (ef * yf + 1e-9);
                    leak2[i] = max(leak2[i], leak * yf);
                }
            }
        }
    }

    wtk_vboxebf6_edra_feed(
        vdr, fftx, ffty, vboxebf6->echo_enable ? vboxebf6->sp_sil : 1,
        vboxebf6->denoise_enable ? vboxebf6->cfg->gbias : 1.0, usecohv,
        vboxebf6->cfg->gf_mask_thresh, leak1, leak2);
    wtk_vboxebf6_aec_feed(vboxebf6, o_fft, ffty);
}

void wtk_vboxebf6_feed_bf(wtk_vboxebf6_t *vboxebf6, wtk_complex_t **fft) {
    wtk_complex_t *fftx = vboxebf6->fftx;
    int k, nbin = vboxebf6->nbin;
    int i;
    int nbfchannel = vboxebf6->cfg->nbfchannel;
    wtk_bf_t *bf = vboxebf6->bf;
    wtk_covm_t *covm;
    int b;
    wtk_vboxebf6_edra_t *vdr = vboxebf6->vdr;
    float gf;
    wtk_complex_t fft2[64];
    wtk_complex_t ffts[64];
    wtk_complex_t ffty[64];
    int clip_s = vboxebf6->cfg->clip_s;
    int clip_e = vboxebf6->cfg->clip_e;
    int bfflush_cnt = vboxebf6->cfg->bfflush_cnt;

    fftx[0].a = fftx[0].b = 0;
    fftx[nbin - 1].a = fftx[nbin - 1].b = 0;
    if (vboxebf6->cfg->use_fixtheta) {
        for (k = 1; k < nbin - 1; ++k) {
            gf = vdr->gf[k];
            for (i = 0; i < nbfchannel; ++i) {
                ffts[i].a = fft[i][k].a * gf;
                ffts[i].b = fft[i][k].b * gf;
            }
            wtk_bf_output_fft_k(bf, ffts, fftx + k, k);
        }
    } else {
        if (vboxebf6->sp_sil == 0 && vboxebf6->echo_enable) {
            if (vboxebf6->sp_sil2 == 0) {
                bf->cfg->mu = vboxebf6->cfg->echo_bfmu2;
            } else {
                bf->cfg->mu = vboxebf6->cfg->echo_bfmu;
            }
            covm = vboxebf6->echo_covm == NULL ? vboxebf6->covm
                                               : vboxebf6->echo_covm;
        } else {
            bf->cfg->mu = vboxebf6->cfg->bfmu;
            covm = vboxebf6->covm;
        }
        for (k = clip_s + 1; k < clip_e; ++k) {
            gf = vdr->gf[k];
            for (i = 0; i < nbfchannel; ++i) {
                ffts[i].a = fft[i][k].a * gf;
                ffts[i].b = fft[i][k].b * gf;

                ffty[i].a = fft[i][k].a * (1 - gf);
                ffty[i].b = fft[i][k].b * (1 - gf);
            }
            if ((vboxebf6->sp_sil == 1 && vboxebf6->cfg->use_fftsbf) ||
                (vboxebf6->sp_sil == 0 && vboxebf6->cfg->use_efftsbf)) {
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
            if (b == 1 && vboxebf6->bfflushnf == bfflush_cnt) {
                wtk_bf_update_w(bf, k);
            }
            wtk_bf_output_fft_k(bf, fft2, fftx + k, k);
            // fftx[k]=fft[0][k];
        }
        --vboxebf6->bfflushnf;
        if (vboxebf6->bfflushnf == 0) {
            vboxebf6->bfflushnf = bfflush_cnt;
        }
    }
    for (k = 0; k <= clip_s; ++k) {
        fftx[k].a = fftx[k].b = 0;
    }
    for (k = clip_e; k < nbin; ++k) {
        fftx[k].a = fftx[k].b = 0;
    }
}

void wtk_vboxebf6_feed_agc2(wtk_vboxebf6_t *vboxebf6, wtk_complex_t *fft) {
    wtk_vboxebf6_edra_t *vdr = vboxebf6->vdr;
    int nbin = vdr->nbin;
    float *gf2 = vdr->gf2;
    float gf;
    int i, j;
    float *freq_mask = vboxebf6->cfg->freq_mask;
    int bands_step = (nbin - 1) / vboxebf6->cfg->bankfeat.nb_bands;
    wtk_bankfeat_t *bankfeat = vboxebf6->vdr->bank_mic;
    int nb_bands = bankfeat->cfg->nb_bands;
    float E = 0;
    float *Ex = bankfeat->Ex;
    float agcaddg = vboxebf6->cfg->agcaddg;

    wtk_bankfeat_compute_band_energy(bankfeat, Ex, fft);
    for (i = 0; i < nb_bands; ++i) {
        E += Ex[i];
    }

    if (vboxebf6->agc_enable && !vdr->bank_mic->silence &&
        E > vboxebf6->cfg->agce_thresh) {
        if (vboxebf6->cfg->use_agcmean) {
            gf = wtk_float_abs_mean(gf2 + 1, nbin - 2);
            if (vboxebf6->cfg->use_freq_mask) {
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
            if (vboxebf6->cfg->use_freq_mask) {
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

void wtk_vboxebf6_feed_cnon(wtk_vboxebf6_t *vboxebf6, wtk_complex_t *fft) {
    wtk_vboxebf6_edra_t *vdr = vboxebf6->vdr;
    int nbin = vdr->nbin;
    float *gf = vdr->gf;
    float sym = vboxebf6->cfg->sym;
    static float fx = 2.0f * PI / RAND_MAX;
    float f, f2;
    int i;

    for (i = 1; i < nbin - 1; ++i) {
        f = rand() * fx;
        f2 = 1.f - gf[i] * gf[i];
        if (f2 > 0) {
            // f2=sqrtf(f2);
            fft[i].a += sym * cosf(f) * f2;
            fft[i].b += sym * sinf(f) * f2;
        }
    }
}

void wtk_vboxebf6_on_ssl2(wtk_vboxebf6_t *vboxebf6, wtk_ssl2_extp_t *nbest_extp,
                          int nbest, int ts, int te) {
    // printf("[%d %d] ==> %d %d %f\n", ts,te, nbest_extp[0].theta,
    // nbest_extp[0].phi, nbest_extp[0].nspecsum);
    if (vboxebf6->notify_ssl) {
        vboxebf6->notify_ssl(vboxebf6->ssl_ths, ts, te, nbest_extp, nbest);
    }
}

void wtk_vboxebf6_control_bs(wtk_vboxebf6_t *vboxebf6, float *out, int len) {
    float *bs_win = vboxebf6->bs_win;
    float max_out = vboxebf6->cfg->max_out;
    float out_max;
    int i;

    if (vboxebf6->mic_sil == 0) {
        out_max = wtk_float_abs_max(out, len);
        if (out_max > max_out) {
            vboxebf6->bs_scale = max_out / out_max;
            if (vboxebf6->bs_scale < vboxebf6->bs_last_scale) {
                vboxebf6->bs_last_scale = vboxebf6->bs_scale;
            } else {
                vboxebf6->bs_scale = vboxebf6->bs_last_scale;
            }
            vboxebf6->bs_max_cnt = 5;
        }
        if (bs_win) {
            for (i = 0; i < len / 2; ++i) {
                out[i] *= vboxebf6->bs_scale * bs_win[i] +
                          vboxebf6->bs_real_scale * (1.0 - bs_win[i]);
            }
            for (i = len / 2; i < len; ++i) {
                out[i] *= vboxebf6->bs_scale;
            }
            vboxebf6->bs_real_scale = vboxebf6->bs_scale;
        } else {
            for (i = 0; i < len; ++i) {
                out[i] *= vboxebf6->bs_scale;
            }
        }
        if (vboxebf6->bs_max_cnt > 0) {
            --vboxebf6->bs_max_cnt;
        }
        if (vboxebf6->bs_max_cnt <= 0 && vboxebf6->bs_scale < 1.0) {
            vboxebf6->bs_scale *= 1.1f;
            vboxebf6->bs_last_scale = vboxebf6->bs_scale;
            if (vboxebf6->bs_scale > 1.0) {
                vboxebf6->bs_scale = 1.0;
                vboxebf6->bs_last_scale = 1.0;
            }
        }
    } else {
        vboxebf6->bs_scale = 1.0;
        vboxebf6->bs_last_scale = 1.0;
        vboxebf6->bs_max_cnt = 0;
    }
}

void wtk_vboxebf6_norm_mix(wtk_vboxebf6_t *vboxebf6, short **data, int len) {
    int nmixchannel = vboxebf6->cfg->nmixchannel;
    int i, j;
    int max_1 = vboxebf6->cfg->max_1;
    int min_1 = vboxebf6->cfg->min_1;
    int max_2 = vboxebf6->cfg->max_2;
    int min_2 = vboxebf6->cfg->min_2;
    int f_win = vboxebf6->cfg->f_win;
    static float f = 1.0;
    float min_val;
    int output;
    int scale = vboxebf6->cfg->scale;

    for (i = 0; i < len; ++i) {
        min_val = 0;
        for (j = 0; j < nmixchannel; ++j) {
            min_val += data[j][i] * scale;
        }
        output = (int)(min_val * f);
        if (output > max_2) {
            f = (1.0 * max_2) * (1.0 / output);
            output = min(max_1, output);
        }
        if (output < min_2) {
            f = (1.0 * min_2) * (1.0 / output);
            output = max(min_1, output);
        }
        if (f < 1.0) {
            f += (1.0 - f) * 1.0 / f_win;
        }
        // printf("%f\n", f);
        data[nmixchannel][i] = (short)output;
    }
}
static void _qtk_vector_cpx_mag_squared(wtk_complex_t *a, float *d, int numSamples) {
    int i;
    for (i = 0; i < numSamples; i++) {
        d[i] = a[i].a * a[i].a + a[i].b * a[i].b;
    }
}

float wtk_vboxebf6_entropy(wtk_vboxebf6_t *vboxebf6, wtk_complex_t *fftx) {
    int rate = vboxebf6->cfg->rate;
    int wins = vboxebf6->cfg->wins;
    int i;
    int fx1 = (250 * 1.0 * wins) / rate;
    int fx2 = (3500 * 1.0 * wins) / rate;
    int km = floor(wins * 1.0 / 8);
    float K = 0.5;
    float E[1024] = {0};
    float P1;
    float Eb[1024] = {0};
    float sum;
    float prob;
    float Hb;

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
        Eb[i] += E[i * 4] + E[i * 4 + 1] + E[i * 4 + 2] + E[i * 4 + 3];
        sum += Eb[i];
    }
    Hb = 0;
    for (i = 0; i < wins; ++i) {
        prob = Eb[i] / sum;
        Hb += -prob * logf(prob + 1e-10);
    }
    // printf("%f\n", Hb);

    return Hb;
}

void wtk_vboxebf6_feed(wtk_vboxebf6_t *vboxebf6, short *data, int len,
                       int is_end) {
    int i, j;
    int nbin = vboxebf6->nbin;
    int nmicchannel = vboxebf6->cfg->nmicchannel;
    int *mic_channel = vboxebf6->cfg->mic_channel;
    int nmixchannel = vboxebf6->cfg->nmixchannel;
    int *mix_channel = vboxebf6->cfg->mix_channel;
    int *mic_channel2 = vboxebf6->cfg->mic_channel2;
    int nspchannel = vboxebf6->cfg->nspchannel;
    int *sp_channel = vboxebf6->cfg->sp_channel;
    int channel = vboxebf6->cfg->channel;
    wtk_strbuf_t **mic = vboxebf6->mic;
    wtk_strbuf_t **mix = vboxebf6->mix;
    wtk_strbuf_t **mic2 = vboxebf6->mic2;
    wtk_strbuf_t **sp = vboxebf6->sp;
    int wins = vboxebf6->cfg->wins;
    int fsize = wins / 2;
    int length;
    float spenr;
    float spenr_thresh = vboxebf6->cfg->spenr_thresh;
    float spenr_thresh2 = vboxebf6->cfg->spenr_thresh2;
    int spenr_cnt = vboxebf6->cfg->spenr_cnt;
    int spenr_cnt2 = vboxebf6->cfg->spenr_cnt2;
    float micenr;
    float micenr_thresh = vboxebf6->cfg->micenr_thresh;
    int micenr_cnt = vboxebf6->cfg->micenr_cnt;
    wtk_drft_t *rfft = vboxebf6->rfft;
    float *rfft_in = vboxebf6->rfft_in;
    wtk_complex_t **fft = vboxebf6->fft;
    wtk_complex_t **fft_sp = vboxebf6->fft_sp;
    float **analysis_mem = vboxebf6->analysis_mem,
          **analysis_mem_sp = vboxebf6->analysis_mem_sp;
    float *synthesis_mem = vboxebf6->synthesis_mem;
    float *analysis_window = vboxebf6->analysis_window,
          *synthesis_window = vboxebf6->synthesis_window;
    wtk_complex_t *fftx = vboxebf6->fftx;
    wtk_complex_t *o_fft = vboxebf6->o_fft;
    float *out = vboxebf6->out;
    short *pv = (short *)out;
    float mic_energy = 0;
    float energy = 0;
    float snr = 0;
    int clip_s = vboxebf6->cfg->clip_s;
    int clip_e = vboxebf6->cfg->clip_e;
    int denoise_clip_s = vboxebf6->cfg->denoise_clip_s;
    int denoise_clip_e = vboxebf6->cfg->denoise_clip_e;
    int aec_clip_s = vboxebf6->cfg->aec_clip_s;
    int aec_clip_e = vboxebf6->cfg->aec_clip_e;
    float denoise_mdl_scale = vboxebf6->cfg->denoise_mdl_scale;
    float denoise_alg_scale;
    float aec_mdl_scale = vboxebf6->cfg->aec_mdl_scale;
    float aec_alg_scale = vboxebf6->cfg->aec_alg_scale;
    float qmmse_mask_scale = vboxebf6->cfg->qmmse_mask_scale;
    float entropy;
    float entropy_cur_scale;

    for (i = 0; i < len; ++i) {
        for (j = 0; j < nmicchannel; ++j) {
            wtk_strbuf_push(mic[j], (char *)(data + mic_channel[j]),
                            sizeof(short));
        }
        for (j = 0; j < nmixchannel; ++j) {
            wtk_strbuf_push(mix[j], (char *)(data + mix_channel[j]),
                            sizeof(short));
        }
        for (j = 0; j < nspchannel; ++j) {
            wtk_strbuf_push(sp[j], (char *)(data + sp_channel[j]),
                            sizeof(short));
        }
        if (mic2) {
            for (j = 0; j < nmicchannel; ++j) {
                wtk_strbuf_push(mic2[j], (char *)(data + mic_channel2[j]),
                                sizeof(short));
            }
        }
        data += channel;
    }
    length = mic[0]->pos / sizeof(short);
    while (length >= fsize) {
        if (vboxebf6->notify_eng) {
            mic_energy = sqrtf(wtk_vboxebf6_fft_energy(fft[0], nbin) * 2);
        }
        for (i = 0, j = nmicchannel; i < nspchannel; ++i, ++j) {
            wtk_drft_frame_analysis2(rfft, rfft_in, analysis_mem_sp[i],
                                     fft_sp[i], (short *)(sp[i]->data), wins,
                                     analysis_window);
        }
        if (nspchannel > 0) {
            spenr = wtk_vboxebf6_sp_energy((short *)sp[0]->data, fsize);
        } else {
            spenr = 0;
        }
        // static int cnt=0;
        // cnt++;
        if (spenr > spenr_thresh) {
            // if(vboxebf6->sp_sil==1)
            // {
            // 	printf("sp start %f %f
            // %f\n", 1.0/16000*cnt*(nbin-1),spenr,spenr_thresh);
            // }
            vboxebf6->sp_sil = 0;
            vboxebf6->sp_silcnt = spenr_cnt;
        } else if (vboxebf6->sp_sil == 0) {
            vboxebf6->sp_silcnt -= 1;
            if (vboxebf6->sp_silcnt <= 0) {
                // printf("sp end %f\n", 1.0/16000*cnt*(nbin-1));
                vboxebf6->sp_sil = 1;
            }
        }

        if (spenr > spenr_thresh2) {
            // if(vboxebf6->sp_sil2==1)
            // {
            // 	printf("sp start %f %f
            // %f\n", 1.0/16000*cnt*(nbin-1),spenr,spenr_thresh);
            // }
            vboxebf6->sp_sil2 = 0;
            vboxebf6->sp_silcnt2 = spenr_cnt2;
        } else if (vboxebf6->sp_sil2 == 0) {
            vboxebf6->sp_silcnt2 -= 1;
            if (vboxebf6->sp_silcnt2 <= 0) {
                // printf("sp end %f\n", 1.0/16000*cnt*(nbin-1));
                vboxebf6->sp_sil2 = 1;
            }
        }

        if (mic2) {
            for (i = 0; i < nmicchannel; ++i) {
                wtk_drft_frame_analysis2(rfft, rfft_in, analysis_mem[i], fft[i],
                                         (short *)(mic2[i]->data), wins,
                                         analysis_window);
            }
        } else {
            for (i = 0; i < nmicchannel; ++i) {
                wtk_drft_frame_analysis2(rfft, rfft_in, analysis_mem[i], fft[i],
                                         (short *)(mic[i]->data), wins,
                                         analysis_window);
            }
        }
        if (vboxebf6->agc_enable == 0 && vboxebf6->inmic_scale > 1.0 && vboxebf6->denoise_enable) {
            for (i = 0; i < nmicchannel; ++i) {
                for (j = 0; j < nbin; ++j) {
                    fft[i][j].a *= vboxebf6->inmic_scale;
                    fft[i][j].b *= vboxebf6->inmic_scale;
                }
            }
        }
        if (vboxebf6->sp_sil == 1) {
            memcpy(o_fft, fft[0], sizeof(wtk_complex_t) * nbin);
            memset(vboxebf6->Yf, 0, sizeof(float) * vboxebf6->nbin);
        }
        wtk_vboxebf6_feed_edra(vboxebf6, fft, fft_sp);
        wtk_vboxebf6_feed_bf(vboxebf6, fft);

        if (vboxebf6->notify_eng) {
            energy = sqrtf(wtk_vboxebf6_fft_energy(fftx, nbin) * 2);
            snr = 20 *
                  log10(max(
                      floor(energy / max(abs(mic_energy - energy), 1e-6) + 0.5),
                      1.0));
            energy =
                20 * log10(max(floor(energy + 0.5), 1.0)) - 20 * log10(32768.0);

            // wtk_debug("%f %f\n", energy, snr);
            vboxebf6->notify_eng(vboxebf6->eng_ths, energy, snr);
        }

        float gc_mask = wtk_float_abs_mean(vboxebf6->vdr->gf, nbin);
        if (gc_mask > vboxebf6->cfg->gc_min_thresh) {
            vboxebf6->gc_cnt = vboxebf6->cfg->gc_cnt;
        } else {
            --vboxebf6->gc_cnt;
        }
        if (vboxebf6->sp_sil == 1) {
            if (vboxebf6->denoise_enable == 0) {
                memcpy(fftx, fft[0], sizeof(wtk_complex_t) * nbin);
            }
        }
        if (vboxebf6->cfg->b_agc_scale != 1.0 && vboxebf6->agc_enable)
        {
            float b_agc_scale = vboxebf6->cfg->b_agc_scale;
            for (i = 0; i < nbin; ++i)
            {
                fftx[i].a *= b_agc_scale;
                fftx[i].b *= b_agc_scale;
            }
        }
        if (vboxebf6->gc_cnt >= 0) {
            if (vboxebf6->agc_enable) {
                if (vboxebf6->agc_on) {
                    wtk_vboxebf6_feed_agc2(vboxebf6, fftx);
                } else if (vboxebf6->gc) {
                    qtk_gain_controller_run(vboxebf6->gc, fftx, fsize, NULL,
                                            gc_mask);
                }
            }
        } else {
            memset(fftx, 0, sizeof(wtk_complex_t) * nbin);
        }

        micenr = wtk_vboxebf6_fft_energy(fftx, nbin);
        // printf("%f\n", micenr);
        if (micenr > micenr_thresh) {
            // if(vboxebf6->mic_sil==1)
            // {
            // 	printf("sp start %f %f
            // %f\n", 1.0/16000*cnt*(nbin-1),micenr,micenr_thresh);
            // }
            vboxebf6->mic_sil = 0;
            vboxebf6->mic_silcnt = micenr_cnt;
        } else if (vboxebf6->mic_sil == 0) {
            vboxebf6->mic_silcnt -= 1;
            if (vboxebf6->mic_silcnt <= 0) {
                // printf("sp end %f\n", 1.0/16000*cnt*(nbin-1));
                vboxebf6->mic_sil = 1;
            }
        }
        if (vboxebf6->mic_sil == 0) {
            denoise_alg_scale = vboxebf6->cfg->denoise_alg_scale2;
            aec_alg_scale = vboxebf6->cfg->aec_alg_scale2;
        } else {
            denoise_alg_scale = vboxebf6->cfg->denoise_alg_scale3;
            aec_alg_scale = vboxebf6->cfg->aec_alg_scale3;
        }

        if (vboxebf6->sp_sil2 == 1) {
            denoise_alg_scale = vboxebf6->cfg->denoise_alg_scale;
            aec_alg_scale = vboxebf6->cfg->aec_alg_scale;
        }

        vboxebf6->last_denoise_alg_scale =
            vboxebf6->last_denoise_alg_scale *
                vboxebf6->cfg->denoise_alg_alpha +
            denoise_alg_scale * (1.0 - vboxebf6->cfg->denoise_alg_alpha);
        vboxebf6->last_aec_alg_scale =
            vboxebf6->last_aec_alg_scale * vboxebf6->cfg->aec_alg_alpha +
            aec_alg_scale * (1.0 - vboxebf6->cfg->aec_alg_alpha);

        if (vboxebf6->cfg->alg_alpha == -1) {
            aec_alg_scale = vboxebf6->last_aec_alg_scale;
            denoise_alg_scale = vboxebf6->last_denoise_alg_scale;
        } else {
            if (vboxebf6->sp_sil2 == 1) {
                vboxebf6->last_alg_scale =
                    vboxebf6->last_alg_scale * vboxebf6->cfg->alg_alpha +
                    denoise_alg_scale * (1.0 - vboxebf6->cfg->alg_alpha);
            } else {
                vboxebf6->last_alg_scale =
                    vboxebf6->last_alg_scale * vboxebf6->cfg->alg_alpha +
                    aec_alg_scale * (1.0 - vboxebf6->cfg->alg_alpha);
            }
            aec_alg_scale = vboxebf6->last_alg_scale;
            denoise_alg_scale = vboxebf6->last_alg_scale;
        }
        if (vboxebf6->cfg->entropy_scale != 1.0) {
            entropy = wtk_vboxebf6_entropy(vboxebf6, fftx);
            micenr = wtk_vboxebf6_fft_energy(fft[0], nbin);
            float e_th = vboxebf6->cfg->e_th;
            if (entropy > vboxebf6->cfg->entropy_thresh &&
                ((e_th > 0 && micenr < e_th) || e_th <= 0)) {
                --vboxebf6->entropy_in_cnt;
                vboxebf6->entropy_state = 0;
                vboxebf6->entropy_state2 = 0;
            } else {
                if (vboxebf6->entropy_state == 1) {
                    if (vboxebf6->entropy_in_cnt <= 0) {
                        vboxebf6->entropy_state2 = 1;
                    } else {
                        vboxebf6->entropy_state2 = 0;
                    }
                    vboxebf6->entropy_in_cnt = vboxebf6->cfg->entropy_cnt;
                } else {
                    vboxebf6->entropy_state = 1;
                }
            }
            if (vboxebf6->entropy_state2 == 1) {
                vboxebf6->entropy_out_cnt = vboxebf6->cfg->entropy_in_cnt;
            }
            if (vboxebf6->entropy_in_cnt <= 0) {
                entropy_cur_scale = vboxebf6->cfg->entropy_scale;
            } else if (vboxebf6->entropy_in_cnt <=
                       vboxebf6->cfg->entropy_cnt / 2) {
                entropy_cur_scale =
                    vboxebf6->cfg->entropy_scale +
                    (1.0 - vboxebf6->cfg->entropy_scale) *
                        pow((1.0 - (vboxebf6->cfg->entropy_cnt / 2 -
                                    vboxebf6->entropy_in_cnt) *
                                       1.0 / (vboxebf6->cfg->entropy_cnt / 2)),
                            0.1);
            } else if (vboxebf6->entropy_out_cnt > 0) {
                --vboxebf6->entropy_out_cnt;
                if (vboxebf6->cfg->entropy_in_scale != 1.0) {
                    entropy_cur_scale =
                        vboxebf6->cfg->entropy_in_scale +
                        (1.0 - vboxebf6->cfg->entropy_in_scale) *
                            pow(((vboxebf6->cfg->entropy_in_cnt -
                                  vboxebf6->entropy_out_cnt) *
                                 1.0 / (vboxebf6->cfg->entropy_in_cnt)),
                                3);
                } else {
                    entropy_cur_scale = 1.0;
                }
            } else {
                entropy_cur_scale = 1.0;
            }
            // printf("%d\n", vboxebf6->entropy_in_cnt);
            // printf("%d\n", vboxebf6->entropy_out_cnt);
            // printf("%f\n", denoise_alg_scale);
            denoise_alg_scale *= entropy_cur_scale;
            aec_alg_scale *= entropy_cur_scale;
        }

        if (vboxebf6->sp_sil == 1) {
            if (vboxebf6->denoise_enable) {
                if (vboxebf6->cfg->use_denoise_mdl) {
                    for (i = 0; i < denoise_clip_s; ++i) {
                        fftx[i].a = 0;
                        fftx[i].b = 0;
                    }
                    for (i = denoise_clip_s; i < denoise_clip_e; ++i) {
                        fftx[i].a *= denoise_mdl_scale;
                        fftx[i].b *= denoise_mdl_scale;
                    }
                    for (i = denoise_clip_e; i < nbin; ++i) {
                        fftx[i].a = 0;
                        fftx[i].b = 0;
                    }
                    if (vboxebf6->cfg->use_denoise_alg) {
                        for (i = 0; i < nbin; ++i) {
                            fftx[i].a += o_fft[i].a * denoise_alg_scale;
                            fftx[i].b += o_fft[i].b * denoise_alg_scale;
                        }
                    }
                } else {
                    for (i = 0; i < nbin; ++i) {
                        fftx[i].a = o_fft[i].a * denoise_alg_scale;
                        fftx[i].b = o_fft[i].b * denoise_alg_scale;
                    }
                }
            }
        } else {
            if (vboxebf6->cfg->use_aec_mdl) {
                for (i = 0; i < aec_clip_s; ++i) {
                    fftx[i].a = 0;
                    fftx[i].b = 0;
                }
                for (i = aec_clip_s; i < aec_clip_e; ++i) {
                    fftx[i].a *= aec_mdl_scale;
                    fftx[i].b *= aec_mdl_scale;
                }
                for (i = aec_clip_e; i < nbin; ++i) {
                    fftx[i].a = 0;
                    fftx[i].b = 0;
                }
                if (vboxebf6->cfg->use_aec_alg) {
                    for (i = 0; i < nbin; ++i) {
                        fftx[i].a += o_fft[i].a * aec_alg_scale;
                        fftx[i].b += o_fft[i].b * aec_alg_scale;
                    }
                }
            } else {
                for (i = 0; i < nbin; ++i) {
                    fftx[i].a = o_fft[i].a * aec_alg_scale;
                    fftx[i].b = o_fft[i].b * aec_alg_scale;
                }
            }
        }
        if (vboxebf6->qmmse4 && vboxebf6->denoise_enable) {
            if (vboxebf6->cfg->use_qmmse_mask) {
                float *gf = vboxebf6->vdr->gf;
                for (i = 0; i < nbin; ++i) {
                    gf[i] *= qmmse_mask_scale;
                }
                wtk_qmmse_feed_mask(vboxebf6->qmmse4, fftx, gf);
            } else {
                float *gf = vboxebf6->vdr->gf;
                for (i = 0; i < nbin; ++i) {
                    gf[i] *= qmmse_mask_scale;
                }
                if (vboxebf6->entropy_in_cnt > vboxebf6->cfg->entropy_cnt / 2 ||
                    vboxebf6->cfg->entropy_scale == 1.0) {
                    wtk_qmmse_feed_echo_denoise4(vboxebf6->qmmse4, fftx,
                                                 vboxebf6->Yf, gf);
                } else if (vboxebf6->entropy_in_cnt > 0) {
                    wtk_qmmse_feed_echo_denoise4(vboxebf6->qmmse4, fftx,
                                                 vboxebf6->Yf, gf);
                    for (i = 0; i < nbin; ++i) {
                        fftx[i].a *= pow(((vboxebf6->cfg->entropy_cnt / 2 +
                                           vboxebf6->entropy_in_cnt) *
                                          1.0 / vboxebf6->cfg->entropy_cnt),
                                         5);
                        fftx[i].b *= pow(((vboxebf6->cfg->entropy_cnt / 2 +
                                           vboxebf6->entropy_in_cnt) *
                                          1.0 / vboxebf6->cfg->entropy_cnt),
                                         5);
                    }
                }
            }
        }
        for (i = 0; i <= clip_s; ++i) {
            fftx[i].a = fftx[i].b = 0;
        }
        for (i = clip_e; i < nbin; ++i) {
            fftx[i].a = fftx[i].b = 0;
        }
        // static int cnt=0;
        // cnt++;
        // printf("%f\n", wtk_float_abs_mean(vboxebf6->qmmse3->noise,
        // vboxebf6->qmmse2->cfg->step)); int u_sum=0;
        // for(i=0;i<vboxebf6->qmmse2->cfg->step;++i){
        // 	u_sum += vboxebf6->qmmse2->update_prob[i];
        // }
        // printf("%d\n", u_sum);

        if (vboxebf6->maskssl2 && vboxebf6->ssl_enable) {
            wtk_maskssl2_feed_fft2(vboxebf6->maskssl2, fft,
                                   vboxebf6->vdr->ssl_gf, vboxebf6->mic_sil);
        } else if (vboxebf6->maskssl && vboxebf6->ssl_enable) {
            wtk_maskssl_feed_fft2(vboxebf6->maskssl, fft, vboxebf6->vdr->ssl_gf,
                                  vboxebf6->mic_sil);
        }

        if (vboxebf6->cfg->use_cnon) {
            wtk_vboxebf6_feed_cnon(vboxebf6, fftx);
        }

        wtk_strbufs_pop(mic, nmicchannel, fsize * sizeof(short));
        wtk_strbufs_pop(mix, nmixchannel, fsize * sizeof(short));
        if (mic2) {
            wtk_strbufs_pop(mic2, nmicchannel, fsize * sizeof(short));
        }
        wtk_strbufs_pop(sp, nspchannel, fsize * sizeof(short));
        length = mic[0]->pos / sizeof(short);

        wtk_drft_frame_synthesis(rfft, rfft_in, synthesis_mem, fftx, out, wins,
                                 synthesis_window);
        if (vboxebf6->eq) {
            wtk_equalizer_feed_float(vboxebf6->eq, out, fsize);
        }
        if (vboxebf6->limiter) {
            wtk_limiter_feed(vboxebf6->limiter, out, fsize);
        } else {
            wtk_vboxebf6_control_bs(vboxebf6, out, fsize);
        }
        for (i = 0; i < fsize; ++i) {
            pv[i] = floorf(out[i] + 0.5);
        }
        if (vboxebf6->notify) {
            vboxebf6->notify(vboxebf6->ths, pv, fsize);
        }
    }
    if (is_end && length > 0) {
        if (vboxebf6->notify) {
            pv = (short *)mic[0]->data;
            vboxebf6->notify(vboxebf6->ths, pv, length);
        }
    }
}

void wtk_vboxebf6_set_micvolume(wtk_vboxebf6_t *vboxebf6, float fscale) {
    vboxebf6->inmic_scale = fscale;
}

void wtk_vboxebf6_set_agcenable(wtk_vboxebf6_t *vboxebf6, int enable) {
    vboxebf6->agc_enable = enable;
}

void wtk_vboxebf6_set_agclevel(wtk_vboxebf6_t *vboxebf6, int level) {
    int n_agc_level = vboxebf6->cfg->n_agc_level;
    float *agc_model_level = vboxebf6->cfg->agc_model_level;
    float *qmmse_agc_level = vboxebf6->cfg->qmmse_agc_level;
    float *qmmse_max_gain = vboxebf6->cfg->qmmse_max_gain;
    float *gc_gain_level = vboxebf6->cfg->gc_gain_level;
    int i;

    level = max(min(level, n_agc_level), 1);
    vboxebf6->agc_level = level;

    if (vboxebf6->agc_on) {
        for (i = 0; i < n_agc_level; ++i) {
            if (level == i + 1) {
                vboxebf6->cfg->agc_a2 = agc_model_level[i];
                break;
            }
        }
    }
    if (vboxebf6->qmmse4) {
        for (i = 0; i < n_agc_level; ++i) {
            if (level == i + 1) {
                vboxebf6->qmmse4->cfg->agc_level = qmmse_agc_level[i];
                vboxebf6->qmmse4->cfg->max_gain = qmmse_max_gain[i];
                break;
            }
        }
    }
    if (vboxebf6->gc) {
        for (i = 0; i < n_agc_level; ++i) {
            if (level == i + 1) {
                vboxebf6->gc->kalman.Z_k = gc_gain_level[i];
                break;
            }
        }
    }
}

void wtk_vboxebf6_set_echoenable(wtk_vboxebf6_t *vboxebf6, int enable) {
    vboxebf6->echo_enable = enable;
}

void wtk_vboxebf6_set_echolevel(wtk_vboxebf6_t *vboxebf6, int level) {
    int n_aec_level = vboxebf6->cfg->n_aec_level;
    float *aec_leak_scale = vboxebf6->cfg->aec_leak_scale;
    int i;

    level = max(min(level, n_aec_level), 1);
    vboxebf6->aec_level = level;
    for (i = 0; i < n_aec_level; ++i) {
        if (level == i + 1) {
            vboxebf6->cfg->leak_scale = aec_leak_scale[i];
            break;
        }
    }
}

void wtk_vboxebf6_set_denoiseenable(wtk_vboxebf6_t *vboxebf6, int enable) {
    vboxebf6->denoise_enable = enable;
    // if(enable==0)
    // {
    // 	wtk_vboxebf6_set_denoisesuppress(vboxebf6, 0);
    // }
}

void wtk_vboxebf6_set_denoisesuppress(wtk_vboxebf6_t *vboxebf6,
                                      float denoisesuppress) {
    if (vboxebf6->qmmse2) {
        vboxebf6->qmmse2->cfg->noise_suppress = denoisesuppress;
    }
    if (vboxebf6->qmmse3) {
        vboxebf6->qmmse3->cfg->noise_suppress = denoisesuppress;
    }
}

void wtk_vboxebf6_set_denoiselevel(wtk_vboxebf6_t *vboxebf6, int level) {
    int n_ans_level = vboxebf6->cfg->n_ans_level;
    float *qmmse_noise_suppress = vboxebf6->cfg->qmmse_noise_suppress;
    int i;

    level = max(min(level, n_ans_level), 1);
    vboxebf6->ans_level = level;

    for (i = 0; i < n_ans_level; ++i) {
        if (level == i + 1) {
            wtk_vboxebf6_set_denoisesuppress(vboxebf6, qmmse_noise_suppress[i]);
            break;
        }
    }
}

void wtk_vboxebf6_ssl_delay_new(wtk_vboxebf6_t *vboxebf6) {
    if (vboxebf6->cfg->use_maskssl) {
        vboxebf6->maskssl = wtk_maskssl_new(&(vboxebf6->cfg->maskssl));
        wtk_maskssl_set_notify(vboxebf6->maskssl, vboxebf6,
                               (wtk_maskssl_notify_f)wtk_vboxebf6_on_ssl2);

    } else if (vboxebf6->cfg->use_maskssl2) {
        vboxebf6->maskssl2 = wtk_maskssl2_new(&(vboxebf6->cfg->maskssl2));
        wtk_maskssl2_set_notify(vboxebf6->maskssl2, vboxebf6,
                                (wtk_maskssl2_notify_f)wtk_vboxebf6_on_ssl2);
    }
    if (vboxebf6->maskssl) {
        wtk_maskssl_reset(vboxebf6->maskssl);
    }
    if (vboxebf6->maskssl2) {
        wtk_maskssl2_reset(vboxebf6->maskssl2);
    }
    vboxebf6->ssl_enable = 1;
}

void wtk_vboxebf6_set_delay(wtk_vboxebf6_t *vboxebf6, float mic_delay,
                            float sp_delay) {
    vboxebf6->mic_delay = mic_delay;
    vboxebf6->sp_delay = sp_delay;
    vboxebf6->mic_delay_samples = floor(mic_delay * vboxebf6->cfg->rate);
    vboxebf6->sp_delay_samples = floor(sp_delay * vboxebf6->cfg->rate);
}
