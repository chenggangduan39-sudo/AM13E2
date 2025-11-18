#ifndef WTK_BFIO_AFILTER_WTK_KALMAN
#define WTK_BFIO_AFILTER_WTK_KALMAN
#include "wtk/bfio/maskdenoise/wtk_drft.h"
#include "wtk/core/wtk_complex.h"
#include "wtk_kalman_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef void (*wtk_kalman_notify_f)(void *ths, float **output, int channel, int len);

typedef struct wtk_kalman wtk_kalman_t;
struct wtk_kalman {
    wtk_kalman_cfg_t *cfg;
    wtk_drft_t *drft;

    float *x_b;             // [wins]
    float **d_n;            // [channel, L]
    float **y_n;            // [channel, L]
    float **e_n;            // [channel, L]
    float *P_b;             // [nbin * B]
    float *Phi_SS;          // [nbin]
    float *Phi_delta;       // [nbin * B]
    float *mu_b;            // [nbin * B]
    float *half_window;     // [wins]
    float *power_block;     // B
    wtk_complex_t *X_b;     // [nbin * B]
    wtk_complex_t *K_b;     // [nbin * B]
    wtk_complex_t **W_b;    // [channel, nbin * B]
    wtk_complex_t **E;      // [channel, nbin * B]
    wtk_complex_t *fft_tmp; // [nbin * B]
    float *tmp;             // [nbin * B]
    float *rfft_in;         // [wins]

    void *ths;
    wtk_kalman_notify_f notify;

    int frame_idx;
};

wtk_kalman_t *wtk_kalman_new(wtk_kalman_cfg_t *cfg);
void wtk_kalman_delete(wtk_kalman_t *kalman);

void wtk_kalman_reset(wtk_kalman_t *kalman);
void wtk_kalman_feed(wtk_kalman_t *kalman, float **d, float *x);

void wtk_kalman_set_notify(wtk_kalman_t *kalman, void *ths,
                           wtk_kalman_notify_f notify);
#ifdef __cplusplus
};
#endif
#endif
