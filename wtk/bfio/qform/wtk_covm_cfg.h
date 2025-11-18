#ifndef WTK_BFIO_QFORM_WTK_COVM_CFG
#define WTK_BFIO_QFORM_WTK_COVM_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_covm_cfg wtk_covm_cfg_t;
struct wtk_covm_cfg
{
    float ncov_alpha;
    int init_ncovnf;

    float scov_alpha;
    int init_scovnf;

    int ncov_hist;
    int scov_hist;

    int ncov_flush_delay;

    unsigned use_scov:1;
    unsigned use_covhist:1;
};

int wtk_covm_cfg_init(wtk_covm_cfg_t *cfg);
int wtk_covm_cfg_clean(wtk_covm_cfg_t *cfg);
int wtk_covm_cfg_update_local(wtk_covm_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_covm_cfg_update(wtk_covm_cfg_t *cfg);
int wtk_covm_cfg_update2(wtk_covm_cfg_t *cfg,wtk_source_loader_t *sl);
#ifdef __cplusplus
};
#endif
#endif