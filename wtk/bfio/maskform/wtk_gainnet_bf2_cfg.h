#ifndef WTK_BFIO_MASKFORM_WTK_GAINNET_BF2_CFG
#define WTK_BFIO_MASKFORM_WTK_GAINNET_BF2_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/bfio/masknet/wtk_gainnet.h"
#include "wtk/bfio/qform/wtk_qenvelope.h"
#include "wtk/bfio/ssl/wtk_ssl2.h"
#include "wtk/bfio/qform/wtk_qmmse.h"
#include "wtk/bfio/dereverb/wtk_wpd.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_gainnet_bf2_cfg wtk_gainnet_bf2_cfg_t;
struct wtk_gainnet_bf2_cfg
{
    int channel;
	int wins;
    int rate;

    int pitch_min_period;
    int pitch_max_period;
    int pitch_frame_size;
    int pitch_buf_size;

    int nb_bands;  
    int ceps_mem;
    int nb_delta_ceps;

    int nb_features;
    int *eband;

    char *mdl_fn;
    wtk_gainnet_cfg_t *gainnet;
    wtk_qmmse_cfg_t qmmse;

    wtk_qenvelope_cfg_t qenvl;
    wtk_ssl2_cfg_t ssl2;

    wtk_wpd_cfg_t wpd;

    float theta;
    float phi;

    unsigned use_rbin_res:1;
    unsigned use_pitchpost:1;
    unsigned use_denoise_single:1;
    unsigned use_ssl:1;
    unsigned use_qmmse:1;
};

int wtk_gainnet_bf2_cfg_init(wtk_gainnet_bf2_cfg_t *cfg);
int wtk_gainnet_bf2_cfg_clean(wtk_gainnet_bf2_cfg_t *cfg);
int wtk_gainnet_bf2_cfg_update_local(wtk_gainnet_bf2_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_gainnet_bf2_cfg_update(wtk_gainnet_bf2_cfg_t *cfg);
int wtk_gainnet_bf2_cfg_update2(wtk_gainnet_bf2_cfg_t *cfg,wtk_source_loader_t *sl);
#ifdef __cplusplus
};
#endif
#endif
