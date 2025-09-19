#ifndef WTK_BFIO_QFORM_WTK_QFORM9
#define WTK_BFIO_QFORM_WTK_QFORM9
#include "wtk_qform9_cfg.h"
#include "wtk/bfio/preemph/wtk_preemph.h"
#include "wtk/core/wtk_fring.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef void(*wtk_qform9_notify_f)(void *ths,short *data,int len,int is_end);
typedef void(*wtk_qform9_notify_two_channel_f)(void *ths,short *data[2],int len,int is_end);
typedef void(*wtk_qform9_notify_f2)(void *ths,wtk_complex_t *qform_out,int is_end);
typedef void(*wtk_qform9_notify_f3)(void *ths,wtk_complex_t *qform_out,int is_end);

typedef struct
{
    /* data */
    int *correct_count;
    int *freq_howl;
    int *no_howl;
}wtk_qform9_howl_t;


typedef struct
{
	wtk_queue_node_t hoard_n;
	wtk_queue_node_t q_n;  //use for third use.
    wtk_stft2_msg_t *smsg;
    float *cohv;
}wtk_qform9_envelopemsg_t;

typedef struct wtk_qform9 wtk_qform9_t;
struct wtk_qform9
{
    wtk_qform9_cfg_t *cfg;

	wtk_strbuf_t **input;
	float notch_mem[10][2];
	float memD[10];
	float memX;
    float memX_class[2];

    wtk_stft2_t *stft2;
    int end_pos;
    int delay_end_pos;
    int delay_end_pos_class[2];

    int nbin;


    wtk_queue_t stft2_q;
    wtk_complex_t *cov;
    float *wint;
    float *winf;
    wtk_complex_t *inv_cov;
	wtk_dcomplex_t *tmp;

    wtk_covm_t *covm;
    wtk_covm_t *covm_class[2];
    wtk_bf_t *bf;
    wtk_bf_t *bf_class[2];

    wtk_aspec_t *aspec;
    wtk_aspec_t **naspec;
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
    int noise_debug_cnt;
    int noise_debug_cnt_class[2];

    int theta;
    int phi;

    wtk_qmmse_t *qmmse;
    wtk_qmmse_t *qmmse_class[2];

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

    wtk_queue_t delay_q;
    wtk_queue_t delay_q2;
    wtk_queue_t delay_q_class[2];
    wtk_queue_t delay_q2_class[2];
    int delay_nf;
    int delay_nf_class[2];
    int delay_cnt;
    int delay_cnt_class[2];
    float **delay_cohv;

	void *ths;
	wtk_qform9_notify_f notify;
    
    void *ths_two_channel;
	wtk_qform9_notify_two_channel_f notify_two_channel;

    void *ths2;
    wtk_qform9_notify_f2 notify2;

    void *ths3;
    wtk_qform9_notify_f3 notify3;

    wtk_qform9_howl_t *howl;

    int entropy_silcnt;
    int entropy_in_cnt;
    int cohv_frame;
    int cohv_frame_class[2];
    float ncohv_sum;
    float ncohv_sum_class[2];
    float scohv_sum;
    float scohv_sum_class[2];
    int ncohv_cnt;
    int ncohv_cnt_class[2];
    float *entropy_E;
    float *entropy_Eb;
    FILE *ncohv_fn;
    FILE *scohv_fn;
    FILE *entropy_fn;
    unsigned entropy_sil:1;

    float bs_scale;
    float bs_last_scale;
    float bs_real_scale;
    int bs_max_cnt;
    float *bs_win;

    int sil_in_cnt;
    int sil_out_cnt;
    int sil_max_range;
    float sil_noise_suppress;
};

wtk_qform9_t* wtk_qform9_new(wtk_qform9_cfg_t *cfg);

void wtk_qform9_delete(wtk_qform9_t *qform9);

void wtk_qform9_reset(wtk_qform9_t *qform9);

void wtk_qform9_set_notify(wtk_qform9_t *qform9,void *ths,wtk_qform9_notify_f notify);

void wtk_qform9_set_notify_two_channel(wtk_qform9_t *qform9,void *ths,wtk_qform9_notify_two_channel_f notify);

void wtk_qform9_start(wtk_qform9_t *qform9, float theta, float phi);

void wtk_qform9_feed(wtk_qform9_t *qform9,short **data,int len,int is_end);


wtk_qform9_t* wtk_qform9_new2(wtk_qform9_cfg_t *cfg, wtk_stft2_t *stft2);

void wtk_qform9_delete2(wtk_qform9_t *qform9);

void wtk_qform9_reset2(wtk_qform9_t *qform9);

void wtk_qform9_start2(wtk_qform9_t *qform9, float theta, float phi);

void wtk_qform9_feed_smsg(wtk_qform9_t *qform9,wtk_stft2_msg_t *smsg,int pos,int is_end);

void wtk_qform9_set_notify2(wtk_qform9_t *qform9,void *ths,wtk_qform9_notify_f2 notify);


wtk_qform9_t* wtk_qform9_new3(wtk_qform9_cfg_t *cfg, int wins, int channel);

void wtk_qform9_delete3(wtk_qform9_t *qform9);

void wtk_qform9_reset3(wtk_qform9_t *qform9);

void wtk_qform9_start3(wtk_qform9_t *qform9, float theta, float phi);

void wtk_qform9_feed_fft(wtk_qform9_t *qform9,wtk_complex_t **fft,int is_end);

void wtk_qform9_set_notify3(wtk_qform9_t *qform9,void *ths,wtk_qform9_notify_f3 notify);

#ifdef __cplusplus
};
#endif
#endif
