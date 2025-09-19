#ifndef WTK_KSR_WAKEUP_WTK_KWAKE_POST_RF_CFG
#define WTK_KSR_WAKEUP_WTK_KWAKE_POST_RF_CFG
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/core/wtk_robin.h"
#include "wtk/core/math/wtk_mat.h"
#include "wtk/core/wtk_fixpoint.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_kwake_post_rf_cfg wtk_kwake_post_rf_cfg_t;

struct wtk_kwake_post_rf_cfg
{
    int rf_win_left_pad;
    int rf_win_right_pad;
    float rf_thresh;
	int *state_array;
	int state_cnt;
    float bkg_sil_ratio;
    int win;
};

int wtk_kwake_post_rf_cfg_init(wtk_kwake_post_rf_cfg_t *cfg);
int wtk_kwake_post_rf_cfg_clean(wtk_kwake_post_rf_cfg_t *cfg);
int wtk_kwake_post_rf_cfg_update(wtk_kwake_post_rf_cfg_t *cfg);
int wtk_kwake_post_rf_cfg_update_local(wtk_kwake_post_rf_cfg_t *cfg, wtk_local_cfg_t *lc);

#ifdef __cplusplus
};
#endif
#endif