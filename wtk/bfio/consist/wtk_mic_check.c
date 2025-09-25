#include "wtk/bfio/consist/wtk_mic_check.h"
#include "qtk/math/qtk_vector.h"
#ifndef WTK_WAV_SHORT_TO_FLOAT
#define WTK_WAV_SHORT_TO_FLOAT(f) ((f) > 0 ? (f / 32767.0) : (f / 32768.0))
#endif
#ifndef WTK_WAV_FLOAT_TO_SHORT
#define WTK_WAV_FLOAT_TO_SHORT(f)                                              \
    ((f) > 0 ? floorf(f * 32767.0 + 0.5) : floorf(f * 32768.0 + 0.5))
#endif

wtk_mic_check_t *wtk_mic_check_new(wtk_mic_check_cfg_t *cfg) {
    int i;
    wtk_mic_check_t *mic_check;

    mic_check = (wtk_mic_check_t *)wtk_malloc(sizeof(wtk_mic_check_t));
    mic_check->cfg = cfg;
    mic_check->ths = NULL;
    mic_check->notify = NULL;
    mic_check->mic = wtk_strbufs_new(mic_check->cfg->nmicchannel);
    mic_check->sp = wtk_strbufs_new(mic_check->cfg->nspchannel);

    mic_check->rfft = NULL;
    mic_check->fft_in = NULL;
    mic_check->rfft_in = NULL;
    mic_check->analysis_window = NULL;
    mic_check->synthesis_window = NULL;
    mic_check->analysis_mem = NULL;
    mic_check->analysis_mem_sp = NULL;
    mic_check->fft = NULL;
    mic_check->fft_sp = NULL;
    if (cfg->use_fft) {
        mic_check->nbin = cfg->wins / 2 + 1;
        if (cfg->use_pffft) {
            mic_check->rfft = wtk_drft_new2(cfg->wins);
        } else {
            mic_check->rfft = wtk_drft_new(cfg->wins);
        }
        mic_check->fft_in = (float *)wtk_malloc(sizeof(float) * cfg->wins);
        mic_check->rfft_in = (float *)wtk_malloc(sizeof(float) * cfg->wins);
        mic_check->analysis_window = wtk_malloc(sizeof(float) * cfg->wins);
        mic_check->synthesis_window = wtk_malloc(sizeof(float) * cfg->wins);
        mic_check->analysis_mem =
            wtk_float_new_p2(cfg->nmicchannel, mic_check->nbin - 1);
        mic_check->analysis_mem_sp =
            wtk_float_new_p2(cfg->nspchannel, mic_check->nbin - 1);

        mic_check->fft = wtk_complex_new_p2(cfg->nmicchannel, mic_check->nbin);
        mic_check->fft_sp =
            wtk_complex_new_p2(cfg->nspchannel, mic_check->nbin);
    }

    mic_check->eng = (float *)wtk_malloc(sizeof(float) * (cfg->channel + 4));
    mic_check->var = (float *)wtk_malloc(sizeof(float) * (cfg->channel + 4));
    mic_check->sc = (float *)wtk_malloc(sizeof(float) * (cfg->channel + 4));
    mic_check->tmp_median =
        (float *)wtk_malloc(sizeof(float) * (cfg->channel + 4));
    mic_check->tmp_state = (int *)wtk_malloc(sizeof(int) * (cfg->channel + 4));
    mic_check->mic_state = (int *)wtk_malloc(sizeof(int) * cfg->nmicchannel);
    mic_check->sp_state = (int *)wtk_malloc(sizeof(int) * cfg->nspchannel);
    mic_check->eng_median = (float *)wtk_malloc(sizeof(float) * cfg->channel);
    mic_check->var_median = (float *)wtk_malloc(sizeof(float) * cfg->channel);
    mic_check->sc_median = (float *)wtk_malloc(sizeof(float) * cfg->channel);

    mic_check->type = (wtk_mic_check_err_type_t *)wtk_malloc(
        sizeof(wtk_mic_check_err_type_t) * cfg->channel);

    wtk_mic_check_reset(mic_check);
    return mic_check;
}
void wtk_mic_check_delete(wtk_mic_check_t *mic_check) {
    int nmicchannel = mic_check->cfg->nmicchannel;
    int nspchannel = mic_check->cfg->nspchannel;
    int nbin = mic_check->nbin;

    wtk_strbufs_delete(mic_check->mic, nmicchannel);
    wtk_strbufs_delete(mic_check->sp, nspchannel);

    if (mic_check->rfft) {
        if (mic_check->cfg->use_pffft) {
            wtk_drft_delete2(mic_check->rfft);
        } else {
            wtk_drft_delete(mic_check->rfft);
        }
    }
    if (mic_check->fft_in) {
        wtk_free(mic_check->fft_in);
    }
    if (mic_check->rfft_in) {
        wtk_free(mic_check->rfft_in);
    }
    if (mic_check->analysis_window) {
        wtk_free(mic_check->analysis_window);
    }
    if (mic_check->synthesis_window) {
        wtk_free(mic_check->synthesis_window);
    }
    if (mic_check->analysis_mem) {
        wtk_float_delete_p2(mic_check->analysis_mem, nmicchannel);
    }
    if (mic_check->analysis_mem_sp) {
        wtk_float_delete_p2(mic_check->analysis_mem_sp, nspchannel);
    }
    if (mic_check->fft) {
        wtk_complex_delete_p2(mic_check->fft, nmicchannel);
    }
    if (mic_check->fft_sp) {
        wtk_complex_delete_p2(mic_check->fft_sp, nspchannel);
    }

    wtk_free(mic_check->eng);
    wtk_free(mic_check->var);
    wtk_free(mic_check->sc);
    wtk_free(mic_check->tmp_median);
    wtk_free(mic_check->tmp_state);
    wtk_free(mic_check->mic_state);
    wtk_free(mic_check->sp_state);
    wtk_free(mic_check->eng_median);
    wtk_free(mic_check->var_median);
    wtk_free(mic_check->sc_median);
    wtk_free(mic_check->type);

    wtk_free(mic_check);
}

