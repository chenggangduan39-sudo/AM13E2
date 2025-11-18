#include "wtk_rtjoin3.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/core/wtk_strbuf.h"

wtk_rtjoin3_t *wtk_rtjoin3_new(wtk_rtjoin3_cfg_t *cfg) {
    wtk_rtjoin3_t *rtjoin3;
    int i;

    rtjoin3 = (wtk_rtjoin3_t *)wtk_malloc(sizeof(wtk_rtjoin3_t));
    rtjoin3->cfg = cfg;
    rtjoin3->ths = NULL;
    rtjoin3->notify = NULL;

    rtjoin3->mic = wtk_strbufs_new(rtjoin3->cfg->nmicchannel);

    rtjoin3->analysis_window = wtk_malloc(sizeof(float) * cfg->wins);  /// 2);
    rtjoin3->synthesis_window = wtk_malloc(sizeof(float) * cfg->wins); /// 2);
    rtjoin3->analysis_mem = wtk_float_new_p2(cfg->nmicchannel, cfg->nbin - 1);
    rtjoin3->synthesis_mem = wtk_malloc(sizeof(float) * (cfg->nbin - 1));
    rtjoin3->mul_synthesis_mem = NULL;
    if (cfg->use_mul_out) {
        rtjoin3->mul_synthesis_mem =
            wtk_float_new_p2(cfg->nmicchannel, cfg->nbin - 1);
    }
    rtjoin3->rfft = wtk_drft_new(cfg->wins);
    rtjoin3->rfft_in = (float *)wtk_malloc(sizeof(float) * (cfg->wins));

    rtjoin3->fft = wtk_complex_new_p2(cfg->nmicchannel, cfg->nbin);

    rtjoin3->fftx =
        (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * cfg->nbin);

    rtjoin3->eq = NULL;
    if (cfg->use_eq) {
        rtjoin3->eq = wtk_equalizer_new(&(cfg->eq));
    }

    rtjoin3->mul_out = NULL;
    if (cfg->use_mul_out) {
        rtjoin3->mul_out = wtk_malloc(sizeof(float) * (cfg->nbin - 1));
        rtjoin3->out =
            wtk_malloc(sizeof(float) * (cfg->nbin - 1) * cfg->nmicchannel);
    } else {
        rtjoin3->out = wtk_malloc(sizeof(float) * (cfg->nbin - 1));
    }

    rtjoin3->qmmse = NULL;
    if (cfg->use_qmmse) {
        rtjoin3->qmmse = wtk_qmmse_new(&(cfg->qmmse));
    }

    rtjoin3->bs_win = NULL;
    if (cfg->use_bs_win) {
        rtjoin3->bs_win = wtk_math_create_hanning_window2(cfg->wins / 2);
    }

    wtk_rtjoin3_reset(rtjoin3);

    return rtjoin3;
}

void wtk_rtjoin3_delete(wtk_rtjoin3_t *rtjoin3) {
    int i;
    wtk_strbufs_delete(rtjoin3->mic, rtjoin3->cfg->nmicchannel);

    wtk_free(rtjoin3->analysis_window);
    wtk_free(rtjoin3->synthesis_window);
    wtk_float_delete_p2(rtjoin3->analysis_mem, rtjoin3->cfg->nmicchannel);
    wtk_free(rtjoin3->synthesis_mem);
    if (rtjoin3->mul_synthesis_mem) {
        wtk_float_delete_p2(rtjoin3->mul_synthesis_mem,
                            rtjoin3->cfg->nmicchannel);
    }
    wtk_free(rtjoin3->rfft_in);
    wtk_drft_delete(rtjoin3->rfft);
    wtk_complex_delete_p2(rtjoin3->fft, rtjoin3->cfg->nmicchannel);

    if (rtjoin3->eq) {
        wtk_equalizer_delete(rtjoin3->eq);
    }

    wtk_free(rtjoin3->fftx);
    if (rtjoin3->mul_out) {
        wtk_free(rtjoin3->mul_out);
    }
    wtk_free(rtjoin3->out);

    if (rtjoin3->qmmse) {
        wtk_qmmse_delete(rtjoin3->qmmse);
    }
    if (rtjoin3->bs_win) {
        wtk_free(rtjoin3->bs_win);
    }

    wtk_free(rtjoin3);
}

void wtk_rtjoin3_start(wtk_rtjoin3_t *rtjoin3) {}

