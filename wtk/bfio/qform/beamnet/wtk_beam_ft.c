#include "wtk_beam_ft.h"
#ifndef WTK_WAV_SHORT_TO_FLOAT
#define WTK_WAV_SHORT_TO_FLOAT(f) ((f) > 0 ? (f / 32767.0) : (f / 32768.0))
#endif
#ifndef WTK_WAV_FLOAT_TO_SHORT
#define WTK_WAV_FLOAT_TO_SHORT(f)                                              \
    ((f) > 0 ? floorf(f * 32767.0 + 0.5) : floorf(f * 32768.0 + 0.5))
#endif

void wtk_beam_ft_compute_multi_channel_phase_shift(wtk_beam_ft_t *beam_ft,
                                                   wtk_complex_t **phase_shift,
                                                   float theta, float phi) {
    int wins = beam_ft->cfg->wins;
    int channel = beam_ft->cfg->channel;
    int nbin = beam_ft->cfg->nbin;
    int rate = beam_ft->cfg->rate;
    float sv = beam_ft->cfg->sv;
    float **mic_pos = beam_ft->cfg->mic_pos;
    float *time_delay = beam_ft->time_delay;
    float *mic;
    float x, y, z;
    int i, k;
    float t;

    theta -= 180.0;
    theta = theta < -180.0 ? theta + 360.0 : theta;
    theta = theta > 180.0 ? theta - 360.0 : theta;

    theta = theta * PI / 180.0;
    phi = phi * PI / 180.0;
    x = cos(phi) * cos(theta);
    y = cos(phi) * sin(theta);
    z = sin(phi);
    x = fabs(x) < 1e-7 ? 0 : x;
    y = fabs(y) < 1e-7 ? 0 : y;
    z = fabs(z) < 1e-7 ? 0 : z;

    for (i = 0; i < channel; ++i) {
        mic = mic_pos[i];
        time_delay[i] = (mic[0] * x + mic[1] * y + mic[2] * z) / sv;
    }
    for (k = 0; k < nbin; ++k) {
        t = 2 * PI * rate * 1.0 / wins * k;
        for (i = 0; i < channel; ++i) {
            phase_shift[i][k].a = cos(t * time_delay[i]);
            phase_shift[i][k].b = -sin(t * time_delay[i]);
        }
    }
}

void wtk_beam_ft_compute_channel_covariance(wtk_beam_ft_t *beam_ft,
                                            wtk_complex_t **fft,
                                            wtk_complex_t **covar_mat,
                                            int norm) {
    int channel = beam_ft->cfg->channel;
    int nbin = beam_ft->cfg->nbin;
    int i, j, k;
    wtk_complex_t *mean_mat = beam_ft->mean_mat;
    wtk_complex_t **input_norm = beam_ft->input_norm;
    int idx;

    if (norm == 1) {
        memset(mean_mat, 0, nbin * sizeof(wtk_complex_t));
        for (k = 0; k < nbin; ++k) {
            for (i = 0; i < channel; ++i) {
                mean_mat[k].a += fft[i][k].a;
                mean_mat[k].b += fft[i][k].b;
            }
            mean_mat[k].a *= 1.0 / channel;
            mean_mat[k].b *= 1.0 / channel;
            for (i = 0; i < channel; ++i) {
                input_norm[i][k].a = fft[i][k].a - mean_mat[k].a;
                input_norm[i][k].b = fft[i][k].b - mean_mat[k].b;
            }
        }
    } else {
        for (i = 0; i < channel; ++i) {
            memcpy(input_norm[i], fft[i], nbin * sizeof(wtk_complex_t));
        }
    }

    for (k = 0; k < nbin; ++k) {
        idx = 0;
        for (i = 1; i < channel; ++i) {
            for (j = 0; j < i; ++j) {
                covar_mat[idx][k].a = input_norm[i][k].a * input_norm[j][k].a +
                                      input_norm[i][k].b * input_norm[j][k].b;
                covar_mat[idx][k].b = input_norm[i][k].b * input_norm[j][k].a -
                                      input_norm[i][k].a * input_norm[j][k].b;
                idx++;
            }
        }
    }
}

