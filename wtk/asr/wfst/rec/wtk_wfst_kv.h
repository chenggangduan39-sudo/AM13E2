#ifndef WTK_FST_REC_REL_WTK_REL_KV
#define WTK_FST_REC_REL_WTK_REL_KV
#include "wtk/core/wtk_type.h" 
#include "wtk/asr/wfst/rec/rescore/lm/nglm/wtk_nglm.h"
#include "wtk_wfst_kv_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_wfst_kv wtk_wfst_kv_t;


typedef struct
{
	union{
		wtk_lm_node_t node;
		wtk_rnn_dec_env_t rnn;
	}v;
}wtk_wfst_kv_env_t;

typedef struct
{
	wtk_wfst_kv_env_t *node;
	float like;
}wtk_wfst_kv_item_t;

struct wtk_wfst_kv
{
	wtk_wfst_kv_cfg_t *cfg;
	wtk_strbuf_t *buf;
	wtk_str_hash_t *hash;
	wtk_rnn_dec_t *rnn;
	wtk_nglm_t *ngram;
};

wtk_wfst_kv_t* wtk_wfst_kv_new(wtk_wfst_kv_cfg_t *cfg);
void wtk_wfst_kv_delete(wtk_wfst_kv_t *kv);
void wtk_wfst_kv_reset(wtk_wfst_kv_t *kv);
//wtk_lm_node_t* wtk_rel_kv_get2(wtk_rel_kv_t *kv,wtk_lm_node_t *node,unsigned int id,double *plike);

wtk_wfst_kv_env_t* wtk_wfst_kv_get_root(wtk_wfst_kv_t *kv);
wtk_wfst_kv_env_t* wtk_wfst_kv_get(wtk_wfst_kv_t *kv,wtk_wfst_kv_env_t *node,unsigned int id,double *plike);
wtk_wfst_kv_env_t* wtk_wfst_kv_get_root2(wtk_wfst_kv_t *kv,int *idx,int nid);
#ifdef __cplusplus
};
#endif
#endif
