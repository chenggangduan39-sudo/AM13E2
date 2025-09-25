#ifndef WTK_BFIO_MASKDENOISE_WTK_GAINNET_DENOISE_CFG
#define WTK_BFIO_MASKDENOISE_WTK_GAINNET_DENOISE_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/bfio/masknet/wtk_gainnet7.h"
#include "wtk/bfio/masknet/wtk_gainnet3.h"
#include "wtk/bfio/qform/wtk_qmmse.h"
#include "wtk/bfio/maskdenoise/wtk_bankfeat.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_gainnet_denoise_cfg wtk_gainnet_denoise_cfg_t;
struct wtk_gainnet_denoise_cfg
{
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;
    int wins;
    int rate;

    wtk_bankfeat_cfg_t bankfeat;

    float agc_a;
    float agc_b;

    float ralpha;
    float gbias;

    char *mdl_fn;
    wtk_gainnet7_cfg_t *gainnet;
    wtk_gainnet3_cfg_t *gainnet3;

    wtk_qmmse_cfg_t qmmse;

    unsigned use_rbin_res:1;
    unsigned use_gainnet3:1;
    unsigned use_qmmse:1;
};

int wtk_gainnet_denoise_cfg_init(wtk_gainnet_denoise_cfg_t *cfg);
int wtk_gainnet_denoise_cfg_clean(wtk_gainnet_denoise_cfg_t *cfg);
int wtk_gainnet_denoise_cfg_update_local(wtk_gainnet_denoise_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_gainnet_denoise_cfg_update(wtk_gainnet_denoise_cfg_t *cfg);
int wtk_gainnet_denoise_cfg_update2(wtk_gainnet_denoise_cfg_t *cfg,wtk_source_loader_t *sl);
wtk_gainnet_denoise_cfg_t* wtk_gainnet_denoise_cfg_new(char *fn);
void wtk_gainnet_denoise_cfg_delete(wtk_gainnet_denoise_cfg_t *cfg);
wtk_gainnet_denoise_cfg_t* wtk_gainnet_denoise_cfg_new_bin(char *fn);
void wtk_gainnet_denoise_cfg_delete_bin(wtk_gainnet_denoise_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
