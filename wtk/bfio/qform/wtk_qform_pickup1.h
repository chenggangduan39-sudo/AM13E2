#ifndef WTK_BFIO_QFORM_WTK_QFORM_PICKUP1
#define WTK_BFIO_QFORM_WTK_QFORM_PICKUP1
#include "wtk_qform_pickup1_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef void(*wtk_qform_pickup1_notify_f)(void *ths,short *data,int len, char *cohv, int len2);

typedef struct
{
    wtk_queue_node_t q_n;
    wtk_stft2_msg_t *smsg;
    char *cohv;
}wtk_qform_pickup1_stft2msg_t;

typedef struct
{
    wtk_queue_node_t q_n;
    wtk_complex_t *scov;
} wtk_qform_pickup1_scovmsg_t;

typedef struct
{
    wtk_queue_node_t q_n;
    wtk_complex_t *ncov;
} wtk_qform_pickup1_ncovmsg_t;

typedef struct wtk_qform_pickup1 wtk_qform_pickup1_t;
struct wtk_qform_pickup1
{
    wtk_qform_pickup1_cfg_t *cfg;

	wtk_strbuf_t **input;
	float notch_mem[10][2];
	float memD[10];
	float memX;

    wtk_stft2_t *stft2;
    int end_pos;

    wtk_queue_t stft2_q;
    wtk_complex_t *cov;
    float *wint;
    float *winf;
    wtk_complex_t *inv_cov;
	wtk_dcomplex_t *tmp;

    wtk_bf_t *bf;
    wtk_aspec_t *aspec;
    wtk_aspec_t **naspec;
    wtk_aspec_t *aspec_class[2];
    wtk_complex_t **fftclass[2];
    wtk_complex_t *covclass[2];
    wtk_complex_t *invcovclass[2];

    float nframe;

    int theta;
    int phi;
    
    wtk_complex_t **ncov;
    float *ncnt_sum;
	wtk_complex_t **scov;
    float *scnt_sum;

    wtk_queue_t qsmsg_q;

    wtk_queue_t *qscov_q;
    wtk_queue_t *qncov_q;

    char *cohv;

	void *ths;
	wtk_qform_pickup1_notify_f notify;
};

wtk_qform_pickup1_t* wtk_qform_pickup1_new(wtk_qform_pickup1_cfg_t *cfg);

void wtk_qform_pickup1_delete(wtk_qform_pickup1_t *qform_pickup1);

void wtk_qform_pickup1_reset(wtk_qform_pickup1_t *qform_pickup1);

void wtk_qform_pickup1_set_notify(wtk_qform_pickup1_t *qform_pickup1,void *ths,wtk_qform_pickup1_notify_f notify);

void wtk_qform_pickup1_start(wtk_qform_pickup1_t *qform_pickup1, float theta, float phi);

void wtk_qform_pickup1_feed(wtk_qform_pickup1_t *qform_pickup1,short **data,int len,int is_end);

void wtk_qform_pickup1_set_grav(wtk_qform_pickup1_t *qform_pickup1,short x,short y,short z);


#ifdef __cplusplus
};
#endif
#endif