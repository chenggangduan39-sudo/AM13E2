#ifndef WTK_BFIO_VBOX_WTK_VBOXBF3_CFG
#define WTK_BFIO_VBOX_WTK_VBOXBF3_CFG
#include "wtk/bfio/afilter/wtk_nlms.h"
#include "wtk/bfio/afilter/wtk_rls.h"
#include "wtk/bfio/agc/qtk_gain_controller.h"
#include "wtk/bfio/ahs/qtk_kalman_cfg.h"
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
typedef struct wtk_vboxebf3_cfg wtk_vboxebf3_cfg_t;
struct wtk_vboxebf3_cfg {
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;

    int wins;
    int rate;

    wtk_covm_cfg_t covm;
    wtk_covm_cfg_t echo_covm;

    int bfflush_cnt;

    float bfmu;
    float bfmu2;
    float echo_bfmu;
    float echo_bfmu2;
    wtk_bf_cfg_t bf;
    wtk_rls_cfg_t echo_rls;
    wtk_nlms_cfg_t echo_nlms;
    qtk_ahs_kalman_cfg_t echo_kalman;

    wtk_qmmse_cfg_t qmmse;
    wtk_qmmse_cfg_t qmmse2;
    qtk_ahs_gain_controller_cfg_t gc;

    float spenr_thresh;
    int spenr_cnt;

    float micenr_thresh;
    int micenr_cnt;

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
    int *mic_channel2;
    int *sp_channel;
    int nspchannel;
    int nbfchannel;

    float theta;
    float phi;

    float ralpha;
    float ralpha2;
    float echo_ralpha;
    float echo_ralpha2;

    int clip_s;
    int clip_e;

    int bf_clip_s;
    int bf_clip_e;

    int cnon_clip_s;
    int cnon_clip_e;

    wtk_equalizer_cfg_t eq;

    wtk_limiter_cfg_t limiter;

    float sym;
    float gbias;
    float gbias2;
    float *freq_mask;
    int freq_count;

    int n_mask_mu;
    float mu_t_alpha;
    float mu_f_alpha;
    int mu_mask_s;
    int mu_mask_e;
    float bf_alpha;
    float mu_entropy_thresh;
    int mu_entropy_cnt;

    float gf_mask_thresh;
    int gf_mask_cnt;
    float gf_bfmu_thresh;

    float eng_scale;
    float eng_sp_scale;
    float eng_thresh;
    int eng_cnt;

    int de_clip_s;
    int de_clip_e;
    float de_thresh;
    float de_alpha;

    int pre_clip_s;
    int pre_clip_e;
    float pre_pow_ratio;
    float pre_mul_ratio;

    float max_out;
    int t_mic_in_scale;
    int t_sp_in_scale;
    float in_scale;
    float gc_gain;
    float gc_min_thresh;
    float echo_gc_min_thresh;
    int gc_cnt;
    int out_agc_level;
    char *eq_fn;
    float *eq_gain;
    int eq_len;

    float *mic_scale;

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
    int mchannel;
    int schannel;
    unsigned use_rbin_res : 1;
    unsigned use_erlssingle : 1;
    unsigned use_nlms : 1;
    unsigned use_rls : 1;
    unsigned use_kalman : 1;
    unsigned use_firstds : 1;
    unsigned use_eq : 1;
    unsigned use_in_eq : 1;
    unsigned use_out_eq : 1;
    unsigned use_maskssl : 1;
    unsigned use_maskssl2 : 1;
    unsigned use_fftsbf : 1;
    unsigned use_efftsbf : 1;
    unsigned use_qmmse : 1;
    unsigned use_qmmse2 : 1;
    unsigned use_echocovm : 1;
    unsigned use_cnon : 1;
    unsigned use_agcmean : 1;
    unsigned use_nsgainnet : 1;
    unsigned use_gsigmoid : 1;
    unsigned use_freq_mask : 1;
    unsigned use_ssl_delay : 1;
    unsigned use_fixtheta : 1;
    unsigned use_freq_preemph : 1;
    unsigned use_simple_bf : 1;
    unsigned use_bs_win : 1;
    unsigned use_gc : 1;
    unsigned use_mul_out : 1;
    unsigned use_mic_scale : 1;
    unsigned use_mul_bf : 1;
    unsigned use_mul_echo_bf : 1;
    unsigned use_mul_mask : 1;
    unsigned use_mul_echo_mask : 1;
    unsigned use_limiter : 1;
};

int wtk_vboxebf3_cfg_init(wtk_vboxebf3_cfg_t *cfg);
int wtk_vboxebf3_cfg_clean(wtk_vboxebf3_cfg_t *cfg);
int wtk_vboxebf3_cfg_update_local(wtk_vboxebf3_cfg_t *cfg, wtk_local_cfg_t *lc);
int wtk_vboxebf3_cfg_update(wtk_vboxebf3_cfg_t *cfg);
int wtk_vboxebf3_cfg_update2(wtk_vboxebf3_cfg_t *cfg, wtk_source_loader_t *sl);

wtk_vboxebf3_cfg_t *wtk_vboxebf3_cfg_new(char *fn);
void wtk_vboxebf3_cfg_delete(wtk_vboxebf3_cfg_t *cfg);
wtk_vboxebf3_cfg_t *wtk_vboxebf3_cfg_new_bin(char *fn);
void wtk_vboxebf3_cfg_delete_bin(wtk_vboxebf3_cfg_t *cfg);

wtk_vboxebf3_cfg_t *wtk_vboxebf3_cfg_new2(char *fn, char *fn2);
void wtk_vboxebf3_cfg_delete2(wtk_vboxebf3_cfg_t *cfg);
wtk_vboxebf3_cfg_t *wtk_vboxebf3_cfg_new_bin2(char *fn, char *fn2);
void wtk_vboxebf3_cfg_delete_bin2(wtk_vboxebf3_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
