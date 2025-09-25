#ifndef WTK_FST_LM_GEN_WTK_LMGEN_REC
#define WTK_FST_LM_GEN_WTK_LMGEN_REC
#include "wtk/core/cfg/wtk_local_cfg.h" 
#include "wtk_lmgen_rec_cfg.h"
#include "wtk/core/wtk_larray.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_lmgen_rec wtk_lmgen_rec_t;
struct wtk_lmgen;


typedef struct
{
	wtk_queue_node_t q_n;
	wtk_lm_node_t *node;
	wtk_array_t *a;
	wtk_array_t *b;
	float prob;
	float ppl;
	float ppl1;
	float hit_prob;
	int nhit;
}wtk_lmgen_tok_t;

typedef struct
{
	wtk_queue_node_t q_n;
	wtk_lmgen_tok_t *tok;
	wtk_string_t *v;
	float ppl;
	float ppl1;
}wtk_lmgen_out_t;

typedef struct
{
	wtk_string_t hit_key;
	unsigned int hit_id;
	float prob;
	unsigned use:1;
	unsigned stop:1;
}wtk_lmgen_hit_t;

typedef struct
{
	wtk_lm_node_t *node;
	float prob;
	float hit_prob;
}wtk_lmgen_tx_t;


struct wtk_lmgen_rec
{
	wtk_lmgen_rec_cfg_t *cfg;
	struct wtk_lmgen *gen;
	wtk_lm_dict_cfg_t *dict;
	wtk_nglm_t *lm;
	wtk_heap_t *heap;
	wtk_lmgen_tx_t *tx;
	int end_id;

	wtk_queue_t tok_q;
	wtk_queue_t output_q;	//wtk_lmgen_out_t
	float max_like;
	int depth;

	wtk_strbuf_t *buf;
	wtk_lmgen_hit_t *hits;
	int nhit;
	int min_hit;
};

wtk_lmgen_rec_t* wtk_lmgen_rec_new(wtk_lmgen_rec_cfg_t *cfg,struct wtk_lmgen *gen);
void wtk_lmgen_rec_delete(wtk_lmgen_rec_t *gen);
void wtk_lmgen_rec_reset(wtk_lmgen_rec_t *gen);
void wtk_lmgen_rec_test(wtk_lmgen_rec_t *gen);
void wtk_lmgen_rec_print(wtk_lmgen_rec_t *g);

int wtk_lmgen_rec_backward(wtk_lmgen_rec_t *r,wtk_lmgen_hit_t *hits,int n);
int wtk_lmgen_rec_forward(wtk_lmgen_rec_t *r,wtk_lmgen_hit_t *hits,int n,wtk_queue_t *backward_q);
int wtk_lmgen_rec_dec(wtk_lmgen_rec_t *r,wtk_lmgen_hit_t *hits,int n);
#ifdef __cplusplus
};
#endif
#endif
