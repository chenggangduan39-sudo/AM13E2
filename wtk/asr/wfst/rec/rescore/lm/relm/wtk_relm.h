#ifndef WTK_FST_LM_RELM_WTK_RELM_H_
#define WTK_FST_LM_RELM_WTK_RELM_H_
#include "wtk/core/wtk_type.h"
#include "wtk_relm_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_relm wtk_relm_t;
struct wtk_relm
{
	wtk_relm_cfg_t *cfg;
	wtk_lmlat_t *lmlat;
};


wtk_relm_t* wtk_relm_new(wtk_relm_cfg_t *cfg,wtk_lm_dict_cfg_t *dict);
void wtk_relm_delete(wtk_relm_t *r);
void wtk_relm_reset(wtk_relm_t *r);
int wtk_relm_bytes(wtk_relm_t *r);
wtk_fst_net2_t* wtk_relm_process(wtk_relm_t *r,wtk_fst_net2_t *input);
#ifdef __cplusplus
};
#endif
#endif
