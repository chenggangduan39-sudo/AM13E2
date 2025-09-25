#ifndef WTK_BFIO_WTK_WBF
#define WTK_BFIO_WTK_WBF
#include "wtk/bfio/wtk_wbf_cfg.h" 
#ifdef __cplusplus
extern "C" {
#endif
typedef void(*wtk_wbf_notify_f)(void *ths,short *data,int len,int idx,int is_end);
typedef void(*wtk_wbf_notify_f2)(void *ths,wtk_complex_t *wbf_out,int idx,int is_end);

typedef struct wtk_wbf wtk_wbf_t;
struct wtk_wbf
{
    wtk_wbf_cfg_t *cfg;

    wtk_stft2_t *stft2;
    wtk_aspec_t *aspec;
    wtk_bf_t **bf;

    int channel;
    int nbin;
    int theta_step;
    int end_pos;
    int theta_range;

    int *theta;

    wtk_queue_t stft2_q;
	wtk_hoard_t msg_hoard;
    wtk_complex_t *cov;
    float *wint;
    float *winf;
    wtk_complex_t *inv_cov;
	wtk_dcomplex_t *tmp;

    wtk_complex_t ***ncov;
    float **ncnt_sum;
	wtk_complex_t ***scov;
    float **scnt_sum;

    float *spec_k;
    float **cohv;

    wtk_qmmse_t **qmmse;

    void *ths;
	wtk_wbf_notify_f notify;
	wtk_wbf_notify_f2 notify2;
};

wtk_wbf_t* wtk_wbf_new(wtk_wbf_cfg_t *cfg);

void wtk_wbf_delete(wtk_wbf_t *wbf);

void wtk_wbf_reset(wtk_wbf_t *wbf);

void wtk_wbf_set_notify(wtk_wbf_t *wbf,void *ths,wtk_wbf_notify_f notify);

void wtk_wbf_start(wtk_wbf_t *wbf);

void wtk_wbf_feed(wtk_wbf_t *wbf,short **data,int len,int is_end);


wtk_wbf_t* wtk_wbf_new2(wtk_wbf_cfg_t *cfg, wtk_stft2_t *stft2);

void wtk_wbf_delete2(wtk_wbf_t *wbf);

void wtk_wbf_reset2(wtk_wbf_t *wbf);

void wtk_wbf_set_notify2(wtk_wbf_t *wbf,void *ths,wtk_wbf_notify_f2 notify);

void wtk_wbf_start2(wtk_wbf_t *wbf);

void wtk_wbf_feed_smsg(wtk_wbf_t *wbf,wtk_stft2_msg_t *msg,int pos,int is_end);


#ifdef __cplusplus
};
#endif
#endif