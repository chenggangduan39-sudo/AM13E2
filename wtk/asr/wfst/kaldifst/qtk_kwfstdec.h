#ifndef QTK_KWFSTDEC_H_
#define QTK_KWFSTDEC_H_
#include <math.h>
#include <limits.h>
#include <float.h>
#include "qtk_kwfstdec_cfg.h"
#include "wtk/asr/wfst/net/wtk_fst_net.h"
#include "wtk/asr/wfst/kaldifst/basetype/qtk_hash_list.h"
#include "wtk/asr/wfst/net/wtk_fst_net.h"
#include "wtk/asr/wfst/kaldifst/qtk_trans_model_cfg.h"
#include "wtk/core/math/wtk_vector.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/asr/fextra/wtk_feat.h"
#include "wtk/asr/wfst/net/wtk_fst_net2.h"
#include "wtk/asr/wfst/net/wtk_fst_net3.h"
#include "wtk/asr/wfst/rec/rescore/wtk_rescore.h"
#include "wtk/asr/fextra/nnet3/qtk_nnet3_component.h"
#include "wtk/core/strlike/wtk_chnlike.h"
#include "wtk/core/wtk_vpool2.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_kwfstdec_token_list qtk_kwfstdec_token_list_t;
typedef struct qtk_kwfstdec qtk_kwfstdec_t;
typedef struct st_q st_q_t;
typedef struct ct_q ct_q_t;
typedef struct tmp_q tmp_q_t;
typedef struct final_cost_q final_cost_q_t;
typedef struct token_map token_map_t;
typedef struct qtk_kwfstdec_class qtk_kwfstdec_classes_t;

struct qtk_kwfstdec_token_list
{
	wtk_queue_node_t q_n;
	qtk_kwfstdec_token_t* token;
	int must_prune_forward_links;
	int must_prune_tokens;
};

struct token_map
{
	wtk_queue_node_t q_n;
	qtk_kwfstdec_token_t* token;
	int stateID;
};

struct st_q
{
	wtk_queue_node_t q_n;
	wtk_fst_state_t *state;
	int state_id;
};

struct ct_q
{
	wtk_queue_node_t q_n;
	int cost;
};

struct tmp_q
{
	wtk_queue_node_t q_n;
	float cost;
};

struct final_cost_q
{
	wtk_queue_node_t q_n;
	qtk_kwfstdec_token_t* token;
	int state;
	float cost;
};

struct qtk_kwfstdec_class
{
	wtk_queue_node_t q_n;
	wtk_string_t *class_n;
	wtk_array_t *word;
};

struct qtk_kwfstdec
{
	qtk_kwfstdec_cfg_t* cfg;
	qtk_hash_list_t* tok;

	//for control mem in decoding. by dmd
	wtk_vpool2_t *tok_pool;
	wtk_vpool2_t *pth_pool;

	wtk_queue_t active_tok; //queue for token list queue
	wtk_queue_t state_q;
	wtk_queue_t tmp_q;
	wtk_queue_t final_cost_q;
	//wtk_queue_t token_map_q;
	wtk_queue_t topsorted_list_q;
	wtk_queue_t hot_words;
	wtk_fst_net_t *net; //get fst from this
	wtk_fst_net_t *context_net; //for context biasing
	qtk_trans_model_t* trans_model;
	qtk_kwfstdec_token_list_t *cur_token_list;
	qtk_hash_elem_t* best_elem;
	qtk_kwfstdec_token_t *best_token;
	qtk_kwfstdec_token_t *cur_token;
	wtk_heap_t *heap;
	wtk_heap_t *heap2;
	wtk_heap_t *tmp_heap;    //using in a function life cycle.
	wtk_fst_net3_t *lat_net;
	wtk_rescore_t *lat_rescore;
	wtk_fst_net2_t *input_net;
	wtk_chnlike_t *chnlike;
	wtk_string_t **hot_sym;
	wtk_fst_node_t **lat_support;
	wtk_strbuf_t *path_cost;
	wtk_strbuf_t *path_id;
	wtk_strbuf_t *path_out_cnt;
	wtk_strbuf_t *path_out_id;
	float *cost_offset;//6000
	int offset_count;
	int cur_frame;
	int handle;
	int num_toks;
	int decoding_finalized;
	int onnx_dec;
	int idle;
	unsigned int *bins;
	float best_weight;
	float best_final_cost;
	float final_relative_cost;
	float final_best_cost;
	float conf;
	float recommand_conf;
	int vad_index;
	int valid_index;
};

int qtk_kwfstdec_set_hot_words(qtk_kwfstdec_t* dec,char *str);
qtk_kwfstdec_t* qtk_kwfstdec_new(qtk_kwfstdec_cfg_t* cfg);
qtk_kwfstdec_t* qtk_kwfstdec_new2(qtk_kwfstdec_cfg_t* cfg, int use_outnet);
int qtk_kwfstdec_start(qtk_kwfstdec_t* dec);
int qtk_kwfstdec_feed(qtk_kwfstdec_t* dec,wtk_feat_t *f);
int qtk_kwfstdec_feed2(qtk_kwfstdec_t* dec,wtk_vector_t *v,int index);
void qtk_kwfstdec_get_hint_result(qtk_kwfstdec_t* dec,wtk_strbuf_t *buf);
void qtk_kwfstdec_get_filter_result(qtk_kwfstdec_t* dec, wtk_strbuf_t *buf,float conf);
void qtk_kwfstdec_get_result(qtk_kwfstdec_t* dec,wtk_strbuf_t *buf);
void qtk_kwfstdec_get_fa(qtk_kwfstdec_t *dec,wtk_strbuf_t *buf);
void qtk_kwfstdec_get_phn_info(qtk_kwfstdec_t *dec,wtk_strbuf_t *buf);
void qtk_kwfstdec_reset(qtk_kwfstdec_t* dec);
void qtk_kwfstdec_delete(qtk_kwfstdec_t* dec);
int qtk_kwfstdec_bytes(qtk_kwfstdec_t* dec);

//using must keep consistent
qtk_kwfstdec_t* qtk_kwfstdec_new3(qtk_kwfstdec_cfg_t* cfg, int use_outnet);
int qtk_kwfstdec_start3(qtk_kwfstdec_t* dec);
int qtk_kwfstdec_feed3(qtk_kwfstdec_t* dec, wtk_vector_t *v, int index);
void qtk_kwfstdec_reset3(qtk_kwfstdec_t* dec);
void qtk_kwfstdec_delete3(qtk_kwfstdec_t* dec);
void qtk_kwfstdec_get_result3(qtk_kwfstdec_t* dec,wtk_strbuf_t *buf);

float qtk_kwfstdec_set_vadindex(qtk_kwfstdec_t* dec,int index);
#ifdef __cplusplus
}
;
#endif
#endif