void wtk_rtjoin3_reset(wtk_rtjoin3_t *rtjoin3) {
    int wins = rtjoin3->cfg->wins;
    int nbin = rtjoin3->cfg->nbin;
    int i;

    wtk_strbufs_reset(rtjoin3->mic, rtjoin3->cfg->nmicchannel);
    for (i = 0; i < wins; ++i) {
        rtjoin3->analysis_window[i] = sin((0.5 + i) * PI / (wins));
    }
    wtk_drft_init_synthesis_window(rtjoin3->synthesis_window,
                                   rtjoin3->analysis_window, wins);

    wtk_float_zero_p2(rtjoin3->analysis_mem, rtjoin3->cfg->nmicchannel,
                      (nbin - 1));
    memset(rtjoin3->synthesis_mem, 0, sizeof(float) * (nbin - 1));
    if (rtjoin3->mul_synthesis_mem) {
        wtk_float_zero_p2(rtjoin3->mul_synthesis_mem, rtjoin3->cfg->nmicchannel,
                          (nbin - 1));
    }

    wtk_complex_zero_p2(rtjoin3->fft, rtjoin3->cfg->nmicchannel, nbin);
    memset(rtjoin3->fftx, 0, sizeof(wtk_complex_t) * (nbin));

    if (rtjoin3->mul_out) {
        memset(rtjoin3->mul_out, 0, sizeof(float) * (nbin - 1));
    }
    memset(rtjoin3->out, 0, sizeof(float) * (nbin - 1));

    if (rtjoin3->qmmse) {
        wtk_qmmse_reset(rtjoin3->qmmse);
    }

    rtjoin3->sp_silcnt = 0;
    rtjoin3->mic_silcnt = 0;
    rtjoin3->sp_sil = 1;
    rtjoin3->mic_sil = 1;

    rtjoin3->bs_scale = 1.0;
    rtjoin3->bs_last_scale = 1.0;
    rtjoin3->bs_real_scale = 1.0;
    rtjoin3->bs_max_cnt = 0;
}

void wtk_rtjoin3_set_notify(wtk_rtjoin3_t *rtjoin3, void *ths,
                            wtk_rtjoin3_notify_f notify) {
    rtjoin3->notify = notify;
    rtjoin3->ths = ths;
}

static void _limiter(wtk_rtjoin3_t *rtjoin3, float *out, int fsize) {
    int max_out = rtjoin3->cfg->max_out;
    float fv = max_out / 32768.0;
    float thresh = rtjoin3->cfg->limiter_thresh;
    float alpha = fv * thresh;
    float alpha_1 = fv * (1 - thresh);
    int i;

    for (i = 0; i < fsize; i++) {
        double x = out[i] / 32768.0;
        if (-alpha <= x && x <= alpha) {
            // 直通区间，无需修改
            continue;
        } else if (x > alpha) {
            // 正向压缩
            out[i] = alpha + alpha_1 * (1 - exp(-(x - alpha)));
        } else {
            // 负向压缩
            out[i] = -alpha - alpha_1 * (1 - exp(-(-x - alpha)));
        }
        out[i] *= 32768.0;
    }
}

void wtk_rtjoin3_control_bs(wtk_rtjoin3_t *rtjoin3, float *out, int len) {
    float *bs_win = rtjoin3->bs_win;
    float max_out = rtjoin3->cfg->max_out;
    float out_max;
    int i;

    out_max = wtk_float_abs_max(out, len);
    if (out_max > max_out) {
        rtjoin3->bs_scale = max_out / out_max;
        if (rtjoin3->bs_scale < rtjoin3->bs_last_scale) {
            rtjoin3->bs_last_scale = rtjoin3->bs_scale;
        } else {
            rtjoin3->bs_scale = rtjoin3->bs_last_scale;
        }
        rtjoin3->bs_max_cnt = 5;
    }
    if (bs_win) {
        for (i = 0; i < len / 2; ++i) {
            out[i] *= rtjoin3->bs_scale * bs_win[i] +
                      rtjoin3->bs_real_scale * (1.0 - bs_win[i]);
        }
        for (i = len / 2; i < len; ++i) {
            out[i] *= rtjoin3->bs_scale;
        }
        rtjoin3->bs_real_scale = rtjoin3->bs_scale;
    } else {
        for (i = 0; i < len; ++i) {
            out[i] *= rtjoin3->bs_scale;
        }
    }
    if (rtjoin3->bs_max_cnt > 0) {
        --rtjoin3->bs_max_cnt;
    }
    if (rtjoin3->bs_max_cnt <= 0 && rtjoin3->bs_scale < 1.0) {
        rtjoin3->bs_scale *= 1.1f;
        rtjoin3->bs_last_scale = rtjoin3->bs_scale;
        if (rtjoin3->bs_scale > 1.0) {
            rtjoin3->bs_scale = 1.0;
            rtjoin3->bs_last_scale = 1.0;
        }
    }
}

