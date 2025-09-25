#ifndef WTK_BFIO_MASKFORM_WTK_gainnet_bf7
#define WTK_BFIO_MASKFORM_WTK_gainnet_bf7
#include "wtk/core/wtk_strbuf.h"
#include "wtk_gainnet_bf7_cfg.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/bfio/maskdenoise/wtk_drft.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_strbuf.h"
#include "wtk/bfio/preemph/wtk_preemph.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_gainnet_bf7 wtk_gainnet_bf7_t;
typedef void(*wtk_gainnet_bf7_notify_f)(void *ths,short *output,int len);
typedef void (*wtk_gainnet_bf7_notify_trfeat_f)(void *ths,float *feat,int len,float *target_g,int g_len);

struct wtk_gainnet_bf7
{
	wtk_gainnet_bf7_cfg_t *cfg;

	wtk_strbuf_t **mic;
	float **notch_mem;
	float *memD;
	float memX;

    float *window;
    float *synthesis_window;
	wtk_drft_t *rfft;
	float *rfft_in;
	int nbin;
    float **analysis_mem;
	float *synthesis_mem;

	wtk_complex_t **fft;
    wtk_complex_t ***ovec;

    float *feature;
    wtk_masknet_t *masknet;
    float *g;

	wtk_covm_t *covm;
	wtk_bf_t *bf;

    wtk_complex_t *fftx;
	float *out;

	void *ths;
	wtk_gainnet_bf7_notify_f notify;

    void *ths_tr;
    wtk_gainnet_bf7_notify_trfeat_f notify_tr;

    unsigned training:1;
};

wtk_gainnet_bf7_t* wtk_gainnet_bf7_new(wtk_gainnet_bf7_cfg_t *cfg);
void wtk_gainnet_bf7_delete(wtk_gainnet_bf7_t *gainnet_bf7);
void wtk_gainnet_bf7_start(wtk_gainnet_bf7_t *gainnet_bf7, float theta, float phi);
void wtk_gainnet_bf7_reset(wtk_gainnet_bf7_t *gainnet_bf7);
void wtk_gainnet_bf7_set_notify(wtk_gainnet_bf7_t *gainnet_bf7,void *ths,wtk_gainnet_bf7_notify_f notify);
/**
 * len=mic array samples
 */
void wtk_gainnet_bf7_feed(wtk_gainnet_bf7_t *gainnet_bf7,short **data,int len,int is_end);

void wtk_gainnet_bf7_set_tr_notify(wtk_gainnet_bf7_t *gainnet_bf7,void *ths,wtk_gainnet_bf7_notify_trfeat_f notify);

void wtk_gainnet_bf7_feed_train3(wtk_gainnet_bf7_t *gainnet_bf7,short **data,short **data2, short **datar, int len, int bb);

#ifdef __cplusplus
};
#endif
#endif
