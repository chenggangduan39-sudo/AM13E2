#ifndef QBL_NN_QBL_NN_SOFTMAX_H
#define QBL_NN_QBL_NN_SOFTMAX_H
#pragma once
#include "qtk/core/qtk_binary.h"
#include "qtk/math/qtk_math.h"
#include "qtk/nn/qtk_nn_utils.h"
#include "qtk/nn/vm/qtk_nn_vm.h"
#include "qtk/numeric/qtk_numeric_type.h"
#ifdef __cplusplus
extern "C" {
#endif

// static void nn_softmax_(qtk_numeric_data_t X, qtk_numeric_data_t Y,
//                         uint32_t row, uint32_t col) {
//     float *x_f32 = X.f32;
//     float *y_f32 = Y.f32;
//     float *x_cursor = x_f32;
//     float *y_cursor = y_f32;

//     for (uint32_t i = 0; i < row; i++, x_cursor += col) {
//         float sum = 0;
//         for (uint32_t j = 0; j < col; j++) {
//             sum += qtk_expf(x_cursor[j]);
//         }
//         for (uint32_t j = 0; j < col; j++, y_cursor++) {
//             *y_cursor = qtk_expf(x_cursor[j]) / sum;
//         }
//     }
// }

static void nn_softmax_(float* X, float* Y,
                        uint32_t row, uint32_t col) {
    //float *x_f32 = X;
    //float *y_f32 = Y;
    float *x_cursor = X;
    float *y_cursor = Y;
    float *y_cursor2 = Y;
    float max_val = -FLT_MAX;
    for(int j = 0; j < row; j++,x_cursor += col) {
        for (int i = 0; i < col; i++) {
            if (x_cursor[i] > max_val) {
                max_val = x_cursor[i];
            }
        }

        float sum_exp = 0.0f;
        for (int i = 0; i < col; i++,y_cursor++) {
            float shifted = x_cursor[i] - max_val;
            *y_cursor = expf(shifted); // 计算 e^(x_i - max_val)
            sum_exp += *y_cursor;
        }

        for (int i = 0; i < col; i++,y_cursor2++) {
            *y_cursor2 /= sum_exp;
        }
    }
}

qtk_maybe_unused static void vm_Softmax_(qtk_nn_vm_t *nv,
                                         uint8_t **instructions) {
    uint16_t x = qtk_littleEndian_uint16_from_bin(*instructions),
             y = qtk_littleEndian_uint16_from_bin(*instructions + 2);
    qtk_numeric_data_t X, Y;
    uint16_t extra_index;
    int8_t axis;
    uint32_t merge_shape[2] = {1, 1};
    uint32_t *in_shape;
    int input_rank;

    X.raw = qtk_nn_get_loc_from_repr(nv, x);
    Y.raw = qtk_nn_get_loc_from_repr(nv, y);

    *instructions += 4;
    extra_index = qtk_littleEndian_uint16_from_bin(*instructions);
    *instructions += 2;
    axis = cast(int8_t *, nv->extra)[extra_index];

    in_shape = qtk_nn_get_shape_from_repr(nv, x);
    input_rank = qtk_nn_get_rank_from_repr(nv, x);

    if (axis < 0) {
        axis += input_rank;
    }

    for (int i = 0; i < axis; i++) {
        merge_shape[0] *= in_shape[i];
    }
    for (int i = axis; i < input_rank; i++) {
        merge_shape[1] *= in_shape[i];
    }
    nn_softmax_(X.f32, Y.f32, merge_shape[0], merge_shape[1]);
}

qtk_maybe_unused static int vm_Softmax_infer_shape_(qtk_nn_vm_t *nv,
                                                    uint8_t *instructions) {
    uint16_t x = qtk_littleEndian_uint16_from_bin(instructions),
             y = qtk_littleEndian_uint16_from_bin(instructions + 2);
    nn_vm_clone_shape_for_repr_(nv, x, y);
    return 0;
}

#ifdef __cplusplus
};
#endif
#endif
