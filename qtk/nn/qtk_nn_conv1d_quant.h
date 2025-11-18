#ifndef QBL_NN_QBL_CONV1DQ_H
#define QBL_NN_QBL_CONV1DQ_H
#pragma once
#include "qtk/nn/qtk_nn_im2col_quant.h"
#include "qtk/nn/qtk_nn_utils.h"
#ifdef __cplusplus
extern "C" {
#endif

qtk_maybe_unused static void
nn_conv1d_quant(qtk_numeric_data_t X, qtk_numeric_data_t Y, qtk_numeric_data_t W,
           int pad, int ksize, int stride,
           int out_channel, int groups, uint32_t *shape,
           qtk_numeric_data_t workspace,uint8_t iz, uint8_t wz) {
    int in_b = shape[0];
    int in_c = shape[1];
    int in_steps = shape[2];

    int out_steps = qtk_nn_pad_dim(in_steps, pad, pad, ksize, stride);

    int m = out_channel / groups;
    int k = ksize * in_c / groups;
    int n = out_steps;

    uint8_t *x_f32 = X.u8;
    uint8_t *w_f32 = W.u8;
    int *y_f32 = Y.i32;
    for (int i = 0; i < in_b; i++) {
        for (int j = 0; j < groups;
             j++, y_f32 += n * m, x_f32 += in_c / groups * in_steps) {
            uint8_t *a = w_f32 + m * k * j;
            int16_t *b = workspace.i16;
            int *c = y_f32;
            uint8_t *im = x_f32;
            // if (ksize == 1) {
            //     b = im;
            // } else {
                im2col_cpu_1d_quant(im, in_c / groups, in_steps, ksize, stride, pad,
                               b, iz);
            //}
            //memset(c, 0, sizeof(int) * m * n);
            conv_qtk_sgemm(0, 0, m, n, k, 1.0, a, k, b, n, 0.0, c, n, wz);
        }
    }
}

#ifdef __cplusplus
};
#endif
#endif
