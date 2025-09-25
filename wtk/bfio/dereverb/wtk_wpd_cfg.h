#ifndef WTK_BFIO_DEREVERB_WTK_WPD_CFG
#define WTK_BFIO_DEREVERB_WTK_WPD_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/bfio/qform/wtk_covm2.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_wpd_cfg wtk_wpd_cfg_t;
struct wtk_wpd_cfg
{
	float speed;
	int rate;
	
	int nmic;
	float **mic_pos;
	
	int L;

    float sigma;
    float p;

	wtk_covm2_cfg_t covm2;
};

int wtk_wpd_cfg_init(wtk_wpd_cfg_t *cfg);
int wtk_wpd_cfg_clean(wtk_wpd_cfg_t *cfg);
int wtk_wpd_cfg_update_local(wtk_wpd_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_wpd_cfg_update(wtk_wpd_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif