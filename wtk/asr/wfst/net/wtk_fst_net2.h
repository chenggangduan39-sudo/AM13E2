#ifndef WTK_FST_NET_WTK_FST_NET2_H_
#define WTK_FST_NET_WTK_FST_NET2_H_
#include "wtk/core/wtk_type.h"
#include "wtk/core/cfg/wtk_source.h"
#include "wtk/core/wtk_str_hash.h"
#include "wtk_fst_net.h"
#include "wtk/asr/net/wtk_lat.h"
#include "wtk/core/wtk_rbtree.h"
#include "wtk/core/wtk_array.h"
#include "wtk/core/wtk_larray.h"
#include "wtk/asr/wfst/rec/rescore/lm/nglm/wtk_nglm.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/asr/wfst/egram/xbnf/qtk_xbnf_post_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_fst_net2 wtk_fst_net2_t;

//----------------- nbest section ------------------------
typedef struct
{
	wtk_queue_node_t q_n;
	wtk_queue_t prev_q;
	wtk_queue_t next_q;
	wtk_fst_state_t *state;
	float score;
	int n;
}wtk_fst_nbest_node_t;

typedef struct
{
	wtk_queue_node_t prev_n;
	wtk_queue_node_t next_n;
	wtk_fst_trans_t *trans;
	wtk_fst_nbest_node_t *from;
	wtk_fst_nbest_node_t *to;
}wtk_fst_nbest_arc_t;

typedef struct wtk_fst_nbest_ent wtk_fst_nbest_ent_t;

struct wtk_fst_nbest_ent
{
	wtk_rbnode_t rb_n;
	wtk_fst_nbest_ent_t *next;	    //next for entry list.
	wtk_fst_nbest_ent_t *prev;		//prev entry in list.
	wtk_fst_nbest_ent_t *path_prev;		//next entry in one best path.
	wtk_fst_nbest_node_t *node;
	wtk_fst_nbest_arc_t *arc;
	float score;
	float like;
	void *hook2; //mark for net;
};

typedef struct
{
	wtk_queue_node_t app_qn;
	wtk_string_t *str;
	wtk_string_t *post;		//除掉空格之后的字符串;
	float score;
	float conf;				//conf of current result;	[0,1], if conf euqal -1,conf is not valid;
}wtk_fst_rec_item_t;

typedef struct
{
	wtk_fst_rec_item_t **recs;
	int n;
}wtk_fst_rec_trans_t;


struct wtk_fst_net2
{
	wtk_fst_net_cfg_t *cfg;
	wtk_heap_t *heap;
	//wtk_fst_state_t *state;
	wtk_fst_state2_t *start;
	wtk_fst_state2_t *end;
	wtk_strbuf_t *buf;
	unsigned int state_id;
	unsigned int trans_id;
	wtk_fst_nbest_ent_t **nbest;
	int nbest_cnt;
	//wtk_larray_t *s_array;
	wtk_larray_t *e_array;
	wtk_larray_t *tmp1_array;
	wtk_fst_net_print_t *print;
	//wtk_larray_t *tmp2_array;
};


wtk_fst_net2_t* wtk_fst_net2_new(wtk_fst_net_cfg_t *cfg);
void wtk_fst_net2_delete(wtk_fst_net2_t *n);
void wtk_fst_net2_reset(wtk_fst_net2_t *n);
wtk_fst_state2_t* wtk_fst_net2_pop_state(wtk_fst_net2_t *net);
wtk_fst_trans2_t* wtk_fst_net2_pop_trans(wtk_fst_net2_t *net);
wtk_fst_trans2_t* wtk_fst_net2_link_state(wtk_fst_net2_t *net,
		wtk_fst_state2_t *s,wtk_fst_state2_t *e,
		unsigned int frame, unsigned int in_id,unsigned int out_id,
		float lm_like,float weight);
wtk_fst_trans2_t* wtk_fst_net2_link_state3(wtk_fst_net2_t *net,
		wtk_fst_state2_t *s,wtk_fst_state2_t *e,
		unsigned int frame, unsigned int in_id,unsigned int out_id,
		float lm_like,float weight);

//-------------------- shortest path ----------------------
typedef struct
{
	wtk_rbnode_t rb_n;
	wtk_fst_state_t *state;
	wtk_fst_trans_t *min_input_trans;
	wtk_fst_state_t *from_state;
	float score;
	float r;
	unsigned char touch:1;
}wtk_fst_net2_pthem_t;

void wtk_fst_net2_attach_short_path_hook(wtk_fst_net2_t *net);

/*
 * 环的处理有问题，目前的识别结果中没有环的问题。
 */
int wtk_fst_net2_shortest_path(wtk_fst_net2_t *net);
void wtk_fst_net2_print_best_path(wtk_fst_net2_t *net);
void wtk_fst_net2_get_short_one_best_path(wtk_fst_net2_t *net,wtk_strbuf_t *buf,char *sep,int bytes);
void wtk_fst_net2_get_short_one_best_path3(wtk_fst_net2_t *net,wtk_strbuf_t *buf,char *sep,int bytes,wtk_string_t **hot_sym);

