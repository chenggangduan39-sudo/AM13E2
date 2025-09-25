#ifndef QTK_K2_CONTEXT_NET_H_
#define QTK_K2_CONTEXT_NET_H_

#include "qtk_k2_wrapper_cfg.h"
#include "wtk/asr/wfst/net/wtk_fst_net.h"
#include "wtk/asr/wfst/egram/wtk_kvdict.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_k2_keywrd_q qtk_k2_keywrd_q_t;
typedef struct qtk_k2_context_net qtk_k2_context_net_t;
typedef wtk_dict_word_t*(*wtk_egram_sym_get_word_f)(void *ths,char *wrd,int wrd_bytes);

struct qtk_k2_keywrd_q
{
	wtk_queue_node_t q_n;
	int ids[40][6];
	wtk_string_t olab;
	int out_id;
	int wrd_cnt;
	int num_phn;
	int* phns;
};

struct qtk_k2_context_net
{
	qtk_k2_wrapper_cfg_t *cfg;
	wtk_xbnfnet_t *xbnf;
	wtk_fst_net_t *net;
	wtk_fst_net2_t* tmp_net;
	wtk_fst_net2_t* tmp_net2;
	wtk_heap_t *heap;
	wtk_label_t *label;
	wtk_egram_sym_t sym;
	wtk_strbuf_t *text;
	union{
		wtk_kvdict_t *kv;
	}dict;
	wtk_slist_t state_l;
	wtk_queue_t keywrd_q;
	wtk_string_t **out_sym;
	int last_outid;
};

qtk_k2_context_net_t* qtk_k2_context_net_new(qtk_k2_wrapper_cfg_t* cfg);
int qtk_k2_context_net_start(qtk_k2_context_net_t* net);
void qtk_k2_context_net_reset(qtk_k2_context_net_t* net);
void qtk_k2_context_net_reset2(qtk_k2_context_net_t* net);
void qtk_k2_context_net_delete(qtk_k2_context_net_t* net);
int qtk_k2_context_net_build(qtk_k2_context_net_t* net,char *data, int len);
int qtk_k2_context_net_build_keyword(qtk_k2_context_net_t* net,char *data, int len);
int qtk_k2_context_net_search(qtk_k2_context_net_t* net, float *score, int stateID, int id);
void qtk_k2_context_net_search_keyword(qtk_k2_context_net_t* net, float *score, int* stateID, int id, int *out, int* stateID2);
void qtk_k2_context_net_dump(qtk_k2_context_net_t* net);
int qtk_k2_context_net_build_keyword_plus(qtk_k2_context_net_t* net,char *data, int len);
wtk_string_t* qtk_k2_context_net_get_outsym(qtk_k2_context_net_t* net, int out_id);
int qtk_k2_context_net_build_keyword_plus_xbnf(qtk_k2_context_net_t* net,char *data, int len);
#ifdef __cplusplus
};
#endif
#endif

