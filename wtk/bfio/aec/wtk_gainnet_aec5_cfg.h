#ifndef WTK_BFIO_AEC_WTK_gainnet_aec5_CFG
#define WTK_BFIO_AEC_WTK_gainnet_aec5_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/bfio/masknet/wtk_gainnet5.h"
#include "wtk/bfio/masknet/wtk_gainnet2.h"
#include "wtk/bfio/masknet/wtk_gainnet6.h"
#include "wtk/bfio/afilter/wtk_nlms.h"
#include "wtk/bfio/afilter/wtk_rls.h"
#include "wtk/bfio/ahs/qtk_kalman_cfg.h"
#include "wtk/bfio/qform/wtk_bf.h"
#include "wtk/bfio/qform/wtk_covm.h"
#include "wtk/bfio/qform/wtk_qmmse.h"
#include "wtk/bfio/maskdenoise/wtk_bankfeat.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_gainnet_aec5_cfg wtk_gainnet_aec5_cfg_t;
struct wtk_gainnet_aec5_cfg
{
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;

	int wins;

    int rate;

	wtk_rls_cfg_t echo_rls;
    wtk_nlms_cfg_t echo_nlms;
    qtk_ahs_kalman_cfg_t echo_kalman;

    wtk_bankfeat_cfg_t bankfeat;

    float spenr_thresh;
    int spenr_cnt;

    char *mdl_fn;
	wtk_gainnet5_cfg_t *gainnet;
    wtk_gainnet2_cfg_t *gainnet2;
    wtk_gainnet6_cfg_t *gainnet6;

    wtk_qmmse_cfg_t qmmse;

    float gbias;
    float ralpha;

    float agc_a;
    float agc_b;

	int nmicchannel;
	int nspchannel;

    int M;
    float power_alpha;
    float mufb;
    int nlms_ms;
    int nlms_nframe;

    unsigned use_rbin_res:1;
    unsigned use_preemph:1;
    unsigned use_epostsingle:1;
    unsigned use_qmmse:1;
    unsigned use_gainnet2:1;
    unsigned use_gainnet6:1;
    unsigned use_maxleak:1;
    unsigned use_nlmsdelay:1;
    unsigned use_nlms:1;
    unsigned use_rls:1;
    unsigned use_kalman:1;
};

int wtk_gainnet_aec5_cfg_init(wtk_gainnet_aec5_cfg_t *cfg);
int wtk_gainnet_aec5_cfg_clean(wtk_gainnet_aec5_cfg_t *cfg);
int wtk_gainnet_aec5_cfg_update_local(wtk_gainnet_aec5_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_gainnet_aec5_cfg_update(wtk_gainnet_aec5_cfg_t *cfg);
int wtk_gainnet_aec5_cfg_update2(wtk_gainnet_aec5_cfg_t *cfg,wtk_source_loader_t *sl);

wtk_gainnet_aec5_cfg_t* wtk_gainnet_aec5_cfg_new(char *fn);
void wtk_gainnet_aec5_cfg_delete(wtk_gainnet_aec5_cfg_t *cfg);
wtk_gainnet_aec5_cfg_t* wtk_gainnet_aec5_cfg_new_bin(char *fn);
void wtk_gainnet_aec5_cfg_delete_bin(wtk_gainnet_aec5_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
