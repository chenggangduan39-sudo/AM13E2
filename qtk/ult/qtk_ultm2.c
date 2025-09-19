#include "qtk_ultm2.h"
#include "wtk/core/fft/wtk_cfft.h"

void qtk_ultm2_on_conv(wtk_strbuf_t *buf, float *data, int len);
void qtk_ultm2_skip(qtk_ultm2_t *m, int n);
void qtk_ultm2_set_cut(qtk_ultm2_t *m, int cut);

qtk_ultm2_t *qtk_ultm2_new(qtk_ultm2_cfg_t *cfg, int L, int skip) {
    qtk_ultm2_t *m;
    int i;
    int channel = cfg->channel;

    m = (qtk_ultm2_t *)wtk_malloc(sizeof(qtk_ultm2_t));
    m->cfg = cfg;
    m->L = L;
    m->skip = skip;
    m->mic_conved =
        (wtk_strbuf_t **)wtk_malloc(sizeof(wtk_strbuf_t *) * channel);
    
    m->echo_raw = NULL;
    m->echo_conved = NULL;
    m->echo_conv = NULL;
    m->echo_align_conved[0] = NULL;
    m->echo_align_conved[1] = NULL;
    m->echo_align_raw[0] = NULL;
    m->echo_align_raw[1] = NULL;
    m->echo_align_conv[0] = NULL;
    m->echo_align_conv[1] = NULL;
    m->align_idx = NULL;
    if (cfg->use_echo) {
        m->echo_conved = wtk_strbuf_new(1024, 1);
        m->echo_raw = wtk_strbuf_new(1024, 1);
        m->echo_conv =
            wtk_conv2_new(cfg->echo_kernel, cfg->echo_fft, NULL, cfg->echo_fft);
        wtk_conv2_set_notify(m->echo_conv,
                             (wtk_conv2_notify_f)qtk_ultm2_on_conv,
                             m->echo_conved);

        if(m->cfg->use_echo_align){
            m->echo_align_conved[0] = wtk_strbuf_new(1024, 1);
            m->echo_align_conved[1] = wtk_strbuf_new(1024, 1);
            m->echo_align_raw[0] = wtk_strbuf_new(1024, 1);
            m->echo_align_raw[1] = wtk_strbuf_new(1024, 1);
            m->echo_align_conv[0] =
                wtk_conv2_new(cfg->r_zc_pad_a, cfg->L, NULL, cfg->L);
            wtk_conv2_set_notify(m->echo_align_conv[0],
                                (wtk_conv2_notify_f)qtk_ultm2_on_conv,
                                m->echo_align_conved[0]);
            m->echo_align_conv[1] =
                wtk_conv2_new(cfg->r_zc_pad_b, cfg->L, NULL, cfg->L);
            wtk_conv2_set_notify(m->echo_align_conv[1],
                                (wtk_conv2_notify_f)qtk_ultm2_on_conv,
                                m->echo_align_conved[1]);
            m->align_idx = wtk_fring_new(cfg->align_cnt);
        }
    }
    m->mic_raw = wtk_strbuf_new(1024, 1);
    // wtk_debug("echo=%d\n",m->echo->pos);

    if (cfg->use_harmonic_d) {
        m->mic_h = wtk_strbuf_new(1024, 1);
        m->mic_h_conv = wtk_conv2_new(cfg->mic_h_kernel, cfg->mic_h_fft, NULL,
                                      cfg->mic_h_fft);
        wtk_conv2_set_notify(m->mic_h_conv,
                             (wtk_conv2_notify_f)qtk_ultm2_on_conv, m->mic_h);
    } else {
        m->mic_h_conv = NULL;
        m->mic_h = NULL;
    }
    // i=(cfg->sweep_n+cfg->num_tap*2+1);
    i = cfg->mic_fft + cfg->mic_fft - 1;
    i = pow(2, wtk_rfft_next_pow(i));
    m->fft = qtk_fft_new(i);
    m->conv2 = (wtk_conv2_t **)wtk_malloc(sizeof(wtk_conv2_t *) * channel);
    for (i = 0; i < channel; ++i) {
        int nx;

        // print_float(cfg->mic_kernel,cfg->mic_fft);
        // exit(0);
        // m->conv=wtk_conv_new(cfg->band_filter,cfg->num_tap*2+1);
        // wtk_conv_set_notify(m->conv,(wtk_conv_notify_f)qtk_ultm2_on_conv,m);
        m->mic_conved[i] = wtk_strbuf_new(1024, 1);
        nx = cfg->mic_fft;
        m->conv2[i] = wtk_conv2_new(cfg->mic_kernel, cfg->mic_fft, m->fft, nx);
        wtk_conv2_set_notify(m->conv2[i], (wtk_conv2_notify_f)qtk_ultm2_on_conv,
                             m->mic_conved[i]);
    }
    m->pv = (float **)wtk_malloc(sizeof(float *) * cfg->channel);
    m->pv_tmp = (float *)wtk_malloc(sizeof(float) * cfg->s2f_size);
    qtk_ultm2_reset(m);
    return m;
}

void qtk_ultm2_delete(qtk_ultm2_t *m) {
    int i;
    int channel = m->cfg->channel;

    wtk_free(m->pv);
    if (m->echo_conved) {
        wtk_strbuf_delete(m->echo_conved);
        wtk_strbuf_delete(m->echo_raw);
        wtk_conv2_delete(m->echo_conv);
    }
    if(m->echo_align_conved[0]){
        wtk_strbuf_delete(m->echo_align_conved[0]);
        wtk_strbuf_delete(m->echo_align_conved[1]);
        wtk_strbuf_delete(m->echo_align_raw[0]);
        wtk_strbuf_delete(m->echo_align_raw[1]);
        wtk_conv2_delete(m->echo_align_conv[0]);
        wtk_conv2_delete(m->echo_align_conv[1]);
        wtk_fring_delete(m->align_idx);
    }
    if (m->mic_raw) {
        wtk_strbuf_delete(m->mic_raw);
    }
    if (m->mic_h_conv) {
        wtk_strbuf_delete(m->mic_h);
        wtk_conv2_delete(m->mic_h_conv);
    }
    if (m->fft) {
        qtk_fft_delete(m->fft);
    }
    wtk_free(m->pv_tmp);
    for (i = 0; i < channel; ++i) {
        wtk_strbuf_delete(m->mic_conved[i]);
        wtk_conv2_delete(m->conv2[i]);
    }
    // wtk_free(m->phase_acc);
    wtk_free(m->mic_conved);
    wtk_free(m->conv2);
    wtk_free(m);
}

