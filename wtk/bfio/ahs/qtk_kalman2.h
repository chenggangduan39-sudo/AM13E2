#ifndef WTK_BFIO_AHS_QTK_KALMAN2_H
#define WTK_BFIO_AHS_QTK_KALMAN2_H
#include "wtk/core/wtk_complex.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_strbuf.h"

#include "wtk/core/wtk_heap.h"
#include "wtk/bfio/maskdenoise/wtk_drft.h"

#include "qtk_kalman2_cfg.h"

#ifdef USE_NEON
#include <arm_neon.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_ahs_kalman2 qtk_ahs_kalman2_t;


struct qtk_ahs_kalman2 {
    qtk_ahs_kalman2_cfg_t* cfg;
    wtk_drft_t *drft;
    
    //input
    float *x_b;
    wtk_complex_t *X_b;

    //km
    float *Phi_ss;
    float *P_b;
    wtk_complex_t *W_b;
    float *mu_b;
    wtk_complex_t *E;
    wtk_complex_t *E_kb;
    wtk_complex_t *fft_buf;
    float *ifft_buf;
    int *half_window;

    float *Phi_EE;

    //output
    float *y;
    float *e;
    wtk_complex_t *Y;
    float *W;
    wtk_complex_t *E_res;
    //wtk_complex_t *E_res; //replace by W

    float A2;
    float Phi_SS_smooth_factor;
    int M;
    int L;
    int B;
};

qtk_ahs_kalman2_t *qtk_kalman2_new(qtk_ahs_kalman2_cfg_t *cfg);
void qtk_kalman2_delete(qtk_ahs_kalman2_t *km);
void qtk_kalman2_reset(qtk_ahs_kalman2_t *km);
void qtk_kalman2_update(qtk_ahs_kalman2_t *km);
void qtk_kalman2_feed(qtk_ahs_kalman2_t *km, float *x, float *d);
#ifdef __cplusplus
};
#endif
#endif
