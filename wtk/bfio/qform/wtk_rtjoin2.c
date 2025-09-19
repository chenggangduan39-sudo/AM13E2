#include "wtk_rtjoin2.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/core/wtk_strbuf.h"

void wtk_rtjoin2_aec_on_gainnet2(wtk_rtjoin2_edra_t *vdr, float *gain, int len,
                                 int is_end);

void wtk_rtjoin2_edra_init(wtk_rtjoin2_edra_t *vdr, wtk_rtjoin2_cfg_t *cfg) {
    vdr->cfg = cfg;
    vdr->nbin = cfg->wins / 2 + 1;

    vdr->bank_mic = wtk_bankfeat_new(&(cfg->bankfeat));
    vdr->bank_sp = wtk_bankfeat_new(&(cfg->bankfeat));

    vdr->g = wtk_malloc(sizeof(float) * cfg->bankfeat.nb_bands);
    vdr->gf = wtk_malloc(sizeof(float) * vdr->nbin);

    vdr->feature_sp = NULL;
    if (cfg->featm_lm + cfg->featsp_lm > 2) {
        vdr->feature_sp = wtk_malloc(sizeof(float) * cfg->bankfeat.nb_features *
                                     (cfg->featm_lm + cfg->featsp_lm - 1));
    }

    vdr->gainnet2 = wtk_gainnet2_new(cfg->gainnet2);
    wtk_gainnet2_set_notify(vdr->gainnet2, vdr,
                            (wtk_gainnet2_notify_f)wtk_rtjoin2_aec_on_gainnet2);
}

void wtk_rtjoin2_edra_clean(wtk_rtjoin2_edra_t *vdr) {
    wtk_bankfeat_delete(vdr->bank_mic);
    wtk_bankfeat_delete(vdr->bank_sp);

    if (vdr->feature_sp) {
        wtk_free(vdr->feature_sp);
    }

    wtk_free(vdr->g);
    wtk_free(vdr->gf);

    wtk_gainnet2_delete(vdr->gainnet2);
}

void wtk_rtjoin2_edra_reset(wtk_rtjoin2_edra_t *vdr) {
    wtk_bankfeat_reset(vdr->bank_mic);
    wtk_bankfeat_reset(vdr->bank_sp);

    if (vdr->feature_sp) {
        memset(vdr->feature_sp, 0,
               sizeof(float) * vdr->bank_sp->cfg->nb_features *
                   (vdr->cfg->featm_lm + vdr->cfg->featsp_lm - 1));
    }

    memset(vdr->g, 0, sizeof(float) * vdr->cfg->bankfeat.nb_bands);
    memset(vdr->gf, 0, sizeof(float) * vdr->nbin);

    wtk_gainnet2_reset(vdr->gainnet2);
}

wtk_rtjoin2_t *wtk_rtjoin2_new(wtk_rtjoin2_cfg_t *cfg) {
    wtk_rtjoin2_t *rtjoin2;
    int i;

    rtjoin2 = (wtk_rtjoin2_t *)wtk_malloc(sizeof(wtk_rtjoin2_t));
    rtjoin2->cfg = cfg;
    rtjoin2->ths = NULL;
    rtjoin2->notify = NULL;

    rtjoin2->mic = wtk_strbufs_new(rtjoin2->cfg->nmicchannel);
    rtjoin2->sp = wtk_strbufs_new(rtjoin2->cfg->nspchannel);

    rtjoin2->nbin = cfg->wins / 2 + 1;
    rtjoin2->analysis_window = wtk_malloc(sizeof(float) * cfg->wins);  /// 2);
    rtjoin2->synthesis_window = wtk_malloc(sizeof(float) * cfg->wins); /// 2);
    rtjoin2->analysis_mem =
        wtk_float_new_p2(cfg->nmicchannel, rtjoin2->nbin - 1);
    rtjoin2->analysis_mem_sp =
        wtk_float_new_p2(cfg->nspchannel, rtjoin2->nbin - 1);
    rtjoin2->synthesis_mem = wtk_malloc(sizeof(float) * (rtjoin2->nbin - 1));
    rtjoin2->mul_synthesis_mem = NULL;
    if (cfg->use_mul_out) {
        rtjoin2->mul_synthesis_mem =
            wtk_float_new_p2(cfg->nmicchannel, rtjoin2->nbin - 1);
    }
    rtjoin2->rfft = wtk_drft_new(cfg->wins);
    rtjoin2->rfft_in = (float *)wtk_malloc(sizeof(float) * (cfg->wins));

    rtjoin2->fft = wtk_complex_new_p2(cfg->nmicchannel, rtjoin2->nbin);
    rtjoin2->fft_sp = wtk_complex_new_p2(cfg->nspchannel, rtjoin2->nbin);

    rtjoin2->fftx =
        (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * rtjoin2->nbin);

    rtjoin2->ffty = NULL;
    if (cfg->use_gainnet2) {
        rtjoin2->ffty =
            (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * rtjoin2->nbin);
    }

    rtjoin2->kernel = (float *)wtk_malloc(sizeof(float) * cfg->kernel_len);
    rtjoin2->res_tmp = (float *)wtk_malloc(sizeof(float) * cfg->conv_len);
    rtjoin2->drft2 = NULL;
    rtjoin2->kernel_fft = NULL;
    rtjoin2->data_fft = NULL;
    rtjoin2->res_fft = NULL;
    rtjoin2->kernel_tmp = NULL;
    rtjoin2->data_tmp = NULL;
    if (cfg->use_conv_fft) {
        rtjoin2->drft2 = wtk_drft_new2(cfg->conv_len);
        rtjoin2->kernel_fft =
            (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * cfg->conv_len);
        rtjoin2->data_fft =
            (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * cfg->conv_len);
        rtjoin2->res_fft =
            (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * cfg->conv_len);
        rtjoin2->kernel_tmp =
            (float *)wtk_malloc(sizeof(float) * cfg->conv_len);
        rtjoin2->data_tmp = (float *)wtk_malloc(sizeof(float) * cfg->conv_len);
    }

    rtjoin2->drft3 = NULL;
    rtjoin2->a_kernel_fft = NULL;
    rtjoin2->a_data_fft = NULL;
    rtjoin2->a_res_fft = NULL;
    rtjoin2->a_kernel_tmp = NULL;
    rtjoin2->a_data_tmp = NULL;
    rtjoin2->a_res_tmp = NULL;
    rtjoin2->a_res = NULL;
    rtjoin2->align_res = NULL;
    rtjoin2->raw_sig = NULL;
    rtjoin2->align_cnt = NULL;
    rtjoin2->align_max_idx = NULL;
    rtjoin2->align_max_value = NULL;
    rtjoin2->min_data = NULL;
    rtjoin2->align_delay = NULL;
    rtjoin2->align_state = NULL;
    if (cfg->use_align_signal) {
        rtjoin2->drft3 = wtk_drft_new2(cfg->align_conv_len);
        rtjoin2->a_kernel_fft = (wtk_complex_t *)wtk_malloc(
            sizeof(wtk_complex_t) * cfg->align_conv_len);
        rtjoin2->a_data_fft = (wtk_complex_t *)wtk_malloc(
            sizeof(wtk_complex_t) * cfg->align_conv_len);
        rtjoin2->a_res_fft = (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) *
                                                         cfg->align_conv_len);
        rtjoin2->a_kernel_tmp =
            (float *)wtk_malloc(sizeof(float) * cfg->align_conv_len);
        rtjoin2->a_data_tmp =
            (float *)wtk_malloc(sizeof(float) * cfg->align_conv_len);
        rtjoin2->a_res_tmp =
            (float *)wtk_malloc(sizeof(float) * cfg->align_conv_len);
        rtjoin2->a_res = wtk_float_new_p2(cfg->nspchannel, cfg->align_conv_len);
        rtjoin2->align_res =
            wtk_float_new_p2(cfg->nspchannel, cfg->align_data_len);
        rtjoin2->raw_sig =
            wtk_float_new_p2(cfg->nspchannel, cfg->align_data_len);
        rtjoin2->align_cnt = (int *)wtk_malloc(sizeof(int) * cfg->nspchannel);
        rtjoin2->align_max_idx =
            (int *)wtk_malloc(sizeof(int) * cfg->nspchannel);
        rtjoin2->align_max_value =
            (float *)wtk_malloc(sizeof(float) * cfg->nspchannel);
        rtjoin2->min_data =
            wtk_float_new_p2(cfg->nspchannel, cfg->min_thresh_cnt);
        rtjoin2->align_delay =
            (int **)wtk_malloc(sizeof(int *) * cfg->nspchannel);
        for (i = 0; i < cfg->nspchannel; ++i) {
            rtjoin2->align_delay[i] =
                (int *)wtk_malloc(sizeof(int) * cfg->play_cnt);
        }
        rtjoin2->align_state = (int *)wtk_malloc(sizeof(int) * cfg->nspchannel);
    }

    rtjoin2->rob_prob_mu = NULL;
    rtjoin2->rob_prob_sigma = NULL;
    rtjoin2->rob_long_mu = NULL;
    rtjoin2->rob_long_sigma = NULL;
    rtjoin2->rob_long_mad = NULL;
    rtjoin2->rob_values = NULL;
    rtjoin2->rob_weights = NULL;
    rtjoin2->rob_values_tmp = NULL;
    rtjoin2->rob_weights_tmp = NULL;
    rtjoin2->residuals = NULL;
    rtjoin2->values_len = NULL;
    rtjoin2->delay_idx = NULL;
    rtjoin2->delay_idx_len = NULL;
    rtjoin2->real_delay = NULL;
    rtjoin2->real_delay_start = NULL;
    if (cfg->use_rob_filter) {
        rtjoin2->rob_prob_mu =
            (float *)wtk_malloc(sizeof(float) * cfg->nspchannel);
        rtjoin2->rob_prob_sigma =
            (float *)wtk_malloc(sizeof(float) * cfg->nspchannel);
        rtjoin2->rob_long_mu =
            (float *)wtk_malloc(sizeof(float) * cfg->nspchannel);
        rtjoin2->rob_long_sigma =
            (float *)wtk_malloc(sizeof(float) * cfg->nspchannel);
        rtjoin2->rob_long_mad =
            (float *)wtk_malloc(sizeof(float) * cfg->nspchannel);
        rtjoin2->rob_values =
            wtk_float_new_p2(cfg->nspchannel, cfg->rob_winsize);
        rtjoin2->rob_weights =
            wtk_float_new_p2(cfg->nspchannel, cfg->rob_winsize);
        rtjoin2->rob_values_tmp =
            wtk_float_new_p2(cfg->nspchannel, cfg->rob_winsize);
        rtjoin2->rob_weights_tmp =
            wtk_float_new_p2(cfg->nspchannel, cfg->rob_winsize);
        rtjoin2->residuals =
            (float *)wtk_malloc(sizeof(float) * cfg->rob_winsize);
        rtjoin2->values_len = (int *)wtk_malloc(sizeof(int) * cfg->nspchannel);
        rtjoin2->delay_idx =
            (int **)wtk_malloc(sizeof(int *) * cfg->nspchannel);
        for (int i = 0; i < cfg->nspchannel; ++i) {
            rtjoin2->delay_idx[i] =
                (int *)wtk_malloc(sizeof(int) * cfg->delay_frame_cnt);
        }
        rtjoin2->delay_idx_len =
            (int *)wtk_malloc(sizeof(int) * cfg->nspchannel);
        rtjoin2->real_delay = (int *)wtk_malloc(sizeof(int) * cfg->nspchannel);
        rtjoin2->real_delay_start =
            (int *)wtk_malloc(sizeof(int) * cfg->nspchannel);
    }

    rtjoin2->local_power = NULL;
    rtjoin2->local_power_sum = NULL;
    rtjoin2->power = NULL;
    rtjoin2->local_weight = NULL;
    rtjoin2->weight = NULL;
    if (cfg->use_avg_mix) {
        rtjoin2->local_power =
            wtk_float_new_p2(cfg->nmicchannel, rtjoin2->nbin);
        rtjoin2->local_power_sum =
            (float *)wtk_malloc(sizeof(float) * cfg->nmicchannel);
        rtjoin2->power = (float *)wtk_malloc(sizeof(float) * cfg->nmicchannel);
        rtjoin2->local_weight =
            (float *)wtk_malloc(sizeof(float) * cfg->nmicchannel);
        rtjoin2->weight = (float *)wtk_malloc(sizeof(float) * cfg->nmicchannel);
    }

    rtjoin2->eq = NULL;
    if (cfg->use_eq) {
        rtjoin2->eq = wtk_equalizer_new(&(cfg->eq));
    }

    rtjoin2->mul_out = NULL;
    if (cfg->use_mul_out) {
        rtjoin2->mul_out = wtk_malloc(sizeof(float) * (rtjoin2->nbin - 1));
        rtjoin2->out =
            wtk_malloc(sizeof(float) * (rtjoin2->nbin - 1) * cfg->nmicchannel);
    } else {
        rtjoin2->out = wtk_malloc(sizeof(float) * (rtjoin2->nbin - 1));
    }

    rtjoin2->gc = NULL;
    if (cfg->use_gc) {
        rtjoin2->gc = qtk_gain_controller_new(&(cfg->gc));
        qtk_gain_controller_set_mode(rtjoin2->gc, 0);
        rtjoin2->gc->kalman.Z_k = cfg->gc_gain;
    }
    rtjoin2->bs_win = NULL;
    if (cfg->use_bs_win) {
        rtjoin2->bs_win = wtk_math_create_hanning_window2(cfg->wins / 2);
    }
    rtjoin2->vdr = NULL;
    if (cfg->use_gainnet2) {
        rtjoin2->vdr =
            (wtk_rtjoin2_edra_t *)wtk_malloc(sizeof(wtk_rtjoin2_edra_t));
        wtk_rtjoin2_edra_init(rtjoin2->vdr, cfg);
    }

    wtk_rtjoin2_reset(rtjoin2);

    return rtjoin2;
}

