#ifndef QBL_NN_QBL_NN_IM2COL_H
#define QBL_NN_QBL_NN_IM2COL_H
#pragma once
#include "qtk/core/qtk_type.h"
#include "qtk/math/qtk_cmp.h"
#ifdef __cplusplus
extern "C" {
#endif

static float im2col_get_pixel_(float *im, int height, int width, int channels,
                               int row, int col, int channel, int* pad) {
    row -= pad[0];
    col -= pad[1];

    if (qtk_is_a_ge_zero_and_a_lt_b(row, height) &&
        qtk_is_a_ge_zero_and_a_lt_b(col, width)) {
        return im[col + width * (row + height * channel)];
    }
    return 0;
}

static float* im2col_get_pixelp_(float *im, int height, int width, int channels,
                               int row, int col, int channel) {
    if (qtk_is_a_ge_zero_and_a_lt_b(row, height) &&
        qtk_is_a_ge_zero_and_a_lt_b(col, width)) {
        return im + col + width * (row + height * channel);
    }
    return NULL;
}

static float im2col_get_pixel_1d_(float *im, int nstep, int channels, int step,
                                  int channel, int pad) {
    step -= pad;

    if (qtk_is_a_ge_zero_and_a_lt_b(step, nstep)) {
        return im[step + channel * nstep];
    }
    return 0;
}

// From Berkeley Vision's Caffe!
// https://github.com/BVLC/caffe/blob/master/LICENSE
qtk_maybe_unused static void im2col_cpu_(float *data_im, int channels,
                                         int height, int width, int *ksize,
                                         int *stride, int *pad, float *data_col,
                                         uint8_t *dilations) {
    int c, h, w;
    const int height_col =
        (height + pad[0] + pad[2] - (dilations[0] * (ksize[0] - 1) + 1)) /
            stride[0] +
        1;
    const int width_col =
        (width + pad[1] + pad[3] - (dilations[1] * (ksize[1] - 1) + 1)) /
            stride[1] +
        1;

    int channels_col = channels * ksize[0] * ksize[1];

    for (c = 0; c < channels_col; ++c) {
        int w_offset = c % ksize[1];
        int h_offset = (c / ksize[1]) % ksize[0];
        h_offset *= dilations[0];
        w_offset *= dilations[1];
        int c_im = c / ksize[0] / ksize[1];
        for (h = 0; h < height_col; ++h) {
            for (w = 0; w < width_col; ++w) {
                int im_row = h_offset + h * stride[0];
                int im_col = w_offset + w * stride[1];
                //int col_index = (c * height_col + h) * width_col + w;
                int col_index = (h * width_col + w) * channels_col + c;
                data_col[col_index] =
                    im2col_get_pixel_(data_im, height, width, channels, im_row,
                                      im_col, c_im, pad);
            }
        }
    }
}

qtk_maybe_unused static void im2col2_cpu_(float *data_im, int channels,
                                         int height, int width, int* ksize,
                                         int* stride, float **data_col, int *id_stride) {
    int c, h;
    int height_col = (height - ksize[0]) / stride[0] + 1;
    int width_col = (width - ksize[1]) / stride[1] + 1;

    int xx = 0;
    int channels_col = channels * ksize[0] * ksize[1];
    *id_stride = width_col;
    for (c = 0; c < channels_col; ++c) {
        int w_offset = c % ksize[1];
        int h_offset = (c / ksize[1]) % ksize[0];
        int c_im = c / ksize[0] / ksize[1];
        for (h = 0; h < height_col; ++h, xx++) {
            int im_row = h_offset + h * stride[0];
            int im_col = w_offset;
            data_col[xx] = im2col_get_pixelp_(data_im, height, width, channels, im_row,
                                    im_col, c_im);
        }
    }
}

qtk_maybe_unused static void im2col_cpu_1d_(float *data_im, int channels,
                                            int steps, int ksize, int stride,
                                            int pad, float *data_col) {
    int c, step;
    int steps_col = (steps + 2 * pad - ksize) / stride + 1;
    int channels_col = channels * ksize;

    for (c = 0; c < channels_col; ++c) {
        int step_offset = c % ksize;
        int c_im = c / ksize;
        for (step = 0; step < steps_col; ++step) {
            int im_row = step_offset + step * stride;
            int col_index = c * steps_col + step;
            data_col[col_index] = im2col_get_pixel_1d_(data_im, steps, channels,
                                                       im_row, c_im, pad);
        }
    }
}

#ifdef __cplusplus
};
#endif
#endif
