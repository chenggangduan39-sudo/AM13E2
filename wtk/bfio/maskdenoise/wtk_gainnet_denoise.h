#ifndef WTK_BFIO_MASKDENOISE_WTK_GAINNET_DENOISE
#define WTK_BFIO_MASKDENOISE_WTK_GAINNET_DENOISE
#include "wtk_gainnet_denoise_cfg.h"
#include "wtk_drft.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_strbuf.h"
#include "pitch.h"
#include "wtk/bfio/preemph/wtk_preemph.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef void (*wtk_gainnet_denoise_notify_trfeat_f)(void *ths,float *feat,int len,float *target_g,int g_len,float  vad);
typedef void (*wtk_gainnet_denoise_notify_f)(void *ths,short *data,int len,int is_end);
typedef void (*wtk_gainnet_denoise_notify_f2)(void *ths, wtk_complex_t *fft,
                                              int is_end);

typedef struct wtk_gainnet_denoise wtk_gainnet_denoise_t;
struct wtk_gainnet_denoise
{
    wtk_gainnet_denoise_cfg_t *cfg;

    wtk_strbuf_t *input;

    float *rfft_in;
    wtk_drft_t *rfft;
    wtk_complex_t *fft;
    float *window;
    float *synthesis_window;
    float *analysis_mem;
    float *synthesis_mem;

    wtk_bankfeat_t *bank_mic;

    float *g;
    float *lastg;
    float *gf;
    float *g2;
    float *gf2;

    int nbin;

    wtk_gainnet7_t *gainnet;
    wtk_gainnet3_t *gainnet3;
    wtk_qmmse_t *qmmse;

    float *pad;

    float *out;

    wtk_gainnet_denoise_notify_trfeat_f notify_tr;
    void *ths_tr;

    wtk_gainnet_denoise_notify_f notify;
    void *ths;

    wtk_gainnet_denoise_notify_f2 notify2;
    void *ths2;
};

wtk_gainnet_denoise_t *wtk_gainnet_denoise_new(wtk_gainnet_denoise_cfg_t *cfg);
void wtk_gainnet_denoise_delete(wtk_gainnet_denoise_t *gdenoise);
void wtk_gainnet_denoise_reset(wtk_gainnet_denoise_t *gdenoise);
void wtk_gainnet_denoise_feed(wtk_gainnet_denoise_t *gdenoise,short *data,int len,int is_end);
void wtk_gainnet_denoise_set_notify(wtk_gainnet_denoise_t *gdenoise,void *ths,wtk_gainnet_denoise_notify_f notify);

wtk_gainnet_denoise_t *wtk_gainnet_denoise_new2(wtk_gainnet_denoise_cfg_t *cfg);
void wtk_gainnet_denoise_delete2(wtk_gainnet_denoise_t *gdenoise);
void wtk_gainnet_denoise_reset2(wtk_gainnet_denoise_t *gdenoise);
void wtk_gainnet_denoise_feed2(wtk_gainnet_denoise_t *gdenoise,
                               wtk_complex_t *infft, int is_end);
void wtk_gainnet_denoise_set_notify2(wtk_gainnet_denoise_t *gdenoise, void *ths,
                                     wtk_gainnet_denoise_notify_f2 notify);

void wtk_gainnet_denoise_set_notify_tr(wtk_gainnet_denoise_t *gdenoise,void *ths,wtk_gainnet_denoise_notify_trfeat_f notify);

void wtk_gainnet_denoise_feed_train(wtk_gainnet_denoise_t *gdenoise,short **data,int len);

#ifdef __cplusplus
};
#endif
#endif