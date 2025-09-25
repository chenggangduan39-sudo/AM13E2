#ifndef WTK_CORE_FFT_WTK_RFFT2_CFG
#define WTK_CORE_FFT_WTK_RFFT2_CFG
#include "wtk/core/cfg/wtk_local_cfg.h" 
#include "wtk/core/math/wtk_math.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_rfft2_cfg wtk_rfft2_cfg_t;
struct wtk_rfft2_cfg
{
	float *winf;
	char *wint;	//ham;hann;conj;sine
	int win;
	int step;
	float overlap;
};

int wtk_rfft2_cfg_init(wtk_rfft2_cfg_t *cfg);
int wtk_rfft2_cfg_clean(wtk_rfft2_cfg_t *cfg);
int wtk_rfft2_cfg_update_local(wtk_rfft2_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_rfft2_cfg_update(wtk_rfft2_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