void wtk_beam_ft_compute_feature(wtk_beam_ft_t *beam_ft, wtk_complex_t **fft) {
    wtk_complex_t **mic_covar = beam_ft->mic_covar;
    wtk_complex_t ***ideal_phase_covar = beam_ft->ideal_phase_covar;
    wtk_complex_t **freq_covar = beam_ft->freq_covar;
    float **freq_covar_sum_a = beam_ft->freq_covar_sum_a;
    float **freq_covar_sum_b = beam_ft->freq_covar_sum_b;
    float *x_mag = beam_ft->x_mag;
    float *csa = beam_ft->csa;
    float *csb = beam_ft->csb;
    float *csna = beam_ft->csna;
    float *csnb = beam_ft->csnb;
    int ntheta = beam_ft->ntheta;
    int *region_mask = beam_ft->region_mask;
    wtk_complex_t *a, *b, *c;
    float *aa, *bb;
    int out_channels = beam_ft->cfg->out_channels;
    int nbin = beam_ft->cfg->nbin;
    float eps = 1e-12;
    float tmp;
    int i, k, n;
    int cs_state = 0;
    int csn_state = 0;

    a = fft[0];
    for (k = 0; k < nbin; ++k, ++a) {
        x_mag[k] = sqrtf(a->a * a->a + a->b * a->b);
        x_mag[k] = max(x_mag[k], eps);
    }
    wtk_beam_ft_compute_channel_covariance(beam_ft, fft, mic_covar, 0);
    for (i = 0; i < out_channels; ++i) {
        a = mic_covar[i];
        for (k = 0; k < nbin; ++k, a++) {
            tmp = 1.0 / (sqrtf(a->a * a->a + a->b * a->b) + eps);
            a->a *= tmp;
            a->b *= tmp;
        }
    }
    if (ntheta == 1) {
        for (i = 0; i < out_channels; ++i) {
            a = mic_covar[i];
            b = ideal_phase_covar[0][i];
            c = freq_covar[i];
            for (k = 0; k < nbin; ++k, ++a, ++b, ++c) {
                c->a = a->a * b->a + a->b * b->b;
                c->b = a->b * b->a - a->a * b->b;
            }
        }
        memset(csa, 0, sizeof(float) * nbin);
        memset(csb, 0, sizeof(float) * nbin);
        for (i = 0; i < out_channels; ++i) {
            a = freq_covar[i];
            for (k = 0; k < nbin; ++k, ++a) {
                csa[k] += a->a;
                csb[k] += a->b;
            }
        }
    } else {
        for (n = 0; n < ntheta; ++n) {
            for (i = 0; i < out_channels; ++i) {
                a = mic_covar[i];
                b = ideal_phase_covar[n][i];
                c = freq_covar[i];
                for (k = 0; k < nbin; ++k, ++a, ++b, ++c) {
                    c->a = a->a * b->a + a->b * b->b;
                    c->b = a->b * b->a - a->a * b->b;
                }
            }
            aa = freq_covar_sum_a[n];
            bb = freq_covar_sum_b[n];
            memset(aa, 0, sizeof(float) * nbin);
            memset(bb, 0, sizeof(float) * nbin);
            for (i = 0; i < out_channels; ++i) {
                a = freq_covar[i];
                for (k = 0; k < nbin; ++k, ++a) {
                    aa[k] += a->a;
                    bb[k] += a->b;
                }
            }
        }
        if (beam_ft->cfg->use_csn) {
            memset(csa, 0, sizeof(float) * nbin);
            memset(csb, 0, sizeof(float) * nbin);
            memset(csna, 0, sizeof(float) * nbin);
            memset(csnb, 0, sizeof(float) * nbin);
            for (n = 0; n < ntheta; ++n) {
                if (region_mask[n] == 1) {
                    aa = freq_covar_sum_a[n];
                    bb = freq_covar_sum_b[n];
                    if (cs_state == 0) {
                        for (k = 0; k < nbin; ++k) {
                            csa[k] = aa[k];
                            csb[k] = bb[k];
                        }
                        cs_state = 1;
                    } else {
                        for (k = 0; k < nbin; ++k) {
                            csa[k] = max(csa[k], aa[k]);
                            csb[k] = max(csb[k], bb[k]);
                        }
                    }
                } else {
                    aa = freq_covar_sum_a[n];
                    bb = freq_covar_sum_b[n];
                    if (csn_state == 0) {
                        for (k = 0; k < nbin; ++k) {
                            csna[k] = aa[k];
                            csnb[k] = bb[k];
                        }
                        csn_state = 1;
                    } else {
                        for (k = 0; k < nbin; ++k) {
                            csna[k] = max(csna[k], aa[k]);
                            csnb[k] = max(csnb[k], bb[k]);
                        }
                    }
                }
            }
        } else {
            memset(csa, 0, sizeof(float) * nbin);
            memset(csb, 0, sizeof(float) * nbin);
            for (n = 0; n < ntheta; ++n) {
                aa = freq_covar_sum_a[n];
                bb = freq_covar_sum_b[n];
                if (cs_state == 0) {
                    for (k = 0; k < nbin; ++k) {
                        csa[k] = aa[k];
                        csb[k] = bb[k];
                    }
                    cs_state = 1;
                } else {
                    for (k = 0; k < nbin; ++k) {
                        csa[k] = max(csa[k], aa[k]);
                        csb[k] = max(csb[k], bb[k]);
                    }
                }
            }
        }
    }
}

