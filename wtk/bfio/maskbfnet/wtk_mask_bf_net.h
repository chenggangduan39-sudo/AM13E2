#ifndef WTK_BFIO_MASKBFNET_WTK_MASK_BF_NET_H
#define WTK_BFIO_MASKBFNET_WTK_MASK_BF_NET_H
#include "qtk/nn/vm/qtk_nn_vm.h"
#include "qtk/nnrt/qtk_nnrt.h"
#include "wtk/bfio/maskbfnet/wtk_mask_bf_net_cfg.h"
#include "wtk/bfio/maskdenoise/wtk_drft.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/core/wtk_strbuf.h"
#ifdef USE_NEON
#include "arm_neon.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_mask_bf_net wtk_mask_bf_net_t;
typedef void (*wtk_mask_bf_net_notify_f)(void *ths, short *output, int len);

typedef struct {
    wtk_mask_bf_net_cfg_t *cfg;
    int nbin;

    wtk_bankfeat_t *bank_mic;
    wtk_bankfeat_t *bank_sp;

    float *feature_sp;

    float *g;
    float *lastg;
    float *gf;

    wtk_gainnet2_t *gainnet2;
} wtk_mask_bf_net_edr_t;

struct wtk_mask_bf_net {
    wtk_mask_bf_net_cfg_t *cfg;

    wtk_strbuf_t **mic;
    wtk_strbuf_t **sp;

    float *analysis_window;
    float *synthesis_window;
    wtk_drft_t *rfft;
    float *rfft_in;
    int nbin;
    float **analysis_mem;
    float **analysis_mem_sp;
    float *synthesis_mem;

    wtk_complex_t **fft;
    wtk_complex_t **fft_sp;
    wtk_complex_t **fft_chn;

    wtk_rls3_t *erls3;

    wtk_covm_t *covm;
    wtk_covm_t *echo_covm;
    wtk_bf_t *bf;

    wtk_complex_t *fftx;
    wtk_complex_t *ffty;
    wtk_complex_t *fft_tmp;

    float *entropy_E;
    float *entropy_Eb;

    float *pre_alpha;

    float *mask;
    float *m_mask;
    float *eta;
    float *epsi;
    wtk_complex_t **vec;
    wtk_complex_t **vec2;
    float *t;
    wtk_complex_t *scov_tmp;
    wtk_complex_t *c_temp;

    wtk_complex_t *ncov_tmp;

    int update_w;
    float *eng;

    wtk_complex_t ***ovec;

    wtk_complex_t **speech_covar_norm;
    float *speech_power_acc;
    float *noise_power_acc;
    float spower_cnt;
    float npower_cnt;
    wtk_complex_t **noise_covar;
    float *scnt;
    float *ncnt;

    float *alpha_bin;
    float *beta_bin;
    float snr_acc;

    float **power_channel;
    float *speech_power_channel;
    float *noise_power_channel;

    wtk_complex_t **covar;
    float *noise_mask;

    float *snr;
    int refchannel;

    wtk_mask_bf_net_edr_t *vdr;
    wtk_mask_bf_net_edr_t *vdr2;

    wtk_qmmse_t *qmmse;
    float *qmmse_gain;
    wtk_qmmse_t *qmmse2;
    qtk_ahs_gain_controller_t *gc;
    wtk_equalizer_t *eq;
    wtk_limiter_t *limiter;

    float *out;

    qtk_nn_vm_t nv1;
    qtk_nn_vm_t nv2;
    qtk_nn_vm_t nv3;
    float *nv1_input[20];
    float *nv1_output[20];
    float *nv2_input[20];
    float *nv2_output[20];
    float *nv3_input[20];
    float *nv3_output[20];
    int nv1_num_in;
    int nv2_num_in;
    int nv3_num_in;
    int *nv1_in_sizes;
    int *nv2_in_sizes;
    int *nv3_in_sizes;
    int *nv1_out_sizes;
    int *nv2_out_sizes;
    int *nv3_out_sizes;
    float *nv1_cache[20];
    float *nv2_cache[20];
    float *nv3_cache[20];
    int64_t nv1_idx;
    int64_t nv2_idx;
    int64_t nv3_idx;

    float *err;
    float *ee;
    float *x_phase;
    float *model_mask;

    qtk_nnrt_value_t input_val[32];

    void *ths;
    wtk_mask_bf_net_notify_f notify;

    float *sim_scov;
    float *sim_ncov;
    float *sim_echo_scov;
    float *sim_echo_ncov;
    int *sim_cnt_sum;
    int *sim_echo_cnt_sum;
    int bf2_alpha_cnt;
    float bf2_alpha;

    float *mask_mu;
    float *mask_mu2;
    float *mask_tmp;

    int sp_silcnt;
    int mic_silcnt;
    int entropy_silcnt;
    int entropy_in_cnt;
    int entropy_sp_silcnt;
    int entropy_sp_in_cnt;

    float bs_scale;
    float bs_last_scale;
    float bs_real_scale;
    int bs_max_cnt;
    float *bs_win;

    int nframe;
    int gc_cnt;
    float *gc_mask;

    int sum_sp_sil;
    int *sp_state;

    int bf_init_frame;

    float mic_scale;
    float sp_scale;

    qtk_nnrt_t *stage1_rt;
    qtk_nnrt_t *stage2_rt;
    qtk_nnrt_t *dr_stage2_rt;

    qtk_nnrt_value_t *stage1_inputs;
    qtk_nnrt_value_t *stage2_inputs;
    qtk_nnrt_value_t *dr_stage2_inputs;

    unsigned sp_sil : 1;
    unsigned mic_sil : 1;
    unsigned entropy_sil : 1;
    unsigned entropy_sp_sil : 1;
    unsigned bf_start : 1;
    unsigned agc_enable : 1;
    unsigned echo_enable : 1;
    unsigned denoise_enable : 1;
    unsigned echo_in : 1;
    unsigned echo_out : 1;
};

wtk_mask_bf_net_t *wtk_mask_bf_net_new(wtk_mask_bf_net_cfg_t *cfg);
void wtk_mask_bf_net_delete(wtk_mask_bf_net_t *mask_bf_net);
void wtk_mask_bf_net_start(wtk_mask_bf_net_t *mask_bf_net);
void wtk_mask_bf_net_reset(wtk_mask_bf_net_t *mask_bf_net);
void wtk_mask_bf_net_feed(wtk_mask_bf_net_t *mask_bf_net, short *data, int len,
                          int is_end);
void wtk_mask_bf_net_set_notify(wtk_mask_bf_net_t *mask_bf_net, void *ths,
                                wtk_mask_bf_net_notify_f notify);

void wtk_mask_bf_net_set_micscale(wtk_mask_bf_net_t *mask_bf_net, float scale);
void wtk_mask_bf_net_set_agcenable(wtk_mask_bf_net_t *mask_bf_net, int enable);
void wtk_mask_bf_net_set_agclevel(wtk_mask_bf_net_t *mask_bf_net, int level);
void wtk_mask_bf_net_set_echoenable(wtk_mask_bf_net_t *mask_bf_net, int enable);
void wtk_mask_bf_net_set_denoiseenable(wtk_mask_bf_net_t *mask_bf_net,
                                       int enable);
void wtk_mask_bf_net_set_denoiselevel(wtk_mask_bf_net_t *mask_bf_net,
                                      int level);
void wtk_mask_bf_net_set_denoisesuppress(wtk_mask_bf_net_t *mask_bf_net,
                                         float suppress); //[0-1]
#ifdef __cplusplus
};
#endif
#endif
