#include "wtk_rt60_cfg.h"
#include "wtk/core/wtk_wavfile.h"

void wtk_rt60_cfg_get_log_sweep(wtk_rt60_cfg_t *cfg) {
    float f1 = cfg->f1;
    float f2 = cfg->f2;
    double omega1 = 2 * M_PI * f1;
    double omega2 = 2 * M_PI * f2;
    float t = cfg->t;
    float amp = cfg->amp;
    int rate = cfg->rate;
    int sweep_len = (int)(t * rate);
    int i;
    double t1, e1;
    double k, w;

    cfg->sweep_len = sweep_len;

    t1 = 0;
    e1 = omega1 * t / log(omega2 / omega1);

    cfg->sweep = (float *)wtk_malloc(sizeof(float) * sweep_len);
    cfg->inv_sweep = (float *)wtk_malloc(sizeof(float) * sweep_len);
    cfg->play_sweep = (short *)wtk_malloc(sizeof(short) * sweep_len);
    for (i = 0; i < sweep_len; ++i) {
        t1 = i * 1.0 / rate;
        cfg->sweep[i] = amp * sin(e1 * (exp(t1 / t * log(omega2 / omega1)) - 1));
        cfg->play_sweep[i] = floorf(cfg->sweep[i] * 32768.0 + 0.5);
    }

    k = -0.3 / log10(2.0);
    double *compensation = (double *)wtk_malloc(sizeof(double) * sweep_len);

    for (i = 0; i < sweep_len; ++i) {
        t1 = (sweep_len - 1 - i) * 1.0/ rate;
        w = 2 * M_PI * f1 * exp(t1 / t * log(f2 / f1));
        compensation[i] = pow(2 * M_PI * f2 / w, k);
        // printf("%.12f\n", compensation[i]);
        cfg->inv_sweep[i] = cfg->sweep[sweep_len - 1 - i] * compensation[i];
        // printf("%.12f\n", cfg->inv_sweep[i]);
    }
    wtk_free(compensation);

    // #include "wtk/core/wtk_wavfile.h"
    // wtk_wavfile_t *wav;
    // wav = wtk_wavfile_new(rate);
    // wtk_wavfile_set_channel(wav, 1);
    // wtk_wavfile_open(wav, "sweep.wav");
    // short *wav_data = (short *)wtk_malloc(sizeof(short) * sweep_len);
    // for (i = 0; i < sweep_len; ++i) {
    //     wav_data[i] = floor(cfg->inv_sweep[i] * 32768.0 + 0.5);
    // }
    // wtk_wavfile_write(wav, (char *)wav_data, sizeof(short) * sweep_len);
    // wtk_free(wav_data);
    // wtk_wavfile_close(wav);
}


int wtk_rt60_cfg_init(wtk_rt60_cfg_t *cfg) {
    cfg->channel = 0;
    cfg->nmicchannel = 0;
    cfg->mic_channel = NULL;
    cfg->wins = 1024;

    cfg->main_cfg = NULL;
    cfg->mbin_cfg = NULL;

    cfg->rate = 16000;

    cfg->f1 = 20.0;
    cfg->f2 = 8000.0;
    cfg->t = 8.0;
    cfg->amp = 0.2;

    cfg->play_sweep = NULL;
    cfg->sweep = NULL;
    cfg->inv_sweep = NULL;
    cfg->sweep_len = 0;

    return 0;
}

int wtk_rt60_cfg_clean(wtk_rt60_cfg_t *cfg) {
    if (cfg->mic_channel) {
        wtk_free(cfg->mic_channel);
    }
    if (cfg->play_sweep) {
        wtk_free(cfg->play_sweep);
    }
    if (cfg->sweep) {
        wtk_free(cfg->sweep);
    }
    if (cfg->inv_sweep) {
        wtk_free(cfg->inv_sweep);
    }
    return 0;
}

int wtk_rt60_cfg_update_local(wtk_rt60_cfg_t *cfg, wtk_local_cfg_t *m) {
    wtk_string_t *v;
    wtk_local_cfg_t *lc;
    int ret;
    wtk_array_t *a;
    int i;

    lc = m;
    wtk_local_cfg_update_cfg_i(lc, cfg, wins, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, channel, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, rate, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, f1, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, f2, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, t, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, amp, v);

    a = wtk_local_cfg_find_array_s(lc, "mic_channel");
    if (a) {
        cfg->mic_channel = (int *)wtk_malloc(sizeof(int) * a->nslot);
        cfg->nmicchannel = a->nslot;
        for (i = 0; i < a->nslot; ++i) {
            v = ((wtk_string_t **)a->slot)[i];
            cfg->mic_channel[i] = wtk_str_atoi(v->data, v->len);
        }
    }
    ret = 0;
end:
    return ret;
}

int wtk_rt60_cfg_update(wtk_rt60_cfg_t *cfg) {
    int ret;

    if (cfg->channel < cfg->nmicchannel) {
        cfg->channel = cfg->nmicchannel;
    }

    wtk_rt60_cfg_get_log_sweep(cfg);

    ret = 0;
end:
    return ret;
}

int wtk_rt60_cfg_update2(wtk_rt60_cfg_t *cfg, wtk_source_loader_t *sl) {
    return wtk_rt60_cfg_update(cfg);
}

wtk_rt60_cfg_t *wtk_rt60_cfg_new(char *fn) {
    wtk_main_cfg_t *main_cfg;
    wtk_rt60_cfg_t *cfg;

    main_cfg = wtk_main_cfg_new_type(wtk_rt60_cfg, fn);
    if (!main_cfg) {
        return NULL;
    }
    cfg = (wtk_rt60_cfg_t *)main_cfg->cfg;
    cfg->main_cfg = main_cfg;
    return cfg;
}

void wtk_rt60_cfg_delete(wtk_rt60_cfg_t *cfg) {
    wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_rt60_cfg_t *wtk_rt60_cfg_new_bin(char *fn) {
    wtk_mbin_cfg_t *mbin_cfg;
    wtk_rt60_cfg_t *cfg;

    mbin_cfg = wtk_mbin_cfg_new_type(wtk_rt60_cfg, fn, "./cfg");
    if (!mbin_cfg) {
        return NULL;
    }
    cfg = (wtk_rt60_cfg_t *)mbin_cfg->cfg;
    cfg->mbin_cfg = mbin_cfg;
    return cfg;
}

void wtk_rt60_cfg_delete_bin(wtk_rt60_cfg_t *cfg) {
    wtk_mbin_cfg_delete(cfg->mbin_cfg);
}