#include "wtk_kalman.h"

static float _complex_abs_mean(wtk_complex_t *a, int n) {
    float sum = 0.0;
    int i;
    for (i = 0; i < n; ++i) {
        sum += sqrt(a[i].a * a[i].a + a[i].b * a[i].b);
    }
    return sum / n;
}

wtk_kalman_t *wtk_kalman_new(wtk_kalman_cfg_t *cfg) {
    wtk_kalman_t *kalman = (wtk_kalman_t *)malloc(sizeof(wtk_kalman_t));

    kalman->cfg = cfg;
    kalman->ths = NULL;
    kalman->notify = NULL;
    kalman->drft = wtk_drft_new2(cfg->wins);
    kalman->x_b = (float *)wtk_malloc(sizeof(float) * cfg->wins);
    kalman->d_n = wtk_float_new_p2(cfg->nmicchannel, cfg->L);
    kalman->y_n = wtk_float_new_p2(cfg->nmicchannel, cfg->L);
    kalman->e_n = wtk_float_new_p2(cfg->nmicchannel, cfg->L);
    kalman->P_b = (float *)wtk_malloc(sizeof(float) * cfg->nbin * cfg->B);
    kalman->Phi_SS = (float *)wtk_malloc(sizeof(float) * cfg->nbin);
    kalman->Phi_delta = (float *)wtk_malloc(sizeof(float) * cfg->nbin * cfg->B);
    kalman->mu_b = (float *)wtk_malloc(sizeof(float) * cfg->nbin * cfg->B);
    kalman->half_window = (float *)wtk_malloc(sizeof(float) * cfg->wins);
    kalman->power_block = (float *)wtk_malloc(sizeof(float) * cfg->B);
    kalman->X_b =
        (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * cfg->nbin * cfg->B);
    kalman->K_b =
        (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * cfg->nbin * cfg->B);
    kalman->W_b = wtk_complex_new_p2(cfg->nmicchannel, cfg->nbin * cfg->B);
    kalman->E = wtk_complex_new_p2(cfg->nmicchannel, cfg->nbin);
    kalman->fft_tmp =
        (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * cfg->nbin * cfg->B);
    kalman->tmp = (float *)wtk_malloc(sizeof(float) * cfg->nbin * cfg->B);
    kalman->rfft_in = (float *)wtk_malloc(sizeof(float) * cfg->wins);

    wtk_kalman_reset(kalman);
    return kalman;
}
void wtk_kalman_delete(wtk_kalman_t *kalman) {
    int nmicchannel = kalman->cfg->nmicchannel;

    wtk_drft_delete2(kalman->drft);
    wtk_free(kalman->x_b);
    wtk_float_delete_p2(kalman->d_n, nmicchannel);
    wtk_float_delete_p2(kalman->y_n, nmicchannel);
    wtk_float_delete_p2(kalman->e_n, nmicchannel);
    wtk_free(kalman->P_b);
    wtk_free(kalman->Phi_SS);
    wtk_free(kalman->Phi_delta);
    wtk_free(kalman->mu_b);
    wtk_free(kalman->half_window);
    wtk_free(kalman->power_block);
    wtk_free(kalman->X_b);
    wtk_free(kalman->K_b);
    wtk_complex_delete_p2(kalman->W_b, nmicchannel);
    wtk_complex_delete_p2(kalman->E, nmicchannel);
    wtk_free(kalman->fft_tmp);
    wtk_free(kalman->tmp);
    wtk_free(kalman->rfft_in);
    wtk_free(kalman);
}