void qtk_ultm2_start(qtk_ultm2_t *m) {}

float qtk_ultm2_get_e(qtk_ultm2_t *m) {
    return sqrt(m->last_mic_e);
    // return m->m->fftd->e2;
}

void qtk_ultm2_reset_post(qtk_ultm2_t *m) {
    int len;

    // wtk_debug("reset exit ==========\n");
    // exit(0);
    m->cut_hint = 0;
    // m->cuted=0;
    // m->normed=0;
    if (m->echo_conved) {
        len = min(m->mic_conved[0]->pos / sizeof(float),
                  m->echo_conved->pos / sizeof(float));
    } else {
        len = m->mic_conved[0]->pos / sizeof(float);
    }
    // wtk_debug("UTLM: reset post skip=%d\n",len);
    if (len > 0) {
        wtk_debug("UTLM: reset post skip=%d\n", len);
        qtk_ultm2_skip(m, len);
    }
    // exit(0);
    // m->state=WTK_ULTM_SEEK_START;
    if (m->cfg->use_echo) {
        m->state = QTK_ULTM2_SEEK_START;
    } else {
        m->state = QTK_ULTM2_SEEK_START;
        // m->state=WTK_ULTM_ALIGND;
    }

    m->last_echo_e = 0;
    m->echo_hint = 0;
    m->last_echo_e2 = 0;
    m->last_mic_e = 0;
    m->nframe = 0;
    m->skipped = 0;
    m->last_hamonic_e = 0;
    m->harmed = 0;
    m->cuted = 0;
    m->harm_hint = 0;
}

void qtk_ultm2_reset(qtk_ultm2_t *m) {
    int channel = m->cfg->channel;
    int i;

    m->mic_err = 0;
    m->harm_hint = 0;
    m->harmed = 0;
    m->cuted = 0;
    // wtk_debug("UTLM: reset\n");
    m->cut_hint = 0;
    m->echoed = 0;
    m->nx = 0;
    if (m->cfg->use_echo) {
        m->state = QTK_ULTM2_INIT;
        // m->state=WTK_ULTM_ALIGND;
    } else {
        m->state = QTK_ULTM2_SEEK_START;
        // m->state=WTK_ULTM_ALIGND;
    }
    if (m->cfg->use_aligned) {
        m->state = QTK_ULTM2_ALIGND;
    }
    if (m->echo_conved) {
        wtk_strbuf_reset(m->echo_conved);
        wtk_strbuf_reset(m->echo_raw);
        wtk_conv2_reset(m->echo_conv);
    }
    if(m->echo_align_conved[0]){
        wtk_strbuf_reset(m->echo_align_conved[0]);
        wtk_strbuf_reset(m->echo_align_conved[1]);
        wtk_strbuf_reset(m->echo_align_raw[0]);
        wtk_strbuf_reset(m->echo_align_raw[1]);
        wtk_conv2_reset(m->echo_align_conv[0]);
        wtk_conv2_reset(m->echo_align_conv[1]);
        wtk_fring_reset(m->align_idx);
    }
    if (m->mic_h_conv) {
        wtk_strbuf_reset(m->mic_h);
        wtk_conv2_reset(m->mic_h_conv);
    }
    if (m->mic_raw) {
        wtk_strbuf_reset(m->mic_raw);
    }
    m->echo_hint = 0;
    m->last_echo_e = 0;
    m->last_echo_e2 = 0;
    m->last_hamonic_e = 0;
    m->last_mic_e = 0;
    m->nframe = 0;
    m->skipped = 0;
    for (i = 0; i < channel; ++i) {
        wtk_strbuf_reset(m->mic_conved[i]);
        wtk_conv2_reset(m->conv2[i]);
    }
}

void qtk_ultm2_on_conv(wtk_strbuf_t *buf, float *data, int len) {
    // print_float(data,min(len,10));
    // wtk_debug("len=%d\n",len);
    // exit(0);
    wtk_strbuf_push(buf, (char *)data, len * sizeof(float));
}

void qtk_ultm2_check_cut(qtk_ultm2_t *m, short *mic_r, int step) {
    int cut;

    // 检查截幅
    cut = wtk_short_cuts(mic_r, step);
    // wtk_debug("cut=%d at %f\n",cut,m->nx*1.0/m->cfg->rate);
    if (cut >= m->cfg->cut_hint) {
        m->cut_hint = m->cfg->cut_hint2;
        qtk_ultm2_set_cut(m, 1);
    } else {
        --m->cut_hint;
        if (m->cut_hint <= 0) {
            if (m->cut_hint >= -1) {
                qtk_ultm2_set_cut(m, 0);
            }
        } else {
            qtk_ultm2_set_cut(m, 1);
        }
    }
}