wtk_beam_ft_t *wtk_beam_ft_new(wtk_beam_ft_cfg_t *cfg) {
    int i;
    wtk_beam_ft_t *beam_ft;

    beam_ft = (wtk_beam_ft_t *)wtk_malloc(sizeof(wtk_beam_ft_t));
    beam_ft->cfg = cfg;

    beam_ft->time_delay = (float *)wtk_malloc(sizeof(float) * cfg->channel);
    beam_ft->mean_mat =
        (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * cfg->nbin);
    beam_ft->input_norm = wtk_complex_new_p2(cfg->channel, cfg->nbin);
    beam_ft->mic_covar = wtk_complex_new_p2(cfg->out_channels, cfg->nbin);
    beam_ft->ideal_phase_covar = NULL;
    beam_ft->freq_covar = wtk_complex_new_p2(cfg->out_channels, cfg->nbin);
    beam_ft->ideal_phase_shift = wtk_complex_new_p2(cfg->channel, cfg->nbin);
    beam_ft->freq_covar_sum_a = NULL;
    beam_ft->freq_covar_sum_b = NULL;
    beam_ft->region_mask = NULL;

    beam_ft->x_mag = (float *)wtk_malloc(sizeof(float) * cfg->nbin);
    beam_ft->csa = (float *)wtk_malloc(sizeof(float) * cfg->nbin);
    beam_ft->csb = (float *)wtk_malloc(sizeof(float) * cfg->nbin);
    beam_ft->csna = NULL;
    beam_ft->csnb = NULL;
    if (cfg->use_csn) {
        beam_ft->csna = (float *)wtk_malloc(sizeof(float) * cfg->nbin);
        beam_ft->csnb = (float *)wtk_malloc(sizeof(float) * cfg->nbin);
    }
    wtk_beam_ft_reset(beam_ft);
    return beam_ft;
}
void wtk_beam_ft_delete(wtk_beam_ft_t *beam_ft) {
    int channel = beam_ft->cfg->channel;
    int out_channels = beam_ft->cfg->out_channels;

    wtk_free(beam_ft->time_delay);
    wtk_free(beam_ft->mean_mat);
    wtk_complex_delete_p2(beam_ft->input_norm, channel);
    wtk_complex_delete_p2(beam_ft->mic_covar, out_channels);
    wtk_complex_delete_p3(beam_ft->ideal_phase_covar, beam_ft->ntheta,
                          out_channels);
    wtk_complex_delete_p2(beam_ft->freq_covar, out_channels);
    wtk_complex_delete_p2(beam_ft->ideal_phase_shift, channel);
    if (beam_ft->freq_covar_sum_a) {
        wtk_float_delete_p2(beam_ft->freq_covar_sum_a, beam_ft->ntheta);
    }
    if (beam_ft->freq_covar_sum_b) {
        wtk_float_delete_p2(beam_ft->freq_covar_sum_b, beam_ft->ntheta);
    }
    if (beam_ft->region_mask) {
        wtk_free(beam_ft->region_mask);
    }
    wtk_free(beam_ft->x_mag);
    wtk_free(beam_ft->csa);
    wtk_free(beam_ft->csb);
    if (beam_ft->csna) {
        wtk_free(beam_ft->csna);
    }
    if (beam_ft->csnb) {
        wtk_free(beam_ft->csnb);
    }
    wtk_free(beam_ft);
}

