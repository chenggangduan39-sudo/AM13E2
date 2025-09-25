#include "wtk/bfio/qform/beamnet/wtk_beam_bf_net.h"
#include "qtk/core/qtk_mem.h"
#include "qtk/math/qtk_vector.h"
#include "qtk/nnrt/qtk_nnrt.h"
#include "qtk/nnrt/qtk_nnrt_value.h"
#ifndef WTK_WAV_SHORT_TO_FLOAT
#define WTK_WAV_SHORT_TO_FLOAT(f) ((f) > 0 ? (f / 32767.0) : (f / 32768.0))
#endif
#ifndef WTK_WAV_FLOAT_TO_SHORT
#define WTK_WAV_FLOAT_TO_SHORT(f)                                              \
    ((f) > 0 ? floorf(f * 32767.0 + 0.5) : floorf(f * 32768.0 + 0.5))
#endif

void wtk_beam_bf_net_compute_multi_channel_phase_shift(
    wtk_beam_bf_net_t *beam_bf_net, wtk_complex_t **phase_shift, float theta,
    float phi) {
    int wins = beam_bf_net->cfg->wins;
    int nmicchannel = beam_bf_net->cfg->nmicchannel;
    int nbin = beam_bf_net->cfg->nbin;
    int rate = beam_bf_net->cfg->rate;
    float sv = beam_bf_net->cfg->sv;
    float **mic_pos = beam_bf_net->cfg->mic_pos;
    float *time_delay = beam_bf_net->time_delay;
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

    for (i = 0; i < nmicchannel; ++i) {
        mic = mic_pos[i];
        time_delay[i] = (mic[0] * x + mic[1] * y + mic[2] * z) / sv;
    }
    for (k = 0; k < nbin; ++k) {
        t = 2 * PI * rate * 1.0 / wins * k;
        for (i = 0; i < nmicchannel; ++i) {
            phase_shift[i][k].a = cos(t * time_delay[i]);
            phase_shift[i][k].b = -sin(t * time_delay[i]);
        }
    }
}

