#ifndef WTK_BFIO_WTK_MASKSSL2_CFG
#define WTK_BFIO_WTK_MASKSSL2_CFG
#include "wtk/core/cfg/wtk_main_cfg.h" 
#include "wtk/core/cfg/wtk_mbin_cfg.h" 
#include "wtk/bfio/aspec/wtk_maskaspec.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_maskssl2_cfg wtk_maskssl2_cfg_t;
struct wtk_maskssl2_cfg
{
    int wins;
    int rate;
    wtk_maskaspec_cfg_t maskaspec;

    int theta_step;
    int phi_step;

    int max_theta;
    int min_theta;
    int max_phi;
    int min_phi;

    int max_extp;
    int min_thetasub;

    float specsum_fs;
    float specsum_fe;
    int specsum_ns;
    int specsum_ne;
    float specsum_thresh;
    float specsum_thresh2;

    int online_frame;
    int online_frame_step;

    void *hook;

    float smooth_theta;
    int smooth_cnt;
    int delay_cnt;

    int freq_skip;

    unsigned use_line:1;
    unsigned use_smooth:1;
    unsigned use_sil_delay:1;
    unsigned use_each_comp:1;
    unsigned use_simple_phi:1;
    unsigned use_spec_k2:1;
};

int wtk_maskssl2_cfg_init(wtk_maskssl2_cfg_t *cfg);
int wtk_maskssl2_cfg_clean(wtk_maskssl2_cfg_t *cfg);
int wtk_maskssl2_cfg_update_local(wtk_maskssl2_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_maskssl2_cfg_update(wtk_maskssl2_cfg_t *cfg);
int wtk_maskssl2_cfg_update2(wtk_maskssl2_cfg_t *cfg,wtk_source_loader_t *sl);

#ifdef __cplusplus
};
#endif
#endif