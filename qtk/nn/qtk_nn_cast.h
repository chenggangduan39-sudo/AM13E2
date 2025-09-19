#ifndef G_Y7MUD66E_MA0X_12WG_V30S_PTRTRA0NQMD6
#define G_Y7MUD66E_MA0X_12WG_V30S_PTRTRA0NQMD6
#pragma once
#include "qtk/core/qtk_binary.h"
#include "qtk/nn/qtk_nn_utils.h"
#include "qtk/nn/vm/qtk_nn_vm.h"
#ifdef __cplusplus
extern "C" {
#endif

qtk_maybe_unused static int vm_Cast_infer_shape_(qtk_nn_vm_t *nv,
                                                 uint8_t *instructions) {
    uint16_t x = qtk_littleEndian_uint16_from_bin(instructions),
             y = qtk_littleEndian_uint16_from_bin(instructions + 2);
    nn_vm_clone_shape_for_repr_(nv, x, y);
    return 0;
}

qtk_maybe_unused static void vm_Cast_(qtk_nn_vm_t *nv, uint8_t **instructions) {
    uint16_t x = qtk_littleEndian_uint16_from_bin(*instructions),
             y = qtk_littleEndian_uint16_from_bin(*instructions + 2);
    qtk_nn_vm_tensor_elem_type_t from, to;
    qtk_numeric_data_t X, Y;
    uint32_t nelem;

    from = QBL_NN_TENSOR_GET_ELEM_TYPE(x);
    to = QBL_NN_TENSOR_GET_ELEM_TYPE(y);
    nelem = nn_vm_tensor_get_nelem_from_repr_(nv, x);

    X.raw = qtk_nn_get_loc_from_repr(nv, x);
    Y.raw = qtk_nn_get_loc_from_repr(nv, y);

    switch (from) {
    case QBL_NN_VM_TENSOR_ELEM_I32:
        if (to == QBL_NN_VM_TENSOR_ELEM_I64) {
            qtk_numeric_i32_to_i64(X, Y, nelem);
        } else if (to == QBL_NN_VM_TENSOR_ELEM_F32){
            qtk_numeric_i32_to_f32(X, Y, nelem);
        }else{
            qtk_assert(0);
        }
        break;
    case QBL_NN_VM_TENSOR_ELEM_BOOL:
        if (to == QBL_NN_VM_TENSOR_ELEM_I64) {
            qtk_numeric_boolean_to_i64(X, Y, nelem);
        } else if (to == QBL_NN_VM_TENSOR_ELEM_I32) {
            qtk_numeric_boolean_to_i32(X, Y, nelem);
        } else {
            qtk_assert(0);
        }
        break;
    case QBL_NN_VM_TENSOR_ELEM_I64:
        if (to == QBL_NN_VM_TENSOR_ELEM_I32) {
            qtk_numeric_i64_to_i32(X, Y, nelem);
        } else {
            qtk_assert(0);
        }
        break;
    case QBL_NN_VM_TENSOR_ELEM_F32:
        qtk_assert(0);
        break;
    default:
        qtk_assert(0);
    }

    *instructions += 6;
}

#ifdef __cplusplus
};
#endif
#endif /* G_Y7MUD66E_MA0X_12WG_V30S_PTRTRA0NQMD6 */
