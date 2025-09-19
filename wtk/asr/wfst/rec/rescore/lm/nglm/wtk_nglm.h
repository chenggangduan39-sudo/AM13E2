#ifndef WTK_FST_LM_WTK_NGLM_H_
#define WTK_FST_LM_WTK_NGLM_H_
#include "wtk/core/wtk_type.h"
#include "wtk/core/wtk_larray.h"
#include "wtk_lm_node.h"
#include "wtk_nglm_cfg.h"
#include "wtk_fbin.h"
#include "wtk_lm_dict_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_nglm wtk_nglm_t;

struct wtk_nglm
{
	wtk_nglm_cfg_t *cfg;
	wtk_lm_dict_cfg_t *dict_cfg;
	wtk_fbin_t *fbin;
	wtk_heap_t *glb_heap;
	wtk_heap_t *loc_heap;
	wtk_lm_node_t *uni_nodes;
	int n_uni_nodes;
	wtk_strbuf_t *buf;
	wtk_lm_node_t *s_node;
	double prob;
	wtk_larray_t *valid_uni_node;
	float prob_scale;
	float bow_scale;
	unsigned int cnt;
	unsigned int bytes;
	unsigned int n_order[4];
	wtk_lm2bin_type_t type;
};

wtk_nglm_t* wtk_nglm_new(wtk_nglm_cfg_t *cfg,wtk_lm_dict_cfg_t *dict_cfg);
void wtk_nglm_delete(wtk_nglm_t *lm);
void wtk_nglm_reset(wtk_nglm_t *lm);
int wtk_nglm_bytes(wtk_nglm_t *lm);
wtk_lm_node_t* wtk_nglm_get_uni_node(wtk_nglm_t *lm,unsigned int id);
wtk_lm_node_t* wtk_nglm_get_child_prob(wtk_nglm_t *lm,wtk_lm_node_t *node,int id,double *pbow);
wtk_lm_node_t* wtk_nglm_get_node(wtk_nglm_t *lm,int *ids,int cnt);
wtk_lm_node_t *wtk_nglm_get_node2(wtk_nglm_t *lm,char *data,int bytes);
double wtk_nglm_get_prob(wtk_nglm_t *lm,char *data,int bytes);
void wtk_nglm_dump(wtk_nglm_t *lm,char *fn);
void wtk_nglm_touch_node(wtk_nglm_t *lm,wtk_lm_node_t *node);
wtk_lm_node_t* wtk_nglm_get_child(wtk_nglm_t *lm,wtk_lm_node_t *node,unsigned int id);
wtk_lm_node_t* wtk_nglm_get_bow_node(wtk_nglm_t *lm,wtk_lm_node_t *node);
wtk_lm_node_t* wtk_nglm_get_bow_node2(wtk_nglm_t *lm,wtk_lm_node_t *node);
wtk_lm_node_t* wtk_nglm_get_node_by_id(wtk_nglm_t *lm,int *ids,int cnt);
void wtk_nglm_print_ids(wtk_nglm_t *lm,int *ids,int cnt);
void wtk_nglm_print_node(wtk_nglm_t *lm,wtk_lm_node_t *n);
#ifdef __cplusplus
};
#endif
#endif
