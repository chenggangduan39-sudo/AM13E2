#ifndef G_L83U0H3A_EEZH_WNGZ_AE7S_TNELS08I1N06
#define G_L83U0H3A_EEZH_WNGZ_AE7S_TNELS08I1N06
#pragma once
#include "qtk/core/qtk_binary.h"
#include "qtk/nn/qtk_nn_utils.h"
#include "qtk/nn/vm/qtk_nn_vm.h"
#ifdef __cplusplus
extern "C" {
#endif

qtk_maybe_unused static int
vm_ConstantOfShape_infer_shape_(qtk_nn_vm_t *nv, uint8_t *instructions) {
    uint32_t *in_shape, out_shape[32];
    qtk_numeric_data_t X;
    uint16_t x = qtk_littleEndian_uint16_from_bin(instructions),
             y = qtk_littleEndian_uint16_from_bin(instructions + 2);

    // if x is initialize nn-compiler should eliminate this op
    in_shape = qtk_nn_get_dynamic_shape(nv, QBL_NN_TENSOR_GET_INDEX(x));
    X.raw = qtk_nn_get_dynamic_loc(nv, QBL_NN_TENSOR_GET_INDEX(x));
    qtk_assert(QBL_NN_VM_TENSOR_ELEM_I64 == QBL_NN_TENSOR_GET_ELEM_TYPE(x));
    for (int i = 0; i < in_shape[0]; i++) {
        out_shape[i] = X.i64[i];
    }
    nn_vm_set_dynamic_shape_for_repr_(nv, out_shape, in_shape[0], y);
    return 0;
}

qtk_maybe_unused static void vm_ConstantOfShape_(qtk_nn_vm_t *nv,
                                                 uint8_t **instructions) {
    uint32_t nelem = 1;
    uint32_t y = qtk_littleEndian_uint16_from_bin(*instructions + 2);
    uint16_t extra_index = qtk_littleEndian_uint16_from_bin(*instructions + 4);
    int elem_type;
    qtk_numeric_data_t Y;

    nelem = nn_vm_tensor_get_nelem_from_repr_(nv, y);
    elem_type = cast(int8_t *, cast(char *, nv->extra) + extra_index)[0];
    Y.raw = qtk_nn_get_dynamic_loc(nv, QBL_NN_TENSOR_GET_INDEX(y));

    if (elem_type == QBL_NN_VM_TENSOR_ELEM_I64) {
        int64_t val;
        memcpy(&val, cast(int8_t *, nv->extra) + extra_index + 1, sizeof(val));
        for (int i = 0; i < nelem; i++) {
            Y.i64[i] = val;
        }
    } else {
        qtk_assert(0);
    }

    *instructions += 6;
}

#ifdef __cplusplus
};
#endif
#endif /* G_L83U0H3A_EEZH_WNGZ_AE7S_TNELS08I1N06 */
