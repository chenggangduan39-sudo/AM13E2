#ifndef G_AGJPMFH2_PG3O_SBR4_M34Z_2CYJIF8KYVDX
#define G_AGJPMFH2_PG3O_SBR4_M34Z_2CYJIF8KYVDX
#pragma once
#include "qtk/core/qtk_binary.h"
#include "qtk/nn/qtk_nn_utils.h"
#include "qtk/nn/vm/qtk_nn_vm.h"
#ifdef __cplusplus
extern "C" {
#endif

qtk_maybe_unused static int vm_Shape_infer_shape_(qtk_nn_vm_t *nv,
                                                  uint8_t *instructions) {
    uint32_t out_shape[16];
    uint16_t x = qtk_littleEndian_uint16_from_bin(instructions),
             y = qtk_littleEndian_uint16_from_bin(instructions + 2);
    int8_t *extra = cast(int8_t *, instructions) + 4;
    int start = extra[0];
    int end = extra[1];
    int input_rank;

    input_rank = qtk_nn_get_rank_from_repr(nv, x);

    if (start < 0) {
        start += input_rank;
    }

    if (end < 0) {
        end += input_rank;
    } else if (end > 4) {
        end = input_rank;
    }
    out_shape[0] = end - start;
    nn_vm_set_dynamic_shape_for_repr_(nv, out_shape, 1, y);
    return 0;
}

qtk_maybe_unused static void vm_Shape_(qtk_nn_vm_t *nv,
                                       uint8_t **instructions) {
    uint32_t *in_shape;
    uint16_t x = qtk_littleEndian_uint16_from_bin(*instructions),
             y = qtk_littleEndian_uint16_from_bin(*instructions + 2);
    uint16_t extra_index = qtk_littleEndian_uint16_from_bin(*instructions + 4);
    int8_t *extra = cast(int8_t *, nv->extra) + extra_index;
    int start = extra[0];
    int end = extra[1];
    int input_rank;
    qtk_numeric_data_t Y;

    in_shape = qtk_nn_get_shape_from_repr(nv, (x));
    Y.raw = qtk_nn_get_loc_from_repr(nv, (y));
    input_rank = qtk_nn_get_rank_from_repr(nv, x);

    if (start < 0) {
        start += input_rank;
    }

    if (end < 0) {
        end += input_rank;
    } else if (end > 4) {
        end = input_rank;
    }

    switch (QBL_NN_TENSOR_GET_ELEM_TYPE(y)) {
    case QBL_NN_VM_TENSOR_ELEM_I32:
        for (int i = start; i < end; i++) {
            Y.i32[i - start] = in_shape[i];
        }
        break;
    case QBL_NN_VM_TENSOR_ELEM_I64:
        for (int i = start; i < end; i++) {
            Y.i64[i - start] = in_shape[i];
        }
        break;
    default:
        qtk_assert(0);
    }

    *instructions += 6;
}

#ifdef __cplusplus
};
#endif
#endif /* G_AGJPMFH2_PG3O_SBR4_M34Z_2CYJIF8KYVDX */
