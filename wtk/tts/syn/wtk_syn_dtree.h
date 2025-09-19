#ifndef WTK_TTS_SYN_WTK_SYN_DTREE
#define WTK_TTS_SYN_WTK_SYN_DTREE
#include "wtk/core/wtk_type.h" 
#include "wtk/core/wtk_strpool.h"
#include "wtk_syn_dtree_cfg.h"
#include "wtk/core/cfg/wtk_source.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_syn_dtree wtk_syn_dtree_t;
#define wtk_syn_dtree_search_qs_s(d,t,s) wtk_syn_dtree_search_qs(d,t,s,sizeof(s)-1)


typedef enum
{
	WTK_SYN_DTREE_TREE_DUR=0,	//duration tree
	WTK_SYN_DTREE_TREE_LF0,	//log F0 tree
	WTK_SYN_DTREE_TREE_MCP,	//spectrum tree
	WTK_SYN_DTREE_TREE_BAP,	//aperiodicity tree
	WTK_SYN_DTREE_TREE_LF0GV,//GV of log F0 tree
	WTK_SYN_DTREE_TREE_MCPGV,	//GV of spectrum tree
	WTK_SYN_DTREE_TREE_BAPGV,	//GV of aperiodicity tree
}wtk_syn_dtree_tree_type_t;


typedef struct
{
	wtk_queue_node_t q_n;
	wtk_string_t *pat;
//	int nstar;
//	int nques;
//	int max;
	unsigned use_str:1;
}wtk_syn_pat_t;

typedef struct
{
	//wtk_queue_node_t q_n;
	wtk_queue_t pat_q;
	wtk_string_t *name;
}wtk_syn_qs_item_t;

typedef struct
{
	//wtk_queue_t item_q;
	wtk_str_hash_t *hash;
}wtk_syn_qs_t;

typedef struct wtk_syn_node wtk_syn_node_t;

struct wtk_syn_node
{
    wtk_syn_qs_item_t *quest;
    wtk_syn_node_t *yes;	/* link to child node (yes) */
    wtk_syn_node_t *no;	/* link to child node (no)  */
    //int idx;		/* index of this node */
    int pdf;		/* index of pdf for this node (leaf node only) */
    //wtk_syn_node_t *next; /* link to next node in all nodes  */
};

typedef struct
{
	wtk_queue_node_t q_n;
	wtk_queue_t pat_q;
	wtk_syn_node_t *root;
	wtk_syn_node_t *leaf;
	int state;

	wtk_syn_node_t *nodes;
	int node_cnt;
}wtk_syn_tree_item_t;

typedef struct
{
	wtk_queue_t item_q;//
}wtk_syn_tree_t;


struct wtk_syn_dtree
{
	wtk_syn_dtree_cfg_t *cfg;
	wtk_heap_t *heap;
	wtk_strpool_t *pool;
	wtk_syn_qs_t *qs[WTK_SYN_DTREE_TREE_BAPGV+1];
	wtk_syn_tree_t *tree[WTK_SYN_DTREE_TREE_BAPGV+1];
};
/* load decision trees for dur, logF0, mel-cep, and aperiodicity */

wtk_syn_dtree_t* wtk_syn_dtree_new(wtk_syn_dtree_cfg_t *cfg,wtk_source_loader_t *sl,wtk_strpool_t *pool);
void wtk_syn_dtree_delete(wtk_syn_dtree_t *m);

int wtk_syn_dtree_count(wtk_syn_dtree_t *d,wtk_syn_dtree_tree_type_t type,int state);

int wtk_syn_dtree_search(wtk_syn_dtree_t *d,wtk_syn_dtree_tree_type_t type,int state,char *s,int s_bytes,int *idx);

wtk_syn_qs_item_t* wtk_syn_dtree_search_qs(wtk_syn_dtree_t *d,wtk_syn_dtree_tree_type_t type,char *s,int s_bytes);

int wtk_syn_qs_item_match(wtk_syn_qs_item_t *item,char *s,int s_bytes);
#ifdef __cplusplus
};
#endif
#endif
