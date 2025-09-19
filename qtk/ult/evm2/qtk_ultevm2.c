#include "qtk/ult/evm2/qtk_ultevm2.h"

// index = abs(f) >= ws1 & abs(f) <= ws2;
static float *gen_filter_(int rate, int fftsize, float xfs, float xfe,
                          int use_scale) {
    float *f, *x;
    int idx1, idx2;
    wtk_rfft_t *fft;
    int i;
    float fx;
    int idx3, idx4;
    float f1, f2, f3, f4;

    fft = wtk_rfft_new(fftsize / 2);
    // wtk_debug("rate=%d fftsize=%d win=%d
    // f=[%d/%d]\n",rate,fftsize,fft->win,xfs,xfe); exit(0);
    f = (float *)wtk_calloc(fftsize, sizeof(float));
    x = (float *)wtk_calloc(fftsize, sizeof(float));
    // idx1=fft->len-((xfs)*nbin*2.0/rate);//+0.5);
    // idx2=fft->len-((xfe)*nbin*2.0/rate-0.5);//+0.5;
    idx1 = idx2 = idx3 = idx4 = -1;
    f1 = f3 = rate;
    f2 = f4 = 0;
    for (i = 0; i < fftsize; ++i) {
        fx = rate * 1.0 * i / fftsize;
        if (i >= fftsize / 2) {
            fx -= rate;
        }
        // wtk_debug("v[%d]=%f\n",i,fx);
        if (fx > 0 && fx >= xfs) {
            if (fx < f1) {
                idx1 = i;
                f1 = fx;
            }
        }
        if (fx > 0 && fx <= xfe) {
            if (fx > f2) {
                idx2 = i;
            }
        }
        if (fx < 0) {
            fx = -fx;
        } else {
            continue;
        }
        if (fx >= xfs) {
            // wtk_debug("v[%d]=%f\n",i,fx);
            if (fx < f3) {
                f3 = fx;
                idx4 = i;
            }
        }
        if (fx <= xfe) {
            // wtk_debug("v[%d]=%f\n",i,fx);
            // idx4=i;
            if (fx > f4) {
                f4 = fx;
                idx3 = i;
            }
        }
    }
    // 918 1130 2966 3178
    // wtk_debug("%d/%d %d/%d band=[%f,%f]\n",idx1,idx2,idx3,idx4,xfs,xfe);
    if (0) {
        idx1 = (int)(xfs * fft->len * 1.0 / rate + 0.5);
        idx2 = (int)(xfe * fft->len * 1.0 / rate + 0.5);
        idx4 = min(fft->len - idx1, fft->len);
        idx3 = fft->len - idx2;
    }
    // wtk_debug("%d/%d => %d/%d %d/%d\n",xfs,xfe,idx1,idx2,idx3,idx4);
    // idx1=((xfs)*nbin*2.0/rate);//+0.5);
    // idx2=((xfe)*nbin*2.0/rate);//+0.5;
    // idx1=(cfg->fs-500)*nbin*2.0/cfg->rate+1;
    // idx2=(cfg->fe+500)*nbin*2.0/cfg->rate;//+0.5;
    // wtk_debug("%f/%f => %d/%d %d/%d
    // [%f/%f]\n",xfs,xfe,idx1,idx2,idx3,idx4,xfs*fft->len*1.0/rate,xfe*fft->len*1.0/rate);
    // exit(0);
    for (i = 0; i < fftsize; ++i) {
        f[i] = 0;
    }
    fx = sqrt(fft->len * 1.0 / (idx2 - idx1 + 2 + idx4 - idx3));
    // wtk_debug("f=%f %d\n",fx,idx2-idx2+1+idx4-idx3);
    // exit(0);
    if (use_scale) { // not use scale for matlab scale = sqrt(winsize /
                     // sum(H.*conj(H))); H = H * scale;
        for (i = idx1; i <= idx2; ++i) {
            f[i] = fx;
            // f[fftsize-i+1]=1.0;
        }
        for (i = idx3; i <= idx4; ++i) {
            f[fft->len - i] = fx;
        }
    } else {
        for (i = idx1; i <= idx2; ++i) {
            f[i] = 1;
            // f[fftsize-i+1]=1.0;
        }
        for (i = idx3; i <= idx4; ++i) {
            f[fft->len - i] = 1;
        }
    }
    //	for(i=fft->len-idx2;i<=(fft->len-idx1);++i)
    //	{
    //		f[i]=fx;
    //	}
    // print_float(f,fftsize);
    // exit(0);
    //	wtk_debug("%d/%d %d/%d\n",idx1,idx2,fftsize-idx1+1,fftsize-idx2+1);
    //	wtk_debug("fs=%d fe=%d\n",cfg->fs,cfg->fe);
    //	exit(0);
    //	return;
    // wtk_rfft_print_fft(f,fftsize);
    // wtk_debug("fv=%f\n",wtk_float_sum(f,fftsize));
    wtk_rfft_process_ifft(fft, f, x);
    // print_float(x,fft->len);
    // exit(0);
    memcpy(f, x, sizeof(float) * fft->win);
    memcpy(x, x + fft->win, sizeof(float) * fft->win);
    memcpy(x + fft->win, f, sizeof(float) * fft->win);
    // print_float(x,fft->len);
    // exit(0);
    wtk_free(f);
    f = wtk_math_create_hanning_window2(fft->len);
    for (i = 0; i < fft->len; ++i) {
        x[i] *= f[i];
    }
    // print_float(x,fft->len);
    // exit(0);
    wtk_rfft_delete(fft);
    wtk_free(f);
    // wtk_free(x);
    // print_float(x,fft->len);
    // exit(0);
    return x;
}

