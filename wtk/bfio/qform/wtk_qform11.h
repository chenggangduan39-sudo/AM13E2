#ifndef WTK_BFIO_QFORM_WTK_QFORM11
#define WTK_BFIO_QFORM_WTK_QFORM11
#include "wtk_qform11_cfg.h"
#include "wtk/bfio/preemph/wtk_preemph.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef void(*wtk_qform11_notify_f)(void *ths,short *data,int len,int is_end);

typedef struct
{
	wtk_queue_node_t hoard_n;
	wtk_queue_node_t q_n;  //use for third use.
    wtk_stft2_msg_t *smsg;
    float *cohv;
}wtk_qform11_envelopemsg_t;

typedef struct wtk_qform11 wtk_qform11_t;
struct wtk_qform11
{
    wtk_qform11_cfg_t *cfg;

	wtk_strbuf_t **input;
	float **notch_mem;
	float *memD;
	float *memX;

    wtk_stft2_t *stft2;
    int end_pos;

    int nbin;

    wtk_queue_t stft2_q;
    wtk_complex_t *cov;
    float *wint;
    float *winf;
    wtk_complex_t *inv_cov;
	wtk_dcomplex_t *tmp;

    wtk_covm_t **covm;
    wtk_bf_t **bf;

    wtk_aspec_t *aspec;
    int *aspec_theta_idx;
    float *spec_k;
    float **specsum;
    int *freqsum;
    int *sum_count;
    short *output;

    wtk_qenvelope_t **qenvelope;
	wtk_hoard_t qenvel_msg_hoard;
    
    wtk_strbuf_t **out_buf;
    
    float nframe;

    int theta;
    int phi;

    wtk_qmmse_t **qmmse;

    float **cohv;

    FILE **cohv_fn;

	void *ths;
	wtk_qform11_notify_f notify;
    
};

wtk_qform11_t* wtk_qform11_new(wtk_qform11_cfg_t *cfg);

void wtk_qform11_delete(wtk_qform11_t *qform11);

void wtk_qform11_reset(wtk_qform11_t *qform11);

void wtk_qform11_set_notify(wtk_qform11_t *qform11,void *ths,wtk_qform11_notify_f notify);

void wtk_qform11_start(wtk_qform11_t *qform11, float theta, float phi);

void wtk_qform11_feed(wtk_qform11_t *qform11,short **data,int len,int is_end);

void wtk_qform11_set_theta(wtk_qform11_t *qform11, float theta, float phi, float range, int idx);
#ifdef __cplusplus
};
#endif
#endif