void wtk_rtjoin3_feed_mix(wtk_rtjoin3_t *rtjoin3, wtk_complex_t **fft,
                          wtk_complex_t *fftx) {
    int nbin = rtjoin3->cfg->nbin;
    int nmicchannel = rtjoin3->cfg->nmicchannel;
    int clip_s = rtjoin3->cfg->clip_s;
    int clip_e = rtjoin3->cfg->clip_e;
    int i, k;

    memset(fftx, 0, nbin * sizeof(wtk_complex_t));
    for (k = clip_s; k < clip_e; ++k) {
        for (i = 0; i < nmicchannel; ++i) {
            fftx[k].a += fft[i][k].a;
            fftx[k].b += fft[i][k].b;
        }
    }
}

void wtk_rtjoin3_feed(wtk_rtjoin3_t *rtjoin3, short *data, int len,
                      int is_end) {
    int i, j, k;
    int nmicchannel = rtjoin3->cfg->nmicchannel;
    int *mic_channel = rtjoin3->cfg->mic_channel;
    int channel = rtjoin3->cfg->channel;
    wtk_strbuf_t **mic = rtjoin3->mic;
    float fv, *fp1;
    int wins = rtjoin3->cfg->wins;
    int fsize = wins / 2;
    int nbin = rtjoin3->cfg->nbin;
    int length;
    float *rfft_in = rtjoin3->rfft_in;
    wtk_drft_t *rfft = rtjoin3->rfft;
    wtk_complex_t **fft = rtjoin3->fft;
    float **analysis_mem = rtjoin3->analysis_mem;
    float *synthesis_mem = rtjoin3->synthesis_mem;
    float **mul_synthesis_mem = rtjoin3->mul_synthesis_mem;
    float *analysis_window = rtjoin3->analysis_window,
          *synthesis_window = rtjoin3->synthesis_window;
    wtk_complex_t *fftx = rtjoin3->fftx;
    float *mul_out = rtjoin3->mul_out;
    float *out = rtjoin3->out;
    short *pv = (short *)out;
    int clip_s = rtjoin3->cfg->clip_s;
    int clip_e = rtjoin3->cfg->clip_e;

    for (i = 0; i < len; ++i) {
        for (j = 0; j < nmicchannel; ++j) {
            fv = data[mic_channel[j]];
            wtk_strbuf_push(mic[j], (char *)(&fv), sizeof(float));
        }
        data += channel;
    }
    length = mic[0]->pos / sizeof(float);
    while (length >= fsize) {
        for (i = 0; i < nmicchannel; ++i) {
            fp1 = (float *)mic[i]->data;
            wtk_drft_frame_analysis(rfft, rfft_in, analysis_mem[i], fft[i], fp1,
                                    wins, analysis_window);
        }

        wtk_rtjoin3_feed_mix(rtjoin3, fft, fftx);

        for (k = 0; k <= clip_s; ++k) {
            fftx[k].a = fftx[k].b = 0;
        }
        for (k = clip_e; k < nbin; ++k) {
            fftx[k].a = fftx[k].b = 0;
        }

        if (rtjoin3->qmmse) {
            wtk_qmmse_denoise(rtjoin3->qmmse, fftx);
        }

        if (rtjoin3->cfg->use_mul_out) {
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
            if (rtjoin3->notify) {
                rtjoin3->notify(rtjoin3->ths, pv, fsize * nmicchannel);
            }
        } else {
            wtk_drft_frame_synthesis(rfft, rfft_in, synthesis_mem, fftx, out,
                                     wins, synthesis_window);
            if (rtjoin3->eq) {
                wtk_equalizer_feed_float(rtjoin3->eq, out, fsize);
            }
            if (rtjoin3->cfg->use_limiter) {
                _limiter(rtjoin3, out, fsize);
            } else if (rtjoin3->cfg->use_control_bs) {
                wtk_rtjoin3_control_bs(rtjoin3, out, fsize);
            }
            for (i = 0; i < fsize; ++i) {
                pv[i] = floorf(out[i] + 0.5);
            }
            if (rtjoin3->notify) {
                rtjoin3->notify(rtjoin3->ths, pv, fsize);
            }
        }
        wtk_strbufs_pop(mic, nmicchannel, fsize * sizeof(float));
        length = mic[0]->pos / sizeof(float);
    }
}