static float *gen_rcosdesign_filter_(float beta, float span, float sps,
                                     int *len) {
    int delay;
    float *h;
    int i, n;
    float fv, f;
    float xf;

    delay = span * sps / 2 + 0.5;
    n = delay * 2 + 1;
    // wtk_debug("%f/%f delay=%d pass=%f\n",span,sps,delay,cfg->passband);
    // exit(0);

    h = (float *)wtk_malloc(sizeof(float) * n);
    fv = -delay;
    xf = 0;
    for (i = 0; i < n; ++i) {
        if (i == n - 1) {
            h[i] = delay / sps;
        } else {
            h[i] = fv / sps;
            fv += 1;
        }
        if (h[i] == 0) {
            //-1 / (np.pi*sps) * (np.pi * (beta-1) - 4*beta)
            h[i] = -1.0 / (PI * sps) * (PI * (beta - 1) - 4 * beta);
        } else {
            // np.fabs(4*beta*t) - 1)
            f = fabs(fabs(4 * beta * h[i]) - 1);
            // wtk_debug("====> v[%d]=%f\n",i,f);
            if (f < 0.000001) {
                /*
    b[idx2] = 1 / (2*np.pi*sps) * (
        np.pi * (beta+1) * np.sin(np.pi * (beta+1) / (4*beta))
        - 4*beta           * np.sin(np.pi * (beta-1) / (4*beta))
        + np.pi*(beta-1)   * np.cos(np.pi * (beta-1) / (4*beta))
    )*/
                h[i] = 1.0 / (2 * PI * sps) *
                       (PI * (beta + 1) * sin(PI * (beta + 1) / (4 * beta)) -
                        4 * beta * sin(PI * (beta - 1) / (4 * beta)) +
                        PI * (beta - 1) * cos(PI * (beta - 1) / (4 * beta)));
                // wtk_debug("==============> v[%d]=%f\n",i,h[i]);
            } else {
                /*
        b[ind] = -4*beta/sps * (np.cos((1+beta)*np.pi*nind) +
                                np.sin((1-beta)*np.pi*nind) / (4*beta*nind)) / (
                                np.pi * (np.power(4*beta*nind, 2) - 1))*/
                float nind = h[i];

                h[i] = -4 * beta / sps *
                       (cos((1 + beta) * PI * nind) +
                        sin((1 - beta) * PI * nind) / (4 * beta * nind)) /
                       (PI * (pow(4 * beta * nind, 2) - 1));
            }
        }
        xf += pow(h[i], 2);
        // wtk_debug("v[%d]=%f\n",i,h[i]);
    }
    // exit(0);
    xf = sqrt(xf);
    for (i = 0; i < n; ++i) {
        h[i] /= xf;
    }

    int use_hann = 1;
    // int use_reverse=1;
    if (use_hann) {
        float *hann = wtk_math_create_hanning_window2(n);
        for (i = 0; i < n; ++i) {
            h[i] *= hann[i];
        }
        // print_float(hann,n);
        wtk_free(hann);
    }

    // if(use_reverse){
    //     float tmp;
    //     for(i=0;i<n/2;++i){
    //         tmp = h[i];
    //         h[i] = h[n-i-1];
    //         h[n-i-1] = tmp;
    //     }
    // }

    // print_float(h,n);
    // exit(0);
    *len = n;
    return h;
}

static float *gen_median_filter_(int len) {
    int i;
    float *data = wtk_malloc(sizeof(float) * len);
    int center_idx = ceilf(len / 2.0);
    for (i = 0; i < len; i++) {
        data[i] = -1.0 / len;
    }
    data[center_idx] = data[0] + 1;
    return data;
}

static float calc_center_freq_(qtk_ultevm2_t *u, float *v) {
    float df = (float)u->cfg->sample_rate / u->cfg->fc_est_len;
    int fc_sig_idx = u->cfg->fc_sig / df;
    int search_width = u->cfg->fc_search_width / df;
    int st = max(0, fc_sig_idx - search_width);
    int et = min(u->cfg->fc_est_len / 2, fc_sig_idx + search_width);
    float max_spec = -1;
    int max_idx;
    int i;
    wtk_rfft_process_fft(u->fc_est_fft, u->fc_est_F, v);
    for (i = st; i < et; i++) {
        wtk_complex_t spec =
            wtk_rfft_get_value(u->fc_est_F, u->cfg->fc_est_len, i);
        float spec_val = spec.a * spec.a + spec.b * spec.b;
        if (spec_val > max_spec) {
            max_spec = spec_val;
            max_idx = i;
        }
    }
    return max_idx * df;
}

static void gen_carrier_phase_(float fc, float *iphase, float *qphase, int len,
                               float fs) {
    int i;
    for (i = 0; i < len; i++) {
        float v = 2 * PI * fc * (i + 1) / fs;
        iphase[i] = sin(v);
        qphase[i] = cos(v);
    }
}

static void update_center_freq_(qtk_ultevm2_t *u, float *v) {
    int i;
    int fc = v ? calc_center_freq_(u, v) : u->cfg->fc_sig;
    int fc_side = fc + 500;
    u->sig_period = u->cfg->sample_rate / qtk_gcd(fc, u->cfg->sample_rate);
    u->sig_side_period =
        u->cfg->sample_rate / qtk_gcd(fc_side, u->cfg->sample_rate);

    if (u->carrier_iphase) {
        wtk_free(u->carrier_iphase);
    }
    if (u->carrier_qphase) {
        wtk_free(u->carrier_qphase);
    }
    if (u->carrier_iphase_side) {
        wtk_free(u->carrier_iphase_side);
    }
    if (u->carrier_qphase_side) {
        wtk_free(u->carrier_qphase_side);
    }
    if (u->sig) {
        wtk_free(u->sig);
    }

    u->sig = wtk_malloc(sizeof(short) * u->sig_period);
    u->carrier_iphase = wtk_malloc(sizeof(float) * u->sig_period);
    u->carrier_qphase = wtk_malloc(sizeof(float) * u->sig_period);
    u->carrier_iphase_side = wtk_malloc(sizeof(float) * u->sig_side_period);
    u->carrier_qphase_side = wtk_malloc(sizeof(float) * u->sig_side_period);

    gen_carrier_phase_(fc, u->carrier_iphase, u->carrier_qphase, u->sig_period,
                       u->cfg->sample_rate);
    gen_carrier_phase_(fc_side, u->carrier_iphase_side, u->carrier_qphase_side,
                       u->sig_side_period, u->cfg->sample_rate);
    for (i = 0; i < u->sig_period; i++) {
        u->sig[i] = 0.5 * u->carrier_iphase[i] * 32768;
    }
}

static void filter_pipeline_(qtk_ultevm2_t *u, int is_end) {
#define CONV_PIPELINE_FEED(which)                                              \
    do {                                                                       \
        int __len = u->which##_data->pos / sizeof(float);                      \
        wtk_strbuf_reset(u->which##_data);                                     \
        wtk_conv2_feed(u->which##_rcos, (float *)u->which##_data->data, __len, \
                       is_end);                                                \
                                                                               \
        __len = u->which##_data->pos / sizeof(float);                          \
        wtk_strbuf_reset(u->which##_data);                                     \
        wtk_conv2_feed(u->which##_median, (float *)u->which##_data->data,      \
                       __len, is_end);                                         \
    } while (0)
    CONV_PIPELINE_FEED(sig_re);
    CONV_PIPELINE_FEED(sig_im);
    CONV_PIPELINE_FEED(sig_side_re);
    CONV_PIPELINE_FEED(sig_side_im);
#undef CONV_PIPELINE_FEED
}

static void downsample_data_(wtk_strbuf_t *re, wtk_strbuf_t *im,
                             wtk_strbuf_t *dst, int Q, int *Q_idx) {
    float *re_data = (float *)re->data;
    float *im_data = (float *)im->data;
    int i;
    int len = re->pos / sizeof(float);
    if (len - 1 + *Q_idx < Q) {
        *Q_idx += len;
        return;
    }
    for (i = Q - *Q_idx; i < len; i += Q) {
        wtk_complex_t v = {re_data[i], im_data[i]};
        wtk_strbuf_push(dst, (const char *)&v, sizeof(wtk_complex_t));
    }
    *Q_idx = len - (i - Q);
    wtk_strbuf_reset(re);
    wtk_strbuf_reset(im);
}

static void downsample_(qtk_ultevm2_t *u) {
    int Q = u->cfg->Q;
    downsample_data_(u->sig_re_data, u->sig_im_data, u->sig_frame, Q,
                     &u->sig_Q_idx);
    downsample_data_(u->sig_side_re_data, u->sig_side_im_data,
                     u->sig_side_frame, Q, &u->sig_side_Q_idx);
}

static float calc_feat_max_(qtk_ultevm2_t *u, wtk_complex_t *sig) {
    int i, j;
    wtk_complex_t *F = wtk_cfft_fft(u->frame_fft, sig);
    float *spec = (float *)F;
    float *spec_end = spec + u->cfg->frame_size - 1;
    int gap = u->cfg->tri_filter_gap;
    int filter_len = 2 * gap + 1;
    float feat_max = -1;
    for (i = 0; i < u->cfg->frame_size; i++) {
        spec[i] = F[i].a * F[i].a + F[i].b * F[i].b;
    }
    for (i = u->cfg->tri_dropbin; i < u->cfg->tri_numbin; i++) {
        float sum = 0;
        for (j = 0; j < filter_len; j++) {
            sum += spec[gap * i + j];
        }
        if (sum > feat_max) {
            feat_max = sum;
        }
    }
    for (i = u->cfg->tri_dropbin; i < u->cfg->tri_numbin; i++) {
        float sum = 0;
        for (j = 0; j < filter_len; j++) {
            sum += spec_end[-(gap * i + j)];
        }
        if (sum > feat_max) {
            feat_max = sum;
        }
    }
    return log(feat_max);
}

static void process_frame_(qtk_ultevm2_t *u, wtk_complex_t *sig,
                           wtk_complex_t *sig_side) {
    int candi_active;
    u->frame_idx++;
    float feat_max_diff = calc_feat_max_(u, sig) - calc_feat_max_(u, sig_side);
    u->feat_max_smoothed =
        u->frame_idx == 1 ? feat_max_diff
                          : u->feat_max_smoothed * u->cfg->feat_max_alpha +
                                feat_max_diff * (1 - u->cfg->feat_max_alpha);
    candi_active = u->feat_max_smoothed > u->cfg->feat_max_diff_thresh;
    if (u->active) {
        if (!candi_active) {
            if (++u->post_trap > u->cfg->active_leave_trap) {
                u->active = 0;
                u->post_trap = 0;
            }
        } else {
            u->post_trap = 0;
        }
    } else {
        if (candi_active) {
            if (++u->post_trap > u->cfg->active_enter_trap) {
                u->active = 1;
                u->post_trap = 0;
            }
        } else {
            u->post_trap = 0;
        }
    }
}

static void post_process_(qtk_ultevm2_t *u) {
    int sig_sz, sig_side_sz;
    int slide_sz, downsamp_fs;
    downsamp_fs = u->cfg->sample_rate / u->cfg->Q;
    slide_sz = downsamp_fs * u->cfg->slide_dur;
    sig_sz = u->sig_frame->pos / sizeof(wtk_complex_t);
    sig_side_sz = u->sig_side_frame->pos / sizeof(wtk_complex_t);
    while (sig_sz >= u->cfg->frame_size && sig_side_sz >= u->cfg->frame_size) {
        process_frame_(u, (wtk_complex_t *)u->sig_frame->data,
                       (wtk_complex_t *)u->sig_side_frame->data);
        wtk_strbuf_pop(u->sig_frame, NULL, slide_sz * sizeof(wtk_complex_t));
        wtk_strbuf_pop(u->sig_side_frame, NULL,
                       slide_sz * sizeof(wtk_complex_t));
        sig_sz = u->sig_frame->pos / sizeof(wtk_complex_t);
        sig_side_sz = u->sig_side_frame->pos / sizeof(wtk_complex_t);
    }
}

static void on_bandpass_(qtk_ultevm2_t *u, float *data, int len) {
    int i;
    float sig_re, sig_im, sig_side_re, sig_side_im;
#define SET_CONV_DATA(which)                                                   \
    wtk_strbuf_push(u->which##_data, (const char *)&which, sizeof(float))
    for (i = 0; i < len; i++) {
        int sig_idx = (i + u->sample_idx) % u->sig_period;
        int sig_side_idx = (i + u->sample_idx) % u->sig_side_period;
        sig_re = data[i] * u->carrier_iphase[sig_idx];
        sig_im = data[i] * u->carrier_qphase[sig_idx];
        sig_side_re = data[i] * u->carrier_iphase_side[sig_side_idx];
        sig_side_im = data[i] * u->carrier_qphase_side[sig_side_idx];
        SET_CONV_DATA(sig_re);
        SET_CONV_DATA(sig_im);
        SET_CONV_DATA(sig_side_re);
        SET_CONV_DATA(sig_side_im);
    }
    u->sample_idx += len;
#undef SET_CONV_DATA
    filter_pipeline_(u, 0);
    downsample_(u); // will cleanup u->which##_data
    post_process_(u);
}

static void on_conv_strbuf_(wtk_strbuf_t *buf, float *data, int len) {
    wtk_strbuf_push(buf, (const char *)data, sizeof(float) * len);
}

static void set_tri_filter_(qtk_ultevm2_t *u) {
    int i;
    int gap = u->cfg->tri_filter_gap;
    int filter_len = gap * 2 + 1;
    float *kernel_end;
    u->tri_filter_kernel = wtk_malloc(sizeof(float) * filter_len);
    kernel_end = u->tri_filter_kernel + filter_len - 1;
    for (i = 0; i <= gap; i++) {
        u->tri_filter_kernel[i] = (i + 1.0) / (gap + 1);
    }
    for (i = 0; i < gap; i++) {
        kernel_end[-i] = u->tri_filter_kernel[i];
    }
}

qtk_ultevm2_t *qtk_ultevm2_new(qtk_ultevm2_cfg_t *cfg) {
    float *kernel;
    int n;
    qtk_ultevm2_t *u = wtk_malloc(sizeof(qtk_ultevm2_t));
    u->cfg = cfg;
    u->fc_est_fft = wtk_rfft_new(cfg->fc_est_len / 2);
    u->fc_est_F = wtk_malloc(sizeof(float) * u->cfg->fc_est_len);
    kernel = gen_filter_(cfg->sample_rate, cfg->fftsize, cfg->ws1, cfg->ws2, 0);
    n = cfg->fftsize + cfg->fftsize - 1;
    n = pow(2, wtk_rfft_next_pow(n));
    u->data = wtk_strbuf_new(1024, 1);
    u->bandpass = wtk_conv2_new(kernel, cfg->fftsize, NULL, cfg->fftsize);
    wtk_free(kernel);
    wtk_conv2_set_notify(u->bandpass, (wtk_conv2_notify_f)on_bandpass_, u);

    kernel = gen_rcosdesign_filter_(cfg->beta, cfg->span, cfg->sps, &n);
    u->sig_re_rcos = wtk_conv2_new(kernel, n, NULL, n);
    u->sig_im_rcos = wtk_conv2_new(kernel, n, NULL, n);
    u->sig_side_re_rcos = wtk_conv2_new(kernel, n, NULL, n);
    u->sig_side_im_rcos = wtk_conv2_new(kernel, n, NULL, n);
    wtk_free(kernel);

    kernel = gen_median_filter_(cfg->median_filter_len);
    u->sig_re_median = wtk_conv2_new(kernel, cfg->median_filter_len, NULL,
                                     cfg->median_filter_len);
    u->sig_im_median = wtk_conv2_new(kernel, cfg->median_filter_len, NULL,
                                     cfg->median_filter_len);
    u->sig_side_re_median = wtk_conv2_new(kernel, cfg->median_filter_len, NULL,
                                          cfg->median_filter_len);
    u->sig_side_im_median = wtk_conv2_new(kernel, cfg->median_filter_len, NULL,
                                          cfg->median_filter_len);
    wtk_free(kernel);

#define SET_CONV_NOTIFY(which)                                                 \
    do {                                                                       \
        u->which##_data = wtk_strbuf_new(1024, 1);                             \
        wtk_conv2_set_notify(u->which##_rcos,                                  \
                             (wtk_conv2_notify_f)on_conv_strbuf_,              \
                             u->which##_data);                                 \
        wtk_conv2_set_notify(u->which##_median,                                \
                             (wtk_conv2_notify_f)on_conv_strbuf_,              \
                             u->which##_data);                                 \
    } while (0)
    SET_CONV_NOTIFY(sig_re);
    SET_CONV_NOTIFY(sig_im);
    SET_CONV_NOTIFY(sig_side_re);
    SET_CONV_NOTIFY(sig_side_im);
#undef SET_CONV_NOTIFY

    u->sig = NULL;
    u->carrier_iphase = NULL;
    u->carrier_qphase = NULL;
    u->carrier_iphase_side = NULL;
    u->carrier_qphase_side = NULL;

    u->sig_frame = wtk_strbuf_new(sizeof(wtk_complex_t) * cfg->frame_size, 1);
    u->sig_side_frame =
        wtk_strbuf_new(sizeof(wtk_complex_t) * cfg->frame_size, 1);
    u->frame_fft = wtk_cfft_new(cfg->frame_size);

    u->sample_idx = 0;
    u->sig_Q_idx = 0;
    u->sig_side_Q_idx = 0;
    u->frame_idx = 0;
    u->active = 0;
    u->post_trap = 0;

    set_tri_filter_(u);
    update_center_freq_(u, NULL);

    return u;
}

void qtk_ultevm2_delete(qtk_ultevm2_t *u) {
    wtk_rfft_delete(u->fc_est_fft);
    wtk_free(u->fc_est_F);
    wtk_conv2_delete(u->bandpass);
    if (u->sig) {
        wtk_free(u->sig);
    }
    if (u->carrier_iphase) {
        wtk_free(u->carrier_iphase);
    }
    if (u->carrier_qphase) {
        wtk_free(u->carrier_qphase);
    }
    if (u->carrier_iphase_side) {
        wtk_free(u->carrier_iphase_side);
    }
    if (u->carrier_qphase_side) {
        wtk_free(u->carrier_qphase_side);
    }
    wtk_conv2_delete(u->sig_re_rcos);
    wtk_conv2_delete(u->sig_im_rcos);
    wtk_conv2_delete(u->sig_side_re_rcos);
    wtk_conv2_delete(u->sig_side_im_rcos);
    wtk_conv2_delete(u->sig_re_median);
    wtk_conv2_delete(u->sig_im_median);
    wtk_conv2_delete(u->sig_side_re_median);
    wtk_conv2_delete(u->sig_side_im_median);
    wtk_strbuf_delete(u->sig_re_data);
    wtk_strbuf_delete(u->sig_im_data);
    wtk_strbuf_delete(u->sig_side_re_data);
    wtk_strbuf_delete(u->sig_side_im_data);
    wtk_strbuf_delete(u->data);

    wtk_strbuf_delete(u->sig_frame);
    wtk_strbuf_delete(u->sig_side_frame);

    wtk_cfft_delete(u->frame_fft);
    wtk_free(u->tri_filter_kernel);
    wtk_free(u);
}

int qtk_ultevm2_feed(qtk_ultevm2_t *u, short *data, int len) {
    int i;
    for (i = 0; i < len; i++) {
        float v = data[i] / 32768.0;
        wtk_strbuf_push(u->data, (const char *)&v, sizeof(float));
    }
    wtk_conv2_feed(u->bandpass, (float *)u->data->data, len, 0);
    wtk_strbuf_reset(u->data);
    return 0;
}

int qtk_ultevm2_feed_end(qtk_ultevm2_t *u) {
    wtk_conv2_feed(u->bandpass, NULL, 0, 1);
    filter_pipeline_(u, 1);
    downsample_(u); // will cleanup u->which##_data
    post_process_(u);
    return 0;
}

int qtk_ultevm2_get_signal(qtk_ultevm2_t *u, short **dat) {
    *dat = u->sig;
    return u->sig_period;
}
