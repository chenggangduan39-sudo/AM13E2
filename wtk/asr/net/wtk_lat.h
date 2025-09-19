#ifndef WTK_DECODER_NET_WTK_LAT_H_
#define WTK_DECODER_NET_WTK_LAT_H_
#include "wtk/asr/model/wtk_dict.h"
#include "wtk/asr/model/wtk_hmmset.h"
#include "wtk/core/wtk_str_encode.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_lat wtk_lat_t;
typedef struct wtk_lnode wtk_lnode_t;
typedef struct wtk_larc wtk_larc_t;
typedef struct wtk_proninst wtk_proninst_t;
typedef struct wtk_netnode wtk_netnode_t;
typedef struct wtk_netlink wtk_netlink_t;
typedef struct wtk_lalign wtk_lalign_t;
struct wtk_netinst;

#define wtk_netnode_hmm(n) ((n)->type & n_hmm)
#define wtk_netnode_word(n) ((n)->type==n_word)
#define wtk_netnode_tr0(n) ((n)->type & n_tr0)
#define wtk_netnode_wd0(n) ((n)->type & n_wd0)

enum
{
	   n_unused=0,            /* Node Instance not yet assigned */
	   n_hmm=2,             /* Node Instance represents HMM */
	   n_word=4,            /* Node Instance represents word end (or null) */
	   n_tr0=4,             /* Entry token reaches exit in t=0 */
	   n_wd0=1,             /* Exit token reaches word node in t=0 */
	   n_wdstart=8,         /* Temporary wdstart node */
	   n_nocontext=15,      /* binary and with this to remove context ids */
	   n_lcontext=16,       /* Multiplication factor for context id */
	   n_rcontext=16384     /* Multiplication factor for context id */
};
typedef int wtk_netnode_type_t;

struct wtk_netnode
{
	wtk_netnode_type_t type;
	union
	{
		wtk_hmm_t *hmm;				//HMM (physical) definition
		wtk_dict_pron_t *pron;		//Word represented (may == null)
	}info;
	int nlinks;						//Number of nodes connected to this one
	wtk_netlink_t *links;			//Array[0..nlinks-1] of links to connected nodes
	int aux;						//used for latset expand
	int first_none_tr0_link_index;	//used for reorder n_tr0 node
	wtk_netnode_t *chain;			//chain in net node hash,used for latset expand.
	void 	*inst;					//used for latset expand
	struct wtk_netinst *net_inst;	//used for viterbi;
};

struct wtk_netlink
{
	wtk_netnode_t *node;	//next node in network.
	float like;				//transition likelihood
};

struct wtk_proninst
{
	wtk_lnode_t *ln;			//node that created this instance.
	wtk_dict_pron_t *pron;		//actual pronunciation.
	wtk_dict_phone_t **phones;	//Phone sequence for the instance
	wtk_netnode_t *starts;		//Chain of initial models
	wtk_netnode_t *ends;		//Chain of final models
	wtk_netnode_t *chain;		//Chain of inner nodes in word
	wtk_proninst_t *next;
	float fct;					//LM likelihood to be factored into each phone.
	int clen; 					//Number of non-cf phones in pronunciation
	int nstart;					//Number of models in starts chain
	int nend;					//Number of models in ends chain
	short nphones;				//number of phones for this instance.
	unsigned tee:1;				//TRUE if word consists solely of tee models

	/* for cross word */
	int ic;                 /* Initial context - cache saves finding for all links */
	int fc;                 /* Final context - cache saves finding for all links */
	unsigned int fci:1;    /* Final phone context independent */

	wtk_str_hash_t *lc;     /* Left contexts - linked to word initial models */
	wtk_str_hash_t *rc;     /* Right contexts - linked to word end nodes */
};

typedef enum
{
	WTK_LNODE_WORD=0,
	WTK_LNODE_SUBLAT
}wtk_lnode_type_t;

struct wtk_lnode
{
	double score;			//field used for pruning, if factor_lm, saved the largest lmlike, when expand lattice,
							//every in arc minus this score to keep path lmlike equal.
	double time;            //time of word boundary at node(second)
	int n;			        //sorted order, used for sort.
	short v;            	/* Pronunciation variant number */
	wtk_lnode_type_t type;
	union
	{
		wtk_dict_word_t *word;
		wtk_lat_t *lat;
		wtk_string_t *fst_wrd;
	}info;
	wtk_proninst_t *pron_chain;
	wtk_larc_t *foll;		//linked list of arcs following node
	wtk_larc_t *pred;		//linked list of arc preceding node
	void *hook;				//used when do lattice copy.
	int index;
};

struct wtk_lalign
{
	int state;				//State number (-1==model end)
	float dur;				//Duration of segment in seconds
	float like;				//Total aclike of label (inc trans within + out)
	wtk_string_t *name;		//Segment label ('phys_hmm[state]' or 'phys_hmm')
};

