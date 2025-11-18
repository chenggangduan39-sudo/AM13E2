#ifndef WTK_FST_EGRAM_WTK_EBNFNET_H_
#define WTK_FST_EGRAM_WTK_EBNFNET_H_
#include "wtk/core/wtk_type.h"
#include "wtk/asr/wfst/net/wtk_fst_net2.h"
#include "wtk/asr/net/wtk_ebnf.h"
#include "wtk/asr/wfst/net/wtk_lat_net.h"
#include "wtk/asr/wfst/egram/wtk_egram_cfg.h"
#include "wtk/asr/wfst/egram/wtk_egram_sym.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_ebnfnet wtk_ebnfnet_t;

struct wtk_ebnfnet
{
	wtk_egram_cfg_t *cfg;
	wtk_egram_sym_t *sym;
	wtk_ebnf_t *ebnf;
	wtk_lat_t *expand_lat;
	wtk_lat_net_t *lat_net;
	wtk_heap_t *heap;
	wtk_strbuf_t *buf;
};

wtk_ebnfnet_t* wtk_ebnfnet_new(wtk_egram_cfg_t *cfg,wtk_egram_sym_t *sym);
void wtk_ebnfnet_delete(wtk_ebnfnet_t *n);
void wtk_ebnfnet_reset(wtk_ebnfnet_t *n);
int wtk_ebnfnet_process(wtk_ebnfnet_t *n,wtk_string_t *ebnf,wtk_fst_net2_t *net);
#ifdef __cplusplus
};
#endif
#endif
