#ifndef QTK_KWDEC_H_
#define QTK_KWDEC_H_
#include <math.h>
#include <limits.h>
#include <float.h>
#include "qtk_kwdec_cfg.h"
#include "wtk/asr/wfst/net/wtk_fst_net.h"
#include "wtk/asr/wfst/kwdec/qtk_wakeup_trans_model_cfg.h"
#include "wtk/core/math/wtk_vector.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/asr/fextra/wtk_feat.h"
#include "wtk/asr/wfst/net/wtk_fst_net2.h"
#include "wtk/asr/wfst/net/wtk_fst_net3.h"
#include "wtk/asr/fextra/nnet3/qtk_nnet3_component.h"
#include "wtk/core/wtk_vpool2.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_kwdec_token qtk_kwdec_token_t;
typedef struct qtk_kwdec_pth qtk_kwdec_pth_t;
typedef struct qtk_kwdec qtk_kwdec_t;
typedef struct qtk_kwdec_words qtk_kwdec_words_t;
extern int qtk_kwdec_post_feed(qtk_kwdec_t* dec, wtk_vector_t* f);

struct qtk_kwdec_token
{
	wtk_queue_node_t q_n;
	float tot_cost;
	//qtk_kwdec_link_t *link;//forward link for lattice
	qtk_kwdec_pth_t *pth;//backward path for one best path
	wtk_fst_state_t *state;
};

struct qtk_kwdec_pth
{
	int out_label;
	int in_label;
	float like;
	qtk_kwdec_pth_t *lbest;
	int frame;
};

struct qtk_kwdec_words
{
	char* word;
	int key_id;
	float pdf_conf;
	int min_kws;
};

struct qtk_kwdec
{
	qtk_kwdec_cfg_t* cfg;
	//wtk_queue_t state_q;
	wtk_queue_t *cur_tokq;
	wtk_queue_t *pre_tokq;
	wtk_fst_net_t *net; //get fst from this
	qtk_wakeup_trans_model_t* trans_model;
	qtk_kwdec_token_t *best_token;
	wtk_heap_t *heap;
	wtk_vpool2_t *tok_pool;
	qtk_kwdec_words_t **words_set;
	int cur_frame;
	int decoding_finalized;
	unsigned int *bins;
	float best_weight;
	//float best_final_cost;
	//float final_relative_cost;
	//float final_best_cost;
	float conf;
	int found;
    int wake_flag;
//  wtk_vpool_t *feat_pool;
	wtk_robin_t *feat_rb;
	int wake_beg_idx;
	int wake_end_idx;
	int key_id;
	int min_kws;
	float pdf_conf;
	int out_col;
	unsigned int reset;
	int reset_frame;
};

qtk_kwdec_t* qtk_kwdec_new(qtk_kwdec_cfg_t* cfg);
int qtk_kwdec_start(qtk_kwdec_t* dec);
int qtk_kwdec_feed(qtk_kwdec_t* dec,wtk_feat_t *f);
int qtk_kwdec_feed2(qtk_kwdec_t* dec,wtk_vector_t *v,int index);
//void qtk_kwdec_get_hint_result(qtk_kwdec_t* dec,wtk_strbuf_t *buf);
void qtk_kwdec_reset(qtk_kwdec_t* dec);
void qtk_kwdec_delete(qtk_kwdec_t* dec);

#ifdef __cplusplus
}
;
#endif
#endif

