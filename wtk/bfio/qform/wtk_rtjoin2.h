#ifndef WTK_BFIO_QFORM_WTK_RTJOIN2
#define WTK_BFIO_QFORM_WTK_RTJOIN2
#include "wtk/bfio/maskdenoise/wtk_drft.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/core/wtk_strbuf.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk_rtjoin2_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_rtjoin2 wtk_rtjoin2_t;
typedef void (*wtk_rtjoin2_notify_f)(void *ths, short *output, int len);

typedef struct {
    wtk_rtjoin2_cfg_t *cfg;
    int nbin;

    wtk_bankfeat_t *bank_mic;
    wtk_bankfeat_t *bank_sp;

    float *feature_sp;

    float *g;
    float *gf;

    wtk_gainnet2_t *gainnet2;
} wtk_rtjoin2_edra_t;

struct wtk_rtjoin2 {
    wtk_rtjoin2_cfg_t *cfg;

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
    float **mul_synthesis_mem;

    wtk_complex_t **fft;
    wtk_complex_t **fft_sp;
    wtk_complex_t *fftx;
    wtk_complex_t *ffty;
    float *mul_out;
    float *out;

    float *kernel;

    wtk_drft_t *drft2;
    wtk_complex_t *kernel_fft;
    wtk_complex_t *data_fft;
    wtk_complex_t *res_fft;
    float *kernel_tmp;
    float *data_tmp;
    float *res_tmp;
    int sp_frame;

    wtk_drft_t *drft3;
    wtk_complex_t *a_kernel_fft;
    wtk_complex_t *a_data_fft;
    wtk_complex_t *a_res_fft;
    float *a_kernel_tmp;
    float *a_data_tmp;
    float *a_res_tmp;
    float **a_res;
    float **align_res;
    float **raw_sig;
    int *align_cnt;
    int align_frame;
    int *align_max_idx;
    float *align_max_value;
    float **min_data;
    int **align_delay;
    int play_cnt;

    float *rob_prob_mu;
    float *rob_prob_sigma;
    float *rob_long_mu;
    float *rob_long_sigma;
    float *rob_long_mad;
    float **rob_values;
    float **rob_weights;
    float **rob_values_tmp;
    float **rob_weights_tmp;
    float *residuals;
    int *values_len;
    int **delay_idx;
    int *delay_idx_len;
    int *real_delay;
    int *real_delay_start;

    float **local_power;
    float *local_power_sum;
    float *power;
    float *local_weight;
    float *weight;

    wtk_equalizer_t *eq;

    qtk_ahs_gain_controller_t *gc;
    int gc_cnt;

    wtk_rtjoin2_edra_t *vdr;

    float bs_scale;
    float bs_last_scale;
    float bs_real_scale;
    int bs_max_cnt;
    float *bs_win;

    void *ths;
    wtk_rtjoin2_notify_f notify;

    int sp_silcnt;
    int mic_silcnt;

    int *align_state;
    int align_pos;

    unsigned sp_sil : 1;
    unsigned mic_sil : 1;
    unsigned align_complete : 1;
    unsigned delay_complete : 1;
};

wtk_rtjoin2_t *wtk_rtjoin2_new(wtk_rtjoin2_cfg_t *cfg);
void wtk_rtjoin2_delete(wtk_rtjoin2_t *rtjoin2);
void wtk_rtjoin2_start(wtk_rtjoin2_t *rtjoin2);
void wtk_rtjoin2_reset(wtk_rtjoin2_t *rtjoin2);
void wtk_rtjoin2_set_notify(wtk_rtjoin2_t *rtjoin2, void *ths,
                            wtk_rtjoin2_notify_f notify);
/**
 * len=mic array samples
 */
void wtk_rtjoin2_feed(wtk_rtjoin2_t *rtjoin2, short *data, int len, int is_end);

short *wtk_rtjoin2_get_play_signal(wtk_rtjoin2_t *rtjoin2, int *len);
#ifdef __cplusplus
};
#endif
#endif
