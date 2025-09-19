#ifndef WTK_BFIO_WTK_WBF2
#define WTK_BFIO_WTK_WBF2
#include "wtk/bfio/wtk_wbf2_cfg.h" 
#ifdef __cplusplus
extern "C" {
#endif
typedef void(*wtk_wbf2_notify_f)(void *ths,short *data,int len,int idx,int is_end);
typedef void(*wtk_wbf2_notify_f2)(void *ths,wtk_complex_t *wbf2_out,int idx,int is_end);

typedef struct wtk_wbf2 wtk_wbf2_t;
struct wtk_wbf2
{
    wtk_wbf2_cfg_t *cfg;

    wtk_stft2_t *stft2;
    wtk_qform2_t **qform2;

    wtk_qform3_t **qform3;
    
    int channel;
    int nbin;
    int theta_step;
    int end_pos;
    int theta_range;

    void *ths;
	wtk_wbf2_notify_f notify;
	wtk_wbf2_notify_f2 notify2;
};

wtk_wbf2_t* wtk_wbf2_new(wtk_wbf2_cfg_t *cfg);

void wtk_wbf2_delete(wtk_wbf2_t *wbf2);

void wtk_wbf2_reset(wtk_wbf2_t *wbf2);

void wtk_wbf2_set_notify(wtk_wbf2_t *wbf2,void *ths,wtk_wbf2_notify_f notify);

void wtk_wbf2_start(wtk_wbf2_t *wbf2);

void wtk_wbf2_feed(wtk_wbf2_t *wbf2,short **data,int len,int is_end);


wtk_wbf2_t* wtk_wbf2_new2(wtk_wbf2_cfg_t *cfg, wtk_stft2_t *stft2);

void wtk_wbf2_delete2(wtk_wbf2_t *wbf2);

void wtk_wbf2_reset2(wtk_wbf2_t *wbf2);

void wtk_wbf2_set_notify2(wtk_wbf2_t *wbf2,void *ths,wtk_wbf2_notify_f2 notify);

void wtk_wbf2_feed_smsg(wtk_wbf2_t *wbf2,wtk_stft2_msg_t *msg,int pos,int is_end);


#ifdef __cplusplus
};
#endif
#endif
