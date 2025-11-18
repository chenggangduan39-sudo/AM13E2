#ifndef WTK_FST_EGRAM_XBNF_WTK_XBNFNET_CFG_H_
#define WTK_FST_EGRAM_XBNF_WTK_XBNFNET_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk_xbnf_cfg.h"
#include "qtk_xbnf_post_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_xbnfnet_cfg wtk_xbnfnet_cfg_t;
struct wtk_xbnfnet_cfg
{
	wtk_xbnf_cfg_t xbnf;
	qtk_xbnf_post_cfg_t xbnf_post;
	int type;
	unsigned lower:1;
	//for leak words. add by dmd 2020.02.20
	unsigned use_leak:1;
	unsigned use_selfloop:1;     //for self loop to whole text.
	unsigned use_addre:1;        //build word-net with linking every words using all words in text
	// for ctx-pron.
	unsigned use_ctx:1;
};

int wtk_xbnfnet_cfg_init(wtk_xbnfnet_cfg_t *cfg);
int wtk_xbnfnet_cfg_clean(wtk_xbnfnet_cfg_t *cfg);
int wtk_xbnfnet_cfg_update_local(wtk_xbnfnet_cfg_t *cfg,wtk_local_cfg_t *main);
int wtk_xbnfnet_cfg_update(wtk_xbnfnet_cfg_t *cfg);
int wtk_xbnfnet_cfg_update2(wtk_xbnfnet_cfg_t *cfg,wtk_source_loader_t *sl);
#ifdef __cplusplus
};
#endif
#endif
