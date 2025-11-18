#ifndef WTK_BFIO_SSL_WTK_SSL
#define WTK_BFIO_SSL_WTK_SSL
#include "wtk_ssl_cfg.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/core/fft/wtk_stft.h"
#ifdef __cplusplus
extern "C" {
#endif


typedef struct 
{
    int theta;
    int phi;
    float nspecsum;
    unsigned is_peak:1;
}wtk_ssl_extp_t;

typedef void(*wtk_ssl_notify_f)(void *ths, wtk_ssl_extp_t *nbest_extp);

typedef struct wtk_ssl wtk_ssl_t;
struct wtk_ssl
{
    wtk_ssl_cfg_t *cfg;

    wtk_stft2_t *stft2;
	wtk_hoard_t msg_hoard;
    int channel;
    int nbin;
    wtk_queue_t stft2_q;
    wtk_complex_t *cov;
    float *wint;
    float *winf;
    wtk_complex_t *inv_cov;
	wtk_dcomplex_t *tmp;
    
    wtk_aspec_t *aspec;

    wtk_ssl_extp_t *nextp;
    int *max_idx;

    wtk_ssl_extp_t *nbest_extp;
    int nbest;

    int ntheta;
    int nphi;
    int nangle;

    float nframe;

	int count_len;
	void *ths;
	wtk_ssl_notify_f notify;
};

wtk_ssl_t* wtk_ssl_new(wtk_ssl_cfg_t *cfg);

void wtk_ssl_delete(wtk_ssl_t *ssl);

void wtk_ssl_reset(wtk_ssl_t *ssl);

void wtk_ssl_set_notify(wtk_ssl_t *ssl,void *ths,wtk_ssl_notify_f notify);

void wtk_ssl_feed(wtk_ssl_t *ssl,short **data,int len,int is_end);

void wtk_ssl_feed_count(wtk_ssl_t *ssl,short **data,int len,int is_end);

wtk_ssl_extp_t*  wtk_ssl_has_theta(wtk_ssl_t *ssl,float theta,float df);

wtk_ssl_t* wtk_ssl_new2(wtk_ssl_cfg_t *cfg,wtk_stft2_t *stft);

void wtk_ssl_delete2(wtk_ssl_t *ssl);

void wtk_ssl_reset2(wtk_ssl_t *ssl);

void wtk_ssl_feed_stft2msg(wtk_ssl_t *ssl,wtk_stft2_msg_t *msg,int is_end);

void wtk_ssl_print(wtk_ssl_t *ssl);


#ifdef __cplusplus
};
#endif
#endif