struct wtk_larc
{
	wtk_lalign_t *lalign;	//Array[0: nalign-1] of alignment records.
	wtk_lnode_t *start;
	wtk_lnode_t *end;
	wtk_larc_t *farc;
	wtk_larc_t *parc;
	float lmlike;		//Language model likelihood of word
	float aclike;		//Acoustic likelihood of word
	float prlike;		//Pronunciation likelihood of arc
	float score;		//field used for pruning.
	short nalign;		//Number of alignment records in word.
	int index;
};

struct wtk_lat
{
	wtk_queue_node_t set_n;
	wtk_heap_t *heap;
	wtk_string_t *name;		//if is sublat, name is valid.
	wtk_lnode_t *lnodes;	//Array of lattice nodes.
	wtk_larc_t *larcs;		//Array of lattice arcs.
	wtk_lnode_t *ls;		//lstart node;
	wtk_netnode_t initial;	//Initial(dummy) node.
	wtk_netnode_t final;	//Final (dummy) node.
	wtk_netnode_t *chain;
	int nlink;
	int nnode;
	int nn;					//number of nodes.
	int na;					//number of arcs.
	float acscale;			//Acoustic scale factor
	float lmscale;			//LM scale factor.
	float wdpenalty;		//Word insertion penalty.
	float prscale;			//Pronunciation scale factor.
	float logbase;			//base of logarithm for likelihoods in lattice file. 1.0=default(e),0.0=no logs.
	float tscale;			//time scale factor(default 1 second)
};

wtk_string_t* wtk_lnode_name(wtk_lnode_t *n);
wtk_lat_t* wtk_lat_new_h(wtk_heap_t *heap);
int wtk_lat_init(wtk_lat_t *lat,wtk_heap_t *heap);
int wtk_lat_clean(wtk_lat_t *lat);
int wtk_lat_reset(wtk_lat_t *lat);
int wtk_lat_create(wtk_lat_t *lat,int nodes,int arcs);

/*
 * dup expand net lattice
 */
wtk_lat_t* wtk_lat_dup(wtk_lat_t *lat,wtk_heap_t *heap);

/**
 * dup net lattice
 */
wtk_lat_t *wtk_lat_dup2(wtk_lat_t *lat,wtk_heap_t *heap);

void wtk_lat_disable_lmlike(wtk_lat_t *lat);
wtk_lnode_t* wtk_lat_start_node(wtk_lat_t *lat);
wtk_lnode_t* wtk_lat_end_node(wtk_lat_t *lat);
int wtk_lnode_next_len(wtk_lnode_t *n);
void wtk_lat_clean_node_hook(wtk_lat_t *lat);
wtk_string_t* wtk_netnode_name(wtk_netnode_t *n);
char* wtk_netnode_name2(wtk_netnode_t *n);
char* wtk_lnode_name2(wtk_lnode_t *n);
void wtk_lnode_mark_back(wtk_lnode_t *ln,int *nn);
wtk_netnode_t* wtk_netnode_wn(wtk_netnode_t *n);
void wtk_netnode_print3(wtk_netnode_t *n);
void wtk_netnode_print_nxtpath(wtk_netnode_t *n);
void wtk_lat_print(wtk_lat_t *lat);
void wtk_netnode_print(wtk_netnode_t *n);
void wtk_proninst_print(wtk_proninst_t *inst);
void wtk_lat_print_net(wtk_lat_t *lat);


void wtk_lat_print2(wtk_lat_t *lat,FILE* file);

/**
 * print order lattice,most use this function to write lattice
 */
void wtk_lat_print3(wtk_lat_t *lat,FILE* file);
void wtk_lat_write(wtk_lat_t *lat,char *fn);

/**
 * print fst wrod
 */
void wtk_lat_print4(wtk_lat_t *lat,FILE* file);
void wtk_lat_print5(wtk_lat_t *lat,FILE* file);
void wtk_lat_print_addr(wtk_lat_t *lat);

wtk_lnode_t* wtk_lat_new_lnode(wtk_lat_t *lat);
wtk_larc_t* wtk_lat_new_arc(wtk_lat_t *lat);
void wtk_lat_link_node2(wtk_lat_t *lat,wtk_lnode_t *s,wtk_lnode_t *e);
void wtk_lat_print_link(wtk_lat_t *lat,FILE *log);
void wtk_lnode_print(wtk_lnode_t *n);
void wtk_lnode_print_prev(wtk_lnode_t *n);
void wtk_lnode_print_next(wtk_lnode_t *n);
int wtk_lnode_input_arcs(wtk_lnode_t *n);

int wtk_lnode_has_same_input(wtk_lnode_t *n1,wtk_lnode_t *n2);
int wtk_lnode_has_same_output(wtk_lnode_t *n1,wtk_lnode_t *n2);
#ifdef __cplusplus
};
#endif
#endif
