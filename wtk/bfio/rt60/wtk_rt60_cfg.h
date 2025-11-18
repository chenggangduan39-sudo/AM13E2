#ifndef WTK_BFIO_RT60_WTK_RT60_CFG
#define WTK_BFIO_RT60_WTK_RT60_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/core/math/wtk_math.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_rt60_cfg wtk_rt60_cfg_t;
struct wtk_rt60_cfg {
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;

    int wins;
    int rate;

    int channel;
    int *mic_channel;
    int nmicchannel;

    float f1;
    float f2;
    float t;
    float amp;

    short *play_sweep;
    float *sweep;
    float *inv_sweep;
    int sweep_len;
    
};

int wtk_rt60_cfg_init(wtk_rt60_cfg_t *cfg);
int wtk_rt60_cfg_clean(wtk_rt60_cfg_t *cfg);
int wtk_rt60_cfg_update_local(wtk_rt60_cfg_t *cfg, wtk_local_cfg_t *lc);
int wtk_rt60_cfg_update(wtk_rt60_cfg_t *cfg);
int wtk_rt60_cfg_update2(wtk_rt60_cfg_t *cfg, wtk_source_loader_t *sl);

wtk_rt60_cfg_t *wtk_rt60_cfg_new(char *fn);
void wtk_rt60_cfg_delete(wtk_rt60_cfg_t *cfg);
wtk_rt60_cfg_t *wtk_rt60_cfg_new_bin(char *fn);
void wtk_rt60_cfg_delete_bin(wtk_rt60_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