void wtk_rtjoin2_delete(wtk_rtjoin2_t *rtjoin2) {
    int i;
    wtk_strbufs_delete(rtjoin2->mic, rtjoin2->cfg->nmicchannel);
    wtk_strbufs_delete(rtjoin2->sp, rtjoin2->cfg->nspchannel);

    wtk_free(rtjoin2->analysis_window);
    wtk_free(rtjoin2->synthesis_window);
    wtk_float_delete_p2(rtjoin2->analysis_mem, rtjoin2->cfg->nmicchannel);
    wtk_float_delete_p2(rtjoin2->analysis_mem_sp, rtjoin2->cfg->nspchannel);
    wtk_free(rtjoin2->synthesis_mem);
    if (rtjoin2->mul_synthesis_mem) {
        wtk_float_delete_p2(rtjoin2->mul_synthesis_mem,
                            rtjoin2->cfg->nmicchannel);
    }
    wtk_free(rtjoin2->rfft_in);
    wtk_drft_delete(rtjoin2->rfft);
    wtk_complex_delete_p2(rtjoin2->fft, rtjoin2->cfg->nmicchannel);
    wtk_complex_delete_p2(rtjoin2->fft_sp, rtjoin2->cfg->nspchannel);
    wtk_free(rtjoin2->kernel);
    if (rtjoin2->drft2) {
        wtk_drft_delete2(rtjoin2->drft2);
        wtk_free(rtjoin2->kernel_fft);
        wtk_free(rtjoin2->data_fft);
        wtk_free(rtjoin2->res_fft);
        wtk_free(rtjoin2->kernel_tmp);
        wtk_free(rtjoin2->data_tmp);
    }
    wtk_free(rtjoin2->res_tmp);
    if (rtjoin2->drft3) {
        wtk_drft_delete2(rtjoin2->drft3);
        wtk_free(rtjoin2->a_kernel_fft);
        wtk_free(rtjoin2->a_data_fft);
        wtk_free(rtjoin2->a_res_fft);
        wtk_free(rtjoin2->a_kernel_tmp);
        wtk_free(rtjoin2->a_data_tmp);
        wtk_free(rtjoin2->a_res_tmp);
        wtk_float_delete_p2(rtjoin2->a_res, rtjoin2->cfg->nspchannel);
        wtk_float_delete_p2(rtjoin2->align_res, rtjoin2->cfg->nspchannel);
        wtk_float_delete_p2(rtjoin2->raw_sig, rtjoin2->cfg->nspchannel);
        wtk_free(rtjoin2->align_cnt);
        wtk_free(rtjoin2->align_max_idx);
        wtk_free(rtjoin2->align_max_value);
        wtk_float_delete_p2(rtjoin2->min_data, rtjoin2->cfg->nspchannel);
        for (i = 0; i < rtjoin2->cfg->nspchannel; ++i) {
            wtk_free(rtjoin2->align_delay[i]);
        }
        wtk_free(rtjoin2->align_delay);
        wtk_free(rtjoin2->align_state);
    }
    if (rtjoin2->rob_prob_mu) {
        wtk_free(rtjoin2->rob_prob_mu);
        wtk_free(rtjoin2->rob_prob_sigma);
        wtk_free(rtjoin2->rob_long_mu);
        wtk_free(rtjoin2->rob_long_sigma);
        wtk_free(rtjoin2->rob_long_mad);
        wtk_float_delete_p2(rtjoin2->rob_values, rtjoin2->cfg->nspchannel);
        wtk_float_delete_p2(rtjoin2->rob_weights, rtjoin2->cfg->nspchannel);
        wtk_float_delete_p2(rtjoin2->rob_values_tmp, rtjoin2->cfg->nspchannel);
        wtk_float_delete_p2(rtjoin2->rob_weights_tmp, rtjoin2->cfg->nspchannel);
        wtk_free(rtjoin2->residuals);
        wtk_free(rtjoin2->values_len);
        for (int i = 0; i < rtjoin2->cfg->nspchannel; ++i) {
            wtk_free(rtjoin2->delay_idx[i]);
        }
        wtk_free(rtjoin2->delay_idx);
        wtk_free(rtjoin2->delay_idx_len);
        wtk_free(rtjoin2->real_delay);
        wtk_free(rtjoin2->real_delay_start);
    }
    if (rtjoin2->local_power) {
        wtk_float_delete_p2(rtjoin2->local_power, rtjoin2->cfg->nmicchannel);
        wtk_free(rtjoin2->local_power_sum);
        wtk_free(rtjoin2->power);
        wtk_free(rtjoin2->local_weight);
        wtk_free(rtjoin2->weight);
    }
    if (rtjoin2->eq) {
        wtk_equalizer_delete(rtjoin2->eq);
    }

    wtk_free(rtjoin2->fftx);
    if (rtjoin2->ffty) {
        wtk_free(rtjoin2->ffty);
    }
    if (rtjoin2->mul_out) {
        wtk_free(rtjoin2->mul_out);
    }
    wtk_free(rtjoin2->out);

    if (rtjoin2->gc) {
        qtk_gain_controller_delete(rtjoin2->gc);
    }
    if (rtjoin2->bs_win) {
        wtk_free(rtjoin2->bs_win);
    }
    if (rtjoin2->vdr) {
        wtk_rtjoin2_edra_clean(rtjoin2->vdr);
        wtk_free(rtjoin2->vdr);
    }

    wtk_free(rtjoin2);
}

