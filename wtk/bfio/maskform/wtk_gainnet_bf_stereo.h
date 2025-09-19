#ifndef WTK_BFIO_MASKFORM_WTK_GAINNET_BF_STEREO
#define WTK_BFIO_MASKFORM_WTK_GAINNET_BF_STEREO
#include "wtk_gainnet_bf_stereo_cfg.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/bfio/maskdenoise/wtk_drft.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_strbuf.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_gainnet_bf_stereo wtk_gainnet_bf_stereo_t;
typedef void(*wtk_gainnet_bf_stereo_notify_f)(void *ths,short **output,int len);

typedef struct
{
	wtk_gainnet_bf_stereo_cfg_t *cfg;
    int nbin;

    wtk_bankfeat_t *bank_mic;

    float *g;
    float *lastg;
    float *gf;

    float *g2;
    float *lastg2;  
    float *gf2;

    float *feature_lm;
    
    wtk_gainnet2_t *gainnet2;
    wtk_gainnet_t *agc_gainnet;
    wtk_qmmse_t *qmmse;
}wtk_gainnet_bf_stereo_dra_t;


struct wtk_gainnet_bf_stereo
{
	wtk_gainnet_bf_stereo_cfg_t *cfg;

	wtk_strbuf_t **mic;

    float *analysis_window;
    float *synthesis_window;
	wtk_drft_t *rfft;
	float *rfft_in;
	int nbin;
    float **analysis_mem;
    float **analysis_mem_sp;
	float **synthesis_mem;

	wtk_complex_t **fft;

    wtk_gainnet_bf_stereo_dra_t *gbfsdr;
	wtk_covm_t **covm;
	wtk_bf_t **bf;
    wtk_complex_t **fftx;
	float **out;
    wtk_equalizer_t **eq;

	void *ths;
	wtk_gainnet_bf_stereo_notify_f notify;

    float *scale;
    float *last_scale;
	int *max_cnt;
    
    int mic_silcnt;
    unsigned mic_sil:1;
};

wtk_gainnet_bf_stereo_t* wtk_gainnet_bf_stereo_new(wtk_gainnet_bf_stereo_cfg_t *cfg);
void wtk_gainnet_bf_stereo_delete(wtk_gainnet_bf_stereo_t *vboxebf_stereo);
void wtk_gainnet_bf_stereo_start(wtk_gainnet_bf_stereo_t *vboxebf_stereo);
void wtk_gainnet_bf_stereo_reset(wtk_gainnet_bf_stereo_t *vboxebf_stereo);
void wtk_gainnet_bf_stereo_set_notify(wtk_gainnet_bf_stereo_t *vboxebf_stereo,void *ths,wtk_gainnet_bf_stereo_notify_f notify);
/**
 * len=mic array samples
 */
void wtk_gainnet_bf_stereo_feed(wtk_gainnet_bf_stereo_t *vboxebf_stereo,short *data,int len,int is_end);

#ifdef __cplusplus
};
#endif
#endif
