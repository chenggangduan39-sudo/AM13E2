#ifndef WTK_KSR_PARM_WTK_KXPARM
#define WTK_KSR_PARM_WTK_KXPARM
#include "wtk/core/wtk_type.h" 
#include "wtk_kxparm_cfg.h"
#include "wtk/asr/fextra/kcmn/qtk_kcmn.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_kxparm wtk_kxparm_t;

typedef void(*wtk_kxparm_notify_f)(void *ths,wtk_kfeat_t *feat);
typedef void(*wtk_kxparm_notify_f2)(void *ths,wtk_feat_t *feat);
typedef void(*wtk_kxparm_notify_end_f)(void *ths);
typedef char (*wtk_kxparm_callback_f)(void *ths,wtk_kfeat_t *feat);
typedef void(*wtk_ivector_notify_f)(void *ths,float *feat,int dim);//notify kxparm update ivector

struct wtk_kxparm
{
	wtk_kxparm_cfg_t *cfg;
	wtk_fextra_t *htk;
	wtk_kparm_t *parm;
	wtk_knn_t *knn;
	qtk_nnet3_t* nnet3;
	wtk_kxparm_notify_f notify;
	wtk_kxparm_notify_f2 notify2;
	wtk_kxparm_notify_end_f notify_end;
	void *ths;

	wtk_kxparm_callback_f vad_callback;
	void *vad_ths;

	wtk_kxparm_notify_f kind_notify;
	void *kind_ths;
};

int wtk_kxparm_bytes(wtk_kxparm_t *parm);
wtk_kxparm_t* wtk_kxparm_new(wtk_kxparm_cfg_t *cfg);
void wtk_kxparm_delete(wtk_kxparm_t *parm);
void wtk_kxparm_set_notify(wtk_kxparm_t *parm,void *ths,wtk_kxparm_notify_f notify);
void wtk_kxparm_set_notify2(wtk_kxparm_t *parm,void *ths,wtk_kxparm_notify_f2 notify);
void wtk_kxparm_set_notify_end(wtk_kxparm_t *parm,void *ths,wtk_kxparm_notify_end_f notify);
void wtk_kxparm_start(wtk_kxparm_t *parm);
void wtk_kxparm_reset(wtk_kxparm_t *parm);
void wtk_kxparm_reset2(wtk_kxparm_t *parm,int reset_cmn);
void wtk_kxparm_feed(wtk_kxparm_t *parm,short *data,int len,int is_end);
void wtk_kxparm_feed_float(wtk_kxparm_t *parm,float *data,int len,int is_end);
void wtk_kxparm_feed_feat(wtk_kxparm_t *parm,wtk_kfeat_t *feat,int is_speech,int is_end);
void wtk_kxparm_feed_feat3(wtk_kxparm_t *parm,wtk_kfeat_t *feat,int is_speech,int is_end);
void wtk_kxparm_feed_feat2(wtk_kxparm_t *parm,wtk_kfeat_t *feat,int is_end);



void wtk_kxparm_set_vad_callback(wtk_kxparm_t *parm,void *ths,wtk_kxparm_callback_f notify);
void wtk_kxparm_set_kind_notify(wtk_kxparm_t *parm,void *ths,wtk_kxparm_notify_f notify);
#ifdef __cplusplus
};
#endif
#endif
