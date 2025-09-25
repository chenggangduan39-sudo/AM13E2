#ifndef WTK_BFIO_MASKBFNET_WTK_BEAM_BF_NET_H
#define WTK_BFIO_MASKBFNET_WTK_BEAM_BF_NET_H
#include "qtk/nn/vm/qtk_nn_vm.h"
#include "qtk/nnrt/qtk_nnrt.h"
#include "wtk/bfio/qform/beamnet/wtk_beam_bf_net_cfg.h"
#include "wtk/bfio/maskdenoise/wtk_drft.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/core/wtk_strbuf.h"
#ifdef USE_NEON
#include "arm_neon.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_beam_bf_net wtk_beam_bf_net_t;
typedef void (*wtk_beam_bf_net_notify_f)(void *ths, short *output, int len);

struct wtk_beam_bf_net {
    wtk_beam_bf_net_cfg_t *cfg;

    wtk_strbuf_t **mic;
    wtk_strbuf_t **sp;

    float *analysis_window;
    float *synthesis_window;
    wtk_drft_t *rfft;
    float *rfft_in;
    float **analysis_mem;
    float **analysis_mem_sp;
    float *synthesis_mem;

    wtk_complex_t **fft;
    wtk_complex_t **fft_sp;
    wtk_complex_t *fftx;

    wtk_mask_bf_t *mask_bf;

    wtk_qmmse_t *qmmse2;
    qtk_ahs_gain_controller_t *gc;
    wtk_equalizer_t *eq;
    wtk_limiter_t *limiter;

    float *out;

    qtk_nn_vm_t nv1;
    float *nv1_input[20];
    float *nv1_output[20];
    int nv1_num_in;
    int *nv1_in_sizes;
    int *nv1_out_sizes;
    float *nv1_cache[20];
    int64_t nv1_idx;

    qtk_nnrt_value_t input_val[32];

    void *ths;
    wtk_beam_bf_net_notify_f notify;

    int sp_silcnt;
    int mic_silcnt;

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

    float mic_scale;
    float sp_scale;

    qtk_nnrt_t *stage1_rt;
    qtk_nnrt_value_t *stage1_inputs;

    float *time_delay;
    wtk_complex_t *mean_mat;
    wtk_complex_t **input_norm;
    wtk_complex_t **mic_covar;
    wtk_complex_t **ideal_phase_covar;
    wtk_complex_t **freq_covar;
    wtk_complex_t **ideal_phase_shift;
    float *mag;
    float *cs;
    float *mask;

    float theta;

    unsigned sp_sil : 1;
    unsigned mic_sil : 1;
    unsigned bf_start : 1;
    unsigned agc_enable : 1;
    unsigned echo_enable : 1;
    unsigned denoise_enable : 1;
};

wtk_beam_bf_net_t *wtk_beam_bf_net_new(wtk_beam_bf_net_cfg_t *cfg);
void wtk_beam_bf_net_delete(wtk_beam_bf_net_t *beam_bf_net);
void wtk_beam_bf_net_start(wtk_beam_bf_net_t *beam_bf_net, float theta, float phi);
void wtk_beam_bf_net_reset(wtk_beam_bf_net_t *beam_bf_net);
void wtk_beam_bf_net_feed(wtk_beam_bf_net_t *beam_bf_net, short *data, int len,
                          int is_end);
void wtk_beam_bf_net_set_notify(wtk_beam_bf_net_t *beam_bf_net, void *ths,
                                wtk_beam_bf_net_notify_f notify);

void wtk_beam_bf_net_set_micscale(wtk_beam_bf_net_t *beam_bf_net, float scale);
void wtk_beam_bf_net_set_agcenable(wtk_beam_bf_net_t *beam_bf_net, int enable);
void wtk_beam_bf_net_set_agclevel(wtk_beam_bf_net_t *beam_bf_net, int level);
void wtk_beam_bf_net_set_echoenable(wtk_beam_bf_net_t *beam_bf_net, int enable);
void wtk_beam_bf_net_set_denoiseenable(wtk_beam_bf_net_t *beam_bf_net,
                                       int enable);
void wtk_beam_bf_net_set_denoiselevel(wtk_beam_bf_net_t *beam_bf_net,
                                      int level);
void wtk_beam_bf_net_set_denoisesuppress(wtk_beam_bf_net_t *beam_bf_net,
                                         float suppress); //[0-1]
#ifdef __cplusplus
};
#endif
#endif
