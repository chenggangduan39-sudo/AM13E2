#ifndef WTK_FST_NET_FST_NET_H_
#define WTK_FST_NET_FST_NET_H_
#include "wtk/core/cfg/wtk_source.h"
#include "wtk/core/wtk_bit_heap.h"
#include "wtk/core/wtk_harray.h"
#include "wtk/core/wtk_shash.h"
#include "wtk/core/wtk_type.h"
#include "wtk/core/wtk_vpool.h"
#include "wtk_fst_net_cfg.h"
#include <limits.h>
#ifdef FLT_MIN
#else
#define FLT_MIN __FLT_MIN__
#endif

#ifdef FLT_MAX
#else
#define FLT_MAX __FLT_MAX__
#endif

#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_fst_net wtk_fst_net_t;

typedef unsigned int wtk_fst_trans_id_t;
typedef unsigned int wtk_fst_state_id_t;
typedef unsigned int wtk_fst_label_id_t;
typedef float wtk_fst_float_t;
//typedef double wtk_fst_float_t;
typedef struct wtk_fst_state wtk_fst_state_t;
typedef struct wtk_fst_final_state wtk_fst_final_state_t;
typedef struct wtk_fst_trans  wtk_fst_trans_t;
typedef struct wtk_fst_trans2  wtk_fst_trans2_t;
typedef struct wtk_fst_state2 wtk_fst_state2_t;

typedef wtk_string_t* (*wtk_fst_net_get_sym_f)(void *ths,wtk_fst_label_id_t id);

typedef struct
{
	void *ths;
	wtk_fst_net_get_sym_f get_insym;
	wtk_fst_net_get_sym_f get_outsym;
}wtk_fst_net_print_t;


struct wtk_fst_trans
{
	wtk_fst_state_t* to_state;
	union {
		void *inst;					//wtk_fst_inst_t* for rec
		wtk_fst_trans_t* next;		//output next arc link for net2;
	}hook;
	wtk_fst_float_t weight;
	wtk_fst_label_id_t in_id;
	wtk_fst_label_id_t out_id;
	int hot;
};

struct wtk_fst_trans2
{
	wtk_fst_state_t* to_state;
	union {
		void *inst;					//wtk_fst_inst_t* for rec
		wtk_fst_trans_t* next;		//output next arc link for net2;
	}hook;
	wtk_fst_float_t weight;
	wtk_fst_label_id_t in_id;
	wtk_fst_label_id_t out_id;
	int hot;
	wtk_fst_float_t lm_like;
	int frame;
	wtk_fst_state_t* from_state;
	void *hook2;
	wtk_fst_trans2_t *in_prev;
};

typedef enum
{
	WTK_FST_TOUCH_STATE,
	WTK_FST_NORM_STATE,
	WTK_FST_FINAL_STATE,
	WTK_FST_NORMAL_ARRAY_STATE,
}wtk_fst_state_type_t;

#define USE_FST_LIST_NODE
//#define ATTACH_NWLP

struct wtk_fst_state
{
#ifdef USE_FST_LIST_NODE
	wtk_slist_node_t q_n;
#else
	wtk_fst_state_t *next;
#endif
	void *hook;
	union
	{
		wtk_fst_trans_t *trans;		//WTK_FST_NORM_STATE, output arc link;
		wtk_fst_float_t weight;		//WTK_FST_FINAL_STATE
	}v;
	wtk_fst_state_id_t id;		//WTK_FST_TOUCH_STATE,state id;
#ifdef ATTACH_NWLP
	unsigned int ntrans;
	unsigned int type;
	float nwlmp;		//next word lm prob;
#else
	unsigned int ntrans:27;
	unsigned int type:4;
	unsigned int custom:1;
	wtk_fst_float_t weight;
	int frame;
	int load;
#endif
};

struct wtk_fst_state2
{
#ifdef USE_FST_LIST_NODE
	wtk_slist_node_t q_n;
#else
	wtk_fst_state_t *next;
#endif
	void *hook;
	union
	{
		wtk_fst_trans_t *trans;		//WTK_FST_NORM_STATE, output arc link;
		wtk_fst_float_t weight;		//WTK_FST_FINAL_STATE
	}v;
	wtk_fst_state_id_t id;		//WTK_FST_TOUCH_STATE,state id;
#ifdef ATTACH_NWLP
	unsigned int ntrans;
	unsigned int type;
	float nwlmp;		//next word lm prob;
#else
	unsigned int ntrans:27;
	unsigned int type:4;
	unsigned int custom:1;
#endif
	wtk_fst_trans2_t *in_prev;
};


