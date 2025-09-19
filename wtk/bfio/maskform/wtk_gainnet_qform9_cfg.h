#ifndef WTK_BFIO_MASKFORM_WTK_GAINNET_QFORM9_CFG
#define WTK_BFIO_MASKFORM_WTK_GAINNET_QFORM9_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/bfio/aspec/wtk_aspec.h"
#include "wtk/bfio/qform/wtk_bf.h"
#include "wtk/core/fft/wtk_stft2.h"
#include "wtk/bfio/qform/wtk_qmmse.h"
#include "wtk/bfio/qform/wtk_covm.h"
#include "wtk/bfio/qform/wtk_qenvelope.h"
#include "wtk/bfio/masknet/wtk_gainnet2.h"
#include "wtk/bfio/maskdenoise/wtk_bankfeat.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_gainnet_qform9_cfg wtk_gainnet_qform9_cfg_t;
struct wtk_gainnet_qform9_cfg
{
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;

    wtk_stft2_cfg_t stft2;
    int rate;

    wtk_aspec_cfg_t aspec;

    float *mic_class[2];

    wtk_covm_cfg_t covm;

    float ralpha;
    float gbias;
    wtk_bf_cfg_t bf;
    int featm_lm;

    int clip_s;
    int clip_e;
    int theta_range;

    int theta_range_class;
    int theta_range_class1;
    int theta_range_class2;
    float theta_center_class1;
    float theta_center_class2;
    float theta_center_class3;

    int lf;
    int lt;

    float specsum_fs;
    float specsum_fe;
    int specsum_ns;
    int specsum_ne;

    float cohv_thresh;

    wtk_qenvelope_cfg_t qenvl;
    wtk_qenvelope_cfg_t qenvl2;
    wtk_qenvelope_cfg_t qenvl3;

    wtk_qmmse_cfg_t qmmse;
    wtk_qmmse_cfg_t mdl_qmmse;

    char *mdl_fn;
    wtk_gainnet2_cfg_t *gainnet2;
    wtk_bankfeat_cfg_t bankfeat;

    float **t_r_qenvl;
    int t_r_number;
    float qenvel_alpha;

    unsigned use_line:1;
    unsigned use_post:1;
    unsigned use_cline1:1;
    unsigned use_cline2:1;

    unsigned use_qenvelope:1;
    unsigned use_sqenvelope:1;
    unsigned use_t_r_qenvelope:1;
    unsigned use_simple_qenvelope:1;
    unsigned use_noise_qenvelope:1;
    unsigned use_preemph:1;
    unsigned use_two_channel:1;
    unsigned debug:1;
    unsigned use_rbin_res:1;
    unsigned use_qmmse:1;
};

int wtk_gainnet_qform9_cfg_init(wtk_gainnet_qform9_cfg_t *cfg);
int wtk_gainnet_qform9_cfg_clean(wtk_gainnet_qform9_cfg_t *cfg);
int wtk_gainnet_qform9_cfg_update_local(wtk_gainnet_qform9_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_gainnet_qform9_cfg_update(wtk_gainnet_qform9_cfg_t *cfg);
int wtk_gainnet_qform9_cfg_update2(wtk_gainnet_qform9_cfg_t *cfg,wtk_source_loader_t *sl);

wtk_gainnet_qform9_cfg_t* wtk_gainnet_qform9_cfg_new(char *cfg_fn);
void wtk_gainnet_qform9_cfg_delete(wtk_gainnet_qform9_cfg_t *cfg);
wtk_gainnet_qform9_cfg_t* wtk_gainnet_qform9_cfg_new_bin(char *bin_fn);
void wtk_gainnet_qform9_cfg_delete_bin(wtk_gainnet_qform9_cfg_t *cfg);

void wtk_gainnet_qform9_cfg_set_theta_range(wtk_gainnet_qform9_cfg_t *cfg,float theta_range);
void wtk_gainnet_qform9_cfg_set_noise_suppress(wtk_gainnet_qform9_cfg_t *cfg,float noise_suppress);
#ifdef __cplusplus
};
#endif
#endif
