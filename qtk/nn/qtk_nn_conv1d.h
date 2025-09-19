#ifndef QBL_NN_QBL_NN_CONV1D_H
#define QBL_NN_QBL_NN_CONV1D_H
#pragma once
#include "qtk/linalg/qtk_gemm.h"
#include "qtk/nn/qtk_nn_im2col.h"
#include "qtk/nn/qtk_nn_utils.h"
#ifdef __cplusplus
extern "C" {
#endif

qtk_maybe_unused static void
nn_conv1d_(qtk_numeric_data_t X, qtk_numeric_data_t Y, qtk_numeric_data_t W,
           qtk_numeric_data_t B, int pad, int ksize, int stride,
           int out_channel, int groups, uint32_t *shape,
           qtk_numeric_data_t workspace) {
    int in_b = shape[0];
    int in_c = shape[1];
    int in_steps = shape[2];

    int out_steps = qtk_nn_pad_dim(in_steps, pad, pad, ksize, stride);

    int m = out_channel / groups;
    int k = ksize * in_c / groups;
    int n = out_steps;

    float *x_f32 = X.f32;
    float *w_f32 = W.f32;
    float *y_f32 = Y.f32;
    float *bias_f32 = B.f32;

    for (int i = 0; i < in_b; i++) {
        for (int j = 0; j < groups;
             j++, y_f32 += n * m, x_f32 += in_c / groups * in_steps) {
            float *a = w_f32 + m * k * j;
            float *b = workspace.f32;
            float *c = y_f32;
            float *im = x_f32;
            if (ksize == 1) {
                b = im;
            } else {
                im2col_cpu_1d_(im, in_c / groups, in_steps, ksize, stride, pad,
                               b);
            }
            memset(c, 0, sizeof(float) * m * n);
            qtk_sgemm(0, 0, m, n, k, 1.0, a, k, b, n, 0.0, c, n);
        }
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

#ifdef __cplusplus
};
#endif
#endif