void qtk_ultm2_check_has_echo(qtk_ultm2_t *m, short *echo_r, int step) {
    float f, f2;

    // 检查回声
    f = wtk_short_sum2(echo_r, step);
    f2 = fabs(m->last_echo_e2 - f);
    // wtk_debug("echo[%d]: %f/%f\n",m->echoed,f,f2);
    // wtk_debug("echo[%d/%ld/%ld]=%f/%f at
    // %f\n",m->echoed,m->echo->pos/sizeof(float),m->recho->pos/sizeof(short),f,f2,m->nx*1.0/m->cfg->rate);
    if (m->echoed) {
        if (f2 < m->cfg->echo_range2) {
            ++m->echo_hint;
            if (m->echo_hint > m->cfg->echo_hint2) {
                if (m->cfg->debug) {
                    wtk_debug("echo to sil [%ld/%ld]=%f/%f at %f\n",
                              m->echo_conved->pos / sizeof(float),
                              m->echo_raw->pos / sizeof(short), f, f2,
                              m->nx * 1.0 / m->cfg->rate);
                }
                m->echo_hint = 0;
                m->echoed = 0;
            }
        } else {
            m->echo_hint = 0;
        }
    } else {
        if (m->nx * 1.0 / m->cfg->rate < 1.0) {

        } else {
            // wtk_debug("echoed[%d]=%f/%f %f/%f at
            // %f\n",m->echoed,m->last_echo_e,m->last_echo_e2,f,f2,m->nx*1.0/m->cfg->rate);
            if (f2 > m->cfg->echo_range1) {
                ++m->echo_hint;
                if (m->echo_hint > m->cfg->echo_hint1) {
                    if (m->cfg->debug) {
                        wtk_debug("sil to echo =%f/%f %f/%f at %f\n",
                                  m->last_echo_e, m->last_echo_e2, f, f2,
                                  m->nx * 1.0 / m->cfg->rate);
                    }
                    // exit(0);
                    m->echo_hint = 0;
                    m->echoed = 1;
                }
            } else {
                m->echo_hint = 0;
            }
        }
    }
    m->last_echo_e2 = f;
}

int qtk_ultm2_check_echo_energy(qtk_ultm2_t *m, float *echo_c, int step) {
    float f, f2;

    f = wtk_float_sum2(echo_c, step);
    f2 = fabs(m->last_echo_e - f);
    // wtk_debug("ULTM echo=%f/%f at
    // %f\n",f,m->last_echo_e,m->nx*1.0/m->cfg->rate);
    if (f < m->cfg->echo_min_f || f2 > m->cfg->echo_min_f2) {
        if (m->cfg->debug) {
            wtk_debug("ULTM echo=%f/%f at %f\n", f, m->last_echo_e,
                      m->nx * 1.0 / m->cfg->rate);
        }
        if (m->nx * 1.0 / m->cfg->rate < 30) {
            // 开始系统调节能量
        } else {
            wtk_debug("echod=%d echo lst=%f cur=%f f2=%f/%f failed at %f\n",
                      m->echoed, m->last_echo_e, f, f2, m->cfg->echo_min_f2,
                      m->nx * 1.0 / m->cfg->rate);
            return -1;
        }
    }
    m->last_echo_e = f;
    return 0;
}

int qtk_ultm2_check_mic_energy(qtk_ultm2_t *m, float *mic_c, int step) {
    float f, f2;

    f = wtk_float_sum2(mic_c, step);
    // wtk_debug("mic=%f/%f/%f at
    // %f\n",f,m->last_mic_e,sqrt(m->last_mic_e),m->nx*1.0/m->cfg->rate);
    f2 = fabs(m->last_mic_e - f);
    if (f < m->cfg->mic_min_f ||
        (m->last_mic_e > 1.0 && f2 > m->cfg->mic_min_f2)) {
        if (m->cfg->debug) {
            wtk_debug("ULTM mic=%f/%f at %f\n", f, m->last_mic_e,
                      m->nx * 1.0 / m->cfg->rate);
        }
        if (m->nx * 1.0 / m->cfg->rate < 30) {

        } else {
            wtk_debug("echod=%d mic %f/%f f2=%f/%f failed at %f\n", m->echoed,
                      m->last_mic_e, f, f2, m->cfg->mic_min_f2,
                      m->nx * 1.0 / m->cfg->rate);
            return -1;
        }
    }
    if (m->cut_hint <= 0) {
        m->last_mic_e = f;
    }
    return 0;
}

void qtk_ultm2_set_harmed(qtk_ultm2_t *m, int harmed) { m->harmed = harmed; }

void qtk_ultm2_check_mic_harmonic(qtk_ultm2_t *m, float *mic_h_c, int step) {
    float f;

    f = wtk_float_sum2(mic_h_c, step);
    // wtk_debug("harmonic=%f at %f\n",f,m->nx*1.0/m->cfg->rate);
    // wtk_umsc2_log_e(&f,1);
    if (m->harmed) {
        if (f >= m->cfg->harm_thresh2) {
            m->harm_hint = 0;
        } else {
            wtk_debug("harmonic=%f at %f\n", f, m->nx * 1.0 / m->cfg->rate);
            ++m->harm_hint;
            if (m->harm_hint >= m->cfg->harm_hint2) {
                m->harm_hint = 0;
                qtk_ultm2_set_harmed(m, 0);
            }
        }
    } else {
        if (f >= m->cfg->harm_thresh) {
            // wtk_debug("harmonic=%f at %f\n", f, m->nx * 1.0 / m->cfg->rate);
            ++m->harm_hint;
            // wtk_debug("f=%f at %f\n",f,post->nframe*0.08*3);
            if (m->harm_hint >= m->cfg->harm_hint) {
                // exit(0);
                m->harm_hint = 0;
                qtk_ultm2_set_harmed(m, 1);
            }
        } else {
            m->harm_hint = 0;
        }
    }

    m->last_hamonic_e = f;
}

int qtk_ultm2_check_echo(qtk_ultm2_t *m, short *echo_r, float *echo_c) {
    int step = m->L;
    int ret;

    if (echo_c) {
        // 检查回声超声能量是否正常
        ret = qtk_ultm2_check_echo_energy(m, echo_c, step);
        if (ret != 0) {
            return ret;
        }
    }
    if (echo_r) {
        // 检查是否有回声
        qtk_ultm2_check_has_echo(m, echo_r, step);
    }
    return 0;
}

int qtk_ultm2_check_mic(qtk_ultm2_t *m, short *mic_r, float *mic_c,
                        float *mic_h_c) {
    int step = m->L;
    int ret;

    if (mic_r) {
        // 检查截幅
        qtk_ultm2_check_cut(m, mic_r, step);
    }
    // 检查超声下面频段是否有谐波
    if (mic_h_c) {
        qtk_ultm2_check_mic_harmonic(m, mic_h_c, step);
    }
    if (mic_c) {
        // 检查录音超声能量是否正常
        ret = qtk_ultm2_check_mic_energy(m, mic_c, step);
        if (ret != 0) {
            return ret;
        }
    }
    return 0;
}

void qtk_ultm2_update_phase3(qtk_ultm2_t *m) {
    int skip = m->skip;
    int channel = m->cfg->channel;
    int j;
    float **pv = m->pv;
    int len;
    int L = m->L;
    int step = L * (skip + 1);
    int cnt;
    wtk_strbuf_t **mic_conved;
    int ret;
    short *mic_r, *echo_r;
    float *mic_c, *echo_c;
    float *mic_h_c;
    float f;

    mic_conved = m->mic_conved;
    len = mic_conved[0]->pos / sizeof(float);
    if (len < step) {
        return;
    }
    if (m->echo_conv) {
        echo_r = (short *)(m->echo_raw->data);
        echo_c = (float *)(m->echo_conved->data);
    } else {
        echo_r = NULL;
        echo_c = NULL;
    }
    mic_r = (short *)(m->mic_raw->data);
    mic_c = (float *)(m->mic_conved[0]->data);
    if (m->mic_h) {
        mic_h_c = (float *)(m->mic_h->data);
    } else {
        mic_h_c = NULL;
    }

    cnt = 0;
    while (len >= step) {
        for (j = 0; j < channel; ++j) {
            pv[j] = (float *)(mic_conved[j]->data) + cnt;
        }
        // print_float(pv[0],128);
        // exit(0);
        ++m->nframe;
        if (echo_r || echo_c) {
            ret = qtk_ultm2_check_echo(m, echo_r ? echo_r + cnt : NULL,
                                       echo_c ? echo_c + cnt : NULL);
            if (ret != 0) {
                wtk_debug("echo data is failed at %f\n",
                          m->nx * 1.0 / m->cfg->rate);
                qtk_ultm2_reset_post(m);
                return;
            }
        }
        if (mic_r || mic_c || mic_h_c) {
            ret = qtk_ultm2_check_mic(m, mic_r ? mic_r + cnt : NULL,
                                      mic_c ? mic_c + cnt : NULL,
                                      mic_h_c ? mic_h_c + cnt : NULL);
            if (ret != 0) {
                wtk_debug("mic data is failed at %f\n",
                          m->nx * 1.0 / m->cfg->rate);
                qtk_ultm2_reset_post(m);
                return;
            }
        }
        f = wtk_float_sum2(pv[0] + L, L);
        if (m->last_mic_e > 0 && f / m->last_mic_e > m->cfg->mic_skip_f) {
            ++m->mic_err;
            wtk_debug("skip data is err %f %f r=%f at %f\n", m->last_mic_e, f,
                      f / m->last_mic_e, m->nx * 1.0 / m->cfg->rate);
            // wave_write_file_float("mic.wav",m->cfg->rate,pv[0],len);
            // exit(0);
            if (m->mic_err >= 10) {
                qtk_ultm2_reset_post(m);
                return;
            } else {
                cnt += step;
                len -= step;
                continue;
            }
        } else {
            m->mic_err = 0;
        }
        // wtk_debug("skip[%d] mic=%f\n",m->nframe,wtk_float_sum2(pv[0]+L,L));
        // wave_write_file_float("mic.wav",m->cfg->rate,pv[0],len);
        if (m->cfg->debug) {
            wtk_debug("v[%.1f]: echo=%f echo2=%f mic=%f harmonic=%f\n",
                      m->nx * 1.0 / m->cfg->rate, m->last_echo_e,
                      m->last_echo_e2, m->last_mic_e, m->last_hamonic_e);
        }
        ret = m->notify(m->upval, pv, step);
        if (ret != 0) {
            qtk_ultm2_reset_post(m);
            return;
        }
        cnt += step;
        len -= step;
    }

    if (cnt > 0) {
        qtk_ultm2_skip(m, cnt);
    }
}