void wtk_rtjoin2_start(wtk_rtjoin2_t *rtjoin2) {
    if (rtjoin2->cfg->use_align_signal) {
        wtk_drft_t *drft3 = rtjoin2->drft3;
        wtk_complex_t *a_kernel_fft = rtjoin2->a_kernel_fft;
        float *a_kernel_tmp = rtjoin2->a_kernel_tmp;
        float *align_signal = rtjoin2->cfg->align_signal;

        memcpy(a_kernel_tmp, align_signal,
               sizeof(float) * rtjoin2->cfg->align_signal_len);
        for (int i = 0; i < rtjoin2->cfg->align_signal_len; ++i) {
            a_kernel_tmp[i] *= rtjoin2->cfg->align_conv_len;
            // printf("%f\n", a_kernel_tmp[i]);
        }
        wtk_drft_fft22(drft3, a_kernel_tmp, a_kernel_fft);
        for (int i = 0; i < rtjoin2->cfg->align_signal_len; ++i) {
            // printf("%.12f %.12f\n", a_kernel_fft[i].a, a_kernel_fft[i].b);
            // printf("%.12f\n", a_kernel_fft[i].a);
        }
        // exit(0);
    }
}

void wtk_rtjoin2_reset(wtk_rtjoin2_t *rtjoin2) {
    int wins = rtjoin2->cfg->wins;
    int i;

    wtk_strbufs_reset(rtjoin2->mic, rtjoin2->cfg->nmicchannel);
    wtk_strbufs_reset(rtjoin2->sp, rtjoin2->cfg->nspchannel);
    for (i = 0; i < wins; ++i) {
        rtjoin2->analysis_window[i] = sin((0.5 + i) * PI / (wins));
    }
    wtk_drft_init_synthesis_window(rtjoin2->synthesis_window,
                                   rtjoin2->analysis_window, wins);

    wtk_float_zero_p2(rtjoin2->analysis_mem, rtjoin2->cfg->nmicchannel,
                      (rtjoin2->nbin - 1));
    wtk_float_zero_p2(rtjoin2->analysis_mem_sp, rtjoin2->cfg->nspchannel,
                      (rtjoin2->nbin - 1));
    memset(rtjoin2->synthesis_mem, 0, sizeof(float) * (rtjoin2->nbin - 1));
    if (rtjoin2->mul_synthesis_mem) {
        wtk_float_zero_p2(rtjoin2->mul_synthesis_mem, rtjoin2->cfg->nmicchannel,
                          (rtjoin2->nbin - 1));
    }

    wtk_complex_zero_p2(rtjoin2->fft, rtjoin2->cfg->nmicchannel, rtjoin2->nbin);
    wtk_complex_zero_p2(rtjoin2->fft_sp, rtjoin2->cfg->nspchannel,
                        rtjoin2->nbin);
    memset(rtjoin2->fftx, 0, sizeof(wtk_complex_t) * (rtjoin2->nbin));
    if (rtjoin2->ffty) {
        memset(rtjoin2->ffty, 0, sizeof(wtk_complex_t) * (rtjoin2->nbin));
    }

    memset(rtjoin2->kernel, 0, sizeof(float) * rtjoin2->cfg->kernel_len);
    if (rtjoin2->drft2) {
        memset(rtjoin2->kernel_fft, 0,
               sizeof(wtk_complex_t) * rtjoin2->cfg->conv_len);
        memset(rtjoin2->data_fft, 0,
               sizeof(wtk_complex_t) * rtjoin2->cfg->conv_len);
        memset(rtjoin2->res_fft, 0,
               sizeof(wtk_complex_t) * rtjoin2->cfg->conv_len);
        memset(rtjoin2->kernel_tmp, 0, sizeof(float) * rtjoin2->cfg->conv_len);
        memset(rtjoin2->data_tmp, 0, sizeof(float) * rtjoin2->cfg->conv_len);
    }
    memset(rtjoin2->res_tmp, 0, sizeof(float) * rtjoin2->cfg->conv_len);

    if (rtjoin2->drft3) {
        memset(rtjoin2->a_kernel_fft, 0,
               sizeof(wtk_complex_t) * rtjoin2->cfg->align_conv_len);
        memset(rtjoin2->a_data_fft, 0,
               sizeof(wtk_complex_t) * rtjoin2->cfg->align_conv_len);
        memset(rtjoin2->a_res_fft, 0,
               sizeof(wtk_complex_t) * rtjoin2->cfg->align_conv_len);
        memset(rtjoin2->a_kernel_tmp, 0,
               sizeof(float) * rtjoin2->cfg->align_conv_len);
        memset(rtjoin2->a_data_tmp, 0,
               sizeof(float) * rtjoin2->cfg->align_conv_len);
        memset(rtjoin2->a_res_tmp, 0,
               sizeof(float) * rtjoin2->cfg->align_conv_len);
        wtk_float_zero_p2(rtjoin2->a_res, rtjoin2->cfg->nspchannel,
                          rtjoin2->cfg->align_conv_len);
        wtk_float_zero_p2(rtjoin2->align_res, rtjoin2->cfg->nspchannel,
                          rtjoin2->cfg->align_data_len);
        wtk_float_zero_p2(rtjoin2->raw_sig, rtjoin2->cfg->nspchannel,
                          rtjoin2->cfg->align_data_len);
        memset(rtjoin2->align_cnt, 0, sizeof(int) * rtjoin2->cfg->nspchannel);
        memset(rtjoin2->align_max_idx, 0,
               sizeof(int) * rtjoin2->cfg->nspchannel);
        memset(rtjoin2->align_max_value, 0,
               sizeof(float) * rtjoin2->cfg->nspchannel);
        wtk_float_zero_p2(rtjoin2->min_data, rtjoin2->cfg->nspchannel,
                          rtjoin2->cfg->min_thresh_cnt);
        for (i = 0; i < rtjoin2->cfg->nspchannel; ++i) {
            memset(rtjoin2->align_delay[i], 0,
                   sizeof(int) * rtjoin2->cfg->play_cnt);
        }
        memset(rtjoin2->align_state, 0, sizeof(int) * rtjoin2->cfg->nspchannel);
    }
    if (rtjoin2->rob_values) {
        for (i = 0; i < rtjoin2->cfg->nspchannel; ++i) {
            rtjoin2->rob_prob_mu[i] = rtjoin2->cfg->rob_prob_mu;
            rtjoin2->rob_prob_sigma[i] = rtjoin2->cfg->rob_prob_sigma;
            rtjoin2->rob_long_mu[i] = rtjoin2->cfg->rob_long_mu;
            rtjoin2->rob_long_sigma[i] = rtjoin2->cfg->rob_long_sigma;
            rtjoin2->rob_long_mad[i] = rtjoin2->cfg->rob_long_mad;
        }
        wtk_float_zero_p2(rtjoin2->rob_values, rtjoin2->cfg->nspchannel,
                          rtjoin2->cfg->rob_winsize);
        wtk_float_zero_p2(rtjoin2->rob_weights, rtjoin2->cfg->nspchannel,
                          rtjoin2->cfg->rob_winsize);
        wtk_float_zero_p2(rtjoin2->rob_values_tmp, rtjoin2->cfg->nspchannel,
                          rtjoin2->cfg->rob_winsize);
        wtk_float_zero_p2(rtjoin2->rob_weights_tmp, rtjoin2->cfg->nspchannel,
                          rtjoin2->cfg->rob_winsize);
        memset(rtjoin2->residuals, 0,
               sizeof(float) * rtjoin2->cfg->rob_winsize);
        memset(rtjoin2->values_len, 0, sizeof(int) * rtjoin2->cfg->nmicchannel);
        for (i = 0; i < rtjoin2->cfg->nspchannel; ++i) {
            memset(rtjoin2->delay_idx[i], 0,
                   sizeof(int) * rtjoin2->cfg->delay_frame_cnt);
        }
        memset(rtjoin2->delay_idx_len, 0,
               sizeof(int) * rtjoin2->cfg->nspchannel);
        memset(rtjoin2->real_delay, 0, sizeof(int) * rtjoin2->cfg->nspchannel);
        memset(rtjoin2->real_delay_start, 0,
               sizeof(int) * rtjoin2->cfg->nspchannel);
    }
    if (rtjoin2->local_power) {
        wtk_float_zero_p2(rtjoin2->local_power, rtjoin2->cfg->nmicchannel,
                          rtjoin2->nbin);
        memset(rtjoin2->local_power_sum, 0,
               sizeof(float) * rtjoin2->cfg->nmicchannel);
        memset(rtjoin2->power, 0, sizeof(float) * rtjoin2->cfg->nmicchannel);
        memset(rtjoin2->local_weight, 0,
               sizeof(float) * rtjoin2->cfg->nmicchannel);
        memset(rtjoin2->weight, 0, sizeof(float) * rtjoin2->cfg->nmicchannel);
    }

    if (rtjoin2->mul_out) {
        memset(rtjoin2->mul_out, 0, sizeof(float) * (rtjoin2->nbin - 1));
    }
    memset(rtjoin2->out, 0, sizeof(float) * (rtjoin2->nbin - 1));

    if (rtjoin2->gc) {
        qtk_gain_controller_reset(rtjoin2->gc);
    }

    if (rtjoin2->vdr) {
        wtk_rtjoin2_edra_reset(rtjoin2->vdr);
    }

    rtjoin2->sp_silcnt = 0;
    rtjoin2->mic_silcnt = 0;
    rtjoin2->sp_sil = 1;
    rtjoin2->mic_sil = 1;

    rtjoin2->bs_scale = 1.0;
    rtjoin2->bs_last_scale = 1.0;
    rtjoin2->bs_real_scale = 1.0;
    rtjoin2->bs_max_cnt = 0;

    rtjoin2->gc_cnt = 0;

    rtjoin2->sp_frame = 0;
    rtjoin2->align_frame = 0;
    rtjoin2->align_complete = 0;
    rtjoin2->delay_complete = 0;
    rtjoin2->align_pos = 0;
    rtjoin2->play_cnt = 0;
}

