#ifndef WTK_BFIO_WTK_BFIO5
#define WTK_BFIO_WTK_BFIO5
#include "wtk_bfio5_cfg.h"
#include "wtk/asr/wfst/kaldifst/qtk_decoder_wrapper.h"
#include "wtk/core/wtk_strbuf.h"
#include "wtk/bfio/preemph/wtk_preemph.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	WTK_BFIO5_VAD_START,
	WTK_BFIO5_VAD_DATA,
	WTK_BFIO5_VAD_END,
	WTK_BFIO5_VAD_CANCEL,
	WTK_BFIO5_ASR_CANCEL,
    WTK_BFIO5_WAKE,
    WTK_BFIO5_WAKE_SSL,
    WTK_BFIO5_ASR_RES,
	WTK_BFIO5_WAKE_RES,
    WTK_BFIO5_SIL_END, // oneshot 安静结束
    WTK_BFIO5_SPEECH_END, // oneshot 到时间强制结束
    WTK_BFIO5_WAKE2,
}wtk_bfio5_cmd_t;

typedef void(*wtk_bfio5_notify_f)(void *ths,wtk_bfio5_cmd_t cmd,short *data,int len);
typedef void (*wtk_bfio5_notify_ssl2_f)(void *ths,wtk_ssl2_extp_t *nbest_extp,int nbest);

typedef struct
{
	wtk_queue_node_t hoard_n;
	wtk_queue_node_t q_n;  //use for third use.
	wtk_complex_t **fft; // |nbin*channel|
	float s;
    int theta;
    int theta2;
    float *mask;
	void *hook;
}wtk_bfio5_edr_msg_t;

typedef struct 
{
	wtk_bfio5_cfg_t *cfg;
    int nbin;

    wtk_bankfeat_t *bank_mic;
    wtk_bankfeat_t *bank_sp;

    float *feature_sp;

    float *g;
    float *gf;

    wtk_gainnet2_t *gainnet2;

    wtk_qmmse_t *qmmse;
}wtk_bfio5_edr_t;

typedef struct
{
    int theta;
    wtk_kvadwake_t *vwake;
    void *hook;
    float wake_fs;
    float wake_fe;
    float wake_prob;
    unsigned waked:1;
}wtk_bfio5_wake_t;

typedef struct wtk_bfio5 wtk_bfio5_t;
struct wtk_bfio5
{
    wtk_bfio5_cfg_t *cfg;

    wtk_strbuf_t **input;
	float **notch_mem;
	float *memD;
	float *memX;

    wtk_stft2_t *stft2;
    wtk_stft2_t *stft2_2;
    wtk_stft2_t *sp_stft2;
    wtk_queue_t mstft_2_q;
    wtk_queue_t mspstft_q;

    int end_pos;
    wtk_queue_t stft2_q;

    wtk_aec_t *aec;
    wtk_complex_t **saec_out;
    float **saec_pad;
    wtk_complex_t **mic2_out;
    float **mic2_pad;
    wtk_wbf_t *wbf;
    wtk_wbf2_t *wbf2;
    wtk_gainnet_denoise_t *gdenoise;
    wtk_bfio5_wake_t **wake;
    wtk_bfio5_wake_t **wake2;
    wtk_bfio5_wake_t *wake3;
    qtk_decoder_wrapper_t *decoder;

    wtk_ssl_t *ssl;
    wtk_ssl2_t *ssl2;
    wtk_qform9_t *qform;
    wtk_qform2_t *qform2;
    wtk_qform3_t *qform3;
    wtk_vad2_t *vad;

    wtk_strbuf_t **de_wake_buf;
    wtk_strbuf_t *aec_wake_buf;
    float de_fe;
    float de_fs;
    float aec_fe;
    float aec_fs;

    wtk_bfio5_edr_msg_t *edr_msg;
    wtk_bfio5_edr_t *edr;
    wtk_maskssl2_t *maskssl2;
    wtk_maskssl2_t *maskssl2_2;
    wtk_hoard_t msg_hoard;
    wtk_queue_t edr_stft_q;
	wtk_rls_t *erls;
    wtk_complex_t *fftx;
    wtk_complex_t *ffty;
    int *mssl_tmp;
    int *mssl_2_tmp;

    int nbin;
    int sp_silcnt;

    int wake_cnt;
    int channel;
    int channel2;
    int sp_channel;

    int wake_theta;
    float wake_fs;
    float wake_fe;
    float wake_extend;
    float wake2_extend;
    float wake_prob;
    float wake2_prob;
    float out_wake_prob;
    int wake_key;
    float sil_ts;
    float speech_ts;

    float vad_output;
    float vad_ts;
    float vad_te;

    void *ths;
    wtk_bfio5_notify_f notify;

    void *ssl2_ths;
    wtk_bfio5_notify_ssl2_f notify_ssl2;

    int reg_theta;
    int reg_tms;

    float sum_low;
    int low_cnt;

    unsigned waked:1;
    unsigned asr : 1;
    unsigned is_end:1;
    unsigned sil_end:1;
    unsigned int reg_bf:1;
    unsigned int reg_end:1;
    unsigned wake2_ready:1;
    unsigned int sp_sil:1;
};


wtk_bfio5_t* wtk_bfio5_new(wtk_bfio5_cfg_t *cfg);

void wtk_bfio5_delete(wtk_bfio5_t *bfio5);

void wtk_bfio5_reset(wtk_bfio5_t *bfio5);

void wtk_bfio5_set_notify(wtk_bfio5_t *bfio5,void *ths,wtk_bfio5_notify_f notify);

void wtk_bfio5_set_ssl2_notify(wtk_bfio5_t *bfio5, void *ths, wtk_bfio5_notify_ssl2_f notify);

void wtk_bfio5_start(wtk_bfio5_t *bfio5);

void wtk_bfio5_feed(wtk_bfio5_t *bfio5,short **data,int len,int is_end);

void wtk_bfio5_set_one_shot(wtk_bfio5_t *bfio5, int on);

void wtk_bfio5_set_offline_qform(wtk_bfio5_t *bfio5, int tms, int theta);

int wtk_bfio5_set_wake_words(wtk_bfio5_t *bfio5,char *words,int len);

#ifdef __cplusplus
};
#endif
#endif
