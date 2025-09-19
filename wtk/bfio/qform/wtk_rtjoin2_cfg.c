#include "wtk_rtjoin2_cfg.h"

void wtk_rtjoin2_cfg_get_signal(wtk_rtjoin2_cfg_t *cfg) {
    float *play_signal_f;
    float *amp;
    float *hann;
    float *hann2;
    int T = cfg->rate / cfg->align_freq;
    int half_T = T / 2;
    int i;

    cfg->play_signal_len = (int)(cfg->align_duration * cfg->rate);
    play_signal_f = (float *)wtk_malloc(sizeof(float) * cfg->play_signal_len);
    cfg->play_signal =
        (short *)wtk_malloc(sizeof(short) * cfg->play_signal_len);
    // 对齐参考信号的长度为大于T最小的2的幂次方
    cfg->align_signal_len = pow(2, (int)(ceil(log2(T))));
    // cfg->align_signal_len = T;
    cfg->align_signal =
        (float *)wtk_malloc(sizeof(float) * cfg->align_signal_len);
    memset(cfg->align_signal, 0, sizeof(float) * cfg->align_signal_len);

    amp = (float *)wtk_malloc(sizeof(float) * cfg->play_signal_len);

    for (i = 0; i < cfg->play_signal_len / 2 - half_T; ++i) {
        amp[i] = cfg->align_amp;
    }
    for (i = cfg->play_signal_len / 2 - half_T;
         i < cfg->play_signal_len / 2 + half_T; ++i) {
        amp[i] = cfg->align_amp2;
    }
    for (i = cfg->play_signal_len / 2 + half_T; i < cfg->play_signal_len; ++i) {
        amp[i] = cfg->align_amp;
    }

    for (i = 0; i < cfg->play_signal_len; ++i) {
        play_signal_f[i] =
            amp[i] * sinf(2 * M_PI * i * cfg->align_freq * 1.0 / cfg->rate);
    }

    hann = wtk_math_create_hanning_window2(cfg->play_signal_len / 4);
    hann2 = wtk_math_create_hanning_window2(T);

    for (i = 0; i < cfg->play_signal_len / 8; ++i) {
        play_signal_f[i] *= hann[i];
    }
    for (i = cfg->play_signal_len / 8 * 7; i < cfg->play_signal_len; ++i) {
        play_signal_f[i] *= hann[i - (cfg->play_signal_len / 8 * 6)];
    }

    for (i = cfg->play_signal_len / 2 - T; i < cfg->play_signal_len / 2 - T / 2;
         ++i) {
        play_signal_f[i] *= hann2[i - (cfg->play_signal_len / 2 - T) + T / 2];
    }
    for (i = cfg->play_signal_len / 2 - T / 2;
         i < cfg->play_signal_len / 2 + T / 2; ++i) {
        play_signal_f[i] *= hann2[i - (cfg->play_signal_len / 2 - T / 2)];
    }
    for (i = cfg->play_signal_len / 2 + T / 2; i < cfg->play_signal_len / 2 + T;
         ++i) {
        play_signal_f[i] *= hann2[i - (cfg->play_signal_len / 2 + T / 2)];
    }

    memcpy(cfg->align_signal,
           play_signal_f + (cfg->play_signal_len / 2 - T / 2),
           sizeof(float) * T);
    for (i = 0; i < cfg->play_signal_len; ++i) {
        cfg->play_signal[i] = (short)(play_signal_f[i] * 32767.0);
    }
    // for (i = 0; i < cfg->play_signal_len; ++i) {
    //     printf("%d\n", cfg->play_signal[i]);
    // }
    // for (i = 0; i < T; ++i) {
    //     // printf("%f\n", cfg->align_signal[i] * 32767.0);
    // }
    for (i = 0; i < cfg->align_signal_len / 2; ++i) {
        float tmp = cfg->align_signal[i];
        cfg->align_signal[i] = cfg->align_signal[cfg->align_signal_len - i - 1];
        cfg->align_signal[cfg->align_signal_len - i - 1] = tmp;
    }

    // for (i = 0; i < cfg->align_signal_len ; ++i) {
    //     printf("%f\n", cfg->align_signal[i]);
    // }
    // exit(0);

// #include "wtk/core/wtk_wavfile.h"
//     wtk_wavfile_t *wav = wtk_wavfile_new(cfg->rate);
//     wtk_wavfile_open(wav, "20hz.wav");
//     wav->max_pend = 0;
//     wtk_wavfile_set_channel(wav, 1);
//     short *pv = (short *)wtk_malloc(sizeof(short) * cfg->align_signal_len);
//     for (i = 0; i < cfg->align_signal_len; ++i) {
//         pv[i] = (short)(cfg->align_signal[i] * 32767.0);
//     }
//     wtk_wavfile_write(wav, (char *)pv, cfg->align_signal_len * 2);
//     wtk_wavfile_close(wav);
//     wtk_free(pv);

    wtk_free(amp);
    wtk_free(hann);
    wtk_free(hann2);
    wtk_free(play_signal_f);
}

