#ifndef WTK_BFIO_QFORM_BEAMNET_WTK_BEAMNET4_CFG_H
#define WTK_BFIO_QFORM_BEAMNET_WTK_BEAMNET4_CFG_H
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/bfio/qform/wtk_qmmse_cfg.h"
#include "wtk/bfio/qform/wtk_bf.h"
#include "wtk/bfio/qform/wtk_covm.h"
#include "wtk/bfio/agc/qtk_gain_controller.h"
#ifdef ONNX_DEC
#include "wtk/asr/fextra/onnxruntime/qtk_onnxruntime_cfg.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_beamnet4_cfg wtk_beamnet4_cfg_t;

struct wtk_beamnet4_cfg {
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;

    int wins;
    int rate;
    int *nmics;
    float ***mic_pos;
    int feature_len;
    float sv;
    int sep_feature_type;
    int bf_feature_type;
    float feat_scaler;
    float scaler;
    float cross_scaler;
    float *target_theta;
    float *target_delay;
    int narray;
    float *mic_scale;

    int channel;
    int nmicchannel;
    int *nmicchannels;
    int **mic_channel;
    int nbfchannel;
    int in_channels;
    int *out_channels;
    int out_channel;
    int covar_channels;
    int gf_channel;

#ifdef ONNX_DEC
    qtk_onnxruntime_cfg_t seperator;
    qtk_onnxruntime_cfg_t beamformer;
#endif
    wtk_qmmse_cfg_t qmmse;
    qtk_ahs_gain_controller_cfg_t gc;

    float entropy_thresh;
    int entropy_in_cnt;
    int entropy_cnt;
    float entropy_ratio;
    float entropy_min_scale;

    int delay_nf;

    wtk_bf_cfg_t bf;
    wtk_covm_cfg_t covm;

    float bfmu;
    float bfmu2;

    int clip_s;
    int clip_e;
    int bf_clip_s;
    int bf_clip_e;

    int bfflush_cnt;

    float gc_gain;
    float gc_min_thresh;
    int gc_cnt;
    int out_agc_level;
    float max_bs_out;

    float mean_gf_thresh;

    unsigned int use_onnx:1;
    unsigned int norm_channel:1;
    unsigned int use_mvdr:1;
    unsigned int use_qmmse:1;
    unsigned int use_bf:1;
    unsigned int use_sim_bf:1;
    unsigned int use_fixtheta:1;
    unsigned int use_gc:1;
    unsigned int use_bs_win:1;
};

int wtk_beamnet4_cfg_init(wtk_beamnet4_cfg_t *cfg);
int wtk_beamnet4_cfg_clean(wtk_beamnet4_cfg_t *cfg);
int wtk_beamnet4_cfg_update(wtk_beamnet4_cfg_t *cfg);
int wtk_beamnet4_cfg_update2(wtk_beamnet4_cfg_t *cfg, wtk_source_loader_t *sl);
int wtk_beamnet4_cfg_update_local(wtk_beamnet4_cfg_t *cfg, wtk_local_cfg_t *lc);

wtk_beamnet4_cfg_t* wtk_beamnet4_cfg_new(char *fn);
void wtk_beamnet4_cfg_delete(wtk_beamnet4_cfg_t *cfg);
wtk_beamnet4_cfg_t* wtk_beamnet4_cfg_new_bin(char *fn);
void wtk_beamnet4_cfg_delete_bin(wtk_beamnet4_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
