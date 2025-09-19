#ifndef WTK_FST_LM_WTK_LMLAT_H_
#define WTK_FST_LM_WTK_LMLAT_H_
#include "wtk/core/wtk_type.h"
#include "wtk/asr/wfst/net/wtk_fst_net2.h"
#include "wtk/core/wtk_array.h"
#include "wtk/core/wtk_str.h"
#include "wtk_lmlat_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_lmlat wtk_lmlat_t;
typedef struct wtk_lm_trans_item wtk_lm_trans_item_t;
typedef struct wtk_lm_history wtk_lm_history_t;
typedef struct wtk_lm_state_item wtk_lm_state_item_t;

struct wtk_lm_history
{
	wtk_queue_node_t inst_n;
	wtk_lm_history_t *prev;
	wtk_wfst_kv_env_t *node;
	wtk_fst_trans2_t *input_trans;
	wtk_fst_state2_t *output_state;
	wtk_lm_trans_item_t *item;
	float prob;
	unsigned used:1;
};

struct wtk_lm_trans_item
{
	wtk_fst_trans2_t *input_trans;
	wtk_lm_history_t **hist;
	//wtk_fst_state2_t *output_to_state;
	int hist_used;
};

struct wtk_lm_state_item
{
	wtk_fst_state2_t *input_state;
	wtk_lm_trans_item_t **trans;
	int trans_used;
};


struct wtk_lmlat
{
	wtk_lmlat_cfg_t *cfg;
	wtk_lm_dict_cfg_t *dict_cfg;
	//wtk_nglm_t *nglm;
	wtk_wfst_kv_t *kv;
	wtk_heap_t *heap;
	wtk_queue_t inst_q;	//wtk_lm_history_t
	wtk_fst_net2_t *output_net;	//cfg is input cfg;
	wtk_fst_net2_t *expand_net;
	wtk_strbuf_t *buf;
	wtk_wfst_kv_env_t *lm_start_node;
	int back_ids[20];
	int nback_id;
};

wtk_lmlat_t* wtk_lmlat_new(wtk_lmlat_cfg_t *cfg,wtk_lm_dict_cfg_t *dict_cfg);
void wtk_lmlat_delete(wtk_lmlat_t *lat);
int wtk_lmlat_bytes(wtk_lmlat_t *lat);
void wtk_lmlat_reset(wtk_lmlat_t *lat);
void wtk_lmlat_update_history(wtk_lmlat_t *lat,int *idx,int nid);
void wtk_lmlat_update_clean_hist(wtk_lmlat_t *lat);
wtk_fst_net2_t* wtk_lmlat_process(wtk_lmlat_t *r,wtk_fst_net2_t *input);
void wtk_lmlat_update_clean_hist(wtk_lmlat_t *lat);
void wtk_lmlat_update_history(wtk_lmlat_t *lat,int *idx,int nid);
#ifdef __cplusplus
};
#endif
#endif
