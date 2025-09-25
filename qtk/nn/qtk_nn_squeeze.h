#ifndef QBL_NN_QBL_NN_SQUEEZE_H
#define QBL_NN_QBL_NN_SQUEEZE_H
#pragma once
#include "qtk/core/qtk_binary.h"
#include "qtk/nn/qtk_nn_utils.h"
#include "qtk/nn/vm/qtk_nn_vm.h"
#ifdef __cplusplus
extern "C" {
#endif

qtk_maybe_unused static int vm_Squeeze_infer_shape_(qtk_nn_vm_t *nv,
                                                    uint8_t *instructions) {
    uint16_t inp_reprs[2];
    uint16_t y;
    uint32_t *shapeX[2], shapeY[32];
    uint8_t ninp = instructions[0];
    int rank = 0;
    int rankX, rank_axes;

    for (int i = 0; i < ninp; i++) {
        inp_reprs[i] =
            qtk_littleEndian_uint16_from_bin(instructions + 1 + (i << 1));
        shapeX[i] = qtk_nn_get_shape_from_repr(nv, inp_reprs[i]);
    }
    rankX = qtk_nn_get_rank_from_repr(nv, inp_reprs[0]);
    rank_axes = qtk_nn_get_rank_from_repr(nv, inp_reprs[1]);
    y = qtk_littleEndian_uint16_from_bin(instructions + 1 + (ninp << 1));

    if (ninp == 1) { // No axes
        for (int i = 0; i < rankX; i++) {
            if (shapeX[0][i] != 1) {
                shapeY[rank++] = shapeX[0][i];
            }
        }
        nn_vm_set_dynamic_shape_for_repr_(nv, shapeY, rank, y);
    } else {
        qtk_numeric_data_t AXES;
        int real_axes[4];
        AXES.raw = qtk_nn_get_loc_from_repr(nv, inp_reprs[1]);
        qtk_assert(rank_axes == 1);
        for (int i = 0; i < shapeX[1][0]; i++) {
            int axes = AXES.i64[i] < 0 ? AXES.i64[i] + rankX : AXES.i64[i];
            if (shapeX[0][axes] != 1) {
                goto err;
            }
            real_axes[i] = axes;
        }
        rank = 0;
        for (int i = 0; i < rankX; i++) {
            int found = 0;
            for (int j = 0; j < shapeX[1][0]; j++) {
                if (real_axes[j] == i) {
                    found = 1;
                }
            }
            if (found == 0) {
                shapeY[rank++] = shapeX[0][i];
            }
        }
        nn_vm_set_dynamic_shape_for_repr_(nv, shapeY, rank, y);
    }
    return 0;
err:
    return -1;
}

qtk_maybe_unused static void vm_Squeeze_(qtk_nn_vm_t *nv,
                                         uint8_t **instructions) {
    uint32_t x, y;
    uint8_t ninp = *instructions[0];
    uint32_t tensor_sz;

    x = qtk_littleEndian_uint16_from_bin(*instructions + 1);
    y = qtk_littleEndian_uint16_from_bin(*instructions + 1 + (ninp << 1));
    qtk_numeric_data_t X, Y;

    tensor_sz = nn_vm_tensor_get_tensor_sz_(nv, x);
    X.raw = qtk_nn_get_loc_from_repr(nv, x);
    Y.raw = qtk_nn_get_loc_from_repr(nv, y);
    memcpy(Y.raw, X.raw, tensor_sz);

    *instructions += 1 + (ninp << 1) + 2;
}

#ifdef __cplusplus
};
#endif
#endif