void wtk_kalman_reset(wtk_kalman_t *kalman) {
    int nmicchannel = kalman->cfg->nmicchannel;
    int wins = kalman->cfg->wins;
    int nbin = kalman->cfg->nbin;
    int B = kalman->cfg->B;
    int L = kalman->cfg->L;
    float P_init = kalman->cfg->P_init;
    float phi_ss_init = kalman->cfg->phi_ss_init;
    float phi_delta_init = kalman->cfg->phi_delta_init;
    int i;

    memset(kalman->x_b, 0, sizeof(float) * wins);
    wtk_float_zero_p2(kalman->d_n, nmicchannel, L);
    wtk_float_zero_p2(kalman->y_n, nmicchannel, L);
    wtk_float_zero_p2(kalman->e_n, nmicchannel, L);
    for (i = 0; i < nbin * B; ++i) {
        kalman->P_b[i] = P_init;
    }
    for (i = 0; i < nbin; ++i) {
        kalman->Phi_SS[i] = phi_ss_init;
    }
    for (i = 0; i < nbin * B; ++i) {
        kalman->Phi_delta[i] = phi_delta_init;
    }
    memset(kalman->mu_b, 0, sizeof(float) * nbin * B);
    memset(kalman->half_window, 0, sizeof(float) * wins);
    for (i = 0; i < L; ++i) {
        kalman->half_window[i] = 1.0;
    }
    memset(kalman->power_block, 0, sizeof(float) * B);
    memset(kalman->X_b, 0, sizeof(wtk_complex_t) * nbin * B);
    memset(kalman->K_b, 0, sizeof(wtk_complex_t) * nbin * B);
    wtk_complex_zero_p2(kalman->W_b, nmicchannel, nbin * B);
    wtk_complex_zero_p2(kalman->E, nmicchannel, nbin);
    memset(kalman->fft_tmp, 0, sizeof(wtk_complex_t) * nbin * B);
    memset(kalman->tmp, 0, nbin * B);
    memset(kalman->rfft_in, 0, wins);

    kalman->frame_idx = 0;
}

void wtk_kalman_set_notify(wtk_kalman_t *kalman, void *ths,
                           wtk_kalman_notify_f notify) {
    kalman->ths = ths;
    kalman->notify = notify;
}

