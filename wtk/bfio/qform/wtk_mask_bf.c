#include "wtk/bfio/qform/wtk_mask_bf.h"

wtk_mask_bf_t *wtk_mask_bf_new(wtk_mask_bf_cfg_t *cfg) {
    wtk_mask_bf_t *mask_bf;

    mask_bf = (wtk_mask_bf_t *)wtk_malloc(sizeof(wtk_mask_bf_t));
    mask_bf->cfg = cfg;
    mask_bf->ths = NULL;
    mask_bf->notify = NULL;

    mask_bf->s_mask = (float *)wtk_malloc(sizeof(float) * cfg->nbin);
    mask_bf->n_mask = (float *)wtk_malloc(sizeof(float) * cfg->nbin);
    mask_bf->p_mask = (float *)wtk_malloc(sizeof(float) * cfg->nbin);
    mask_bf->power = wtk_float_new_p2(cfg->channel, cfg->nbin);
    mask_bf->s_power = (float *)wtk_malloc(sizeof(float) * cfg->channel);
    mask_bf->n_power = (float *)wtk_malloc(sizeof(float) * cfg->channel);
    mask_bf->eta = (float *)wtk_malloc(sizeof(float) * cfg->nbin);
    mask_bf->epsi = (float *)wtk_malloc(sizeof(float) * cfg->nbin);
    mask_bf->update_speech = (int *)wtk_malloc(sizeof(int) * cfg->nbin);
    mask_bf->update_noise = (int *)wtk_malloc(sizeof(int) * cfg->nbin);
    mask_bf->entropy_E = (float *)wtk_malloc(sizeof(float) * cfg->nbin);
    mask_bf->entropy_Eb = (float *)wtk_malloc(sizeof(float) * cfg->wins);
    mask_bf->fft_tmp = (wtk_complex_t *)wtk_malloc(
        sizeof(wtk_complex_t) * cfg->nbin); //// max(nbin, channel*channel*2)
    mask_bf->covar = wtk_complex_new_p2(cfg->nbin, cfg->channelx2);
    mask_bf->Rss = wtk_complex_new_p2(cfg->nbin, cfg->channelx2);
    mask_bf->Rnn = wtk_complex_new_p2(cfg->nbin, cfg->channelx2);
    mask_bf->Rss_e = wtk_complex_new_p2(cfg->nbin, cfg->channelx2);
    mask_bf->Rnn_e = wtk_complex_new_p2(cfg->nbin, cfg->channelx2);
    mask_bf->Rss_norm = wtk_complex_new_p2(cfg->nbin, cfg->channelx2);
    mask_bf->Rnn_norm = wtk_complex_new_p2(cfg->nbin, cfg->channelx2);
    mask_bf->Rss_ =
        (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * cfg->channelx2);
    mask_bf->Rnn_ =
        (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * cfg->channelx2);
    mask_bf->vec = wtk_complex_new_p2(cfg->nbin, cfg->channel);
    mask_bf->w = wtk_complex_new_p2(cfg->nbin, cfg->channel);
    mask_bf->c_temp =
        (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * cfg->channel);
    mask_bf->fftx = (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * cfg->nbin);
    mask_bf->Rss_cnt = (float *)wtk_malloc(sizeof(float) * cfg->nbin);
    mask_bf->Rnn_cnt = (float *)wtk_malloc(sizeof(float) * cfg->nbin);
    mask_bf->Rss_cnt_e = (float *)wtk_malloc(sizeof(float) * cfg->nbin);
    mask_bf->Rnn_cnt_e = (float *)wtk_malloc(sizeof(float) * cfg->nbin);
    mask_bf->scnt = (int *)wtk_malloc(sizeof(int) * cfg->nbin);
    mask_bf->ncnt = (int *)wtk_malloc(sizeof(int) * cfg->nbin);

    mask_bf->s_power_acc = (float *)wtk_malloc(sizeof(float) * cfg->channel);
    mask_bf->n_power_acc = (float *)wtk_malloc(sizeof(float) * cfg->channel);
    mask_bf->s_power_acc_e = (float *)wtk_malloc(sizeof(float) * cfg->channel);
    mask_bf->n_power_acc_e = (float *)wtk_malloc(sizeof(float) * cfg->channel);
    mask_bf->speech_power = (float *)wtk_malloc(sizeof(float) * cfg->channel);
    mask_bf->noise_power = (float *)wtk_malloc(sizeof(float) * cfg->channel);
    mask_bf->snr = (float *)wtk_malloc(sizeof(float) * cfg->channel);

    wtk_mask_bf_reset(mask_bf);

    return mask_bf;
}

