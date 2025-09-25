#ifndef WTK_BFIO_AHS_QTK_GAIN
#define WTK_BFIO_AHS_QTK_GAIN
#include "qtk_gain_controller_cfg.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_strbuf.h"
#include "wtk/core/wtk_heap.h"
#include "wtk/bfio/maskdenoise/wtk_drft.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_ahs_gain_controller qtk_ahs_gain_controller_t;
typedef struct qtk_gc_kalman qtk_gc_kalman_t;
typedef struct qtk_ahs_limiter qtk_ahs_limiter_t;
typedef struct qtk_gc_auto_calibration qtk_gc_auto_calibration_t;

struct qtk_gc_kalman
{
    float alpha;
    float update_thresh;
    float R_k;
    float P_k;
    float X_k;
    float Z_k;
    float Q_k;
    float H_K;
    float beta;
};

struct qtk_ahs_limiter{
    float Aa;
    float Ar;
    float p;
    float c0;
    float g1;
    float c;
    float G;
};

struct qtk_gc_auto_calibration{
    float *buffer4delay;
    float *buffer4static;
    float* milestones;
    float* cdf;
    float* pdf;
    int *bin_cnt;
    float *tmp;
    float *tmp2;
    float *tmp3;
    int *bins;

    int nfrm_delay;
    int frm_counter;
    int static_size;
    int N;
};

struct qtk_ahs_gain_controller {
    qtk_ahs_gain_controller_cfg_t *cfg;
    qtk_gc_kalman_t kalman;
    qtk_gc_auto_calibration_t auto_calibration;
    qtk_ahs_limiter_t limiter;
    wtk_drft_t *drft;

    int fft_length;
    int ft_sample_rate;
    int nband;
    int frm_idx;
    float *frm;
    float *prev_half_win;
    float Maximun_Atten;
    wtk_strbuf_t *galis;
    float G_cali;
    float gain_cali;//store the gain(mic_shift * echo_shift)
    float init_Intensity;
    int test_mode;

    float *voice_buf;
    int voice_idx;
    float voice_eng;
    int voice_cnt;
};

void qtk_ahs_limiter_init(qtk_ahs_limiter_t *limiter,float ta, float tr, float rho, float threshold, int sample_rate);
void qtk_ahs_limiter_process(qtk_ahs_limiter_t *limiter, float *data, int len);

qtk_ahs_gain_controller_t *qtk_gain_controller_new(qtk_ahs_gain_controller_cfg_t* cfg);
void qtk_gain_controller_delete(qtk_ahs_gain_controller_t *gc);
void qtk_gain_controller_reset(qtk_ahs_gain_controller_t *gc);
void qtk_gain_controller_run(qtk_ahs_gain_controller_t *gc, wtk_complex_t *data, int len, float *out_wav,float prob);
void qtk_gain_controller_set_mode(qtk_ahs_gain_controller_t *gc, int mode);
void qtk_gain_controller_update_calibration(qtk_ahs_gain_controller_t *gc, float gain);
#ifdef __cplusplus
};
#endif
#endif
