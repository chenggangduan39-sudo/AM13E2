#ifndef WTK_BFIO_SOUNDFIELD_SYNTHEIS_CFG
#define WTK_BFIO_SOUNDFIELD_SYNTHEIS_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/math/wtk_math.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_soundfield_syntheis_cfg qtk_soundfield_syntheis_cfg_t;
struct qtk_soundfield_syntheis_cfg
{
    float pw_angle;
    float array_spacing;
    int hop_size;
    int fs;
    float *window;
    float *win_gain;
    int N;
    float center[3];
    float scale;
};

int qtk_soundfield_syntheis_cfg_init(qtk_soundfield_syntheis_cfg_t *cfg);
int qtk_soundfield_syntheis_cfg_clean(qtk_soundfield_syntheis_cfg_t *cfg);
int qtk_soundfield_syntheis_cfg_update_local(qtk_soundfield_syntheis_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_soundfield_syntheis_cfg_update(qtk_soundfield_syntheis_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif