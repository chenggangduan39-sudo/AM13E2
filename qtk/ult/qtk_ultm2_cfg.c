#include "qtk_ultm2_cfg.h"

int qtk_ultm2_cfg_init(qtk_ultm2_cfg_t *cfg) {
    cfg->rate = 48000;
    cfg->s2f_size = 1024;
    // cfg->s2f_size=2048;
    // cfg->s2f_size=1024*10;
    cfg->channel = 1;
    cfg->ws1 = 1000;
    cfg->ws2 = 1000;
    cfg->mic_fft = 4096;
    cfg->mic_kernel = NULL;
    cfg->echo_kernel = NULL;
    cfg->echo_fft = 128;
    cfg->use_echo = 0;
    cfg->mic_min_f = 30;
    cfg->mic_min_f2 = 20;
    cfg->echo_min_f = 100;
    cfg->echo_min_f2 = 50;
    cfg->echo_range1 = 0.5;
    cfg->echo_range2 = 0.5;
    cfg->echo_hint1 = 3;
    cfg->echo_hint2 = 5;
    cfg->debug = 0;

    cfg->mic_h_fs = 17000;
    cfg->mic_h_fe = 20000;
    cfg->mic_h_fft = 128;
    cfg->mic_h_kernel = NULL;
    cfg->use_harmonic_d = 0;
    cfg->cut_hint2 = 10;
    cfg->harm_thresh = 6.0;
    cfg->harm_thresh2 = 6.0;
    cfg->harm_hint = 10;
    cfg->harm_hint2 = 10;

    cfg->cut_hint = 1;
    cfg->use_aligned = 0;
    cfg->mic_skip_f = 50;
    cfg->scale = 1;

    cfg->r_zc_pad_a = NULL;
    cfg->r_zc_pad_b = NULL;
    cfg->L = 0;
    cfg->align_cnt = 1;
    cfg->use_echo_align = 0;
    return 0;
}

int qtk_ultm2_cfg_clean(qtk_ultm2_cfg_t *cfg) {
    if (cfg->echo_kernel) {
        wtk_free(cfg->echo_kernel);
    }
    if (cfg->mic_kernel) {
        wtk_free(cfg->mic_kernel);
    }
    if (cfg->mic_h_kernel) {
        wtk_free(cfg->mic_h_kernel);
    }
    if(cfg->r_zc_pad_a){
        wtk_free(cfg->r_zc_pad_a);
    }
    if(cfg->r_zc_pad_b){
        wtk_free(cfg->r_zc_pad_b);
    }
    return 0;
}

int qtk_ultm2_cfg_update_local(qtk_ultm2_cfg_t *cfg, wtk_local_cfg_t *main) {
    wtk_local_cfg_t *lc;
    wtk_string_t *v;

    lc = main;

    // wtk_local_cfg_print(lc);
    wtk_local_cfg_update_cfg_f(lc, cfg, mic_skip_f, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_aligned, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, cut_hint, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, harm_thresh, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, harm_hint, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, harm_thresh2, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, harm_hint2, v);

    wtk_local_cfg_update_cfg_i(lc, cfg, cut_hint2, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, s2f_size, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, rate, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, ws1, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, ws2, v);

    wtk_local_cfg_update_cfg_i(lc, cfg, mic_fft, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, channel, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, echo_min_f, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, echo_min_f2, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, mic_min_f, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, mic_min_f2, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, echo_range1, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, echo_range2, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, echo_hint1, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, echo_hint2, v);

    wtk_local_cfg_update_cfg_i(lc, cfg, use_echo, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, echo_fft, v);

    wtk_local_cfg_update_cfg_b(lc, cfg, debug, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_harmonic_d, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_echo_align, v);

    wtk_local_cfg_update_cfg_i(lc, cfg, mic_fft, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, channel, v);

    wtk_local_cfg_update_cfg_i(lc, cfg, mic_h_fs, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, mic_h_fe, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, mic_h_fft, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, scale, v);

    return 0;
}

// index = abs(f) >= ws1 & abs(f) <= ws2;
float *qtk_ultm2_cfg_gen_filter(int rate, int fftsize, float xfs, float xfe) {
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
    for (i = idx1; i <= idx2; ++i) {
        f[i] = fx;
        // f[fftsize-i+1]=1.0;
    }
    for (i = idx3; i <= idx4; ++i) {
        f[fft->len - i] = fx;
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

void qtk_ultm2_cfg_update_mic_filter(qtk_ultm2_cfg_t *cfg) {
    cfg->mic_kernel =
        qtk_ultm2_cfg_gen_filter(cfg->rate, cfg->mic_fft, cfg->ws1, cfg->ws2);
    // print_float(cfg->mic_kernel,cfg->mic_fft);
    // exit(0);
}

void qtk_ultm2_cfg_update_echo_filter(qtk_ultm2_cfg_t *cfg) {
    cfg->echo_kernel =
        qtk_ultm2_cfg_gen_filter(cfg->rate, cfg->echo_fft, cfg->ws1, cfg->ws2);
}

void qtk_ultm2_cfg_update_mic_h_filter(qtk_ultm2_cfg_t *cfg) {
    cfg->mic_h_kernel = qtk_ultm2_cfg_gen_filter(cfg->rate, cfg->mic_h_fft,
                                                 cfg->mic_h_fs, cfg->mic_h_fe);

    // wtk_debug("%d %d/%d\n",cfg->mic_h_fft,cfg->mic_h_fs,cfg->mic_h_fe);
    // exit(0);
}

int qtk_ultm2_cfg_update(qtk_ultm2_cfg_t *cfg) {

    qtk_ultm2_cfg_update_mic_filter(cfg);
    if (cfg->use_echo) {
        qtk_ultm2_cfg_update_echo_filter(cfg);
    }
    if (cfg->use_harmonic_d) {
        qtk_ultm2_cfg_update_mic_h_filter(cfg);
    }
    cfg->scale /= 32768.0;
    // wtk_ultmd_cfg_update(&(cfg->md),cfg->rate,cfg->T,cfg->Q,cfg->skip);
    return 0;
}