void wtk_beam_ft_theta_reset(wtk_beam_ft_t *beam_ft) {
    if(beam_ft->cfg->use_csn) {
        if (beam_ft->region_mask) {
            wtk_free(beam_ft->region_mask);
        }
    }
    if (beam_ft->ideal_phase_covar) {
        wtk_complex_delete_p3(beam_ft->ideal_phase_covar, beam_ft->ntheta,
                              beam_ft->cfg->out_channels);
    }
    if (beam_ft->freq_covar_sum_a) {
        wtk_float_delete_p2(beam_ft->freq_covar_sum_a, beam_ft->ntheta);
    }
    if (beam_ft->freq_covar_sum_b) {
        wtk_float_delete_p2(beam_ft->freq_covar_sum_b, beam_ft->ntheta);
    }
}

void wtk_beam_ft_start(wtk_beam_ft_t *beam_ft, float theta, float theta2,
                       float phi) {
    wtk_complex_t **ideal_phase_shift = beam_ft->ideal_phase_shift;
    wtk_complex_t ***ideal_phase_covar;
    float theta_step = beam_ft->cfg->theta_step;
    float min_theta = beam_ft->cfg->min_theta;
    float max_theta = beam_ft->cfg->max_theta;
    int n_theta = beam_ft->cfg->n_theta;
    int out_channels = beam_ft->cfg->out_channels;
    int nbin = beam_ft->cfg->nbin;
    int i;
    float n;

    wtk_beam_ft_theta_reset(beam_ft);

    while (theta >= 360) {
        theta -= 360;
    }
    while (theta < 0) {
        theta += 360;
    }
    while (theta2 >= 360) {
        theta2 -= 360;
    }
    while (theta2 < 0) {
        theta2 += 360;
    }
    if (theta2 < theta) {
        theta -= 360;
    }

    beam_ft->theta = theta;
    beam_ft->theta2 = theta2;
    beam_ft->ntheta = (int)((theta2 - theta) / theta_step) + 1;
    if (beam_ft->cfg->use_csn) {
        beam_ft->ntheta = n_theta;
        beam_ft->region_mask = (int *)wtk_malloc(sizeof(int) * n_theta);
        memset(beam_ft->region_mask, 0, sizeof(int) * n_theta);
    }

    beam_ft->ideal_phase_covar =
        wtk_complex_new_p3(beam_ft->ntheta, out_channels, nbin);
    ideal_phase_covar = beam_ft->ideal_phase_covar;
    wtk_complex_zero_p3(ideal_phase_covar, beam_ft->ntheta, out_channels, nbin);
    beam_ft->freq_covar_sum_a = wtk_float_new_p2(beam_ft->ntheta, nbin);
    wtk_float_zero_p2(beam_ft->freq_covar_sum_a, beam_ft->ntheta, nbin);
    beam_ft->freq_covar_sum_b = wtk_float_new_p2(beam_ft->ntheta, nbin);
    wtk_float_zero_p2(beam_ft->freq_covar_sum_b, beam_ft->ntheta, nbin);

    if (beam_ft->cfg->use_csn) {
        for (n = min_theta, i = 0; n <= max_theta; n += theta_step, ++i) {
            if (n >= theta && n <= theta2) {
                beam_ft->region_mask[i] = 1;
            }
            wtk_beam_ft_compute_multi_channel_phase_shift(
                beam_ft, ideal_phase_shift, n, 0);
            wtk_beam_ft_compute_channel_covariance(beam_ft, ideal_phase_shift,
                                                   ideal_phase_covar[i], 0);
            // printf("%d %f\n", beam_ft->region_mask[i], n);
        }
    } else {
        i = 0;
        for (n = min_theta; n <= max_theta; n += theta_step) {
            if (n >= theta && n <= theta2) {
                wtk_beam_ft_compute_multi_channel_phase_shift(
                    beam_ft, ideal_phase_shift, n, 0);
                wtk_beam_ft_compute_channel_covariance(beam_ft, ideal_phase_shift,
                                                    ideal_phase_covar[i], 0);
                // printf("%f\n", n);
                ++i;
            }
        }
        beam_ft->ntheta = i;
        // for (n = theta, i = 0; n < theta2; n += theta_step, ++i) {
        //     wtk_beam_ft_compute_multi_channel_phase_shift(
        //         beam_ft, ideal_phase_shift, n, 0);
        //     wtk_beam_ft_compute_channel_covariance(beam_ft, ideal_phase_shift,
        //                                            ideal_phase_covar[i], 0);
        // }
    }
}