void wtk_mic_check_start(wtk_mic_check_t *mic_check) {}
void wtk_mic_check_reset(wtk_mic_check_t *mic_check) {
    int wins = mic_check->cfg->wins;
    int fsize = wins / 2;
    int i, nbin = mic_check->nbin;
    int nmicchannel = mic_check->cfg->nmicchannel;
    int nspchannel = mic_check->cfg->nspchannel;
    int channel = mic_check->cfg->channel;

    wtk_strbufs_reset(mic_check->mic, nmicchannel);
    wtk_strbufs_reset(mic_check->sp, nspchannel);

    if (mic_check->fft_in) {
        memset(mic_check->fft_in, 0, sizeof(float) * wins);
    }
    if (mic_check->rfft_in) {
        memset(mic_check->rfft_in, 0, sizeof(float) * wins);
    }
    if (mic_check->analysis_window) {
        for (i = 0; i < wins; ++i) {
            mic_check->analysis_window[i] = sin((0.5 + i) * PI / (wins));
        }
        if (mic_check->synthesis_window) {
            wtk_drft_init_synthesis_window(mic_check->synthesis_window,
                                           mic_check->analysis_window, wins);
        }
    }
    if (mic_check->analysis_mem) {
        wtk_float_zero_p2(mic_check->analysis_mem, nmicchannel, (nbin - 1));
    }
    if (mic_check->analysis_mem_sp) {
        wtk_float_zero_p2(mic_check->analysis_mem_sp, nspchannel, (nbin - 1));
    }

    if (mic_check->fft) {
        wtk_complex_zero_p2(mic_check->fft, nmicchannel, nbin);
    }
    if (mic_check->fft_sp) {
        wtk_complex_zero_p2(mic_check->fft_sp, nspchannel, nbin);
    }

    memset(mic_check->eng, 0, sizeof(float) * (channel + 4));
    memset(mic_check->var, 0, sizeof(float) * (channel + 4));
    memset(mic_check->sc, 0, sizeof(float) * (channel + 4));
    memset(mic_check->tmp_median, 0, sizeof(float) * (channel + 4));
    memset(mic_check->tmp_state, 0, sizeof(int) * (channel + 4));
    memset(mic_check->mic_state, 0, sizeof(int) * nmicchannel);
    memset(mic_check->sp_state, 0, sizeof(int) * nspchannel);
    memset(mic_check->eng_median, 0, sizeof(float) * channel);
    memset(mic_check->var_median, 0, sizeof(float) * channel);
    memset(mic_check->sc_median, 0, sizeof(float) * channel);
    memset(mic_check->type, 0, sizeof(wtk_mic_check_err_type_t) * channel);

    mic_check->sp_silcnt = 0;
    mic_check->sp_sil = 1;
    mic_check->mic_silcnt = 0;
    mic_check->mic_sil = 1;

    mic_check->mic_scale = mic_check->cfg->mic_scale;
    mic_check->sp_scale = mic_check->cfg->sp_scale;
    mic_check->frame = 0;
}
void wtk_mic_check_set_notify(wtk_mic_check_t *mic_check, void *ths,
                              wtk_mic_check_notify_f notify) {
    mic_check->notify = notify;
    mic_check->ths = ths;
}