int wtk_rtjoin2_cfg_init(wtk_rtjoin2_cfg_t *cfg) {
    cfg->channel = 0;
    cfg->nmicchannel = 0;
    cfg->mic_channel = NULL;
    cfg->nspchannel = 0;
    cfg->sp_channel = NULL;
    cfg->wins = 320;

    cfg->main_cfg = NULL;
    cfg->mbin_cfg = NULL;

    cfg->rate = 16000;

    cfg->clip_s = 0;
    cfg->clip_e = 8000;

    cfg->spenr_thresh = 100;
    cfg->spenr_cnt = 10;
    cfg->micenr_thresh = 100;
    cfg->micenr_cnt = 10;

    cfg->kernel_len = 32;
    cfg->N1 = 4;
    cfg->N2 = 4;
    cfg->N = 0;
    cfg->conv_len = 0;
    cfg->sp_conv_thresh = 0.002;

    cfg->align_duration = 1.0;
    cfg->align_freq = 20;
    cfg->align_amp = 0.1;
    cfg->align_amp2 = 0.6;
    cfg->play_signal = NULL;
    cfg->play_signal_len = 0;
    cfg->align_signal = NULL;
    cfg->align_signal_len = 0;
    cfg->align_min_frame = 0;
    cfg->align_data_len = 0;
    cfg->align_conv_len = 0;
    cfg->align_thresh = 0.1;
    cfg->align_min_thresh = 0;
    cfg->align_mean_thresh = 1;
    cfg->min_thresh_cnt = 2;
    cfg->play_cnt = 2;
    cfg->blank_frame = 10;

    cfg->rob_winsize = 30;
    cfg->rob_conf_decay = 0.99;
    cfg->rob_mad_threshold = 3.0;
    cfg->rob_reliability_ratio = 2.0;
    cfg->rob_prob_mu = 0.05;
    cfg->rob_prob_sigma = 0.01;
    cfg->rob_long_mu = 0.0;
    cfg->rob_long_sigma = 1.0;
    cfg->rob_long_mad = 1.0;
    cfg->rob_long_alpha1 = 0.1;
    cfg->rob_long_alpha2 = 0.95;

    cfg->power_alpha = 0.9;
    cfg->weight_alpha = 0.9;
    cfg->weight_thresh = 0.3;

    qtk_ahs_gain_controller_cfg_init(&(cfg->gc));
    cfg->gc_gain = 25000.0;
    cfg->gc_min_thresh = 0;
    cfg->gc_cnt = 10;

    cfg->max_out = 32700.0;
    cfg->out_agc_level = -1;

    wtk_equalizer_cfg_init(&(cfg->eq));
    cfg->use_eq = 0;

    cfg->featm_lm = 1;
    cfg->featsp_lm = 1;
    cfg->aecmdl_fn = NULL;
    cfg->gainnet2 = NULL;
    wtk_bankfeat_cfg_init(&(cfg->bankfeat));

    cfg->use_control_bs = 0;
    cfg->use_conv_fft = 1;
    cfg->use_conv_power = 1;
    cfg->use_rob_filter = 1;
    cfg->delay_in_cnt = 20;
    cfg->delay_frame_cnt = 1000;

    cfg->use_mul_out = 0;
    cfg->use_avg_mix = 0;
    cfg->use_bs_win = 0;
    cfg->use_gc = 0;
    cfg->use_gainnet2 = 0;
    cfg->use_rbin_res = 0;
    cfg->use_align_signal = 0;

    return 0;
}

