#ifndef WTK_BFIO_VBOX_WTK_VBOXBF6_CFG
#define WTK_BFIO_VBOX_WTK_VBOXBF6_CFG
#include "wtk/bfio/afilter/wtk_nlms.h"
#include "wtk/bfio/afilter/wtk_rls.h"
#include "wtk/bfio/agc/qtk_gain_controller.h"
#include "wtk/bfio/eq/wtk_equalizer.h"
#include "wtk/bfio/maskdenoise/wtk_bankfeat.h"
#include "wtk/bfio/masknet/wtk_gainnet.h"
#include "wtk/bfio/masknet/wtk_gainnet2.h"
#include "wtk/bfio/masknet/wtk_gainnet7.h"
#include "wtk/bfio/qform/wtk_bf.h"
#include "wtk/bfio/qform/wtk_covm.h"
#include "wtk/bfio/qform/wtk_qmmse.h"
#include "wtk/bfio/ssl/wtk_maskssl.h"
#include "wtk/bfio/ssl/wtk_maskssl2.h"
#include "wtk/bfio/ssl/wtk_ssl2.h"
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/bfio/limiter/wtk_limiter.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_vboxebf6_cfg wtk_vboxebf6_cfg_t;
struct wtk_vboxebf6_cfg {
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;

    int wins;
    int rate;

    wtk_covm_cfg_t covm;
    wtk_covm_cfg_t echo_covm;

    int bfflush_cnt;

    float bfmu;
    float echo_bfmu;
    float echo_bfmu2;
    wtk_bf_cfg_t bf;
    wtk_rls_cfg_t echo_rls;
    wtk_nlms_cfg_t echo_nlms;

    wtk_qmmse_cfg_t qmmse;
    wtk_qmmse_cfg_t qmmse2;
    wtk_qmmse_cfg_t qmmse3;
    wtk_qmmse_cfg_t qmmse4;
    wtk_qmmse_cfg_t qmmse5;
    qtk_ahs_gain_controller_cfg_t gc;

    float spenr_thresh;
    float spenr_thresh2;
    int spenr_cnt;
    int spenr_cnt2;

    float micenr_thresh;
    int micenr_cnt;

    float entropy_thresh;
    int entropy_cnt;
    int entropy_in_cnt;
    float entropy_scale;
    float entropy_in_scale;

    wtk_bankfeat_cfg_t bankfeat;

    int featm_lm;
    int featsp_lm;

    char *mdl_fn;
    wtk_gainnet7_cfg_t *gainnet7;
    char *aecmdl_fn;
    wtk_gainnet2_cfg_t *gainnet2;

    float g_minthresh;

    char *agcmdl_fn;
    wtk_gainnet_cfg_t *agc_gainnet;
    float agc_a;
    float agc_a2;
    float agc_b;
    float eagc_a;
    float eagc_b;
    float agce_thresh;
    float agcaddg;

    float g_a;
    float g_b;
    float g_min;
    float g_max;
    float g2_min;
    float g2_max;

    wtk_maskssl_cfg_t maskssl;
    wtk_maskssl2_cfg_t maskssl2;

    int channel;
    int *mic_channel;
    int nmicchannel;
    int *mix_channel;
    int nmixchannel;
    int *mic_channel2;
    int *sp_channel;
    int nspchannel;
    int nbfchannel;

    int max_1;
    int min_1;
    int max_2;
    int min_2;

    int f_win;
    int scale;

    float theta;
    float phi;

    float ralpha;
    float ralpha2;
    float echo_ralpha;
    float echo_ralpha2;

    int clip_s;
    int clip_e;
    int denoise_clip_s;
    int denoise_clip_e;
    int aec_clip_s;
    int aec_clip_e;

    wtk_equalizer_cfg_t eq;

    wtk_limiter_cfg_t limiter;

    float sym;
    float gbias;
    float *freq_mask;
    int freq_count;

    float gf_mask_thresh;

    float leak_scale;
    float denoise_mdl_scale;
    float denoise_alg_scale;
    float denoise_alg_scale2;
    float denoise_alg_scale3;
    float aec_mdl_scale;
    float aec_alg_scale;
    float aec_alg_scale2;
    float aec_alg_scale3;
    float qmmse_mask_scale;
    float denoise_alg_alpha;
    float aec_alg_alpha;
    float alg_alpha;

    float e_th;

    int leak_frame;

    int b_agc_scale;

    float max_out;
    float gc_gain;
    float gc_min_thresh;
    int gc_cnt;
    int out_agc_level;

    float *agc_model_level;
    float *gc_gain_level;
    float *qmmse_agc_level;
    float *qmmse_max_gain;
    int n_agc_level;

    float *qmmse_noise_suppress;
    float *gbias_level;
    int n_ans_level;

    float *aec_leak_scale;
    int n_aec_level;

    unsigned use_rbin_res : 1;
    unsigned use_erlssingle : 1;
    unsigned use_nlms : 1;
    unsigned use_rls : 1;
    unsigned use_firstds : 1;
    unsigned use_eq : 1;
    unsigned use_maskssl : 1;
    unsigned use_maskssl2 : 1;
    unsigned use_fftsbf : 1;
    unsigned use_efftsbf : 1;
    unsigned use_qmmse : 1;
    unsigned use_qmmse2 : 1;
    unsigned use_qmmse3 : 1;
    unsigned use_qmmse4 : 1;
    unsigned use_qmmse5 : 1;
    unsigned use_echocovm : 1;
    unsigned use_cnon : 1;
    unsigned use_agcmean : 1;
    unsigned use_nsgainnet : 1;
    unsigned use_gsigmoid : 1;
    unsigned use_freq_mask : 1;
    unsigned use_ssl_delay : 1;
    unsigned use_fixtheta : 1;
    unsigned use_denoise_mdl : 1;
    unsigned use_denoise_alg : 1;
    unsigned use_aec_mdl : 1;
    unsigned use_aec_alg : 1;
    unsigned use_qmmse_mask : 1;
    unsigned use_qmmse_state : 1;
    unsigned use_frame_leak : 1;
    unsigned use_bs_win : 1;
    unsigned use_gc : 1;
    unsigned use_limiter : 1;
};

int wtk_vboxebf6_cfg_init(wtk_vboxebf6_cfg_t *cfg);
int wtk_vboxebf6_cfg_clean(wtk_vboxebf6_cfg_t *cfg);
int wtk_vboxebf6_cfg_update_local(wtk_vboxebf6_cfg_t *cfg, wtk_local_cfg_t *lc);
int wtk_vboxebf6_cfg_update(wtk_vboxebf6_cfg_t *cfg);
int wtk_vboxebf6_cfg_update2(wtk_vboxebf6_cfg_t *cfg, wtk_source_loader_t *sl);

wtk_vboxebf6_cfg_t *wtk_vboxebf6_cfg_new(char *fn);
void wtk_vboxebf6_cfg_delete(wtk_vboxebf6_cfg_t *cfg);
wtk_vboxebf6_cfg_t *wtk_vboxebf6_cfg_new_bin(char *fn);
void wtk_vboxebf6_cfg_delete_bin(wtk_vboxebf6_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