static float wtk_mic_check_var_energy(float *p, int n) {
    float f, f2;
    float tmp;
    int i;

    f = 0;
    for (i = 0; i < n; ++i) {
        f += p[i];
    }
    f /= n;

    f2 = 0;
    for (i = 0; i < n; ++i) {
        tmp = p[i] - f;
        f2 += tmp * tmp;
    }
    f2 /= n;

    return f2;
}

static float wtk_mic_check_energy(float *p, int n) {
    float f;
    int i;

    f = 0;
    for (i = 0; i < n; ++i) {
        f += p[i] * p[i];
    }
    f /= n;

    return f;
}

static float wtk_mic_check_spectral_centroid(wtk_complex_t *fft, int nbin) {
    float f, f2;
    float tmp;
    int i;

    f = 0;
    f2 = 0;
    for (i = 0; i < nbin; ++i) {
        tmp = sqrtf(fft[i].a * fft[i].a + fft[i].b * fft[i].b);
        f += i * tmp;
        f2 += tmp;
    }

    if (f2 > 0) {
        f /= f2;
    } else {
        f = 0;
    }

    return f;
}

void wtk_mic_check_rcd_check(wtk_mic_check_t *mic_check, int is_end) {
    wtk_strbuf_t **mic = mic_check->mic;
    wtk_complex_t **fft = mic_check->fft;
    int nbin = mic_check->nbin;
    int wins = mic_check->cfg->wins;
    int fsize = wins / 2;
    int nmicchannel = mic_check->cfg->nmicchannel;
    int micenr_cnt = mic_check->cfg->micenr_cnt;
    float micenr_thresh = mic_check->cfg->micenr_thresh;
    float eng_thresh = mic_check->cfg->eng_thresh;
    float var_thresh = mic_check->cfg->var_thresh;
    float sc_thresh = mic_check->cfg->sc_thresh;
    float eng_thresh2 = mic_check->cfg->eng_thresh2;
    float var_thresh2 = mic_check->cfg->var_thresh2;
    float sc_thresh2 = mic_check->cfg->sc_thresh2;
    float rcd_thresh = mic_check->cfg->rcd_thresh;
    float micenr = 0;
    float *tmp_median = mic_check->tmp_median;
    float alpha = mic_check->cfg->alpha;
    float alpha_1 = 1.0 - alpha;
    int i;
    float *fv;

    if (is_end) {
        for (i = 0; i < nmicchannel; ++i) {
            if (mic_check->mic_state[i] == 0) {
                mic_check->type[i] = WTK_MIC_CHECK_NORMAL;
                // printf("0\n");
            } else {
                if (mic_check->mic_state[i] * (1.0 / mic_check->frame) >
                    rcd_thresh) {
                    mic_check->type[i] = WTK_MIC_CHECK_RCD_ERROR;
                } else {
                    mic_check->type[i] = WTK_MIC_CHECK_NORMAL;
                }
                // printf("%f\n", mic_check->mic_state[i] * (1.0 /
                // mic_check->frame));
            }
        }
        if (mic_check->notify) {
            mic_check->notify(mic_check->ths, mic_check->type, nmicchannel);
        }
        return;
    }

    for (i = 0; i < nmicchannel; ++i) {
        fv = (float *)(mic[i]->data);
        micenr = max(wtk_mic_check_energy(fv, fsize), micenr);
    }
    if (micenr > micenr_thresh) {
        mic_check->mic_sil = 0;
        mic_check->mic_silcnt = micenr_cnt;
    } else if (mic_check->mic_sil == 0) {
        mic_check->mic_silcnt -= 1;
        if (mic_check->mic_silcnt <= 0) {
            mic_check->mic_sil = 1;
        }
    }

    if (mic_check->mic_sil == 0) {
        mic_check->frame++;
        memset(mic_check->tmp_state, 0, sizeof(int) * nmicchannel);
        for (i = 0; i < nmicchannel; ++i) {
            fv = (float *)(mic[i]->data);
            mic_check->eng[i] = mic_check->eng[i] * alpha +
                                wtk_mic_check_energy(fv, fsize) * alpha_1;
            mic_check->var[i] = mic_check->var[i] * alpha +
                                wtk_mic_check_var_energy(fv, fsize) * alpha_1;
            if (mic_check->cfg->use_fft) {
                mic_check->sc[i] =
                    mic_check->sc[i] * alpha +
                    wtk_mic_check_spectral_centroid(fft[i], nbin) * alpha_1;
            } else {
                mic_check->sc[i] = 0;
            }
            // printf("%f\n", mic_check->eng[i]);
            // printf("%f\n", mic_check->var[i]);
            // printf("%f\n", mic_check->sc[i]);
        }
        for (i = 0; i < nmicchannel; ++i) {
            if (mic_check->eng[i] < eng_thresh) {
                mic_check->tmp_state[i] = 1;
            }
            if (mic_check->var[i] < var_thresh) {
                mic_check->tmp_state[i] = 1;
            }
            if (mic_check->sc[i] < sc_thresh) {
                mic_check->tmp_state[i] = 1;
            }
        }

        memcpy(tmp_median, mic_check->eng, sizeof(float) * nmicchannel);
        mic_check->eng[nmicchannel] = wtk_float_median(tmp_median, nmicchannel);
        memcpy(tmp_median, mic_check->var, sizeof(float) * nmicchannel);
        mic_check->var[nmicchannel] = wtk_float_median(tmp_median, nmicchannel);
        memcpy(tmp_median, mic_check->sc, sizeof(float) * nmicchannel);
        mic_check->sc[nmicchannel] = wtk_float_median(tmp_median, nmicchannel);

        for (i = 0; i < nmicchannel; ++i) {
            mic_check->eng_median[i] =
                fabs(mic_check->eng[nmicchannel] - mic_check->eng[i]);
            mic_check->var_median[i] =
                fabs(mic_check->var[nmicchannel] - mic_check->var[i]);
            mic_check->sc_median[i] =
                fabs(mic_check->sc[nmicchannel] - mic_check->sc[i]);
            // printf("%f\n", mic_check->eng_median[i]);
            // printf("%f\n", mic_check->var_median[i]);
            // printf("%f\n", mic_check->sc_median[i]);
        }
        for (i = 0; i < nmicchannel; ++i) {
            if (mic_check->eng_median[i] > eng_thresh2) {
                mic_check->tmp_state[i] = 1;
            }
            if (mic_check->sc_median[i] > sc_thresh2) {
                mic_check->tmp_state[i] = 1;
            }
        }
        for (i = 0; i < nmicchannel; ++i) {
            if (mic_check->tmp_state[i]) {
                mic_check->mic_state[i]++;
            }
        }
    }
}

