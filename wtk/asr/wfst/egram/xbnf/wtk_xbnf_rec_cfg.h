#ifndef WTK_FST_EGRAM_XBNF_WTK_XBNF_REC_CFG_H_
#define WTK_FST_EGRAM_XBNF_WTK_XBNF_REC_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_source.h"
#include "wtk_xbnf.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_xbnf_rec_cfg wtk_xbnf_rec_cfg_t;
struct wtk_xbnf_rec_cfg
{
	wtk_xbnf_cfg_t xbnf;
	wtk_xbnf_t *xb;
	char *xbnf_fn;
	int inst_cache;
};

int wtk_xbnf_rec_cfg_init(wtk_xbnf_rec_cfg_t *cfg);
int wtk_xbnf_rec_cfg_clean(wtk_xbnf_rec_cfg_t *cfg);
int wtk_xbnf_rec_cfg_update_local(wtk_xbnf_rec_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_xbnf_rec_cfg_update(wtk_xbnf_rec_cfg_t *cfg);
int wtk_xbnf_rec_cfg_update2(wtk_xbnf_rec_cfg_t *cfg,wtk_source_loader_t *sl);

int wtk_xbnf_rec_cfg_update3(wtk_xbnf_rec_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
