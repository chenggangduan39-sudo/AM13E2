#ifndef WTK_VITE_VPRINT_TRAIN_WTK_VTRAIN
#define WTK_VITE_VPRINT_TRAIN_WTK_VTRAIN
#include "wtk/core/wtk_type.h" 
#include "wtk_vtrain_cfg.h"
#include "wtk/asr/vprint/parm/wtk_vparm.h"
#include "wtk/core/rbin/wtk_rbin2.h"
#include "wtk/core/rbin/wtk_ubin.h"
//#include "wtk/os/wtk_log.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_vtrain wtk_vtrain_t;

typedef float wtk_vtrain_float_t;

typedef struct
{
	wtk_vtrain_float_t *mp;/* mean vector counts */
	wtk_vtrain_float_t occ;/* occ for states sharing this mpdf */
}wtk_vtrain_mix_t;


typedef struct
{
	wtk_vtrain_mix_t *mix;
	wtk_vtrain_float_t *mixprob;
	int nmix;
	int vsize;	/* feature vector size*/
}wtk_vtrain_acc_t;

typedef struct
{
	wtk_vtrain_mix_t *mix;
	int nmix;
	int vsize;	/* feature vector size*/
}wtk_vtrain_acc2_t;	//used for acc;


struct wtk_vtrain
{
	wtk_vtrain_cfg_t *cfg;
	wtk_vparm_cfg_t *parm_cfg;
	wtk_vparm_t *parm;
	wtk_heap_t *heap;
	wtk_vtrain_acc_t *acc;
	wtk_vtrain_acc2_t *acc2;	//用于持续累计;
	//wtk_stream_t *raw_speech;
	wtk_stream_t *speech;
	int index;
	int cnt;
	int nframe;
	//wtk_log_t *log;
};

wtk_vtrain_t* wtk_vtrain_new(wtk_vtrain_cfg_t *cfg,wtk_vparm_cfg_t *parm_cfg,int use_share_parm);
void wtk_vtrain_delete(wtk_vtrain_t *t);
void wtk_vtrain_reset(wtk_vtrain_t *t);
void wtk_vtrain_reset_acc2(wtk_vtrain_t *t);
void wtk_vtrain_update_acc2(wtk_vtrain_t *v);
void wtk_vtrain_start(wtk_vtrain_t *t);
void wtk_vtrain_feed(wtk_vtrain_t *v,char *data,int bytes,int is_end);
void wtk_vtrain_feed_feature(wtk_vtrain_t *v,wtk_vparm_feature_t *f,int is_end);
void wtk_vtrain_update_hmm(wtk_vtrain_t *v);
void wtk_vtrain_write_hmm(wtk_vtrain_t *v,char *fn);
void wtk_vtrain_write_hmm_bin(wtk_vtrain_t *v,char *fn);
void wtk_vtrain_write_hmm_bin2(wtk_vtrain_t *v,char *fn,int len,wtk_ubin_t *ubin);
#ifdef __cplusplus
};
#endif
#endif
