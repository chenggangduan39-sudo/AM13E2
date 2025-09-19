#ifndef WTK_BFIO_DEREVERB_WTK_ADMM_CFG
#define WTK_BFIO_DEREVERB_WTK_ADMM_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/fft/wtk_stft2.h"
#include "wtk/bfio/afilter/wtk_rls.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_admm_cfg wtk_admm_cfg_t;
struct wtk_admm_cfg
{
    wtk_stft2_cfg_t stft;
    wtk_rls_cfg_t rls;    
	int D;
	int L;
};

int wtk_admm_cfg_init(wtk_admm_cfg_t *cfg);
int wtk_admm_cfg_clean(wtk_admm_cfg_t *cfg);
int wtk_admm_cfg_update_local(wtk_admm_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_admm_cfg_update(wtk_admm_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif