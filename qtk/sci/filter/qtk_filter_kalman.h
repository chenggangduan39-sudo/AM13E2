#ifndef QBL_SCI_FILTER_QBL_FILTER_KALMAN_H
#define QBL_SCI_FILTER_QBL_FILTER_KALMAN_H
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_filter_kalman qtk_filter_kalman_t;

struct qtk_filter_kalman {
    int dim_x; // number of state variables
    int dim_z; // number of measurement inputs

    float *x;
    float *z;
    float *P;
    float *Q;
    float *F;
    float *H;
    float *R;
    float *M;
    float *y;
    float *S;
    float *SI;
    float *K;
    float *I;

    float P1;
    float P2;

    float alpha_sq;

    void *workspace;
    int si_inv_lwork;
    int pdf_lwork;
};

int qtk_filter_kalman_init(qtk_filter_kalman_t *k, int dim_x, int dim_z);
void qtk_filter_kalman_clean(qtk_filter_kalman_t *k);
void qtk_filter_kalman_predict(qtk_filter_kalman_t *k);
void qtk_filter_kalman_update(qtk_filter_kalman_t *k, float *z);
void qtk_filter_kalman_reset(qtk_filter_kalman_t *k);

#ifdef __cplusplus
};
#endif
#endif
