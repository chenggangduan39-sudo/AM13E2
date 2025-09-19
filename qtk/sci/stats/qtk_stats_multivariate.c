#include "qtk/sci/stats/qtk_stats_multivariate.h"
#include "wtk/core/math/wtk_math.h"

extern int sgetri_(int *n, float *a, int *lda, int *ipiv, float *work,
                   int *lwork, int *info);

extern int sgetrf_(int *m, int *n, float *a, int *lda, int *ipiv, int *info);

float qtk_stats_multivariate_pdf(const float *x, const float *mu,
                                 const float *sigma, int sz, float *workspace,
                                 int *cap, int *info) {
    float det;
    float norm_const;
    int lwork_sz = sz + sz + sz * sz + sz;
    float *x_mu = workspace;
    float *inv_work = x_mu + sz;
    float *sigma_inv = inv_work + sz;
    int *ipiv = (int *)(sigma_inv + sz * sz);
    float f = 0;

    *info = -1;

    if (!workspace) {
        *cap = lwork_sz;
        return 0;
    }

    if (*cap < lwork_sz) {
        return 0;
    }

    memcpy(sigma_inv, sigma, sizeof(float) * sz * sz);
    for (int i = 0; i < sz; i++) {
        x_mu[i] = x[i] - mu[i];
    }
    sgetrf_(&sz, &sz, sigma_inv, &sz, ipiv, info);
    if (*info != 0) {
        return 0;
    }
    det = 1;
    for (int i = 0; i < sz; i++) {
        det *= sigma_inv[i * sz + i];
    }
    norm_const = 1.0 / (pow((2 * PI), (float)sz / 2) * sqrt(det));
    sgetri_(&sz, sigma_inv, &sz, ipiv, inv_work, &sz, info);
    if (*info != 0) {
        return 0;
    }
    for (int i = 0; i < sz; i++) {
        float sum = 0;
        for (int j = 0; j < sz; j++) {
            sum += x_mu[j] * sigma_inv[j * sz + i];
        }
        f += x_mu[i] * sum;
    }
    *info = 0;
    return norm_const * exp(-0.5 * f);
}