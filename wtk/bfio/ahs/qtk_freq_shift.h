#ifndef WTK_BFIO_AHS_QTK_FREQ_SHIFT
#define WTK_BFIO_AHS_QTK_FREQ_SHIFT
#include "wtk/core/math/wtk_math.h"
#include "wtk/bfio/maskdenoise/wtk_drft.h"
#include "wtk/bfio/ahs/qtk_freq_shift_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_ahs_freq_shift qtk_ahs_freq_shift_t;

struct qtk_ahs_freq_shift {
    qtk_ahs_freq_shift_cfg_t *cfg;
    wtk_drft_t *drft;
    float *q_tap;
    wtk_complex_t *Q_TAP;
    wtk_complex_t *tmp_cpx;
    float *i_tap;
    float *blackman_win;
    int sample_rate;
    int df;
    int N_tap;
    float *I_cache;
    float *Q_cache;
    float *tmp;
    float *I;
    float *Q;
    float *theta;
    int offset;
    int chunk_size;
    int n;
};

qtk_ahs_freq_shift_t *qtk_freq_shift_new(qtk_ahs_freq_shift_cfg_t* cfg);
void qtk_freq_shift_delete(qtk_ahs_freq_shift_t *freq_shift);
void qtk_freq_shift_reset(qtk_ahs_freq_shift_t *freq_shift);
void qtk_freq_shift_feed(qtk_ahs_freq_shift_t *freq_shift, float *data);
void qtk_freq_shift_feed2(qtk_ahs_freq_shift_t *freq_shift, short *data);
#ifdef __cplusplus
};
#endif
#endif