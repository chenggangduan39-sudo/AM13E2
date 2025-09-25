#ifndef WTK_DECODER_NET_WTK_LATSET_H_
#define WTK_DECODER_NET_WTK_LATSET_H_
#include "wtk/asr/model/wtk_dict.h"
#include "wtk_net_cfg.h"
#include "wtk_lat.h"
#include "wtk_hmmset_ctx.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_latset wtk_latset_t;
#define wtk_latset_add_lat(ls,lat) wtk_queue_push(&((ls)->lat_queue),&((lat)->set_n))

struct wtk_latset
{
	wtk_net_cfg_t *cfg;
	wtk_queue_t lat_queue;
	wtk_hmmset_ctx_t *hc;
	wtk_dict_t *dict;
	wtk_hmmset_t *hl;
	wtk_label_t *label;
	wtk_heap_t *heap;
	wtk_lat_t *main;
	int wn_nodes;
	wtk_netnode_t **wn_node_hash;
	wtk_strbuf_t *buf;
	wtk_array_t *tmp_array;
	wtk_dict_word_find_f dwf;
	void *dwf_data;

	/* for cross word */
	wtk_dict_word_t *null_word;
	unsigned tee_words:1;  /* True if any tee words are present */
	wtk_string_t **cxs;     /* Sorted array of labids indexed by context */
};

wtk_latset_t* wtk_latset_new(wtk_net_cfg_t* cfg,wtk_dict_t *d,wtk_hmmset_t *h,wtk_dict_word_find_f dwf,void *dwf_data);
int wtk_latset_reset(wtk_latset_t *ls);
int wtk_latset_delete(wtk_latset_t *l);
int wtk_latset_init(wtk_latset_t *ls,wtk_net_cfg_t* cfg,wtk_dict_t *dict,wtk_hmmset_t *hl,wtk_dict_word_find_f dwf,void *dwf_data);
int wtk_latset_clean(wtk_latset_t *ls);
wtk_lat_t* wtk_latset_find_lat(wtk_latset_t *ls,char* s,int ns);
wtk_lat_t* wtk_latset_new_lat(wtk_latset_t *ls);
int wtk_latset_load(wtk_latset_t *ls,wtk_source_t *s);
int wtk_latset_load_file(wtk_latset_t *ls,char *fn);
int wtk_latset_expand(wtk_latset_t *ls);
int wtk_latset_expand_lat2(wtk_latset_t *ls,wtk_lat_t *lat);
int wtk_latset_expand_lat(wtk_latset_t *ls,wtk_lat_t *lat);
wtk_string_t* wtk_hmmset_ctx_get_hci_ctx(wtk_hmmset_ctx_t *hc,wtk_string_t *name);
void wtk_latset_print_words(wtk_latset_t *ls,wtk_lat_t *lat);
void wtk_latset_print_network(wtk_latset_t *ls,wtk_lat_t *lat);
#ifdef __cplusplus
};
#endif
#endif
