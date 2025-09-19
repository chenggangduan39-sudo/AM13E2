#ifndef WTK_FST_LM_WTK_LM_RESCORE_CFG_H_
#define WTK_FST_LM_WTK_LM_RESCORE_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/asr/wfst/rec/rescore/lm/nglm/wtk_nglm.h"
#include "wtk/asr/wfst/net/wtk_fst_net_cfg.h"
#include "wtk/asr/wfst/rec/rescore/lm/relm/wtk_relm.h"
#include "wtk/asr/wfst/rec/rescore/lm/nglm/wtk_lm_dict_cfg.h"
#include "wtk/asr/wfst/rec/rescore/lm/lmlat/wtk_lmlat.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_lm_rescore_cfg wtk_lm_rescore_cfg_t;


struct wtk_lm_rescore_cfg
{
	wtk_lm_dict_cfg_t dict;
	wtk_lmlat_cfg_t main_lm;
	wtk_lmlat_cfg_t post_lm;
	wtk_relm_cfg_t* custom_lm;
	int n_custom;
	unsigned use_post:1;
};

int wtk_lm_rescore_cfg_init(wtk_lm_rescore_cfg_t *cfg);
int wtk_lm_rescore_cfg_clean(wtk_lm_rescore_cfg_t *cfg);
int wtk_lm_rescore_cfg_update_local(wtk_lm_rescore_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_lm_rescore_cfg_update(wtk_lm_rescore_cfg_t *cfg);
void wtk_lm_rescore_cfg_set_sym_out(wtk_lm_rescore_cfg_t *cfg,wtk_fst_sym_t *sym_out);
int wtk_lm_rescore_cfg_update2(wtk_lm_rescore_cfg_t *cfg,wtk_source_loader_t *sl);
#ifdef __cplusplus
};
#endif
#endif
