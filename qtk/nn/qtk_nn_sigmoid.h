#ifndef QBL_NN_QBL_NN_SIGMOID_H
#define QBL_NN_QBL_NN_SIGMOID_H
#pragma once
#include "qtk/core/qtk_binary.h"
#include "qtk/math/qtk_math.h"
#include "qtk/nn/qtk_nn_utils.h"
#include "qtk/nn/vm/qtk_nn_vm.h"
#ifdef __cplusplus
extern "C" {
#endif

qtk_maybe_unused static int vm_Sigmoid_infer_shape_(qtk_nn_vm_t *nv,
                                                    uint8_t *instructions) {
    uint16_t x = qtk_littleEndian_uint16_from_bin(instructions),
             y = qtk_littleEndian_uint16_from_bin(instructions + 2);
    nn_vm_clone_shape_for_repr_(nv, x, y);
    return 0;
}

qtk_maybe_unused static void vm_Sigmoid_(qtk_nn_vm_t *nv,
                                         uint8_t **instructions) {
    uint16_t x = qtk_littleEndian_uint16_from_bin(*instructions),
             y = qtk_littleEndian_uint16_from_bin(*instructions + 2);
    qtk_numeric_data_t X, Y;
    X.raw = qtk_nn_get_loc_from_repr(nv, x);
    Y.raw = qtk_nn_get_loc_from_repr(nv, y);
    uint32_t nelem;

    nelem = nn_vm_tensor_get_nelem_from_repr_(nv, x);
    qtk_assert(QBL_NN_TENSOR_GET_ELEM_TYPE(x) == QBL_NN_VM_TENSOR_ELEM_F32);

    // y = 1 / (1 + exp(-x))
    for (uint32_t i = 0; i < nelem; i++) {
        Y.f32[i] = 1.0 / (1.0 + qtk_expf(-X.f32[i]));
    }

    *instructions += 4;
}

#ifdef __cplusplus
};
#endif
#endif
