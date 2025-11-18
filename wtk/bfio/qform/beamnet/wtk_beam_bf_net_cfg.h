#ifndef WTK_BFIO_MASKBFNET_WTK_BEAM_BF_NET_CFG_H
#define WTK_BFIO_MASKBFNET_WTK_BEAM_BF_NET_CFG_H
#include "qtk/nnrt/qtk_nnrt.h"
#include "qtk/nnrt/qtk_nnrt_cfg.h"
#include "wtk/bfio/afilter/wtk_rls3.h"
#include "wtk/bfio/agc/qtk_gain_controller.h"
#include "wtk/bfio/eq/wtk_equalizer.h"
#include "wtk/bfio/qform/wtk_mask_bf.h"
#include "wtk/bfio/qform/wtk_qmmse.h"
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_beam_bf_net_cfg wtk_beam_bf_net_cfg_t;

struct wtk_beam_bf_net_cfg {
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;

    wtk_rls3_cfg_t echo_rls3;
    wtk_mask_bf_cfg_t mask_bf;
    wtk_qmmse_cfg_t qmmse2;
    qtk_ahs_gain_controller_cfg_t gc;
    wtk_equalizer_cfg_t eq;
    char *qnn1_fn;
    wtk_strbuf_t *qnn1_buf;

    qtk_nnrt_cfg_t stage1_rt;

    int wins;
    int nbin;
    int rate;
    float sv;

    int channel;
    int *mic_channel;
    int nmicchannel;
    int *sp_channel;
    int nspchannel;
    int nbfchannel;
    int out_channels;
    int sp_main_chn;
    int nmic;
    float **mic_pos;

    float mic_scale;
    float sp_scale;

    int num_frame;

    float spenr_thresh;
    int spenr_cnt;

    float micenr_thresh;
    int micenr_cnt;

    int clip_s;
    int clip_e;

    int cnon_clip_s;
    int cnon_clip_e;

    float sym;

    float gc_gain;
    float gc_min_thresh;
    int gc_cnt;
    int out_agc_level;
    float max_out;

    float *qmmse2_agc_level;
    float *qmmse2_max_gain;
    float *gc_gain_level;
    int n_agc_level;

    float *qmmse2_noise_suppress;
    int n_ans_level;

    float qmmse2_mask_thresh;

    float model1_scale;
    float model1_sp_scale;

    float theta_step;
    float min_theta;
    float max_theta;
    int n_theta;

    float *compsamp;
    wtk_complex_t **comp_filter;

    float *mapping_theta;
    float *real_theta;
    int n_mapping_theta;
    float limiter_thresh;

    unsigned use_pffft : 1;
    unsigned use_rls3 : 1;
    unsigned use_stage1_rt : 1;
    unsigned use_edr_stage1_rt : 1;
    unsigned use_qmmse2 : 1;
    unsigned use_mask_bf : 1;
    unsigned use_gc : 1;
    unsigned use_cnon : 1;
    unsigned use_bs_win : 1;
    unsigned use_eq : 1;
    unsigned use_qnn : 1;
    unsigned use_limiter : 1;
    unsigned use_two_pass : 1;
    unsigned use_post_mask : 1;
    unsigned use_csn : 1;
    unsigned use_line : 1;
    unsigned use_theta_mapping : 1;
};

int wtk_beam_bf_net_cfg_init(wtk_beam_bf_net_cfg_t *cfg);
int wtk_beam_bf_net_cfg_clean(wtk_beam_bf_net_cfg_t *cfg);
int wtk_beam_bf_net_cfg_update(wtk_beam_bf_net_cfg_t *cfg);
int wtk_beam_bf_net_cfg_update2(wtk_beam_bf_net_cfg_t *cfg,
                                wtk_source_loader_t *sl);
int wtk_beam_bf_net_cfg_update_local(wtk_beam_bf_net_cfg_t *cfg,
                                     wtk_local_cfg_t *lc);

wtk_beam_bf_net_cfg_t *wtk_beam_bf_net_cfg_new(char *fn);
void wtk_beam_bf_net_cfg_delete(wtk_beam_bf_net_cfg_t *cfg);
wtk_beam_bf_net_cfg_t *wtk_beam_bf_net_cfg_new_bin(char *fn);
void wtk_beam_bf_net_cfg_delete_bin(wtk_beam_bf_net_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
