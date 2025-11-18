#include "wtk_equalizer2.h"
#ifndef WTK_WAV_SHORT_TO_FLOAT
#define WTK_WAV_SHORT_TO_FLOAT(f) ((f) > 0 ? (f / 32767.0) : (f / 32768.0))
#endif
#ifndef WTK_WAV_FLOAT_TO_SHORT
#define WTK_WAV_FLOAT_TO_SHORT(f)                                              \
    ((f) > 0 ? floorf(f * 32767.0 + 0.5) : floorf(f * 32768.0 + 0.5))
#endif

wtk_equalizer2_t *wtk_equalizer2_new(wtk_equalizer2_cfg_t *cfg) {
    wtk_equalizer2_t *eq = NULL;

    eq = wtk_malloc(sizeof(wtk_equalizer2_t));
    eq->cfg = cfg;
    eq->notify = NULL;
    eq->ths = NULL;

    if (cfg->use_pffft) {
        // 2的幂数
        eq->drft_wins = 1;
        while (eq->drft_wins < cfg->nfir) {
            eq->drft_wins <<= 1;
        }
        eq->drft_wins <<= 1;
        eq->drft = wtk_drft_new2(eq->drft_wins);
    } else {
        eq->drft_wins = cfg->nfir * 2;
        eq->drft = wtk_drft_new(eq->drft_wins);
    }
    eq->nbin = eq->drft_wins / 2 + 1;
    eq->mic = wtk_strbuf_new(1024, 1);
    eq->fir_fft =
        (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * eq->drft_wins);
    eq->fft =
        (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * eq->drft_wins);
    eq->fftx =
        (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * eq->drft_wins);
    eq->rfft_in = (float *)wtk_malloc(sizeof(float) * eq->drft_wins);
    eq->cache = (float *)wtk_malloc(sizeof(float) * cfg->nfir * 2);
    eq->out = (short *)wtk_malloc(sizeof(short) * eq->nbin);

    wtk_equalizer2_reset(eq);
    return eq;
}

void wtk_equalizer2_start(wtk_equalizer2_t *eq) {
    int wins = eq->drft_wins;
    int nbin = eq->nbin;
    wtk_complex_t *fir_fft = eq->fir_fft;
    int i;
    memcpy(eq->rfft_in, eq->cfg->cf, sizeof(float) * eq->cfg->nfir);
    if (eq->cfg->use_pffft) {
        wtk_drft_fft23(eq->drft, eq->rfft_in, fir_fft);
    } else {
        wtk_drft_fft2(eq->drft, eq->rfft_in, fir_fft);
        for (i = 0; i < nbin; ++i) {
            fir_fft[i].a *= wins;
            fir_fft[i].b *= wins;
        }
    }
}

void wtk_equalizer2_reset(wtk_equalizer2_t *eq) {
    wtk_strbuf_reset(eq->mic);
    memset(eq->fir_fft, 0, sizeof(wtk_complex_t) * eq->drft_wins);
    memset(eq->fft, 0, sizeof(wtk_complex_t) * eq->drft_wins);
    memset(eq->fftx, 0, sizeof(wtk_complex_t) * eq->drft_wins);
    memset(eq->rfft_in, 0, sizeof(float) * eq->drft_wins);
    memset(eq->cache, 0, sizeof(float) * eq->cfg->nfir * 2);
    memset(eq->out, 0, sizeof(short) * eq->nbin);
}

void wtk_equalizer2_delete(wtk_equalizer2_t *eq) {
    if (eq->cfg->use_pffft) {
        wtk_drft_delete2(eq->drft);
    } else {
        wtk_drft_delete(eq->drft);
    }
    wtk_strbuf_delete(eq->mic);
    wtk_free(eq->fir_fft);
    wtk_free(eq->fft);
    wtk_free(eq->fftx);
    wtk_free(eq->rfft_in);
    wtk_free(eq->cache);
    wtk_free(eq->out);
    wtk_free(eq);
}

void wtk_equalizer2_set_notify(wtk_equalizer2_t *eq, void *ths,
                               wtk_equalizer2_notify_f notify) {
    eq->notify = notify;
    eq->ths = ths;
}

void wtk_equalizer2_feed(wtk_equalizer2_t *eq, short *data, int len, int is_end) {
    wtk_strbuf_t *mic = eq->mic;
    wtk_complex_t *fir_fft = eq->fir_fft;
    wtk_complex_t *fft = eq->fft;
    wtk_complex_t *fftx = eq->fftx;
    float *rfft_in = eq->rfft_in;
    float *cache = eq->cache;
    short *out = eq->out;
    int i;
    int wins = eq->drft_wins;
    int fsize = eq->cfg->nfir;
    int nbin = eq->nbin;
    int length;
    float fv;
    float *fv1;
    for (i = 0; i < len; i++) {
        fv = WTK_WAV_SHORT_TO_FLOAT(data[i]);
        wtk_strbuf_push(eq->mic, (char *)(&fv), sizeof(float));
    }
    length = mic->pos / sizeof(float);

    while (length >= fsize) {
        fv1 = (float *)mic->data;
        memmove(cache, cache + fsize, sizeof(float) * fsize);
        memcpy(cache + fsize, fv1, sizeof(float) * fsize);
        memcpy(rfft_in, cache, sizeof(float) * fsize * 2);
        if (eq->cfg->use_pffft) {
            wtk_drft_fft23(eq->drft, rfft_in, fft);
        } else {
            wtk_drft_fft2(eq->drft, rfft_in, fft);
            for (i = 0; i < nbin; ++i) {
                fft[i].a *= wins;
                fft[i].b *= wins;
            }
        }
        for (i = 0; i < nbin; ++i) {
            fftx[i].a = fft[i].a * fir_fft[i].a + fft[i].b * fir_fft[i].b;
            fftx[i].b = - fft[i].a * fir_fft[i].b + fft[i].b * fir_fft[i].a;
            // printf("%f %f %f %f %f %f\n", fft[i].a, fft[i].b, fir_fft[i].a, fir_fft[i].b, fftx[i].a, fftx[i].b);
        }

        if (eq->cfg->use_pffft) {
            wtk_drft_ifft23(eq->drft, fftx, rfft_in);
        } else {
            for (i = 0; i < nbin; ++i) {
                fftx[i].a /= wins;
                fftx[i].b /= wins;
            }
            wtk_drft_ifft2(eq->drft, fftx, rfft_in);
        }

        for (i = 0; i < fsize; ++i) {
            out[i] = WTK_WAV_FLOAT_TO_SHORT(rfft_in[i]);
        }
        if (eq->notify) {
            eq->notify(eq->ths, out, fsize);
        }

        wtk_strbuf_pop(mic, NULL, sizeof(float) * fsize);
        length = mic->pos / sizeof(float);
    }
    if (is_end && length > 0) {
        if (eq->notify) {
            fv1 = (float *)mic->data;
            out = (short *)fv1;
            for (i = 0; i < length; ++i) {
                out[i] = WTK_WAV_FLOAT_TO_SHORT(fv1[i]);
            }
            eq->notify(eq->ths, out, length);
        }
    }
}