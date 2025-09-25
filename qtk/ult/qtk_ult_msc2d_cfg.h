#ifndef F6905CE9_5A5B_41B4_89AD_3D8BB60ADF26
#define F6905CE9_5A5B_41B4_89AD_3D8BB60ADF26

#include "wtk/core/cfg/wtk_local_cfg.h"

typedef struct qtk_ult_msc2d_cfg qtk_ult_msc2d_cfg_t;

struct qtk_ult_msc2d_cfg {
    int nsensor;
    int nmic;
    int lag;
    int nzc;
    int num_tgt;
    float xcorr_alpha;
    unsigned xcorr_use_alpha : 1;
    float *mic;

    unsigned use_fft : 1;
};

int qtk_ult_msc2d_cfg_init(qtk_ult_msc2d_cfg_t *cfg);
int qtk_ult_msc2d_cfg_clean(qtk_ult_msc2d_cfg_t *cfg);
int qtk_ult_msc2d_cfg_update(qtk_ult_msc2d_cfg_t *cfg);
int qtk_ult_msc2d_cfg_update_local(qtk_ult_msc2d_cfg_t *cfg,
                                   wtk_local_cfg_t *lc);

#endif /* F6905CE9_5A5B_41B4_89AD_3D8BB60ADF26 */
