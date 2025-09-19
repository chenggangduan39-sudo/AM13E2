#include "qtk/ult/qtk_ult_ofdm_cfg.h"
#include "wtk/core/fft/fft.h"
#include "wtk/core/fft/wtk_rfft.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_alloc.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/core/wtk_str.h"

static void gen_zc_seq_(int N, int u, int q, wtk_complex_t *zc) {
    for (int i = 0; i < N; i++) {
        float f = PI * u * i * (i + 1.0 + 2.0 * q) / N;
        zc[i].a = cos(f);
        zc[i].b = sin(-f);
    }
}

int qtk_ult_ofdm_cfg_init(qtk_ult_ofdm_cfg_t *cfg) {
    cfg->nsymbols = 64;
    cfg->sampling_rate = 48000;
    cfg->central_freq = 22500;
    cfg->period = 2048;
    cfg->gap_frame = 4;
    cfg->hann_L = 256;
    return 0;
}

int qtk_ult_ofdm_cfg_clean(qtk_ult_ofdm_cfg_t *cfg) {
    wtk_free(cfg->wav);
    wtk_free(cfg->wav_freq);
    wtk_free(cfg->power_spec);
    wtk_free(cfg->zc_seq);
    return 0;
}

static void gen_ofdm_freq_(qtk_ult_ofdm_cfg_t *cfg, int st, int et) {
    int L, l;
    wtk_rfft_t *fft;
    float maxv = 0;

    L = cfg->period * cfg->gap_frame;
    float *w = wtk_calloc(sizeof(float), L);
    float *wf = wtk_malloc(sizeof(float) * L);

    l = et - st + 1;
    st *= cfg->gap_frame;
    et = st + l * cfg->gap_frame;

    l = et - st;

    cfg->st = st;
    cfg->et = et;

    cfg->wav_freq = wtk_malloc(sizeof(wtk_complex_t) * l);
    cfg->power_spec = wtk_malloc(sizeof(float) * l);

    fft = wtk_rfft_new(L / 2);
    memcpy(w, cfg->wav, sizeof(float) * cfg->period);
    wtk_rfft_process_fft(fft, wf, w);
    for (int i = st; i < et; i++) {
        float f;
        wtk_complex_t c = wtk_rfft_get_value(wf, L, i);
        cfg->wav_freq[i - st] = c;
        f = c.a * c.a + c.b * c.b;
        cfg->power_spec[i - st] = f;
        if (f > maxv) {
            maxv = f;
        }
    }

    for (int i = 0; i < l; i++) {
        cfg->power_spec[i] += 0.01 * maxv;
    }

    wtk_free(wf);
    wtk_rfft_delete(fft);
    wtk_free(w);
}

int qtk_ult_ofdm_cfg_update(qtk_ult_ofdm_cfg_t *cfg) {
    int left_hand, right_hand;
    int st, et, st1, et1;
    int n;
    float scale;
    float *win;

    cfg->wav = wtk_malloc(sizeof(float) * cfg->period);
    cfg->zc_seq = wtk_malloc(sizeof(wtk_complex_t) * cfg->nsymbols);
    wtk_complex_t *buf = wtk_calloc(sizeof(wtk_complex_t), cfg->period);
    gen_zc_seq_(cfg->nsymbols, 1, 0, cfg->zc_seq);
    wtk_complex_fft(cfg->zc_seq, cfg->nsymbols);
    left_hand = (cfg->nsymbols - 1) / 2;
    right_hand = cfg->nsymbols - left_hand - 1;
    st = cfg->central_freq * cfg->period / cfg->sampling_rate - left_hand - 1;
    et = cfg->central_freq * cfg->period / cfg->sampling_rate + right_hand - 1;
    memcpy(buf + st, cfg->zc_seq, sizeof(wtk_complex_t) * cfg->nsymbols);

    st1 = cfg->period - et;
    et1 = cfg->period - st + 1;
    for (int i = st1, j = cfg->nsymbols - 1; i < et1; i++, j--) {
        buf[i].a = cfg->zc_seq[j].a;
        buf[i].b = -cfg->zc_seq[j].b;
    }

    wtk_cdft(cfg->period * 2, 1, (float *)buf);

    scale = 1.0 / cfg->period;

    for (int i = 0; i < cfg->period; i++) {
        cfg->wav[i] = buf[i].a * scale;
    }

    win = wtk_math_create_hanning_window2(cfg->hann_L);
    n = cfg->hann_L / 2;
    for (int i = 0; i < n; i++) {
        cfg->wav[i] *= win[i];
    }
    for (int i = n; i < cfg->period - n; i++) {
        cfg->wav[i] *= win[n];
    }
    for (int i = cfg->period - n, j = n; i < cfg->period; i++, j++) {
        cfg->wav[i] *= win[j];
    }

    gen_ofdm_freq_(cfg, st, et);

    wtk_free(buf);
    wtk_free(win);
    return 0;
}

int qtk_ult_ofdm_cfg_update_local(qtk_ult_ofdm_cfg_t *cfg,
                                  wtk_local_cfg_t *lc) {
    wtk_string_t *v;

    wtk_local_cfg_update_cfg_i(lc, cfg, nsymbols, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, sampling_rate, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, central_freq, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, period, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, gap_frame, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, hann_L, v);

    return 0;
}
