#ifndef WTK_BFIO_SWEETSPOT_CFG
#define WTK_BFIO_SWEETSPOT_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_sweetspot_cfg qtk_sweetspot_cfg_t;
struct qtk_sweetspot_cfg
{
    float speaker_distance;
    float gain;

    int hop_size;
    int max_N_delay;
    float *window;
    float *win_gain;
};

int qtk_sweetspot_cfg_init(qtk_sweetspot_cfg_t *cfg);
int qtk_sweetspot_cfg_clean(qtk_sweetspot_cfg_t *cfg);
int qtk_sweetspot_cfg_update_local(qtk_sweetspot_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_sweetspot_cfg_update(qtk_sweetspot_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif