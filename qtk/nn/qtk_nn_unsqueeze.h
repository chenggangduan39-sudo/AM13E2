#ifndef G_Z1I4LV9A_89QR_DWQK_8PL2_FYZ31B0PRDC4
#define G_Z1I4LV9A_89QR_DWQK_8PL2_FYZ31B0PRDC4
#pragma once
#include "qtk/core/qtk_binary.h"
#include "qtk/nn/qtk_nn_utils.h"
#include "qtk/nn/vm/qtk_nn_vm.h"
#ifdef __cplusplus
extern "C" {
#endif

qtk_maybe_unused static int vm_Unsqueeze_infer_shape_(qtk_nn_vm_t *nv,
                                                      uint8_t *instructions) {
    uint32_t *in_shape, out_shape[32], *axes_shape;
    uint16_t x = qtk_littleEndian_uint16_from_bin(instructions),
             axes = qtk_littleEndian_uint16_from_bin(instructions + 2),
             y = qtk_littleEndian_uint16_from_bin(instructions + 4);
    qtk_numeric_data_t AXES;
    int input_rank = 0;
    int output_rank;
    int64_t axes_real[4] = {0};

    in_shape = qtk_nn_get_dynamic_shape(nv, QBL_NN_TENSOR_GET_INDEX(x));
    axes_shape = qtk_nn_get_shape_from_repr(nv, axes);

    int axes_idx = 0;
    int in_shape_idx = 0;

    AXES.raw = qtk_nn_get_loc_from_repr(nv, axes);

    input_rank = qtk_nn_get_rank_from_repr(nv, x);
    output_rank = input_rank + axes_shape[0];

    for (int i = 0; i < axes_shape[0]; i++) {
        axes_real[i] = (AXES.i64[i] + output_rank) % output_rank;
        out_shape[axes_real[i]] = 1;
    }

    for (int i = 0; i < output_rank; i++) {
        if (axes_real[axes_idx] == i) {
            axes_idx += 1;
            if (axes_idx == axes_shape[0]) {
                for (i++; i < output_rank; i++) {
                    out_shape[i] = in_shape[in_shape_idx++];
                }
                break;
            }
        } else {
            out_shape[i] = in_shape[in_shape_idx++];
        }
    }

    nn_vm_set_dynamic_shape_for_repr_(nv, out_shape, output_rank, y);

    return 0;
}

qtk_maybe_unused static void vm_Unsqueeze_(qtk_nn_vm_t *nv,
                                           uint8_t **instructions) {
    uint32_t tot_sz;
    qtk_numeric_data_t X, Y;
    uint16_t x = qtk_littleEndian_uint16_from_bin(*instructions),
             y = qtk_littleEndian_uint16_from_bin(*instructions + 4);

    Y.raw = qtk_nn_get_loc_from_repr(nv, y);
    X.raw = qtk_nn_get_loc_from_repr(nv, x);

    tot_sz = nn_vm_tensor_get_tensor_sz_(nv, y);
    memcpy(Y.raw, X.raw, tot_sz);
    *instructions += 6;
}

#ifdef __cplusplus
};
#endif
#endif /* G_Z1I4LV9A_89QR_DWQK_8PL2_FYZ31B0PRDC4 */
