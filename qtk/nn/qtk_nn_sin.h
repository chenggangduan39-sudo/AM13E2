#ifndef EF9B43AA_7315_4D7D_95C0_BBA6BACCA93C
#define EF9B43AA_7315_4D7D_95C0_BBA6BACCA93C

#include "qtk/core/qtk_binary.h"
#include "qtk/nn/qtk_nn_utils.h"
#include "qtk/nn/vm/qtk_nn_vm.h"

static void nn_sin_f32_(float *x, float *y, uint32_t nelem) {
    for (uint32_t i = 0; i < nelem; i++) {
        y[i] = sinf(x[i]);
    }
}

qtk_maybe_unused static void vm_Sin_(qtk_nn_vm_t *nv, uint8_t **instructions) {
    uint16_t x = qtk_littleEndian_uint16_from_bin(*instructions),
             y = qtk_littleEndian_uint16_from_bin(*instructions + 2);
    qtk_numeric_data_t X, Y;
    uint32_t nelem;

    X.raw = qtk_nn_get_loc_from_repr(nv, x);
    Y.raw = qtk_nn_get_loc_from_repr(nv, y);

    nelem = nn_vm_tensor_get_nelem_from_repr_(nv, x);

    switch (QBL_NN_TENSOR_GET_ELEM_TYPE(x)) {
    case QBL_NN_VM_TENSOR_ELEM_F32:
        nn_sin_f32_(X.f32, Y.f32, nelem);
        break;
    default:
        qtk_assert(0);
    }

    *instructions += 4;
}

#endif /* EF9B43AA_7315_4D7D_95C0_BBA6BACCA93C */
