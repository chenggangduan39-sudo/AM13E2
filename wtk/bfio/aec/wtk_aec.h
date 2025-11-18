#ifndef WTK_BFIO_AEC_WTK_AEC
#define WTK_BFIO_AEC_WTK_AEC
#include "wtk_aec_cfg.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/bfio/preemph/wtk_preemph.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef void(*wtk_aec_notify_f)(void *ths,short **data,int len,int is_end);
typedef void(*wtk_aec_notify_f2)(void *ths,wtk_stft2_msg_t *msg,int pos,int is_end);

typedef struct wtk_aec wtk_aec_t;
struct wtk_aec
{
    wtk_aec_cfg_t *cfg;
    wtk_stft2_t *stft;
    int nbin;
    int channel;
    float nframe;

    float **output;
    float **pad;

    int end_pos;
    int sp_sil;
    int sp_silcnt;

    wtk_strbuf_t **input;
	float **notch_mem;
	float *memD;
	float *memX;

    wtk_nlms_t *nlms;
    wtk_rls_t *rls;

    float **Yf;
    wtk_complex_t **out;

    float **Se;
    float **Sd;
    wtk_complex_t **Sed;

    wtk_qmmse_t **qmmse;

    wtk_aec_notify_f notify;
    void *ths;

    wtk_aec_notify_f2 notify2;
};

wtk_aec_t* wtk_aec_new(wtk_aec_cfg_t *cfg);
void wtk_aec_reset(wtk_aec_t *aec);
void wtk_aec_delete(wtk_aec_t *aec);
void wtk_aec_set_notify(wtk_aec_t *aec,void *ths,wtk_aec_notify_f notify);
void wtk_aec_feed(wtk_aec_t *aec,short **mic,int len,int is_end);



wtk_aec_t* wtk_aec_new2(wtk_aec_cfg_t *cfg, wtk_stft2_t *stft);
void wtk_aec_reset2(wtk_aec_t *aec);
void wtk_aec_delete2(wtk_aec_t *aec);
void wtk_aec_set_notify2(wtk_aec_t *aec,void *ths,wtk_aec_notify_f2 notify);
void wtk_aec_feed_stftmsg(wtk_aec_t *aec,wtk_stft2_msg_t *smsg,wtk_stft2_msg_t *sp_smsg,int pos,int is_end);

#ifdef __cplusplus
};
#endif
#endif
