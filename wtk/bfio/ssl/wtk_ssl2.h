#ifndef WTK_BFIO_SSL_WTK_SSL2
#define WTK_BFIO_SSL_WTK_SSL2
#include "wtk_ssl2_cfg.h"
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
}wtk_ssl2_extp_t;

typedef void(*wtk_ssl2_notify_f)(void *ths, wtk_ssl2_extp_t *nbest_extp,int nbest, int ts,int te);

typedef struct wtk_ssl2 wtk_ssl2_t;
struct wtk_ssl2
{
    wtk_ssl2_cfg_t *cfg;
    wtk_hoard_t msg_hoard;

    int channel;
    int nbin;
    wtk_queue_t msg_q;
    wtk_complex_t *cov;
    float *wint;
    float *winf;
    wtk_complex_t *inv_cov;
	wtk_dcomplex_t *tmp;
    
    wtk_aspec_t *aspec;

    wtk_ssl2_extp_t *nextp;
    int *max_idx;

    wtk_ssl2_extp_t *nbest_extp;
    int nbest;

    int ntheta;
    int nphi;
    int nangle;
    float *spec;

    float nframe;
    int oframe;

	void *ths;
	wtk_ssl2_notify_f notify;
};


wtk_ssl2_t* wtk_ssl2_new(wtk_ssl2_cfg_t *cfg);

void wtk_ssl2_delete(wtk_ssl2_t *ssl2);

void wtk_ssl2_reset(wtk_ssl2_t *ssl2);

void wtk_ssl2_feed_fft(wtk_ssl2_t *ssl2,wtk_complex_t **fft,int is_end);
void wtk_ssl2_feed_fft2(wtk_ssl2_t *ssl2,wtk_complex_t **fft,int is_end);

void wtk_ssl2_print(wtk_ssl2_t *ssl2);

void wtk_ssl2_set_notify(wtk_ssl2_t *ssl2, void *ths, wtk_ssl2_notify_f notify);

#ifdef __cplusplus
};
#endif
#endif