void wtk_mask_bf_reset(wtk_mask_bf_t *mask_bf) {
    int channel = mask_bf->cfg->channel;
    int channelx2 = mask_bf->cfg->channelx2;
    int nbin = mask_bf->cfg->nbin;
    int wins = mask_bf->cfg->wins;

    memset(mask_bf->s_mask, 0, sizeof(float) * nbin);
    memset(mask_bf->n_mask, 0, sizeof(float) * nbin);
    memset(mask_bf->p_mask, 0, sizeof(float) * nbin);
    wtk_float_zero_p2(mask_bf->power, channel, nbin);
    memset(mask_bf->s_power, 0, sizeof(float) * channel);
    memset(mask_bf->n_power, 0, sizeof(float) * channel);
    memset(mask_bf->eta, 0, sizeof(float) * nbin);
    memset(mask_bf->epsi, 0, sizeof(float) * nbin);
    memset(mask_bf->update_speech, 0, sizeof(int) * nbin);
    memset(mask_bf->update_noise, 0, sizeof(int) * nbin);
    memset(mask_bf->entropy_E, 0, sizeof(float) * nbin);
    memset(mask_bf->entropy_Eb, 0, sizeof(float) * wins);
    memset(mask_bf->fft_tmp, 0, sizeof(wtk_complex_t) * nbin);
    wtk_complex_zero_p2(mask_bf->covar, nbin, channelx2);
    wtk_complex_zero_p2(mask_bf->Rss, nbin, channelx2);
    wtk_complex_zero_p2(mask_bf->Rnn, nbin, channelx2);
    wtk_complex_zero_p2(mask_bf->Rss_e, nbin, channelx2);
    wtk_complex_zero_p2(mask_bf->Rnn_e, nbin, channelx2);
    wtk_complex_zero_p2(mask_bf->Rss_norm, nbin, channelx2);
    wtk_complex_zero_p2(mask_bf->Rnn_norm, nbin, channelx2);
    memset(mask_bf->Rss_, 0, sizeof(wtk_complex_t) * channelx2);
    memset(mask_bf->Rnn_, 0, sizeof(wtk_complex_t) * channelx2);
    wtk_complex_zero_p2(mask_bf->vec, nbin, channel);
    wtk_complex_zero_p2(mask_bf->w, nbin, channel);
    memset(mask_bf->c_temp, 0, sizeof(wtk_complex_t) * channel);
    memset(mask_bf->fftx, 0, sizeof(wtk_complex_t) * nbin);
    memset(mask_bf->Rss_cnt, 0, sizeof(float) * nbin);
    memset(mask_bf->Rnn_cnt, 0, sizeof(float) * nbin);
    memset(mask_bf->Rss_cnt_e, 0, sizeof(float) * nbin);
    memset(mask_bf->Rnn_cnt_e, 0, sizeof(float) * nbin);
    memset(mask_bf->scnt, 0, sizeof(int) * nbin);
    memset(mask_bf->ncnt, 0, sizeof(int) * nbin);

    memset(mask_bf->s_power_acc, 0, sizeof(float) * channel);
    memset(mask_bf->n_power_acc, 0, sizeof(float) * channel);
    memset(mask_bf->s_power_acc_e, 0, sizeof(float) * channel);
    memset(mask_bf->n_power_acc_e, 0, sizeof(float) * channel);
    memset(mask_bf->speech_power, 0, sizeof(float) * channel);
    memset(mask_bf->noise_power, 0, sizeof(float) * channel);
    memset(mask_bf->snr, 0, sizeof(float) * channel);

    mask_bf->scov_alpha = mask_bf->cfg->scov_alpha;
    mask_bf->snr_acc = 0;
    mask_bf->snr_acc_e = 0;
    mask_bf->s_power_cnt = 0;
    mask_bf->n_power_cnt = 0;
    mask_bf->s_power_cnt_e = 0;
    mask_bf->n_power_cnt_e = 0;
    mask_bf->ref_channel = 0;
    mask_bf->nframe = 0;
    mask_bf->update_w = 0;
}

void wtk_mask_bf_delete(wtk_mask_bf_t *mask_bf) {
    int channel = mask_bf->cfg->channel;
    int nbin = mask_bf->cfg->nbin;

    wtk_free(mask_bf->s_mask);
    wtk_free(mask_bf->n_mask);
    wtk_free(mask_bf->p_mask);
    wtk_float_delete_p2(mask_bf->power, channel);
    wtk_free(mask_bf->s_power);
    wtk_free(mask_bf->n_power);
    wtk_free(mask_bf->eta);
    wtk_free(mask_bf->epsi);
    wtk_free(mask_bf->update_speech);
    wtk_free(mask_bf->update_noise);
    wtk_free(mask_bf->entropy_E);
    wtk_free(mask_bf->entropy_Eb);
    wtk_free(mask_bf->fft_tmp);
    wtk_complex_delete_p2(mask_bf->covar, nbin);
    wtk_complex_delete_p2(mask_bf->Rss, nbin);
    wtk_complex_delete_p2(mask_bf->Rnn, nbin);
    wtk_complex_delete_p2(mask_bf->Rss_e, nbin);
    wtk_complex_delete_p2(mask_bf->Rnn_e, nbin);
    wtk_complex_delete_p2(mask_bf->Rss_norm, nbin);
    wtk_complex_delete_p2(mask_bf->Rnn_norm, nbin);
    wtk_free(mask_bf->Rss_);
    wtk_free(mask_bf->Rnn_);
    wtk_complex_delete_p2(mask_bf->vec, nbin);
    wtk_complex_delete_p2(mask_bf->w, nbin);
    wtk_free(mask_bf->c_temp);
    wtk_free(mask_bf->fftx);
    wtk_free(mask_bf->Rss_cnt);
    wtk_free(mask_bf->Rnn_cnt);
    wtk_free(mask_bf->Rss_cnt_e);
    wtk_free(mask_bf->Rnn_cnt_e);
    wtk_free(mask_bf->scnt);
    wtk_free(mask_bf->ncnt);

    wtk_free(mask_bf->s_power_acc);
    wtk_free(mask_bf->n_power_acc);
    wtk_free(mask_bf->s_power_acc_e);
    wtk_free(mask_bf->n_power_acc_e);
    wtk_free(mask_bf->speech_power);
    wtk_free(mask_bf->noise_power);
    wtk_free(mask_bf->snr);

    wtk_free(mask_bf);
}

