#ifndef WTK_ASR_PARM_WTK_DELTA_CFG
#define WTK_ASR_PARM_WTK_DELTA_CFG
#include "wtk/core/cfg/wtk_local_cfg.h" 
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_delta_cfg wtk_delta_cfg_t;

struct wtk_delta_cfg
{
	int order; //D A T
	int win;
};

int wtk_delta_cfg_init(wtk_delta_cfg_t *cfg);
int wtk_delta_cfg_clean(wtk_delta_cfg_t *cfg);
int wtk_delta_cfg_update_local(wtk_delta_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_delta_cfg_update(wtk_delta_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