void qtk_ultm2_update_phase2(qtk_ultm2_t *m) {
    int skip = m->skip;
    int channel = m->cfg->channel;
    int j;
    float **pv = m->pv;
    int len;
    int step = m->L;
    int cnt;
    wtk_strbuf_t **mic_conved;
    int ret;
    short *mic_r, *echo_r;
    float *mic_c, *echo_c;
    float *mic_h_c;

    mic_conved = m->mic_conved;
    len = mic_conved[0]->pos / sizeof(float);
    if (len < step) {
        return;
    }
    if (m->echo_conv) {
        echo_r = (short *)(m->echo_raw->data);
        echo_c = (float *)(m->echo_conved->data);
    } else {
        echo_r = NULL;
        echo_c = NULL;
    }
    mic_r = (short *)(m->mic_raw->data);
    mic_c = (float *)(m->mic_conved[0]->data);
    if (m->mic_h) {
        mic_h_c = (float *)(m->mic_h->data);
    } else {
        mic_h_c = NULL;
    }

    cnt = 0;
    while (len >= step) {
        for (j = 0; j < channel; ++j) {
            pv[j] = (float *)(mic_conved[j]->data) + cnt;
        }
        ++m->nframe;
        if (skip > 0) {
            ++m->skipped;
            if (m->skipped >= (skip + 1)) {
                m->skipped = 0;
            }
        } else {
            m->skipped = 1;
        }
        if (m->skipped == 1) {
            if (echo_r || echo_c) {
                ret = qtk_ultm2_check_echo(m, echo_r ? echo_r + cnt : NULL,
                                           echo_c ? echo_c + cnt : NULL);
                if (ret != 0) {
                    wtk_debug("echo data is failed at %f\n",
                              m->nx * 1.0 / m->cfg->rate);
                    qtk_ultm2_reset_post(m);
                    return;
                }
            }
            if (mic_r || mic_c || mic_h_c) {
                ret = qtk_ultm2_check_mic(m, mic_r ? mic_r + cnt : NULL,
                                          mic_c ? mic_c + cnt : NULL,
                                          mic_h_c ? mic_h_c + cnt : NULL);
                if (ret != 0) {
                    wtk_debug("mic data is failed at %f\n",
                              m->nx * 1.0 / m->cfg->rate);
                    qtk_ultm2_reset_post(m);
                    return;
                }
            }
            if (m->cfg->debug) {
                wtk_debug("v[%.1f]: echo=%f echo2=%f mic=%f harmonic=%f\n",
                          m->nx * 1.0 / m->cfg->rate, m->last_echo_e,
                          m->last_echo_e2, m->last_mic_e, m->last_hamonic_e);
            }
            // wtk_debug("ultm feed [%d]\n",m->nframe);
            ret = m->notify(m->upval, pv, step);
            if (ret != 0) {
                wtk_debug("feed ultmp failed at %f\n",
                          m->nx * 1.0 / m->cfg->rate);
                // exit(0);
                qtk_ultm2_reset_post(m);
                return;
            }
        }
        cnt += step;
        len -= step;
    }

    if (cnt > 0) {
        qtk_ultm2_skip(m, cnt);
    }
}

void qtk_ultm2_skip(qtk_ultm2_t *m, int n) {
    wtk_strbuf_t **phase_win = m->mic_conved;
    int i, nx;

    if (n <= 0) {
        return;
    }
    nx = n * sizeof(float);
    for (i = 0; i < m->cfg->channel; ++i) {
        wtk_strbuf_pop(phase_win[i], NULL, nx);
    }
    if (m->mic_h) {
        wtk_strbuf_pop(m->mic_h, NULL, nx);
    }
    if (m->mic_raw) {
        wtk_strbuf_pop(m->mic_raw, NULL, n * sizeof(short));
    }
    if (m->echo_conved) {
        wtk_strbuf_pop(m->echo_conved, NULL, nx);
        wtk_strbuf_pop(m->echo_raw, NULL, n * sizeof(short));
    }
}

void qtk_ultm2_seek_echo2(qtk_ultm2_t *m) {
    int skip = m->skip;
    int step = m->L;
    wtk_strbuf_t *mic = m->mic_conved[0];
    float *pv;
    int len = mic->pos / sizeof(float);
    int i;
    float f;
    int idx = 0;
    float maxf = -1;

    if (m->mic_conved[0]->pos < step * (skip + 1) * 2 * sizeof(float)) {
        return;
    }
    pv = (float *)(mic->data);
    len -= step;
    // len=min(len,step*2);
    for (i = 0; i < len; ++i) {
        f = wtk_float_sum2(pv + i, step);
        if (f > maxf) {
            maxf = f;
            idx = i;
        }
    }
    if (m->mic_conved[0]->pos / sizeof(float) - idx < step) {
        return;
    }

    // wtk_debug("idx=%d maxf=%f\n",idx,maxf);
    if (maxf > m->cfg->mic_min_f) {
        // wave_write_file_float("mic.wav",48000,(float*)(m->phase_win[0]->data),m->phase_win[0]->pos/sizeof(float));
        // wave_write_file_float("echo.wav",48000,(float*)(echo->data),echo->pos/sizeof(float));
        if (m->cfg->debug) {
            wtk_debug("ULTM seek succes idx=%d echo=%f at %f\n", idx, maxf,
                      m->nx * 1.0 / m->cfg->rate);
        }

        // exit(0);
        if (idx > 0) {
            qtk_ultm2_skip(m, idx);
        }
        if (m->mic_conved[0]->pos >= step * sizeof(float)) {
            m->last_mic_e =
                wtk_float_sum2((float *)(m->mic_conved[0]->data), step);
        } else {
            m->last_mic_e = 0;
        }
        if (m->cfg->debug) {
            wtk_debug("ULTM seek done idx=%d echo=%f mic=%f %ld at %f\n", idx,
                      m->last_echo_e, m->last_mic_e,
                      m->mic_conved[0]->pos / sizeof(float),
                      m->nx * 1.0 / m->cfg->rate);
        }
        m->state = QTK_ULTM2_ALIGND;
        // exit(0);
    } else {
        if (m->cfg->debug) {
            wtk_debug("ULTM seek failed idx=%d echo=%f at %f\n", idx, maxf,
                      m->nx * 1.0 / m->cfg->rate);
        }
        if (m->cfg->debug) {
            wtk_debug("ULTM echo start err %d %f len=%ld/%ld at %f\n", idx,
                      maxf, m->mic_conved[0]->pos / sizeof(float),
                      mic->pos / sizeof(float), m->nx * 1.0 / m->cfg->rate);
        }
        // wave_write_file_float("mic.wav",48000,(float*)(m->phase_win[0]->data),m->phase_win[0]->pos/sizeof(float));
        step = m->mic_conved[0]->pos / sizeof(float);
        // step=min(len,step);
        // exit(0);
        // exit(0);
        qtk_ultm2_skip(m, step);
        m->state = QTK_ULTM2_SEEK_START;
        // exit(0);
    }
}

