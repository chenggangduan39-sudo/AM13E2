#ifndef WTK_BFIO_RESAMPLE_WTK_RESAMPLE2_CFG
#define WTK_BFIO_RESAMPLE_WTK_RESAMPLE2_CFG
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_resample2_cfg wtk_resample2_cfg_t;

struct wtk_resample2_cfg {
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;

    int SR;
    int new_SR;
    unsigned int is_upsample:1;
    int upsamp_rate;
    int downsamp_rate;
    int frm_size;
    int weight_length;
    int window_sz;
    int taps;
    float beta;
    float cutoff_ratio;
};


int wtk_resample2_cfg_init(wtk_resample2_cfg_t *cfg);
int wtk_resample2_cfg_clean(wtk_resample2_cfg_t *cfg);
int wtk_resample2_cfg_update_local(wtk_resample2_cfg_t *cfg, wtk_local_cfg_t *lc);
int wtk_resample2_cfg_update(wtk_resample2_cfg_t *cfg);
int wtk_resample2_cfg_update2(wtk_resample2_cfg_t *cfg, wtk_source_loader_t *sl);

wtk_resample2_cfg_t *wtk_resample2_cfg_new(char *fn);
void wtk_resample2_cfg_delete(wtk_resample2_cfg_t *cfg);
wtk_resample2_cfg_t* wtk_resample2_cfg_new_bin(char *fn);
void wtk_resample2_cfg_delete_bin(wtk_resample2_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
