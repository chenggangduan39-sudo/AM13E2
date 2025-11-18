#ifndef WTK_BFIO_MASKFORM_WTK_GAINNET_BF4
#define WTK_BFIO_MASKFORM_WTK_GAINNET_BF4
#include "wtk/core/wtk_strbuf.h"
#include "wtk_gainnet_bf4_cfg.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/bfio/maskdenoise/wtk_drft.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_strbuf.h"
#include "wtk/bfio/preemph/wtk_preemph.h"
#include "wtk/core/wtk_fring.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_gainnet_bf4 wtk_gainnet_bf4_t;
typedef void(*wtk_gainnet_bf4_notify_f)(void *ths,short *output,int len);
typedef void (*wtk_gainnet_bf4_notify_ssl_f)(void *ths, float ts, float te,wtk_ssl2_extp_t *nbest_extp,int nbest);

typedef struct
{
	wtk_gainnet_bf4_cfg_t *cfg;
    int nbin;
    int *eband;

    float  *Ex;
    float *dct_table;
    float **cepstral_mem;
    int memid;
    float **cepstral_mem_sp;
    int memid_sp;

    float *g;
    float *lastg;
    float *gf;
    float *g2;
    float *gf2;

    float gf2_m;

    float *Ly;
    float *features;
    wtk_gainnet5_t *gainnet;

    wtk_qmmse_t *qmmse;

    unsigned silence:1;
}wtk_gainnet_bf4_aec_t;

typedef struct
{
    wtk_queue_node_t hoard_n;
	wtk_queue_node_t q_n; 
	wtk_complex_t **fft;
    float *cohv;
}wtk_gainnet_bf4_fft_msg_t;

struct wtk_gainnet_bf4
{
	wtk_gainnet_bf4_cfg_t *cfg;

	wtk_strbuf_t **mic;
	wtk_strbuf_t **sp;
	float **notch_mem;
	float *memD;
	float memX;

    float *window;
    float *synthesis_window;
	wtk_drft_t *rfft;
	float *rfft_in;
	int nbin;
    float **analysis_mem;
    float **analysis_mem_sp;
	float *synthesis_mem;

	wtk_complex_t **fft;
	wtk_complex_t **fft_sp;
	wtk_rls_t *erls;
    wtk_complex_t **ffty;
    wtk_complex_t **ffts;
    wtk_complex_t **ffty2;

    wtk_gainnet_bf4_aec_t *gaec;

    float pframe;

    wtk_hoard_t msg_hoard;
    wtk_queue_t fft_q;

    wtk_aspec_t *aspec;
    wtk_complex_t *cov;
    float *wint;
    float *winf;
    wtk_complex_t *inv_cov;
	wtk_dcomplex_t *tmp;

	wtk_covm_t *covm;
	wtk_bf_t *bf;

    FILE *cohv_fn;
    wtk_qenvelope_t *qenvelope;
    wtk_maskssl2_t *maskssl2;
    wtk_fring_t *q_fring;
    float q_spec;
    int right_nf;
    int nframe;

    wtk_qmmse_t *qmmse;

    wtk_complex_t *fftx;
	float *out;

	void *ths;
	wtk_gainnet_bf4_notify_f notify;

    void *ssl_ths;
	wtk_gainnet_bf4_notify_ssl_f notify_ssl;

    int sp_silcnt;
    unsigned sp_sil:1;

    int mic_silcnt;
    unsigned mic_sil:1;
    unsigned ssl_enable:1;
};

wtk_gainnet_bf4_t* wtk_gainnet_bf4_new(wtk_gainnet_bf4_cfg_t *cfg);
void wtk_gainnet_bf4_delete(wtk_gainnet_bf4_t *gainnet_bf4);
void wtk_gainnet_bf4_start(wtk_gainnet_bf4_t *gainnet_bf4);
void wtk_gainnet_bf4_reset(wtk_gainnet_bf4_t *gainnet_bf4);
void wtk_gainnet_bf4_set_notify(wtk_gainnet_bf4_t *gainnet_bf4,void *ths,wtk_gainnet_bf4_notify_f notify);
void wtk_gainnet_bf4_set_ssl_notify(wtk_gainnet_bf4_t *gainnet_bf4,void *ths,wtk_gainnet_bf4_notify_ssl_f notify);
/**
 * len=mic array samples
 */
void wtk_gainnet_bf4_feed(wtk_gainnet_bf4_t *gainnet_bf4,short *data,int len,int is_end);

void wtk_gainnet_bf4_ssl_delay_new(wtk_gainnet_bf4_t *gainnet_bf4);
#ifdef __cplusplus
};
#endif
#endif
