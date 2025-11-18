#ifndef WTK_FST_EGRAM_WTK_EGRAM_H_
#define WTK_FST_EGRAM_WTK_EGRAM_H_
#include "wtk/core/wtk_type.h"
#include "wtk/asr/net/wtk_ebnf.h"
#include "wtk/asr/wfst/net/wtk_fst_net2.h"
#include "wtk/core/wtk_array.h"
#include "wtk_e2fst.h"
#include "wtk_egram_cfg.h"
#include "wtk_kvdict.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/core/rbin/wtk_rbin2.h"
#include "wtk/asr/wfst/egram/xbnf/wtk_xbnfnet.h"
#include "wtk/asr/wfst/egram/ebnf/wtk_ebnfnet.h"
#include "wtk_ecut_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_egram wtk_egram_t;
typedef struct wtk_egram_kwdec_item wtk_egram_kwdec_item_t;
typedef struct wtk_egram_kwdec_trans wtk_egram_kwdec_trans_t;

struct wtk_egram_kwdec_trans
{
	int in;
	int out;
	int from;
	int to;
	float weight;
	int base_state;
	int* map;
};

struct wtk_egram_kwdec_item 
{
	wtk_string_t* name;
	wtk_egram_kwdec_trans_t** trans;
	int *state_ntrans;//TODO ntrans from state
	int ntrans;
	int nstate;
	int n_1st_phns;
	int start_state;
};

struct wtk_egram
{
	wtk_egram_cfg_t *cfg;
	wtk_label_t *label;
	union
	{
		wtk_kvdict_t *kv;
	}dict;
	wtk_e2fst_t *e2fst;
	wtk_strbuf_t *buf;
	wtk_egram_sym_t sym;
	wtk_string_t *ebnf;
	union {
		wtk_ebnfnet_t *ebnf;
		wtk_xbnfnet_t *xbnf;
	}v;
	wtk_xbnfnet_t *xbnf;     //deprecated
	wtk_dict_word_find_f gwh;
	wtk_dict_word_find_f2 gwh2;
	void *gwh_data;
	wtk_ecut_cfg_t* ecut;
	wtk_array_t *a;
	wtk_heap_t *heap;
	wtk_egram_kwdec_item_t** items;
	wtk_strbuf_t *kwdec_tmp;
	int nwords;
};

wtk_egram_t* wtk_egram_new(wtk_egram_cfg_t *cfg,wtk_rbin2_t *rbin);
wtk_egram_t* wtk_egram_new2(wtk_mbin_cfg_t *cfg);
void wtk_egram_delete(wtk_egram_t *e);
void wtk_egram_reset(wtk_egram_t *e);
int wtk_egram_dump(wtk_egram_t *e,wtk_fst_net_t *net);
int wtk_egram_dump2(wtk_egram_t *e,wtk_fst_net_t *net);
int wtk_egram_dump3(wtk_egram_t *e,wtk_fst_net_t *net,wtk_fst_sym_t* sym);
int wtk_egram_dump_kwdec(wtk_egram_t *e,wtk_fst_net_t *net);
wtk_fst_net2_t* wtk_egram_get_net(wtk_egram_t *e);
wtk_dict_word_t* wtk_egram_get_dict_word(wtk_egram_t *e,char *data,int len);
wtk_dict_word_t* wtk_egram_get_dict_word2(wtk_egram_t *e,char *data,int len);
wtk_dict_word_t* wtk_egram_get_dict_word3(wtk_egram_t *e,char *data,int len, void* info);
void wtk_egram_set_word_notify(wtk_egram_t *e, wtk_dict_word_find_f gwh, void *data);
void wtk_egram_set_word_notify2(wtk_egram_t *e, wtk_dict_word_find_f2 gwh2, void *data);
wtk_dict_t* wtk_egram_get_dict(wtk_egram_t *e);
int wtk_egram_ebnf2fst(wtk_egram_t *e,char *ebnf,int ebnf_bytes);
int wtk_egram_ebnf2fst2(wtk_egram_t *e,char *fn);
int wtk_egram_ebnf2fst_kwdec(wtk_egram_t *e,char *ebnf,int ebnf_bytes);
void wtk_egram_get_outsym_txt(wtk_egram_t *e,wtk_strbuf_t *buf);
void wtk_egram_ebnf_2bin_kdec(wtk_egram_t *e,wtk_strbuf_t *res);
void wtk_egram_qlite_bin(wtk_egram_t *e,wtk_strbuf_t *res);
int wtk_egram_update_cmds(wtk_egram_t *e, char *words, int len);

void wtk_egram_get_outsym(wtk_egram_t *e,wtk_string_t *v);
void wtk_egram_get_outsym2(wtk_egram_t *e,wtk_strbuf_t *buf);
int wtk_egram_write(wtk_egram_t *e,char *fn);
void wtk_egram_write_txt(wtk_egram_t *e,char *out_fn,char *fsm_fn);
void wtk_egram_write_txtmono(wtk_egram_t *e,char *out_fn,char *fsm_fn);
void wtk_egram_write_txtbi(wtk_egram_t *e, char *out_fn, char *fsm_fn);
void wtk_egram_write_txtwrd(wtk_egram_t *e,char *out_fn,char *fsm_fn);
void wtk_egram_write_bin(wtk_egram_t *e,char *out_fn,char *fsm_fn);
#ifdef __cplusplus
};
#endif
#endif
