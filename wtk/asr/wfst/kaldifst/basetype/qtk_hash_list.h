#ifndef QTK_HASH_LIST_H_
#define QTK_HASH_LIST_H_
#include "wtk/core/wtk_heap.h"
#include "wtk/asr/wfst/net/wtk_fst_net.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_kwfstdec_token qtk_kwfstdec_token_t;
typedef struct qtk_kwfstdec_pth qtk_kwfstdec_pth_t;
typedef struct qtk_kwfstdec_link qtk_kwfstdec_link_t;
typedef struct qtk_hash_elem qtk_hash_elem_t;
typedef struct qtk_hash_bucket qtk_hash_bucket_t;
typedef struct qtk_hash_list qtk_hash_list_t;

struct qtk_kwfstdec_token
{
	float tot_cost;
	float extra_cost;
	float ac_cost;
	float context_cost;
	qtk_kwfstdec_link_t *link; // forward link for lattice
	qtk_kwfstdec_pth_t *pth;//backward path for one best path
	qtk_kwfstdec_token_t *next;
	wtk_fst_state_t *context_state;
	void *hook; // valid?
	int state; //support for lattice generate
};

struct qtk_kwfstdec_pth
{
	unsigned short in_label;
	int out_label;
	short frame;
	short used;      //number of use in decode net.
	float ac_cost;
	qtk_kwfstdec_token_t *lbest;
};

struct qtk_kwfstdec_link
{
	qtk_kwfstdec_token_t *next_tok;
	int in_label;
	int out_label;
	float graph_cost;
	float acoustic_cost;
	float context_score;
	qtk_kwfstdec_link_t *next;
};

struct qtk_hash_elem
{
	int key;
	wtk_fst_state_t *state;
	qtk_kwfstdec_token_t *token;
	qtk_hash_elem_t *tail;
};

struct qtk_hash_bucket
{
	//wtk_queue_node_t q_n;
	int prev_bucket;
	qtk_hash_elem_t  *last_elem;
};

struct qtk_hash_list
{
	qtk_hash_elem_t *list_head;
	qtk_hash_elem_t *list_tail;   //add for reused when free. by dmd
	qtk_hash_elem_t *freed_head;
	int bucket_list_tail;
	int size;
	qtk_hash_bucket_t **bucket_q;//bucket q
	qtk_hash_bucket_t *bucket_q_bak;//bucket q
	wtk_heap_t *heap;
};

qtk_hash_list_t* qtk_hash_list_new(int size);
qtk_kwfstdec_token_t* qtk_hash_list_new_token(qtk_hash_list_t *hash, float extra_cost);
qtk_kwfstdec_pth_t* qtk_hash_list_new_pth(qtk_hash_list_t* hash,qtk_kwfstdec_token_t *token);
void qtk_hash_list_reset(qtk_hash_list_t* hash);
void qtk_hash_list_delete(qtk_hash_list_t* hash);
int qtk_hash_list_Insert(qtk_hash_list_t* hash,int state_id,wtk_fst_state_t* state,qtk_kwfstdec_token_t* token);
//qtk_hash_elem_t*  qtk_hash_list_find(qtk_hash_list_t* hash,int state);
qtk_hash_elem_t* qtk_hash_list_clear(qtk_hash_list_t* hash);
qtk_hash_elem_t*  qtk_hash_list_new_elem(qtk_hash_list_t* hash);
void qtk_hash_list_del_elem(qtk_hash_list_t* hash,qtk_hash_elem_t* elem);
qtk_kwfstdec_link_t* qtk_hash_list_link_new(qtk_hash_list_t *hash,qtk_kwfstdec_token_t* token,int in_label,int out_label,float graph_cost,float acoustic_cost,qtk_kwfstdec_link_t* next);
void qtk_hash_list_link_delete(qtk_kwfstdec_token_t* token);
void qtk_hash_list_del(qtk_hash_list_t* hash, qtk_hash_elem_t* elem);
//TODO set size,get head

//OPERATION
//Note: follow function_x must keep __x consistent in using.
qtk_hash_list_t* qtk_hash_list_new2(int size);
void qtk_hash_list_reset2(qtk_hash_list_t* hash);
void qtk_hash_list_delete2(qtk_hash_list_t* hash);
int qtk_hash_list_Insert2(qtk_hash_list_t* hash,int state_id,wtk_fst_state_t* state,qtk_kwfstdec_token_t* token);
qtk_hash_elem_t* qtk_hash_list_clear2(qtk_hash_list_t* hash);
qtk_hash_elem_t*  qtk_hash_list_new_elem2(qtk_hash_list_t* hash, int size);

qtk_hash_list_t* qtk_hash_list_new3(int size);
void qtk_hash_list_reset3(qtk_hash_list_t* hash);
void qtk_hash_list_delete3(qtk_hash_list_t* hash);
qtk_hash_elem_t*  qtk_hash_list_new_elem3(qtk_hash_list_t* hash, int size);
void qtk_hash_list_Insert3(qtk_hash_list_t* hash, int state_id,wtk_fst_state_t* state,
		qtk_kwfstdec_token_t* token);
qtk_hash_elem_t* qtk_hash_list_clear3(qtk_hash_list_t* hash);
void qtk_hash_list_free(qtk_hash_list_t* hash, qtk_hash_elem_t *head, qtk_hash_elem_t *tail);
int qtk_hash_list_bytes(qtk_hash_list_t* hash);
#ifdef __cplusplus
};
#endif
#endif

