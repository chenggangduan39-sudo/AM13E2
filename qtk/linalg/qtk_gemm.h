#ifndef G_RQL9ZE7J_DZQT_5PM6_OJXZ_5BWTVOKHW6KP
#define G_RQL9ZE7J_DZQT_5PM6_OJXZ_5BWTVOKHW6KP
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define qtk_linalg_sgemm(a, b, c, m, k, n)                                     \
    qtk_sgemm(0, 0, m, n, k, 1.0, a, k, b, n, 0, c, n)

void qtk_sgemm(int TA, int TB, int M, int N, int K, float ALPHA, float *A,
               int lda, float *B, int ldb, float BETA, float *C, int ldc);

#ifdef __cplusplus
};
#endif
#endif /* G_RQL9ZE7J_DZQT_5PM6_OJXZ_5BWTVOKHW6KP */
