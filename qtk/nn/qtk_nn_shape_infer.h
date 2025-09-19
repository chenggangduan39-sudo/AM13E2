#ifndef QBL_NN_QBL_NN_SHAPE_INFER_H
#define QBL_NN_QBL_NN_SHAPE_INFER_H
#pragma once
#include "qtk/core/qtk_binary.h"
#include "qtk/nn/qtk_nn_utils.h"
#include "qtk/nn/vm/qtk_nn_vm.h"
#ifdef __cplusplus
extern "C" {
#endif

qtk_maybe_unused static int multidirectional_broadcasting_shape_infer_impl_(
    qtk_nn_vm_t *nv, uint32_t *shapeA, uint32_t *shapeB, int rankA, int rankB,
    qtk_nn_vm_repr_t y) {
    int alignedRank;
    uint32_t shapeY[32];
    alignedRank = rankA;
    uint32_t *alignedA = shapeA, *alignedB = shapeB;
    uint32_t tmp[32];

    if (rankA > rankB) {
        alignedB = tmp;
        for (int i = 0; i < rankA - rankB; i++) {
            alignedB[i] = 1;
        }
        for (int i = rankA - rankB; i < rankA; i++) {
            alignedB[i] = shapeB[i - (rankA - rankB)];
        }
        alignedRank = rankA;
    } else if (rankB > rankA) {
        alignedA = tmp;
        for (int i = 0; i < rankB - rankA; i++) {
            alignedA[i] = 1;
        }
        for (int i = rankB - rankA; i < rankB; i++) {
            alignedA[i] = shapeA[i - (rankB - rankA)];
        }
        alignedRank = rankB;
    }

    for (int i = 0; i < alignedRank; i++) {
        if (alignedA[i] == alignedB[i]) {
            shapeY[i] = alignedA[i];
        } else {
            if (alignedA[i] == 1) {
                shapeY[i] = alignedB[i];
            } else if (alignedB[i] == 1) {
                shapeY[i] = alignedA[i];
            } else {
                goto err;
            }
        }
    }
    nn_vm_set_dynamic_shape_for_repr_(nv, shapeY, alignedRank, y);
    return 0;
err:
    return -1;
}

qtk_maybe_unused static int
multidirectional_broadcasting_shape_infer_(qtk_nn_vm_t *nv, qtk_nn_vm_repr_t a,
                                           qtk_nn_vm_repr_t b,
                                           qtk_nn_vm_repr_t y) {
    int rankA, rankB;
    uint32_t *shapeA = qtk_nn_get_shape_from_repr(nv, a);
    uint32_t *shapeB = qtk_nn_get_shape_from_repr(nv, b);
    rankA = qtk_nn_get_rank_from_repr(nv, a);
    rankB = qtk_nn_get_rank_from_repr(nv, b);
    return multidirectional_broadcasting_shape_infer_impl_(nv, shapeA, shapeB,
                                                           rankA, rankB, y);
}

#ifdef __cplusplus
};
#endif
#endif
