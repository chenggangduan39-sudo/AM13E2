#ifndef qtk_kwfstdec_lite_H_
#define qtk_kwfstdec_lite_H_
#include <math.h>
#include <limits.h>
#include <float.h>
#include "qtk_kwfstdec_cfg.h"
#include "wtk/asr/wfst/net/wtk_fst_net.h"
//#include "wtk/asr/wfst/basetype/qtk_hash_list.h"
#include "wtk/asr/wfst/net/wtk_fst_net.h"
#include "wtk/asr/wfst/kaldifst/qtk_trans_model_cfg.h"
#include "wtk/core/math/wtk_vector.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/asr/fextra/wtk_feat.h"
#include "wtk/asr/wfst/net/wtk_fst_net3.h"
#include "wtk/core/wtk_vpool2.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_kwfstdec_lite_token qtk_kwfstdec_lite_token_t;
typedef struct qtk_kwfstdec_lite_pth qtk_kwfstdec_lite_pth_t;
typedef struct qtk_kwfstdec_lite qtk_kwfstdec_lite_t;
typedef struct st_q_lite st_q_lite_t;
//typedef struct final_cost_q_lite final_cost_q_lite_t;

struct st_q_lite
{
	wtk_queue_node_t q_n;
	wtk_fst_state_t *state;
	//int state_id;
};

struct qtk_kwfstdec_lite_token
{
	wtk_queue_node_t q_n;
	float tot_cost;
	//qtk_kwfstdec_lite_link_t *link;//forward link for lattice
	qtk_kwfstdec_lite_pth_t *pth;//backward path for one best path
	wtk_fst_state_t *state;
};

struct qtk_kwfstdec_lite_pth
{
	int out_label;
	int in_label;
	float like;
	qtk_kwfstdec_lite_pth_t *lbest;
};


struct qtk_kwfstdec_lite
{
	qtk_kwfstdec_cfg_t* cfg;
	//wtk_queue_t state_q;
	wtk_queue_t *cur_tokq;
	wtk_queue_t *pre_tokq;
	wtk_fst_net_t *net; //get fst from this
	qtk_trans_model_t* trans_model;
	qtk_kwfstdec_lite_token_t *best_token;
	wtk_heap_t *heap;
	wtk_vpool2_t *tok_pool;
	int cur_frame;
	int decoding_finalized;
	unsigned int *bins;
	float best_weight;
	//float best_final_cost;
	//float final_relative_cost;
	//float final_best_cost;
	float conf;
};

qtk_kwfstdec_lite_t* qtk_kwfstdec_lite_new(qtk_kwfstdec_cfg_t* cfg);
qtk_kwfstdec_lite_t* qtk_kwfstdec_lite_new2(qtk_kwfstdec_cfg_t* cfg, int use_outnet);
int qtk_kwfstdec_lite_start(qtk_kwfstdec_lite_t* dec);
int qtk_kwfstdec_lite_feed(qtk_kwfstdec_lite_t* dec,wtk_feat_t *f);
int qtk_kwfstdec_lite_feed2(qtk_kwfstdec_lite_t* dec,wtk_vector_t *v,int index);
void qtk_kwfstdec_lite_get_result(qtk_kwfstdec_lite_t* dec,wtk_strbuf_t *buf);
void qtk_kwfstdec_lite_reset(qtk_kwfstdec_lite_t* dec);
void qtk_kwfstdec_lite_delete(qtk_kwfstdec_lite_t* dec);

#ifdef __cplusplus
}
;
#endif
#endif

