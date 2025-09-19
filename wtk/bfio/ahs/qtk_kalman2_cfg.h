#ifndef WTK_BFIO_AHS_QTK_KALMAN2_CFG
#define WTK_BFIO_AHS_QTK_KALMAN2_CFG
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_ahs_kalman2_cfg qtk_ahs_kalman2_cfg_t;

struct qtk_ahs_kalman2_cfg {
    int B;
    int nbin;
    float alpha;
    float keep_m_gate;
    float p_initial;
    unsigned use_res:1;
};

int qtk_ahs_kalman2_cfg_init(qtk_ahs_kalman2_cfg_t *cfg);
int qtk_ahs_kalman2_cfg_clean(qtk_ahs_kalman2_cfg_t *cfg);
int qtk_ahs_kalman2_cfg_update_local(qtk_ahs_kalman2_cfg_t *cfg, wtk_local_cfg_t *lc);
int qtk_ahs_kalman2_cfg_update(qtk_ahs_kalman2_cfg_t *cfg);
int qtk_ahs_kalman2_cfg_update2(qtk_ahs_kalman2_cfg_t *cfg, wtk_source_loader_t *sl);

#ifdef __cplusplus
};
#endif
#endif