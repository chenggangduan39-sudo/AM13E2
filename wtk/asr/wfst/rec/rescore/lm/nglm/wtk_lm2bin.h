#ifndef WTK_FST_LM_WTK_LM2BIN_H_
#define WTK_FST_LM_WTK_LM2BIN_H_
#include "wtk/core/wtk_type.h"
#include "wtk/asr/wfst/net/sym/wtk_fst_insym.h"
#include "wtk/core/cfg/wtk_source.h"
#include "wtk/core/wtk_array.h"
#include "wtk/core/wtk_larray.h"
#include "wtk/core/wtk_queue2.h"
#include "wtk/core/wtk_vpool.h"
#include "wtk_lm_node.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_lm2bin wtk_lm2bin_t;

#define MAX_LM_ORDER 100



struct wtk_lm2bin
{
	wtk_label_t *label;
	wtk_fst_insym_t *sym;
	FILE *olog;
	uint64_t order[MAX_LM_ORDER];
	int max_order;
	wtk_lm_node_t *uni_nodes;
	wtk_strbuf_t *buf;
	wtk_vpool_t *node_pool;
	float prob_scale;
	float bow_scale;
	wtk_lm2bin_type_t type;
	//unsigned use_small:1;
};

wtk_lm2bin_t* wtk_lm2bin_new();
void wtk_lm2bin_delete(wtk_lm2bin_t *b);
int wtk_lm2bin_dump(wtk_lm2bin_t *b,char *wl_fn,char *lm_fn,char *bin_fn);
#ifdef __cplusplus
};
#endif
#endif
