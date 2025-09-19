#ifndef WTK_BFIO_WTK_MASKSSL_CFG
#define WTK_BFIO_WTK_MASKSSL_CFG
#include "wtk/core/cfg/wtk_main_cfg.h" 
#include "wtk/core/cfg/wtk_mbin_cfg.h" 
#include "wtk/bfio/aspec/wtk_maskaspec.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_maskssl_cfg wtk_maskssl_cfg_t;
struct wtk_maskssl_cfg
{
    int wins;
    int rate;
    wtk_maskaspec_cfg_t maskaspec;

    int theta_step;
    int phi_step;

    int max_theta;
    int max_phi;

    int max_extp;
    int min_thetasub;

    float specsum_fs;
    float specsum_fe;
    int specsum_ns;
    int specsum_ne;
    float specsum_thresh;

    int online_tms;
    int online_frame;

    void *hook;

    unsigned use_line:1;
    unsigned use_online:1;
};

int wtk_maskssl_cfg_init(wtk_maskssl_cfg_t *cfg);
int wtk_maskssl_cfg_clean(wtk_maskssl_cfg_t *cfg);
int wtk_maskssl_cfg_update_local(wtk_maskssl_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_maskssl_cfg_update(wtk_maskssl_cfg_t *cfg);
int wtk_maskssl_cfg_update2(wtk_maskssl_cfg_t *cfg,wtk_source_loader_t *sl);

#ifdef __cplusplus
};
#endif
#endif