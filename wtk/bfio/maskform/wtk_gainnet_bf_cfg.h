#ifndef WTK_BFIO_MASKFORM_WTK_GAINNET_BF_CFG
#define WTK_BFIO_MASKFORM_WTK_GAINNET_BF_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/bfio/masknet/wtk_gainnet.h"
#include "wtk/bfio/qform/wtk_qenvelope.h"
#include "wtk/bfio/ssl/wtk_ssl2.h"
#include "wtk/bfio/qform/wtk_qmmse.h"
#include "wtk/bfio/qform/wtk_bf.h"
#include "wtk/bfio/qform/wtk_covm.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_gainnet_bf_cfg wtk_gainnet_bf_cfg_t;
struct wtk_gainnet_bf_cfg
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

    int agc_nb_features;
    char *agc_mdl_fn;
	wtk_gainnet_cfg_t *agc_gainnet;
    float agc_a;
    float agc_b;

    wtk_qenvelope_cfg_t qenvl;
    wtk_ssl2_cfg_t ssl2;

    wtk_covm_cfg_t covm;
    wtk_bf_cfg_t bf;

    float theta;
    float phi;     

    float pframe_thresh;
    float pframe_alpha;

    unsigned use_rbin_res:1;
    unsigned use_pitchpost:1;
    unsigned use_denoise_single:1;
    unsigned use_ssl:1;
    unsigned use_qmmse:1;
    unsigned use_agc:1;
};

int wtk_gainnet_bf_cfg_init(wtk_gainnet_bf_cfg_t *cfg);
int wtk_gainnet_bf_cfg_clean(wtk_gainnet_bf_cfg_t *cfg);
int wtk_gainnet_bf_cfg_update_local(wtk_gainnet_bf_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_gainnet_bf_cfg_update(wtk_gainnet_bf_cfg_t *cfg);
int wtk_gainnet_bf_cfg_update2(wtk_gainnet_bf_cfg_t *cfg,wtk_source_loader_t *sl);
#ifdef __cplusplus
};
#endif
#endif
