#ifndef WTK_BFIO_MASKFORM_WTK_GAINNET_BF6_CFG
#define WTK_BFIO_MASKFORM_WTK_GAINNET_BF6_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/bfio/masknet/wtk_gainnet2.h"
#include "wtk/bfio/masknet/wtk_masknet.h"
#include "wtk/bfio/qform/wtk_bf.h"
#include "wtk/bfio/qform/wtk_covm.h"
#include "wtk/bfio/qform/wtk_qmmse.h"
#include "wtk/bfio/aspec/wtk_aspec.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_gainnet_bf6_cfg wtk_gainnet_bf6_cfg_t;
struct wtk_gainnet_bf6_cfg
{
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;

	int wins;
    int rate;

	int nb_bands;  
    int ceps_mem;
    int nb_delta_ceps;

    int nb_features_x;
    int nb_features;

    float micnenr;

    char *mdl_fn;
	wtk_gainnet2_cfg_t *gainnet;
    wtk_masknet_cfg_t *masknet;

    float agc_a;
    float agc_b;

	int channel;

    wtk_aspec_cfg_t aspec;
    wtk_covm_cfg_t covm;
	wtk_bf_cfg_t bf;

    int thstep;

    int theta_range;

    int lf;

    float ls_eye;

    int specsum_fs;
    int specsum_fe;

    int clip_s;
    int clip_e;

    wtk_qmmse_cfg_t qmmse;

    unsigned use_line:1;
    unsigned use_rbin_res:1;
    unsigned use_preemph:1;
    unsigned use_ceps:1;
    unsigned use_spec2:1;
    unsigned use_qmmse:1;    
    unsigned use_premask:1;    
    unsigned use_specmean:1;
    unsigned use_nbands:1;
    unsigned use_masknet:1;
    unsigned use_lsbf:1;
};

int wtk_gainnet_bf6_cfg_init(wtk_gainnet_bf6_cfg_t *cfg);
int wtk_gainnet_bf6_cfg_clean(wtk_gainnet_bf6_cfg_t *cfg);
int wtk_gainnet_bf6_cfg_update_local(wtk_gainnet_bf6_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_gainnet_bf6_cfg_update(wtk_gainnet_bf6_cfg_t *cfg);
int wtk_gainnet_bf6_cfg_update2(wtk_gainnet_bf6_cfg_t *cfg,wtk_source_loader_t *sl);

wtk_gainnet_bf6_cfg_t* wtk_gainnet_bf6_cfg_new(char *fn);
void wtk_gainnet_bf6_cfg_delete(wtk_gainnet_bf6_cfg_t *cfg);
wtk_gainnet_bf6_cfg_t* wtk_gainnet_bf6_cfg_new_bin(char *fn);
void wtk_gainnet_bf6_cfg_delete_bin(wtk_gainnet_bf6_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
