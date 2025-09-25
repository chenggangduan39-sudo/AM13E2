#ifndef WTK_BFIO_MASKFORM_WTK_GAINNET_BF5_CFG
#define WTK_BFIO_MASKFORM_WTK_GAINNET_BF5_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/bfio/masknet/wtk_gainnet.h"
#include "wtk/bfio/qform/wtk_bf.h"
#include "wtk/bfio/qform/wtk_covm.h"
#include "wtk/bfio/qform/wtk_qmmse.h"
#include "wtk/bfio/aspec/wtk_aspec.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_gainnet_bf5_cfg wtk_gainnet_bf5_cfg_t;
struct wtk_gainnet_bf5_cfg
{
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;

	int wins;
    int rate;
    float speed;
    
    wtk_qmmse_cfg_t qmmse;

	int nb_bands;  
    int ceps_mem;
    int nb_delta_ceps;

    int nb_features;
    int nb_features_x;

    char *mdl_fn;
	wtk_gainnet_cfg_t *gainnet;

    float agc_a;
    float agc_b;

	int channel;
	float **mic_pos;

    int specsum_fs;
    int specsum_fe;

    float lds_eye;

    unsigned use_rbin_res:1;
    unsigned use_preemph:1;
    unsigned use_ceps:1;
    unsigned use_qmmse:1;
    unsigned use_line:1;
};

int wtk_gainnet_bf5_cfg_init(wtk_gainnet_bf5_cfg_t *cfg);
int wtk_gainnet_bf5_cfg_clean(wtk_gainnet_bf5_cfg_t *cfg);
int wtk_gainnet_bf5_cfg_update_local(wtk_gainnet_bf5_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_gainnet_bf5_cfg_update(wtk_gainnet_bf5_cfg_t *cfg);
int wtk_gainnet_bf5_cfg_update2(wtk_gainnet_bf5_cfg_t *cfg,wtk_source_loader_t *sl);

wtk_gainnet_bf5_cfg_t* wtk_gainnet_bf5_cfg_new(char *fn);
void wtk_gainnet_bf5_cfg_delete(wtk_gainnet_bf5_cfg_t *cfg);
wtk_gainnet_bf5_cfg_t* wtk_gainnet_bf5_cfg_new_bin(char *fn);
void wtk_gainnet_bf5_cfg_delete_bin(wtk_gainnet_bf5_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
