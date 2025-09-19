#ifndef QTK_WAKEUP_HASH_LIST_H_
#define QTK_WAKEUP_HASH_LIST_H_
#include "wtk/core/wtk_heap.h"
#include "wtk/asr/wfst/net/wtk_fst_net.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_kwdec_token qtk_kwdec_token_t;
typedef struct qtk_kwdec_pth qtk_kwdec_pth_t;
typedef struct qtk_kwdec_link qtk_kwdec_link_t;
typedef struct qtk_kwdec_hash_elem qtk_kwdec_hash_elem_t;
typedef struct qtk_kwdec_hash_bucket qtk_kwdec_hash_bucket_t;
typedef struct qtk_kwdec_hash_list qtk_kwdec_hash_list_t;

struct qtk_kwdec_token
{
	float tot_cost;
	float extra_cost;
	float ac_cost;
	qtk_kwdec_link_t *link;//forward link for lattice
	qtk_kwdec_pth_t *pth;//backward path for one best path
	qtk_kwdec_token_t *next;
	void* hook;
	int state; //support for lattice generate
};

struct qtk_kwdec_pth
{
	int out_label;
	int in_label;
	int frame;
	float ac_cost;
	qtk_kwdec_token_t *lbest;
};

struct qtk_kwdec_link
{
	qtk_kwdec_token_t *next_tok;
	int in_label;
	int out_label;
	float graph_cost;
	float acoustic_cost;
	qtk_kwdec_link_t *next;
};

struct qtk_kwdec_hash_elem
{
	int key;
	wtk_fst_state_t *state;
	qtk_kwdec_token_t *token;
	qtk_kwdec_hash_elem_t *tail;
};

struct qtk_kwdec_hash_bucket
{
	//wtk_queue_node_t q_n;
	int prev_bucket;
	qtk_kwdec_hash_elem_t  *last_elem;
};

struct qtk_kwdec_hash_list
{
	qtk_kwdec_hash_elem_t *list_head;
	qtk_kwdec_hash_elem_t *freed_head;
	int bucket_list_tail;
	int size;
	qtk_kwdec_hash_bucket_t **bucket_q;//bucket q
	wtk_heap_t *heap;
};

qtk_kwdec_hash_list_t* qtk_kwdec_hash_list_new(int size);
qtk_kwdec_token_t* qtk_kwdec_hash_list_new_token(qtk_kwdec_hash_list_t *hash,int extra_cost);
qtk_kwdec_pth_t* qtk_kwdec_hash_list_new_pth(qtk_kwdec_hash_list_t* hash,qtk_kwdec_token_t *token);
void qtk_kwdec_hash_list_reset(qtk_kwdec_hash_list_t* hash);
void qtk_kwdec_hash_list_delete(qtk_kwdec_hash_list_t* hash);
int qtk_kwdec_hash_list_Insert(qtk_kwdec_hash_list_t* hash,int state_id,wtk_fst_state_t* state,qtk_kwdec_token_t* token);
qtk_kwdec_hash_elem_t*  qtk_kwdec_hash_list_find(qtk_kwdec_hash_list_t* hash,int state);
qtk_kwdec_hash_elem_t* qtk_kwdec_hash_list_clear(qtk_kwdec_hash_list_t* hash);
qtk_kwdec_hash_elem_t*  qtk_kwdec_hash_list_new_elem(qtk_kwdec_hash_list_t* hash);
void qtk_kwdec_hash_list_del_elem(qtk_kwdec_hash_list_t* hash,qtk_kwdec_hash_elem_t* elem);
qtk_kwdec_link_t* qtk_kwdec_hash_list_link_new(qtk_kwdec_hash_list_t *hash,qtk_kwdec_token_t* token,int in_label,int out_label,float graph_cost,float acoustic_cost,qtk_kwdec_link_t* next);
void qtk_kwdec_hash_list_link_delete(qtk_kwdec_token_t* token);
void qtk_kwdec_hash_list_del(qtk_kwdec_hash_list_t* hash, qtk_kwdec_hash_elem_t* elem);
//TODO set size,get head

#ifdef __cplusplus
};
#endif
#endif