void qtk_ultm2_seek_echo(qtk_ultm2_t *m) {
    int skip = m->skip;
    int step = m->L;
    wtk_strbuf_t *echo = m->echo_conved;
    float *pv;
    int len = echo->pos / sizeof(float);
    int i;
    float f;
    int idx = 0;
    float maxf = -1;

    // if(m->mic_conved[0]->pos<step*(skip+1)*2*sizeof(float))
    if (echo->pos < step * (skip + 1) * 2 * sizeof(float)) {
        return;
    }

    if(m->cfg->use_echo_align){
        int pos;
        float *pv1;
        float *pv2;
        pv = (float *)(echo->data);
        wtk_strbuf_reset(m->echo_align_conved[0]);
        wtk_strbuf_reset(m->echo_align_conved[1]);
        wtk_conv2_reset(m->echo_align_conv[0]);
        wtk_conv2_reset(m->echo_align_conv[1]);
        wtk_conv2_feed(m->echo_align_conv[0], pv, len, 0);
        wtk_conv2_feed(m->echo_align_conv[1], pv, len, 0);
        pos = m->echo_align_conved[0]->pos/sizeof(float);
        pv1 = (float *)m->echo_align_conved[0]->data;
        pv2 = (float *)m->echo_align_conved[1]->data;
        for(i=0;i<pos;++i){
            f = (pv1[i]*pv1[i] + pv2[i]*pv2[i]);
            if(f > maxf){
                maxf = f;
                idx = i;
            }
        }
        idx += step>>1;
        // wtk_strbuf_pop(m->echo_align_conved[0], NULL, pos);
        // wtk_strbuf_pop(m->echo_align_conved[1], NULL, pos);

        maxf = -1;
        len -= step;
        len = min(len, step * (skip + 1));
        for (i = 0; i < len; ++i) {
            f = wtk_float_sum2(pv + i, step);
            // wtk_debug("v[%d]: %f %d/%f\n",i,f,idx,maxf);
            if (f > maxf) {
                maxf = f;
            }
        }
        if (m->mic_conved[0]->pos / sizeof(float) - idx < step) {
            return;
        }
        if (maxf > m->cfg->echo_min_f) {
            while(idx > step){
                idx -= step;
            }
            if(m->align_idx->used < m->align_idx->nslot){
                wtk_fring_push2(m->align_idx, idx);
                qtk_ultm2_skip(m, step);
                m->state = QTK_ULTM2_SEEK_START;
                // wtk_fring_print(m->align_idx);
            }else{
                // wtk_fring_print(m->align_idx);
                wtk_debug("ULTM seek succes idx=%d echo=%f at %f %f\n", idx, maxf,
                        m->nx * 1.0 / m->cfg->rate, ((m->nx + idx) * 1.0/m->cfg->rate));
                if (idx > 0) {
                    qtk_ultm2_skip(m, idx);
                }
                m->last_echo_e = maxf;
                m->last_echo_e2 = wtk_short_sum2((short *)(m->echo_raw->data), step);
                if (m->mic_conved[0]->pos >= step * sizeof(float)) {
                    m->last_mic_e =
                        wtk_float_sum2((float *)(m->mic_conved[0]->data), step);
                } else {
                    m->last_mic_e = 0;
                }
                m->state = QTK_ULTM2_ALIGND;
            }
        }else{
            step = min(m->echo_conved->pos / sizeof(float),
                    m->mic_conved[0]->pos / sizeof(float));
            // step=min(len,step);
            // exit(0);
            // exit(0);
            qtk_ultm2_skip(m, step);
            m->state = QTK_ULTM2_SEEK_START;
        }
        

    }else{
        pv = (float *)(echo->data);
        len -= step;
        // wtk_debug("len=%d step=%d\n",len,step);
        len = min(len, step * (skip + 1));
        for (i = 0; i < len; ++i) {
            f = wtk_float_sum2(pv + i, step);
            // wtk_debug("v[%d]: %f %d/%f\n",i,f,idx,maxf);
            if (f > maxf) {
                maxf = f;
                idx = i;
            }
        }
        if (m->mic_conved[0]->pos / sizeof(float) - idx < step) {
            return;
        }
        // wtk_debug("ULTM seek succes idx=%d echo=%f at %f\n", idx, maxf,
        //           m->nx * 1.0 / m->cfg->rate);
        //	if(m->nx*1.0/48000>100)
        //	{
        //		exit(0);
        //	}
        if (maxf > m->cfg->echo_min_f) {
            // wave_write_file_float("mic.wav",48000,(float*)(m->phase_win[0]->data),m->phase_win[0]->pos/sizeof(float));
            // wave_write_file_float("echo.wav",48000,(float*)(echo->data),echo->pos/sizeof(float));
            if (m->cfg->debug) {
                wtk_debug("ULTM seek succes idx=%d echo=%f at %f\n", idx, maxf,
                        m->nx * 1.0 / m->cfg->rate);
            }
            if (idx > 0) {
                qtk_ultm2_skip(m, idx);
            }
            // wave_write_file_float("mic.wav",48000,(float*)(m->mic_conved[0]->data),m->mic_conved[0]->pos/sizeof(float));
            // wave_write_file_float("echo.wav",48000,(float*)(echo->data),echo->pos/sizeof(float));
            // wtk_debug("idx=%d maxf=%f\n",idx,maxf);
            // exit(0);
            m->last_echo_e = maxf;
            m->last_echo_e2 = wtk_short_sum2((short *)(m->echo_raw->data), step);
            if (m->mic_conved[0]->pos >= step * sizeof(float)) {
                m->last_mic_e =
                    wtk_float_sum2((float *)(m->mic_conved[0]->data), step);
            } else {
                m->last_mic_e = 0;
            }
            if (m->cfg->debug) {
                wtk_debug("ULTM seek done idx=%d echo=%f mic=%f %ld at %f\n", idx,
                        m->last_echo_e, m->last_mic_e,
                        m->mic_conved[0]->pos / sizeof(float),
                        m->nx * 1.0 / m->cfg->rate);
            }
            m->state = QTK_ULTM2_ALIGND;
            // exit(0);
        } else {
            if (m->cfg->debug) {
                wtk_debug("ULTM seek failed idx=%d echo=%f at %f\n", idx, maxf,
                        m->nx * 1.0 / m->cfg->rate);
            }
            if (m->cfg->debug) {
                wtk_debug("ULTM echo start err %d %f len=%ld/%ld at %f\n", idx,
                        maxf, m->mic_conved[0]->pos / sizeof(float),
                        echo->pos / sizeof(float), m->nx * 1.0 / m->cfg->rate);
            }
            // wave_write_file_float("mic.wav",48000,(float*)(m->phase_win[0]->data),m->phase_win[0]->pos/sizeof(float));
            step = min(m->echo_conved->pos / sizeof(float),
                    m->mic_conved[0]->pos / sizeof(float));
            // step=min(len,step);
            // exit(0);
            // exit(0);
            qtk_ultm2_skip(m, step);
            m->state = QTK_ULTM2_SEEK_START;
            // exit(0);
        }
        //	{
        //		static int ki=0;
        //
        //		++ki;
        //		if(ki>5)
        //		{
        //			exit(0);
        //		}
        //	}
    }
}

