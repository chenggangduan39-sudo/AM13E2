#ifndef F9A6B2BA_B874_4870_8D63_F2ED23E4649B
#define F9A6B2BA_B874_4870_8D63_F2ED23E4649B
#pragma once
#include "qtk/core/qtk_binary.h"
#include "qtk/nn/qtk_nn_shape_infer.h"
#include "qtk/nn/qtk_nn_utils.h"
#include "qtk/nn/vm/qtk_nn_vm.h"
#ifdef __cplusplus
extern "C" {
#endif

qtk_maybe_unused static int vm_Less_infer_shape_(qtk_nn_vm_t *nv,
                                                 uint8_t *instructions) {
    uint16_t a = qtk_littleEndian_uint16_from_bin(instructions),
             b = qtk_littleEndian_uint16_from_bin(instructions + 2),
             y = qtk_littleEndian_uint16_from_bin(instructions + 4);
    return multidirectional_broadcasting_shape_infer_(nv, a, b, y);
}

qtk_maybe_unused static void vm_Less_(qtk_nn_vm_t *nv, uint8_t **instructions) {
    uint16_t a = qtk_littleEndian_uint16_from_bin(*instructions),
             b = qtk_littleEndian_uint16_from_bin(*instructions + 2),
             y = qtk_littleEndian_uint16_from_bin(*instructions + 4);
    uint32_t nelemA, nelemB;
    qtk_numeric_data_t A, B, Y;
    qtk_nn_vm_tensor_elem_type_t elem_type = QBL_NN_TENSOR_GET_ELEM_TYPE(a);

    A.raw = qtk_nn_get_loc_from_repr(nv, a);
    B.raw = qtk_nn_get_loc_from_repr(nv, b);
    Y.raw = qtk_nn_get_loc_from_repr(nv, y);

    nelemA = nn_vm_tensor_get_nelem_from_repr_(nv, a);
    nelemB = nn_vm_tensor_get_nelem_from_repr_(nv, b);

    if (nelemA == nelemB) {
        if (elem_type == QBL_NN_VM_TENSOR_ELEM_F32) {
            for (uint32_t i = 0; i < nelemA; i++) {
                Y.boolean[i] = A.f32[i] < B.f32[i] ? 1 : 0;
            }
        } else {
            qtk_debug("%d\n", elem_type);
            qtk_assert(0);
        }
    } else {
        if (nelemB == 1) {
            if (elem_type == QBL_NN_VM_TENSOR_ELEM_F32) {
                for (uint32_t i = 0; i < nelemA; i++) {
                    Y.boolean[i] = A.f32[i] < B.f32[0] ? 1 : 0;
                }
            } else {
                qtk_debug("%d\n", elem_type);
                qtk_assert(0);
            }
        } else {
            qtk_assert(0);
        }
    }

    *instructions += 6;
}

#ifdef __cplusplus
};
#endif
#endif /* F9A6B2BA_B874_4870_8D63_F2ED23E4649B */
