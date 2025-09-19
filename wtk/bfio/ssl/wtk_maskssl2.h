#ifndef WTK_BFIO_WTK_MASKSSL2
#define WTK_BFIO_WTK_MASKSSL2
#include "wtk_maskssl2_cfg.h"
#include "wtk/core/wtk_hoard.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct 
{
    int theta;
    int phi;
    float nspecsum;
    unsigned is_peak:1;
}wtk_maskssl2_extp_t;

typedef struct
{
	wtk_queue_node_t hoard_n;
	wtk_queue_node_t q_n;
	wtk_complex_t **fft;
    float *mask;
    float *nspecsum;
    float **spec;
}wtk_maskssl2_msg_t;

typedef void(*wtk_maskssl2_notify_f)(void *ths, wtk_maskssl2_extp_t *nbest_extp,int nbest, int ts,int te);

typedef struct wtk_maskssl2 wtk_maskssl2_t;
struct wtk_maskssl2
{
    wtk_maskssl2_cfg_t *cfg;
    int channel;
    int nbin;

    wtk_hoard_t msg_hoard;
    wtk_queue_t msg_q;

    float *kmask_s;
    float *kmask_n;

    wtk_complex_t **scov;
    wtk_complex_t **ncov;
    wtk_complex_t *inv_cov;
	wtk_dcomplex_t *tmp;

    wtk_maskaspec_t *maskaspec;

    wtk_maskssl2_extp_t *nextp;
    int *max_idx;
    
    wtk_maskssl2_extp_t *nbest_extp;
    int nbest;

    int ntheta;
    int nphi;
    int nangle;

    float nframe;

    wtk_maskssl2_notify_f notify;
    void *ths;

	float theta;
	float theta2;
    int change_count;
    int delay_cnt;

    int sil_delay;

    float *spec;
};

wtk_maskssl2_t *wtk_maskssl2_new(wtk_maskssl2_cfg_t *cfg);
void wtk_maskssl2_start(wtk_maskssl2_t *maskssl2);
void wtk_maskssl2_delete(wtk_maskssl2_t *maskssl2);
void wtk_maskssl2_reset(wtk_maskssl2_t *maskssl2);

//fft:nbin*channel;
void wtk_maskssl2_feed_fft(wtk_maskssl2_t *maskssl2,wtk_complex_t **fft,float *mask,int is_sil);
//fft:channel*nbin;
void wtk_maskssl2_feed_fft2(wtk_maskssl2_t *maskssl2,wtk_complex_t **fft,float *mask,int is_sil);


void wtk_maskssl2_print(wtk_maskssl2_t *maskssl2);
void wtk_maskssl2_set_notify(wtk_maskssl2_t *maskssl2, void *ths, wtk_maskssl2_notify_f notify);

#ifdef __cplusplus
};
#endif
#endif