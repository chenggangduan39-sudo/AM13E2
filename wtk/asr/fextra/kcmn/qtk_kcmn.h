#ifndef QTK_KCMN_H_
#define QTK_KCMN_H_
#include "wtk/core/math/wtk_vector.h"
#include "wtk/core/wtk_robin.h"
#include "wtk/asr/fextra/kparm/wtk_kfeat.h"
#include "qtk_kcmn_cfg.h"
#include "wtk/asr/fextra/pitch/core/qtk_array.h"

#ifdef __cplusplus
extern "C" {
#endif
/**
 * cepstral mean normalization
 */
typedef struct qtk_kcmn qtk_kcmn_t;

struct qtk_kcmn
{
	qtk_kcmn_cfg_t *cfg;
	void *notify_ths;
	wtk_kfeat_notify_f notify;
	float *sumq;
	float *stat;
	int frame_cnt;
	int cur_cnt;
	int cmvn_cnt;
	int feat_dim;
	qtk_array_t *input;//float** save origin feats
	qtk_array_t *output;//float** save cmvn results
	//wtk_queue_t pend_q;				//used for delay;
	int vec_size;
	int frames;
	unsigned save_cmn:1;
};

qtk_kcmn_t* qtk_kcmn_new(qtk_kcmn_cfg_t *cfg,int len);
int qtk_kcmn_delete(qtk_kcmn_t *z);
int qtk_kcmn_reset(qtk_kcmn_t *z);
int qtk_kcmn_last_frame(qtk_kcmn_t *cmn,int n);
int qtk_kcmn_dim(qtk_kcmn_t *cmn);
int qtk_kcmn_frame_ready(qtk_kcmn_t *cmn);
void qtk_kcmn_get_raw_frame(qtk_kcmn_t *cmn,int frame, float* feats, int len);
void qtk_kcmn_get_out_frame(qtk_kcmn_t *cmn,int frame, float* feats, int len);
void qtk_kcmn_test(qtk_kcmn_t *cmn);
void qtk_kcmn_feed(qtk_kcmn_t *cmn,wtk_kfeat_t *feat);
void qtk_kcmn_flush(qtk_kcmn_t *cmn);
void qtk_kcmn_set_notify(qtk_kcmn_t *cmn,void *ths,wtk_kfeat_notify_f notify);
///**
// * @brief parm static post callback;
// */
//void wtk_cmn_static_post_feed(wtk_cmn_t *p,wtk_feat_t *f);
//
///**
// * @brief parm post callback;
// */
//void wtk_cmn_post_feed(wtk_cmn_t *p,wtk_feat_t *f);
//
///**
// * @brief flush post queue;
// */
//void wtk_cmn_flush_extra_post_queue(wtk_cmn_t *z);
//
//
//
////=======================================   private =============================
///**
// * @brief used for cache cmn buf;
// */
//void wtk_cmn_update_buf(wtk_cmn_t *z,wtk_vector_t *v);
//
///**
// * @brief used for vparm cmn vector update;
// */
//void wtk_cmn_update_cmn(wtk_cmn_t *z);
//
///**
// * @brief used for vparm feature process;
// */
//void wtk_cmn_process_cmn(wtk_cmn_t *z,wtk_vector_t *feat,int idx);
//
///**
// * @brief flush mean;
// */
//void wtk_cmn_flush(wtk_cmn_t *z,int force);
//
//int wtk_cmn_can_flush_all(wtk_cmn_t *z);
//void wtk_cmn_dup_hist(wtk_cmn_t *dst,wtk_cmn_t *src);
#ifdef __cplusplus
};
#endif
#endif
