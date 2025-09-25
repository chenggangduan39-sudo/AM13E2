#ifndef WTK_VITE_F0_AVG_WTK_FAVG_CFG_H_
#define WTK_VITE_F0_AVG_WTK_FAVG_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_favg_cfg wtk_favg_cfg_t;
struct wtk_favg_cfg
{
	int win;
	float avg_prior;
	float norm_avg_prior;
	float norm_var_prior;
	float alpha;
};

int wtk_favg_cfg_init(wtk_favg_cfg_t *cfg);
int wtk_favg_cfg_clean(wtk_favg_cfg_t *cfg);
int wtk_favg_cfg_update_local(wtk_favg_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_favg_cfg_update(wtk_favg_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
