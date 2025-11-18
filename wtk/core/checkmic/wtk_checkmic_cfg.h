#ifndef WTK_CHECKMIC_WTK_CHECKMIC_CFG
#define WTK_CHECKMIC_WTK_CHECKMIC_CFG
#include "wtk/asr/vad/wtk_vad.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_checkmic_cfg wtk_checkmic_cfg_t;

struct wtk_checkmic_cfg
{
    float **mic_pos;
    int channel;
    int rate;

};

int wtk_checkmic_cfg_init(wtk_checkmic_cfg_t *cfg);
int wtk_checkmic_cfg_clean(wtk_checkmic_cfg_t *cfg);
int wtk_checkmic_cfg_update_local(wtk_checkmic_cfg_t *cfg, wtk_local_cfg_t *lc);
int wtk_checkmic_cfg_update(wtk_checkmic_cfg_t *cfg);


#ifdef __cplusplus
};
#endif
#endif