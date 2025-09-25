#ifndef WTK_BFIO3_WTK_BFIO3
#define WTK_BFIO3_WTK_BFIo3
#include "wtk_bfio3_cfg.h"
#include "wtk/asr/wfst/kaldifst/qtk_decoder_wrapper.h"
#include "wtk/core/wtk_strbuf.h"
#include "wtk/bfio/preemph/wtk_preemph.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	WTK_BFIO3_VAD_START,
	WTK_BFIO3_VAD_DATA,
	WTK_BFIO3_VAD_END,
	WTK_BFIO3_VAD_CANCEL,
    WTK_BFIO3_WAKE,
    WTK_BFIO3_WAKE_SSL,
    WTK_BFIO3_ASR_RES,
	WTK_BFIO3_WAKE_RES
}wtk_bfio3_cmd_t;

typedef void(*wtk_bfio3_notify_f)(void *ths,wtk_bfio3_cmd_t cmd,short *data,int len);

typedef struct 
{
    int theta;
    wtk_kvadwake_t *vwake;
    void *hook;
    float wake_fs;
    float wake_fe;
    float wake_prob;
    unsigned waked:1;
}wtk_bfio3_wake_t;

typedef struct
{
	wtk_queue_node_t hoard_n;
	wtk_queue_node_t q_n;
    wtk_stft2_msg_t *stft2_msg;
    int pos;
    unsigned is_end:1;
}wtk_bfio3_msg_t;


typedef struct wtk_bfio3 wtk_bfio3_t;
struct wtk_bfio3
{
    wtk_bfio3_cfg_t *cfg;

    wtk_strbuf_t **input;
	float **notch_mem;
	float *memD;
	float *memX;

    wtk_stft2_t *stft2;
    wtk_stft2_t *sp_stft2;
    wtk_queue_t mspstft_q;

    int end_pos;
    wtk_queue_t stft2_q;

	wtk_sem_t aec_sem;  ////
	wtk_sem_t kwake_sem;
	wtk_lockhoard_t msg_lockhoard;
    wtk_blockqueue_t aec_queue;
    wtk_blockqueue_t wbf_queue;
    wtk_blockqueue_t kwake_queue;
    wtk_blockqueue_t qform_queue;
    wtk_thread_t aec_t;
    wtk_thread_t wbf_t;
    wtk_thread_t kwake_t;
    wtk_thread_t qform_t;

    wtk_aec_t *aec;
    wtk_wbf_t *wbf;
    wtk_wbf2_t *wbf2;
    wtk_bfio3_wake_t **wake;
    qtk_decoder_wrapper_t *decoder;

    wtk_ssl_t *ssl;
    wtk_qform9_t *qform;
    wtk_qform2_t *qform2;
    wtk_vad2_t *vad;

    int wake_cnt;
    int channel;

    int wake_theta;
    float wake_fs;
    float wake_fe;
    float wake_prob;

    float vad_output;
    float vad_ts;
    float vad_te;

    void *ths;
    wtk_bfio3_notify_f notify;

    int state;//for trick
    float idle_frame;
    int idle_trigger_frame;

    unsigned waked:1;
    unsigned asr : 1;
    unsigned is_end:1;
    unsigned wbf_run:1;  //// 
    unsigned kwake_run:1;
    unsigned qform_run:1;
};


wtk_bfio3_t* wtk_bfio3_new(wtk_bfio3_cfg_t *cfg);

void wtk_bfio3_delete(wtk_bfio3_t *bfio3);

void wtk_bfio3_reset(wtk_bfio3_t *bfio3);

void wtk_bfio3_set_notify(wtk_bfio3_t *bfio3,void *ths,wtk_bfio3_notify_f notify);

void wtk_bfio3_start(wtk_bfio3_t *bfio3);

void wtk_bfio3_feed(wtk_bfio3_t *bfio3,short **data,int len,int is_end);

#ifdef __cplusplus
};
#endif
#endif
