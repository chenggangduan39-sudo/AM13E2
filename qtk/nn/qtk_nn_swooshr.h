#ifndef QBL_NN_QBL_NN_SWOOSHR_H
#define QBL_NN_QBL_NN_SWOOSHR_H
#pragma once
#include "qtk/core/qtk_binary.h"
#include "qtk/core/qtk_type.h"
#include "qtk/nn/qtk_nn_utils.h"
#include "qtk/nn/vm/qtk_nn_vm.h"
#include "qtk/numeric/qtk_numeric_type.h"
#ifdef __cplusplus
extern "C" {
#endif

static void nn_swooshr_f32_(float *x, float *y, uint32_t nelem) {
    float max_val;
    float diff;
    float tmp;
    for (uint32_t i = 0; i < nelem; i++) {
        tmp = x[i] - 1;
        max_val = tmp > 0 ? tmp : 0;
        diff = -fabs(tmp);
        y[i] = max_val + logf(1 + expf(diff)) - 0.08 * x[i] - 0.313261687;
    }
}

qtk_maybe_unused static void vm_Swooshr_(qtk_nn_vm_t *nv,
                                         uint8_t **instructions) {
    uint16_t x = qtk_littleEndian_uint16_from_bin(*instructions),
             y = qtk_littleEndian_uint16_from_bin(*instructions + 2);
    qtk_numeric_data_t X, Y;
    uint32_t nelem;

    X.raw = qtk_nn_get_loc_from_repr(nv, x);
    Y.raw = qtk_nn_get_loc_from_repr(nv, y);

    nelem = nn_vm_tensor_get_nelem_from_repr_(nv, x);

    *instructions += 4;

    switch (QBL_NN_TENSOR_GET_ELEM_TYPE(x)) {
    case QBL_NN_VM_TENSOR_ELEM_F32:
        nn_swooshr_f32_(X.f32, Y.f32, nelem);
        break;
    default:
        qtk_assert(0);
    }
}

qtk_maybe_unused static int vm_Swooshr_infer_shape_(qtk_nn_vm_t *nv,
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