struct wtk_fst_final_state
{
	wtk_fst_state_id_t state_id;
	wtk_fst_float_t 	weight;
};

typedef enum
{
	WTK_FST_NET_IDX_HASH,
	WTK_FST_NET_IDX_ARRAY,
	WTK_FST_NET_IDX_HINT,
}wtk_fst_net_idx_type_t;

typedef struct
{
	void **p;
	int n;
	int valid;
}wtk_fst_net_hint_t;


struct wtk_fst_net
{
	wtk_fst_net_cfg_t *cfg;
	//--------- information ---
	wtk_fst_state_t *init_state;
	wtk_fst_state_t *rbin_states;
	int nrbin_states;
	wtk_fst_net_idx_type_t type;
	union
	{
		wtk_shash_t *hash;
		wtk_harray_t *array;
		wtk_fst_net_hint_t hint;
	}idx;
	wtk_heap_t *heap;
	union{
		struct wtk_fst_binet *bin;
	}v;
	wtk_fst_net_print_t *print;
	unsigned int count;
	double bytes;
	double time;
	double ftime;
	double xtime;
	unsigned use_rbin:1;
};

wtk_fst_net_t* wtk_fst_net_new(wtk_fst_net_cfg_t *cfg);
void wtk_fst_net_delete(wtk_fst_net_t *net);
void wtk_fst_net_reset(wtk_fst_net_t *net);
void wtk_fst_net_reset_heap(wtk_fst_net_t *net);
void wtk_fst_net_reset_hook(wtk_fst_net_t *net);
wtk_fst_state_t* wtk_fst_net_get_start_state(wtk_fst_net_t *net);
wtk_fst_state_t* wtk_fst_net_get_state(wtk_fst_net_t *kv,unsigned int id);
wtk_fst_state_t* wtk_fst_net_get_load_state(wtk_fst_net_t *net,unsigned int id);
void wtk_fst_net_load_state(wtk_fst_net_t *net,wtk_fst_state_t *state);

void wtk_fst_trans_init(wtk_fst_trans_t *trans);
void wtk_fst_state_check(wtk_fst_state_t *s);
void wtk_fst_state_clean_hook(wtk_fst_state_t *s);
int wtk_fst_state_reach_end(wtk_fst_state_t *s);
int wtk_fst_state_can_be_end(wtk_fst_state_t *s);
int wtk_fst_state_has_out_id(wtk_fst_state_t *s,unsigned int id);
//----------------------------print -------------------------
int wtk_fst_net_bytes(wtk_fst_net_t *v);
int wtk_fst_net_print(wtk_fst_net_t *net);
int wtk_fst_net_print2(wtk_fst_net_t *net);
void wtk_fst_net_print_trans(wtk_fst_net_t *net,wtk_fst_trans_t *trans);
void wtk_fst_net_print_state(wtk_fst_net_t *net,wtk_fst_state_t *state);
void wtk_fst_trans2_print_prev(wtk_fst_trans2_t *trans);
void wtk_fst_net_cfg_print_prev(wtk_fst_net_cfg_t *cfg,wtk_fst_trans2_t *trans);
void wtk_fst_net_cfg_print_next(wtk_fst_net_cfg_t *cfg,wtk_fst_trans_t *trans);
void wtk_fst_net_cfg_print_trans(wtk_fst_net_cfg_t *cfg,wtk_fst_trans_t *trans);
void wtk_fst_net_cfg_print_trans2(wtk_fst_net_cfg_t *cfg,wtk_fst_trans2_t *trans);
void wtk_fst_net_cfg_print_state(wtk_fst_net_cfg_t *cfg,wtk_fst_state_t *state);
void wtk_fst_state2_print(wtk_fst_state2_t *s);
float wtk_fst_times(float f1,float f2);
float wtk_fst_plus(float f1,float f2);

