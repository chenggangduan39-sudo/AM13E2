#ifndef QBL_NN_QBL_CONV2DQ_H
#define QBL_NN_QBL_CONV2DQ_H
#pragma once
#include "qtk/nn/qtk_nn_im2col_quant.h"
#include "qtk/nn/qtk_nn_utils.h"
#include "qtk/numeric/qtk_numeric_type.h"
#ifdef __cplusplus
extern "C" {
#endif
#define IN_RANGE(n, left, right) ((n) >= (left) && (n) <= (right))
static void nn_conv2d_pad_quant(int16_t *pv,uint8_t *pv_old, uint32_t *shape, int pad1, int pad2, uint8_t zero_point){
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
                        *pv = *pv_old++ - zero_point;//TODO
                        pv++;
                    } else {
                        *pv++ = 0;//TODO
                    }
                }
            }
        }
    }
}

void conv2d_acc_quant(int *c, int a, int16_t *b, int len, int stride){
    int i,j;
    for (i = 0, j = 0; i < len; i++,j+=stride){
        c[i] += a * b[j];
    }
}

void cov2d_sgemm_quant(int M, int N, int K, uint8_t *A,
    int lda, int16_t **B, int ldb, int *C, int ldc, int stride, int pstride, uint8_t zero_point) {
    int i, j, k;

    if(N % stride != 0){
        exit(0);
    }
    int n = N / stride;

    for (i = 0; i < M; i++) {
        for (j = 0; j < N; j++) {
            C[i * ldc + j] = 0;
        }
    }

    int xx;
    for (i = 0; i < M; ++i) {
        xx = 0;
        for (k = 0; k < K; ++k) {
            register int A_PART = A[i * lda + k] - zero_point;
            for (j = 0; j < n; ++j, ++xx) {
                conv2d_acc_quant(C + i * ldc + j * stride, A_PART, B[xx], stride, pstride);
                //C[i * ldc + j] += A_PART * B[k * ldb + j];
            }
        }
    }
}


qtk_maybe_unused static void
nn_conv2d_quant_(int16_t *X, int16_t **idx,qtk_numeric_data_t Y, qtk_numeric_data_t W,
           int* ksize, int *stride,
           int out_channel, int groups, uint32_t *shape, uint8_t wzero_point) {
    int in_b = shape[0];
    int in_c = shape[1];
    int in_h = shape[2];
    int in_w = shape[3];
    int out_w = qtk_nn_pad_dim(in_w, 0, 0, ksize[1], stride[1]);
    int out_h = qtk_nn_pad_dim(in_h, 0, 0, ksize[0], stride[0]);

    int m = out_channel / groups;
    int k = ksize[0] * ksize[1] * in_c / groups;
    int n = out_w * out_h;
    int16_t *x_f32 = X;
    uint8_t *w_f32 = W.u8;
    int *y_f32 = Y.i32;

    int16_t **index = idx;
    int id_stride;
    for (int i = 0; i < in_b; i++) {
        for (int j = 0; j < groups;
             j++, y_f32 += n * m, x_f32 += in_c / groups * in_h * in_w) {
            uint8_t *a = w_f32 + m * k * j;
            int16_t *b;
            int *c = y_f32;
            int16_t *im = x_f32;
            if (ksize[0] == 1 && ksize[1] == 1) {
                b = im;
                conv_qtk_sgemm(0, 0, m, n, k, 1.0, a, k, b, n, 0.0, c, n, wzero_point);
            } else {
                //im2col_cpu_(im, in_c / groups, in_h, in_w, ksize, stride, npad,
                //            b);
                im2col2_cpu_quant_(im, in_c / groups, in_h, in_w, ksize, stride,
                            index, &id_stride);
                cov2d_sgemm_quant(m, n, k, a, k, index, n, c, n, id_stride, stride[1], wzero_point);
            }
        }
    }
}


qtk_maybe_unused static void
nn_conv2d_quant_naive_(qtk_numeric_data_t X, qtk_numeric_data_t Y,
                 qtk_numeric_data_t W, int *pad,
                 int *ksize, int *stride, int out_channel, int groups,
                 uint32_t *shape, uint8_t *dilations,
                 qtk_numeric_data_t workspace, uint8_t wzero_point, uint8_t xzero_point) {
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
    uint8_t *x_f32 = X.u8;
    uint8_t *w_f32 = W.u8;
    int *y_f32 = Y.f32;

    for (int i = 0; i < in_b; i++) {
        for (int j = 0; j < groups;
             j++, y_f32 += n * m, x_f32 += in_c / groups * in_h * in_w) {
            uint8_t *a = w_f32 + m * k * j;
            int16_t *b = workspace.i16;
            int *c = y_f32;
            uint8_t *im = x_f32;
            //if (ksize[0] == 1 && ksize[1] == 1) {
                //b = im;
            //} else {
                im2col2_cpu_quant_naive_(im, in_c / groups, in_h, in_w, ksize, stride, pad,
                            b, dilations,xzero_point);
            //}

            memset(c, 0, sizeof(int) * m * n);
            conv_qtk_sgemm(0, 0, m, n, k, 1.0, a, k, b, n, 0.0, c, n, wzero_point);
        }
    }
}

#ifdef __cplusplus
};
#endif
#endif
