#ifndef WTK_BFIO_AHS_QTK_KALMANV3
#define WTK_BFIO_AHS_QTK_KALMANV3
#include "wtk/core/wtk_complex.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_strbuf.h"

#include "wtk/core/wtk_heap.h"
#include "wtk/core/fft/wtk_rfft.h"

#include "qtk_kalman_cfg.h"

#ifdef USE_NEON
#include <arm_neon.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_res_cancel qtk_res_cancel_t;
typedef struct qtk_ahs_kalman qtk_ahs_kalman_t;

struct qtk_res_cancel {
    float *Se;
    wtk_complex_t *Xed;
    float *hnled;
    float gamma;
    int nbin;
};

struct qtk_ahs_kalman {
    qtk_ahs_kalman_cfg_t* cfg;
    int nbin;
    //cache val
    wtk_complex_t *x_cache;
    wtk_complex_t *h_w;
    wtk_complex_t *Ph;
    float *Pv;
    float *Pw;
    int x_cache_shape[2];
    int h_w_shape[3];
    int Ph_shape[3];
    int Pv_shape[2];
    int Pw_shape[3];
    int x_cache_len;

    //tmp val
    float *pweyetmp;//use Ph_shape
    wtk_complex_t *x_inp;
    wtk_complex_t *e;
    wtk_complex_t *r;
    wtk_complex_t *r2;
    wtk_complex_t *kg;
    wtk_complex_t *tmp1;
    wtk_complex_t *tmp2;
    wtk_complex_t *tmp3;
    //v3 new
    wtk_complex_t *obser;
    wtk_complex_t *NonLinear_SPEC;
    float *fft_tmp;

    int x_inp_len;

    //output val
    wtk_complex_t *lsty_hat;
    wtk_complex_t *s_hat;

    qtk_res_cancel_t *rc;

    int dg_cnt;
    int dg_row;
    int dg_col;

    //params
    int L_;
    int L;
    int n;
    float pworg;
    float pvorg;
    float a;
    float pwtmp;

    float kalman_thresh;
    int nonlinear_order;

    int *forPh;

    float mean;
    float* sum;
    int last_idx;
    int type;

    float *Phi_SS;
    float *Phi_EE;
    float *Wiener;
    float *HPH;
};
qtk_res_cancel_t* _res_cancel_new(int nbin,float gamma);
void _res_cancel_delete(qtk_res_cancel_t *rc);
void _res_cancel_forward(qtk_res_cancel_t *rc,wtk_complex_t *out_frm, wtk_complex_t *ref_frm);

qtk_ahs_kalman_t *qtk_kalman_new(qtk_ahs_kalman_cfg_t *cfg, int batchsize, int nbin);
void qtk_kalman_delete(qtk_ahs_kalman_t *km);
void qtk_kalman_reset(qtk_ahs_kalman_t *km);
void qtk_kalman_update(qtk_ahs_kalman_t *km, wtk_complex_t *y, int nbin, int batchsize, wtk_complex_t *x_fft);

void qtk_kalman_update2_3(qtk_ahs_kalman_t *km, wtk_complex_t *y,
                        int nbin, int batchsize,
                        wtk_complex_t *x_fft);

void qtk_kalman_update2_4(qtk_ahs_kalman_t *km, wtk_complex_t *y,
                        int nbin, int batchsize,
                        wtk_complex_t *x_fft);


#ifdef __cplusplus
};
#endif
#endif
