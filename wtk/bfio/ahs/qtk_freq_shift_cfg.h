#ifndef WTK_BFIO_AHS_QTK_FREQ_SHIFT_CFG
#define WTK_BFIO_AHS_QTK_FREQ_SHIFT_CFG
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_ahs_freq_shift_cfg qtk_ahs_freq_shift_cfg_t;

struct qtk_ahs_freq_shift_cfg {
    int N_tap;
    int freq_shift;
};

int qtk_ahs_freq_shift_cfg_init(qtk_ahs_freq_shift_cfg_t *cfg);
int qtk_ahs_freq_shift_cfg_clean(qtk_ahs_freq_shift_cfg_t *cfg);
int qtk_ahs_freq_shift_cfg_update_local(qtk_ahs_freq_shift_cfg_t *cfg, wtk_local_cfg_t *lc);
int qtk_ahs_freq_shift_cfg_update(qtk_ahs_freq_shift_cfg_t *cfg);
int qtk_ahs_freq_shift_cfg_update2(qtk_ahs_freq_shift_cfg_t *cfg, wtk_source_loader_t *sl);

#ifdef __cplusplus
};
#endif
#endif