int wtk_rtjoin2_cfg_clean(wtk_rtjoin2_cfg_t *cfg) {
    if (cfg->mic_channel) {
        wtk_free(cfg->mic_channel);
    }
    if (cfg->sp_channel) {
        wtk_free(cfg->sp_channel);
    }
    if (cfg->play_signal) {
        wtk_free(cfg->play_signal);
    }
    if (cfg->align_signal) {
        wtk_free(cfg->align_signal);
    }
    if (cfg->gainnet2) {
        if (cfg->use_rbin_res) {
            wtk_gainnet2_cfg_delete_bin3(cfg->gainnet2);
        } else {
            wtk_gainnet2_cfg_delete_bin2(cfg->gainnet2);
        }
    }
    wtk_bankfeat_cfg_clean(&(cfg->bankfeat));
    qtk_ahs_gain_controller_cfg_clean(&(cfg->gc));
    wtk_equalizer_cfg_clean(&(cfg->eq));

    return 0;
}

int wtk_rtjoin2_cfg_update_local(wtk_rtjoin2_cfg_t *cfg, wtk_local_cfg_t *m) {
    wtk_string_t *v;
    wtk_local_cfg_t *lc;
    int ret;
    wtk_array_t *a;
    int i;

    lc = m;
    wtk_local_cfg_update_cfg_str(lc, cfg, aecmdl_fn, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, kernel_len, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, N1, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, N2, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, wins, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, channel, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, rate, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, sp_conv_thresh, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, align_duration, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, align_freq, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, align_amp, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, align_amp2, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, align_thresh, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, align_min_thresh, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, align_mean_thresh, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, min_thresh_cnt, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, play_cnt, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, blank_frame, v);

    wtk_local_cfg_update_cfg_i(lc, cfg, clip_s, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, clip_e, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, spenr_thresh, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, spenr_cnt, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, micenr_thresh, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, micenr_cnt, v);

    wtk_local_cfg_update_cfg_i(lc, cfg, rob_winsize, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, rob_conf_decay, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, rob_mad_threshold, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, rob_reliability_ratio, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, rob_prob_mu, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, rob_prob_sigma, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, rob_long_mu, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, rob_long_sigma, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, rob_long_mad, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, rob_long_alpha1, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, rob_long_alpha2, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, gc_gain, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, gc_min_thresh, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, gc_cnt, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, max_out, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, out_agc_level, v);

    wtk_local_cfg_update_cfg_i(lc, cfg, featm_lm, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, featsp_lm, v);

    wtk_local_cfg_update_cfg_b(lc, cfg, use_eq, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_control_bs, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_conv_fft, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_conv_power, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_rob_filter, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, delay_in_cnt, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, delay_frame_cnt, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_mul_out, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_avg_mix, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_bs_win, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_gc, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_gainnet2, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_align_signal, v);

    a = wtk_local_cfg_find_array_s(lc, "mic_channel");
    if (a) {
        cfg->mic_channel = (int *)wtk_malloc(sizeof(int) * a->nslot);
        cfg->nmicchannel = a->nslot;
        for (i = 0; i < a->nslot; ++i) {
            v = ((wtk_string_t **)a->slot)[i];
            cfg->mic_channel[i] = wtk_str_atoi(v->data, v->len);
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
    lc = wtk_local_cfg_find_lc_s(m, "bankfeat");
    if (lc) {
        ret = wtk_bankfeat_cfg_update_local(&(cfg->bankfeat), lc);
        if (ret != 0) {
            goto end;
        }
    }
    ret = 0;
end:
    return ret;
}

int wtk_rtjoin2_cfg_update(wtk_rtjoin2_cfg_t *cfg) {
    int ret;

    if (cfg->use_rbin_res == 0 && cfg->aecmdl_fn) {
        cfg->gainnet2 = wtk_gainnet2_cfg_new_bin2(cfg->aecmdl_fn);
        if (!cfg->gainnet2) {
            ret = -1;
            goto end;
        }
    }
    cfg->N = cfg->N1 + cfg->N2 + 1;
    cfg->slid_len = cfg->N * cfg->wins / 2;
    cfg->conv_len = cfg->slid_len + cfg->kernel_len - 1;
    if (cfg->use_conv_power) {
        // Expand to powers of 2
        cfg->conv_len = 2 << (int)(log2(cfg->conv_len));
    } else {
        // Expand to Multiple of 2
        cfg->conv_len = ((cfg->conv_len + 1) / 2) * 2;
    }

    if (cfg->channel < cfg->nmicchannel + cfg->nspchannel) {
        cfg->channel = cfg->nmicchannel + cfg->nspchannel;
    }
    cfg->clip_s = (cfg->clip_s * 1.0 * cfg->wins) / cfg->rate;
    cfg->clip_s = max(1, cfg->clip_s);
    cfg->clip_e = (cfg->clip_e * 1.0 * cfg->wins) / cfg->rate;
    cfg->clip_e = min(cfg->wins / 2, cfg->clip_e);

    ret = qtk_ahs_gain_controller_cfg_update(&(cfg->gc));
    if (ret != 0) {
        goto end;
    }
    ret = wtk_equalizer_cfg_update(&(cfg->eq));
    if (ret != 0) {
        goto end;
    }
    ret = wtk_bankfeat_cfg_update(&(cfg->bankfeat));
    if (ret != 0) {
        goto end;
    }

    if (cfg->use_align_signal) {
        wtk_rtjoin2_cfg_get_signal(cfg);
        cfg->align_min_frame =
            (int)(ceil(cfg->align_signal_len / (cfg->wins / 2))) + 1;
        cfg->align_data_len = cfg->align_min_frame * cfg->wins / 2;
        // cfg->align_conv_len = cfg->wins / 2 + cfg->align_signal_len - 1;
        cfg->align_conv_len = cfg->align_data_len + cfg->align_signal_len - 1;
        cfg->align_conv_len = 2 << (int)(log2(cfg->align_conv_len));
        // printf("%d %d %d %d\n", cfg->align_signal_len, cfg->align_min_frame,
        //        cfg->align_data_len, cfg->align_conv_len);
        // exit(0);
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

int wtk_rtjoin2_cfg_update2(wtk_rtjoin2_cfg_t *cfg, wtk_source_loader_t *sl) {
    wtk_rbin2_item_t *item;
    wtk_rbin2_t *rbin = (wtk_rbin2_t *)(sl->hook);
    int ret;

    cfg->use_rbin_res = 1;
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
    ret = 0;
end:
    if (ret != 0) {
        wtk_debug("update gainnet2 failed\n");
        return ret;
    }
    return wtk_rtjoin2_cfg_update(cfg);
}

void wtk_rtjoin2_cfg_set_channel(wtk_rtjoin2_cfg_t *cfg, int channel,
                                 char *micchannel, char *spkchannel) {
    wtk_free(cfg->mic_channel);
    wtk_free(cfg->sp_channel);
    int i;

    if (micchannel && spkchannel) {
        cfg->nmicchannel = channel / 2;
        cfg->nspchannel = channel / 2;
    } else if (micchannel) {
        cfg->nmicchannel = channel;
        cfg->nspchannel = 0;
    }

    if (micchannel) {
        cfg->mic_channel = (int *)wtk_malloc(sizeof(int) * cfg->nmicchannel);
        for (i = 0; i < cfg->nmicchannel; ++i) {
            cfg->mic_channel[i] = micchannel[i];
        }
    }
    if (spkchannel) {
        cfg->sp_channel = (int *)wtk_malloc(sizeof(int) * cfg->nspchannel);
        for (i = 0; i < cfg->nspchannel; ++i) {
            cfg->sp_channel[i] = spkchannel[i];
        }
    }
    cfg->channel = channel;
}

wtk_rtjoin2_cfg_t *wtk_rtjoin2_cfg_new(char *fn) {
    wtk_main_cfg_t *main_cfg;
    wtk_rtjoin2_cfg_t *cfg;
	wtk_debug("--------------------------------------------\n");
    main_cfg = wtk_main_cfg_new_type(wtk_rtjoin2_cfg, fn);
    if (!main_cfg) {
        return NULL;
    }
    cfg = (wtk_rtjoin2_cfg_t *)main_cfg->cfg;
    cfg->main_cfg = main_cfg;
    return cfg;
}

void wtk_rtjoin2_cfg_delete(wtk_rtjoin2_cfg_t *cfg) {
    wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_rtjoin2_cfg_t *wtk_rtjoin2_cfg_new_bin(char *fn) {
    wtk_mbin_cfg_t *mbin_cfg;
    wtk_rtjoin2_cfg_t *cfg;

    mbin_cfg = wtk_mbin_cfg_new_type(wtk_rtjoin2_cfg, fn, "./cfg");
    if (!mbin_cfg) {
        return NULL;
    }
    cfg = (wtk_rtjoin2_cfg_t *)mbin_cfg->cfg;
    cfg->mbin_cfg = mbin_cfg;
    return cfg;
}

void wtk_rtjoin2_cfg_delete_bin(wtk_rtjoin2_cfg_t *cfg) {
    wtk_mbin_cfg_delete(cfg->mbin_cfg);
}