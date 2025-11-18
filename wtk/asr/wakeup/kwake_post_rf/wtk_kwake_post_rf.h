#ifndef WTK_KSR_WAKEUP_WTK_KWAKE_POST_RF
#define WTK_KSR_WAKEUP_WTK_KWAKE_POST_RF
#include "wtk_kwake_post_rf_cfg.h"
#include "wtk_kwake_post_rf_mdl.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_kwake_post_rf_inst wtk_kwake_post_rf_inst_t;

struct wtk_kwake_post_rf_inst
{
    wtk_kwake_post_rf_cfg_t *cfg;
    float *state;
	wtk_mati_t *mat_path;
	int *path;
	float *path_prob;
	int rf_s;
	int rf_e;
    int sil_idx;
    int bkg_idx;
    float thresh;
};

wtk_kwake_post_rf_inst_t* wtk_kwake_post_rf_inst_new(wtk_kwake_post_rf_cfg_t* cfg);
int wtk_kwake_post_rf_inst_delete(wtk_kwake_post_rf_inst_t *rf);
void wtk_kwake_post_rf_inst_reset2(wtk_kwake_post_rf_inst_t *rf);
int wtk_kwake_check_post_rf(wtk_kwake_post_rf_inst_t *rf,wtk_robin_t *rb,int* wake_slot,int n);
int wtk_kwake_check_post_rf_fix(wtk_kwake_post_rf_inst_t *rf,wtk_robin_t *rb,int* wake_slot,int n,int shift);


#ifdef __cplusplus
};
#endif
#endif