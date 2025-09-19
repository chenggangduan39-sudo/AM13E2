#ifndef WTK_BFIO_MASKFORM_WTK_GAINNET_QFORM9
#define WTK_BFIO_MASKFORM_WTK_GAINNET_QFORM9
#include "wtk_gainnet_qform9_cfg.h"
#include "wtk/bfio/preemph/wtk_preemph.h"
#include "wtk/core/wtk_fring.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef void(*wtk_gainnet_qform9_notify_f)(void *ths,short *data,int len,int is_end);
typedef void(*wtk_gainnet_qform9_notify_two_channel_f)(void *ths,short *data[2],int len,int is_end);
typedef void(*wtk_gainnet_qform9_notify_f2)(void *ths,wtk_complex_t *qform_out,int is_end);

typedef struct
{
	wtk_queue_node_t hoard_n;
	wtk_queue_node_t q_n;  //use for third use.
    wtk_stft2_msg_t *smsg;
    float *cohv;
}wtk_gainnet_qform9_envelopemsg_t;

typedef struct
{
	wtk_gainnet_qform9_cfg_t *cfg;
    int nbin;

    wtk_bankfeat_t *bank_mic;

    float *g;
    float *lastg;
    float *gf;

    float *feature_lm;

    wtk_gainnet2_t *gainnet2;

    wtk_qmmse_t *qmmse;
}wtk_gainnet_qform9_edra_t;

typedef struct wtk_gainnet_qform9 wtk_gainnet_qform9_t;
struct wtk_gainnet_qform9
{
    wtk_gainnet_qform9_cfg_t *cfg;

	wtk_strbuf_t **input;
	float notch_mem[10][2];
	float memD[10];
	float memX;
    float memX_class[2];

    wtk_stft2_t *stft2;
    int end_pos;

    int nbin;

    wtk_queue_t stft2_q;
    wtk_complex_t *cov;
    float *wint;
    float *winf;
    wtk_complex_t *inv_cov;
	wtk_dcomplex_t *tmp;

    wtk_complex_t *fftx;
    wtk_covm_t *covm;
    wtk_covm_t *covm_class[2];
    wtk_bf_t *bf;
    wtk_bf_t *bf_class[2];

    wtk_aspec_t *aspec;
    wtk_aspec_t *aspec_class[3];
    wtk_complex_t **fftclass[2];
    wtk_complex_t *covclass[2];
    wtk_complex_t *invcovclass[2];

    wtk_qenvelope_t *qenvelope;
    wtk_qenvelope_t *qenvel[2];
	wtk_hoard_t qenvel_msg_hoard;
	wtk_hoard_t stft_msg_hoard;

    wtk_strbuf_t *chn1_buf;
    wtk_strbuf_t *chn2_buf;
    wtk_strbuf_t *out_buf;
    
    float nframe;

    int theta;
    int phi;

    wtk_qmmse_t *qmmse;
    wtk_qmmse_t *qmmse_class[2];

    wtk_gainnet_qform9_edra_t *vdr;

    wtk_fring_t *q_fring;
    wtk_fring_t *q_fring_class[3];
    float q_spec;
    float q_spec_class[3];
    int right_nf;
    int right_nf_class[3];

    float *cohv;
    float *cohv_class[3];

    FILE *cohv_fn;
    FILE *cohv_fn_class[2];

	void *ths;
	wtk_gainnet_qform9_notify_f notify;
    
    void *ths_two_channel;
	wtk_gainnet_qform9_notify_two_channel_f notify_two_channel;

    void *ths2;
    wtk_gainnet_qform9_notify_f2 notify2;
};

wtk_gainnet_qform9_t* wtk_gainnet_qform9_new(wtk_gainnet_qform9_cfg_t *cfg);

void wtk_gainnet_qform9_delete(wtk_gainnet_qform9_t *gainnet_qform9);

void wtk_gainnet_qform9_reset(wtk_gainnet_qform9_t *gainnet_qform9);

void wtk_gainnet_qform9_set_notify(wtk_gainnet_qform9_t *gainnet_qform9,void *ths,wtk_gainnet_qform9_notify_f notify);

void wtk_gainnet_qform9_set_notify_two_channel(wtk_gainnet_qform9_t *gainnet_qform9,void *ths,wtk_gainnet_qform9_notify_two_channel_f notify);

void wtk_gainnet_qform9_start(wtk_gainnet_qform9_t *gainnet_qform9, float theta, float phi);

void wtk_gainnet_qform9_feed(wtk_gainnet_qform9_t *gainnet_qform9,short **data,int len,int is_end);


wtk_gainnet_qform9_t* wtk_gainnet_qform9_new2(wtk_gainnet_qform9_cfg_t *cfg, wtk_stft2_t *stft2);

void wtk_gainnet_qform9_delete2(wtk_gainnet_qform9_t *gainnet_qform9);

void wtk_gainnet_qform9_reset2(wtk_gainnet_qform9_t *gainnet_qform9);

void wtk_gainnet_qform9_start2(wtk_gainnet_qform9_t *gainnet_qform9, float theta, float phi);

void wtk_gainnet_qform9_feed_smsg(wtk_gainnet_qform9_t *gainnet_qform9,wtk_stft2_msg_t *smsg,int pos,int is_end);

void wtk_gainnet_qform9_set_notify2(wtk_gainnet_qform9_t *gainnet_qform9,void *ths,wtk_gainnet_qform9_notify_f2 notify);

#ifdef __cplusplus
};
#endif
#endif
