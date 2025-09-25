#ifndef WTK_BFIO_MASKFORM_WTK_GAINNET_BF_STEREO_CFG
#define WTK_BFIO_MASKFORM_WTK_GAINNET_BF_STEREO_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/bfio/masknet/wtk_gainnet2.h"
#include "wtk/bfio/masknet/wtk_gainnet.h"
#include "wtk/bfio/qform/wtk_bf.h"
#include "wtk/bfio/qform/wtk_covm.h"
#include "wtk/bfio/qform/wtk_qmmse.h"
#include "wtk/bfio/eq/wtk_equalizer.h"
#include "wtk/bfio/maskdenoise/wtk_bankfeat.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_gainnet_bf_stereo_cfg wtk_gainnet_bf_stereo_cfg_t;
struct wtk_gainnet_bf_stereo_cfg
{
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;

	int wins;
    int rate;
    
    wtk_covm_cfg_t covm;
	wtk_bf_cfg_t bf;

    wtk_qmmse_cfg_t qmmse;

    float micenr_thresh;
    int micenr_cnt;

    wtk_bankfeat_cfg_t bankfeat;

    char *mdl_fn;
    wtk_gainnet2_cfg_t *gainnet2;

    float g_min;
    float g_max;
    float g_a;
    float g_b;
    float gbias;
    float g_minthresh;

    char *agcmdl_fn;
	wtk_gainnet_cfg_t *agc_gainnet;
    float agc_a;
    float agc_b;
    float g2_min;
    float g2_max;
    float agce_thresh;

	int *mic_channel;
    int *out_channel;
    int noutchannel;
	int nmicchannel;
    int channel;

    float theta;
	float phi;

    float ralpha;
    float ralpha2;

    int clip_s;
    int clip_e;

    int featm_lm;
    int repate_outchannel;

	wtk_equalizer_cfg_t eq;

    unsigned use_rbin_res:1;
    unsigned use_eq:1;
    unsigned use_fftsbf:1;
    unsigned use_qmmse:1;
    unsigned use_gsigmoid:1;
};

int wtk_gainnet_bf_stereo_cfg_init(wtk_gainnet_bf_stereo_cfg_t *cfg);
int wtk_gainnet_bf_stereo_cfg_clean(wtk_gainnet_bf_stereo_cfg_t *cfg);
int wtk_gainnet_bf_stereo_cfg_update_local(wtk_gainnet_bf_stereo_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_gainnet_bf_stereo_cfg_update(wtk_gainnet_bf_stereo_cfg_t *cfg);
int wtk_gainnet_bf_stereo_cfg_update2(wtk_gainnet_bf_stereo_cfg_t *cfg,wtk_source_loader_t *sl);

wtk_gainnet_bf_stereo_cfg_t* wtk_gainnet_bf_stereo_cfg_new(char *fn);
void wtk_gainnet_bf_stereo_cfg_delete(wtk_gainnet_bf_stereo_cfg_t *cfg);
wtk_gainnet_bf_stereo_cfg_t* wtk_gainnet_bf_stereo_cfg_new_bin(char *fn);
void wtk_gainnet_bf_stereo_cfg_delete_bin(wtk_gainnet_bf_stereo_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