//---------------------load str ---------------------------------------o
int wtk_fst_net_load_fsm(wtk_fst_net_t *net,wtk_source_t *src);
int wtk_fst_net_load_str(wtk_fst_net_t *net,char *data,int bytes);
int wtk_fst_net_load_clg(wtk_fst_net_t *net,char *fsm_fn,char *in_sym_fn,char *out_sym_fn);
int wtk_fst_net_load_clg2(wtk_fst_net_t *net,char *dn,int bytes);


typedef enum
{
	WTK_FST_RNET_NIL,
	WTK_FST_RNET_IN_NIL,
	WTK_FST_RNET_OUT_NIL,
	WTK_FST_RNET_IN_OUT,
}wtk_fst_rnet_type_t;
int wtk_fst_net_load_rbin(wtk_fst_net_t *net,wtk_source_t *src);
void wtk_fst_net_reset_rbin(wtk_fst_net_t *net);
//-------------------- dump lm look ahead prob -------------------------
void wtk_fst_net_dump_lookahead_bin(wtk_fst_net_t *net,char *fn);
int wtk_fst_state_ntrans(wtk_fst_state_t *s);
wtk_fst_trans2_t* wtk_fst_state2_find(wtk_fst_state2_t *s,int in_id,int out_id);
wtk_fst_trans2_t* wtk_fst_state2_find2(wtk_fst_state2_t *s,wtk_fst_state2_t *e,int in_id,int out_id);

wtk_fst_trans2_t* wtk_fst_state2_find_next(wtk_fst_state2_t *s,int in_id,int out_id,wtk_fst_state2_t *to);
wtk_fst_trans2_t* wtk_fst_state2_find_prev(wtk_fst_state2_t *s,int in_id,int out_id,wtk_fst_state2_t *from);
/**
 *  find next;
 */
wtk_fst_trans2_t* wtk_fst_state2_find_reverse(wtk_fst_state2_t *s,int in_id,int out_id);
/**
 * find prev;
 */
wtk_fst_trans2_t* wtk_fst_state2_find_reverse_prev(wtk_fst_state2_t *s,int in_id,int out_id);

wtk_fst_trans2_t* wtk_fst_state2_find_reverse_next(wtk_fst_state2_t *s,int in_id,int out_id);
wtk_fst_trans2_t* wtk_fst_state2_find3(wtk_fst_state2_t *s,wtk_fst_state2_t *from,int in_id,int out_id);

int wtk_fst_state2_has_eps_to(wtk_fst_state2_t *s,wtk_fst_state2_t *t);

void wtk_fst_net_print_init(wtk_fst_net_print_t *p,void *ths,wtk_fst_net_get_sym_f get_insym,
		wtk_fst_net_get_sym_f get_outsym);
wtk_string_t* wtk_fst_net_print_get_insym(wtk_fst_net_print_t *p,unsigned int id);
wtk_string_t* wtk_fst_net_print_get_outsym(wtk_fst_net_print_t *p,unsigned int id);
void wtk_fst_net_print_trans_prev(wtk_fst_net_print_t *p,wtk_fst_trans2_t *trans);
void wtk_fst_net_print_trans_next(wtk_fst_net_print_t *p,wtk_fst_trans2_t *trans);
void wtk_fst_net_print_trans2(wtk_fst_net_print_t *p,wtk_fst_trans2_t *trans);
void wtk_fst_net_print_trans_next_x(wtk_fst_net_print_t *p,wtk_fst_trans_t *trans);
void wtk_fst_net_cfg_print_trans_next_x(wtk_fst_net_cfg_t *cfg,wtk_fst_trans_t *trans);
int wtk_fst_state2_inarcs(wtk_fst_state2_t* s);
int wtk_fst_state2_outarcs(wtk_fst_state2_t *s);
int wtk_fst_state2_has_sampe_output(wtk_fst_state2_t *s1,wtk_fst_state2_t *s2);
int wtk_fst_state2_has_sampe_input(wtk_fst_state2_t *s1,wtk_fst_state2_t *s2);
int wtk_fst_state2_has_eps_to2(wtk_fst_state2_t *s,wtk_fst_state2_t *t);
void wtk_fst_state_init(wtk_fst_state_t *state,wtk_fst_state_id_t id);

void wtk_fst_net_load_all(wtk_fst_net_t *net);
#ifdef __cplusplus
};
#endif
#endif