void wtk_mask_bf_start(wtk_mask_bf_t *mask_bf) {}

void wtk_mask_bf_set_notify(wtk_mask_bf_t *mask_bf, void *ths, wtk_mask_bf_notify_f notify) {
    mask_bf->notify = notify;
    mask_bf->ths = ths;
}

float wtk_mask_bf_entropy(wtk_mask_bf_t *mask_bf, wtk_complex_t *fftx) {
    int wins = mask_bf->cfg->wins;
    int nbin = mask_bf->cfg->nbin;
    int i;
    int km = floor(wins * 1.0 / 8);
    float K = 0.5;
    float *E = mask_bf->entropy_E;
    float P1;
    float *Eb = mask_bf->entropy_Eb;
    float sum;
    float prob;
    float Hb;
    int fx1 = mask_bf->cfg->en_low_bin;
    int fx2 = mask_bf->cfg->en_high_bin;

    memset(E, 0, sizeof(float) * nbin);
    memset(Eb, 0, sizeof(float) * wins);
    for (i = fx1; i < fx2; ++i) {
        E[i] = fftx[i].a * fftx[i].a + fftx[i].b * fftx[i].b;
    }
    sum = 1e-10;
    for (i = fx1; i < fx2; ++i) {
        sum += E[i];
    }
    sum = 1.0 / sum;
    for (i = fx1; i < fx2; ++i) {
        P1 = E[i] * sum;
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

void wtk_mask_bf_get_ref_channel_snr(wtk_mask_bf_t *mask_bf, int sp_sil) {
    float *s_power_acc = mask_bf->s_power_acc;
    float *n_power_acc = mask_bf->n_power_acc;
    float *s_power_acc_e = mask_bf->s_power_acc_e;
    float *n_power_acc_e = mask_bf->n_power_acc_e;
    float *speech_power = mask_bf->speech_power;
    float *noise_power = mask_bf->noise_power;
    float *snr = mask_bf->snr;
    int channel = mask_bf->cfg->channel;
    float snr_thresh = mask_bf->cfg->snr_thresh;
    float snr_ref;
    int ref_channel_new;
    float snr_new;
    float max_snr;
    int ref_channel = mask_bf->ref_channel;
    int i;
    float tmp_s, tmp_n;

    if (sp_sil) {
        tmp_s = 1.0 / (mask_bf->s_power_cnt + 1e-3);
        tmp_n = 1.0 / (mask_bf->n_power_cnt + 1e-3);
        for (i = 0; i < channel; ++i) {
            speech_power[i] = s_power_acc[i] * tmp_s;
            noise_power[i] = n_power_acc[i] * tmp_n;
            snr[i] = speech_power[i] / (noise_power[i] + 1e-9);
        }
    } else {
        tmp_s = 1.0 / (mask_bf->s_power_cnt_e + 1e-3);
        tmp_n = 1.0 / (mask_bf->n_power_cnt_e + 1e-3);
        for (i = 0; i < channel; ++i) {
            speech_power[i] = s_power_acc_e[i] * tmp_s;
            noise_power[i] = n_power_acc_e[i] * tmp_n;
            snr[i] = speech_power[i] / (noise_power[i] + 1e-9);
        }
    }

    ref_channel_new = 0;
    max_snr = snr[0];
    for (i = 1; i < channel; ++i) {
        if (snr[i] > max_snr) {
            max_snr = snr[i];
            ref_channel_new = i;
        }
    }
    snr_ref = snr[ref_channel];
    if (ref_channel_new != ref_channel) {
        snr_new = snr[ref_channel_new];
        if (snr_new > snr_ref * snr_thresh) {
            ref_channel = ref_channel_new;
            snr_ref = snr_new;
        }
    }
    if (mask_bf->cfg->use_ref_change) {
        mask_bf->ref_channel = ref_channel;
    }
    snr_ref = min(snr_ref, 50);
    if (sp_sil) {
        mask_bf->snr_acc = mask_bf->snr_acc * 0.95 + snr_ref * 0.05;
    } else {
        mask_bf->snr_acc_e = mask_bf->snr_acc_e * 0.95 + snr_ref * 0.05;
    }
}

void wtk_mask_bf_update(wtk_mask_bf_t *mask_bf, wtk_complex_t **fft, int sp_sil) {
    int channel = mask_bf->cfg->channel;
    int low_bin = mask_bf->cfg->low_bin;
    int high_bin = mask_bf->cfg->high_bin;
    int nbin = mask_bf->cfg->nbin;
    float *s_mask = mask_bf->s_mask;
    float *n_mask = mask_bf->n_mask;
    float *power;
    float *s_power = mask_bf->s_power;
    float *n_power = mask_bf->n_power;
    float *eta = mask_bf->eta;
    float *epsi = mask_bf->epsi;
    int *update_speech = mask_bf->update_speech;
    int *update_noise = mask_bf->update_noise;
    float *s_power_acc = mask_bf->s_power_acc;
    float *n_power_acc = mask_bf->n_power_acc;
    float *s_power_acc_e = mask_bf->s_power_acc_e;
    float *n_power_acc_e = mask_bf->n_power_acc_e;
    wtk_complex_t *fft_tmp = mask_bf->fft_tmp;
    wtk_complex_t *fft_n;
    float zeta = mask_bf->cfg->zeta;
    float s_power_thresh = mask_bf->cfg->s_power_thresh;
    float n_power_thresh = mask_bf->cfg->n_power_thresh;
    float *snr_acc_tuple = mask_bf->cfg->snr_acc_tuple;
    float *scov_alpha_tuple = mask_bf->cfg->scov_alpha_tuple;
    float *s_frame_thresh_tuple = mask_bf->cfg->s_frame_thresh_tuple;
    float *s_bin_thresh_tuple = mask_bf->cfg->s_bin_thresh_tuple;
    float *n_bin_thresh_tuple = mask_bf->cfg->n_bin_thresh_tuple;
    float *En_thresh_tuple = mask_bf->cfg->En_thresh_tuple;
    int snr_acc_tuple_n = mask_bf->cfg->snr_acc_tuple_n;
    float entropy;
    float s_mean, n_mean;
    float zeta_frame;
    float s_frame_thresh;
    float s_bin_thresh;
    float n_bin_thresh;
    float En_thresh;
    int i, k;

    // 计算当前帧语音和噪声功率，用于后续信噪比估计
    for (i = 0; i < channel; ++i) {
        fft_n = fft[i];
        power = mask_bf->power[i];
        s_power[i] = 0;
        n_power[i] = 0;
        for (k = low_bin; k < high_bin; ++k) {
            power[k] = fft_n[k].a * fft_n[k].a + fft_n[k].b * fft_n[k].b;
            s_power[i] += power[k] * s_mask[k];
            n_power[i] += power[k] * n_mask[k];
        }
    }

    // 计算基于mask的相关判决参数
    s_mean = 0;
    n_mean = 0;
    for (k = low_bin; k < high_bin; ++k) {
        s_mean += s_mask[k];
        n_mean += n_mask[k];
    }
    s_mean = s_mean / (high_bin - low_bin);
    n_mean = n_mean / (high_bin - low_bin);

    for (k = 0; k < nbin; ++k) {
        eta[k] = powf(s_mask[k], channel);
        epsi[k] = powf(n_mask[k], channel);
    }

    fft_n = fft[0];
    for (k = 0; k < nbin; ++k) {
        fft_tmp[k].a = fft_n[k].a * s_mask[k];
        fft_tmp[k].b = fft_n[k].b * s_mask[k];
    }
    entropy = wtk_mask_bf_entropy(mask_bf, fft_tmp);

    for (i = 1; i < channel; ++i) {
        fft_n = fft[i];
        for (k = 0; k < nbin; ++k) {
            fft_tmp[k].a = fft_n[k].a * s_mask[k];
            fft_tmp[k].b = fft_n[k].b * s_mask[k];
        }
        entropy = min(wtk_mask_bf_entropy(mask_bf, fft_tmp), entropy);
    }

    // 平滑信噪比估计参数，获取相关判决阈值
    if (sp_sil) {
        if (mask_bf->s_power_cnt > s_power_thresh) {
            zeta_frame = zeta;
        } else {
            zeta_frame = 1.0;
        }
        for (i = 0; i < channel; ++i) {
            s_power_acc[i] *= zeta_frame;
        }
        mask_bf->s_power_cnt *= zeta_frame;

        if (mask_bf->n_power_cnt > n_power_thresh) {
            zeta_frame = zeta;
        } else {
            zeta_frame = 1.0;
        }
        for (i = 0; i < channel; ++i) {
            n_power_acc[i] *= zeta_frame;
        }
        mask_bf->n_power_cnt *= zeta_frame;

        if (snr_acc_tuple_n > 0) {
            int idx = snr_acc_tuple_n;
            for (i = 0; i < snr_acc_tuple_n; ++i) {
                if (mask_bf->snr_acc <= snr_acc_tuple[i]) {
                    idx = i;
                    break;
                }
            }
            mask_bf->scov_alpha = scov_alpha_tuple[idx];
            s_frame_thresh = s_frame_thresh_tuple[idx];
            s_bin_thresh = s_bin_thresh_tuple[idx];
            n_bin_thresh = n_bin_thresh_tuple[idx];
            En_thresh = En_thresh_tuple[idx];
        } else {
            mask_bf->scov_alpha = mask_bf->cfg->scov_alpha;
            s_frame_thresh = mask_bf->cfg->s_frame_thresh;
            s_bin_thresh = mask_bf->cfg->s_bin_thresh;
            n_bin_thresh = mask_bf->cfg->n_bin_thresh;
            En_thresh = mask_bf->cfg->En_thresh;
        }
    } else {
        if (mask_bf->s_power_cnt_e > s_power_thresh) {
            zeta_frame = zeta;
        } else {
            zeta_frame = 1.0;
        }
        for (i = 0; i < channel; ++i) {
            s_power_acc_e[i] *= zeta_frame;
        }
        mask_bf->s_power_cnt_e *= zeta_frame;

        if (mask_bf->n_power_cnt_e > n_power_thresh) {
            zeta_frame = zeta;
        } else {
            zeta_frame = 1.0;
        }
        for (i = 0; i < channel; ++i) {
            n_power_acc_e[i] *= zeta_frame;
        }
        mask_bf->n_power_cnt_e *= zeta_frame;

        mask_bf->scov_alpha = mask_bf->cfg->scov_alpha_e;
        s_frame_thresh = mask_bf->cfg->s_frame_thresh_e;
        s_bin_thresh = mask_bf->cfg->s_bin_thresh_e;
        n_bin_thresh = mask_bf->cfg->n_bin_thresh_e;
        En_thresh = mask_bf->cfg->En_thresh_e;
    }

    // 更新信噪比估计
    if (s_mean > s_frame_thresh && entropy < En_thresh) {
        wtk_mask_bf_get_ref_channel_snr(mask_bf, sp_sil);
    }

    // 对协方差矩阵是否更新进行频点级别的判断
    memset(update_speech, 0, sizeof(int) * nbin);
    memset(update_noise, 0, sizeof(int) * nbin);

    if (s_mean > s_frame_thresh && entropy < En_thresh) {
        for (k = 0; k < nbin; ++k) {
            if (s_mask[k] > s_bin_thresh) {
                update_speech[k] = 1;
            }
        }
    } else if ((s_mean <= s_frame_thresh && entropy < En_thresh) ||
               (s_mean > s_frame_thresh && entropy >= En_thresh)) {
        for (k = 0; k < nbin; ++k) {
            if (n_mask[k] > n_bin_thresh) {
                update_noise[k] = 1;
            }
        }
    } else {
        for (k = 0; k < nbin; ++k) {
            update_noise[k] = 1;
        }
    }

    // 更新语音和噪声功率
    if (sp_sil) {
        if (wtk_int_sum(update_speech, nbin) > 0) {
            for (i = 0; i < channel; ++i) {
                s_power_acc[i] += s_power[i] * s_mean;
            }
            mask_bf->s_power_cnt += s_mean;
        }
        if (wtk_int_sum(update_noise, nbin) > 0) {
            for (i = 0; i < channel; ++i) {
                n_power_acc[i] += n_power[i] * n_mean;
            }
            mask_bf->n_power_cnt += n_mean;
        }
    } else {
        if (wtk_int_sum(update_speech, nbin) > 0) {
            for (i = 0; i < channel; ++i) {
                s_power_acc_e[i] += s_power[i] * s_mean;
            }
            mask_bf->s_power_cnt_e += s_mean;
        }
        if (wtk_int_sum(update_noise, nbin) > 0) {
            for (i = 0; i < channel; ++i) {
                n_power_acc_e[i] += n_power[i] * n_mean;
            }
            mask_bf->n_power_cnt_e += n_mean;
        }
    }
}

void wtk_mask_bf_feed_covm(wtk_mask_bf_t *mask_bf, wtk_complex_t **fft, int sp_sil) {
    wtk_complex_t *covar;
    wtk_complex_t **Rss;
    wtk_complex_t **Rnn;
    wtk_complex_t *Rss_norm;
    wtk_complex_t *Rnn_norm;
    wtk_complex_t *Rss_tmp;
    wtk_complex_t *Rnn_tmp;
    float eta;
    float epsi;
    float *Rss_cnt;
    float *Rnn_cnt;
    int *scnt = mask_bf->scnt;
    int *ncnt = mask_bf->ncnt;
    int *update_speech = mask_bf->update_speech;
    int *update_noise = mask_bf->update_noise;
    int channel = mask_bf->cfg->channel;
    int channelx2 = mask_bf->cfg->channelx2;
    int nbin = mask_bf->cfg->nbin;
    float gamma = mask_bf->cfg->gamma;
    float scov_alpha;
    float scov_alpha_1;
    float ncov_alpha;
    float ncov_alpha_1;
    int i, j, k;
    int idx;
    float tmp_s, tmp_n;

    for (k = 0; k < nbin; ++k) {
        covar = mask_bf->covar[k];
        memset(covar, 0, sizeof(wtk_complex_t) * channelx2);
        for (i = 0; i < channel; ++i) {
            for (j = 0; j < channel; ++j) {
                idx = i * channel + j;
                covar[idx].a =
                    fft[i][k].a * fft[j][k].a + fft[i][k].b * fft[j][k].b;
                covar[idx].b =
                    -fft[i][k].a * fft[j][k].b + fft[i][k].b * fft[j][k].a;
            }
        }
    }

    if (sp_sil) {
        ncov_alpha = mask_bf->cfg->ncov_alpha;
        Rss = mask_bf->Rss;
        Rnn = mask_bf->Rnn;
        Rss_cnt = mask_bf->Rss_cnt;
        Rnn_cnt = mask_bf->Rnn_cnt;
    } else {
        ncov_alpha = mask_bf->cfg->ncov_alpha_e;
        Rss = mask_bf->Rss_e;
        Rnn = mask_bf->Rnn_e;
        Rss_cnt = mask_bf->Rss_cnt_e;
        Rnn_cnt = mask_bf->Rnn_cnt_e;
    }
    scov_alpha = mask_bf->scov_alpha;
    scov_alpha_1 = 1.0 - scov_alpha;
    ncov_alpha_1 = 1.0 - ncov_alpha;

    for (k = 0; k < nbin; ++k) {
        covar = mask_bf->covar[k];
        Rss_tmp = Rss[k];
        Rnn_tmp = Rnn[k];
        Rss_norm = mask_bf->Rss_norm[k];
        Rnn_norm = mask_bf->Rnn_norm[k];
        eta = mask_bf->eta[k];
        epsi = mask_bf->epsi[k];
        if (update_speech[k]) {
            scnt[k]++;
            if (scnt[k] >= 100) {
                eta *= scov_alpha;
                for (i = 0; i < channel; ++i) {
                    for (j = 0; j < channel; ++j) {
                        idx = i * channel + j;
                        Rss_tmp[idx].a =
                            Rss_tmp[idx].a * scov_alpha_1 + covar[idx].a * eta;
                        Rss_tmp[idx].b =
                            Rss_tmp[idx].b * scov_alpha_1 + covar[idx].b * eta;
                    }
                }
                Rss_cnt[k] = scov_alpha_1 * Rss_cnt[k] + eta;
            } else {
                for (i = 0; i < channel; ++i) {
                    for (j = 0; j < channel; ++j) {
                        idx = i * channel + j;
                        Rss_tmp[idx].a = Rss_tmp[idx].a + covar[idx].a * eta;
                        Rss_tmp[idx].b = Rss_tmp[idx].b + covar[idx].b * eta;
                    }
                }
                Rss_cnt[k] = Rss_cnt[k] + eta;
            }
        }
        if (update_noise[k]) {
            ncnt[k]++;
            if (ncnt[k] >= 100) {
                epsi *= ncov_alpha;
                for (i = 0; i < channel; ++i) {
                    for (j = 0; j < channel; ++j) {
                        idx = i * channel + j;
                        Rnn_tmp[idx].a =
                            Rnn_tmp[idx].a * ncov_alpha_1 + covar[idx].a * epsi;
                        Rnn_tmp[idx].b =
                            Rnn_tmp[idx].b * ncov_alpha_1 + covar[idx].b * epsi;
                    }
                }
                Rnn_cnt[k] = ncov_alpha_1 * Rnn_cnt[k] + epsi;
            } else {
                for (i = 0; i < channel; ++i) {
                    for (j = 0; j < channel; ++j) {
                        idx = i * channel + j;
                        Rnn_tmp[idx].a = Rnn_tmp[idx].a + covar[idx].a * epsi;
                        Rnn_tmp[idx].b = Rnn_tmp[idx].b + covar[idx].b * epsi;
                    }
                }
                Rnn_cnt[k] = Rnn_cnt[k] + epsi;
            }
        }
        tmp_s = 1.0 / (Rss_cnt[k] + 1e-10);
        tmp_n = 1.0 / (Rnn_cnt[k] + 1e-10);
        for (i = 0; i < channel; ++i) {
            for (j = 0; j < channel; ++j) {
                idx = i * channel + j;
                Rss_norm[idx].a = Rss_tmp[idx].a * tmp_s;
                Rss_norm[idx].b = Rss_tmp[idx].b * tmp_s;
                Rnn_norm[idx].a = Rnn_tmp[idx].a * tmp_n;
                Rnn_norm[idx].b = Rnn_tmp[idx].b * tmp_n;
            }
        }
    }

    if (gamma > 0.0) {
        float Rss_p;
        float Rnn_p;
        float scaler;
        for (k = 0; k < nbin; ++k) {
            Rss_norm = mask_bf->Rss_norm[k];
            Rnn_norm = mask_bf->Rnn_norm[k];
            Rss_p = Rnn_p = 0;
            for (i = 0; i < channel; ++i) {
                idx = i * channel + i;
                Rss_p += Rss_norm[idx].a * Rss_norm[idx].a;
                Rnn_p += Rnn_norm[idx].a * Rnn_norm[idx].a;
            }
            Rss_p = sqrtf(Rss_p);
            Rnn_p = sqrtf(Rnn_p);
            scaler = min(max(Rss_p / (gamma * Rnn_p + 1e-10), 0.0), 1.0);
            for (i = 0; i < channel; ++i) {
                for (j = 0; j < channel; ++j) {
                    idx = i * channel + j;
                    Rss_norm[idx].a =
                        Rss_norm[idx].a - Rnn_norm[idx].a * scaler;
                    Rss_norm[idx].b =
                        Rss_norm[idx].b - Rnn_norm[idx].b * scaler;
                }
            }
        }
    }
}

void wtk_mask_bf_update_mvdr_w_f(wtk_mask_bf_t *mask_bf, int k) {
    int channel = mask_bf->cfg->channel;
    int channelx2 = mask_bf->cfg->channelx2;
    float eye = mask_bf->cfg->eye;
    int i;
    float fa, f;
    wtk_complex_t *w = mask_bf->w[k];
    wtk_complex_t *fft_tmp = mask_bf->fft_tmp;
    wtk_complex_t *a;
    wtk_complex_t *Rnn_norm, *vec;
    wtk_complex_t *Rnn_ = mask_bf->Rnn_;

    Rnn_norm = mask_bf->Rnn_norm[k];
    vec = mask_bf->vec[k];

    memcpy(Rnn_, Rnn_norm, sizeof(wtk_complex_t) * channelx2);
    for (i = 0; i < channel; ++i) {
        Rnn_[i * channel + i].a += eye;
    }
    // int j, idx, idx2;
    // for (i = 0; i < channel; ++i) {
    //     for (j = i; j < channel; ++j) {
    //         idx = i * channel + j;
    //         idx2 = j * channel + i;
    //         if (i != j) {
    //             Rnn_[idx].a = 0.5 * (Rnn_norm[idx].a + Rnn_norm[idx2].a);
    //             Rnn_[idx].b = 0.5 * (Rnn_norm[idx].b - Rnn_norm[idx2].b);
    //             Rnn_[idx2].a = Rnn_[idx].a;
    //             Rnn_[idx2].b = -Rnn_[idx].b;
    //         }
    //     }
    // }

    wtk_complex_guass_elimination_p1_f(Rnn_, vec, fft_tmp, channel, w);

    fa = 0;
    a = w;
    for (i = 0; i < channel; ++i, ++vec, ++a) {
        fa += vec->a * a->a + vec->b * a->b;
    }
    f = 1.0 / fa;
    a = w;
    for (i = 0; i < channel; ++i, ++a) {
        a->a *= f;
        a->b *= f;
    }
}

void wtk_mask_bf_output_fft_k(wtk_mask_bf_t *mask_bf, wtk_complex_t *fft,
                          wtk_complex_t *out, int k) {
    wtk_complex_t *w = mask_bf->w[k];
    int i;
    int channel = mask_bf->cfg->channel;
    float ta, tb;

    ta = tb = 0;
    for (i = 0; i < channel; ++i, ++fft, ++w) {
        ta += w->a * fft->a + w->b * fft->b;
        tb += w->a * fft->b - w->b * fft->a;
    }
    out->a = ta;
    out->b = tb;
}

void wtk_mask_bf_feed_bf(wtk_mask_bf_t *mask_bf, wtk_complex_t **fft) {
    int channel = mask_bf->cfg->channel;
    int rss_iter = mask_bf->cfg->rss_iter;
    int update_w_cnt = mask_bf->cfg->update_w_cnt;
    int *update_w_freq = mask_bf->cfg->update_w_freq;
    int ref_channel = mask_bf->ref_channel;
    int nbin = mask_bf->cfg->nbin;
    int i, j, k, n;
    wtk_complex_t *vec;
    wtk_complex_t *Rss_norm;
    wtk_complex_t *Rss_;
    wtk_complex_t *c_temp = mask_bf->c_temp;
    wtk_complex_t *fftx = mask_bf->fftx;
    wtk_complex_t fft2[64];
    float *p_mask = mask_bf->p_mask;
    float eps = 1e-16;
    float t;
    float c_norm;
    int idx;

    ++mask_bf->update_w;
    if (mask_bf->nframe == 1) {
        for (k = 0; k < nbin; ++k) {
            vec = mask_bf->vec[k];
            memset(vec, 0, sizeof(wtk_complex_t) * channel);
            vec[ref_channel].a = 1.0;
        }
    } else {
        for (k = 0; k < nbin; ++k) {
            vec = mask_bf->vec[k];
            Rss_norm = mask_bf->Rss_norm[k];
            Rss_ = mask_bf->Rss_;
            t = 0;
            for (i = 0; i < channel; ++i) {
                idx = i * channel + i;
                t += Rss_norm[idx].a * Rss_norm[idx].a;
            }
            t = sqrtf(t);
            t = 1.0 / (t + eps);
            for (i = 0; i < channel; ++i) {
                for (j = 0; j < channel; ++j) {
                    idx = i * channel + j;
                    Rss_[idx].a = Rss_norm[idx].a * t;
                    Rss_[idx].b = Rss_norm[idx].b * t;
                }
            }
            for (n = 0; n < rss_iter; ++n) {
                memset(c_temp, 0, sizeof(wtk_complex_t) * channel);
                for (i = 0; i < channel; ++i) {
                    for (j = 0; j < channel; ++j) {
                        idx = i * channel + j;
                        c_temp[i].a +=
                            Rss_[idx].a * vec[j].a - Rss_[idx].b * vec[j].b;
                        c_temp[i].b +=
                            Rss_[idx].a * vec[j].b + Rss_[idx].b * vec[j].a;
                    }
                }
                memcpy(vec, c_temp, sizeof(wtk_complex_t) * channel);
            }
            c_norm = 0;
            for (i = 0; i < channel; ++i) {
                c_norm += vec[i].a * vec[i].a + vec[i].b * vec[i].b;
            }
            c_norm = sqrtf(c_norm);
            if (c_norm < eps) {
                memset(vec, 0, sizeof(wtk_complex_t) * channel);
                vec[ref_channel].a = 1.0;
            } else {
                c_norm = 1.0 / (c_norm + eps);
                for (i = 0; i < channel; ++i) {
                    vec[i].a *= c_norm;
                    vec[i].b *= c_norm;
                }
            }

            float ref_real = vec[ref_channel].a;
            float ref_imag = vec[ref_channel].b;
            t = 0;
            for (i = 0; i < channel; ++i) {
                vec[i].a = vec[i].a * ref_real + vec[i].b * ref_imag;
                
                // vec[i].b = vec[i].b * ref_real - vec[i].a * ref_imag;
                if (ref_channel != i) {
                    vec[i].b = vec[i].b * ref_real - vec[i].a * ref_imag;
                } else {
                    vec[i].b = 0.0;
                }

                t += vec[i].a * vec[i].a + vec[i].b * vec[i].b;
            }
            t = 1.0 / (sqrtf(t) + eps);
            for (i = 0; i < channel; ++i) {
                vec[i].a *= t;
                vec[i].b *= t;
            }
        }
    }

    for (k = 0; k < nbin; ++k) {
        for (i = 0; i < channel; ++i) {
            fft2[i].a = fft[i][k].a;
            fft2[i].b = fft[i][k].b;
        }
        if (mask_bf->update_w == update_w_freq[k]) {
            wtk_mask_bf_update_mvdr_w_f(mask_bf, k);
        }
        wtk_mask_bf_output_fft_k(mask_bf, fft2, fftx + k, k);
    }
    for (k = 0; k < nbin; ++k) {
        fftx[k].a *= p_mask[k];
        fftx[k].b *= p_mask[k];
    }
    if (mask_bf->update_w % update_w_cnt == 0) {
        mask_bf->update_w = 0;
    }
}

/*
 * 输入语音信号和mask，做beamforming
 * 输入：
 *   fft: 多通道频域数据
 *   speech_mask: 语音mask
 *   noise_mask: 噪声mask，可缺省
 *   post_mask: 后处理mask，可缺省
 *   sp_sil: 是否存在回声，1为不存在，0为存在，回声和非回声处理不同
 */
void wtk_mask_bf_feed(wtk_mask_bf_t *mask_bf, wtk_complex_t **fft, float *speech_mask,
                  float *noise_mask, float *post_mask, int sp_sil) {
    float post_clip;
    float init_post_clip = mask_bf->cfg->init_post_clip;
    int init_cnt = mask_bf->cfg->init_cnt;
    float *s_mask = mask_bf->s_mask;
    float *n_mask = mask_bf->n_mask;
    float *p_mask = mask_bf->p_mask;
    wtk_complex_t *fftx = mask_bf->fftx;
    int nbin = mask_bf->cfg->nbin;
    int k;

    ++mask_bf->nframe;
    memcpy(s_mask, speech_mask, sizeof(float) * nbin);
    if (noise_mask == NULL) {
        for (k = 0; k < nbin; ++k) {
            n_mask[k] = 1.0 - s_mask[k];
        }
    } else {
        memcpy(n_mask, noise_mask, sizeof(float) * nbin);
    }
    if (post_mask == NULL) {
        memcpy(p_mask, s_mask, sizeof(float) * nbin);
    } else {
        memcpy(p_mask, post_mask, sizeof(float) * nbin);
    }
    if (sp_sil) {
        post_clip = mask_bf->cfg->post_clip;
    } else {
        post_clip = mask_bf->cfg->post_clip_e;
    }
    if (mask_bf->nframe < init_cnt) {
        post_clip = init_post_clip;
    }
    for (k = 0; k < nbin; ++k) {
        p_mask[k] = min(max(post_clip, p_mask[k]), 1.0);
    }

    wtk_mask_bf_update(mask_bf, fft, sp_sil);
    wtk_mask_bf_feed_covm(mask_bf, fft, sp_sil);
    wtk_mask_bf_feed_bf(mask_bf, fft);

    if (mask_bf->notify) {
        mask_bf->notify(mask_bf->ths, fftx);
    }
}
