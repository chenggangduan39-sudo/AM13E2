#ifndef WTK_BFIO_WTK_BFIO4
#define WTK_BFIO_WTK_BFIO4
#include "wtk_bfio4_cfg.h"
// #include "wtk/asr/wfst/kaldifst/qtk_decoder_wrapper.h"
#include "wtk/core/wtk_strbuf.h"
#include "wtk/os/wtk_thread.h"
#include "wtk/os/wtk_lockhoard.h"
#include "wtk/asr/img/qtk_img.h"
#include "wtk/asr/img/qtk_img2.h"
#include "wtk/core/wtk_wavfile.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    WTK_BFIO4_WAKE,
    WTK_BFIO4_ASR,
}wtk_bfio4_cmd_t;

typedef enum{
    WTK_BFIO4_MSG_WAKE,
    WTK_BFIO4_MSG_ASR,
    WTK_BFIO4_MSG_WAKE_DENOISE,
    WTK_BFIO4_MSG_ASR_DENOISE,
    WTK_BFIO4_MSG_MIC,
    WTK_BFIO4_MSG_END,
}wtk_bfio4_msg_type_t;

typedef struct
{
	wtk_queue_node_t hoard_n;
	wtk_queue_node_t q_n;
    wtk_bfio4_msg_type_t type;
    short *pv;
    int len;
}wtk_bfio4_msg_t;

typedef enum{
    WTK_BFIO4_MODEL_WAKE,
    WTK_BFIO4_MODEL_ASR,
    WTK_BFIO4_MODEL_DENOISE,
}wtk_bfio4_model_t;

typedef void(*wtk_bfio4_notify_f)(void *ths, wtk_bfio4_cmd_t cmd, int res);

typedef struct 
{
    qtk_img_rec_t *img;
    wtk_bfio4_model_t wake_model;
    void *hook;
    float wake_fs;
    float wake_fe;
    float wake_prob;
    unsigned waked:1;
}wtk_bfio4_wake_t;

typedef struct 
{
    qtk_img2_rec_t *img2;
    wtk_bfio4_model_t asr_model;
    void *hook;
    float asr_fs;
    float asr_fe;
    float asr_prob;
    int asr_id;
}wtk_bfio4_asr_t;

typedef struct wtk_bfio4 wtk_bfio4_t;
struct wtk_bfio4
{
    wtk_bfio4_cfg_t *cfg;

    wtk_gainnet_denoise_t *denoise;
    wtk_bfio4_wake_t **wake;
    wtk_bfio4_asr_t **asr;
    wtk_strbuf_t *wake_buf;
    wtk_strbuf_t *asr_buf;

    int wake_cnt;
    int asr_cnt;

    float wake_fs;
    float wake_fe;
    float wake_prob;
    float asr_fs;
    float asr_fe;
    float asr_prob;
    int asr_id;

    void *ths;
    wtk_bfio4_notify_f notify;


	wtk_lockhoard_t msg_lockhoard;
	wtk_thread_t thread[2];
	wtk_sem_t end_sem;
	wtk_sem_t end_sem2;
	wtk_blockqueue_t wake_q;
	wtk_blockqueue_t asr_q;
	unsigned run:1;

    unsigned waked:1;
    unsigned is_end:1;
};


wtk_bfio4_t* wtk_bfio4_new(wtk_bfio4_cfg_t *cfg);

void wtk_bfio4_delete(wtk_bfio4_t *bfio4);

void wtk_bfio4_reset(wtk_bfio4_t *bfio4);

void wtk_bfio4_set_notify(wtk_bfio4_t *bfio4,void *ths,wtk_bfio4_notify_f notify);

void wtk_bfio4_start(wtk_bfio4_t *bfio4);

void wtk_bfio4_feed(wtk_bfio4_t *bfio4,short *data,int len,int is_end);

#ifdef __cplusplus
};
#endif
#endif
