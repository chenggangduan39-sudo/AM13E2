#include "qtk/ult/fmcw/qtk_ult_fmcw_cfg.h"
#include "wtk/core/cfg/wtk_local_cfg.h"

static void gen_fmwc_signal_(qtk_ult_fmcw_cfg_t *cfg, float start_freq,
                             float end_freq, wtk_complex_t *sweep) {
    int i;
    float period = cfg->nsymbols;
    float duration = (float)(period) / cfg->sampling_rate;
    float slope = (end_freq - start_freq) / duration;
    int leftwin = (cfg->winsize - 1.0) / 2 + 1;
    int rightwin = cfg->winsize - leftwin;
    float *win = NULL;
    for (i = 0; i < period; i++) {
        float tidx = i / (float)cfg->sampling_rate;
        float f = 2.0 * M_PI * (start_freq * tidx + 0.5 * slope * tidx * tidx);
        sweep[i].a = cos(f);
        sweep[i].b = sin(f);
    }

    win = wtk_math_create_hanning_window2(cfg->winsize);
    if (period == cfg->winsize) {
        for (i = 0; i < period; i++) {
            sweep[i].a *= win[i];
            sweep[i].b *= win[i];
        }
    } else {
        for (i = 0; i < leftwin; i++) {
            sweep[i].a *= win[i];
            sweep[i].b *= win[i];
        }
        for (i = leftwin - 1; i < period - rightwin; i++) {
            sweep[i].a *= win[leftwin];
            sweep[i].b *= win[leftwin];
        }
        for (i = cfg->period - rightwin; i < period; i++) {
            sweep[i].a *= win[cfg->winsize - rightwin + i];
            sweep[i].b *= win[cfg->winsize - rightwin + i];
        }
    }
    wtk_free(win);
}

static void gen_carrier_(qtk_ult_fmcw_cfg_t *cfg) {
    int i;
    for (i = 0; i < cfg->period; i++) {
        float f = 2 * M_PI * cfg->central_freq * i / cfg->sampling_rate;
        cfg->carrier[i].a = cos(f) / 32768.0;
        cfg->carrier[i].b = -sin(f) / 32768.0;
    }
}

int qtk_ult_fmcw_cfg_init(qtk_ult_fmcw_cfg_t *cfg) {
    cfg->nsymbols = 1024;
    cfg->sampling_rate = 96e3;
    cfg->central_freq = 40e3;
    cfg->bandwidth = 10e3;
    cfg->period = 8192;
    cfg->winsize = 1024;
    cfg->subsample_factor = 64;
    cfg->nsamples = 64;
    cfg->sweep = NULL;
    cfg->carrier = NULL;
    cfg->dis_start = 0;
    cfg->dis_end = 12;
    cfg->channel = 2;
    cfg->peak_thresh = 9e16 / (32768 * 32768);
    cfg->sync_pos_tolerance = 5;
    return 0;
}

int qtk_ult_fmcw_cfg_clean(qtk_ult_fmcw_cfg_t *cfg) {
    wtk_free(cfg->sweep);
    wtk_free(cfg->filter_params);
    wtk_free(cfg->carrier);
    return 0;
}

int qtk_ult_fmcw_cfg_update(qtk_ult_fmcw_cfg_t *cfg) {
    float max_range;
    int i;
    float start_freq = -1.0 * cfg->bandwidth / 2.0;
    float end_freq = -1.0 * start_freq;
    cfg->sweep = wtk_malloc(sizeof(wtk_complex_t) * cfg->nsymbols);
    cfg->filter_params = wtk_malloc(sizeof(wtk_complex_t) * cfg->nsymbols);
    gen_fmwc_signal_(cfg, start_freq, end_freq, cfg->sweep);
    for (i = 0; i < cfg->nsymbols; i++) {
        cfg->filter_params[i].a = cfg->sweep[cfg->nsymbols - 1 - i].a * 0.5;
        cfg->filter_params[i].b = -cfg->sweep[cfg->nsymbols - 1 - i].b * 0.5;
    }

    cfg->carrier = wtk_malloc(sizeof(wtk_complex_t) * cfg->period);
    gen_carrier_(cfg);
    max_range = 1.0 / cfg->sampling_rate * cfg->subsample_factor *
                cfg->nsamples * 343.0 / 2;
    cfg->range_unit = max_range / cfg->nsamples;
    return 0;
}

int qtk_ult_fmcw_cfg_update_local(qtk_ult_fmcw_cfg_t *cfg,
                                  wtk_local_cfg_t *lc) {
    wtk_string_t *v;
    wtk_local_cfg_update_cfg_i(lc, cfg, nsymbols, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, sampling_rate, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, central_freq, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, bandwidth, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, period, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, winsize, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, subsample_factor, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, nsamples, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, channel, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, dis_start, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, dis_end, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, peak_thresh, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, sync_pos_tolerance, v);
    return 0;
}

int qtk_ult_fmcw_cfg_update2(qtk_ult_fmcw_cfg_t *cfg, wtk_source_loader_t *sl) {
    return qtk_ult_fmcw_cfg_update(cfg);
}

int qtk_ult_fmcw_cfg_get_signal(qtk_ult_fmcw_cfg_t *cfg, float fc, float scale,
                                float *wav) {
    int i;
    float start_freq = fc - cfg->bandwidth / 2.0;
    float end_freq = fc + cfg->bandwidth / 2.0;
    wtk_complex_t *sweep = wtk_malloc(sizeof(wtk_complex_t) * cfg->nsymbols);
    gen_fmwc_signal_(cfg, start_freq, end_freq, sweep);
    for (i = 0; i < cfg->nsymbols; i++) {
        wav[i] = sweep[i].a * scale;
    }
    wtk_free(sweep);
    return 0;
}