void wtk_beam_bf_net_compute_channel_covariance(wtk_beam_bf_net_t *beam_bf_net,
                                                wtk_complex_t **fft,
                                                wtk_complex_t **covar_mat,
                                                int norm) {
    int nmicchannel = beam_bf_net->cfg->nmicchannel;
    int nbin = beam_bf_net->cfg->nbin;
    int i, j, k;
    wtk_complex_t *mean_mat = beam_bf_net->mean_mat;
    wtk_complex_t **input_norm = beam_bf_net->input_norm;
    int idx;

    if (norm == 1) {
        memset(mean_mat, 0, nbin * sizeof(wtk_complex_t));
        for (k = 0; k < nbin; ++k) {
            for (i = 0; i < nmicchannel; ++i) {
                mean_mat[k].a += fft[i][k].a;
                mean_mat[k].b += fft[i][k].b;
            }
            mean_mat[k].a *= 1.0 / nmicchannel;
            mean_mat[k].b *= 1.0 / nmicchannel;
            for (i = 0; i < nmicchannel; ++i) {
                input_norm[i][k].a = fft[i][k].a - mean_mat[k].a;
                input_norm[i][k].b = fft[i][k].b - mean_mat[k].b;
            }
        }
    } else {
        for (i = 0; i < nmicchannel; ++i) {
            memcpy(input_norm[i], fft[i], nbin * sizeof(wtk_complex_t));
        }
    }

    for (k = 0; k < nbin; ++k) {
        idx = 0;
        for (i = 1; i < nmicchannel; ++i) {
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

void wtk_beam_bf_net_compute_feature(wtk_beam_bf_net_t *beam_bf_net) {
    wtk_complex_t **fft = beam_bf_net->fft;
    wtk_complex_t **mic_covar = beam_bf_net->mic_covar;
    wtk_complex_t **ideal_phase_covar = beam_bf_net->ideal_phase_covar;
    wtk_complex_t **freq_covar = beam_bf_net->freq_covar;
    float *mag = beam_bf_net->mag;
    float *cs = beam_bf_net->cs;
    wtk_complex_t *a, *b, *c;
    int out_channels = beam_bf_net->cfg->out_channels;
    int nbin = beam_bf_net->cfg->nbin;
    float eps = 1e-12;
    float tmp;
    int i, k;

    a = fft[0];
    for (k = 0; k < nbin; ++k, ++a) {
        mag[k] = sqrtf(a->a * a->a + a->b * a->b);
    }
    wtk_beam_bf_net_compute_channel_covariance(beam_bf_net, fft, mic_covar, 0);
    for (i = 0; i < out_channels; ++i) {
        a = mic_covar[i];
        for (k = 0; k < nbin; ++k, a++) {
            tmp = 1.0 / (sqrtf(a->a * a->a + a->b * a->b) + eps);
            a->a *= tmp;
            a->b *= tmp;
        }
    }
    for (i = 0; i < out_channels; ++i) {
        a = mic_covar[i];
        b = ideal_phase_covar[i];
        c = freq_covar[i];
        for (k = 0; k < nbin; ++k, ++a, ++b, ++c) {
            c->a = a->a * b->a + a->b * b->b;
        }
    }
    memset(cs, 0, sizeof(float) * nbin);
    for (i = 0; i < out_channels; ++i) {
        a = freq_covar[i];
        for (k = 0; k < nbin; ++k, ++a) {
            cs[k] += a->a;
        }
    }
}

wtk_beam_bf_net_t *wtk_beam_bf_net_new(wtk_beam_bf_net_cfg_t *cfg) {
    int i;
    wtk_beam_bf_net_t *beam_bf_net;

    beam_bf_net = (wtk_beam_bf_net_t *)wtk_malloc(sizeof(wtk_beam_bf_net_t));
    beam_bf_net->cfg = cfg;
    beam_bf_net->ths = NULL;
    beam_bf_net->notify = NULL;
    beam_bf_net->mic = wtk_strbufs_new(beam_bf_net->cfg->nmicchannel);
    beam_bf_net->sp = wtk_strbufs_new(beam_bf_net->cfg->nspchannel);

    if (cfg->use_pffft) {
        beam_bf_net->rfft = wtk_drft_new2(cfg->wins);
    } else {
        beam_bf_net->rfft = wtk_drft_new(cfg->wins);
    }
    beam_bf_net->rfft_in = (float *)wtk_malloc(sizeof(float) * cfg->wins);
    beam_bf_net->analysis_window = wtk_malloc(sizeof(float) * cfg->wins);
    beam_bf_net->synthesis_window = wtk_malloc(sizeof(float) * cfg->wins);
    beam_bf_net->analysis_mem =
        wtk_float_new_p2(cfg->nmicchannel, beam_bf_net->cfg->nbin - 1);
    beam_bf_net->analysis_mem_sp =
        wtk_float_new_p2(cfg->nspchannel, beam_bf_net->cfg->nbin - 1);
    beam_bf_net->synthesis_mem =
        wtk_malloc(sizeof(float) * (beam_bf_net->cfg->nbin - 1));

    beam_bf_net->fft = wtk_complex_new_p2(
        cfg->nmicchannel, beam_bf_net->cfg->nbin * cfg->num_frame);
    beam_bf_net->fft_sp = wtk_complex_new_p2(
        max(1, cfg->nspchannel), beam_bf_net->cfg->nbin * cfg->num_frame);

    beam_bf_net->mask_bf = NULL;
    if (cfg->use_mask_bf) {
        beam_bf_net->mask_bf = wtk_mask_bf_new(&(cfg->mask_bf));
    }

    beam_bf_net->fftx = (wtk_complex_t *)wtk_malloc(
        sizeof(wtk_complex_t) * beam_bf_net->cfg->nbin * cfg->num_frame);

    beam_bf_net->qmmse2 = NULL;
    if (cfg->use_qmmse2) {
        beam_bf_net->qmmse2 = wtk_qmmse_new(&(cfg->qmmse2));
    }

    beam_bf_net->gc_mask = (float *)wtk_malloc(sizeof(float) * cfg->num_frame);
    beam_bf_net->gc = NULL;
    if (cfg->use_gc) {
        beam_bf_net->gc = qtk_gain_controller_new(&(cfg->gc));
        qtk_gain_controller_set_mode(beam_bf_net->gc, 0);
        beam_bf_net->gc->kalman.Z_k = cfg->gc_gain;
    }

    beam_bf_net->eq = NULL;
    if (cfg->use_eq) {
        beam_bf_net->eq = wtk_equalizer_new(&(cfg->eq));
    }
    beam_bf_net->limiter = NULL;
    if (cfg->use_limiter) {
        beam_bf_net->limiter = wtk_limiter_new(&(cfg->limiter));
    }

    beam_bf_net->out = wtk_malloc(sizeof(float) * (beam_bf_net->cfg->nbin - 1));

    beam_bf_net->stage1_rt = NULL;
    if (cfg->use_stage1_rt) {
        beam_bf_net->stage1_rt = qtk_nnrt_new(&cfg->stage1_rt);
        beam_bf_net->stage1_inputs = wtk_malloc(sizeof(qtk_nnrt_value_t) *
                                                beam_bf_net->stage1_rt->num_in);
        for (i = 0; i < beam_bf_net->stage1_rt->num_in; i++) {
            beam_bf_net->stage1_inputs[i] =
                qtk_nnrt_create_input(beam_bf_net->stage1_rt, i);
        }
    }
    if (cfg->use_qnn) {
        int i, j;
        int ndim, nelem;
        if (cfg->qnn1_buf) {
            qtk_mem_t mem;
            qtk_mem_init(&mem, (unsigned char *)cfg->qnn1_buf->data,
                         cfg->qnn1_buf->pos);
            qtk_nn_vm_load(&beam_bf_net->nv1, (qtk_io_reader)qtk_mem_read,
                           &mem);
            beam_bf_net->nv1_num_in = 30; //// TODO: fix this
            qtk_nn_vm_get_input(&beam_bf_net->nv1, &beam_bf_net->nv1_num_in,
                                (void **)beam_bf_net->nv1_input);
            beam_bf_net->nv1_in_sizes = (uint32_t *)wtk_malloc(
                sizeof(uint32_t) * beam_bf_net->nv1_num_in);
            beam_bf_net->nv1_out_sizes = NULL;
            for (i = 0; i < beam_bf_net->nv1_num_in; i++) {
                uint32_t *shape =
                    qtk_nn_vm_get_input_shape(&beam_bf_net->nv1, i, &ndim);
                nelem = 1;
                for (j = 0; j < ndim; j++) {
                    nelem *= shape[j];
                }
                beam_bf_net->nv1_in_sizes[i] = nelem * sizeof(float);
                memset(beam_bf_net->nv1_input[i], 0,
                       beam_bf_net->nv1_in_sizes[i]);
            }
        }
    }

    beam_bf_net->bs_win = NULL;
    if (cfg->use_bs_win) {
        beam_bf_net->bs_win = wtk_math_create_hanning_window2(cfg->wins / 2);
    }
    beam_bf_net->sp_state = (int *)wtk_malloc(sizeof(int) * cfg->num_frame);

    beam_bf_net->time_delay =
        (float *)wtk_malloc(sizeof(float) * cfg->nmicchannel);
    beam_bf_net->mean_mat =
        (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * cfg->nbin);
    beam_bf_net->input_norm = wtk_complex_new_p2(cfg->nmicchannel, cfg->nbin);
    beam_bf_net->mic_covar = wtk_complex_new_p2(cfg->out_channels, cfg->nbin);
    beam_bf_net->ideal_phase_covar =
        wtk_complex_new_p2(cfg->out_channels, cfg->nbin);
    beam_bf_net->freq_covar = wtk_complex_new_p2(cfg->out_channels, cfg->nbin);
    beam_bf_net->ideal_phase_shift =
        wtk_complex_new_p2(cfg->nmicchannel, cfg->nbin);

    beam_bf_net->mag =
        (float *)wtk_malloc(sizeof(float) * cfg->nbin * cfg->num_frame);
    beam_bf_net->cs =
        (float *)wtk_malloc(sizeof(float) * cfg->nbin * cfg->num_frame);
    beam_bf_net->mask =
        (float *)wtk_malloc(sizeof(float) * cfg->nbin * cfg->num_frame);
    wtk_beam_bf_net_reset(beam_bf_net);
    return beam_bf_net;
}
void wtk_beam_bf_net_delete(wtk_beam_bf_net_t *beam_bf_net) {
    int nmicchannel = beam_bf_net->cfg->nmicchannel;
    int nspchannel = beam_bf_net->cfg->nspchannel;
    int out_channels = beam_bf_net->cfg->out_channels;
    int nbin = beam_bf_net->cfg->nbin;

    wtk_strbufs_delete(beam_bf_net->mic, nmicchannel);
    wtk_strbufs_delete(beam_bf_net->sp, nspchannel);

    if (beam_bf_net->cfg->use_pffft) {
        wtk_drft_delete2(beam_bf_net->rfft);
    } else {
        wtk_drft_delete(beam_bf_net->rfft);
    }
    wtk_free(beam_bf_net->rfft_in);
    wtk_free(beam_bf_net->analysis_window);
    wtk_free(beam_bf_net->synthesis_window);
    wtk_float_delete_p2(beam_bf_net->analysis_mem, nmicchannel);
    wtk_float_delete_p2(beam_bf_net->analysis_mem_sp, nspchannel);
    wtk_free(beam_bf_net->synthesis_mem);
    wtk_complex_delete_p2(beam_bf_net->fft, nmicchannel);
    wtk_complex_delete_p2(beam_bf_net->fft_sp, max(1, nspchannel));
    wtk_free(beam_bf_net->fftx);

    if (beam_bf_net->mask_bf) {
        wtk_mask_bf_delete(beam_bf_net->mask_bf);
    }

    if (beam_bf_net->qmmse2) {
        wtk_qmmse_delete(beam_bf_net->qmmse2);
    }

    if (beam_bf_net->gc) {
        qtk_gain_controller_delete(beam_bf_net->gc);
    }
    if (beam_bf_net->gc_mask) {
        wtk_free(beam_bf_net->gc_mask);
    }

    if (beam_bf_net->eq) {
        wtk_equalizer_delete(beam_bf_net->eq);
    }
    if (beam_bf_net->limiter) {
        wtk_limiter_delete(beam_bf_net->limiter);
    }

    wtk_free(beam_bf_net->out);
    if (beam_bf_net->cfg->use_stage1_rt) {
        int i;
        for (i = 0; i < beam_bf_net->stage1_rt->num_in; i++) {
            qtk_nnrt_value_release(beam_bf_net->stage1_rt,
                                   beam_bf_net->stage1_inputs[i]);
        }
        wtk_free(beam_bf_net->stage1_inputs);
        qtk_nnrt_delete(beam_bf_net->stage1_rt);
    }

    if (beam_bf_net->bs_win) {
        wtk_free(beam_bf_net->bs_win);
    }
    wtk_free(beam_bf_net->sp_state);

    if (beam_bf_net->cfg->use_qnn) {
        int i;
        int outer_in_num = 2;

        if (beam_bf_net->cfg->qnn1_buf) {
            qtk_nn_vm_clean(&beam_bf_net->nv1);
            wtk_free(beam_bf_net->nv1_in_sizes);
            if (beam_bf_net->nv1_out_sizes) {
                wtk_free(beam_bf_net->nv1_out_sizes);
                for (i = outer_in_num; i < beam_bf_net->nv1_num_in; i++) {
                    wtk_free(beam_bf_net->nv1_cache[i - outer_in_num + 1]);
                }
            }
        }
    }

    wtk_free(beam_bf_net->time_delay);
    wtk_free(beam_bf_net->mean_mat);
    wtk_complex_delete_p2(beam_bf_net->input_norm, nmicchannel);
    wtk_complex_delete_p2(beam_bf_net->mic_covar, out_channels);
    wtk_complex_delete_p2(beam_bf_net->ideal_phase_covar, out_channels);
    wtk_complex_delete_p2(beam_bf_net->freq_covar, out_channels);
    wtk_complex_delete_p2(beam_bf_net->ideal_phase_shift, nmicchannel);
    wtk_free(beam_bf_net->mag);
    wtk_free(beam_bf_net->cs);
    wtk_free(beam_bf_net->mask);
    
    wtk_free(beam_bf_net);
}

void wtk_beam_bf_net_start(wtk_beam_bf_net_t *beam_bf_net, float theta,
                           float phi) {
    wtk_complex_t **ideal_phase_shift = beam_bf_net->ideal_phase_shift;
    wtk_complex_t **ideal_phase_covar = beam_bf_net->ideal_phase_covar;
    beam_bf_net->theta = theta;
    wtk_beam_bf_net_compute_multi_channel_phase_shift(
        beam_bf_net, ideal_phase_shift, theta, 0);
    wtk_beam_bf_net_compute_channel_covariance(beam_bf_net, ideal_phase_shift,
                                               ideal_phase_covar, 0);
}
void wtk_beam_bf_net_reset(wtk_beam_bf_net_t *beam_bf_net) {
    int wins = beam_bf_net->cfg->wins;
    int fsize = wins / 2;
    int i, nbin = beam_bf_net->cfg->nbin;
    int num_frame = beam_bf_net->cfg->num_frame;
    int nmicchannel = beam_bf_net->cfg->nmicchannel;
    int nbfchannel = beam_bf_net->cfg->nbfchannel;
    int nspchannel = beam_bf_net->cfg->nspchannel;
    int out_channels = beam_bf_net->cfg->out_channels;

    wtk_strbufs_reset(beam_bf_net->mic, nmicchannel);
    wtk_strbufs_reset(beam_bf_net->sp, nspchannel);

    if (num_frame > 1) {
        wtk_strbufs_push_float(beam_bf_net->mic, nmicchannel, NULL,
                               (num_frame - 1) * fsize);
        wtk_strbufs_push_float(beam_bf_net->sp, nspchannel, NULL,
                               (num_frame - 1) * fsize);
    }

    memset(beam_bf_net->rfft_in, 0, sizeof(float) * wins);
    for (i = 0; i < wins; ++i) {
        beam_bf_net->analysis_window[i] = sin((0.5 + i) * PI / (wins));
    }
    // wtk_math_init_blackman_window(beam_bf_net->analysis_window, wins);
    wtk_drft_init_synthesis_window(beam_bf_net->synthesis_window,
                                   beam_bf_net->analysis_window, wins);

    wtk_float_zero_p2(beam_bf_net->analysis_mem, nmicchannel, (nbin - 1));
    wtk_float_zero_p2(beam_bf_net->analysis_mem_sp, nspchannel, (nbin - 1));
    memset(beam_bf_net->synthesis_mem, 0, sizeof(float) * (nbin - 1));

    wtk_complex_zero_p2(beam_bf_net->fft, nmicchannel, nbin * num_frame);
    wtk_complex_zero_p2(beam_bf_net->fft_sp, max(1, nspchannel),
                        nbin * num_frame);
    memset(beam_bf_net->fftx, 0, sizeof(wtk_complex_t) * nbin * num_frame);

    if (beam_bf_net->mask_bf) {
        wtk_mask_bf_reset(beam_bf_net->mask_bf);
    }

    if (beam_bf_net->qmmse2) {
        wtk_qmmse_reset(beam_bf_net->qmmse2);
    }

    if (beam_bf_net->gc) {
        qtk_gain_controller_reset(beam_bf_net->gc);
    }
    if (beam_bf_net->gc_mask) {
        memset(beam_bf_net->gc_mask, 0, sizeof(float) * num_frame);
    }
    memset(beam_bf_net->out, 0, sizeof(float) * (nbin - 1));
    if (beam_bf_net->limiter) {
        wtk_limiter_reset(beam_bf_net->limiter);
    }

    if (beam_bf_net->cfg->use_stage1_rt) {
        qtk_nnrt_reset(beam_bf_net->stage1_rt);
        for (i = 0; i < beam_bf_net->stage1_rt->num_in; i++) {
            void *data_ptr = qtk_nnrt_value_get_data(
                beam_bf_net->stage1_rt, beam_bf_net->stage1_inputs[i]);
            size_t nbytes =
                qtk_nnrt_get_input_nbytes(beam_bf_net->stage1_rt, i);
            memset(data_ptr, 0, nbytes);
        }
    }

    for (i = 0; i < beam_bf_net->cfg->num_frame; i++) {
        beam_bf_net->sp_state[i] = 1;
    }
    memset(beam_bf_net->time_delay, 0, sizeof(float) * nmicchannel);
    memset(beam_bf_net->mean_mat, 0, sizeof(wtk_complex_t) * nbin);
    wtk_complex_zero_p2(beam_bf_net->input_norm, nmicchannel, nbin);
    wtk_complex_zero_p2(beam_bf_net->mic_covar, out_channels, nbin);
    wtk_complex_zero_p2(beam_bf_net->ideal_phase_covar, out_channels, nbin);
    wtk_complex_zero_p2(beam_bf_net->freq_covar, out_channels, nbin);
    wtk_complex_zero_p2(beam_bf_net->ideal_phase_shift, nmicchannel, nbin);

    memset(beam_bf_net->mag, 0, sizeof(float) * nbin * num_frame);
    memset(beam_bf_net->cs, 0, sizeof(float) * nbin * num_frame);
    memset(beam_bf_net->mask, 0, sizeof(float) * nbin * num_frame);

    beam_bf_net->sp_silcnt = 0;
    beam_bf_net->sp_sil = 1;
    beam_bf_net->mic_silcnt = 0;
    beam_bf_net->mic_sil = 1;

    beam_bf_net->bs_scale = 1.0;
    beam_bf_net->bs_last_scale = 1.0;
    beam_bf_net->bs_real_scale = 1.0;
    beam_bf_net->bs_max_cnt = 0;

    beam_bf_net->nframe = 0;
    beam_bf_net->gc_cnt = 0;

    beam_bf_net->bf_start = 0;
    beam_bf_net->sum_sp_sil = 0;

    beam_bf_net->mic_scale = beam_bf_net->cfg->mic_scale;
    beam_bf_net->sp_scale = beam_bf_net->cfg->sp_scale;

    beam_bf_net->agc_enable = 1;
    beam_bf_net->echo_enable = 1;
    beam_bf_net->denoise_enable = 1;

    if (beam_bf_net->cfg->use_qnn) {
        beam_bf_net->nv1_idx = 0;
    }
    beam_bf_net->theta = 90;
}
void wtk_beam_bf_net_set_notify(wtk_beam_bf_net_t *beam_bf_net, void *ths,
                                wtk_beam_bf_net_notify_f notify) {
    beam_bf_net->notify = notify;
    beam_bf_net->ths = ths;
}

static float wtk_beam_bf_net_sp_energy(float *p, int n) {
    float f, f2;
    int i;

    f = 0;
    for (i = 0; i < n; ++i) {
        f += p[i] * 32768.0;
    }
    f /= n;

    f2 = 0;
    for (i = 0; i < n; ++i) {
        f2 += (p[i] * 32768.0 - f) * (p[i] * 32768.0 - f);
    }
    f2 /= n;

    return f2;
}

static float wtk_beam_bf_net_fft_energy(wtk_complex_t *fftx, int nbin) {
    return qtk_vector_cpx_mag_squared_sum(fftx + 1, nbin - 2);
}

void wtk_beam_bf_net_feed_qnn(wtk_beam_bf_net_t *beam_bf_net) {
    int i, j;
    float *mag = beam_bf_net->mag;
    float *cs = beam_bf_net->cs;
    float *mask = beam_bf_net->mask;
    qtk_nn_vm_t *nv = &beam_bf_net->nv1;
    int outer_in_num = 2;
    int outer_out_num = 1;
    int idx = 0;

    int nout = 30, ndim, nelem;

    if (beam_bf_net->nv1_idx != 0) {
        qtk_nn_vm_get_input(nv, &beam_bf_net->nv1_num_in,
                            (void **)beam_bf_net->nv1_input);
        for (i = outer_in_num; i < beam_bf_net->nv1_num_in; i++) {
            memcpy(
                beam_bf_net->nv1_input[i],
                beam_bf_net->nv1_cache[i - outer_in_num + outer_out_num],
                beam_bf_net->nv1_out_sizes[i - outer_in_num + outer_out_num]);
        }
    }
    memcpy(beam_bf_net->nv1_input[idx], mag, beam_bf_net->nv1_in_sizes[idx]);
    idx++;
    memcpy(beam_bf_net->nv1_input[idx], cs, beam_bf_net->nv1_in_sizes[idx]);
    idx++;

    qtk_nn_vm_run(nv);
    qtk_nn_vm_get_output(nv, &nout, (void **)beam_bf_net->nv1_output);

    if (!beam_bf_net->nv1_out_sizes) {
        beam_bf_net->nv1_out_sizes = wtk_malloc(sizeof(int) * nout);
        for (i = 0; i < nout; i++) {
            nelem = 1;
            uint32_t *shape = qtk_nn_vm_get_output_shape(nv, i, &ndim);
            for (j = 0; j < ndim; j++) {
                nelem *= shape[j];
            }
            beam_bf_net->nv1_out_sizes[i] = nelem * sizeof(float);
            if (i > 0) {
                beam_bf_net->nv1_cache[i] =
                    wtk_malloc(beam_bf_net->nv1_out_sizes[i]);
            }
        }
    }
    idx = 0;
    memcpy(mask, beam_bf_net->nv1_output[idx], beam_bf_net->nv1_out_sizes[idx]);
    idx++;
    for (i = idx; i < nout; i++) {
        memcpy(beam_bf_net->nv1_cache[i], beam_bf_net->nv1_output[i],
               beam_bf_net->nv1_out_sizes[i]);
    }

    beam_bf_net->nv1_idx++;
    qtk_nn_vm_reset(nv);
    // memcpy(item->val, xx, item->bytes * item->in_dim);
}

void wtk_beam_bf_net_feed_onnx(wtk_beam_bf_net_t *beam_bf_net) {
    int i;
    int nbin = beam_bf_net->cfg->nbin;
    int num_frame = beam_bf_net->cfg->num_frame;
    float *mask = beam_bf_net->mask;
    float *mag = beam_bf_net->mag;
    float *cs = beam_bf_net->cs;
    float *mag_data = qtk_nnrt_value_get_data(beam_bf_net->stage1_rt,
                                              beam_bf_net->stage1_inputs[0]);
    int numout = beam_bf_net->stage1_rt->num_out;
    int numin = beam_bf_net->stage1_rt->num_in;
    int outer_in_num = 2;
    int outer_out_num = 1;
    int idx = 0;

    memcpy(mag_data, mag, sizeof(float) * nbin * num_frame);
    idx++;
    float *cs_data = qtk_nnrt_value_get_data(beam_bf_net->stage1_rt,
                                             beam_bf_net->stage1_inputs[idx]);
    memcpy(cs_data, cs, sizeof(float) * nbin * num_frame);
    idx++;

    for (i = 0; i < numin; i++) {
        qtk_nnrt_feed(beam_bf_net->stage1_rt, beam_bf_net->stage1_inputs[i], i);
    }

    qtk_nnrt_run(beam_bf_net->stage1_rt);

    for (i = outer_out_num; i < numout; i++) {
        int in_idx = i - outer_out_num + outer_in_num;
        if (beam_bf_net->stage1_rt->cfg->use_ss_ipu) {
            qtk_nnrt_value_t out_val;
            void *src, *dst;
            qtk_nnrt_get_output(beam_bf_net->stage1_rt, &out_val, i);
            src = qtk_nnrt_value_get_data(beam_bf_net->stage1_rt, out_val);
            dst = qtk_nnrt_value_get_data(beam_bf_net->stage1_rt,
                                          beam_bf_net->stage1_inputs[in_idx]);
            memcpy(dst, src,
                   qtk_nnrt_get_input_nbytes(beam_bf_net->stage1_rt, in_idx));
            qtk_nnrt_value_release(beam_bf_net->stage1_rt, out_val);
        } else {
            qtk_nnrt_value_release(beam_bf_net->stage1_rt,
                                   beam_bf_net->stage1_inputs[in_idx]);
            qtk_nnrt_get_output(beam_bf_net->stage1_rt,
                                &beam_bf_net->stage1_inputs[in_idx], i);
        }
    }

    {
        idx = 0;
        qtk_nnrt_value_t output_mask;
        qtk_nnrt_get_output(beam_bf_net->stage1_rt, &output_mask, idx);
        memcpy(mask,
               qtk_nnrt_value_get_data(beam_bf_net->stage1_rt, output_mask),
               sizeof(float) * nbin * num_frame);
        qtk_nnrt_value_release(beam_bf_net->stage1_rt, output_mask);
        qtk_nnrt_reset(beam_bf_net->stage1_rt);
    }
}

void wtk_beam_bf_net_feed_cnon(wtk_beam_bf_net_t *beam_bf_net,
                               wtk_complex_t *fft) {
    int nbin = beam_bf_net->cfg->nbin;
    float sym = beam_bf_net->cfg->sym;
    static float fx = 2.0f * PI / RAND_MAX;
    int cnon_clip_s = beam_bf_net->cfg->cnon_clip_s;
    int cnon_clip_e = beam_bf_net->cfg->cnon_clip_e;
    float f, f2;
    int i;

    for (i = max(1, cnon_clip_s); i < min(nbin - 1, cnon_clip_e); ++i) {
        f = rand() * fx;
        f2 = 1.f - powf((nbin - i) * 1.0 / nbin, 0.5f);
        if (f2 > 0) {
            // f2=sqrtf(f2);
            fft[i].a += sym * cosf(f) * f2;
            fft[i].b += sym * sinf(f) * f2;
        }
    }
}

void wtk_beam_bf_net_control_bs(wtk_beam_bf_net_t *beam_bf_net, float *out,
                                int len) {
    float *bs_win = beam_bf_net->bs_win;
    float out_max;
    int i;

    if (beam_bf_net->mic_sil == 0) {
        out_max = wtk_float_abs_max(out, len);
        if (out_max > beam_bf_net->cfg->max_out) {
            beam_bf_net->bs_scale = beam_bf_net->cfg->max_out / out_max;
            if (beam_bf_net->bs_scale < beam_bf_net->bs_last_scale) {
                beam_bf_net->bs_last_scale = beam_bf_net->bs_scale;
            } else {
                beam_bf_net->bs_scale = beam_bf_net->bs_last_scale;
            }
            beam_bf_net->bs_max_cnt = 5;
        }
        if (bs_win) {
            for (i = 0; i < len / 2; ++i) {
                out[i] *= beam_bf_net->bs_scale * bs_win[i] +
                          beam_bf_net->bs_real_scale * (1.0 - bs_win[i]);
            }
            for (i = len / 2; i < len; ++i) {
                out[i] *= beam_bf_net->bs_scale;
            }
            beam_bf_net->bs_real_scale = beam_bf_net->bs_scale;
        } else {
            for (i = 0; i < len; ++i) {
                out[i] *= beam_bf_net->bs_scale;
            }
        }
        if (beam_bf_net->bs_max_cnt > 0) {
            --beam_bf_net->bs_max_cnt;
        }
        if (beam_bf_net->bs_max_cnt <= 0 && beam_bf_net->bs_scale < 1.0) {
            beam_bf_net->bs_scale *= 1.1f;
            beam_bf_net->bs_last_scale = beam_bf_net->bs_scale;
            if (beam_bf_net->bs_scale > 1.0) {
                beam_bf_net->bs_scale = 1.0;
                beam_bf_net->bs_last_scale = 1.0;
            }
        }
    } else {
        beam_bf_net->bs_scale = 1.0;
        beam_bf_net->bs_last_scale = 1.0;
        beam_bf_net->bs_max_cnt = 0;
    }
}

void wtk_beam_bf_net_feed_mask_bf(wtk_beam_bf_net_t *beam_bf_net) {
    int nbin = beam_bf_net->cfg->nbin;
    float *mask = beam_bf_net->mask;
    wtk_mask_bf_t *mask_bf = beam_bf_net->mask_bf;
    wtk_complex_t **fft = beam_bf_net->fft;
    wtk_complex_t *fftx = beam_bf_net->fftx;

    wtk_mask_bf_feed(mask_bf, fft, mask, NULL, NULL, beam_bf_net->sp_sil);
    memcpy(fftx, mask_bf->fftx, sizeof(wtk_complex_t) * nbin);
}

void wtk_beam_bf_net_feed_post_process(wtk_beam_bf_net_t *beam_bf_net) {
    int nbin = beam_bf_net->cfg->nbin;
    int num_frame = beam_bf_net->cfg->num_frame;
    wtk_complex_t *fftx;
    wtk_complex_t *ffty;
    int i, n;

    wtk_drft_t *rfft = beam_bf_net->rfft;
    float *rfft_in = beam_bf_net->rfft_in;
    float *synthesis_mem = beam_bf_net->synthesis_mem;
    float *synthesis_window = beam_bf_net->synthesis_window;
    float *out = beam_bf_net->out;
    short *pv = (short *)out;
    int wins = beam_bf_net->cfg->wins;
    int fsize = wins / 2;

    float micenr;
    float micenr_thresh = beam_bf_net->cfg->micenr_thresh;
    int micenr_cnt = beam_bf_net->cfg->micenr_cnt;

    float qmmse2_mask_thresh = beam_bf_net->cfg->qmmse2_mask_thresh;
    float *mask = beam_bf_net->mask;
    int clip_s = beam_bf_net->cfg->clip_s;
    int clip_e = beam_bf_net->cfg->clip_e;

    fftx = beam_bf_net->fftx;
    for (n = 0; n < num_frame; ++n, fftx += nbin, ffty += nbin, mask += nbin) {
        // static int cnt=0;
        // cnt++;
        micenr = wtk_beam_bf_net_fft_energy(fftx, nbin);
        // printf("%f\n", micenr);
        if (micenr > micenr_thresh) {
            // if(beam_bf_net->mic_sil==1)
            // {
            // 	printf("sp start %f %f
            // %f\n", 1.0/16000*cnt*(nbin-1),micenr,micenr_thresh);
            // }
            beam_bf_net->mic_sil = 0;
            beam_bf_net->mic_silcnt = micenr_cnt;
        } else if (beam_bf_net->mic_sil == 0) {
            beam_bf_net->mic_silcnt -= 1;
            if (beam_bf_net->mic_silcnt <= 0) {
                // printf("sp end %f\n", 1.0/16000*cnt*(nbin-1));
                beam_bf_net->mic_sil = 1;
            }
        }

        if (beam_bf_net->qmmse2 && beam_bf_net->denoise_enable) {
            float mean_mask = wtk_float_abs_mean(mask, nbin);
            // printf("%f\n", mean_mask);
            if (mean_mask < qmmse2_mask_thresh) {
                wtk_qmmse_feed_mask(beam_bf_net->qmmse2, fftx, mask);
            } else {
                wtk_qmmse_denoise(beam_bf_net->qmmse2, fftx);
            }
        }
    }

    fftx = beam_bf_net->fftx;
    for (n = 0; n < num_frame; ++n, fftx += nbin) {
        for (i = 0; i < clip_s; ++i) {
            fftx[i].a = 0;
            fftx[i].b = 0;
        }
        for (i = clip_e; i < nbin; ++i) {
            fftx[i].a = 0;
            fftx[i].b = 0;
        }
        if (beam_bf_net->cfg->use_pffft) {
            wtk_drft_istft2(rfft, rfft_in, synthesis_mem, fftx, out, wins,
                            synthesis_window);
        } else {
            wtk_drft_istft(rfft, rfft_in, synthesis_mem, fftx, out, wins,
                           synthesis_window);
        }

        for (i = 0; i < fsize; ++i) {
            out[i] *= 32768.0;
        }
        if (beam_bf_net->limiter) {
            wtk_limiter_feed(beam_bf_net->limiter, out, fsize);
        } else {
            wtk_beam_bf_net_control_bs(beam_bf_net, out, fsize);
        }
        for (i = 0; i < fsize; ++i) {
            pv[i] = floorf(out[i] + 0.5);
        }
        if (beam_bf_net->notify) {
            beam_bf_net->notify(beam_bf_net->ths, pv, fsize);
        }
    }
}

void wtk_beam_bf_net_feed_model1(wtk_beam_bf_net_t *beam_bf_net) {
    wtk_complex_t **fft = beam_bf_net->fft;
    int nbin = beam_bf_net->cfg->nbin;
    int num_frame = beam_bf_net->cfg->num_frame;
    float *mag = beam_bf_net->mag;
    float *cs = beam_bf_net->cs;
    float model1_scale = beam_bf_net->cfg->model1_scale;
    float model1_sp_scale = beam_bf_net->cfg->model1_sp_scale;
    int i;

    wtk_beam_bf_net_compute_feature(beam_bf_net);

    for (i = 0; i < nbin * num_frame; ++i) {
        mag[i] *= model1_scale;
        cs[i] *= model1_scale;
    }
    if (beam_bf_net->cfg->use_stage1_rt) {
        wtk_beam_bf_net_feed_onnx(beam_bf_net);
    } else if (beam_bf_net->cfg->use_qnn) {
        wtk_beam_bf_net_feed_qnn(beam_bf_net);
    }
}

void wtk_beam_bf_net_feed(wtk_beam_bf_net_t *beam_bf_net, short *data, int len,
                          int is_end) {
    int i, j;
    int nbin = beam_bf_net->cfg->nbin;
    int nmicchannel = beam_bf_net->cfg->nmicchannel;
    int *mic_channel = beam_bf_net->cfg->mic_channel;
    int nspchannel = beam_bf_net->cfg->nspchannel;
    int *sp_channel = beam_bf_net->cfg->sp_channel;
    int sp_main_chn = beam_bf_net->cfg->sp_main_chn;
    int channel = beam_bf_net->cfg->channel;
    int wins = beam_bf_net->cfg->wins;
    int fsize = wins / 2;
    int num_frame = beam_bf_net->cfg->num_frame;
    int nfsize = fsize * num_frame;
    int offset = nfsize - fsize;
    int f_offset = 0;
    wtk_drft_t *rfft = beam_bf_net->rfft;
    float *rfft_in = beam_bf_net->rfft_in;
    wtk_complex_t **fft = beam_bf_net->fft;
    wtk_complex_t **fft_sp = beam_bf_net->fft_sp;
    wtk_complex_t *fftx = beam_bf_net->fftx;
    float **analysis_mem = beam_bf_net->analysis_mem;
    float **analysis_mem_sp = beam_bf_net->analysis_mem_sp;
    float *analysis_window = beam_bf_net->analysis_window;
    float *out = beam_bf_net->out;
    short *pv = (short *)out;
    wtk_strbuf_t **mic = beam_bf_net->mic;
    wtk_strbuf_t **sp = beam_bf_net->sp;
    float fv;
    int length;
    float spenr;
    float spenr_thresh = beam_bf_net->cfg->spenr_thresh;
    int spenr_cnt = beam_bf_net->cfg->spenr_cnt;
    float mic_scale = beam_bf_net->mic_scale;
    float sp_scale = beam_bf_net->sp_scale;
    float *fv1;

    for (i = 0; i < len; ++i) {
        for (j = 0; j < nmicchannel; ++j) {
            fv = WTK_WAV_SHORT_TO_FLOAT(data[mic_channel[j]]) * mic_scale;
            wtk_strbuf_push(mic[j], (char *)&(fv), sizeof(float));
        }
        for (j = 0; j < nspchannel; ++j) {
            fv = WTK_WAV_SHORT_TO_FLOAT(data[sp_channel[j]]) * sp_scale;
            wtk_strbuf_push(sp[j], (char *)&(fv), sizeof(float));
        }
        data += channel;
    }
    length = mic[0]->pos / sizeof(float);
    while (length >= nfsize) {
        ++beam_bf_net->nframe;
        f_offset = (beam_bf_net->nframe - 1) * beam_bf_net->cfg->nbin;
        for (i = 0; i < nmicchannel; ++i) {
            fv1 = (float *)(mic[i]->data);
            // {
            //     int ii;
            //     int n = wins;

            //     for (ii = 0; ii < n; ++ii) {
            //         analysis_window[ii] =
            //             sqrtf(0.5 * (1 - cos(2 * PI * (ii) / (n - 1))));
            //     }
            // }
            if (beam_bf_net->cfg->use_pffft) {
                wtk_drft_stft2(rfft, rfft_in, analysis_mem[i],
                               fft[i] + f_offset, fv1 + offset, wins,
                               analysis_window);
            } else {
                wtk_drft_stft(rfft, rfft_in, analysis_mem[i], fft[i] + f_offset,
                              fv1 + offset, wins, analysis_window);
            }
        }
        if (beam_bf_net->echo_enable) {
            for (i = 0; i < nspchannel; ++i) {
                fv1 = (float *)(sp[i]->data);
                if (beam_bf_net->cfg->use_pffft) {
                    wtk_drft_stft2(rfft, rfft_in, analysis_mem_sp[i],
                                   fft_sp[i] + f_offset, fv1 + offset, wins,
                                   analysis_window);
                } else {
                    wtk_drft_stft(rfft, rfft_in, analysis_mem_sp[i],
                                  fft_sp[i] + f_offset, fv1 + offset, wins,
                                  analysis_window);
                }
            }
        } else {
            for (i = 0; i < nspchannel; ++i) {
                memset(fft_sp[i], 0, sizeof(wtk_complex_t) * nbin * num_frame);
            }
        }

        if (nspchannel > 0) {
            fv1 = (float *)(sp[0]->data);
            spenr = wtk_beam_bf_net_sp_energy(fv1 + offset, fsize);
        } else {
            spenr = 0;
        }
        // printf("%f\n", spenr);
        // static int cnt=0;
        // cnt++;
        if (spenr > spenr_thresh) {
            // if(beam_bf_net->sp_sil==1)
            // {
            // 	printf("sp start %f %f
            // %f\n", 1.0/16000*cnt*(nbin-1),spenr,spenr_thresh);
            // }
            beam_bf_net->sp_sil = 0;
            beam_bf_net->sp_silcnt = spenr_cnt;
        } else if (beam_bf_net->sp_sil == 0) {
            beam_bf_net->sp_silcnt -= 1;
            if (beam_bf_net->sp_silcnt <= 0) {
                // printf("sp end %f\n", 1.0/16000*cnt*(nbin-1));
                beam_bf_net->sp_sil = 1;
            }
        }

        beam_bf_net->sum_sp_sil += beam_bf_net->sp_sil;
        beam_bf_net->sp_state[beam_bf_net->nframe - 1] = beam_bf_net->sp_sil;

        if (beam_bf_net->nframe == num_frame) {
            if (beam_bf_net->sum_sp_sil == num_frame) {
                if (beam_bf_net->cfg->use_stage1_rt ||
                    beam_bf_net->cfg->use_qnn) {
                    wtk_beam_bf_net_feed_model1(beam_bf_net);
                    if (beam_bf_net->cfg->use_mask_bf) {
                        wtk_beam_bf_net_feed_mask_bf(beam_bf_net);
                    }
                } else {
                    memcpy(fftx, fft[sp_main_chn],
                           sizeof(wtk_complex_t) * nbin * num_frame);
                }
            }
            wtk_beam_bf_net_feed_post_process(beam_bf_net);
            beam_bf_net->sum_sp_sil = 0;
            beam_bf_net->nframe = 0;
        }
        wtk_strbufs_pop(mic, nmicchannel, fsize * sizeof(float));
        wtk_strbufs_pop(sp, nspchannel, fsize * sizeof(float));
        length = mic[0]->pos / sizeof(float);
    }
    if (is_end && length > 0) {
        if (beam_bf_net->notify) {
            out = (float *)mic[0]->data;
            pv = (short *)out;
            for (i = 0; i < length; ++i) {
                pv[i] = WTK_WAV_FLOAT_TO_SHORT(out[i]);
            }
            beam_bf_net->notify(beam_bf_net->ths, pv, length);
        }
    }
}

void wtk_beam_bf_net_set_micscale(wtk_beam_bf_net_t *beam_bf_net, float scale) {
    beam_bf_net->mic_scale = scale;
}
void wtk_beam_bf_net_set_agcenable(wtk_beam_bf_net_t *beam_bf_net, int enable) {
    beam_bf_net->agc_enable = enable;
    if (beam_bf_net->qmmse2) {
        if (enable) {
            beam_bf_net->qmmse2->cfg->use_agc = 1;
        } else {
            beam_bf_net->qmmse2->cfg->use_agc = 0;
        }
    }
}
void wtk_beam_bf_net_set_agclevel(wtk_beam_bf_net_t *beam_bf_net, int level) {
    int i;
    int n_agc_level = beam_bf_net->cfg->n_agc_level;
    float *qmmse2_agc_level = beam_bf_net->cfg->qmmse2_agc_level;
    float *qmmse2_max_gain = beam_bf_net->cfg->qmmse2_max_gain;
    float *gc_gain_level = beam_bf_net->cfg->gc_gain_level;
    level = min(level, n_agc_level);
    if (beam_bf_net->qmmse2) {
        for (i = 0; i < n_agc_level; ++i) {
            if (level == i + 1) {
                beam_bf_net->qmmse2->cfg->agc_level = qmmse2_agc_level[i];
                beam_bf_net->qmmse2->cfg->max_gain = qmmse2_max_gain[i];
                break;
            }
        }
    }
    if (beam_bf_net->gc) {
        for (i = 0; i < n_agc_level; ++i) {
            if (level == i + 1) {
                beam_bf_net->gc->kalman.Z_k = gc_gain_level[i];
                break;
            }
        }
    }
}
void wtk_beam_bf_net_set_echoenable(wtk_beam_bf_net_t *beam_bf_net,
                                    int enable) {
    beam_bf_net->echo_enable = enable;
}
void wtk_beam_bf_net_set_denoiseenable(wtk_beam_bf_net_t *beam_bf_net,
                                       int enable) {
    beam_bf_net->denoise_enable = enable;
}
void wtk_beam_bf_net_set_denoiselevel(wtk_beam_bf_net_t *beam_bf_net,
                                      int level) {
    int i;
    int n_ans_level = beam_bf_net->cfg->n_ans_level;
    float *qmmse2_noise_suppress = beam_bf_net->cfg->qmmse2_noise_suppress;
    level = min(level, n_ans_level);
    if (beam_bf_net->qmmse2) {
        for (i = 0; i < n_ans_level; ++i) {
            if (level == i + 1) {
                beam_bf_net->qmmse2->cfg->noise_suppress =
                    qmmse2_noise_suppress[i];
                break;
            }
        }
    }
}
void wtk_beam_bf_net_set_denoisesuppress(wtk_beam_bf_net_t *beam_bf_net,
                                         float suppress) {
    if (beam_bf_net->qmmse2) {
        beam_bf_net->qmmse2->cfg->noise_suppress = suppress;
    }
}
