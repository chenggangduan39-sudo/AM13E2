static void gemm_nn_(int M, int N, int K, float ALPHA, float *A, int lda,
                     float *B, int ldb, float *C, int ldc) {
    int i, j, k;
    for (i = 0; i < M; ++i) {
        for (k = 0; k < K; ++k) {
            register float A_PART = ALPHA * A[i * lda + k];
            for (j = 0; j < N; ++j) {
                C[i * ldc + j] += A_PART * B[k * ldb + j];
            }
        }
    }
}

static void gemm_nt_(int M, int N, int K, float ALPHA, float *A, int lda,
                     float *B, int ldb, float *C, int ldc) {
    int i, j, k;
    for (i = 0; i < M; ++i) {
        for (j = 0; j < N; ++j) {
            register float sum = 0;
            for (k = 0; k < K; ++k) {
                sum += ALPHA * A[i * lda + k] * B[j * ldb + k];
            }
            C[i * ldc + j] += sum;
        }
    }
}

static void gemm_tn_(int M, int N, int K, float ALPHA, float *A, int lda,
                     float *B, int ldb, float *C, int ldc) {
    int i, j, k;
    for (i = 0; i < M; ++i) {
        for (k = 0; k < K; ++k) {
            register float A_PART = ALPHA * A[k * lda + i];
            for (j = 0; j < N; ++j) {
                C[i * ldc + j] += A_PART * B[k * ldb + j];
            }
        }
    }
}

static void gemm_tt_(int M, int N, int K, float ALPHA, float *A, int lda,
                     float *B, int ldb, float *C, int ldc) {
    int i, j, k;
    for (i = 0; i < M; ++i) {
        for (j = 0; j < N; ++j) {
            register float sum = 0;
            for (k = 0; k < K; ++k) {
                sum += ALPHA * A[i + k * lda] * B[k + j * ldb];
            }
            C[i * ldc + j] += sum;
        }
    }
}

void qtk_sgemm(int TA, int TB, int M, int N, int K, float ALPHA, float *A,
               int lda, float *B, int ldb, float BETA, float *C, int ldc) {
    int i, j;
    for (i = 0; i < M; i++) {
        for (j = 0; j < N; j++) {
            C[i * ldc + j] *= BETA;
        }
    }

    if (!TA && !TB) {
        gemm_nn_(M, N, K, ALPHA, A, lda, B, ldb, C, ldc);
    } else if (TA && !TB) {
        gemm_tn_(M, N, K, ALPHA, A, lda, B, ldb, C, ldc);
    } else if (!TA && TB) {
        gemm_nt_(M, N, K, ALPHA, A, lda, B, ldb, C, ldc);
    } else {
        gemm_tt_(M, N, K, ALPHA, A, lda, B, ldb, C, ldc);
    }
}
