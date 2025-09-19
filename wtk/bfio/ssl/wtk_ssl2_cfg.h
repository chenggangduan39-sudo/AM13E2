#ifndef WTK_BFIO_SSL_WTK_SSL2_CFG
#define WTK_BFIO_SSL_WTK_SSL2_CFG
#include "wtk/core/fft/wtk_stft2.h"
#include "wtk/bfio/aspec/wtk_aspec.h" 
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_ssl2_cfg wtk_ssl2_cfg_t;
struct wtk_ssl2_cfg
{
    int wins;
    wtk_aspec_cfg_t aspec;

    int theta_step;
    int phi_step;

    int max_extp;
    int min_thetasub;

    float specsum_fs;
    float specsum_fe;
    int specsum_ns;
    int specsum_ne;
    float specsum_thresh;

    int lf;
    int lt;

    int max_theta;
    int max_phi;

    int rate;

    int online_tms;
    int online_frame;

    unsigned use_line:1;
    unsigned use_online:1;
};

int wtk_ssl2_cfg_init(wtk_ssl2_cfg_t *cfg);
int wtk_ssl2_cfg_clean(wtk_ssl2_cfg_t *cfg);
int wtk_ssl2_cfg_update_local(wtk_ssl2_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_ssl2_cfg_update(wtk_ssl2_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif