#ifndef WTK_EQUALIZER2_H
#define WTK_EQUALIZER2_H
#include "wtk/bfio/maskdenoise/wtk_drft.h"
#include "wtk/core/wtk_strbuf.h"
#include "wtk_equalizer2_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef void (*wtk_equalizer2_notify_f)(void *ths, short *output, int len);

typedef struct wtk_equalizer2 {
    wtk_equalizer2_cfg_t *cfg;

    wtk_drft_t *drft;
    int drft_wins;
    int nbin;

    wtk_strbuf_t *mic;

    wtk_complex_t *fir_fft;
    wtk_complex_t *fft;
    wtk_complex_t *fftx;
    float *rfft_in;
    float *cache;

    wtk_equalizer2_notify_f notify;
    void *ths;

    short *out;

} wtk_equalizer2_t;

wtk_equalizer2_t *wtk_equalizer2_new(wtk_equalizer2_cfg_t *cfg);
void wtk_equalizer2_start(wtk_equalizer2_t *eq);
void wtk_equalizer2_reset(wtk_equalizer2_t *eq);
void wtk_equalizer2_feed(wtk_equalizer2_t *eq, short *data, int len, int is_end);
void wtk_equalizer2_delete(wtk_equalizer2_t *equalizer2);
void wtk_equalizer2_set_notify(wtk_equalizer2_t *equalizer2, void *ths,
                               wtk_equalizer2_notify_f notify);

#ifdef __cplusplus
};
#endif

#endif