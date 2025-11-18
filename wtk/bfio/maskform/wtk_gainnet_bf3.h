#ifndef WTK_BFIO_MASKFORM_WTK_GAINNET_BF3
#define WTK_BFIO_MASKFORM_WTK_GAINNET_BF3
#include "wtk/core/wtk_strbuf.h"
#include "wtk_gainnet_bf3_cfg.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/bfio/maskdenoise/wtk_drft.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/bfio/preemph/wtk_preemph.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_gainnet_bf3 wtk_gainnet_bf3_t;
typedef void(*wtk_gainnet_bf3_notify_f)(void *ths,short *output,int len);
typedef void (*wtk_gainnet_bf3_notify_ssl_f)(void *ths,wtk_ssl2_extp_t *nbest_extp,int nbest);
typedef void (*wtk_gainnet_bf3_notify_trfeat_f)(void *ths,float *feat,int len,float *target_agc,float *target_g,float *target_ns,int g_len);

typedef struct
{
	wtk_gainnet_bf3_cfg_t *cfg;
    int nbin;

    wtk_bankfeat_t *bank_mic;
    float *xvec;

    float *g;
    float *lastg;
    float *gf;

    float *g2;
    float *lastg2;  
    float *gf2;

    float *feature_lm;

    wtk_gainnet7_t *gainnet;
    wtk_gainnet2_t *gainnet2;
    wtk_gainnet4_t *gainnet4;

    wtk_gainnet_t *agc_gainnet;

    wtk_qmmse_t *qmmse;
}wtk_gainnet_bf3_denoise_t;


struct wtk_gainnet_bf3
{
	wtk_gainnet_bf3_cfg_t *cfg;

	wtk_strbuf_t **mic;

    float *analysis_window;
    float *synthesis_window;
	wtk_drft_t *rfft;
	float *rfft_in;
	int nbin;
    float **analysis_mem;
	float *synthesis_mem;

	wtk_complex_t **fft;

    float *howl_energy;
    float *howl_scale;
    float *howl_power_x;
    int nframe;
    wtk_complex_t **howl_xld;
    wtk_complex_t **howl_W;
    int *howl_cnt;

    wtk_gainnet_bf3_denoise_t *gdenoise;

    float pframe;

	wtk_covm_t *covm;
	wtk_bf_t *bf;

    wtk_complex_t *fftx;

    wtk_maskssl_t *maskssl;
    wtk_maskssl2_t *maskssl2;

	float *out;

    wtk_equalizer_t *eq;

	void *ths;
	wtk_gainnet_bf3_notify_f notify;

    void *ths_tr;
    wtk_gainnet_bf3_notify_trfeat_f notify_tr;

    void *ssl_ths;
	wtk_gainnet_bf3_notify_ssl_f notify_ssl;

    int mic_silcnt;
    unsigned mic_sil:1;

    unsigned agc_enable:1;
    unsigned denoise_enable:1;   
};

wtk_gainnet_bf3_t* wtk_gainnet_bf3_new(wtk_gainnet_bf3_cfg_t *cfg);
void wtk_gainnet_bf3_delete(wtk_gainnet_bf3_t *gainnet_bf3);
void wtk_gainnet_bf3_start(wtk_gainnet_bf3_t *gainnet_bf3);
void wtk_gainnet_bf3_start_xv(wtk_gainnet_bf3_t *gainnet_bf3, char *xv_fn);
void wtk_gainnet_bf3_reset(wtk_gainnet_bf3_t *gainnet_bf3);
void wtk_gainnet_bf3_set_notify(wtk_gainnet_bf3_t *gainnet_bf3,void *ths,wtk_gainnet_bf3_notify_f notify);
void wtk_gainnet_bf3_set_tr_notify(wtk_gainnet_bf3_t *gainnet_bf3,void *ths,wtk_gainnet_bf3_notify_trfeat_f notify);
void wtk_gainnet_bf3_set_ssl_notify(wtk_gainnet_bf3_t *gainnet_bf3,void *ths,wtk_gainnet_bf3_notify_ssl_f notify);
/**
 * len=mic array samples
 */
void wtk_gainnet_bf3_feed(wtk_gainnet_bf3_t *gainnet_bf3,short *data,int len,int is_end);

void wtk_gainnet_bf3_feed_howl(wtk_gainnet_bf3_t *gainnet_bf3,short *data,int len,int is_end);

void wtk_gainnet_bf3_feed_train(wtk_gainnet_bf3_t *gainnet_bf3,short **data,int len,int channel, int bb);
void wtk_gainnet_bf3_feed_train2(wtk_gainnet_bf3_t *gainnet_bf3,short **data,int len,int channel, int bb);

void wtk_gainnet_bf3_set_agcenable(wtk_gainnet_bf3_t *gainnet_bf3,int enable);
void wtk_gainnet_bf3_set_denoiseenable(wtk_gainnet_bf3_t *gainnet_bf3,int enable);
void wtk_gainnet_bf3_set_denoisesuppress(wtk_gainnet_bf3_t *gainnet_bf3,float suppress); //[0-1]
#ifdef __cplusplus
};
#endif
#endif
