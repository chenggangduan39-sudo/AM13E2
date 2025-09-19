#ifndef WTK_BFIO_MASKFORM_WTK_GAINNET_BF
#define WTK_BFIO_MASKFORM_WTK_GAINNET_BF
#include "wtk_gainnet_bf_cfg.h"
#include "wtk/bfio/maskdenoise/wtk_drft.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_strbuf.h"
#include "wtk/bfio/maskdenoise/pitch.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_gainnet_bf wtk_gainnet_bf_t;
typedef void (*wtk_gainnet_bf_notify_f)(void *ths,short *data,int len,int is_end);
typedef void (*wtk_gainnet_bf_notify_ssl_f)(void *ths,wtk_ssl2_extp_t *nbest_extp,int nbest);

typedef struct
{
	wtk_gainnet_bf_cfg_t *cfg;
    int nbin;

    wtk_complex_t *fft_p;
    float  *Ex;
    float *Ep;
    float *Exp;
    float *Exp_dct;

    float *dct_table;
    float **cepstral_mem;
    int memid;
    
    float *pitch_buf;
    float *pitch_buf_tmp;
    float last_gain;
    int last_period;

    float *g;
    float *lastg;
    float *gf;

    float *r;
    float *rf;
    float *newE;
    float *norm;
    float *normf;

    float *Ly;
    float *features;
    wtk_gainnet_t *gainnet;
    wtk_qmmse_t *qmmse;
}wtk_gainnet_bf_denoise_t;

typedef struct
{
	wtk_gainnet_bf_cfg_t *cfg;
    int nbin;

    int *eband;
    float  *Ex;

    float *dct_table;
    float **cepstral_mem;
    int memid; 

    float *g;
    float *gf;

    float *Ly;
    float *features;
    wtk_gainnet_t *gainnet;
}wtk_gainnet_bf_agc_t;


struct wtk_gainnet_bf
{
	wtk_gainnet_bf_cfg_t *cfg;

	wtk_strbuf_t **mic;

    float **mem_hp_x;
    float **a_hp;
    float **b_hp;
    float *half_window;
    wtk_drft_t *rfft;
	float *rfft_in;
	int nbin;
    float **analysis_mem;
	float *synthesis_mem;

	wtk_complex_t **fft;
    wtk_complex_t **ffts;
    wtk_complex_t **ffty;
    
    wtk_gainnet_bf_denoise_t *qdenoise;

    float pframe;
    wtk_gainnet_bf_agc_t *agc;

    float *out;
   
    wtk_qenvelope_t *qenvl;
    wtk_ssl2_t *ssl2;

    wtk_covm_t *covm;
    wtk_bf_t *bf;

	void *ths;
	wtk_gainnet_bf_notify_f notify;

    void *ssl_ths;
    wtk_gainnet_bf_notify_ssl_f notify_ssl;

    unsigned sil:1;
};

wtk_gainnet_bf_t* wtk_gainnet_bf_new(wtk_gainnet_bf_cfg_t *cfg);
void wtk_gainnet_bf_delete(wtk_gainnet_bf_t *gainnet_bf);
void wtk_gainnet_bf_start(wtk_gainnet_bf_t *gainnet_bf);
void wtk_gainnet_bf_reset(wtk_gainnet_bf_t *gainnet_bf);
void wtk_gainnet_bf_set_notify(wtk_gainnet_bf_t *gainnet_bf,void *ths,wtk_gainnet_bf_notify_f notify);
void wtk_gainnet_bf_set_ssl_notify(wtk_gainnet_bf_t *gainnet_bf,void *ths,wtk_gainnet_bf_notify_ssl_f notify);

void wtk_gainnet_bf_feed(wtk_gainnet_bf_t *gainnet_bf,short **data,int len,int is_end);

#ifdef __cplusplus
};
#endif
#endif
