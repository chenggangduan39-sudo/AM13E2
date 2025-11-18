#ifndef WTK_BFIO_AEC_CMASK_AEC_WTK_CMASK_AEC_CFG
#define WTK_BFIO_AEC_CMASK_AEC_WTK_CMASK_AEC_CFG
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/bfio/afilter/wtk_nlms.h"
#include "wtk/bfio/afilter/wtk_rls.h"
#include "wtk/bfio/afilter/wtk_rls3.h"
#include "wtk/bfio/qform/wtk_qmmse.h"
#include "wtk/bfio/eq/wtk_equalizer.h"
#include "wtk/bfio/qform/wtk_covm.h"
#include "wtk/bfio/qform/wtk_bf.h"
#include "wtk/bfio/ssl/wtk_ssl2.h"
#include "wtk/bfio/ssl/wtk_maskssl2.h"
#include "wtk/bfio/agc/qtk_gain_controller.h"

#ifdef ONNX_DEC
#include "wtk/asr/fextra/onnxruntime/qtk_onnxruntime_cfg.h"
#endif
#include "qtk/nnrt/qtk_nnrt_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_cmask_aec_cfg wtk_cmask_aec_cfg_t;

struct wtk_cmask_aec_cfg {
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;

	int wins;
    int rate;
    float sv;

	wtk_rls_cfg_t echo_rls;
    wtk_rls3_cfg_t echo_rls3;
    wtk_nlms_cfg_t echo_nlms;

    wtk_qmmse_cfg_t qmmse;
    wtk_qmmse_cfg_t qmmse2;
    qtk_ahs_gain_controller_cfg_t gc;
    wtk_covm_cfg_t covm;
    wtk_covm_cfg_t echo_covm;
    wtk_covm_cfg_t icovm;
	wtk_bf_cfg_t bf;
	wtk_bf_cfg_t ibf;
    wtk_maskssl2_cfg_t maskssl2;
    wtk_maskssl2_cfg_t ibf_ssl;
    float bfmu;
    float bfmu2;
    float echo_bfmu;
    float echo_bfmu2;
    int bf_clip_s;
    int bf_clip_e;

    float spenr_thresh;
    int spenr_cnt;

    float micenr_thresh;
    int micenr_cnt;

    float entropy_thresh;
    float entropy_sp_thresh;
    int entropy_in_cnt;
    int entropy_cnt;

    float raw_alpha1;
    float raw_alpha2;
    float raw_alpha3;
    float raw_alpha4;
    float pow_scale;
    float alpha1;
    float alpha2;
    float alpha3;
    float alpha4;
    float scale1;
    float scale2;
    float scale3;
    float scale4;
    float init_low_freq;
    float init_high_freq;

    int de_clip_s;
    int de_clip_e;
    float de_thresh;
    float de_alpha;

    float eng_scale;
    float eng1_thresh;
    float eng1_thresh2;
    float eng2_thresh;
    float eng2_thresh2;
    float eng_freq;
    int eng_cnt;

    float sp_max_smooth_gain;
    float sp_min_smooth_gain;
    float mic_max_smooth_gain;
    float mic_min_smooth_gain;

	int channel;
	int *mic_channel;
	int nmicchannel;
	int *sp_channel;
	int nspchannel;
    int *chns;
    int *change_mic;
    int *change_ref;
    int ncmicchannel;
    int ncrefchannel;
    int ncchannel;
    int nbfchannel;
    int nibfchannel;

    int clip_s;
    int clip_e;
    int pre_clip_s;
    int pre_clip_e;
    float pre_pow_ratio;
    float pre_mul_ratio;

    float sdb_alpha;

	wtk_equalizer_cfg_t eq;
    float sym;

    unsigned use_nlms:1;
    unsigned use_rls:1;
    unsigned use_rls3:1;
    unsigned use_eq:1;
    unsigned use_qmmse:1;
    unsigned use_qmmse2:1;
    unsigned use_cnon:1;
    unsigned use_echocovm:1;
#ifdef ONNX_DEC
    qtk_onnxruntime_cfg_t onnx;
#endif
    int num_frame;

    unsigned use_ccm:1;
    unsigned use_entropy:1;
    unsigned use_bf:1;

    qtk_nnrt_cfg_t rt;
    int num_in;
    int num_out;

    qtk_nnrt_cfg_t dereb_rt;
    int dereb_num_in;
    int dereb_num_out;

    int dereb_delay;
    int dereb_taps;
    float dereb_alpha;
    float wpe_thresh;
    int wpe_cnt;
    float wpe_alpha;
    float wpe_power;

    float change_thresh;
    int change_cnt;
    float change_alpha;
    float change_alpha2;
    int change_delay;

    float entropy_ratio;
    float entropy_min_scale;

    float max_bs_out;

    float micenr_thresh2;
    float micenr_thresh3;
    int micenr_cnt2;
    int micenr_cnt3;
    float micenr_scale;

    int sp_ibf_delay;

    float *mic_theta;
    float ibf_theta;
    float ibf_thresh;
    float init_ibf_thresh;

    float gc_gain;
    float gc_min_thresh;
    int gc_cnt;
    int out_agc_level;
    char *eq_fn;
    float *eq_gain;
    int eq_len;

    float bf_theta;
    float ds_w_alpha;

    float gain_alpha;
    float gain_alpha2;
    float gain_beta;

    unsigned use_onnx:1;
    unsigned use_ncnn:1;
    unsigned use_dereb:1;
    unsigned use_wpe:1;
    unsigned use_change_mic:1;
    unsigned use_freq_preemph:1;
    unsigned use_change_mic2:1;
    unsigned use_echo_bf:1;
    unsigned use_maskssl2:1;
    unsigned use_entropy_scale:1;
    unsigned use_ibf:1;
    unsigned use_ibf_ssl:1;
    unsigned use_bs_win:1;
    unsigned use_gc:1;
    unsigned use_mask_bf:1;
    unsigned use_in_eq:1;
    unsigned use_out_eq:1;
    unsigned use_scale_qmmse:1;
    unsigned use_scale_qmmse2:1;
    unsigned use_ds:1;
    unsigned use_freq_atten:1;
};

int wtk_cmask_aec_cfg_init(wtk_cmask_aec_cfg_t *cfg);
int wtk_cmask_aec_cfg_clean(wtk_cmask_aec_cfg_t *cfg);
int wtk_cmask_aec_cfg_update(wtk_cmask_aec_cfg_t *cfg);
int wtk_cmask_aec_cfg_update2(wtk_cmask_aec_cfg_t *cfg, wtk_source_loader_t *sl);
int wtk_cmask_aec_cfg_update_local(wtk_cmask_aec_cfg_t *cfg, wtk_local_cfg_t *lc);

wtk_cmask_aec_cfg_t* wtk_cmask_aec_cfg_new(char *fn);
void wtk_cmask_aec_cfg_delete(wtk_cmask_aec_cfg_t *cfg);
wtk_cmask_aec_cfg_t* wtk_cmask_aec_cfg_new_bin(char *fn);
void wtk_cmask_aec_cfg_delete_bin(wtk_cmask_aec_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
