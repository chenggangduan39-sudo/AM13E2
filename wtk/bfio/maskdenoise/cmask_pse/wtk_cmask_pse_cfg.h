#ifndef WTK_BFIO_MASKDENOISE_CMASK_PSE_WTK_CMASK_PSE_CFG_H
#define WTK_BFIO_MASKDENOISE_CMASK_PSE_WTK_CMASK_PSE_CFG_H
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/bfio/maskdenoise/wtk_fbank.h"
#include "wtk/bfio/afilter/wtk_rls3.h"
#include "wtk/bfio/qform/wtk_qmmse.h"
#include "wtk/bfio/eq/wtk_equalizer.h"
#include "wtk/bfio/qform/wtk_covm.h"
#include "wtk/bfio/qform/wtk_bf.h"
#include "wtk/bfio/maskdenoise/cmask_pse/wtk_cmask_sv.h"
// #define ONNX_DEC
#ifdef ONNX_DEC
#include "wtk/asr/fextra/onnxruntime/qtk_onnxruntime_cfg.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_cmask_pse_cfg wtk_cmask_pse_cfg_t;

struct wtk_cmask_pse_cfg {
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;

	int wins;
    int rate;

	int channel;
	int *mic_channel;
	int nmicchannel;
	int *sp_channel;
	int nspchannel;
    int nbfchannel;

    int emb0_len;
    int emb1_len;
    int emb2_len;
    int emb3_len;
    int gamma_len;
    int beta_len;
    int gamma1_len;
    int beta1_len;
    int gamma2_len;
    int beta2_len;
    int emb_mask_len;
    int emb_mask_type;

#ifdef ONNX_DEC
    qtk_onnxruntime_cfg_t emb;
    qtk_onnxruntime_cfg_t pse;
#endif
    int num_frame;
    wtk_fbank_cfg_t fbank;

    wtk_rls3_cfg_t echo_rls3;
    wtk_qmmse_cfg_t qmmse;
    wtk_covm_cfg_t covm;
    wtk_covm_cfg_t echo_covm;
	wtk_bf_cfg_t bf;
	wtk_equalizer_cfg_t eq;
    wtk_cmask_sv_cfg_t sv;

    float bfmu;
    float bfmu2;
    float echo_bfmu;
    float echo_bfmu2;
    int bf_clip_s;
    int bf_clip_e;
    float sym;

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
    float de_pow_ratio;
    float de_mul_ratio;

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

    int clip_s;
    int clip_e;
    int pre_clip_s;
    int pre_clip_e;
    float pre_pow_ratio;
    float pre_mul_ratio;

    float entropy_ratio;
    float entropy_min_scale;

    float max_bs_out;
    float sv_thresh;
    float sv_thresh_mid;
    float sv_thresh_low;

    unsigned use_onnx:1;
    unsigned use_aec_model:1;
    unsigned use_ccm:1;
    unsigned use_rls3:1;
    unsigned use_eq:1;
    unsigned use_qmmse:1;
    unsigned use_cnon:1;
    unsigned use_echocovm:1;
    unsigned use_bf:1;
    unsigned use_echo_bf:1;
    unsigned use_freq_preemph:1;
    unsigned use_trick:1;
    unsigned use_sv_check:1;
};

int wtk_cmask_pse_cfg_init(wtk_cmask_pse_cfg_t *cfg);
int wtk_cmask_pse_cfg_clean(wtk_cmask_pse_cfg_t *cfg);
int wtk_cmask_pse_cfg_update(wtk_cmask_pse_cfg_t *cfg);
int wtk_cmask_pse_cfg_update2(wtk_cmask_pse_cfg_t *cfg, wtk_source_loader_t *sl);
int wtk_cmask_pse_cfg_update_local(wtk_cmask_pse_cfg_t *cfg, wtk_local_cfg_t *lc);

wtk_cmask_pse_cfg_t* wtk_cmask_pse_cfg_new(char *fn);
void wtk_cmask_pse_cfg_delete(wtk_cmask_pse_cfg_t *cfg);
wtk_cmask_pse_cfg_t* wtk_cmask_pse_cfg_new_bin(char *fn);
void wtk_cmask_pse_cfg_delete_bin(wtk_cmask_pse_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