//----------------------- nbest ----------------------
int wtk_fst_net2_nbest_path(wtk_fst_net2_t *net,int k,int max_search);
void wtk_fst_net2_get_nbest_one_path(wtk_fst_net2_t *net,wtk_strbuf_t *buf,char *sep,int bytes);
wtk_fst_rec_trans_t* wtk_fst_net2_get_nbest(wtk_fst_net2_t *net,wtk_heap_t *heap,char *sep,int bytes);
void wtk_fst_rec_trans_print(wtk_fst_rec_trans_t *trans);
wtk_fst_rec_item_t* wtk_fst_rec_trans_find(wtk_fst_rec_trans_t *trans,char *data,int bytes);

//---------------------- network unique -------------
int wtk_fst_net2_unique(wtk_fst_net2_t *net);
//--------------load file --------------------------------
int wtk_fst_net2_bytes(wtk_fst_net2_t *net);
void wtk_fst_net2_print(wtk_fst_net2_t *net);
void wtk_fst_net2_reverse_trans(wtk_fst_net2_t *net);
wtk_str_hash_t *wtk_fst_net2_load_sym(char *fn,int nslot);
int wtk_fst_net2_load_lat2(wtk_fst_net2_t *net,char *lat_fn,wtk_str_hash_t *sym,float lmscale);
int wtk_fst_net2_from_lat(wtk_fst_net2_t *net,wtk_lat_t *lat,wtk_str_hash_t *sym);
int wtk_fst_net2_from_lat2(wtk_fst_net2_t *net,wtk_lat_t *lat);
//---------------------- link two net ------------
int wtk_fst_net2_link(wtk_fst_net2_t *net1,wtk_fst_net2_t *net2,int is_end);
//----------------------- print net2 ------------
void wtk_fst_net2_print_fsm(wtk_fst_net2_t *net,wtk_strbuf_t *buf);
void wtk_fst_net2_print_fsm2(wtk_fst_net2_t *net);
void wtk_fst_net2_print_fsm3(wtk_fst_net2_t *net);
//----------------------- print lattice ------------------------
int wtk_fst_net2_print_lat(wtk_fst_net2_t *net,FILE *log);
int wtk_fst_net2_write_lat(wtk_fst_net2_t *net,char *fn);
int wtk_fst_net2_nbest_to_net(wtk_fst_net2_t *net,wtk_fst_net2_t *dst);
int wtk_fst_net2_nbest_mark_wrd(wtk_fst_net2_t *net,int index);

//-------------- result rec string  ---------------------
wtk_fst_rec_item_t* wtk_fst_rec_item_new(wtk_heap_t *heap,char *data,int len);
wtk_string_t* wtk_fst_rec_item_get_str(wtk_fst_rec_item_t *item);
void wtk_fst_net3_print_nbest(wtk_fst_net2_t *net);
wtk_fst_rec_item_t* wtk_fst_rec_item_new(wtk_heap_t *heap,char *data,int len);

//-------------- resore use nglm  ----------------------
void wtk_fst_net2_print_shortest_path(wtk_fst_net2_t *net);

int wtk_fst_net2_rescore(wtk_fst_net2_t *net,wtk_nglm_t *lm);

int wtk_fst_net2_determinize(wtk_fst_net2_t *net,wtk_fst_net2_t *output);

void wtk_fst_net2_remove_nil_node(wtk_fst_net2_t *net);
int wtk_fst_net2_states(wtk_fst_net2_t *net);

/**
 * @brief clean state->hook and trans->hook2;
 */
void wtk_fst_net2_clean_hook(wtk_fst_net2_t *net);

/**
 * @brief clean state->hook;
 */
void wtk_fst_net2_clean_hook2(wtk_fst_net2_t *net);



void wtk_fst_net2_link_state2(wtk_fst_net2_t *net,
		wtk_fst_state2_t *s,wtk_fst_state2_t *e,wtk_fst_trans2_t *trans2,
		unsigned int frame,unsigned int in_id,unsigned int out_id,
		float lm_like,float weight);

int wtk_fst_net2_get_history(wtk_fst_net2_t *net,int *ids,int max);

/*
 * add path for diff function. such as: leak, review, reread etc on.
 */
int wtk_fst_net2_addleakpath(wtk_fst_net2_t *net, qtk_xbnf_post_cfg_t *xbnf_post);
int wtk_fst_net2_addselfloop(wtk_fst_net2_t *net, qtk_xbnf_post_cfg_t *xbnf_post);
int wtk_fst_net2_addreview(wtk_fst_net2_t *net, qtk_xbnf_post_cfg_t *xbnf_post);
/**
 * for support kinds of path dealing, need merge addleakpath/addselfloop/addreview
 */
int wtk_fst_net2_addpath(wtk_fst_net2_t *net, qtk_xbnf_post_cfg_t *xbnf_post, int use_leak, int use_addre, int use_selfloop);
int wtk_fst_net2_addwrdloop(wtk_fst_net2_t *net, wtk_fst_state2_t *es, wtk_fst_state2_t *ee);
#ifdef __cplusplus
};
#endif
#endif
