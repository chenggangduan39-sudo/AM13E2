#ifndef QBL_NN_QBL_NN_Max_H
#define QBL_NN_QBL_NN_Max_H
#pragma once
#include "qtk/core/qtk_binary.h"
#include "qtk/core/qtk_type.h"
#include "qtk/nn/qtk_nn_utils.h"
#include "qtk/nn/vm/qtk_nn_vm.h"
#include "qtk/numeric/qtk_numeric_type.h"
#ifdef __cplusplus
extern "C" {
#endif

static void nn_max_f32_(float *x, float *y, uint32_t nelem, float *m) {
    float mx = *m;
    for (uint32_t i = 0; i < nelem; i++) {
        *y = *x > mx ? *x : mx;
        x++;
        y++;
    }
}

qtk_maybe_unused static void vm_Max_(qtk_nn_vm_t *nv, uint8_t **instructions) {
    uint16_t x = qtk_littleEndian_uint16_from_bin(*instructions),
             x2 = qtk_littleEndian_uint16_from_bin(*instructions + 2),
             y = qtk_littleEndian_uint16_from_bin(*instructions + 4);
    qtk_numeric_data_t X, X2, Y;
    uint32_t nelem;

    X.raw = qtk_nn_get_loc_from_repr(nv, x);
    X2.raw = qtk_nn_get_loc_from_repr(nv, x2);
    Y.raw = qtk_nn_get_loc_from_repr(nv, y);

    nelem = nn_vm_tensor_get_nelem_from_repr_(nv, x2);

    switch (QBL_NN_TENSOR_GET_ELEM_TYPE(x)) {
    case QBL_NN_VM_TENSOR_ELEM_F32:
    nn_max_f32_(X2.f32, Y.f32, nelem, X.f32);
        break;
    default:
        qtk_assert(0);
    }

    *instructions += 6;
}

qtk_maybe_unused static int vm_Max_infer_shape_(qtk_nn_vm_t *nv,
                                                 uint8_t *instructions) {
    uint16_t x = qtk_littleEndian_uint16_from_bin(instructions + 2),
             y = qtk_littleEndian_uint16_from_bin(instructions + 4);
    nn_vm_clone_shape_for_repr_(nv, x, y);
    return 0;
}

#ifdef __cplusplus
};

#endif
#endif
