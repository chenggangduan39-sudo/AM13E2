#include "qtk/sci/filter/qtk_filter_kalman.h"
#include "qtk/sci/stats/qtk_stats_multivariate.h"
#include "wtk/core/wtk_alloc.h"
#include "qtk/core/qtk_type.h"
#include "qtk/math/qtk_matrix.h"
#include "qtk/math/qtk_vector.h"

#include <string.h>

extern int sgemm_(char *, char *, int *, int *, int *, float *, float *, int *,
                  float *, int *, float *, float *, int *);
extern int sgetri_(int *n, float *a, int *lda, int *ipiv, float *work,
                   int *lwork, int *info);
extern int sgetrf_(int *m, int *n, float *a, int *lda, int *ipiv, int *info);

static void kalman_set_default_(qtk_filter_kalman_t *k) {
    memset(k->x, 0, sizeof(float) * k->dim_x);
    qtk_matrix_eye(k->P, k->dim_x);
    qtk_matrix_eye(k->Q, k->dim_x);
    qtk_matrix_eye(k->F, k->dim_x);
    memset(k->H, 0, sizeof(float) * k->dim_z * k->dim_x);
    qtk_matrix_eye(k->R, k->dim_z);
    memset(k->M, 0, sizeof(float) * k->dim_z * k->dim_z);
    qtk_matrix_eye(k->I, k->dim_x);
}

static void kalman_set_workspace_(qtk_filter_kalman_t *k) {
    int max_sz;
    max_sz = k->dim_x * k->dim_x * sizeof(float);

    {
        int ipiv = k->dim_z;
        float work[1];
        int N = k->dim_z;
        int info = -1;
        int lwork = -1;
        sgetri_(&N, NULL, &N, NULL, work, &lwork, &info);
        qtk_assert(info == 0);
        // calc SI && calc PHT
        lwork = work[0];
        max_sz = max(max_sz, sizeof(float) * (lwork + k->dim_x * k->dim_z) +
                                 sizeof(int) * ipiv);
        k->si_inv_lwork = lwork;
    }

    {
        // I_KH && I_KHP && KR
        max_sz = max(max_sz, sizeof(float) * (k->dim_x * k->dim_x * 2 +
                                              k->dim_x * k->dim_z));
    }

    {
        int pdf_work_sz = 0;
        int max_pdf_work_sz;
        int info;
        qtk_stats_multivariate_pdf(NULL, NULL, NULL, k->dim_x, NULL,
                                   &max_pdf_work_sz, &info);
        qtk_stats_multivariate_pdf(NULL, NULL, NULL, k->dim_z, NULL,
                                   &pdf_work_sz, &info);
        max_pdf_work_sz = max(max_pdf_work_sz, pdf_work_sz);
        k->pdf_lwork = max_pdf_work_sz;
        max_sz = max(max_sz, max_pdf_work_sz + k->dim_x);
    }

    k->workspace = wtk_malloc(max_sz);
}

int qtk_filter_kalman_init(qtk_filter_kalman_t *k, int dim_x, int dim_z) {
    k->dim_x = dim_x;
    k->dim_z = dim_z;

    k->x = wtk_malloc(sizeof(float) * dim_x);
    k->z = wtk_malloc(sizeof(float) * dim_z);
    k->P = wtk_malloc(sizeof(float) * dim_x * dim_x);
    k->Q = wtk_malloc(sizeof(float) * dim_x * dim_x);
    k->F = wtk_malloc(sizeof(float) * dim_x * dim_x);
    k->H = wtk_malloc(sizeof(float) * dim_z * dim_x);
    k->R = wtk_malloc(sizeof(float) * dim_z * dim_z);
    k->M = wtk_malloc(sizeof(float) * dim_z * dim_z);
    k->y = wtk_malloc(sizeof(float) * dim_z);
    k->S = wtk_malloc(sizeof(float) * dim_z * dim_z);
    k->SI = wtk_malloc(sizeof(float) * dim_z * dim_z);
    k->K = wtk_malloc(sizeof(float) * dim_x * dim_z);
    k->I = wtk_malloc(sizeof(float) * dim_x * dim_x);
    k->alpha_sq = 1;

    kalman_set_workspace_(k);
    kalman_set_default_(k);

    return 0;
}

void qtk_filter_kalman_reset(qtk_filter_kalman_t *k) { kalman_set_default_(k); }

void qtk_filter_kalman_clean(qtk_filter_kalman_t *k) {
    wtk_free(k->x);
    wtk_free(k->z);
    wtk_free(k->P);
    wtk_free(k->Q);
    wtk_free(k->F);
    wtk_free(k->H);
    wtk_free(k->R);
    wtk_free(k->M);
    wtk_free(k->y);
    wtk_free(k->S);
    wtk_free(k->SI);
    wtk_free(k->K);
    wtk_free(k->I);
    wtk_free(k->workspace);
}

void qtk_filter_kalman_predict(qtk_filter_kalman_t *k) {
    char T = 'T';
    char N = 'N';
    float alpha;
    float beta;
    float *x;
    float *FP;
    int one = 1;

    x = cast(float *, k->workspace);
    memcpy(x, k->x, sizeof(float) * k->dim_x);

    // x = Fx
    alpha = 1.0;
    beta = 0.0;
    sgemm_(&N, &N, &k->dim_x, &one, &k->dim_x, &alpha, k->F, &k->dim_x, x,
           &k->dim_x, &beta, k->x, &k->dim_x);

    // P = alpha_sq * FPF' + Q
    FP = cast(float *, k->workspace);
    sgemm_(&N, &N, &k->dim_x, &k->dim_x, &k->dim_x, &alpha, k->F, &k->dim_x,
           k->P, &k->dim_x, &beta, FP, &k->dim_x);

    memcpy(k->P, k->Q, sizeof(float) * k->dim_x * k->dim_x);

    alpha = k->alpha_sq;
    beta = 1.0;
    sgemm_(&N, &T, &k->dim_x, &k->dim_x, &k->dim_x, &alpha, FP, &k->dim_x, k->F,
           &k->dim_x, &beta, k->P, &k->dim_x);
}

