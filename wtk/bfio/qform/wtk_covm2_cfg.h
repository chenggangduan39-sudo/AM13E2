#ifndef WTK_BFIO_QFORM_WTK_COVM2_CFG
#define WTK_BFIO_QFORM_WTK_COVM2_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_covm2_cfg wtk_covm2_cfg_t;
struct wtk_covm2_cfg
{
    int cov_hist;
    float eye;
};

int wtk_covm2_cfg_init(wtk_covm2_cfg_t *cfg);
int wtk_covm2_cfg_clean(wtk_covm2_cfg_t *cfg);
int wtk_covm2_cfg_update_local(wtk_covm2_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_covm2_cfg_update(wtk_covm2_cfg_t *cfg);
int wtk_covm2_cfg_update2(wtk_covm2_cfg_t *cfg,wtk_source_loader_t *sl);
#ifdef __cplusplus
};
#endif
#endif