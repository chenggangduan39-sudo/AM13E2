#ifndef G_YO18G1W5_EDH0_FGEQ_9RSG_RDE178Y4LA18
#define G_YO18G1W5_EDH0_FGEQ_9RSG_RDE178Y4LA18
#pragma once
#include "qtk/core/qtk_binary.h"
#include "qtk/nn/qtk_nn_utils.h"
#include "qtk/nn/vm/qtk_nn_vm.h"
#ifdef __cplusplus
extern "C" {
#endif

qtk_maybe_unused static int vm_Range_infer_shape_(qtk_nn_vm_t *nv,
                                                  uint8_t *instructions) {
    uint16_t start = qtk_littleEndian_uint16_from_bin(instructions),
             limit = qtk_littleEndian_uint16_from_bin(instructions + 2),
             delta = qtk_littleEndian_uint16_from_bin(instructions + 4),
             y = qtk_littleEndian_uint16_from_bin(instructions + 6);
    uint32_t out_shape[4];
    qtk_numeric_data_t START, LIMIT, DELTA;

    START.raw = qtk_nn_get_loc_from_repr(nv, start);
    LIMIT.raw = qtk_nn_get_loc_from_repr(nv, limit);
    DELTA.raw = qtk_nn_get_loc_from_repr(nv, delta);

    out_shape[0] = (*LIMIT.i64 - *START.i64) / *DELTA.i64;
    nn_vm_set_dynamic_shape_for_repr_(nv, out_shape, 1, y);
    return 0;
}

qtk_maybe_unused static void vm_Range_(qtk_nn_vm_t *nv,
                                       uint8_t **instructions) {
    uint16_t start = qtk_littleEndian_uint16_from_bin(*instructions),
             delta = qtk_littleEndian_uint16_from_bin(*instructions + 4),
             y = qtk_littleEndian_uint16_from_bin(*instructions + 6);
    qtk_numeric_data_t Y;
    qtk_numeric_data_t START, DELTA;
    uint32_t *out_shape;

    START.raw = qtk_nn_get_loc_from_repr(nv, start);
    DELTA.raw = qtk_nn_get_loc_from_repr(nv, delta);

    Y.raw = qtk_nn_get_dynamic_loc(nv, QBL_NN_TENSOR_GET_INDEX(y));
    out_shape = qtk_nn_get_dynamic_shape(nv, QBL_NN_TENSOR_GET_INDEX(y));

    if (QBL_NN_TENSOR_GET_ELEM_TYPE(y) == QBL_NN_VM_TENSOR_ELEM_I64) {
        int64_t start, delta, val;
        start = *START.i64;
        delta = *DELTA.i64;
        int i;
        for (i = 0, val = start; i < out_shape[0]; i++, val += delta) {
            Y.i64[i] = val;
        }
    } else {
        qtk_assert(0);
    }

    *instructions += 8;
}

#ifdef __cplusplus
};
#endif
#endif /* G_YO18G1W5_EDH0_FGEQ_9RSG_RDE178Y4LA18 */
