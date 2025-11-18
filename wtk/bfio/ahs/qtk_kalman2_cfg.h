#ifndef WTK_BFIO_AHS_QTK_KALMAN2_CFG
#define WTK_BFIO_AHS_QTK_KALMAN2_CFG
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/bfio/ahs/qtk_freq_shift.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_ahs_kalman2_cfg qtk_ahs_kalman2_cfg_t;

struct qtk_ahs_kalman2_cfg {
    int B;
    int L;
    int M;
    float alpha;
    float Phi_SS_smooth_factor;
    float p_initial;
    char *wb_fn;
    wtk_strbuf_t *wb_buf;
    qtk_ahs_freq_shift_cfg_t freq_shift;
    float update_threshold;
    unsigned use_res:1;
    unsigned use_se:1;
    unsigned use_fs:1;
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