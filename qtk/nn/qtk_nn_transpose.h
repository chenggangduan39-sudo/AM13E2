#ifndef G_5NZ5R5IN_1WIF_QFYL_L9BW_XTBESOB6S97U
#define G_5NZ5R5IN_1WIF_QFYL_L9BW_XTBESOB6S97U
#pragma once
#include "qtk/core/qtk_binary.h"
#include "qtk/nn/qtk_nn_utils.h"
#include "qtk/nn/vm/qtk_nn_vm.h"
#ifdef __cplusplus
extern "C" {
#endif

#define MAX_DIMS 16

void transpose_recursive(float *input, float *output, const int *perm,
                         size_t *indices, int depth, int *inshape,
                         int *instrides, int *ostrides, int rank) {
    if (depth == rank) {
        size_t in_offset = 0, out_offset = 0;
        for (int i = 0; i < rank; ++i) {
            in_offset += indices[i] * instrides[i];
            out_offset += indices[perm[i]] * ostrides[i];
        }
        output[out_offset] = input[in_offset];
        return;
    }

    for (size_t i = 0; i < inshape[depth]; ++i) {
        indices[depth] = i;
        transpose_recursive(input, output, perm, indices, depth + 1, inshape, instrides, ostrides, rank);
    }
}

void static nn_transpose_(float* input, float *output, int inrank, int* shape, const int* perm, int* strides, int* ostrides) {
    size_t indices[MAX_DIMS] = {0};
    transpose_recursive(input, output, perm, indices, 0, shape, strides, ostrides, inrank);
}

qtk_maybe_unused static void vm_Transpose_(qtk_nn_vm_t *nv,
                                           uint8_t **instructions) {
    int perm_tmp[32];
    int nperm;
    int in_shape_tmp[32];
    uint16_t x = qtk_littleEndian_uint16_from_bin(*instructions),
             y = qtk_littleEndian_uint16_from_bin(*instructions + 2);
    qtk_numeric_data_t X, Y;
    uint16_t extra_index;
    uint8_t *extra;
    uint32_t *in_shape = qtk_nn_get_shape_from_repr(nv, x);
    uint32_t *out_shape = qtk_nn_get_shape_from_repr(nv, y);

    int rank = qtk_nn_get_rank_from_repr(nv, x);
    int strides[MAX_DIMS];
    int ostrides[MAX_DIMS];

    X.raw = qtk_nn_get_loc_from_repr(nv, x);
    Y.raw = qtk_nn_get_loc_from_repr(nv, y);
    *instructions += 4;
    extra_index = qtk_littleEndian_uint16_from_bin(*instructions);
    *instructions += 2;

    extra = cast(uint8_t *, nv->extra) + extra_index;

    nperm = extra[0];
    for (int i = 0; i < nperm; i++) {
        perm_tmp[i] = extra[1 + i];
        in_shape_tmp[i] = in_shape[i];
    }

    strides[rank-1] = 1;
    for (int i = rank-2; i >= 0; --i) {
        strides[i] = strides[i+1] * in_shape[i+1];
    }

    ostrides[rank-1] = 1;
    for (int i = rank-2; i >= 0; --i) {
        ostrides[i] = ostrides[i+1] * out_shape[i+1];
    }

    qtk_assert(QBL_NN_TENSOR_GET_ELEM_TYPE(y) == QBL_NN_VM_TENSOR_ELEM_F32);

    nn_transpose_(X.f32, Y.f32, rank, in_shape_tmp, perm_tmp, strides, ostrides);
}

qtk_maybe_unused static int vm_Transpose_infer_shape_(qtk_nn_vm_t *nv,
                                                      uint8_t *instructions) {
    uint16_t x = qtk_littleEndian_uint16_from_bin(instructions),
             y = qtk_littleEndian_uint16_from_bin(instructions + 2);
    uint8_t *extra;
    uint32_t *shapeX, shapeY[32];
    int nperm;
    shapeX = qtk_nn_get_shape_from_repr(nv, x);
    extra = instructions + 4;
    nperm = extra[0];
    for (int i = 0; i < nperm; i++) {
        shapeY[i] = shapeX[extra[1 + i]];
    }
    nn_vm_set_dynamic_shape_for_repr_(nv, shapeY, nperm, y);
    return 0;
}

#ifdef __cplusplus
};
#endif
#endif /* G_5NZ5R5IN_1WIF_QFYL_L9BW_XTBESOB6S97U */
