#ifndef WTK_FST_EGRAM_XBNF_WTK_XBNF_CFG_H_
#define WTK_FST_EGRAM_XBNF_WTK_XBNF_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/segmenter/wtk_segmenter.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_xbnf_cfg wtk_xbnf_cfg_t;
struct wtk_xbnf_cfg
{
	wtk_segmenter_cfg_t segmenter;
	wtk_array_t *en_pre;
	wtk_array_t *en_mid;
	wtk_array_t *en_pst;
	int hash_hint;
	int expr_hint;
	unsigned use_merge:1;
	unsigned use_seg:1;
};

int wtk_xbnf_cfg_init(wtk_xbnf_cfg_t *cfg);
int wtk_xbnf_cfg_clean(wtk_xbnf_cfg_t *cfg);
int wtk_xbnf_cfg_update_local(wtk_xbnf_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_xbnf_cfg_update(wtk_xbnf_cfg_t *cfg);
int wtk_xbnf_cfg_update2(wtk_xbnf_cfg_t *cfg, wtk_source_loader_t *sl);
#ifdef __cplusplus
};
#endif
#endif
