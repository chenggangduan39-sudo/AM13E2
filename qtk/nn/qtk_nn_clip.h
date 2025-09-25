#ifndef A0BB844B_59B6_E546_9EAA_046142BD4718
#define A0BB844B_59B6_E546_9EAA_046142BD4718
#include "qtk/core/qtk_binary.h"
#include "qtk/nn/qtk_nn_utils.h"
#include "qtk/nn/vm/qtk_nn_vm.h"

qtk_maybe_unused static void vm_Clip_(qtk_nn_vm_t *nv, uint8_t **instructions) {
    uint16_t x = qtk_littleEndian_uint16_from_bin(*instructions),
             low = qtk_littleEndian_uint16_from_bin(*instructions + 2),
             high = qtk_littleEndian_uint16_from_bin(*instructions + 4),
             y = qtk_littleEndian_uint16_from_bin(*instructions + 6);
    qtk_numeric_data_t X, Y, Low, High;
    X.raw = qtk_nn_get_loc_from_repr(nv, x);
    Y.raw = qtk_nn_get_loc_from_repr(nv, y);
    Low.raw = qtk_nn_get_loc_from_repr(nv, low);
    High.raw = qtk_nn_get_loc_from_repr(nv, high);
    int nelemLow = nn_vm_tensor_get_nelem_from_repr_(nv, low);
    int nelemHigh = nn_vm_tensor_get_nelem_from_repr_(nv, high);
    int N = nn_vm_tensor_get_nelem_from_repr_(nv, x);
    qtk_assert(QBL_NN_TENSOR_GET_ELEM_TYPE(x) == QBL_NN_VM_TENSOR_ELEM_F32);
    qtk_assert(QBL_NN_TENSOR_GET_ELEM_TYPE(y) == QBL_NN_VM_TENSOR_ELEM_F32);
    qtk_assert(QBL_NN_TENSOR_GET_ELEM_TYPE(low) == QBL_NN_VM_TENSOR_ELEM_F32);
    qtk_assert(QBL_NN_TENSOR_GET_ELEM_TYPE(high) == QBL_NN_VM_TENSOR_ELEM_F32);
    qtk_assert(nelemLow == 1);
    qtk_assert(nelemHigh == 1);
    for (int i = 0; i < N; i++) {
        Y.f32[i] = X.f32[i] < Low.f32[0]    ? Low.f32[0]
                   : X.f32[i] > High.f32[0] ? High.f32[0]
                                            : X.f32[i];
    }
    *instructions += 8;
}

#endif /* A0BB844B_59B6_E546_9EAA_046142BD4718 */
