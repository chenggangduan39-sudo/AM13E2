#ifndef WTK_KSR_PARM_WTK_PCEN_CFG
#define WTK_KSR_PARM_WTK_PCEN_CFG
#include "wtk/core/cfg/wtk_local_cfg.h" 
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_pcen_cfg wtk_pcen_cfg_t;

struct wtk_pcen_cfg
{
	float smooth_factor;
	float epsilon;
	float alpha;
	float gamma;
	float delta;
	unsigned use_torch:1;
};

int wtk_pcen_cfg_init(wtk_pcen_cfg_t *cfg);
int wtk_pcen_cfg_clean(wtk_pcen_cfg_t *cfg);
int wtk_pcen_cfg_update_local(wtk_pcen_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_pcen_cfg_update(wtk_pcen_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
