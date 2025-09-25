#ifndef WTK_BFIO_AGC_WTK_AGC2
#define WTK_BFIO_AGC_WTK_AGC2
#include "wtk_agc2_cfg.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/bfio/maskdenoise/wtk_drft.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_strbuf.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_agc2 wtk_agc2_t;
typedef void(*wtk_agc2_notify_f)(void *ths,short *output,int len);

struct wtk_agc2
{
	wtk_agc2_cfg_t *cfg;

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

    float *audgram;
    float *fbg;
    float *state;
    float *D;
    float *E;
    float min_E;

	float *out;

    wtk_equalizer_t *eq;

	void *ths;
	wtk_agc2_notify_f notify;

    int mic_silcnt;

    float bs_scale;
    float bs_last_scale;
    int bs_max_cnt;

    unsigned mic_sil:1;
};

wtk_agc2_t* wtk_agc2_new(wtk_agc2_cfg_t *cfg);
void wtk_agc2_delete(wtk_agc2_t *agc2);
void wtk_agc2_start(wtk_agc2_t *agc2);
void wtk_agc2_reset(wtk_agc2_t *agc2);
void wtk_agc2_set_notify(wtk_agc2_t *agc2,void *ths,wtk_agc2_notify_f notify);
/**
 * len=mic array samples
 */
void wtk_agc2_feed(wtk_agc2_t *agc2,short *data,int len,int is_end);
void wtk_agc2_feed_tf_agc(wtk_agc2_t *agc2, wtk_complex_t *fftx);
#ifdef __cplusplus
};
#endif
#endif
