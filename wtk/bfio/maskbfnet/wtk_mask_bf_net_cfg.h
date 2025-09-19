#ifndef WTK_BFIO_MASKBFNET_WTK_MASK_BF_NET_CFG_H
#define WTK_BFIO_MASKBFNET_WTK_MASK_BF_NET_CFG_H
#include "qtk/nnrt/qtk_nnrt.h"
#include "qtk/nnrt/qtk_nnrt_cfg.h"
#include "wtk/bfio/afilter/wtk_rls3.h"
#include "wtk/bfio/agc/qtk_gain_controller.h"
#include "wtk/bfio/eq/wtk_equalizer.h"
#include "wtk/bfio/maskdenoise/wtk_bankfeat.h"
#include "wtk/bfio/masknet/wtk_gainnet2.h"
#include "wtk/bfio/qform/wtk_bf.h"
#include "wtk/bfio/qform/wtk_covm.h"
#include "wtk/bfio/qform/wtk_qmmse.h"
#include "wtk/bfio/limiter/wtk_limiter.h"
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_mask_bf_net_cfg wtk_mask_bf_net_cfg_t;

struct wtk_mask_bf_net_cfg {
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;

    wtk_rls3_cfg_t echo_rls3;

    wtk_covm_cfg_t covm;
    wtk_covm_cfg_t echo_covm;
    wtk_bf_cfg_t bf;
    wtk_bankfeat_cfg_t bankfeat;
    wtk_bankfeat_cfg_t bankfeat2;
    char *aecmdl_fn;
    char *aecmdl_fn2;
    wtk_qmmse_cfg_t qmmse;
    wtk_qmmse_cfg_t qmmse2;
    wtk_gainnet2_cfg_t *gainnet2;
    wtk_gainnet2_cfg_t *gainnet2_2;
    qtk_ahs_gain_controller_cfg_t gc;
    wtk_equalizer_cfg_t eq;
    wtk_limiter_cfg_t limiter;
    char *qnn1_fn;
    char *qnn2_fn;
    char *qnn3_fn;
    wtk_strbuf_t *qnn1_buf;
    wtk_strbuf_t *qnn2_buf;
    wtk_strbuf_t *qnn3_buf;

    qtk_nnrt_cfg_t stage1_rt;
    qtk_nnrt_cfg_t stage2_rt;
    qtk_nnrt_cfg_t dr_stage2_rt;

    int wins;
    int rate;
    float sv;

    int channel;
    int *mic_channel;
    int nmicchannel;
    int *sp_channel;
    int nspchannel;
    int nbfchannel;
    int sp_main_chn;

    float mic_scale;
    float sp_scale;

    int num_frame;
    int rss_iter;
    int update_w_cnt;

    int *update_w_freq;

    float spenr_thresh;
    int spenr_cnt;

    float micenr_thresh;
    int micenr_cnt;

    int clip_s;
    int clip_e;
    int pre_clip_s;
    int pre_clip_e;
    float pre_pow_ratio;
    float pre_mul_ratio;

    float entropy_thresh;
    float entropy_sp_thresh;
    int entropy_in_cnt;
    int entropy_cnt;

    float bfmu;
    float bfmu2;
    float echo_bfmu;
    float echo_bfmu2;
    int bf_clip_s;
    int bf_clip_e;

    int featm_lm;
    int featsp_lm;
    int featm_lm2;
    int featsp_lm2;

    int cnon_clip_s;
    int cnon_clip_e;

    float sym;

    float scov_alpha;
    float ncov_alpha;
    int init_covnf;

    float mu_entropy_thresh;

    float gc_gain;
    float gc_min_thresh;
    int gc_cnt;
    int out_agc_level;
    float max_out;

    float max_mask;
    float mask_peak;
    float sp_max_mask;
    float sp_mask_peak;

    float *all_scov_alpha;
    float *eta_thresh;
    int *scov_alpha_idx;
    int scov_alpha_n;

    float *qmmse2_agc_level;
    float *qmmse2_max_gain;
    float *gc_gain_level;
    int n_agc_level;

    float *qmmse2_noise_suppress;
    int n_ans_level;

    float min_mask;
    float eta_mean_thresh;
    float scov_entropy_thresh;
    float echo_scov_entropy_thresh;
    float epsi_thresh;
    int eta_clip_s;
    int eta_clip_e;

    int ntheta;
    float sdb_alpha;
    int n_mask_mu;

    float mu_t_alpha;
    float mu_f_alpha;
    int mu_mask_s;
    int mu_mask_e;
    float bf2_alpha;
    float echo_bf2_alpha;
    int bf_change_frame;

    float qmmse2_mask_thresh;

    int bf_init_frame;
    float vec_init;

    float beta;
    float gamma;
    float zeta;
    float bin_speech_thresh;
    float bin_noise_thresh;
    float frame_speech_thresh;
    float frame_noise_thresh;
    float scnt_thresh;
    float ncnt_thresh;
    float post_clip;

    float model1_scale;
    float model1_sp_scale;
    float model2_scale;
    float model2_sp_scale;

    unsigned use_pffft : 1;
    unsigned use_rls3 : 1;
    unsigned use_stage1_rt : 1;
    unsigned use_edr_stage1_rt : 1;
    unsigned use_freq_preemph : 1;
    unsigned use_stage2_rt : 1;
    unsigned use_dr_stage2_rt : 1;
    unsigned use_mask_model : 1;
    unsigned use_qmmse : 1;
    unsigned use_qmmse2 : 1;
    unsigned use_gainnet2 : 1;
    unsigned use_gainnet2_2 : 1;
    unsigned use_ccm : 1;
    unsigned use_bf : 1;
    unsigned use_bf_v2 : 1;
    unsigned use_echo_bf : 1;
    unsigned use_bf2 : 1;
    unsigned use_echocovm : 1;
    unsigned use_gc : 1;
    unsigned use_cnon : 1;
    unsigned use_bs_win : 1;
    unsigned use_eq : 1;
    unsigned use_rbin_res : 1;
    unsigned use_median_chn : 1;
    unsigned use_phase_corr : 1;
    unsigned use_ds : 1;
    unsigned use_debug : 1;
    unsigned use_qnn : 1;
    unsigned use_debug_echo_model : 1;
    unsigned use_limiter : 1;
    unsigned use_raw_add : 1;
};

int wtk_mask_bf_net_cfg_init(wtk_mask_bf_net_cfg_t *cfg);
int wtk_mask_bf_net_cfg_clean(wtk_mask_bf_net_cfg_t *cfg);
int wtk_mask_bf_net_cfg_update(wtk_mask_bf_net_cfg_t *cfg);
int wtk_mask_bf_net_cfg_update2(wtk_mask_bf_net_cfg_t *cfg,
                                wtk_source_loader_t *sl);
int wtk_mask_bf_net_cfg_update_local(wtk_mask_bf_net_cfg_t *cfg,
                                     wtk_local_cfg_t *lc);

wtk_mask_bf_net_cfg_t *wtk_mask_bf_net_cfg_new(char *fn);
void wtk_mask_bf_net_cfg_delete(wtk_mask_bf_net_cfg_t *cfg);
wtk_mask_bf_net_cfg_t *wtk_mask_bf_net_cfg_new_bin(char *fn);
void wtk_mask_bf_net_cfg_delete_bin(wtk_mask_bf_net_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
