#ifndef WTK_FST_LM_RELM_WTK_RELM_CFG_H_
#define WTK_FST_LM_RELM_WTK_RELM_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/asr/wfst/rec/rescore/lm/lmlat/wtk_lmlat.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_relm_cfg wtk_relm_cfg_t;
struct wtk_relm_cfg
{
	wtk_string_t name;
	wtk_lmlat_cfg_t lmlat;
};

int wtk_relm_cfg_init(wtk_relm_cfg_t *cfg);
int wtk_relm_cfg_clean(wtk_relm_cfg_t *cfg);
int wtk_relm_cfg_update_local(wtk_relm_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_relm_cfg_update(wtk_relm_cfg_t *cfg,wtk_lm_dict_cfg_t *dict);
void wtk_relm_cfg_set_sym_out(wtk_relm_cfg_t *cfg,wtk_fst_sym_t *sym_out);
#ifdef __cplusplus
};
#endif
#endif
