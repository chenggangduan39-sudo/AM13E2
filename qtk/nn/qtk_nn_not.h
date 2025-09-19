#ifndef G_EQOLS460_N6MS_K9TC_3HID_QNATLO2251XD
#define G_EQOLS460_N6MS_K9TC_3HID_QNATLO2251XD
#pragma once
#include "qtk/core/qtk_binary.h"
#include "qtk/nn/qtk_nn_utils.h"
#include "qtk/nn/vm/qtk_nn_vm.h"
#ifdef __cplusplus
extern "C" {
#endif

qtk_maybe_unused static int vm_Not_infer_shape_(qtk_nn_vm_t *nv,
                                                uint8_t *instructions) {
    uint16_t x = qtk_littleEndian_uint16_from_bin(instructions),
             y = qtk_littleEndian_uint16_from_bin(instructions + 2);
    nn_vm_clone_shape_for_repr_(nv, x, y);
    return 0;
}

qtk_maybe_unused static void vm_Not_(qtk_nn_vm_t *nv, uint8_t **instructions) {
    uint16_t x = qtk_littleEndian_uint16_from_bin(*instructions),
             y = qtk_littleEndian_uint16_from_bin(*instructions + 2);
    uint32_t nelem;
    qtk_numeric_data_t X, Y;

    nelem = nn_vm_tensor_get_nelem_from_repr_(nv, x);

    X.raw = qtk_nn_get_loc_from_repr(nv, x);
    Y.raw = qtk_nn_get_loc_from_repr(nv, y);

    for (uint32_t i = 0; i < nelem; i++) {
        Y.boolean[i] = !X.boolean[i];
    }

    *instructions += 4;
}

#ifdef __cplusplus
};
#endif
#endif /* G_EQOLS460_N6MS_K9TC_3HID_QNATLO2251XD */