void wtk_mic_check_play_check(wtk_mic_check_t *mic_check, float *play_vol,
                              int is_end) {
    wtk_strbuf_t **mic = mic_check->mic;
    wtk_strbuf_t **sp = mic_check->sp;
    int nbin = mic_check->nbin;
    int wins = mic_check->cfg->wins;
    int fsize = wins / 2;
    int nmicchannel = mic_check->cfg->nmicchannel;
    int nspchannel = mic_check->cfg->nspchannel;
    int micenr_cnt = mic_check->cfg->micenr_cnt;
    float micenr_thresh = mic_check->cfg->micenr_thresh;
    float var_thresh = mic_check->cfg->var_thresh;
    float play_thresh = mic_check->cfg->play_thresh;
    float play_thresh2 = mic_check->cfg->play_thresh2;
    float micenr = 0;
    float alpha = mic_check->cfg->alpha;
    float alpha_1 = 1.0 - alpha;
    float *fv, *fv1;
    int i;

    if (is_end) {
        for (i = 0; i < nmicchannel; ++i) {
            if (mic_check->mic_state[i] == 0) {
                mic_check->type[i] = WTK_MIC_CHECK_NORMAL;
                // printf("0\n");
            } else {
                if (mic_check->mic_state[i] * (1.0 / mic_check->frame) <
                    play_thresh) {
                    if (mic_check->sp_state[i] *
                            (1.0 / mic_check->frame) >
                        play_thresh) {
                        mic_check->type[i] = WTK_MIC_CHECK_PLAY_ERROR;
                    } else {
                        mic_check->type[i] = WTK_MIC_CHECK_NORMAL;
                    }
                } else {
                    mic_check->type[i] = WTK_MIC_CHECK_NORMAL;
                }
                // printf("%f %f\n",
                //        mic_check->mic_state[i] * (1.0 / mic_check->frame),
                //        mic_check->mic_state[i + nmicchannel] *
                //            (1.0 / mic_check->frame));
            }
        }
        if (mic_check->notify) {
            mic_check->notify(mic_check->ths, mic_check->type, nmicchannel);
        }
        return;
    }

    if (play_vol == NULL) {
        wtk_debug("play_vol is NULL\n");
        return;
    }

    for (i = 0; i < nmicchannel; ++i) {
        fv = (float *)(mic[i]->data);
        micenr = max(wtk_mic_check_energy(fv, fsize), micenr);
    }
    if (micenr > micenr_thresh) {
        mic_check->mic_sil = 0;
        mic_check->mic_silcnt = micenr_cnt;
    } else if (mic_check->mic_sil == 0) {
        mic_check->mic_silcnt -= 1;
        if (mic_check->mic_silcnt <= 0) {
            mic_check->mic_sil = 1;
        }
    }

    if (mic_check->mic_sil == 0) {
        mic_check->frame++;
        memset(mic_check->tmp_state, 0, sizeof(int) * nmicchannel * 2);
        for (i = 0; i < nmicchannel; ++i) {
            fv = (float *)(mic[i]->data);
            fv1 = (float *)(sp[i]->data);
            mic_check->var[i] = mic_check->var[i] * alpha +
                                wtk_mic_check_var_energy(fv, fsize) * alpha_1;
            mic_check->var[i + nmicchannel] =
                mic_check->var[i + nmicchannel] * alpha +
                wtk_mic_check_var_energy(fv1, fsize) * alpha_1;
            // printf("%f\n", mic_check->var[i]);
        }
        for (i = 0; i < nmicchannel; ++i) {
            if (play_vol[i] > 0) {
                if (mic_check->var[i] < var_thresh) {
                    mic_check->tmp_state[i] = 1;
                }
                if (mic_check->var[i + nmicchannel] < var_thresh) {
                    mic_check->tmp_state[i + nmicchannel] = 1;
                }
            }
        }
        for (i = 0; i < nmicchannel; ++i) {
            if (mic_check->tmp_state[i]) {
                mic_check->mic_state[i]++;
            }
            if (mic_check->tmp_state[i + nmicchannel]) {
                mic_check->sp_state[i]++;
            }
        }
    }
}

