#ifndef WTK_DECODER_NET_WTK_EBNF_H_
#define WTK_DECODER_NET_WTK_EBNF_H_
#include "wtk/asr/net/wtk_latset.h"
#include "wtk/core/wtk_array.h"
#include "wtk/core/errno/wtk_eos.h"
#ifdef __cplusplus
extern "C" {
#endif
#define wtk_ebnf_feed_s(ebnf,s) wtk_ebnf_feed(ebnf,s,sizeof(s)-1)
#define wtk_ebnf_set_err_s(e,s) wtk_ebnf_set_err(e,s,sizeof(s)-1)

typedef struct wtk_ebnf wtk_ebnf_t;
typedef struct wtk_enode wtk_enode_t;
typedef struct wtk_enodeset wtk_enodeset_t;
typedef struct wtk_enk wtk_enk_t;
typedef struct wtk_subnet wtk_subnet_t;
typedef struct wtk_enodeinfo wtk_enodeinfo_t;

typedef enum
{
	unknown,
	wdInternal,wdExternal,wdBegin,wdEnd,nullNode
}wtk_enodetype_t;

struct wtk_enodeinfo
{
	wtk_enodetype_t type;
	wtk_enode_t *history;
	int nodes;
	unsigned seen:1;
};

struct wtk_enodeset
{
	int n_use;			//num sharing this node set.
	wtk_array_t *link_array;
	void *user;
};

struct wtk_enode
{
	wtk_name_t *name; //name of node.
	wtk_name_t *ext_name;//external name (used in compatability mode) or sub lat
	wtk_enode_t *chain;
	wtk_enodeset_t *succ;
	wtk_enodeset_t *pred;
	void *user;
};

struct wtk_enk
{
	wtk_enode_t *enter;
	wtk_enode_t *exit;
	wtk_enode_t *chain;
};

struct wtk_subnet
{
	wtk_name_t *name;
	wtk_enk_t network;
	wtk_subnet_t *next;
};

struct wtk_ebnf
{
	wtk_label_t label;
	wtk_enk_t net;
	wtk_eos_t *os;
	wtk_dict_word_find_f gwh;
	void *gwh_data;
	wtk_dict_t *dict;
	wtk_heap_t *heap;
	wtk_enode_t *chain;
	wtk_name_t *sub_net_id;
	wtk_name_t *enter_id;
	wtk_name_t *exit_id;
	wtk_name_t *enter_exit_id;
	wtk_subnet_t *sub_list;
	wtk_lat_t *lat;
	int link_chunk_size;
};

wtk_ebnf_t* wtk_ebnf_new(wtk_eos_t *os,wtk_dict_t *d,wtk_dict_word_find_f gwh,void *data);
int wtk_ebnf_init(wtk_ebnf_t *ebnf,wtk_eos_t *os,wtk_dict_t *dict,wtk_dict_word_find_f gwh,void *data);
int wtk_ebnf_clean(wtk_ebnf_t *ebnf);
int wtk_ebnf_delete(wtk_ebnf_t *e);
int wtk_ebnf_reset(wtk_ebnf_t *ebnf);
int wtk_ebnf_feed(wtk_ebnf_t *ebnf,char* data,int len);
void wtk_ebnf_set_err(wtk_ebnf_t *e,char *msg,int msg_bytes);
void wtk_ebnf_set_word_err(wtk_ebnf_t *e,wtk_string_t *w);
#ifdef __cplusplus
};
#endif
#endif
