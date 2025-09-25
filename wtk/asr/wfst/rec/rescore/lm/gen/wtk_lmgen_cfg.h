#ifndef WTK_FST_LM_NGLM_WTK_LMGEN_CFG
#define WTK_FST_LM_NGLM_WTK_LMGEN_CFG
#include "wtk/core/cfg/wtk_local_cfg.h" 
#include "wtk_lmgen_rec.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_lmgen_cfg wtk_lmgen_cfg_t;
struct wtk_lmgen_cfg
{
	wtk_lm_dict_cfg_t dict;
	wtk_lmgen_rec_cfg_t backward;
	wtk_lmgen_rec_cfg_t forward;
	char *stop_wrd_fn;
	wtk_str_hash_t *stop_wrd;
};

int wtk_lmgen_cfg_init(wtk_lmgen_cfg_t *cfg);
int wtk_lmgen_cfg_clean(wtk_lmgen_cfg_t *cfg);
int wtk_lmgen_cfg_update_local(wtk_lmgen_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_lmgen_cfg_update(wtk_lmgen_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
