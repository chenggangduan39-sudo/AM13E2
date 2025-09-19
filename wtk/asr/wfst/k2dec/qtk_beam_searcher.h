#ifndef QTK_BEAM_SEARCHER_H_
#define QTK_BEAM_SEARCHER_H_
#include "qtk_k2_context_net.h"
#include "qtk_k2_wrapper_cfg.h"
#include "wtk/core/wtk_vpool2.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_beam_searcher qtk_beam_searcher_t;
typedef struct qtk_beam_search_hyp qtk_beam_search_hyp_t;
typedef struct qtk_beam_searcher_result qtk_beam_searcher_result_t;
typedef struct qtk_beam_searcher_keyword qtk_beam_searcher_keyword_t;

struct qtk_beam_search_hyp
{
	wtk_strbuf_t *ys;//int buf
	wtk_strbuf_t *timestamp;//int buf
	wtk_strbuf_t *hw_t;
	wtk_strbuf_t *hw_match_t;
	wtk_strbuf_t *token_score;//float buf
	float log_prob;
	float context_score;
	float am_score;
	float acc_score;
	int state_id[5];
	int state_id2;
};

struct qtk_beam_searcher_result
{
	float key_avg_prob;
	float whole_avg_prob;
	int start_idx;
	int end_idx;
	int label;
	int kw_score_cnt;
	int tokens_cnt;
	wtk_queue_node_t q_n;
};


struct qtk_beam_searcher_keyword
{
	float key_avg_prob;
	float whole_avg_prob;
	float aver_amprob;
	float ref_aver_amprob;
	int num_toks;
	int keywrd;
	int nihao;
	wtk_queue_t resq;
	wtk_strbuf_t *timestamp;
	qtk_beam_search_hyp_t *best_pth;
	qtk_beam_search_hyp_t *ref_pth;
};

struct qtk_beam_searcher
{
	qtk_k2_wrapper_cfg_t *cfg;
	qtk_beam_search_hyp_t **toks;
	qtk_beam_search_hyp_t **ptoks;
	qtk_beam_search_hyp_t **ntoks;
	qtk_k2_context_net_t *net;
	wtk_vpool2_t *pool;
	qtk_beam_searcher_keyword_t keywrd;
	int context_net_ok;
	float *probs;
	int *indexes;
	int *hyp_indexes;
	int num_toks;
	int ntopk;
	int out_col;
	int index;
};

qtk_beam_searcher_t* qtk_beam_searcher_new(qtk_k2_wrapper_cfg_t* cfg);
int qtk_beam_searcher_start(qtk_beam_searcher_t* searcher);
int qtk_beam_searcher_feed(qtk_beam_searcher_t* searcher,float *f, float *f2,int n,int64_t* dshape,int64_t* dinput);
void qtk_beam_searcher_reset(qtk_beam_searcher_t* searcher);
void qtk_beam_searcher_delete(qtk_beam_searcher_t* searcher);
void qtk_beam_searcher_keyword_detect(qtk_beam_searcher_t* searcher);
void qtk_beam_searcher_keyword_detect2(qtk_beam_searcher_t* searcher);
void qtk_beam_searcher_debug(qtk_beam_searcher_t* wrapper,qtk_beam_search_hyp_t **toks,int x);
void qtk_beam_search_keywrd_reset(qtk_beam_searcher_t* searcher);
#ifdef __cplusplus
};
#endif
#endif

