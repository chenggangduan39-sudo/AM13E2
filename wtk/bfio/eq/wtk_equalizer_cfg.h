#ifndef WTK_EQUALIZER_CFG_H
#define WTK_EQUALIZER_CFG_H

#include "wtk/core/cfg/wtk_local_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EQ_MAX_BANDS 15
#define EQ_MIN_BANDS 10

typedef struct wtk_equalizer_cfg{
    int band_count;
    float *rate;
    float *value;
    float octave;
    float sfreq;
    unsigned int extra_filter:1;
}wtk_equalizer_cfg_t;

int wtk_equalizer_cfg_init(wtk_equalizer_cfg_t *cfg);
int wtk_equalizer_cfg_clean(wtk_equalizer_cfg_t *cfg);
int wtk_equalizer_cfg_update_local(wtk_equalizer_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_equalizer_cfg_update(wtk_equalizer_cfg_t *cfg);

#ifdef __cplusplus
};
#endif

#endif