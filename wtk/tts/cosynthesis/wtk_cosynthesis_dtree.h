#ifndef WTK_COSYN_DTREE
#define WTK_COSYN_DTREE
#include "wtk/core/wtk_type.h" 
#include "wtk/core/wtk_strpool.h"
#include "wtk_cosynthesis_dtree_cfg.h"
#include "wtk/core/cfg/wtk_source.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_cosynthesis_dtree wtk_cosynthesis_dtree_t;
#define wtk_cosynthesis_dtree_search_qs_s(d,t,s) wtk_cosynthesis_dtree_search_qs(d,t,s,sizeof(s)-1)


typedef enum
{
    WTK_SYN_DTREE_TREE_DUR=0,   //duration tree
    WTK_SYN_DTREE_TREE_LF0, //log F0 tree
    WTK_SYN_DTREE_TREE_MCP, //spectrum tree
    WTK_SYN_DTREE_TREE_BAP, //aperiodicity tree
    WTK_SYN_DTREE_TREE_CONCA_LF0, //连接代价lf0的树
    WTK_SYN_DTREE_TREE_CONCA_MGC, //连接代价的mgc树
    WTK_SYN_DTREE_TREE_LF0GV,//GV of log F0 tree
    WTK_SYN_DTREE_TREE_MCPGV,   //GV of spectrum tree
    WTK_SYN_DTREE_TREE_BAPGV,   //GV of aperiodicity tree
}wtk_cosynthesis_dtree_tree_type_t;


typedef struct
{
    wtk_queue_node_t q_n;
    wtk_string_t *pat;
//  int nstar;
//  int nques;
//  int max;
    unsigned use_str:1;
}wtk_cosynthesis_pat_t;

typedef struct
{
    //wtk_queue_node_t q_n;
    wtk_queue_t pat_q;
    wtk_string_t *name;
}wtk_cosynthesis_qs_item_t;

typedef struct
{
    //wtk_queue_t item_q;
    wtk_str_hash_t *hash;
}wtk_cosynthesis_qs_t;

typedef struct wtk_cosynthesis_node wtk_cosynthesis_node_t;

struct wtk_cosynthesis_node
{
    wtk_cosynthesis_qs_item_t *quest;
    wtk_cosynthesis_node_t *yes;    /* link to child node (yes) */
    wtk_cosynthesis_node_t *no; /* link to child node (no)  */
    //int idx;      /* index of this node */
    int pdf;        /* index of pdf for this node (leaf node only) */
    //wtk_cosynthesis_node_t *next; /* link to next node in all nodes  */
};

typedef struct
{
    wtk_queue_node_t q_n;
    wtk_queue_t pat_q;
    wtk_cosynthesis_node_t *root;
    wtk_cosynthesis_node_t *leaf;
    int state;

    wtk_cosynthesis_node_t *nodes;
    int node_cnt;
}wtk_cosynthesis_tree_item_t;

typedef struct
{
    wtk_queue_t item_q;//
}wtk_cosynthesis_tree_t;


struct wtk_cosynthesis_dtree
{
    wtk_cosynthesis_dtree_cfg_t *cfg;
    wtk_heap_t *heap;
    wtk_strpool_t *pool;
    wtk_cosynthesis_qs_t *qs[WTK_SYN_DTREE_TREE_BAPGV+1];
    wtk_cosynthesis_tree_t *tree[WTK_SYN_DTREE_TREE_BAPGV+1];
};
/* load decision trees for dur, logF0, mel-cep, and aperiodicity */

wtk_cosynthesis_dtree_t* wtk_cosynthesis_dtree_new(wtk_cosynthesis_dtree_cfg_t *cfg,wtk_source_loader_t *sl,wtk_strpool_t *pool);
void wtk_cosynthesis_dtree_delete(wtk_cosynthesis_dtree_t *m);

int wtk_cosynthesis_dtree_count(wtk_cosynthesis_dtree_t *d,wtk_cosynthesis_dtree_tree_type_t type,int state);

int wtk_cosynthesis_dtree_search(wtk_cosynthesis_dtree_t *d,wtk_cosynthesis_dtree_tree_type_t type,int state,char *s,int s_bytes,int *idx);

wtk_cosynthesis_qs_item_t* wtk_cosynthesis_dtree_search_qs(wtk_cosynthesis_dtree_t *d,wtk_cosynthesis_dtree_tree_type_t type,char *s,int s_bytes);

int wtk_cosynthesis_qs_item_match(wtk_cosynthesis_qs_item_t *item,char *s,int s_bytes);
#ifdef __cplusplus
};
#endif
#endif
