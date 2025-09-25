#ifndef WTK_FST_LM_WTK_NGLM_CFG_H_
#define WTK_FST_LM_WTK_NGLM_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/asr/wfst/net/sym/wtk_fst_insym.h"
#include "wtk/core/rbin/wtk_rbin2.h"
#include "wtk_lm_node.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_nglm_cfg wtk_nglm_cfg_t;

struct wtk_nglm_cfg
{
	wtk_rbin2_t *rbin;
	//wtk_string_t name;
	char *lm_fn;
	int max_order;
	float reset_max_bytes;
	unsigned load_all:1;
	unsigned debug:1;
	unsigned use_dynamic_reset:1;
};

int wtk_nglm_cfg_init(wtk_nglm_cfg_t *cfg);
int wtk_nglm_cfg_clean(wtk_nglm_cfg_t *cfg);
int wtk_nglm_cfg_update_local(wtk_nglm_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_nglm_cfg_update(wtk_nglm_cfg_t *cfg);
void wtk_nglm_cfg_update_id(wtk_nglm_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