void wtk_mic_check_feed(wtk_mic_check_t *mic_check, short *data, int len,
                        float *play_vol, int is_end) {
    int i, j;
    int nbin = mic_check->nbin;
    int nmicchannel = mic_check->cfg->nmicchannel;
    int *mic_channel = mic_check->cfg->mic_channel;
    int nspchannel = mic_check->cfg->nspchannel;
    int *sp_channel = mic_check->cfg->sp_channel;
    int channel = mic_check->cfg->channel;
    int wins = mic_check->cfg->wins;
    int fsize = wins / 2;
    wtk_drft_t *rfft = mic_check->rfft;
    float *fft_in = mic_check->fft_in;
    float *rfft_in = mic_check->rfft_in;
    wtk_complex_t **fft = mic_check->fft;
    wtk_complex_t **fft_sp = mic_check->fft_sp;
    float **analysis_mem = mic_check->analysis_mem;
    float **analysis_mem_sp = mic_check->analysis_mem_sp;
    float *analysis_window = mic_check->analysis_window;
    wtk_strbuf_t **mic = mic_check->mic;
    wtk_strbuf_t **sp = mic_check->sp;
    float fv;
    int length;
    float spenr;
    float spenr_thresh = mic_check->cfg->spenr_thresh;
    int spenr_cnt = mic_check->cfg->spenr_cnt;
    float mic_scale = mic_check->mic_scale;
    float sp_scale = mic_check->sp_scale;
    float *fv1;

    for (i = 0; i < len; ++i) {
        for (j = 0; j < nmicchannel; ++j) {
            // fv = WTK_WAV_SHORT_TO_FLOAT(data[mic_channel[j]]) * mic_scale;
            fv = data[mic_channel[j]] * mic_scale;
            wtk_strbuf_push(mic[j], (char *)&(fv), sizeof(float));
        }
        for (j = 0; j < nspchannel; ++j) {
            // fv = WTK_WAV_SHORT_TO_FLOAT(data[sp_channel[j]]) * sp_scale;
            fv = data[sp_channel[j]] * sp_scale;
            wtk_strbuf_push(sp[j], (char *)&(fv), sizeof(float));
        }
        data += channel;
    }
    length = mic[0]->pos / sizeof(float);
    while (length >= fsize) {
        if (mic_check->cfg->use_fft) {
            for (i = 0; i < nmicchannel; ++i) {
                fv1 = (float *)(mic[i]->data);
                qtk_vector_scale(fv1, fft_in, fsize, 1.0 / 32768.0);
                // {
                //     int ii;
                //     int n = wins;

                //     for (ii = 0; ii < n; ++ii) {
                //         analysis_window[ii] =
                //             sqrtf(0.5 * (1 - cos(2 * PI * (ii) / (n - 1))));
                //     }
                // }
                if (mic_check->cfg->use_pffft) {
                    wtk_drft_stft2(rfft, rfft_in, analysis_mem[i], fft[i], fv1,
                                   wins, analysis_window);
                } else {
                    wtk_drft_stft(rfft, rfft_in, analysis_mem[i], fft[i], fv1,
                                  wins, analysis_window);
                }
            }
            for (i = 0; i < nspchannel; ++i) {
                fv1 = (float *)(sp[i]->data);
                qtk_vector_scale(fv1, fft_in, fsize, 1.0 / 32768.0);
                if (mic_check->cfg->use_pffft) {
                    wtk_drft_stft2(rfft, rfft_in, analysis_mem_sp[i], fft_sp[i],
                                   fv1, wins, analysis_window);
                } else {
                    wtk_drft_stft(rfft, rfft_in, analysis_mem_sp[i], fft_sp[i],
                                  fv1, wins, analysis_window);
                }
            }
        }
        if (nspchannel > 0) {
            fv1 = (float *)(sp[0]->data);
            spenr = wtk_mic_check_var_energy(fv1, fsize);
        } else {
            spenr = 0;
        }
        // printf("%f\n", spenr);
        // static int cnt=0;
        // cnt++;
        if (spenr > spenr_thresh) {
            // if(mic_check->sp_sil==1)
            // {
            // 	printf("sp start %f %f
            // %f\n", 1.0/16000*cnt*(nbin-1),spenr,spenr_thresh);
            // }
            mic_check->sp_sil = 0;
            mic_check->sp_silcnt = spenr_cnt;
        } else if (mic_check->sp_sil == 0) {
            mic_check->sp_silcnt -= 1;
            if (mic_check->sp_silcnt <= 0) {
                // printf("sp end %f\n", 1.0/16000*cnt*(nbin-1));
                mic_check->sp_sil = 1;
            }
        }

        switch (mic_check->cfg->type) {
        case WTK_MIC_CHECK_RCD_CHECK:
            wtk_mic_check_rcd_check(mic_check, is_end);
            break;
        case WTK_MIC_CHECK_PLAY_CHECK:
            wtk_mic_check_play_check(mic_check, play_vol, is_end);
            break;
        default:
            break;
        }

        wtk_strbufs_pop(mic, nmicchannel, fsize * sizeof(float));
        wtk_strbufs_pop(sp, nspchannel, fsize * sizeof(float));
        length = mic[0]->pos / sizeof(float);
    }
    if (is_end) {
        switch (mic_check->cfg->type) {
        case WTK_MIC_CHECK_RCD_CHECK:
            wtk_mic_check_rcd_check(mic_check, is_end);
            break;
        case WTK_MIC_CHECK_PLAY_CHECK:
            wtk_mic_check_play_check(mic_check, play_vol, is_end);
            break;
        default:
            break;
        }
    }
}

void wtk_mic_check_set_micscale(wtk_mic_check_t *mic_check, float scale) {
    mic_check->mic_scale = scale;
}