#ifndef WTK_BFIO_DEREVERB_WTK_ADMM
#define WTK_BFIO_DEREVERB_WTK_ADMM
#include "wtk_admm_cfg.h"
#include "wtk/core/wtk_wavfile.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef void(*wtk_admm_notify_f)(void *ths,short **data,int len,int is_end);
typedef void(*wtk_admm_notify_f2)(void *ths,wtk_stft2_msg_t *msg,wtk_stft2_msg_t *err_msg,wtk_stft2_msg_t *lsty_msg,int pos,int is_end);

typedef struct wtk_admm wtk_admm_t;
struct wtk_admm
{
    wtk_admm_cfg_t *cfg;
    wtk_stft2_t *stft;
    int nbin;
    int channel;
    float nframe;

    wtk_complex_t **norig;

    float **output;
    float **pad;

    int end_pos;

    wtk_rls_t *rls;

    wtk_complex_t **out;

    wtk_admm_notify_f notify;
    void *ths;

    wtk_admm_notify_f2 notify2;
};

wtk_admm_t* wtk_admm_new(wtk_admm_cfg_t *cfg);
void wtk_admm_reset(wtk_admm_t *admm);
void wtk_admm_delete(wtk_admm_t *admm);
void wtk_admm_set_notify(wtk_admm_t *admm,void *ths,wtk_admm_notify_f notify);
void wtk_admm_feed(wtk_admm_t *admm,short **mic,int len,int is_end);



wtk_admm_t* wtk_admm_new2(wtk_admm_cfg_t *cfg, wtk_stft2_t *stft);
void wtk_admm_reset2(wtk_admm_t *admm);
void wtk_admm_delete2(wtk_admm_t *admm);
void wtk_admm_set_notify2(wtk_admm_t *admm,void *ths,wtk_admm_notify_f2 notify);
void wtk_admm_feed_stftmsg(wtk_admm_t *admm,wtk_stft2_msg_t *smsg,int pos,int is_end);

#ifdef __cplusplus
};
#endif
#endif