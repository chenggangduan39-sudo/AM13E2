#ifndef QBL_MATH_QBL_MATRIX_H
#define QBL_MATH_QBL_MATRIX_H
#pragma once
#include "qtk/core/qtk_type.h"
#ifdef __cplusplus
extern "C" {
#endif

qtk_maybe_unused QTK_INLINE static void qtk_matrix_eye(float *d, int sz) {
    int i,j;
    for (i = 0; i < sz; i++) {
        for (j = 0; j < sz; j++, d++) {
            *d = i == j ? 1.0 : 0.0;
        }
    }
}

// for i = 0 to N - 2
//    for j = i + 1 to N - 1
//        swap A(i,j) with A(j,i)
qtk_maybe_unused QTK_INLINE static void
qtk_matrix_square_transpose_inplace(float *a, int n) {
   float t;
   int i,j;
   for (i = 0; i < n - 1; i++) {
        for (j = i + 1; j < n; j++) {
            t = a[i * n + j];
            a[i * n + j] = a[j * n + i];
            a[j * n + i] = t;
        }
    }
}

// c = a * b
// a => m x k
// b => k x n
// c => m x n
qtk_maybe_unused QTK_INLINE static void
qtk_matrix_dot(float *a, float *b, float *c, int m, int n, int k) {
    float *c_ptr = c;
    int i,j,d; 
    for (i = 0; i < m; i++) {
        for (j = 0; j < n; j++) {
            float sum = 0;
            for (d = 0; d < k; d++) {
                sum += a[i * k + d] * b[d * n + j];
            }
            *c_ptr++ = sum;
        }
    }
}

qtk_maybe_unused QTK_INLINE static void qtk_matrix_transposef2(float *a, float *b, int m, int n,
                                   int need_n) {
    int i,j;
    for (j = 0; j < need_n; j++) {
        for (i = 0; i < m; i++) {
            *b++ = a[i * n + j];
        }
    }
}

qtk_maybe_unused QTK_INLINE static void qtk_matrix_transposef(float *a, float *b, int m, int n) {
    qtk_matrix_transposef2(a, b, m, n, n);
}

#ifdef __cplusplus
};
#endif
#endif
