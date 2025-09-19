#ifndef WTK_CYBER_CODE_WTK_ZMEAN_H_
#define WTK_CYBER_CODE_WTK_ZMEAN_H_
#include "wtk/core/math/wtk_vector.h"
#include "wtk/core/wtk_robin.h"
#include "../wtk_feat.h"
#include "wtk_cmn_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
/**
 * cepstral mean normalization
 */
typedef struct wtk_cmn wtk_cmn_t;

struct wtk_fextra;

typedef struct
{
	wtk_queue_node_t q_n;
	int idx;
	float *feat;
}wtk_cmn_feat_t;


struct wtk_cmn
{
	wtk_cmn_cfg_t *cfg;
	struct wtk_fextra *parm;
	//wtk_queue_t pend_q;				//used for delay;
	wtk_queue_t post_feature_q;		//used for post update;
	wtk_vector_t *cur;	//!< current mean vector.
	wtk_vector_t *buf;	//!< accumulator buffer for cmn update.
	wtk_queue_t cmn_q;
	int vec_size;
	int frames;
	unsigned save_cmn:1;
	unsigned use_post2:1;
};

wtk_cmn_t* wtk_cmn_new(wtk_cmn_cfg_t *cfg,struct wtk_fextra *parm);
int wtk_cmn_delete(wtk_cmn_t *z);
int wtk_cmn_reset(wtk_cmn_t *z);

/**
 * @brief parm static post callback;
 */
void wtk_cmn_static_post_feed(wtk_cmn_t *p,wtk_feat_t *f);
void wtk_cmn_static_post_feed2(wtk_cmn_t *z,wtk_feat_t *f);

/**
 * @brief parm post callback;
 */
void wtk_cmn_post_feed(wtk_cmn_t *p,wtk_feat_t *f);

/**
 * @brief flush post queue;
 */
void wtk_cmn_flush_extra_post_queue(wtk_cmn_t *z);
void wtk_cmn_flush_extra_post_queue2(wtk_cmn_t *z);


//=======================================   private =============================
/**
 * @brief used for cache cmn buf;
 */
void wtk_cmn_update_buf(wtk_cmn_t *z,wtk_vector_t *v);

/**
 * @brief used for vparm cmn vector update;
 */
void wtk_cmn_update_cmn(wtk_cmn_t *z);

/**
 * @brief used for vparm feature process;
 */
void wtk_cmn_process_cmn(wtk_cmn_t *z,wtk_vector_t *feat,int idx);

/**
 * @brief flush mean;
 */
void wtk_cmn_flush(wtk_cmn_t *z,int force);

int wtk_cmn_can_flush_all(wtk_cmn_t *z);
void wtk_cmn_dup_hist(wtk_cmn_t *dst,wtk_cmn_t *src);
#ifdef __cplusplus
};
#endif
#endif