void qtk_ultm2_align_echo_mic(qtk_ultm2_t *m) {
    int skip = m->skip;
    int step1 = m->L;
    int step = step1 * (skip + 1) * 2 * sizeof(float);
    wtk_strbuf_t *buf = m->mic_conved[0];
    wtk_strbuf_t *echo = m->echo_conved;
    int idx;
    float f, f2;

    if (echo->pos < step || buf->pos < step) {
        return;
    }
    f = wtk_float_sum2((float *)(echo->data) + step1,
                       step / sizeof(float) - step1);
    f2 = wtk_float_sum2((float *)(buf->data) + step1,
                        step / sizeof(float) - step1);
    // wtk_debug("f=%f pos=%ld
    // %d/%d\n",f,buf->pos/sizeof(float),step1,step-step1);
    if (f < m->cfg->echo_min_f || f2 < m->cfg->mic_min_f) {
        idx = min(echo->pos, buf->pos) / sizeof(float) - m->L;
        if (idx > 0) {
            qtk_ultm2_skip(m, idx);
        }
        return;
    }
    // wtk_debug("f=%f f2=%f echo=%ld mic=%ld step=%ld\n", f, f2,
    //           echo->pos / sizeof(float), buf->pos / sizeof(float),
    //           step / sizeof(float));
    // print_float((float*)(buf->data)+step1,step-step1);
    // 对齐数据
    step = step / sizeof(float);
    // wtk_debug("mic0=%d echo=%d\n",buf->pos,echo->pos);
    idx =
        wtk_rfft_xcorr_float2((float *)(buf->data) + step1, step - step1,
                              (float *)(echo->data) + step1, step - step1, &f);
    // idx=wtk_rfft_xcorr_float2((float*)(echo->data)+step1,step-step1,(float*)(buf->data)+step1,step-step1,&f);
    // f=0;
    // idx=wtk_rfft_xcorr_float((float*)(echo->data)+step1,step-step1,(float*)(buf->data)+step1,step-step1);
    if (m->cfg->debug) {
        wtk_debug("align idx=%d f=%f at %f\n", idx, f,
                  m->nx * 1.0 / m->cfg->rate);
    }
    wtk_debug("align idx=%d f=%f at %f\n", idx, f, m->nx * 1.0 / m->cfg->rate);
    if (idx > 0) {
        wtk_strbuf_push_front(echo, NULL, idx * sizeof(float));
        wtk_strbuf_push_front(m->echo_raw, NULL, idx * sizeof(short));
        // wtk_strbuf_pop(echo,NULL,idx*sizeof(float));
    } else if (idx < 0) {
        int i;

        wtk_strbuf_push_front(m->mic_raw, NULL, -idx * sizeof(short));
        for (i = 0; i < m->cfg->channel; ++i) {
            wtk_strbuf_push_front(m->mic_conved[i], NULL, -idx * sizeof(float));
        }
        // wtk_strbuf_pop_front(echo,NULL,-idx*sizeof(float));
        // wtk_strbuf_push(echo,NULL,-idx*sizeof(float));
    }
    // wtk_debug("ULTM align idx=%d maxf=%f at
    // %f\n",idx,f,m->nx*1.0/m->cfg->rate);
    m->state = QTK_ULTM2_SEEK_START;
    qtk_ultm2_skip(m, step1 * (skip));
    // wtk_strbuf_pop(buf,NULL,step);
    // int wtk_strbuf_pop(wtk_strbuf_t *s,char* data,int bytes);
    // wave_write_file_float("mic.wav",48000,(float*)(buf->data),buf->pos/sizeof(float));
    // wave_write_file_float("echo.wav",48000,(float*)(echo->data),echo->pos/sizeof(float));
    // exit(0);
    if (m->echo_conved->pos > step1 * (skip + 1) * 2 * sizeof(float)) {
        qtk_ultm2_seek_echo(m);
    }
    // exit(0);
}

