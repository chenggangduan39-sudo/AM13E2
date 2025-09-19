#ifndef WTK_FST_LM_GEN_WTK_LMGEN2
#define WTK_FST_LM_GEN_WTK_LMGEN2
#include "wtk/core/cfg/wtk_local_cfg.h" 
#include "wtk/core/wtk_queue.h"
#include "wtk/core/wtk_vpool.h"
#include "wtk/core/wtk_larray.h"
#include "wtk_lmgen2_cfg.h"
#include "wtk/core/wtk_str_parser.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_lmgen2 wtk_lmgen2_t;

typedef struct wtk_lmgen2_tok wtk_lmgen2_tok_t;
typedef struct wtk_lmgen2_path wtk_lmgen2_path_t;

struct wtk_lmgen2_path
{
	wtk_lmgen2_path_t *prev;
	wtk_lm_node_t *node;
	float prob;
	float ppl;
	int depth;
};

struct wtk_lmgen2_tok
{
	wtk_queue_node_t q_n;
	wtk_lmgen2_path_t *pth;
};

struct wtk_lmgen2
{
	wtk_lmgen2_cfg_t *cfg;
	wtk_rbin2_t *rbin;
	wtk_nglm_t *lm;
	//wtk_heap_t *heap;
	wtk_larray_t *filter;
	wtk_vpool_t *tok_heap;
	wtk_vpool_t *pth_heap;
	wtk_prune_t *prune;
	wtk_prune_t *predict_prune;
	wtk_queue_t tok_q;
	float max;
	float last_max;
	float max_ppl;
	wtk_lmgen2_path_t *max_pth;
	wtk_strbuf_t *buf;
	int index;
	int is_last:1;
};

wtk_lmgen2_t* wtk_lmgen2_new(wtk_lmgen2_cfg_t *cfg,wtk_rbin2_t *rbin);
void wtk_lmgen2_delete(wtk_lmgen2_t *gen);
void wtk_lmgen2_reset(wtk_lmgen2_t *g);
void wtk_lmgen2_start(wtk_lmgen2_t *g,float max_ppl);
void wtk_lmgen2_add_filter(wtk_lmgen2_t *g,char *v,int v_len);
void wtk_lmgen2_add_filter2(wtk_lmgen2_t *g,char *v,int v_len);
void wtk_lmgen2_feed(wtk_lmgen2_t *gen,char *v,int v_len);
void wtk_lmgen2_feed_last(wtk_lmgen2_t *g);
wtk_string_t wtk_lmgen2_get_result(wtk_lmgen2_t *g);
void wtk_lmgen2_print(wtk_lmgen2_t *g);

wtk_string_t wtk_lmgen2_predict(wtk_lmgen2_t *g,char *v,int v_len,int max_wrd,float max_ppl);
void wtk_lmgen2_print_path(wtk_lmgen2_t *g,wtk_lmgen2_path_t *pth);
#ifdef __cplusplus
};
#endif
#endif
