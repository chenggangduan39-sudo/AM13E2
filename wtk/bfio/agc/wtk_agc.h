#ifndef WTK_BFIO_AGC_WTK_AGC
#define WTK_BFIO_AGC_WTK_AGC
#include "wtk_agc_cfg.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/bfio/maskdenoise/wtk_drft.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_strbuf.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_agc wtk_agc_t;
typedef void(*wtk_agc_notify_f)(void *ths,short *output,int len);

struct wtk_agc
{
	wtk_agc_cfg_t *cfg;

	wtk_strbuf_t **mic;

    float *analysis_window;
    float *synthesis_window;
	wtk_drft_t *rfft;
	float *rfft_in;
	int nbin;
    float **analysis_mem;
	float *synthesis_mem;

	wtk_complex_t **fft;
    wtk_complex_t *fftx;

    wtk_qmmse_t *qmmse;

	float *out;

    wtk_equalizer_t *eq;

	void *ths;
	wtk_agc_notify_f notify;

    int mic_silcnt;

    float bs_scale;
    float bs_last_scale;
    float bs_real_scale;
    int bs_max_cnt;
    float *bs_win;

    unsigned mic_sil:1;
    unsigned int denoise_enable:1;
};

wtk_agc_t* wtk_agc_new(wtk_agc_cfg_t *cfg);
void wtk_agc_delete(wtk_agc_t *agc);
void wtk_agc_start(wtk_agc_t *agc);
void wtk_agc_reset(wtk_agc_t *agc);
void wtk_agc_set_notify(wtk_agc_t *agc,void *ths,wtk_agc_notify_f notify);
/**
 * len=mic array samples
 */
void wtk_agc_feed(wtk_agc_t *agc,short *data,int len,int is_end);

void wtk_agc_set_denoiseenable(wtk_agc_t *agc,int enable);
void wtk_agc_set_noise_suppress(wtk_agc_t *agc,float suppress); //[0-100]
#ifdef __cplusplus
};
#endif
#endif
