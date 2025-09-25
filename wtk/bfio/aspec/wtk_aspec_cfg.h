#ifndef WTK_BFIO_ASPEC_WTK_ASPEC_CFG
#define WTK_BFIO_ASPEC_WTK_ASPEC_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_aspec_cfg wtk_aspec_cfg_t;
struct wtk_aspec_cfg
{
    int rate;
	float speed;
	int channel;
	float **mic_pos;

    int pairs_n;
    float **pairs_mic;

    float ls_eye;
    unsigned use_gccspec:1;
    unsigned use_ml:1;
    unsigned use_gccspec2:1;
    unsigned use_ngccspec2:1;
    unsigned use_dnmspec:1;
    unsigned use_dnmspec2:1;
    unsigned use_mvdrspec:1;
    unsigned use_mvdrspec2:1;
    unsigned use_mvdrwspec:1;
    unsigned use_mvdrwspec2:1;
    unsigned use_dsspec:1;
    unsigned use_dsspec2:1;
    unsigned use_dswspec:1;
    unsigned use_dswspec2:1;
    unsigned use_musicspec2:1;
    unsigned use_zdsspec:1;
    unsigned use_zdswspec:1;
    unsigned use_quick:1;
    unsigned use_fftnbinfirst:1;
    unsigned use_line:1;
};

int wtk_aspec_cfg_init(wtk_aspec_cfg_t *cfg);
int wtk_aspec_cfg_clean(wtk_aspec_cfg_t *cfg);
int wtk_aspec_cfg_update_local(wtk_aspec_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_aspec_cfg_update(wtk_aspec_cfg_t *cfg);
int wtk_aspec_cfg_update2(wtk_aspec_cfg_t *cfg,wtk_source_loader_t *sl);

#ifdef __cplusplus
};
#endif
#endif
