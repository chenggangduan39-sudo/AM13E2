#ifndef QBL_NN_QBL_NN_MATMUL_H
#define QBL_NN_QBL_NN_MATMUL_H
#pragma once

#include "qtk/core/qtk_binary.h"
#include "qtk/linalg/qtk_gemm.h"
#include "qtk/nn/qtk_nn_debug.h"
#include "qtk/nn/qtk_nn_utils.h"
#include "qtk/nn/vm/qtk_nn_vm.h"

#ifdef __cplusplus
extern "C" {
#endif

qtk_maybe_unused static int vm_MatMul_infer_shape_(qtk_nn_vm_t *nv,
                                                   uint8_t *instructions) {
    uint16_t a = qtk_littleEndian_uint16_from_bin(instructions),
             b = qtk_littleEndian_uint16_from_bin(instructions + 2),
             y = qtk_littleEndian_uint16_from_bin(instructions + 4);
    uint32_t *shapeA, *shapeB, shapeY[16];
    int rankA, rankB, rankY;

    shapeA = qtk_nn_get_shape_from_repr(nv, a);
    shapeB = qtk_nn_get_shape_from_repr(nv, b);

    rankA = qtk_nn_get_rank_from_repr(nv, a);
    rankB = qtk_nn_get_rank_from_repr(nv, b);

    if (rankB == 2) {
        qtk_assert(rankA >= rankB);
        for (int i = 0; i < rankA - rankB; i++) {
            shapeY[i] = shapeA[i];
        }
        shapeY[rankA - rankB] = shapeA[rankA - rankB];
        shapeY[rankA - rankB + 1] = shapeB[1];
        rankY = rankA - rankB + 2;
    } else {
        qtk_assert(rankA == rankB && rankA > 2);
        for (int i = 0; i < rankA - 2; i++) {
            qtk_assert(shapeA[i] == shapeB[i]);
            shapeY[i] = shapeB[i];
        }
        shapeY[rankA - 2] = shapeA[rankA - 2];
        shapeY[rankA - 1] = shapeB[rankA - 1];
        rankY = rankA;
    }
    nn_vm_set_dynamic_shape_for_repr_(nv, shapeY, rankY, y);
    return 0;
}

qtk_maybe_unused static void vm_MatMul_(qtk_nn_vm_t *nv,
                                        uint8_t **instructions) {
    uint16_t a = qtk_littleEndian_uint16_from_bin(*instructions),
             b = qtk_littleEndian_uint16_from_bin(*instructions + 2),
             y = qtk_littleEndian_uint16_from_bin(*instructions + 4);
    uint32_t *shapeA, *shapeB;
    qtk_numeric_data_t A, B, Y;
    qtk_nn_vm_tensor_elem_type_t elem_type;
    int rankA, rankB;

    shapeA = qtk_nn_get_shape_from_repr(nv, a);
    shapeB = qtk_nn_get_shape_from_repr(nv, b);
    rankA = qtk_nn_get_rank_from_repr(nv, a);
    rankB = qtk_nn_get_rank_from_repr(nv, b);

    elem_type = QBL_NN_TENSOR_GET_ELEM_TYPE(y);

    A.raw = qtk_nn_get_loc_from_repr(nv, a);
    B.raw = qtk_nn_get_loc_from_repr(nv, b);
    Y.raw = qtk_nn_get_loc_from_repr(nv, y);

    if (elem_type == QBL_NN_VM_TENSOR_ELEM_F32) {
        if (rankB == 2) {
            qtk_assert(rankA >= 2);
            uint32_t nblock = 1;
            for (int i = 0; i < rankA - rankB; i++) {
                nblock *= shapeA[i];
            }

            for (int i = 0; i < nblock; i++) {
                int m, n, k;

                m = shapeA[rankA - rankB];
                n = shapeB[1];
                k = shapeB[0];

                memset(Y.f32 + i * m * n, 0, m * n * sizeof(float));
                qtk_sgemm(0, 0, m, n, k, 1.0, A.f32 + i * m * k, k, B.f32, n,
                          0.0, Y.f32 + i * m * n, n);
            }
        } else {
            qtk_assert(rankA == rankB && rankA > 2);
            uint32_t nblock = 1;
            for (int i = 0; i < rankA - 2; i++) {
                nblock *= shapeA[i];
            }
            for (int i = 0; i < nblock; i++) {
                int m, n, k;

                m = shapeA[rankA - 2];
                n = shapeB[rankA - 1];
                k = shapeB[rankA - 2];

                memset(Y.f32 + i * m * n, 0, m * n * sizeof(float));
                qtk_sgemm(0, 0, m, n, k, 1.0, A.f32 + i * m * k, k,
                          B.f32 + i * k * n, n, 0.0, Y.f32 + i * m * n, n);
            }
        }
    } else {
        qtk_assert(0);
    }

    *instructions += 6;
}

#ifdef __cplusplus
};
#endif
#endif