void qtk_ultm2_update_phase(qtk_ultm2_t *m) {
    int skip = m->skip;
    int use_skip_dat = 1;

    // wtk_debug("============== use phse %d =======\n",m->mic_conved[0]->pos);
    // exit(0);
    // wtk_debug("ULTM1 state=%d at %f
    // mic=%ld\n",m->state,m->nx*1.0/m->cfg->rate,m->phase_win[0]->pos/sizeof(float));
    switch (m->state) {
    case QTK_ULTM2_INIT:
        qtk_ultm2_align_echo_mic(m);
        break;
    case QTK_ULTM2_SEEK_START:
        if (m->echo_conv) {
            if (m->echo_conved->pos > m->L * (skip + 1) * sizeof(float)) {
                qtk_ultm2_seek_echo(m);
            }
        } else if (m->mic_conved[0]->pos > m->L * (skip + 1) * sizeof(float)) {
            qtk_ultm2_seek_echo2(m);
        }
        break;
    case QTK_ULTM2_ALIGND:
        // qtk_ultm2_update_phase2(m);
        // qtk_ultm2_check_echo(m);
        break;
    }
    // wtk_debug("ULTM2 state=%d at %f
    // mic=%ld\n",m->state,m->nx*1.0/m->cfg->rate,m->phase_win[0]->pos/sizeof(float));
    if (m->state == QTK_ULTM2_ALIGND) {
        if (use_skip_dat) {
            qtk_ultm2_update_phase3(m);
        } else {
            qtk_ultm2_update_phase2(m);
        }
    }
}

void qtk_ultm2_feed2_conv(qtk_ultm2_t *m, char **data, int len, int is_end) {
    int j;
    int channel = m->cfg->channel;
    float *fv;
    short *pv[10];
    int i;
    int step_bytes = m->L * sizeof(float);

    // short char 问题
    if (len <= 0) {
        if (is_end) {
            for (j = 0; j < channel; ++j) {
                wtk_conv2_feed(m->conv2[j], NULL, 0, is_end);
            }
            if (m->mic_h_conv) {
                wtk_conv2_feed(m->mic_h_conv, NULL, 0, is_end);
            }
            if (m->mic_conved[0]->pos >= step_bytes) {
                qtk_ultm2_update_phase(m);
            }
            // wtk_debug("pos=%d\n",m->phase_win->pos);
        }
    } else {
        int step = m->cfg->s2f_size;
        int n1;
        float fx = m->cfg->scale; // 1.0/32768;
        short *ppv;
        // float ps=m->mp->pad_scale;

        // fx=fx*m->mp->scale;
        //		if(ps>0)
        //		{
        //			fx/=ps;
        //		}
        // wtk_debug("scale=%f/%f\n",m->mp->scale,fx);
        // 用于检查录音数据是否截幅
        wtk_strbuf_push(m->mic_raw, data[0], len);
        len = len >> 1;
        fv = m->pv_tmp;
        // fv=(float*)wtk_malloc(sizeof(float)*len);
        for (i = 0; i < channel; ++i) {
            pv[i] = (short *)(data[i]);
        }
        // wtk_debug("channel=%d\n",channel);
        // exit(0);
        while (len > 0) {
            n1 = min(step, len);
            for (j = 0; j < channel; ++j) {
                ppv = pv[j];
                {
                    for (i = 0; i < n1; ++i) {
                        //					wtk_debug("v[%d/%d/%d]=%d/%d\n",j,i,n1,step,len);
                        //					wtk_debug("v[%d/%d/%d]=%d/%f\n",j,i,n1,ppv[i],fv[i]);
                        fv[i] = ppv[i] * fx;
                        // wtk_debug("v[%d]: %d/%f\n",i,ppv[i],fv[i]);
                        // exit(0);
                    }
                }
                // print_float(fv,n1);
                // exit(0);
                if (m->mic_h_conv && j == 0) {
                    wtk_conv2_feed(m->mic_h_conv, fv, n1, 0);
                }
                wtk_conv2_feed(m->conv2[j], fv, n1, 0);
                pv[j] += n1;
            }
            len -= n1;
        }
        if (is_end) {
            for (j = 0; j < channel; ++j) {
                wtk_conv2_feed(m->conv2[j], NULL, 0, is_end);
            }
            if (m->mic_h_conv) {
                wtk_conv2_feed(m->mic_h_conv, NULL, 0, is_end);
            }
        }
        if (m->mic_conved[0]->pos >= step_bytes) {
            qtk_ultm2_update_phase(m);
        }
        // wtk_free(fv);
    }
    // exit(0);
}

void qtk_ultm2_set_cut(qtk_ultm2_t *m, int cut) { m->cuted = cut; }

void qtk_ultm2_feed(qtk_ultm2_t *m, char **data, int len, int is_end, int nc) {
    int i;
    int xlen;
    short *ppv;
    int n1;
    int step;
    int ei;

    xlen = len / 2;
    m->nx += xlen;
    // wtk_debug("feed %.1f echo=%d
    // e=%f\n",m->nx*1.0/48000,m->echoed,qtk_ultm2_get_e(m));
    if (m->cfg->use_echo) {
        float fx = m->cfg->scale; // 1.0/32768;
        float *fv;

        ei = nc - 1;
        step = m->cfg->s2f_size;
        if (len > 0) {
            wtk_strbuf_push(m->echo_raw, data[ei], len);
        }
        xlen = len;
        // wtk_debug("len=%d end=%d\n",len,is_end);
        if (len > 0) {
            len = len >> 1;
            fv = m->pv_tmp;
            ppv = (short *)(data[ei]);
            while (len > 0) {
                n1 = min(step, len);
                for (i = 0; i < n1; ++i) {
                    fv[i] = ppv[i] * fx;
                }
                wtk_conv2_feed(m->echo_conv, fv, n1, 0);
                len -= n1;
                ppv += n1;
            }
        }
        if (is_end) {
            wtk_conv2_feed(m->echo_conv, NULL, 0, 1);
        }
        qtk_ultm2_feed2_conv(m, data, xlen, is_end);
        // wtk_debug("len=%d\n",m->phase_win[0]->pos);
    } else {
        qtk_ultm2_feed2_conv(m, data, len, is_end);
    }
}

void qtk_ultm2_set_notify(qtk_ultm2_t *m, qtk_ultm2_notify_f notify,
                          void *upval) {
    m->notify = notify;
    m->upval = upval;
}
