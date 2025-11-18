#ifndef QBL_NN_QBL_NN_SPLIT_H
#define QBL_NN_QBL_NN_SPLIT_H
#pragma once
#include "qtk/core/qtk_binary.h"
#include "qtk/nn/qtk_nn_utils.h"
#include "qtk/nn/vm/qtk_nn_vm.h"
#ifdef __cplusplus
extern "C" {
#endif

qtk_maybe_unused static int vm_Split_infer_shape_(qtk_nn_vm_t *nv,
                                                  uint8_t *instructions) {
    uint16_t out_reprs[16];
    uint16_t inp_reprs[2];
    uint32_t *shapeX[2];
    uint32_t shapeY[16][16];
    int8_t axis;
    int rankX;
    uint8_t ninp = instructions[0];

    for (int i = 0; i < ninp; i++) {
        inp_reprs[i] =
            qtk_littleEndian_uint16_from_bin(instructions + 1 + (i << 1));
        shapeX[i] = qtk_nn_get_shape_from_repr(nv, inp_reprs[i]);
    }
    uint8_t noutp = (instructions)[1 + (ninp << 1)];
    qtk_assert(noutp <= 16);
    for (int i = 0; i < noutp; i++) {
        out_reprs[i] = qtk_littleEndian_uint16_from_bin(
            instructions + 1 + (ninp << 1) + 1 + (i << 1));
    }
    axis = cast(int8_t *, instructions)[1 + (ninp << 1) + 1 + (noutp << 1)];
    rankX = qtk_nn_get_rank_from_repr(nv, inp_reprs[0]);
    if (axis < 0) {
        axis = (axis + rankX) % rankX;
    }

    if (ninp == 1) { // No Split Input
        uint32_t splited_dim = shapeX[0][axis] / noutp;
        for (int i = 0; i < noutp; i++) {
            memcpy(shapeY[i], shapeX[0], rankX * sizeof(shapeX[0]));
            shapeY[i][axis] = splited_dim;
        }
    } else {
        qtk_numeric_data_t SPLIT;
        SPLIT.raw = qtk_nn_get_loc_from_repr(nv, inp_reprs[1]);
        for (int i = 0; i < noutp; i++) {
            memcpy(shapeY[i], shapeX[0], rankX * sizeof(shapeX[0]));
            qtk_assert(SPLIT.i64[i] > 0);
            shapeY[i][axis] = SPLIT.i64[i];
        }
    }

    for (int i = 0; i < noutp; i++) {
        nn_vm_set_dynamic_shape_for_repr_(nv, shapeY[i], rankX, out_reprs[i]);
    }

    return 0;
}

qtk_maybe_unused static void vm_Split_(qtk_nn_vm_t *nv,
                                       uint8_t **instructions) {
    uint16_t out_reprs[16];
    uint16_t inp_reprs[2];
    uint32_t *shapeX[2];
//    uint32_t *shapeY[16];
    uint16_t extra_index;
    uint32_t split_elems[16];
    int8_t axis;
    uint8_t ninp = *instructions[0];
    qtk_numeric_data_t X, Y;
    int elem_sz;
    uint32_t pre_dim = 1, post_dim = 1;
    uint32_t axis_offset;
    int rankX;

    for (int i = 0; i < ninp; i++) {
        inp_reprs[i] =
            qtk_littleEndian_uint16_from_bin(*instructions + 1 + (i << 1));
        shapeX[i] = qtk_nn_get_shape_from_repr(nv, inp_reprs[i]);
    }
    uint8_t noutp = (*instructions)[1 + (ninp << 1)];
    qtk_assert(noutp <= 16);
    for (int i = 0; i < noutp; i++) {
        out_reprs[i] = qtk_littleEndian_uint16_from_bin(
            *instructions + 1 + (ninp << 1) + 1 + (i << 1));
//        shapeY[i] = qtk_nn_get_shape_from_repr(nv, out_reprs[i]);
    }
    extra_index = qtk_littleEndian_uint16_from_bin(
        *instructions + 1 + (ninp << 1) + 1 + (noutp << 1));
    axis = cast(int8_t *, nv->extra)[extra_index];

    rankX = qtk_nn_get_rank_from_repr(nv, inp_reprs[0]);
    if (axis < 0) {
        axis += rankX;
    }

    X.raw = qtk_nn_get_loc_from_repr(nv, inp_reprs[0]);
    elem_sz = nn_vm_tensor_get_elem_sz_(nv, inp_reprs[0]);

    for (int i = 0; i < axis; i++) {
        pre_dim *= shapeX[0][i];
    }

    for (int i = axis + 1; i < rankX; i++) {
        post_dim *= shapeX[0][i];
    }

    if (ninp == 1) { // No Split Input
        uint32_t splited_dim = shapeX[0][axis] / noutp;
        for (int i = 0; i < noutp; i++) {
            split_elems[i] = splited_dim;
        }
    } else {
        qtk_numeric_data_t SPLIT;
        SPLIT.raw = qtk_nn_get_loc_from_repr(nv, inp_reprs[1]);
        for (int i = 0; i < noutp; i++) {
            split_elems[i] = SPLIT.i64[i];
        }
    }

    axis_offset = 0;
    for (int i = 0; i < noutp; i++) {
        Y.raw = qtk_nn_get_loc_from_repr(nv, out_reprs[i]);
        for (int j = 0; j < pre_dim; j++) {
            memcpy(Y.chr + j * split_elems[i] * post_dim * elem_sz,
                   X.chr + (j * post_dim * shapeX[0][axis] +
                            axis_offset * post_dim) *
                               elem_sz,
                   split_elems[i] * post_dim * elem_sz);
        }
        axis_offset += split_elems[i];
    }

    *instructions += 1 + (ninp << 1) + 1 + (noutp << 1) + 2;
}

#ifdef __cplusplus
};
#endif
#endif