void qtk_filter_kalman_update(qtk_filter_kalman_t *k, float *z) {
    char T = 'T';
    char N = 'N';
    float alpha, beta;
    float *PHT, *I_KH, *I_KHP, *KR, *x_add, *pdf_work;
    int info;

    int one = 1;
    alpha = -1.0;
    beta = 1.0;

    if (z == NULL) {
        memset(k->y, 0, sizeof(k->y[0]) * k->dim_z);
        return;
    }

    memcpy(k->y, z, sizeof(float) * k->dim_z);
    // y = -Hx + z
    sgemm_(&N, &N, &k->dim_z, &one, &k->dim_x, &alpha, k->H, &k->dim_z, k->x,
           &k->dim_x, &beta, k->y, &k->dim_z);

    alpha = 1.0;
    beta = 0.0;

    PHT = cast(float *, k->workspace);
    // PHT = P * H'
    sgemm_(&N, &T, &k->dim_x, &k->dim_z, &k->dim_x, &alpha, k->P, &k->dim_x,
           k->H, &k->dim_z, &beta, PHT, &k->dim_x);

    alpha = 1.0;
    beta = 1.0;

    memcpy(k->S, k->R, sizeof(float) * k->dim_z * k->dim_z);
    // S = HPH' + R = H{PHT} + R
    sgemm_(&N, &N, &k->dim_z, &k->dim_z, &k->dim_x, &alpha, k->H, &k->dim_z,
           PHT, &k->dim_x, &beta, k->S, &k->dim_z);

    // SI = inv(S)
    {
        int *ipiv = cast(int *, PHT + k->dim_x * k->dim_z);
        float *work = cast(float *, ipiv + k->dim_z);
        int info, n;
        memcpy(k->SI, k->S, sizeof(float) * k->dim_z * k->dim_z);
        n = k->dim_z;
        sgetrf_(&n, &n, k->SI, &n, ipiv, &info);
        qtk_assert(info == 0);
        sgetri_(&n, k->SI, &n, ipiv, work, &k->si_inv_lwork, &info);
        qtk_assert(info == 0);
    }

    // K = PH'inv(S) = {PHT}{SI}
    alpha = 1.0;
    beta = 0.0;
    sgemm_(&N, &N, &k->dim_x, &k->dim_z, &k->dim_z, &alpha, PHT, &k->dim_x,
           k->SI, &k->dim_z, &beta, k->K, &k->dim_x);
    // PHT lifetime end

    x_add = cast(float *, k->workspace);
    pdf_work = x_add + k->dim_x;
    memcpy(x_add, k->x, sizeof(float) * k->dim_x);
    // x = Ky + x
    alpha = 1.0;
    beta = 1.0;
    sgemm_(&N, &N, &k->dim_x, &one, &k->dim_z, &alpha, k->K, &k->dim_x, k->y,
           &k->dim_z, &beta, k->x, &k->dim_x);
    k->P1 = qtk_stats_multivariate_pdf(k->x, x_add, k->Q, k->dim_x, pdf_work,
                                       &k->pdf_lwork, &info);
    // z = Hx
    alpha = 1.0;
    beta = 0.0;
    sgemm_(&N, &N, &k->dim_z, &one, &k->dim_x, &alpha, k->H, &k->dim_z, k->x,
           &k->dim_x, &beta, k->z, &k->dim_z);
    pdf_work = cast(float *, k->workspace);
    k->P2 = qtk_stats_multivariate_pdf(z, k->z, k->R, k->dim_z, pdf_work,
                                       &k->pdf_lwork, &info);

    alpha = -1.0;
    beta = 1.0;
    I_KH = cast(float *, k->workspace);
    memcpy(I_KH, k->I, sizeof(float) * k->dim_x * k->dim_x);
    I_KHP = I_KH + k->dim_x * k->dim_x;
    KR = I_KHP + k->dim_x * k->dim_x;

    // P = (I - KH)P(I - KH)' + KRK' = {I_KH}P{I_KH}' + KRK' = {I_KHP}{I_KH}' +
    // {KR}K' step1: calc I_KH
    sgemm_(&N, &N, &k->dim_x, &k->dim_x, &k->dim_z, &alpha, k->K, &k->dim_x,
           k->H, &k->dim_z, &beta, I_KH, &k->dim_x);
    alpha = 1.0;
    beta = 0.0;
    // step2: calc I_KHP = {I_KH}P
    sgemm_(&N, &N, &k->dim_x, &k->dim_x, &k->dim_x, &alpha, I_KH, &k->dim_x,
           k->P, &k->dim_x, &beta, I_KHP, &k->dim_x);
    // step3: calc KR
    sgemm_(&N, &N, &k->dim_x, &k->dim_z, &k->dim_z, &alpha, k->K, &k->dim_x,
           k->R, &k->dim_z, &beta, KR, &k->dim_x);
    // step4: calc P = {KR}K'
    sgemm_(&N, &T, &k->dim_x, &k->dim_x, &k->dim_z, &alpha, KR, &k->dim_x, k->K,
           &k->dim_x, &beta, k->P, &k->dim_x);
    // step5: calc P = {I_KHP}{I_KH}' + P
    beta = 1.0;
    sgemm_(&N, &T, &k->dim_x, &k->dim_x, &k->dim_x, &alpha, I_KHP, &k->dim_x,
           I_KH, &k->dim_x, &beta, k->P, &k->dim_x);
}
