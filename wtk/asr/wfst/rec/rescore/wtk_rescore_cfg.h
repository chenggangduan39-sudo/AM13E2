#ifndef WTK_FST_RESCORE_WTK_RESCORE_CFG_H_
#define WTK_FST_RESCORE_WTK_RESCORE_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/asr/wfst/rec/rescore/lm/wtk_lm_rescore.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_rescore_cfg wtk_rescore_cfg_t;

struct wtk_rescore_cfg
{
	wtk_lm_rescore_cfg_t lm;
	int nbest_max_search;
	unsigned use_hist:1;
};

int wtk_rescore_cfg_init(wtk_rescore_cfg_t *cfg);
int wtk_rescore_cfg_clean(wtk_rescore_cfg_t *cfg);
int wtk_rescore_cfg_update_local(wtk_rescore_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_rescore_cfg_update(wtk_rescore_cfg_t *cfg);
void wtk_rescore_set_sym_out(wtk_rescore_cfg_t *cfg,wtk_fst_sym_t *sym_out);
int wtk_rescore_cfg_update2(wtk_rescore_cfg_t *cfg,wtk_source_loader_t *sl);
#ifdef __cplusplus
};
#endif
#endif
