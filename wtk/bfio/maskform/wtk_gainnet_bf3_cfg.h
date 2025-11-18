#ifndef WTK_BFIO_MASKFORM_WTK_GAINNET_BF3_CFG
#define WTK_BFIO_MASKFORM_WTK_GAINNET_BF3_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/bfio/masknet/wtk_gainnet7.h"
#include "wtk/bfio/masknet/wtk_gainnet2.h"
#include "wtk/bfio/masknet/wtk_gainnet.h"
#include "wtk/bfio/masknet/wtk_gainnet4.h"
#include "wtk/bfio/qform/wtk_bf.h"
#include "wtk/bfio/qform/wtk_covm.h"
#include "wtk/bfio/qform/wtk_qmmse.h"
#include "wtk/bfio/eq/wtk_equalizer.h"
#include "wtk/bfio/ssl/wtk_ssl2.h"
#include "wtk/bfio/ssl/wtk_maskssl.h"
#include "wtk/bfio/ssl/wtk_maskssl2.h"
#include "wtk/bfio/maskdenoise/wtk_bankfeat.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_gainnet_bf3_cfg wtk_gainnet_bf3_cfg_t;
struct wtk_gainnet_bf3_cfg
{
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;

	int wins;
    int rate;
    
    wtk_covm_cfg_t covm;
	wtk_bf_cfg_t bf;

    wtk_qmmse_cfg_t qmmse;

    wtk_bankfeat_cfg_t bankfeat;

    float micenr_thresh;
    int micenr_cnt;

    char *mdl_fn;
	wtk_gainnet7_cfg_t *gainnet;
    wtk_gainnet2_cfg_t *gainnet2;
    wtk_gainnet4_cfg_t *gainnet4;

    int xv_len;

    wtk_maskssl_cfg_t maskssl;
    wtk_maskssl2_cfg_t maskssl2;

	int channel;

    float g_min;
    float g_max;
    float g2_min;
    float g2_max;

    float g_minthresh;

    float g_a;
    float g_b;
    float agc_a;
    float agc_b;
    float agce_thresh;
    char *agcmdl_fn;
	wtk_gainnet_cfg_t *agc_gainnet;

    float ralpha;

    float theta;
	float phi;

    int pframe_fs;
    int pframe_fe;
    float pframe_thresh;
    float pframe_alpha;

    int clip_s;
    int clip_e;
    
    int save_s;

	wtk_equalizer_cfg_t eq;

    float gbias;

    int featm_lm;

    float ptpr_thresh;
    float papr_thresh;
    float pnpr_thresh;
    int LM;
    int LD;
    float howllms_palpha;
    float howllms_mufb;
    int howl_cnt;
    int howl_lf;

    unsigned use_rbin_res:1;
    unsigned use_fixtheta:1;
    unsigned use_qmmse:1;
    unsigned use_eq:1;
    unsigned use_maskssl:1;
    unsigned use_maskssl2:1;
    unsigned use_gainnet2:1;
    unsigned use_gainnet4:1;
    unsigned use_fftsbf:1;
    unsigned use_xvector:1;
    unsigned use_gsigmoid:1;
    unsigned use_agcmean:1;
    unsigned use_howl_detection:1;
};

int wtk_gainnet_bf3_cfg_init(wtk_gainnet_bf3_cfg_t *cfg);
int wtk_gainnet_bf3_cfg_clean(wtk_gainnet_bf3_cfg_t *cfg);
int wtk_gainnet_bf3_cfg_update_local(wtk_gainnet_bf3_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_gainnet_bf3_cfg_update(wtk_gainnet_bf3_cfg_t *cfg);
int wtk_gainnet_bf3_cfg_update2(wtk_gainnet_bf3_cfg_t *cfg,wtk_source_loader_t *sl);

wtk_gainnet_bf3_cfg_t* wtk_gainnet_bf3_cfg_new(char *fn);
void wtk_gainnet_bf3_cfg_delete(wtk_gainnet_bf3_cfg_t *cfg);
wtk_gainnet_bf3_cfg_t* wtk_gainnet_bf3_cfg_new_bin(char *fn);
void wtk_gainnet_bf3_cfg_delete_bin(wtk_gainnet_bf3_cfg_t *cfg);

void wtk_gainnet_bf3_cfg_set_channel(wtk_gainnet_bf3_cfg_t *cfg, int channel);
#ifdef __cplusplus
};
#endif
#endif
