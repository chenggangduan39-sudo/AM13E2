#ifndef WTK_BFIO_MASKDENOISE_WTK_BANKFEAT_CFG
#define WTK_BFIO_MASKDENOISE_WTK_BANKFEAT_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_bankfeat_cfg wtk_bankfeat_cfg_t;
struct wtk_bankfeat_cfg
{
    int rate;

    int wins;

	int nb_bands;  
    int ceps_mem;
    int nb_delta_ceps;

    int *eband;

    int nb_features;

    unsigned use_ceps:1;
};

int wtk_bankfeat_cfg_init(wtk_bankfeat_cfg_t *cfg);
int wtk_bankfeat_cfg_clean(wtk_bankfeat_cfg_t *cfg);
int wtk_bankfeat_cfg_update_local(wtk_bankfeat_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_bankfeat_cfg_update(wtk_bankfeat_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif