#ifndef WTK_BFIO_AHS_QTK_KALMAN_CFG
#define WTK_BFIO_AHS_QTK_KALMAN_CFG
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_ahs_kalman_cfg qtk_ahs_kalman_cfg_t;

struct qtk_ahs_kalman_cfg {
    float kalman_thresh;
    float pworg;
    float pvorg;
    float kalman_a;
    float gamma;
    float Phi_SS_smooth_factor;
    int kalman_order;
    int kalman_type;
    int L;
    int dg_row;
    int dg_col;
    unsigned use_dg:1;
    unsigned use_residual_cancellation:1;
    unsigned use_symmetric_ph:1;
    unsigned use_res:1;
};

int qtk_ahs_kalman_cfg_init(qtk_ahs_kalman_cfg_t *cfg);
int qtk_ahs_kalman_cfg_clean(qtk_ahs_kalman_cfg_t *cfg);
int qtk_ahs_kalman_cfg_update_local(qtk_ahs_kalman_cfg_t *cfg, wtk_local_cfg_t *lc);
int qtk_ahs_kalman_cfg_update(qtk_ahs_kalman_cfg_t *cfg);
int qtk_ahs_kalman_cfg_update2(qtk_ahs_kalman_cfg_t *cfg, wtk_source_loader_t *sl);

#ifdef __cplusplus
};
#endif
#endif