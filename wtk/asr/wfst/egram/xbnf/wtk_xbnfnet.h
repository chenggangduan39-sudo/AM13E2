#ifndef WTK_FST_EGRAM_XBNF_WTK_XBNFNET_H_
#define WTK_FST_EGRAM_XBNF_WTK_XBNFNET_H_
#include "wtk/core/wtk_type.h"
#include "wtk_xbnfnet_cfg.h"
#include "wtk/asr/wfst/egram/wtk_egram_sym.h"
#include "wtk/asr/wfst/net/wtk_fst_net2.h"
#include "wtk_xbnf.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_xbnfnet wtk_xbnfnet_t;

struct wtk_xbnfnet
{
	wtk_xbnfnet_cfg_t *cfg;
	wtk_egram_sym_t *sym;
	wtk_xbnf_t *xbnf;
	wtk_fst_net2_t *output;
	wtk_slist_t state_l;
	unsigned use_rep:1;
	unsigned use_leak:1;
	unsigned use_addre:1;
	unsigned use_wrd:1;
};

wtk_xbnfnet_t* wtk_xbnfnet_new(wtk_xbnfnet_cfg_t *cfg,wtk_egram_sym_t *sym);
void wtk_xbnfnet_delete(wtk_xbnfnet_t *xb);
void wtk_xbnfnet_reset(wtk_xbnfnet_t *xb);
int wtk_xbnfnet_process(wtk_xbnfnet_t *xb,wtk_string_t *ebnf,wtk_fst_net2_t *net);
int wtk_xbnfnet_process2(wtk_xbnfnet_t *xb,char *fn,wtk_fst_net2_t *net);

void wtk_xbnfnet_attach_wrd2(wtk_xbnfnet_t *xb,wtk_fst_net2_t *output, wtk_string_t *str,
		wtk_fst_state2_t *pre_s,wtk_fst_state2_t *ie, float lm_like);
wtk_fst_state2_t* wtk_xbnfnet_load_net(wtk_xbnfnet_t *xb, wtk_string_t *data, wtk_fst_state2_t** ps, wtk_fst_state2_t**pe);
#ifdef __cplusplus
};
#endif
#endif
