#ifndef QBL_NN_QBL_CONV2D_H
#define QBL_NN_QBL_CONV2D_H
#pragma once
#include "qtk/core/qtk_type.h"
#include "qtk/linalg/qtk_gemm.h"
#include "qtk/nn/qtk_nn_im2col.h"
#include "qtk/nn/qtk_nn_utils.h"
#include "qtk/numeric/qtk_numeric_type.h"
#ifdef __cplusplus
extern "C" {
#endif
#define IN_RANGE(n, left, right) ((n) >= (left) && (n) <= (right))
static void nn_conv2d_pad(float *pv,float *pv_old, uint32_t *shape, int pad1, int pad2){
    int a, b, c, d;
    int h = shape[2] + pad1 * 2;
    int w = shape[3] + pad2 * 2;
    for (a = 0; a < shape[0]; a++) {
        for (b = 0; b < shape[1]; b++) {
            for (c = 0; c < h; c++) {
                for (d = 0; d < w; d++) {
                    int c1, d1;
                    c1 = c - pad1;
                    d1 = d - pad2;
                    if (IN_RANGE(c1, 0, shape[2] - 1) &&
                        IN_RANGE(d1, 0, shape[3] - 1)) {
                        *pv++ = *pv_old++;
                    } else {
                        *pv++ = 0.0;
                    }
                }
            }
        }
    }
}

void conv2d_acc(float *c, float a, float *b, int len, int stride){
    int i,j;
    for (i = 0, j = 0; i < len; i++,j+=stride){
        c[i] += a * b[j];
    }
}

void cov2d_sgemm(int M, int N, int K, float ALPHA, float *A,
    int lda, float **B, int ldb, float BETA, float *C, int ldc, int stride, int pstride) {
    int i, j, k;

    if(N % stride != 0){
        exit(0);
    }
    int n = N / stride;

    for (i = 0; i < M; i++) {
        for (j = 0; j < N; j++) {
            C[i * ldc + j] *= BETA;
        }
    }

    int xx;
    for (i = 0; i < M; ++i) {
        xx = 0;
        for (k = 0; k < K; ++k) {
            register float A_PART = ALPHA * A[i * lda + k];
            for (j = 0; j < n; ++j, ++xx) {
                conv2d_acc(C + i * ldc + j * stride, A_PART, B[xx], stride, pstride);
                //C[i * ldc + j] += A_PART * B[k * ldb + j];
            }
        }
    }
}

qtk_maybe_unused static void
nn_conv2d_(float *X, float **idx,qtk_numeric_data_t Y, qtk_numeric_data_t W,
           qtk_numeric_data_t B, int* ksize, int *stride,
           int out_channel, int groups, uint32_t *shape, int hasB) {
    int in_b = shape[0];
    int in_c = shape[1];
    int in_h = shape[2];
    int in_w = shape[3];
    int out_w = qtk_nn_pad_dim(in_w, 0, 0, ksize[1], stride[1]);
    int out_h = qtk_nn_pad_dim(in_h, 0, 0, ksize[0], stride[0]);

    int m = out_channel / groups;
    int k = ksize[0] * ksize[1] * in_c / groups;
    int n = out_w * out_h;
    float *x_f32 = X;
    float *w_f32 = W.f32;
    float *y_f32 = Y.f32;
    float *bias_f32 = B.f32;

    float **index = idx;
    int id_stride;

    for (int i = 0; i < in_b; i++) {
        for (int j = 0; j < groups;
             j++, y_f32 += n * m, x_f32 += in_c / groups * in_h * in_w) {
            float *a = w_f32 + m * k * j;
            float *b;
            float *c = y_f32;
            float *im = x_f32;
            if (ksize[0] == 1 && ksize[1] == 1) {
                b = im;
                qtk_sgemm(0, 0, m, n, k, 1.0, a, k, b, n, 0.0, c, n);
            } else {
                //im2col_cpu_(im, in_c / groups, in_h, in_w, ksize, stride, npad,
                //            b);
                im2col2_cpu_(im, in_c / groups, in_h, in_w, ksize, stride,
                            index, &id_stride);
                cov2d_sgemm(m, n, k, 1.0, a, k, index, n, 0.0, c, n, id_stride, stride[1]);
            }
        }
    }

    if(!hasB){
        return;
    }
    y_f32 = Y.f32;
    for (int i = 0; i < in_b; i++) {
        for (int j = 0; j < out_channel; j++) {
            for (int ii = 0; ii < n; ii++) {
                *y_f32++ += bias_f32[j];
            }
        }
    }
}

qtk_maybe_unused static void
nn_conv2d_naive_(float *X, float* Y,
                 qtk_numeric_data_t W, qtk_numeric_data_t B, int *pad,
                 int *ksize, int *stride, int out_channel, int groups,
                 uint32_t *shape, uint8_t *dilations,
                 float* workspace, int hasB, int prelu) {
    int in_b = shape[0];
    int in_c = shape[1];
    int in_h = shape[2];
    int in_w = shape[3];
    const int out_h =
        (shape[2] + pad[0] + pad[2] - (dilations[0] * (ksize[0] - 1) + 1)) /
            stride[0] +
        1;
    const int out_w =
        (shape[3] + pad[1] + pad[3] - (dilations[1] * (ksize[1] - 1) + 1)) /
            stride[1] +
        1;

    int m = out_channel / groups;
    int k = ksize[0] * ksize[1] * in_c / groups;
    int n = out_w * out_h;
    float *x_f32 = X;
    float *w_f32 = W.f32;
    float *y_f32 = Y;
    float *bias_f32 = B.f32;

    for (int i = 0; i < in_b; i++) {
        for (int j = 0; j < groups;
             j++, y_f32 += n * m, x_f32 += in_c / groups * in_h * in_w) {
            float *a = w_f32 + m * k * j;
            float *b = workspace;
            float *c = y_f32;
            float *im = x_f32;
            //TODO for fast matmul in im2col b is transposed
            //if (ksize[0] == 1 && ksize[1] == 1) {
            //    b = im;
            //} else {
            im2col_cpu_(im, in_c / groups, in_h, in_w, ksize, stride, pad,
                        b, dilations);
            //}
            memset(c, 0, sizeof(float) * m * n);
            qtk_sgemm(0, 1, m, n, k, 1.0, a, k, b, k, 0.0, c, n);
        }
    }

    if(prelu){
        float *slope = bias_f32 + out_channel;
        y_f32 = Y;
        for (int i = 0; i < in_b; i++) {
            for (int j = 0; j < out_channel; j++) {
                for (int ii = 0; ii < n; ii++, y_f32++) {
                    *y_f32 += bias_f32[j];
                    if(*y_f32 < 0){
                        *y_f32 *= slope[j];
                    }
                }
            }
        }
    }else if(hasB){
        y_f32 = Y;
        for (int i = 0; i < in_b; i++) {
            for (int j = 0; j < out_channel; j++) {
                for (int ii = 0; ii < n; ii++) {
                    *y_f32++ += bias_f32[j];
                }
            }
        }
    }
}

#ifdef __cplusplus
};
#endif
#endif
