#ifndef WTK_BFIO_QFORM_CMASK_BFSE_WTK_QFORM_CMASK_BFSE_CFG_H
#define WTK_BFIO_QFORM_CMASK_BFSE_WTK_QFORM_CMASK_BFSE_CFG_H
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/bfio/qform/wtk_qmmse.h"
#include "wtk/bfio/eq/wtk_equalizer.h"
#include "wtk/bfio/qform/wtk_covm.h"
#include "wtk/bfio/qform/wtk_bf.h"
#include "wtk/bfio/aspec/wtk_aspec.h"
// #define ONNX_DEC
#ifdef ONNX_DEC
#include "wtk/asr/fextra/onnxruntime/qtk_onnxruntime_cfg.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_cmask_bfse_cfg wtk_cmask_bfse_cfg_t;

struct wtk_cmask_bfse_cfg {
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;

	int wins;
    int rate;
    int nmic;
    float **mic_pos;
    float sv;
    int feat_len;
    float *theta;
    float ntheta;

	int channel;
	int *mic_channel;
	int nmicchannel;
	int *sp_channel;
	int nspchannel;
    int nbfchannel;
    int out_channels;

#ifdef ONNX_DEC
    qtk_onnxruntime_cfg_t sep;
    qtk_onnxruntime_cfg_t bfse;
#endif
    wtk_qmmse_cfg_t qmmse;
    wtk_qmmse_cfg_t qmmse2;
    wtk_covm_cfg_t covm;
	wtk_bf_cfg_t bf;
	wtk_equalizer_cfg_t eq;
    wtk_aspec_cfg_t aspec;

    float bfmu;
    float bfmu2;
    int bf_clip_s;
    int bf_clip_e;
    float sym;

    float micenr_thresh;
    int micenr_cnt;

    float entropy_thresh;
    int entropy_in_cnt;
    int entropy_cnt;

    int de_clip_s;
    int de_clip_e;
    float de_thresh;
    float de_alpha;
    float de_pow_ratio;
    float de_mul_ratio;

    int clip_s;
    int clip_e;
    int pre_clip_s;
    int pre_clip_e;
    float pre_pow_ratio;
    float pre_mul_ratio;

    float entropy_ratio;
    float entropy_min_scale;

    float max_bs_out;

    float theta_range;
    float specsum_fs;
    float specsum_fe;
    int specsum_ns;
    int specsum_ne;

    int q_nf;
    int right_nf;
    float min_speccrest;
    float envelope_thresh;
    float right_min_thresh;
    float q_alpha;

    unsigned use_onnx:1;
    unsigned use_eq:1;
    unsigned use_qmmse:1;
    unsigned use_cnon:1;
    unsigned use_bf:1;
    unsigned use_freq_preemph:1;
    unsigned use_trick:1;
    unsigned use_norm:1;
    unsigned use_aspec:1;
    unsigned use_sqenvelope:1;
    unsigned use_line:1;
};

int wtk_cmask_bfse_cfg_init(wtk_cmask_bfse_cfg_t *cfg);
int wtk_cmask_bfse_cfg_clean(wtk_cmask_bfse_cfg_t *cfg);
int wtk_cmask_bfse_cfg_update(wtk_cmask_bfse_cfg_t *cfg);
int wtk_cmask_bfse_cfg_update2(wtk_cmask_bfse_cfg_t *cfg, wtk_source_loader_t *sl);
int wtk_cmask_bfse_cfg_update_local(wtk_cmask_bfse_cfg_t *cfg, wtk_local_cfg_t *lc);

wtk_cmask_bfse_cfg_t* wtk_cmask_bfse_cfg_new(char *fn);
void wtk_cmask_bfse_cfg_delete(wtk_cmask_bfse_cfg_t *cfg);
wtk_cmask_bfse_cfg_t* wtk_cmask_bfse_cfg_new_bin(char *fn);
void wtk_cmask_bfse_cfg_delete_bin(wtk_cmask_bfse_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