void wtk_rtjoin2_set_notify(wtk_rtjoin2_t *rtjoin2, void *ths,
                            wtk_rtjoin2_notify_f notify) {
    rtjoin2->notify = notify;
    rtjoin2->ths = ths;
}

static float wtk_rtjoin2_sp_energy(float *p, int n) {
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

void wtk_rtjoin2_control_bs(wtk_rtjoin2_t *rtjoin2, float *out, int len) {
    float *bs_win = rtjoin2->bs_win;
    float max_out = rtjoin2->cfg->max_out;
    float out_max;
    int i;

    out_max = wtk_float_abs_max(out, len);
    if (out_max > max_out) {
        rtjoin2->bs_scale = max_out / out_max;
        if (rtjoin2->bs_scale < rtjoin2->bs_last_scale) {
            rtjoin2->bs_last_scale = rtjoin2->bs_scale;
        } else {
            rtjoin2->bs_scale = rtjoin2->bs_last_scale;
        }
        rtjoin2->bs_max_cnt = 5;
    }
    if (bs_win) {
        for (i = 0; i < len / 2; ++i) {
            out[i] *= rtjoin2->bs_scale * bs_win[i] +
                      rtjoin2->bs_real_scale * (1.0 - bs_win[i]);
        }
        for (i = len / 2; i < len; ++i) {
            out[i] *= rtjoin2->bs_scale;
        }
        rtjoin2->bs_real_scale = rtjoin2->bs_scale;
    } else {
        for (i = 0; i < len; ++i) {
            out[i] *= rtjoin2->bs_scale;
        }
    }
    if (rtjoin2->bs_max_cnt > 0) {
        --rtjoin2->bs_max_cnt;
    }
    if (rtjoin2->bs_max_cnt <= 0 && rtjoin2->bs_scale < 1.0) {
        rtjoin2->bs_scale *= 1.1f;
        rtjoin2->bs_last_scale = rtjoin2->bs_scale;
        if (rtjoin2->bs_scale > 1.0) {
            rtjoin2->bs_scale = 1.0;
            rtjoin2->bs_last_scale = 1.0;
        }
    }
}

void wtk_rtjoin2_feed_conv_fft(wtk_rtjoin2_t *rtjoin2, float *kernel,
                               float *data, int *idx, float *prob, int chn) {
    int kernel_len = rtjoin2->cfg->kernel_len;
    int slid_len = rtjoin2->cfg->slid_len;
    int conv_len = rtjoin2->cfg->conv_len;
    int nbin = conv_len / 2 + 1;
    int wins = rtjoin2->cfg->wins;
    int fsize = wins / 2;
    int N1 = rtjoin2->cfg->N1;
    float sp_conv_thresh = rtjoin2->cfg->sp_conv_thresh;
    wtk_drft_t *drft2 = rtjoin2->drft2;
    wtk_complex_t *kernel_fft = rtjoin2->kernel_fft;
    wtk_complex_t *data_fft = rtjoin2->data_fft;
    wtk_complex_t *res_fft = rtjoin2->res_fft;
    float *kernel_tmp = rtjoin2->kernel_tmp;
    float *data_tmp = rtjoin2->data_tmp;
    float *res_tmp = rtjoin2->res_tmp;
    int i;

    // memcpy(kernel_tmp, kernel, kernel_len * sizeof(float));
    // memcpy(data_tmp, data, slid_len * sizeof(float));

    for (i = 0; i < kernel_len; ++i) {
        kernel_tmp[i] = kernel[i] / 32768.0 * conv_len;
    }
    for (i = 0; i < slid_len; ++i) {
        data_tmp[i] = data[i] / 32768.0 * conv_len;
    }

    wtk_drft_fft22(drft2, kernel_tmp, kernel_fft);
    wtk_drft_fft22(drft2, data_tmp, data_fft);

    // 在频域中相乘
    for (i = 0; i < nbin; i++) {
        res_fft[i].a =
            kernel_fft[i].a * data_fft[i].a - kernel_fft[i].b * data_fft[i].b;
        res_fft[i].b =
            kernel_fft[i].a * data_fft[i].b + kernel_fft[i].b * data_fft[i].a;
    }

    wtk_drft_ifft22(drft2, res_fft, res_tmp);

    for (i = 0; i < conv_len; i++) {
        res_tmp[i] = res_tmp[i] / conv_len; // 除以FFT长度进行归一化
    }

    // 找到卷积结果中的最大值及其索引
    float max_value = res_tmp[0];
    int max_index = 0;
    for (i = 1; i < conv_len; i++) {
        if (fabs(res_tmp[i]) > max_value) {
            max_value = fabs(res_tmp[i]);
            max_index = i;
        }
    }
    if (max_value > sp_conv_thresh) {
        max_index = N1 * fsize - max_index + kernel_len;
        // if (chn == 2) {
        // printf("%d\n", max_index);
        // printf("%f\n", max_value);
        // }
        *idx = max_index;
        *prob = max_value;
    } else {
        *idx = -1;
        *prob = -1;
    }
}

void wtk_rtjoin2_feed_conv(wtk_rtjoin2_t *rtjoin2, float *kernel, float *data,
                           int *idx, float *prob) {
    int kernel_len = rtjoin2->cfg->kernel_len;
    int slid_len = rtjoin2->cfg->slid_len;
    int wins = rtjoin2->cfg->wins;
    int fsize = wins / 2;
    int N1 = rtjoin2->cfg->N1;
    float *res_tmp = rtjoin2->res_tmp;

    // 卷积结果的长度
    int result_len = slid_len - kernel_len + 1;

    // 进行卷积操作
    for (int i = 0; i < result_len; i++) {
        res_tmp[i] = 0;
        for (int j = 0; j < kernel_len; j++) {
            res_tmp[i] += kernel[j] * data[i + j];
        }
    }

    // 找到卷积结果中的最大值及其索引
    float max_value = res_tmp[0];
    int max_index = 0;
    for (int i = 1; i < result_len; i++) {
        if (res_tmp[i] > max_value) {
            max_value = res_tmp[i];
            max_index = i;
        }
    }

    max_index = N1 * fsize - max_index + kernel_len / 2;
    // printf("%d\n", max_index);
    // printf("%f\n", max_value*1.0/1e4);
    *idx = max_index;
    *prob = max_value;
}

void wtk_rtjoin2_robust_filter(wtk_rtjoin2_t *rtjoin2, int idx, float prob,
                               int chn) {
    float conf_decay = rtjoin2->cfg->rob_conf_decay;
    float mad_threshold = rtjoin2->cfg->rob_mad_threshold;
    float reliability_ratio = rtjoin2->cfg->rob_reliability_ratio;
    float rob_long_alpha1 = rtjoin2->cfg->rob_long_alpha1;
    float rob_long_alpha2 = rtjoin2->cfg->rob_long_alpha2;
    float delta;
    float conf_threshold;
    float *rob_values = rtjoin2->rob_values[chn];
    float *rob_weights = rtjoin2->rob_weights[chn];
    float *rob_values_tmp = rtjoin2->rob_values_tmp[chn];
    float *rob_weights_tmp = rtjoin2->rob_weights_tmp[chn];
    int rob_winsize = rtjoin2->cfg->rob_winsize;
    int *delay_idx = rtjoin2->delay_idx[chn];
    int delay_frame_cnt = rtjoin2->cfg->delay_frame_cnt;
    int delay_in_cnt = rtjoin2->cfg->delay_in_cnt;
    int i;
    float value;
    int f_idx;

    value = (float)idx;

    // ===== 1. 置信度过滤 =====
    conf_threshold =
        rtjoin2->rob_prob_mu[chn] - 2 * rtjoin2->rob_prob_sigma[chn];
    if (prob < conf_threshold) {
        return;
    }
    // 更新置信度统计 (仅用有效数据)
    rtjoin2->rob_prob_mu[chn] =
        conf_decay * rtjoin2->rob_prob_mu[chn] + (1 - conf_decay) * prob;
    delta = prob - rtjoin2->rob_prob_mu[chn];
    rtjoin2->rob_prob_sigma[chn] =
        sqrt(conf_decay * rtjoin2->rob_prob_sigma[chn] *
                 rtjoin2->rob_prob_sigma[chn] +
             (1 - conf_decay) * delta * delta);

    // ===== 2. 鲁棒预处理 =====
    // 计算当前窗口临时统计量
    float median;
    float mad;
    float *residuals = rtjoin2->residuals;
    if (rtjoin2->values_len[chn] > 0) {
        memcpy(rob_values_tmp, rob_values,
               rtjoin2->values_len[chn] * sizeof(float));
        median = wtk_float_median(rob_values_tmp, rtjoin2->values_len[chn]);
        for (i = 0; i < rtjoin2->values_len[chn]; ++i) {
            residuals[i] = fabs(rob_values[i] - median);
        }
        mad = 1.4826 * wtk_float_median(residuals, rtjoin2->values_len[chn]);
    } else {
        median = value;
        mad = 0;
    }

    // MAD异常值检测与权重调整
    float adj_weight;
    residuals[0] = fabs(value - median);
    if (mad > 0 && residuals[0] > mad_threshold * mad) {
        adj_weight = min(1.0, (mad_threshold * mad) / residuals[0]);
    } else {
        adj_weight = 1.0;
    }

    // ===== 3. 更新数据窗口 =====
    memcpy(rob_values + 1, rob_values, (rob_winsize - 1) * sizeof(float));
    rob_values[0] = value;
    memcpy(rob_weights + 1, rob_weights, (rob_winsize - 1) * sizeof(float));
    rob_weights[0] = prob * adj_weight;
    if (rtjoin2->values_len[chn] < rob_winsize) {
        ++rtjoin2->values_len[chn];
    }

    // ===== 4. 双模态估计 =====
    // 短期估计 (加权中位数)
    float short_term;
    if (rtjoin2->values_len[chn] > 0) {
        memcpy(rob_values_tmp, rob_values,
               rtjoin2->values_len[chn] * sizeof(float));
        memcpy(rob_weights_tmp, rob_weights,
               rtjoin2->values_len[chn] * sizeof(float));
        short_term = wtk_float_weighted_median(rob_values_tmp, rob_weights_tmp,
                                               rtjoin2->values_len[chn]);
    } else {
        short_term = value;
    }

    // 长期自适应融合
    float alpha;
    float estimate;
    alpha = max(
        0.0, min(1.0, 0.1 + 0.4 * (mad / rtjoin2->rob_long_mad[chn] + 1e-9)));
    estimate = alpha * short_term + (1 - alpha) * rtjoin2->rob_long_mu[chn];
    // ===== 5. 可靠性判断 =====
    float mad_ratio = fabs(estimate - rtjoin2->rob_long_mu[chn]) /
                      (rtjoin2->rob_long_mad[chn] + 1e-9);

    // ===== 6. 更新长期统计 =====
    float alpha_1;
    if (rtjoin2->delay_idx_len[chn] < delay_frame_cnt) {
        alpha = rob_long_alpha1;
        alpha_1 = 1.0 - alpha;
        rtjoin2->rob_long_mu[chn] =
            alpha * rtjoin2->rob_long_mu[chn] + alpha_1 * estimate;
        alpha = rob_long_alpha2;
        alpha_1 = 1.0 - alpha;
        delta = estimate - rtjoin2->rob_long_mu[chn];
        rtjoin2->rob_long_sigma[chn] =
            sqrt(alpha * rtjoin2->rob_long_sigma[chn] *
                     rtjoin2->rob_long_sigma[chn] +
                 alpha_1 * delta * delta);
        rtjoin2->rob_long_mad[chn] =
            alpha * rtjoin2->rob_long_mad[chn] + alpha_1 * mad;
    } else {
        alpha = rob_long_alpha2;
        alpha_1 = 1.0 - alpha;
        rtjoin2->rob_long_mu[chn] =
            alpha * rtjoin2->rob_long_mu[chn] + alpha_1 * estimate;
        delta = estimate - rtjoin2->rob_long_mu[chn];
        rtjoin2->rob_long_sigma[chn] =
            sqrt(alpha * rtjoin2->rob_long_sigma[chn] *
                     rtjoin2->rob_long_sigma[chn] +
                 alpha_1 * delta * delta);
        rtjoin2->rob_long_mad[chn] =
            alpha * rtjoin2->rob_long_mad[chn] + alpha_1 * mad;
    }

    if (mad_ratio > reliability_ratio) {
        return;
    } else {
        f_idx = (int)(estimate + 0.5);
        memcpy(delay_idx + 1, delay_idx, (delay_frame_cnt - 1) * sizeof(int));
        delay_idx[0] = f_idx;
        if (rtjoin2->delay_idx_len[chn] < delay_frame_cnt) {
            ++rtjoin2->delay_idx_len[chn];
        }
        f_idx = wtk_int_cnt_mode(delay_idx, rtjoin2->delay_idx_len[chn],
                                 delay_in_cnt);
        if (f_idx != 0) {
            rtjoin2->real_delay[chn] = f_idx;
            rtjoin2->real_delay_start[chn] = 1;
        }
    }
}

void wtk_rtjoin2_feed_align_signal(wtk_rtjoin2_t *rtjoin2) {
    wtk_strbuf_t **sp = rtjoin2->sp;
    int nspchannel = rtjoin2->cfg->nspchannel;
    int i, j;
    int pos;
    float *fv, fv1;
    float max_value[16] = {0};
    int max_idx[16] = {0};
    int *align_max_idx = rtjoin2->align_max_idx;
    float *align_max_value = rtjoin2->align_max_value;
    float **min_data = rtjoin2->min_data;

    wtk_drft_t *drft3 = rtjoin2->drft3;
    wtk_complex_t *a_kernel_fft = rtjoin2->a_kernel_fft;
    wtk_complex_t *a_data_fft = rtjoin2->a_data_fft;
    wtk_complex_t *a_res_fft = rtjoin2->a_res_fft;
    float *a_data_tmp = rtjoin2->a_data_tmp;
    float *a_res_tmp = rtjoin2->a_res_tmp;
    float **a_res = rtjoin2->a_res;
    float **align_res = rtjoin2->align_res;
    float **raw_sig = rtjoin2->raw_sig;
    int align_conv_len = rtjoin2->cfg->align_conv_len;
    float align_thresh = rtjoin2->cfg->align_thresh;
    int align_data_len = rtjoin2->cfg->align_data_len;
    int min_thresh_cnt = rtjoin2->cfg->min_thresh_cnt;
    int fsize = rtjoin2->cfg->wins / 2;
    int nbin = align_conv_len / 2 + 1;

    pos = sp[0]->pos / sizeof(float);

    if (pos >= align_data_len) {
        for (i = 0; i < nspchannel; ++i) {
            if (rtjoin2->align_state[i] > 0) {
                rtjoin2->align_max_idx[i] -= fsize;
            }
            fv = (float *)sp[i]->data;
            for (j = 0; j < align_data_len; ++j) {
                // a_data_tmp[j] = fv[j] / 32767.0 * align_conv_len;
                a_data_tmp[j] = fv[j] / 32767.0;
            }
            wtk_drft_fft22(drft3, a_data_tmp, a_data_fft);
            for (j = 0; j < nbin; ++j) {
                a_res_fft[j].a = a_kernel_fft[j].a * a_data_fft[j].a -
                                 a_kernel_fft[j].b * a_data_fft[j].b;
                a_res_fft[j].b = a_kernel_fft[j].a * a_data_fft[j].b +
                                 a_kernel_fft[j].b * a_data_fft[j].a;
            }
            wtk_drft_ifft22(drft3, a_res_fft, a_res_tmp);
            for (j = 0; j < align_conv_len; ++j) {
                // a_res_tmp[j] = a_res_tmp[j] / align_conv_len;
                a_res[i][j] += a_res_tmp[j];
            }
            memcpy(align_res[i], align_res[i] + fsize,
                   sizeof(float) * (align_data_len - fsize));
            memcpy(align_res[i] + align_data_len - fsize, a_res[i],
                   sizeof(float) * fsize);
            memcpy(raw_sig[i], raw_sig[i] + fsize,
                   sizeof(float) * (align_data_len - fsize));
            memcpy(raw_sig[i] + align_data_len - fsize, a_data_tmp,
                   sizeof(float) * fsize);

            memcpy(a_res[i], a_res[i] + fsize,
                   sizeof(float) * (align_conv_len - fsize));
        }
        rtjoin2->align_frame++;
        if (rtjoin2->align_frame == rtjoin2->cfg->align_min_frame) {
            rtjoin2->align_frame = 0;
            for (i = 0; i < nspchannel; ++i) {
                max_value[i] = align_res[i][0];
                max_idx[i] = 0;
                for (j = 1; j < align_data_len; ++j) {
                    if (align_res[i][j] > max_value[i]) {
                        max_value[i] = align_res[i][j];
                        max_idx[i] = j;
                    }
                }
                fv1 = wtk_float_min(raw_sig[i], align_data_len);
                memcpy(min_data[i], min_data[i] + 1,
                       sizeof(float) * (min_thresh_cnt - 1));
                min_data[i][min_thresh_cnt - 1] = fv1;
            }
            // printf("%f\n", max_value[0]);
            // if (max_value[0] > 0.1) {
            //     printf("%d\n", max_idx[0]);
            // } else {
            //     printf("0\n");
            // }

            for (i = 0; i < nspchannel; ++i) {
                fv1 = wtk_float_min(min_data[i], min_thresh_cnt);
                if (rtjoin2->align_state[i] == 0) {
                    if (max_value[i] > align_thresh) {
                        if (fv1 < rtjoin2->cfg->align_min_thresh &&
                            wtk_float_mean(raw_sig[i], align_data_len) <
                                rtjoin2->cfg->align_mean_thresh) {
                            rtjoin2->align_state[i] = 1;
                            align_max_idx[i] = max_idx[i];
                            align_max_value[i] = max_value[i];
                        } else {
                            rtjoin2->align_state[i] = 0;
                            align_max_idx[i] = 0;
                            align_max_value[i] = 0;
                        }
                    }
                } else if (rtjoin2->align_state[i] == 1) {
                    if (max_value[i] < align_max_value[i]) {
                        rtjoin2->align_state[i] = 2;
                    } else if (max_value[i] > align_thresh) {
                        if (fv1 < rtjoin2->cfg->align_min_thresh &&
                            wtk_float_mean(raw_sig[i], align_data_len) <
                                rtjoin2->cfg->align_mean_thresh) {
                            rtjoin2->align_state[i] = 1;
                            align_max_idx[i] = max_idx[i];
                            align_max_value[i] = max_value[i];
                        } else {
                            rtjoin2->align_state[i] = 0;
                            align_max_idx[i] = 0;
                            align_max_value[i] = 0;
                        }
                    } else {
                        rtjoin2->align_state[i] = 0;
                        align_max_idx[i] = 0;
                        align_max_value[i] = 0;
                    }
                } else if (rtjoin2->align_state[i] == 2) {
                    if (max_value[i] < align_thresh) {
                        rtjoin2->align_cnt[i]++;
                    }
                    if (rtjoin2->align_cnt[i] > 2) {
                        rtjoin2->align_state[i] = 0;
                        align_max_idx[i] = 0;
                        align_max_value[i] = 0;
                        rtjoin2->align_cnt[i] = 0;
                    }
                }
            }
            for (i = 0; i < nspchannel; ++i) {
                if (rtjoin2->align_state[i] == 2) {
                    rtjoin2->align_complete = 1;
                } else {
                    rtjoin2->align_complete = 0;
                    break;
                }
            }
            // printf("%f\n", max_value[0]*1e-3);
            // printf("%f\n", wtk_float_min(raw_sig[0], align_data_len));
            // for (i = 0; i < nspchannel; ++i)
            // {
            //     printf("%d ", rtjoin2->align_state[i]);
            // }
            // for (i = 0; i < nspchannel; ++i)
            // {
            //     // printf("%d %d ", max_idx[i], align_max_idx[i]);
            //     printf("%d ", align_max_idx[i]);
            // }
            // for (i = 0; i < nspchannel; ++i)
            // {
            //     printf("%f %f ", max_value[i], align_max_value[i]);
            // }
            // for (i = 0; i < nspchannel; ++i)
            // {
            //     fv = (float *)sp[i]->data;
            //     printf("%f ", wtk_float_min(fv, rtjoin2->cfg->N *
            //     rtjoin2->cfg->wins / 2)); printf("%f ", wtk_float_mean(fv,
            //     rtjoin2->cfg->N * rtjoin2->cfg->wins / 2));
            // }
            // printf("\n");
        }
    }
}

void wtk_rtjoin2_feed_align(wtk_rtjoin2_t *rtjoin2) {
    wtk_strbuf_t **sp = rtjoin2->sp;
    int nspchannel = rtjoin2->cfg->nspchannel;
    int play_cnt = rtjoin2->cfg->play_cnt;
    int i, j;
    float spenr;
    float spenr_thresh = rtjoin2->cfg->spenr_thresh;
    int spenr_cnt = rtjoin2->cfg->spenr_cnt;
    int N1 = rtjoin2->cfg->N1;
    // int N2=rtjoin2->cfg->N2;
    // int N=rtjoin2->cfg->N;
    int kernel_len = rtjoin2->cfg->kernel_len;
    int wins = rtjoin2->cfg->wins;
    int fsize = wins / 2;
    float *kernel = rtjoin2->kernel;
    float *fv;
    float *data;
    int idx;
    float prob;

    spenr = wtk_rtjoin2_sp_energy((float *)sp[0]->data + N1 * fsize, fsize);
    // static int cnt=0;
    // cnt++;
    if (spenr > spenr_thresh) {
        // if(rtjoin2->sp_sil==1)
        // {
        // 	printf("sp start %f %f
        // %f\n", 1.0/16000*cnt*(nbin-1),spenr,spenr_thresh);
        // }
        rtjoin2->sp_sil = 0;
        rtjoin2->sp_silcnt = spenr_cnt;
    } else if (rtjoin2->sp_sil == 0) {
        rtjoin2->sp_silcnt -= 1;
        if (rtjoin2->sp_silcnt <= 0) {
            // printf("sp end %f\n", 1.0/16000*cnt*(nbin-1));
            rtjoin2->sp_sil = 1;
        }
    }
    if (rtjoin2->cfg->use_align_signal) {
        wtk_rtjoin2_feed_align_signal(rtjoin2);
        if (rtjoin2->align_complete == 1) {
            if (play_cnt > 1) {
                for (i = 0; i < nspchannel; ++i) {
                    memcpy(rtjoin2->align_delay[i], rtjoin2->align_delay[i] + 1,
                           sizeof(int) * (play_cnt - 1));
                }
            }
            idx = wtk_int_max(rtjoin2->align_max_idx, nspchannel);
            // idx = wtk_int_min(rtjoin2->align_max_idx, nspchannel);
            for (i = 0; i < nspchannel; ++i) {
                rtjoin2->align_delay[i][play_cnt - 1] =
                    idx - rtjoin2->align_max_idx[i];
                // rtjoin2->align_delay[play_cnt - 1][i] =
                // rtjoin2->align_max_idx[i] - idx;
                // printf("%d ", rtjoin2->align_delay[i][play_cnt - 1]);
                rtjoin2->align_state[i] = 0;
                rtjoin2->align_max_idx[i] = 0;
                rtjoin2->align_max_idx[i] = 0;
                rtjoin2->align_cnt[i] = 0;
            }
            // printf("\n");
            if (play_cnt == 1) {
                for (i = 0; i < nspchannel; ++i) {
                    rtjoin2->real_delay[i] =
                        rtjoin2->align_delay[i][play_cnt - 1];
                    // printf("1 %d ", rtjoin2->real_delay[i]);
                }
                // printf("\n");
            } else {
                ++rtjoin2->play_cnt;
                if (rtjoin2->play_cnt == play_cnt) {
                    int blank[10] = {0};
                    int blank_state = 1;
                    for (j = 1; j < play_cnt; ++j) {
                        blank[j] = rtjoin2->align_delay[0][j] -
                                   rtjoin2->align_delay[0][0];
                    }
                    for (i = 0; i < nspchannel; ++i) {
                        for (j = 1; j < play_cnt; ++j) {
                            if (abs((rtjoin2->align_delay[i][j] -
                                     rtjoin2->align_delay[i][0]) -
                                    blank[j]) > 5) {
                                blank_state = 0;
                                break;
                            }
                        }
                        if (blank_state == 0) {
                            break;
                        }
                    }
                    if (blank_state == 1) {
                        for (i = 0; i < nspchannel; ++i) {
                            rtjoin2->real_delay[i] =
                                rtjoin2->align_delay[i][play_cnt - 1];
                            // printf("2 %d ", rtjoin2->real_delay[i]);
                            memset(rtjoin2->align_delay[i], 0,
                                   sizeof(int) * play_cnt);
                        }
                        rtjoin2->delay_complete = 1;
                        // printf("\n");
                        rtjoin2->play_cnt = 0;
                    } else {
                        rtjoin2->play_cnt--;
                    }
                }
            }
            rtjoin2->align_complete = 0;
        }
    }
    // ++rtjoin2->sp_frame;
    // if (rtjoin2->sp_frame < rtjoin2->cfg->N) {
    //     return;
    // }
    // rtjoin2->sp_frame = 0;
    if (rtjoin2->sp_sil == 0) {
        fv = (float *)sp[0]->data + N1 * fsize;
        for (i = 0; i < kernel_len; ++i) {
            kernel[i] = fv[kernel_len - 1 - i];
        }
        for (i = 1; i < nspchannel; ++i) {
            data = (float *)sp[i]->data;
            if (rtjoin2->cfg->use_conv_fft) {
                wtk_rtjoin2_feed_conv_fft(rtjoin2, kernel, data, &idx, &prob,
                                          i);
            } else {
                wtk_rtjoin2_feed_conv(rtjoin2, kernel, data, &idx, &prob);
            }
            // if(i==2){
            // 	printf("%d\n", idx);
            // }
            if (prob > 0) {
                // if(i==2){
                // printf("%d\n", idx);
                // printf("%f\n", prob);
                // }
                if (rtjoin2->cfg->use_rob_filter) {
                    wtk_rtjoin2_robust_filter(rtjoin2, idx, prob, i);
                } else {
                    rtjoin2->real_delay[i] = idx;
                }
            }
        }
    } else {
        // printf("-1\n");
    }
}

static float wtk_rtjoin2_speech_energy(float *p, int n, int min_n, int max_n) {
    float f, f2;
    int i;
    min_n = max(0, min_n);
    max_n = min(n, max_n);

    f = 0;
    for (i = min_n; i < max_n; ++i) {
        f += p[i];
    }
    f /= n;

    f2 = 0;
    for (i = min_n; i < max_n; ++i) {
        f2 += (p[i] - f) * (p[i] - f);
    }
    f2 /= n;

    return f2;
}

void wtk_rtjoin2_feed_avg_mix(wtk_rtjoin2_t *rtjoin2, wtk_complex_t **fft,
                              wtk_complex_t *fftx) {
    int nbin = rtjoin2->nbin;
    int nmicchannel = rtjoin2->cfg->nmicchannel;
    float micenr_thresh = rtjoin2->cfg->micenr_thresh;
    int micenr_cnt = rtjoin2->cfg->micenr_cnt;
    float power_alpha = rtjoin2->cfg->power_alpha;
    float power_alpha_1 = 1.0 - power_alpha;
    float weight_alpha = rtjoin2->cfg->weight_alpha;
    float weight_alpha_1 = 1.0 - weight_alpha;
    float weight_thresh = rtjoin2->cfg->weight_thresh;

    float **local_power = rtjoin2->local_power;
    float *local_power_sum = rtjoin2->local_power_sum;
    float *power = rtjoin2->power;
    float *local_weight = rtjoin2->local_weight;
    float *weight = rtjoin2->weight;
    float micenr = 0;
    float power_sum = 1e-9;
    float weight_sum = 1e-9;
    float fft_scale = rtjoin2->cfg->wins;
    int i, k;

    for (i = 0; i < nmicchannel; ++i) {
        local_power_sum[i] = 0;
        for (k = 0; k < nbin; ++k) {
            local_power[i][k] =
                sqrtf(fft[i][k].a * fft[i][k].a + fft[i][k].b * fft[i][k].b);
            local_power_sum[i] += local_power[i][k];
        }
        micenr = max(wtk_rtjoin2_speech_energy(local_power[i], nbin, 0, nbin) *
                         fft_scale,
                     micenr);
    }
    // printf("%f\n", micenr);
    if (micenr > micenr_thresh) {
        rtjoin2->mic_sil = 0;
        rtjoin2->mic_silcnt = micenr_cnt;
    } else if (rtjoin2->mic_sil == 0) {
        rtjoin2->mic_silcnt -= 1;
        if (rtjoin2->mic_silcnt <= 0) {
            rtjoin2->mic_sil = 1;
        }
    }

    if (rtjoin2->mic_sil == 0) {
        for (i = 0; i < nmicchannel; ++i) {
            power[i] =
                power_alpha * power[i] + power_alpha_1 * local_power_sum[i];
            power_sum += power[i];
        }
        for (i = 0; i < nmicchannel; ++i) {
            local_weight[i] = power[i] / power_sum;
            if (local_weight[i] >= weight_thresh) {
                local_weight[i] = weight_thresh;
            } else if (local_weight[i] < weight_thresh * 0.25) {
                local_weight[i] =
                    local_weight[i] * local_weight[i] / (weight_thresh * 0.25);
            }
            weight_sum += local_weight[i];
        }
        for (i = 0; i < nmicchannel; ++i) {
            local_weight[i] = local_weight[i] / weight_sum;
            weight[i] =
                weight_alpha * weight[i] + weight_alpha_1 * local_weight[i];
        }
    }

    memset(fftx, 0, nbin * sizeof(wtk_complex_t));
    if (rtjoin2->delay_complete == 0) {
        int max_idx = 0;
        float max_value = 0;
        for (i = 0; i < nmicchannel; ++i) {
            if (weight[i] > max_value) {
                max_value = weight[i];
                max_idx = i;
            }
        }
        for (k = 0; k < nbin; ++k) {
            fftx[k].a = fft[max_idx][k].a * weight[max_idx];
            fftx[k].b = fft[max_idx][k].b * weight[max_idx];
        }
    } else {
        for (k = 0; k < nbin; ++k) {
            for (i = 0; i < nmicchannel; ++i) {
                fftx[k].a += fft[i][k].a * weight[i];
                fftx[k].b += fft[i][k].b * weight[i];
            }
        }
    }
}

void wtk_rtjoin2_feed_mix(wtk_rtjoin2_t *rtjoin2, wtk_complex_t **fft,
                          wtk_complex_t *fftx) {
    // int fsize=rtjoin2->cfg->wins/2;
    int nbin = rtjoin2->nbin;
    int nmicchannel = rtjoin2->cfg->nmicchannel;
    int clip_s = rtjoin2->cfg->clip_s;
    int clip_e = rtjoin2->cfg->clip_e;
    int i, k;

    memset(fftx, 0, nbin * sizeof(wtk_complex_t));
    // float sum=0;
    // float diff=0;
    // int a = 2;
    // int b = 1;
    for (k = clip_s; k < clip_e; ++k) {
        // diff = (fft[a][k].a*fft[a][k].a +
        // fft[a][k].b*fft[a][k].b)-(fft[b][k].a*fft[b][k].a +
        // fft[b][k].b*fft[b][k].b); if(diff>0){ 	sum += diff;
        // }
        for (i = 0; i < nmicchannel; ++i) {
            fftx[k].a += fft[i][k].a;
            fftx[k].b += fft[i][k].b;
        }
        fftx[k].a /= nmicchannel;
        fftx[k].b /= nmicchannel;
    }
    // printf("%f\n", sum);
}

void wtk_rtjoin2_aec_on_gainnet2(wtk_rtjoin2_edra_t *vdr, float *gain, int len,
                                 int is_end) {
    memcpy(vdr->g, gain, sizeof(float) * vdr->cfg->bankfeat.nb_bands);
}

void wtk_rtjoin2_edra_feed_gainnet2(wtk_rtjoin2_t *rtjoin2, wtk_complex_t *fftx,
                                    wtk_complex_t *ffty) {

    wtk_rtjoin2_edra_t *vdr = rtjoin2->vdr;
    int nbin = vdr->nbin;
    float *g = vdr->g, *gf = vdr->gf;
    wtk_gainnet2_t *gainnet2 = vdr->gainnet2;
    wtk_bankfeat_t *bank_mic = vdr->bank_mic;
    wtk_bankfeat_t *bank_sp = vdr->bank_sp;
    int featsp_lm = vdr->cfg->featsp_lm;
    int featm_lm = vdr->cfg->featm_lm;
    float *feature_sp = vdr->feature_sp;
    int nb_features = bank_mic->cfg->nb_features;

    wtk_bankfeat_flush_frame_features(bank_mic, fftx);
    wtk_bankfeat_flush_frame_features(bank_sp, ffty);
    if (feature_sp && featsp_lm > 1) {
        memmove(feature_sp + nb_features * featm_lm,
                feature_sp + nb_features * (featm_lm - 1),
                sizeof(float) * nb_features * (featsp_lm - 1));
        memcpy(feature_sp + nb_features * (featm_lm - 1), bank_sp->features,
               sizeof(float) * nb_features);
    }
    if (feature_sp) {
        wtk_gainnet2_feed2(gainnet2, bank_mic->features, nb_features,
                           feature_sp, nb_features * (featm_lm + featsp_lm - 1),
                           0);
    } else {
        wtk_gainnet2_feed2(gainnet2, bank_mic->features, nb_features,
                           bank_sp->features, nb_features, 0);
    }
    wtk_bankfeat_interp_band_gain(bank_mic, nbin, gf, g);
    if (feature_sp && featm_lm > 1) {
        memmove(feature_sp + nb_features, feature_sp,
                sizeof(float) * nb_features * (featm_lm - 2));
        memcpy(feature_sp, bank_mic->features, sizeof(float) * nb_features);
    }
}

void wtk_rtjoin2_feed(wtk_rtjoin2_t *rtjoin2, short *data, int len,
                      int is_end) {
    int i, j, k;
    int nmicchannel = rtjoin2->cfg->nmicchannel;
    int *mic_channel = rtjoin2->cfg->mic_channel;
    int nspchannel = rtjoin2->cfg->nspchannel;
    int *sp_channel = rtjoin2->cfg->sp_channel;
    int channel = rtjoin2->cfg->channel;
    wtk_strbuf_t **mic = rtjoin2->mic;
    wtk_strbuf_t **sp = rtjoin2->sp;
    float fv, *fp1;
    int wins = rtjoin2->cfg->wins;
    int fsize = wins / 2;
    int length;
    float *rfft_in = rtjoin2->rfft_in;
    wtk_drft_t *rfft = rtjoin2->rfft;
    wtk_complex_t **fft = rtjoin2->fft;
    wtk_complex_t **fft_sp = rtjoin2->fft_sp;
    float **analysis_mem = rtjoin2->analysis_mem;
    float **analysis_mem_sp = rtjoin2->analysis_mem_sp;
    float *synthesis_mem = rtjoin2->synthesis_mem;
    float **mul_synthesis_mem = rtjoin2->mul_synthesis_mem;
    float *analysis_window = rtjoin2->analysis_window,
          *synthesis_window = rtjoin2->synthesis_window;
    wtk_complex_t *fftx = rtjoin2->fftx;
    float *mul_out = rtjoin2->mul_out;
    float *out = rtjoin2->out;
    short *pv = (short *)out;
    int nbin = rtjoin2->nbin;
    int clip_s = rtjoin2->cfg->clip_s;
    int clip_e = rtjoin2->cfg->clip_e;
    int slid_len = rtjoin2->cfg->slid_len;
    int N1 = rtjoin2->cfg->N1;
    int N2 = rtjoin2->cfg->N2;
    float gc_mask = 1.0;

    for (i = 0; i < len; ++i) {
        for (j = 0; j < nmicchannel; ++j) {
            fv = data[mic_channel[j]];
            wtk_strbuf_push(mic[j], (char *)(&fv), sizeof(float));
        }
        for (j = 0; j < nspchannel; ++j) {
            fv = data[sp_channel[j]];
            wtk_strbuf_push(sp[j], (char *)(&fv), sizeof(float));
        }
        data += channel;
    }
    length = mic[0]->pos / sizeof(float);
    // if(length>slid_len){
    // 	wtk_strbufs_pop(mic, nmicchannel, fsize*sizeof(float));
    // 	wtk_strbufs_pop(sp, nspchannel, fsize*sizeof(float));
    // }

    while (length >= slid_len) {
        if (nspchannel == nmicchannel) {
            wtk_rtjoin2_feed_align(rtjoin2);
        }
        // printf("%d\n", rtjoin2->align_complete);
        // for (i = 0; i < nmicchannel; ++i) {
        //     printf("%d ", rtjoin2->real_delay[i]);
        //     printf("%d ", min(max(N1 * fsize - rtjoin2->real_delay[i], 0),
        //                       (N1 + N2) * fsize));
        // }
        // printf("\n");
        // printf("%d\n", min(max(N1 * fsize + rtjoin2->real_delay[2], 0),
        //                    (N1 + N2) * fsize));

        for (i = 0; i < nmicchannel; ++i) {
            fp1 = (float *)mic[i]->data +
                  min(max(N1 * fsize - rtjoin2->real_delay[i], 0),
                      (N1 + N2) * fsize);
            // fp1=(float *)mic[i]->data+min(max(N1*fsize,
            // min(max(N1*fsize-rtjoin2->real_delay[i], 0), (N1+N2)*fsize)),
            // N1*fsize);
            wtk_drft_frame_analysis(rfft, rfft_in, analysis_mem[i], fft[i], fp1,
                                    wins, analysis_window);
        }
        for (i = 0; i < nspchannel; ++i) {
            fp1 = (float *)sp[i]->data;
            wtk_drft_frame_analysis(rfft, rfft_in, analysis_mem_sp[i],
                                    fft_sp[i], fp1, wins, analysis_window);
        }

        if (rtjoin2->cfg->use_avg_mix) {
            wtk_rtjoin2_feed_avg_mix(rtjoin2, fft, fftx);
        } else {
            wtk_rtjoin2_feed_mix(rtjoin2, fft, fftx);
        }

        for (k = 0; k <= clip_s; ++k) {
            fftx[k].a = fftx[k].b = 0;
        }
        for (k = clip_e; k < nbin; ++k) {
            fftx[k].a = fftx[k].b = 0;
        }

        if (rtjoin2->cfg->use_gainnet2) {
            wtk_complex_t *ffty = rtjoin2->ffty;
            wtk_rtjoin2_edra_feed_gainnet2(rtjoin2, fftx, ffty);
            gc_mask = wtk_float_abs_mean(rtjoin2->vdr->gf, nbin);
        }

        if (gc_mask > rtjoin2->cfg->gc_min_thresh) {
            rtjoin2->gc_cnt = rtjoin2->cfg->gc_cnt;
        } else {
            --rtjoin2->gc_cnt;
        }
        if (rtjoin2->gc_cnt >= 0) {
            if (rtjoin2->gc) {
                qtk_gain_controller_run(rtjoin2->gc, fftx, fsize, NULL,
                                        gc_mask);
            }
            if (rtjoin2->gc_cnt < rtjoin2->cfg->gc_cnt) {
                float gain =
                    powf(rtjoin2->gc_cnt * 1.0 / rtjoin2->cfg->gc_cnt, 0.5);
                for (i = 0; i < nbin; ++i) {
                    fftx[i].a *= gain;
                    fftx[i].b *= gain;
                }
            }
        } else {
            memset(fftx, 0, sizeof(wtk_complex_t) * nbin);
        }

        if (rtjoin2->cfg->use_mul_out) {
            for (i = 0; i < nmicchannel; ++i) {
                wtk_drft_frame_synthesis(rfft, rfft_in, mul_synthesis_mem[i],
                                         fft[i], mul_out, wins,
                                         synthesis_window);
                for (j = 0; j < fsize; ++j) {
                    out[j * nmicchannel + i] = mul_out[j];
                }
            }
            for (i = 0; i < fsize * nmicchannel; ++i) {
                pv[i] = floorf(out[i] + 0.5);
            }
            if (rtjoin2->notify) {
                rtjoin2->notify(rtjoin2->ths, pv, fsize * nmicchannel);
            }
        } else {
            wtk_drft_frame_synthesis(rfft, rfft_in, synthesis_mem, fftx, out,
                                     wins, synthesis_window);
            if (rtjoin2->eq) {
                wtk_equalizer_feed_float(rtjoin2->eq, out, fsize);
            }
            if (rtjoin2->cfg->use_control_bs) {
                wtk_rtjoin2_control_bs(rtjoin2, out, fsize);
            }
            for (i = 0; i < fsize; ++i) {
                pv[i] = floorf(out[i] + 0.5);
            }
            if (rtjoin2->notify) {
                rtjoin2->notify(rtjoin2->ths, pv, fsize);
            }
        }
        wtk_strbufs_pop(mic, nmicchannel, fsize * sizeof(float));
        wtk_strbufs_pop(sp, nspchannel, fsize * sizeof(float));
        rtjoin2->align_pos += fsize;
        length = mic[0]->pos / sizeof(float);
    }
}

short *wtk_rtjoin2_get_play_signal(wtk_rtjoin2_t *rtjoin2, int *len) {
    if (rtjoin2->cfg->use_align_signal) {
        *len = rtjoin2->cfg->play_signal_len;
        return rtjoin2->cfg->play_signal;
    } else {
        wtk_debug("wtk_rtjoin2_get_play_signal: use_align_signal is false, "
                  "play signal not available.");
        return NULL;
    }
}
