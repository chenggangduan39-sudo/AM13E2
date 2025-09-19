#ifndef WTK_VITE_VPRINT_TRAIN_WTK_VTRAIN_CFG
#define WTK_VITE_VPRINT_TRAIN_WTK_VTRAIN_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_vtrain_cfg wtk_vtrain_cfg_t;
struct wtk_vtrain_cfg
{
	float mapTau;
	int skip_frame;
	unsigned use_with_detect:1;
};

int wtk_vtrain_cfg_init(wtk_vtrain_cfg_t *cfg);
int wtk_vtrain_cfg_clean(wtk_vtrain_cfg_t *cfg);
int wtk_vtrain_cfg_update_local(wtk_vtrain_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_vtrain_cfg_update(wtk_vtrain_cfg_t *cfg);
int wtk_vtrain_cfg_update2(wtk_vtrain_cfg_t *cfg,wtk_source_loader_t *sl);
#ifdef __cplusplus
};
#endif
#endif
