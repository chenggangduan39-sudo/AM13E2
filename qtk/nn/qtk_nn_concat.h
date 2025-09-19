#ifndef G_5YBZAWCE_7V6K_DMPD_1MMC_H92MSICWKTBE
#define G_5YBZAWCE_7V6K_DMPD_1MMC_H92MSICWKTBE
#pragma once
#include "qtk/core/qtk_binary.h"
#include "qtk/nn/qtk_nn_utils.h"
#include "qtk/nn/vm/qtk_nn_vm.h"
#ifdef __cplusplus
extern "C" {
#endif

qtk_maybe_unused static int vm_Concat_infer_shape_(qtk_nn_vm_t *nv,
                                                   uint8_t *instructions) {
    uint32_t out_shape[32];
    int ninput;
    int axis;
    uint8_t *input_desc;
    ninput = instructions[0];
    input_desc = instructions + 1;
    instructions += 1 + (ninput + 1) * 2;
    uint16_t extra_index;
    extra_index = qtk_littleEndian_uint16_from_bin(instructions);
    axis = cast(uint8_t *, nv->extra)[extra_index];
    uint16_t input_reprs[16];
    uint16_t y;
    int rank;

    qtk_assert(ninput <= 16 && ninput > 0);

    for (int i = 0; i < ninput; i++) {
        uint16_t x = qtk_littleEndian_uint16_from_bin(
            input_desc + (i * sizeof(qtk_nn_vm_repr_t)));
        input_reprs[i] = x;
    }
    rank = qtk_nn_get_rank_from_repr(nv, input_reprs[0]);
    y = qtk_littleEndian_uint16_from_bin(input_desc +
                                         (ninput * sizeof(qtk_nn_vm_repr_t)));
    memcpy(out_shape, qtk_nn_get_shape_from_repr(nv, input_reprs[0]),
           qtk_nn_get_shape_bytes_from_repr(nv, input_reprs[0]));

    for (int i = 1; i < ninput; i++) {
        uint32_t *shapeX;
        shapeX = qtk_nn_get_shape_from_repr(nv, input_reprs[i]);
        out_shape[axis] += shapeX[axis];
    }

    nn_vm_set_dynamic_shape_for_repr_(nv, out_shape, rank, y);

    return 0;
}

qtk_maybe_unused static void vm_Concat_(qtk_nn_vm_t *nv,
                                        uint8_t **instructions) {
    int ninput;
    int axis;
    uint8_t *input_desc;
    ninput = *instructions[0];
    input_desc = *instructions + 1;
    *instructions += 1 + (ninput + 1) * 2;
    uint16_t extra_index;
    extra_index = qtk_littleEndian_uint16_from_bin(*instructions);
    *instructions += 2;
    axis = cast(int8_t *, nv->extra)[extra_index];
    uint16_t input_reprs[16];
    uint32_t row = 1;
    uint32_t stride[16];
    uint32_t *shapeX0;
    uint16_t y, y_idx;
    int rank;
    qtk_numeric_data_t X[16];
    qtk_numeric_data_t Y;

    qtk_assert(ninput <= 16 && ninput > 0);

    for (int i = 0; i < ninput; i++) {
        uint16_t x = qtk_littleEndian_uint16_from_bin(
            input_desc + (i * sizeof(qtk_nn_vm_repr_t)));
        input_reprs[i] = x;
    }

    y = qtk_littleEndian_uint16_from_bin(input_desc +
                                         (ninput * sizeof(qtk_nn_vm_repr_t)));
    y_idx = QBL_NN_TENSOR_GET_INDEX(y);

    rank = qtk_nn_get_rank_from_repr(nv, input_reprs[0]);
    shapeX0 = qtk_nn_get_shape_from_repr(nv, input_reprs[0]);

    Y.raw = qtk_nn_get_dynamic_loc(nv, y_idx);

    if (axis < 0) {
        axis += rank;
    }

    for (int i = 0; i < axis; i++) {
        row *= shapeX0[i];
    }
    for (int i = 0; i < ninput; i++) {
        int col = 1;
        uint32_t *shapeX;
        shapeX = qtk_nn_get_shape_from_repr(nv, input_reprs[i]);
        for (int j = axis; j < rank; j++) {
            col *= shapeX[j];
        }
        stride[i] = col;
        X[i].raw = qtk_nn_get_loc_from_repr(nv, input_reprs[i]);
    }

    int elem_sz = nn_vm_tensor_elem_sz_[QBL_NN_TENSOR_GET_ELEM_TYPE(y)];
    for (int i = 0; i < row; i++) {
        for (int j = 0; j < ninput; j++) {
            memcpy(Y.raw, X[j].raw, elem_sz * stride[j]);
            X[j].chr += stride[j] * elem_sz;
            Y.chr += stride[j] * elem_sz;
        }
    }
}

#ifdef __cplusplus
};
#endif
#endif /* G_5YBZAWCE_7V6K_DMPD_1MMC_H92MSICWKTBE */