void wtk_beam_ft_reset(wtk_beam_ft_t *beam_ft) {
    int i, nbin = beam_ft->cfg->nbin;
    int channel = beam_ft->cfg->channel;
    int out_channels = beam_ft->cfg->out_channels;

    memset(beam_ft->time_delay, 0, sizeof(float) * channel);
    memset(beam_ft->mean_mat, 0, sizeof(wtk_complex_t) * nbin);
    wtk_complex_zero_p2(beam_ft->input_norm, channel, nbin);
    wtk_complex_zero_p2(beam_ft->mic_covar, out_channels, nbin);
    wtk_complex_zero_p2(beam_ft->freq_covar, out_channels, nbin);
    wtk_complex_zero_p2(beam_ft->ideal_phase_shift, channel, nbin);

    memset(beam_ft->x_mag, 0, sizeof(float) * nbin);
    memset(beam_ft->csa, 0, sizeof(float) * nbin);
    memset(beam_ft->csb, 0, sizeof(float) * nbin);
    if (beam_ft->csna) {
        memset(beam_ft->csna, 0, sizeof(float) * nbin);
    }
    if (beam_ft->csnb) {
        memset(beam_ft->csnb, 0, sizeof(float) * nbin);
    }
    beam_ft->theta = 90;
    beam_ft->theta2 = 110;
}

// fft [channel, nbin]
// 输出为 x_mag, csa, csb 如果开启了csn 增加输出为 csna, csnb
void wtk_beam_ft_feed(wtk_beam_ft_t *beam_ft, wtk_complex_t **fft, int channel,
                      int nbin) {
    if (channel != beam_ft->cfg->channel || nbin != beam_ft->cfg->nbin) {
        wtk_debug("channel or nbin not match\n");
        return;
    }
    wtk_beam_ft_compute_feature(beam_ft, fft);
}
