#ifndef WTK_FST_NET_WTK_FST_NET3_H_
#define WTK_FST_NET_WTK_FST_NET3_H_
#include "wtk/core/wtk_type.h"
#include "wtk/core/wtk_vpool.h"
#include "wtk/asr/wfst/net/wtk_fst_net.h"
#include "wtk_fst_net3_cfg.h"
#include "wtk/core/wtk_slist.h"
#include "wtk/core/wtk_queue2.h"
#include "wtk/core/wtk_sort.h"
#include "wtk/asr/wfst/net/wtk_fst_net2.h"
#ifdef __cplusplus
extern "C" {
#endif
struct wtk_wfst_path;
typedef struct wtk_fst_net3 wtk_fst_net3_t;
typedef struct wtk_fst_node wtk_fst_node_t;
typedef struct wtk_fst_arc wtk_fst_arc_t;
typedef struct wtk_fst_net3_combine wtk_fst_net3_combine_t;
#define wtk_fst_arc_in_id(arc) arc->trans?arc->trans->in_id:0

typedef struct
{
	//wtk_slist_t list_q;
	wtk_queue2_t list_q;
}wtk_fst_lat_inst_t;

struct wtk_fst_node
{
	wtk_queue_node_t q_n;		//active node queue
	wtk_queue_node_t inst_n;	//inst queue;
	wtk_queue2_t next_q;
	wtk_queue2_t prev_q;
	wtk_fst_arc_t **pre;
#ifdef USE_LAT_FST_PATH
	wtk_queue2_t path_q;
#endif
	wtk_fst_state_t *state;
	wtk_fst_lat_inst_t *inst;
	float ac_like;
	float lm_like;
	float arc_like;
	union
	{
		float f;
		int i;
		void *p;
	}hook;		//temp function can use
	unsigned int frame;
	unsigned int num;
	//unsigned char trans_state_index;		//index of state,[1,5]
	unsigned char collect:1;
	unsigned char eof:1;
	unsigned char touch:1;
	unsigned char leaf:1;
};

typedef struct
{
	wtk_slist_node_t list_n;
	wtk_fst_node_t *node;
}wtk_fst_node_item_t;

typedef struct
{
	wtk_fst_net_cfg_t *net_cfg;
	wtk_fst_arc_t *arc;
	unsigned int index;
}wtk_fst_arc_item_t;

struct wtk_fst_arc
{
	wtk_queue_node_t prev_n;
	wtk_queue_node_t next_n;
	wtk_fst_node_t *from_node;
	wtk_fst_node_t *to_node;
	void *hook;
	void *hook2;
	int hot;
#ifndef USE_LAT_FST_PATH
	struct wtk_wfst_path *pth;
#endif
	wtk_fst_trans_t *trans;
	wtk_fst_label_id_t in_id;
	wtk_fst_label_id_t out_id;
	//float like;
	float arc_like;
	float ac_like;
	float lm_like;
	unsigned int frame;
};

struct wtk_fst_net3_combine
{
	wtk_fst_node_t *start;
	wtk_fst_node_t *end;
	wtk_string_t *key;
	float ac_like;
	float lm_like;
};

struct wtk_fst_net3
{
	wtk_fst_net3_cfg_t *cfg;
	wtk_fst_net_cfg_t *net_cfg;
	wtk_prune_t *hg;
	wtk_vpool_t *node_pool;
	wtk_vpool_t *arc_pool;
	wtk_vpool_t *lat_inst_pool;	//wtk_fst_lat_inst_t

	wtk_fst_node_t *start;
	wtk_fst_node_t *end;
	wtk_fst_state_t *end_state;
	wtk_fst_node_t *null_prev_node;
	wtk_fst_node_t *null_node;
	double max_like;
	double end_ac_like;
	double end_lm_like;
	wtk_slist_t list_q;
	wtk_queue_t active_node_q;
	wtk_heap_t *heap;

	wtk_fst_lat_inst_t *output_inst_q;
	wtk_str_hash_t* outsym_hash;

	int cnt;
	unsigned int last_frame;
	unsigned eof:1;
};

wtk_fst_net3_t* wtk_fst_net3_new(wtk_fst_net3_cfg_t *cfg,wtk_fst_net_cfg_t *net_cfg);
void wtk_fst_net3_delete(wtk_fst_net3_t *n);
void wtk_fst_net3_reset(wtk_fst_net3_t *n);
int wtk_fst_net3_bytes(wtk_fst_net3_t *n);
void wtk_fst_net3_push_node(wtk_fst_net3_t *net,wtk_fst_node_t *node);
void wtk_fst_net3_push_arc(wtk_fst_net3_t *net,wtk_fst_arc_t *arc);
void wtk_fst_net3_reset_end(wtk_fst_net3_t *net);
wtk_fst_node_t* wtk_fst_net3_pop_node(wtk_fst_net3_t *n,unsigned int frame,
		wtk_fst_trans_t *trans,wtk_fst_state_t *state);
wtk_fst_node_t* wtk_fst_net3_pop_node2(wtk_fst_net3_t *n,unsigned int frame,
		wtk_fst_trans_t *trans,wtk_fst_state_t *state,int num);

void wtk_fst_net3_push_node(wtk_fst_net3_t *net,wtk_fst_node_t *n);
wtk_fst_arc_t*	wtk_fst_net3_pop_arc(wtk_fst_net3_t *n,
		wtk_fst_node_t *s,wtk_fst_node_t *e,
		wtk_fst_trans_t *trans,
		float arc_like,float ac_like,float lm_like,unsigned int frame);
wtk_fst_arc_t*	wtk_fst_net3_pop_arc2(wtk_fst_net3_t *n,
		wtk_fst_node_t *s,wtk_fst_node_t *e,int in,
		int out,
		float arc_like,float ac_like,float lm_like,
		unsigned int frame,int is_hot);
void wtk_fst_net3_update_prune_eof(wtk_fst_net3_t *net);
void wtk_fst_net3_update_eof(wtk_fst_net3_t *net);

void wtk_fst_net3_prune_node(wtk_fst_net3_t *net,wtk_fst_node_t *root,float beam,int hg_nodes);
void wtk_fst_net3_collect_node(wtk_fst_net3_t *net,wtk_fst_node_t *fn);

void wtk_fst_net3_remove_eps(wtk_fst_net3_t *net);

void wtk_fst_net3_finish(wtk_fst_net3_t *net);

int wtk_fst_net3_is_arc_in(wtk_fst_net3_t *net,wtk_fst_node_t *s,
		wtk_fst_node_t *to_node,
		unsigned int input_id,unsigned int output_id,
		float arc_like,unsigned int frame);

wtk_fst_arc_t* wtk_fst_net3_add_eps_link_arc(wtk_fst_net3_t *net,wtk_fst_node_t *root,wtk_fst_arc_t *arc1,
		float arc_like,float ac_like,float lm_like);

wtk_fst_arc_t* wtk_fst_net3_add_link_arc(wtk_fst_net3_t *net,wtk_fst_node_t *root,wtk_fst_node_t *to,
		unsigned int out_id,float like,float arc_like,float ac_like,float lm_like,unsigned int frame);


wtk_fst_node_t* wtk_fst_net3_get_trans_node(wtk_fst_net3_t *net,wtk_fst_trans_t *trans,unsigned int frame);
wtk_fst_lat_inst_t* wtk_fst_net3_pop_lat_inst(wtk_fst_net3_t *n);
void wtk_fst_net3_push_lat_inst(wtk_fst_net3_t *n,wtk_fst_lat_inst_t *inst);
wtk_fst_lat_inst_t* wtk_fst_net3_get_trans_inst(wtk_fst_net3_t *net,wtk_fst_trans_t *trans);

int wtk_fst_net3_to_net2(wtk_fst_net3_t *net,wtk_fst_net2_t *net2);
//void wtk_fst_net3_add_extra_pth(wtk_fst_net3_t *net,wtk_queue_t *class);
void wtk_fst_net3_get_sym_combination(wtk_fst_net3_t *net);
//---------------------------- lattice section -------------------------------------
void wtk_fst_net3_print_lat(wtk_fst_net3_t *net,FILE *log);
void wtk_fst_net3_write_lat(wtk_fst_net3_t *net,char *fn);
void wtk_fst_net3_print_lat2(wtk_fst_net3_t *net,FILE *log);
void wtk_fst_net3_write_lat2(wtk_fst_net3_t *net,char *fn);

//----------------------------- print section ---------------------------------------
void wtk_fst_net3_print(wtk_fst_net3_t *net);
void wtk_fst_net3_print_prev(wtk_fst_net3_t *net,wtk_fst_node_t *node);
void wtk_fst_net3_print_next(wtk_fst_net3_t *net,wtk_fst_node_t *node);
void wtk_fst_net3_check_node(wtk_fst_net3_t *net,wtk_fst_node_t *node);
void wtk_fst_net3_remove_node(wtk_fst_net3_t *net,wtk_fst_node_t *n);
void wtk_fst_net_remove_node_from_prev(wtk_fst_net3_t *net,wtk_fst_node_t *node);
void wtk_fst_net3_remove_arc_from_dst_node(wtk_fst_net3_t *net,wtk_fst_node_t *n,wtk_fst_arc_t *arc);
void wtk_fst_net3_remove_arc_from_src_node(wtk_fst_net3_t *net,wtk_fst_node_t *n,wtk_fst_arc_t *arc);
void wtk_fst_net3_unlink_arc(wtk_fst_net3_t *net,wtk_fst_arc_t *arc);
void wtk_fst_net3_add_arc(wtk_fst_net3_t *net,wtk_fst_arc_t *arc);
void wtk_fst_net3_remove_nil_node(wtk_fst_net3_t *net,wtk_fst_node_t *n);
int wtk_fst_node_nxt_arcs(wtk_fst_node_t *n);
int wtk_fst_node_pre_arcs(wtk_fst_node_t *n);
double wtk_fst_net3_max_path_aclike(wtk_fst_net3_t *net,wtk_fst_arc_t *arc);
double wtk_fst_net3_max_path_aclike2(wtk_fst_net3_t *net,wtk_fst_node_t *node);
double wtk_fst_net3_max_path_aclike3(wtk_fst_net3_t *net,wtk_fst_node_t *node);
double wtk_fst_net3_max_path_lmlike(wtk_fst_net3_t *net,wtk_fst_arc_t *arc);
double wtk_fst_net3_max_path_lmlike2(wtk_fst_net3_t *net,wtk_fst_node_t *node);
int wtk_fst_net_is_node_eq(wtk_fst_net3_t *net,wtk_fst_node_t *node1,wtk_fst_node_t *node2);
#define wtk_fst_net3_print_arc(net,arc) wtk_fst_net3_print_arc_x(net,arc,__FUNCTION__,__LINE__)
void wtk_fst_net3_print_arc_x(wtk_fst_net3_t *net,wtk_fst_arc_t *arc,const char *f,int l);
void wtk_fst_net3_print_prev_arc(wtk_fst_net3_t *net,wtk_fst_node_t *fn,int depth);
void wtk_fst_net3_print_next_arc(wtk_fst_net3_t *net,wtk_fst_node_t *fn,int depth);
void wtk_fst_net3_check_arcx(wtk_fst_net3_t *net);
int wtk_fst_node_prev_has_output(wtk_fst_node_t *fn,unsigned int out_id,unsigned int frame);
int wtk_fst_node_prev_has_output2(wtk_fst_node_t *fn,unsigned int out_id);
int wtk_fst_node_nxt_has_output(wtk_fst_node_t *fn,unsigned int out_id,unsigned int frame);
int wtk_fst_node_nxt_has_output2(wtk_fst_node_t *fn,unsigned int out_id);
int wtk_fst_net3_reach_end(wtk_fst_net3_t *net,wtk_fst_node_t *fn);
int wtk_fst_net3_reach_start(wtk_fst_net3_t *net,wtk_fst_node_t *fn);
void wtk_fst_net3_print_lat_inst(wtk_fst_net3_t *net,wtk_fst_lat_inst_t *inst);

void wtk_fst_net3_check_node(wtk_fst_net3_t *net,wtk_fst_node_t *n);
void wtk_fst_net3_check_active_node(wtk_fst_net3_t *net);
void wtk_fst_net3_is_start_active(wtk_fst_net3_t *net);

int wtk_fst_net3_compose(wtk_fst_net3_t *net,wtk_fst_net2_t *net2);
void wtk_fst_net3_clean_hook(wtk_fst_net3_t *net);
void wtk_fst_net3_check_arc(wtk_fst_net3_t *net);
void wtk_fst_net3_check_node(wtk_fst_net3_t *net,wtk_fst_node_t *n);
void wtk_fst_net3_check_collect(wtk_fst_net3_t *net,int c);
void wtk_fst_net3_prune_path(wtk_fst_net3_t *net);
void wtk_fst_arc_link_path(wtk_fst_arc_t *arc,struct wtk_wfst_path *pth);
wtk_fst_arc_t* wtk_fst_net3_found_arc(wtk_fst_net3_t *net,wtk_fst_node_t *s,
		wtk_fst_node_t *to_node,
		unsigned int input_id,unsigned int output_id,
		float arc_like,unsigned int frame);
void wtk_fst_net3_remove_virutal_node(wtk_fst_net3_t *net);
void wtk_fst_net3_remove_path(wtk_fst_net3_t *net,wtk_fst_node_t *n);
void wtk_fst_net3_remove_unlink_path(wtk_fst_net3_t *net);
#define USE_NET_MAC
#ifdef USE_NET_MAC
#define wtk_fst_node_add_nxt_arc(n,arc) wtk_queue2_push(&((n)->next_q),&((arc)->next_n))
#define wtk_fst_node_set_eof(n,eofv) (n)->eof=eofv
#else
void wtk_fst_node_set_eof(wtk_fst_node_t *n,int eof);
void wtk_fst_node_add_nxt_arc(wtk_fst_node_t *n,wtk_fst_arc_t *arc);
#endif

//wtk_fst_node_t* wtk_fst_path_get_lat_node(wtk_fst_path_t *pth)

#ifdef USE_LAT_FST_PATH
#define wtk_fst_path_get_lat_node(pth) (wtk_fst_node_t*)((pth)->hook)
#else
#define wtk_fst_path_get_lat_node(pth)	(((pth)->hook)?((wtk_fst_arc_t*)(pth->hook))->to_node:NULL)
#endif

#ifdef __cplusplus
};
#endif
#endif
