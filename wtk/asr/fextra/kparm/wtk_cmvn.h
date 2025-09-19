#ifndef WTK_ASR_PARM_WTK_CMVN
#define WTK_ASR_PARM_WTK_CMVN
#include "wtk/core/wtk_type.h" 
#include "wtk_cmvn_cfg.h"
#include "wtk_kfeat.h"
#include "wtk/core/wtk_fixpoint.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_cmvn wtk_cmvn_t;


struct wtk_cmvn
{
	wtk_cmvn_cfg_t *cfg;
	wtk_queue_t q;
        wtk_queue_t post_q;
        wtk_queue_t feat_q;
        int vec_size;
	float *mean;
	int nframe;
	int nvalid;
	void *notify_ths;
	wtk_kfeat_notify_f notify;
	int sliding_flag;
};

wtk_cmvn_t* wtk_cmvn_new(wtk_cmvn_cfg_t *cfg,int vec_size);
int wtk_cmvn_bytes(wtk_cmvn_t *cmvn);
void wtk_cmvn_delete(wtk_cmvn_t *cmvn);
void wtk_cmvn_reset(wtk_cmvn_t *cmvn);
void wtk_cmvn_set_notify(wtk_cmvn_t *cmvn,void *ths,wtk_kfeat_notify_f notify);
void wtk_cmvn_feed(wtk_cmvn_t *cmvn,wtk_kfeat_t *feat);
void wtk_cmvn_flush(wtk_cmvn_t *cmvn);
void wtk_cmvn_flush_fix(wtk_cmvn_t *cmvn);
void wtk_cmvn_feed_fix(wtk_cmvn_t *cmvn,wtk_kfeat_t *feat);

void wtk_cmvn_print(wtk_cmvn_t *cmvn);
#ifdef __cplusplus
};
#endif
#endif
