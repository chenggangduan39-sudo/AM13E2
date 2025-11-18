#ifndef WTK_BFIO_MASKFORM_WTK_GAINNET_BF6
#define WTK_BFIO_MASKFORM_WTK_GAINNET_BF6
#include "wtk_gainnet_bf6_cfg.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/bfio/maskdenoise/wtk_drft.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_strbuf.h"
#include "wtk/bfio/preemph/wtk_preemph.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_gainnet_bf6 wtk_gainnet_bf6_t;
typedef void(*wtk_gainnet_bf6_notify_f)(void *ths,short *output,int len);
typedef void (*wtk_gainnet_bf6_notify_trfeat_f)(void *ths,float *feat,int len,float *target_g,int g_len,float *target_gr,int gr_len,float *target_gn,int gn_len);

typedef struct
{
	wtk_gainnet_bf6_cfg_t *cfg;
    int nbin;
    int *eband;

    float  *Ex;

    float *dct_table;
    float **cepstral_mem;
    int memid;
    
    float  *Ex2;
    float **cepstral_mem2;
    int memid2;

    float *g;
    float *gf;

    float *g2;
    float *gf2;

    float *Ly;
    float *features;
    wtk_gainnet2_t *gainnet;
    wtk_masknet_t *masknet;

    unsigned silence:1;
}wtk_gainnet_bf6_mask_t;

typedef struct
{
	wtk_gainnet_bf6_cfg_t *cfg;
    int nbin;
    float *g;
    float *features;
    int features_len;

    wtk_gainnet2_t *gainnet;
    wtk_masknet_t *masknet;
}wtk_gainnet_bf6_mask2_t;

struct wtk_gainnet_bf6
{
	wtk_gainnet_bf6_cfg_t *cfg;

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
    wtk_complex_t **fft2;
    float *aspec_mask;

    wtk_aspec_t *aspec;
    wtk_complex_t *cov;
    float *winf;
    wtk_complex_t *inv_cov;
	wtk_dcomplex_t *tmp;
    
    wtk_complex_t **ovec;

    float *angle_spec;

    int thetas;
    int thetad;

	wtk_covm_t *covm;
	wtk_bf_t *bf;

    wtk_complex_t *fftx;

    wtk_gainnet_bf6_mask_t *gmask;
    wtk_gainnet_bf6_mask2_t *gmask2;

    wtk_qmmse_t *qmmse;
	float *out;

	void *ths;
	wtk_gainnet_bf6_notify_f notify;

    void *ths_tr;
    wtk_gainnet_bf6_notify_trfeat_f notify_tr;

    unsigned training:1;
};

wtk_gainnet_bf6_t* wtk_gainnet_bf6_new(wtk_gainnet_bf6_cfg_t *cfg);
void wtk_gainnet_bf6_delete(wtk_gainnet_bf6_t *gainnet_bf6);
void wtk_gainnet_bf6_start(wtk_gainnet_bf6_t *gainnet_bf6, float theta, float phi);
void wtk_gainnet_bf6_reset(wtk_gainnet_bf6_t *gainnet_bf6);
void wtk_gainnet_bf6_set_notify(wtk_gainnet_bf6_t *gainnet_bf6,void *ths,wtk_gainnet_bf6_notify_f notify);
/**
 * len=mic array samples
 */
void wtk_gainnet_bf6_feed(wtk_gainnet_bf6_t *gainnet_bf6,short **data,int len,int is_end);

void wtk_gainnet_bf6_set_tr_notify(wtk_gainnet_bf6_t *gainnet_bf6,void *ths,wtk_gainnet_bf6_notify_trfeat_f notify);

void wtk_gainnet_bf6_feed_train3(wtk_gainnet_bf6_t *gainnet_bf6,short **data,short **data2,short **datar, int len, int bb);
void wtk_gainnet_bf6_feed_train4(wtk_gainnet_bf6_t *gainnet_bf6,short **data,short **data2,short **datar, int len, int bb);
#ifdef __cplusplus
};
#endif
#endif
