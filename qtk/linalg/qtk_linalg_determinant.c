#include "qtk_linalg_determinant.h"
#include "qtk/math/qtk_math.h"

extern int sgetrf_(int *m, int *n, float *a, int *lda, int *ipiv, int *info);

// workspace bytes >= sizeof(int) * N
float qtk_linalg_determinant(float *matrix, int N, void *workspace, int *succ) {
    int i;
    int *ipiv = (int *)workspace;
    float det = 1;
    int sign = 1;
    sgetrf_(&N, &N, matrix, &N, ipiv, succ);
    if (*succ != 0) {
        return NAN;
    }
    for (i = 0; i < N; i++) {
        det *= matrix[i * N + i];
        if (ipiv[i] != i + 1) {
            sign *= -1;
        }
    }
    det *= sign;
    return det;
}
