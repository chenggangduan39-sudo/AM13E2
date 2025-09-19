#ifndef __WTK_COSYN_H__
#define __WTK_COSYN_H__

#include "wtk_cosynthesis_cfg.h"
#include "wtk_cosynthesis_backend.h"
#include "wtk_cosynthesis_cal.h"
#include "wtk/core/wtk_hash.h"
#include "wtk/core/wtk_heap.h"
#include <stdio.h>

#include "wtk_cosynthesis_output.h"
#ifdef USE_PHRASE
#include "wtk_cosynthesis_phrase.h"
#endif
#ifdef __cplusplus
extern "C" {
#endif

typedef void (*wtk_cosynthesis_notity_f)(void *ths, short *data, int len, short *uid, int id_len);

typedef struct 
{
    wtk_queue_node_t q_n;
    float cost;
    uint16_t unit_id;
    wtk_unit_t *unit;
    uint16_t *unit_path;
}wtk_cosynthesis_token_t;

typedef struct
{
	wtk_trietree_cfg_t *cfg;
    FILE *trie_fp;
    FILE *trie_inset_fp;
	wtk_trieroot_t **root;
	wtk_trieroot_t **root_inset;
	wtk_heap_t *heap;
	unsigned int load_all;
}wtk_mtrietree_t;

typedef struct
{
    wtk_cosynthesis_cfg_t *cfg;
    wtk_heap_t *heap;
    wtk_heap_t *dec_heap;   //maybe multi-times using in a circle
    wtk_cosynthesis_lexicon_t *lexicon;
    wtk_mtrietree_t* mtrie;
    wtk_vpool2_t *tok_pool;
    wtk_queue_t *cur_token_q;
    wtk_queue_t *pre_token_q;
    uint16_t *best_path;
    float best_cost;
    hts_lab *lab;
    ais_voice *front_end;
    wtk_cosynthesis_backend_t *back_end;
    wtk_wsola_t *wsola;
#ifdef USE_PHRASE
    wtk_cosynthesis_phrase_t *phrase;
#endif
    void *ths;
	wtk_cosynthesis_notity_f notify;
	wtk_cosynthesis_output_t *output;
	wtk_strbuf_t *raw_txt;
	wtk_strbuf_t *proc_txt;
	wtk_strbuf_t *buf;
}wtk_cosynthesis_t;

wtk_cosynthesis_t *wtk_cosynthesis_new(wtk_cosynthesis_cfg_t *cfg);
void wtk_cosynthesis_delete(wtk_cosynthesis_t *cs);
void wtk_cosynthesis_reset(wtk_cosynthesis_t *cs);
void wtk_cosynthesis_unit_decode_reset(wtk_cosynthesis_t *cs);
int wtk_cosynthesis_feed(wtk_cosynthesis_t *cs,char* data, int bytes);
int wtk_cosynthesis_feedm(wtk_cosynthesis_t *cs,char* data, int bytes, const char* sep);
void wtk_cosynthesis_set_notify(wtk_cosynthesis_t *cs,void *ths,wtk_cosynthesis_notity_f notify);

//other fun.
char* wtk_cosynthesis_get_rawtxt(wtk_cosynthesis_t *cs);
char* wtk_cosynthesis_get_proctxt(wtk_cosynthesis_t *cs);
void wtk_cosynthesis_cleaninfo(wtk_cosynthesis_t *cs);

wtk_cosynthesis_info_t* wtk_cosynthesis_getOutput(wtk_cosynthesis_t *cs);
#ifdef __cplusplus
};
#endif

#endif
