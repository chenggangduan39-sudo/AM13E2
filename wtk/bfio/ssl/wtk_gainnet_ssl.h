#ifndef WTK_BFIO_SSL_WTK_GAINNET_SSL
#define WTK_BFIO_SSL_WTK_GAINNET_SSL
#include "wtk_gainnet_ssl_cfg.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/bfio/maskdenoise/wtk_drft.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_strbuf.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_gainnet_ssl wtk_gainnet_ssl_t;
typedef void (*wtk_gainnet_ssl_notify_ssl_f)(void *ths, float ts, float te,wtk_ssl2_extp_t *nbest_extp,int nbest);

typedef struct
{
	wtk_gainnet_ssl_cfg_t *cfg;
    int nbin;

    wtk_bankfeat_t *bank_mic;
    wtk_bankfeat_t *bank_sp;

    float *g;
    float *lastg;
    float *gf;

    wtk_gainnet2_t *gainnet2;

    wtk_gainnet7_t *gainnet7;

    wtk_qmmse_t *qmmse;
}wtk_gainnet_ssl_edra_t;


struct wtk_gainnet_ssl
{
	wtk_gainnet_ssl_cfg_t *cfg;

	wtk_strbuf_t **mic;
	wtk_strbuf_t **sp;

    float *analysis_window;
	wtk_drft_t *rfft;
	float *rfft_in;
	int nbin;
    float **analysis_mem;
    float **analysis_mem_sp;

	wtk_complex_t **fft;
	wtk_complex_t **fft_sp;

	wtk_rls_t *erls;
    wtk_complex_t **ffts;
    wtk_complex_t *ffty;

    wtk_gainnet_ssl_edra_t *vdr;

    float pframe;

    wtk_complex_t *fftx;

    wtk_maskssl_t *maskssl;
    wtk_maskssl2_t *maskssl2;

    void *ssl_ths;
	wtk_gainnet_ssl_notify_ssl_f notify_ssl;

    int sp_silcnt;
    int mic_silcnt;

    unsigned sp_sil:1;
    unsigned mic_sil:1;

    unsigned echo_enable:1;   
};

wtk_gainnet_ssl_t* wtk_gainnet_ssl_new(wtk_gainnet_ssl_cfg_t *cfg);
void wtk_gainnet_ssl_delete(wtk_gainnet_ssl_t *gainnet_ssl);
void wtk_gainnet_ssl_start(wtk_gainnet_ssl_t *gainnet_ssl);
void wtk_gainnet_ssl_reset(wtk_gainnet_ssl_t *gainnet_ssl);
void wtk_gainnet_ssl_set_ssl_notify(wtk_gainnet_ssl_t *gainnet_ssl,void *ths,wtk_gainnet_ssl_notify_ssl_f notify);
/**
 * len=mic array samples
 */
void wtk_gainnet_ssl_feed(wtk_gainnet_ssl_t *gainnet_ssl,short *data,int len,int is_end);


void wtk_gainnet_ssl_set_echoenable(wtk_gainnet_ssl_t *gainnet_ssl,int enable);

#ifdef __cplusplus
};
#endif
#endif
