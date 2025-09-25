#ifndef WTK_BFIO_MASKFORM_WTK_GAINNET_BF2
#define WTK_BFIO_MASKFORM_WTK_GAINNET_BF2
#include "wtk_gainnet_bf2_cfg.h"
#include "wtk/bfio/maskdenoise/wtk_drft.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_strbuf.h"
#include "wtk/bfio/maskdenoise/pitch.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_gainnet_bf2 wtk_gainnet_bf2_t;
typedef void (*wtk_gainnet_bf2_notify_f)(void *ths,short *data,int len,int is_end);
typedef void (*wtk_gainnet_bf2_notify_ssl_f)(void *ths,wtk_ssl2_extp_t *nbest_extp,int nbest);

typedef struct
{
	wtk_gainnet_bf2_cfg_t *cfg;
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
}wtk_gainnet_bf2_denoise_t;

struct wtk_gainnet_bf2
{
	wtk_gainnet_bf2_cfg_t *cfg;

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
    
    wtk_gainnet_bf2_denoise_t *qdenoise;

    float *out;
   
    wtk_qenvelope_t *qenvl;
    wtk_ssl2_t *ssl2;

    wtk_wpd_t *wpd;

	void *ths;
	wtk_gainnet_bf2_notify_f notify;

    void *ssl_ths;
    wtk_gainnet_bf2_notify_ssl_f notify_ssl;

    unsigned sil:1;
};

wtk_gainnet_bf2_t* wtk_gainnet_bf2_new(wtk_gainnet_bf2_cfg_t *cfg);
void wtk_gainnet_bf2_delete(wtk_gainnet_bf2_t *gainnet_bf2);
void wtk_gainnet_bf2_start(wtk_gainnet_bf2_t *gainnet_bf2);
void wtk_gainnet_bf2_reset(wtk_gainnet_bf2_t *gainnet_bf2);
void wtk_gainnet_bf2_set_notify(wtk_gainnet_bf2_t *gainnet_bf2,void *ths,wtk_gainnet_bf2_notify_f notify);
void wtk_gainnet_bf2_set_ssl_notify(wtk_gainnet_bf2_t *gainnet_bf2,void *ths,wtk_gainnet_bf2_notify_ssl_f notify);

void wtk_gainnet_bf2_feed(wtk_gainnet_bf2_t *gainnet_bf2,short **data,int len,int is_end);

#ifdef __cplusplus
};
#endif
#endif
