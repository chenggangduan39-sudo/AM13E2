#ifndef WTK_FST_LM_WTK_LM_DICT_CFG_H_
#define WTK_FST_LM_WTK_LM_DICT_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/asr/wfst/net/sym/wtk_fst_insym.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_lm_dict_cfg wtk_lm_dict_cfg_t;
struct wtk_lm_dict_cfg
{
	wtk_fst_insym_t *sym;
	int snt_start_id;
	int snt_end_id;
	char *sym_out_fn;
	unsigned int use_sym_bin:1;
};

int wtk_lm_dict_cfg_init(wtk_lm_dict_cfg_t *cfg);
int wtk_lm_dict_cfg_clean(wtk_lm_dict_cfg_t *cfg);
int wtk_lm_dict_cfg_update_local(wtk_lm_dict_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_lm_dict_cfg_update(wtk_lm_dict_cfg_t *cfg);
int wtk_lm_dict_cfg_update2(wtk_lm_dict_cfg_t *cfg,wtk_source_loader_t *sl);
#ifdef __cplusplus
};
#endif
#endif
