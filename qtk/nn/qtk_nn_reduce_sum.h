#ifndef D5A9B3F3_EB18_44AF_B9EC_CCF3F34C1ACB
#define D5A9B3F3_EB18_44AF_B9EC_CCF3F34C1ACB
#pragma once
#include "qtk/core/qtk_binary.h"
#include "qtk/nn/vm/qtk_nn_vm.h"
#ifdef __cplusplus
extern "C" {
#endif

qtk_maybe_unused static int vm_ReduceSum_infer_shape_(qtk_nn_vm_t *nv,
                                                      uint8_t *instructions) {
    qtk_assert(0);
    // TODO
    return 0;
}

qtk_maybe_unused static void vm_ReduceSum_(qtk_nn_vm_t *nv,
                                           uint8_t **instructions) {
    uint16_t x = qtk_littleEndian_uint16_from_bin(*instructions),
             r = qtk_littleEndian_uint16_from_bin(*instructions + 2),
             y = qtk_littleEndian_uint16_from_bin(*instructions + 4);
    qtk_numeric_data_t X, R, Y;

    X.raw = qtk_nn_get_loc_from_repr(nv, x);
    R.raw = qtk_nn_get_loc_from_repr(nv, r);
    Y.raw = qtk_nn_get_loc_from_repr(nv, y);
    int rankX = qtk_nn_get_rank_from_repr(nv, x);
    uint32_t *shapeX = qtk_nn_get_shape_from_repr(nv, x);

    qtk_assert(nn_vm_tensor_get_nelem_from_repr_(nv, r) == 1);
    qtk_assert(QBL_NN_TENSOR_GET_ELEM_TYPE(x) == QBL_NN_VM_TENSOR_ELEM_F32);

    uint32_t merge_shape[3] = {1, 1, 1};
    int axes = R.i32[0];
    if (axes < 0) {
        axes += rankX;
    }

    for (int i = 0; i < axes; i++) {
        merge_shape[0] *= shapeX[i];
    }

    merge_shape[1] = shapeX[axes];

    for (int i = axes + 1; i < rankX; i++) {
        merge_shape[2] *= shapeX[i];
    }

    uint32_t N = shapeX[axes];

    float *output = Y.f32;
    for (uint32_t i = 0; i < merge_shape[0]; i++) {
        for (uint32_t j = 0; j < merge_shape[2]; j++, output++) {
            float sum = 0;
            for (uint32_t k = 0; k < N; k++) {
                sum += X.f32[i * merge_shape[1] * merge_shape[2] +
                             k * merge_shape[2] + j];
            }
            *output = sum;
        }
    }

    *instructions += 6;
}

#ifdef __cplusplus
};
#endif
#endif
