#ifndef WTK_BFIO_MASKASPEC_WTK_MASKASPEC_CFG
#define WTK_BFIO_MASKASPEC_WTK_MASKASPEC_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_maskaspec_cfg wtk_maskaspec_cfg_t;
struct wtk_maskaspec_cfg
{
    int rate;
	float speed;
	int channel;
	float **mic_pos;
	float **mic_pos2;
	float th_step;

	float ls_eye;

	unsigned use_maskmvdr:1;
	unsigned use_maskds:1;
	unsigned use_maskgcc:1;
	unsigned use_maskzds:1;
	unsigned use_line:1;
	unsigned use_mic2:1;
};

int wtk_maskaspec_cfg_init(wtk_maskaspec_cfg_t *cfg);
int wtk_maskaspec_cfg_clean(wtk_maskaspec_cfg_t *cfg);
int wtk_maskaspec_cfg_update_local(wtk_maskaspec_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_maskaspec_cfg_update(wtk_maskaspec_cfg_t *cfg);
int wtk_maskaspec_cfg_update2(wtk_maskaspec_cfg_t *cfg,wtk_source_loader_t *sl);

#ifdef __cplusplus
};
#endif
#endif