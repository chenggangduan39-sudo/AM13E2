#ifndef WTK_BFIO_WTK_MASKSSL
#define WTK_BFIO_WTK_MASKSSL
#include "wtk_maskssl_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct 
{
    int theta;
    int phi;
    float nspecsum;
    unsigned is_peak:1;
}wtk_maskssl_extp_t;

typedef void(*wtk_maskssl_notify_f)(void *ths, wtk_maskssl_extp_t *nbest_extp,int nbest, int ts,int te);

typedef struct wtk_maskssl wtk_maskssl_t;
struct wtk_maskssl
{
    wtk_maskssl_cfg_t *cfg;
    int channel;
    int nbin;

    float *kmask_s;
    float *kmask_n;

    // float **refmask
    wtk_complex_t **scov;
    wtk_complex_t **ncov;
    wtk_complex_t *inv_cov;
	wtk_dcomplex_t *tmp;

    wtk_maskaspec_t *maskaspec;

    wtk_maskssl_extp_t *nextp;
    int *max_idx;
    
    wtk_maskssl_extp_t *nbest_extp;
    int nbest;

    int ntheta;
    int nphi;
    int nangle;

    float nframe;
    int oframe;

    wtk_maskssl_notify_f notify;
    void *ths;
};

wtk_maskssl_t *wtk_maskssl_new(wtk_maskssl_cfg_t *cfg);
void wtk_maskssl_start(wtk_maskssl_t *maskssl);
void wtk_maskssl_delete(wtk_maskssl_t *maskssl);
void wtk_maskssl_reset(wtk_maskssl_t *maskssl);

//fft:nbin*channel;
void wtk_maskssl_feed_fft(wtk_maskssl_t *maskssl,wtk_complex_t **fft,float *mask,int is_sil);

//fft:channel*nbin;
void wtk_maskssl_feed_fft2(wtk_maskssl_t *maskssl,wtk_complex_t **fft,float *mask,int is_sil);

void wtk_maskssl_print(wtk_maskssl_t *maskssl);
void wtk_maskssl_set_notify(wtk_maskssl_t *maskssl, void *ths, wtk_maskssl_notify_f notify);

#ifdef __cplusplus
};
#endif
#endif