#ifndef QBL_NN_QBL_NN_IM2COLQ_H
#define QBL_NN_QBL_NN_IM2COLQ_H
#pragma once
#include "qtk/core/qtk_type.h"
#include "qtk/math/qtk_cmp.h"
#ifdef __cplusplus
extern "C" {
#endif


static uint16_t* im2col_get_pixelp_quant_(int16_t *im, int height, int width, int channels,
                               int row, int col, int channel) {
    if (qtk_is_a_ge_zero_and_a_lt_b(row, height) &&
        qtk_is_a_ge_zero_and_a_lt_b(col, width)) {
        return im + col + width * (row + height * channel);
    }
    return NULL;
}

static int16_t im2col_get_pixel_1d_quant(uint8_t *im, int nstep, int channels, int step,
                                  int channel, int pad, uint8_t input_zero_point) {
    step -= pad;
    int16_t ret;
    if (qtk_is_a_ge_zero_and_a_lt_b(step, nstep)) {
        ret = im[step + channel * nstep] - input_zero_point;
        return ret;
    }
    return 0;
}

qtk_maybe_unused static void im2col2_cpu_quant_(int16_t *data_im, int channels,
                                         int height, int width, int* ksize,
                                         int* stride, int16_t **data_col, int *id_stride) {
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
            data_col[xx] = im2col_get_pixelp_quant_(data_im, height, width, channels, im_row,
                                    im_col, c_im);
        }
    }
}

static int16_t im2col_get_pixelp_quant_naive_(uint8_t *im, int height, int width, int channels,
                               int row, int col, int channel, int* pad, uint8_t xzero_point) {
    row -= pad[0];
    col -= pad[1];

    if (qtk_is_a_ge_zero_and_a_lt_b(row, height) &&
        qtk_is_a_ge_zero_and_a_lt_b(col, width)) {
        return im[col + width * (row + height * channel)] - xzero_point;
    }
    return 0;
}

qtk_maybe_unused static void im2col2_cpu_quant_naive_(uint8_t *data_im, int channels,
                                         int height, int width, int *ksize,
                                         int *stride, int *pad, int16_t *data_col,
                                         uint8_t *dilations,uint8_t xzero_point) {
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
                int col_index = (c * height_col + h) * width_col + w;
                data_col[col_index] =
                    im2col_get_pixelp_quant_naive_(data_im, height, width, channels, im_row,
                                      im_col, c_im, pad, xzero_point);
            }
        }
    }
}

qtk_maybe_unused static void im2col_cpu_1d_quant(uint8_t *data_im, int channels,
                                            int steps, int ksize, int stride,
                                            int pad, int16_t *data_col, uint8_t input_zero_point) {
    int c, step;
    int steps_col = (steps + 2 * pad - ksize) / stride + 1;
    int channels_col = channels * ksize;

    for (c = 0; c < channels_col; ++c) {
        int step_offset = c % ksize;
        int c_im = c / ksize;
        for (step = 0; step < steps_col; ++step) {
            int im_row = step_offset + step * stride;
            int col_index = c * steps_col + step;
            data_col[col_index] = im2col_get_pixel_1d_quant(data_im, steps, channels,
                                                       im_row, c_im, pad, input_zero_point);
        }
    }
}


static void conv_gemm_nn_(int M, int N, int K, float ALPHA, uint8_t *A, int lda,
            int16_t *B, int ldb, int *C, int ldc, uint8_t wzero_point) {
    int i, j, k;
    for (i = 0; i < M; ++i) {
        for (k = 0; k < K; ++k) {
            register int A_PART = A[i * lda + k] - wzero_point;
            for (j = 0; j < N; ++j) {
                C[i * ldc + j] += A_PART * B[k * ldb + j];
            }
        }
    }
}

void conv_qtk_sgemm(int TA, int TB, int M, int N, int K, float ALPHA, uint8_t *A,
               int lda, int16_t *B, int ldb, float BETA, int *C, int ldc, uint8_t wzero_point) {
    int i, j;
    // for (i = 0; i < M; i++) {
    //     for (j = 0; j < N; j++) {
    //         C[i * ldc + j] *= BETA;
    //     }
    // }
    if(BETA == 0){
        memset(C,0,sizeof(int)*M*N);
    }
    if (!TA && !TB) {
        conv_gemm_nn_(M, N, K, ALPHA, A, lda, B, ldb, C, ldc, wzero_point);
    } else {
        exit(0);
    }
}

static void conv_gemm_nn2_(int M, int N, int K, float ALPHA, uint8_t *A, int lda,
                     uint8_t *B, int ldb, int *C, int ldc, uint8_t az, uint8_t bz) {
    int i, j, k;
    for (i = 0; i < M; ++i) {
        for (k = 0; k < K; ++k) {
            register int A_PART = A[i * lda + k] - az;
            for (j = 0; j < N; ++j) {
                C[i * ldc + j] += A_PART * (B[k * ldb + j] - bz);
            }
        }
    }
}

void conv_qtk_sgemm2(int TA, int TB, int M, int N, int K, float ALPHA, uint8_t *A,
               int lda, uint8_t *B, int ldb, float BETA, int *C, int ldc, uint8_t az, uint8_t bz) {
    int i, j;
    // for (i = 0; i < M; i++) {
    //     for (j = 0; j < N; j++) {
    //         C[i * ldc + j] *= BETA;
    //     }
    // }
    if(BETA == 0){
        memset(C,0,sizeof(int)*M*N);
    }
    if (!TA && !TB) {
        conv_gemm_nn2_(M, N, K, ALPHA, A, lda, B, ldb, C, ldc, az, bz);
    } else {
        exit(0);
    }
}

#ifdef __cplusplus
};
#endif
#endif