void wtk_kalman_feed(wtk_kalman_t *kalman, float **d, float *x) {
    wtk_drft_t *drft = kalman->drft;
    wtk_complex_t *fft_tmp = kalman->fft_tmp;
    float *tmp = kalman->tmp;
    float *rfft_in = kalman->rfft_in;
    float fv;
    int idx;

    float *x_b = kalman->x_b;
    float **y_n = kalman->y_n;
    float **e_n = kalman->e_n;
    float *Phi_SS = kalman->Phi_SS;
    float *P_b = kalman->P_b;
    float *mu_b = kalman->mu_b;
    // float *half_window = kalman->half_window;
    float *Phi_EE;
    float *Phi_delta_raw;
    float *Phi_delta = kalman->Phi_delta;
    float *W;

    wtk_complex_t *x_b_curr;
    wtk_complex_t *X_b = kalman->X_b, *Xtmp;
    wtk_complex_t *Y, *Ytmp;
    wtk_complex_t **W_b = kalman->W_b, *Wtmp;
    wtk_complex_t **E = kalman->E, *Etmp;
    wtk_complex_t *K_b = kalman->K_b, *Ktmp;
    wtk_complex_t *W_acc;
    wtk_complex_t *E_res;

    int nmicchannel = kalman->cfg->nmicchannel;
    int wins = kalman->cfg->wins;
    int nbin = kalman->cfg->nbin;
    int B = kalman->cfg->B;
    int L = kalman->cfg->L;
    float alpha = kalman->cfg->alpha;
    float alpha_1 = 1.0 - alpha;
    float beta = kalman->cfg->beta;
    float beta_1 = 1.0 - beta;
    float update_thresh = kalman->cfg->update_thresh;
    float A = kalman->cfg->A;
    float A_pow = powf(A, 2);
    float A_pow_1 = 1.0 - A_pow;
    float clip_thresh = kalman->cfg->clip_thresh;
    int i, j, k;

    memcpy(x_b, x_b + L, sizeof(float) * L);
    memcpy(x_b + L, x, sizeof(float) * L);
    x_b_curr = fft_tmp;
    wtk_drft_fft23(drft, x_b, x_b_curr);

    memmove(X_b + nbin, X_b, sizeof(wtk_complex_t) * (B - 1) * nbin);
    memcpy(X_b, x_b_curr, sizeof(wtk_complex_t) * nbin);

    kalman->frame_idx++;
    if (kalman->frame_idx == B) {
        kalman->frame_idx = 0;
    }
    kalman->power_block[kalman->frame_idx] = _complex_abs_mean(x_b_curr, nbin);

    for (i = 0; i < nmicchannel; ++i) {
        Y = fft_tmp;
        memset(Y, 0, sizeof(wtk_complex_t) * nbin);
        Xtmp = X_b;
        Wtmp = W_b[i];
        for (j = 0; j < B; ++j) {
            Ytmp = Y;
            for (k = 0; k < nbin; ++k, ++Ytmp, ++Xtmp, ++Wtmp) {
                Ytmp->a += Wtmp->a * Xtmp->a - Wtmp->b * Xtmp->b;
                Ytmp->b += Wtmp->a * Xtmp->b + Wtmp->b * Xtmp->a;
            }
        }
        wtk_drft_ifft23(drft, Y, rfft_in);
        memcpy(y_n[i], rfft_in + L, sizeof(float) * L);
        for (k = 0; k < L; ++k) {
            e_n[i][k] = d[i][k] - y_n[i][k];
        }
        memset(rfft_in, 0, sizeof(float) * L);
        memcpy(rfft_in + L, e_n[i], sizeof(float) * L);
        wtk_drft_fft23(drft, rfft_in, E[i]);
    }

    memset(tmp, 0, sizeof(float) * nbin);
    for (k = 0; k < nbin; ++k) {
        for (i = 0; i < nmicchannel; ++i) {
            tmp[k] += E[i][k].a * E[i][k].a + E[i][k].b * E[i][k].b;
        }
        tmp[k] /= nmicchannel;
    }

    for (k = 0; k < nbin; ++k) {
        Phi_SS[k] = alpha * Phi_SS[k] + alpha_1 * tmp[k];
    }

    if (wtk_float_max(kalman->power_block, B) > update_thresh) {
        memset(tmp, 0, sizeof(float) * nbin);
        Xtmp = X_b;
        for (i = 0; i < B; ++i) {
            for (k = 0; k < nbin; ++k, ++Xtmp) {
                tmp[k] += (Xtmp->a * Xtmp->a + Xtmp->b * Xtmp->b) * P_b[k];
            }
        }
        Phi_EE = tmp;
        for (k = 0; k < nbin; ++k) {
            Phi_EE[k] = 0.5 * tmp[k] + Phi_SS[k];
        }

        idx = 0;
        Ktmp = K_b;
        Xtmp = X_b;
        for (i = 0; i < B; ++i) {
            for (k = 0; k < nbin; ++k, ++idx, ++Ktmp, ++Xtmp) {
                mu_b[idx] = 0.5 * P_b[idx] / Phi_EE[k];
                Ktmp->a = mu_b[idx] * Xtmp->a;
                Ktmp->b = -mu_b[idx] * Xtmp->b;
            }
        }

        for (i = 0; i < nmicchannel; ++i) {
            Ktmp = K_b;
            Wtmp = W_b[i];
            for (j = 0; j < B; ++j) {
                Etmp = E[i];
                for (k = 0; k < nbin; ++k, ++Ktmp, ++Etmp) {
                    fft_tmp[k].a = Ktmp->a * Etmp->a - Ktmp->b * Etmp->b;
                    fft_tmp[k].b = Ktmp->a * Etmp->b + Ktmp->b * Etmp->a;
                }
                wtk_drft_ifft23(drft, fft_tmp, rfft_in);
                // for (k = 0; k < wins; ++k) {
                //     rfft_in[k] *= half_window[k];
                // }
                memset(rfft_in + L, 0, sizeof(float) * L);
                W_acc = fft_tmp;
                wtk_drft_fft23(drft, rfft_in, W_acc);
                for (k = 0; k < nbin; ++k, ++Wtmp, ++W_acc) {
                    Wtmp->a += W_acc->a;
                    Wtmp->b += W_acc->b;
                }
            }
        }

        if (kalman->cfg->use_sec_iter) {
            for (i = 0; i < nmicchannel; ++i) {
                Y = fft_tmp;
                memset(Y, 0, sizeof(wtk_complex_t) * nbin);
                Xtmp = X_b;
                Wtmp = W_b[i];
                for (j = 0; j < B; ++j) {
                    Ytmp = Y;
                    for (k = 0; k < nbin; ++k, ++Ytmp, ++Xtmp, ++Wtmp) {
                        Ytmp->a += Wtmp->a * Xtmp->a - Wtmp->b * Xtmp->b;
                        Ytmp->b += Wtmp->a * Xtmp->b + Wtmp->b * Xtmp->a;
                    }
                }
                wtk_drft_ifft23(drft, Y, rfft_in);
                memcpy(y_n[i], rfft_in + L, sizeof(float) * L);
                for (k = 0; k < L; ++k) {
                    e_n[i][k] = d[i][k] - y_n[i][k];
                }
            }
        }

        Phi_delta_raw = tmp;
        memset(Phi_delta_raw, 0, sizeof(float) * nbin * B);
        idx = 0;
        for (i = 0; i < B; ++i) {
            for (k = 0; k < nbin; ++k, ++idx) {
                Phi_delta_raw[idx] = 0;
                for (j = 0; j < nmicchannel; ++j) {
                    Phi_delta_raw[idx] += W_b[j][idx].a * W_b[j][idx].a +
                                          W_b[j][idx].b * W_b[j][idx].b;
                }
                Phi_delta_raw[idx] /= nmicchannel;
                Phi_delta_raw[idx] *= A_pow_1;
                Phi_delta[idx] =
                    beta * Phi_delta[idx] + beta_1 * Phi_delta_raw[idx];
            }
        }

        idx = 0;
        Ktmp = K_b;
        Xtmp = X_b;
        for (i = 0; i < B; ++i) {
            for (k = 0; k < nbin; ++k, ++idx, ++Ktmp, ++Xtmp) {
                P_b[idx] =
                    A_pow *
                        (1 - 0.5 * (Ktmp->a * Xtmp->a - Ktmp->b * Xtmp->b)) *
                        P_b[idx] +
                    Phi_delta[idx];
                if (P_b[idx] < clip_thresh) {
                    P_b[idx] = clip_thresh;
                }
            }
        }
        if (kalman->cfg->use_res) {
            W = tmp;
            memset(W, 0, sizeof(float) * nbin);

            idx = 0;
            Xtmp = X_b;
            for (i = 0; i < B; ++i) {
                for (k = 0; k < nbin; ++k, ++idx, ++Xtmp) {
                    W[k] += mu_b[idx] * (Xtmp->a * Xtmp->a + Xtmp->b * Xtmp->b);
                }
            }
            for (k = 0; k < nbin; ++k) {
                W[k] = 1.0 - W[k];
            }

            for (i = 0; i < nmicchannel; ++i) {
                memset(rfft_in, 0, sizeof(float) * L);
                memcpy(rfft_in + L, e_n[i], sizeof(float) * L);
                wtk_drft_fft23(drft, rfft_in, E[i]);
                Etmp = E[i];
                E_res = fft_tmp;
                for (k = 0; k < nbin; ++k, ++E_res, ++Etmp, ++Wtmp) {
                    E_res->a = W[k] * Etmp->a;
                    E_res->b = W[k] * Etmp->b;
                }
                wtk_drft_ifft23(drft, fft_tmp, rfft_in);
                memcpy(e_n[i], rfft_in + L, sizeof(float) * L);
                for (k = 0; k < L; ++k) {
                    y_n[i][k] += d[i][k] - e_n[i][k];
                }
            }
        }
    }

    if (kalman->notify) {
        kalman->notify(kalman->ths, e_n, nmicchannel, L);
    }
}