#ifndef QBL_NN_QBL_NN_MAXPOOL_H
#define QBL_NN_QBL_NN_MAXPOOL_H
#pragma once
#include "qtk/core/qtk_binary.h"
#include "qtk/core/qtk_type.h"
#include "qtk/nn/qtk_nn_utils.h"
#include "qtk/numeric/qtk_numeric_type.h"
#ifdef __cplusplus
extern "C" {
#endif

qtk_maybe_unused static void nn_maxpool_pad0_(qtk_numeric_data_t X,
                                              qtk_numeric_data_t Y,
                                              uint32_t *in_shape, int ksize,
                                              int stride) {
    float *x = X.f32;
    float *y = Y.f32;

    uint32_t in_b = in_shape[0];
    uint32_t in_c = in_shape[1];
    uint32_t in_h = in_shape[2];
    uint32_t in_w = in_shape[3];

    uint32_t out_h = qtk_nn_pad_dim(in_h, 0, 0, ksize, stride);
    uint32_t out_w = qtk_nn_pad_dim(in_w, 0, 0, ksize, stride);

#define get_in_pixel(b, c, h, w)                                               \
    (x[(b)*in_c * in_h * in_w + (c)*in_h * in_w + (h)*in_w + (w)])

    for (uint32_t b_i = 0; b_i < in_b; b_i++) {
        for (uint32_t c_i = 0; c_i < in_c; c_i++) {
            for (uint32_t out_h_i = 0; out_h_i < out_h; out_h_i++) {
                for (uint32_t out_w_i = 0; out_w_i < out_w; out_w_i++, y++) {
                    float candidate = FLT_MIN;
                    for (int k1 = 0; k1 < ksize; k1++) {
                        for (int k2 = 0; k2 < ksize; k2++) {
                            uint32_t h_idx = out_h_i * stride + k1;
                            uint32_t w_idx = out_w_i * stride + k2;
                            candidate =
                                max(get_in_pixel(b_i, c_i, h_idx, w_idx),
                                    candidate);
                        }
                    }
                    *y = candidate;
                }
            }
        }
    }

#undef get_in_pixel
}

qtk_maybe_unused static void vm_MaxPool_(qtk_nn_vm_t *nv,
                                         uint8_t **instructions) {
    uint16_t x = qtk_littleEndian_uint16_from_bin(*instructions),
             y = qtk_littleEndian_uint16_from_bin(*instructions + 2);
    uint16_t x_idx = x & QBL_NN_TENSOR_INDEX_MASK,
             y_idx = y & QBL_NN_TENSOR_INDEX_MASK;
    uint16_t extra_index;
    uint8_t *extra;
    uint8_t *dilations, *kernel_shape, *pads, *strides;
    int ceil_mode;

    uint32_t *in_shape = qtk_nn_get_shape_from_repr(nv, x);
    qtk_numeric_data_t X, Y;

    X.raw = qtk_nn_get_dynamic_loc(nv, x_idx);
    Y.raw = qtk_nn_get_dynamic_loc(nv, y_idx);

    *instructions += 4;
    extra_index = qtk_littleEndian_uint16_from_bin(*instructions);
    *instructions += 2;

    extra = cast(uint8_t *, nv->extra) + extra_index;
    dilations = extra;
    kernel_shape = dilations + 2;
    pads = kernel_shape + 2;
    strides = pads + 4;
    ceil_mode = *(strides + 2);
    unuse(ceil_mode);

    qtk_assert(kernel_shape[0] == kernel_shape[1] && strides[0] == strides[1]);
    qtk_assert(pads[0] == pads[1] && pads[1] == pads[2] && pads[2] == pads[3] &&
               pads[0] == 0);

    nn_maxpool_pad0_(X, Y, in_shape, kernel_shape[0], strides[0]);
}

#ifdef __cplusplus
};
#endif
#endif
