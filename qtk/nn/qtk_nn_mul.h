#ifndef QBL_NN_QBL_NN_MUL_H
#define QBL_NN_QBL_NN_MUL_H
#pragma once
#include "qtk/core/qtk_binary.h"
#include "qtk/math/qtk_vector.h"
#include "qtk/nn/qtk_nn_shape_infer.h"
#include "qtk/nn/vm/qtk_nn_vm.h"
#include "qtk/numeric/qtk_ndarray_iter.h"
#ifdef __cplusplus
extern "C" {
#endif

qtk_maybe_unused static void vm_Mul_(qtk_nn_vm_t *nv, uint8_t **instructions) {
    uint32_t *shapeB, *shapeA, *shapeY;
    uint16_t a = qtk_littleEndian_uint16_from_bin(*instructions),
             b = qtk_littleEndian_uint16_from_bin(*instructions + 2),
             y = qtk_littleEndian_uint16_from_bin(*instructions + 4);
    qtk_numeric_data_t A, B, Y;
    uint32_t nelem = 1;
    uint32_t idx;
    int rankA;
    int rankB;
    int rankY;
    qtk_nn_vm_tensor_elem_type_t elem_type;

    shapeB = qtk_nn_get_shape_from_repr(nv, b);
    elem_type = QBL_NN_TENSOR_GET_ELEM_TYPE(a);

    A.raw = qtk_nn_get_loc_from_repr(nv, a);
    B.raw = qtk_nn_get_loc_from_repr(nv, b);
    Y.raw = qtk_nn_get_dynamic_loc(nv, QBL_NN_TENSOR_GET_INDEX(y));
    shapeA = qtk_nn_get_shape_from_repr(nv, a);
    shapeY = qtk_nn_get_shape_from_repr(nv, y);
    rankA = qtk_nn_get_rank_from_repr(nv, a);
    rankB = qtk_nn_get_rank_from_repr(nv, b);
    rankY = qtk_nn_get_rank_from_repr(nv, y);

    for (int i = 0; i < rankA; i++) {
        nelem *= shapeA[i];
    }

    if ((rankB == 1 && shapeB[0] == 1) || rankB == 0) {
        switch (elem_type) {
        case QBL_NN_VM_TENSOR_ELEM_F32:
            for (idx = 0; idx < nelem; idx++) {
                Y.f32[idx] = A.f32[idx] * B.f32[0];
            }
            break;
        case QBL_NN_VM_TENSOR_ELEM_I32:
            for (idx = 0; idx < nelem; idx++) {
                Y.i32[idx] = A.i32[idx] * B.i32[0];
            }
            break;
        default:
            qtk_debug("%d\n", QBL_NN_TENSOR_GET_ELEM_TYPE(b));
            qtk_assert(0);
        }
    } else if (rankA == rankB &&
               0 == memcmp(shapeA, shapeB, sizeof(shapeB[0]) * rankB)) {
        switch (elem_type) {
        case QBL_NN_VM_TENSOR_ELEM_F32:
            qtk_vector_multipy_elewise(A.f32, B.f32, Y.f32, nelem);
            break;
        default:
            qtk_debug("%d\n", QBL_NN_TENSOR_GET_ELEM_TYPE(b));
            qtk_assert(0);
        }
    }else{
        qtk_assert(elem_type == QBL_NN_VM_TENSOR_ELEM_F32);
        int rank[3] = {rankA, rankB, rankY};
        uint32_t *shape[3] = {shapeA, shapeB, shapeY};
        qtk_numeric_data_t data[3] = {A, B, Y};
        uint8_t elem_szs[3] = {4, 4, 4};
        qtk_ndarray_iter_t iter;
        qtk_ndarray_iter_init(&iter, data, rank, shape, shapeY, rankA, 3,
                              elem_szs);
        do {
            qtk_numeric_data_t a, b, y;
            a = iter.data[0];
            b = iter.data[1];
            y = iter.data[2];
            *y.f32 = *a.f32 * *b.f32;
        } while (qtk_ndarray_iter_next(&iter));
    }
    *instructions += 6;
}

qtk_maybe_unused static int vm_Mul_infer_shape_(qtk_nn_vm_t *nv,
                                                uint8_t *instructions) {
    uint16_t a = qtk_littleEndian_uint16_from_bin(instructions),
             b = qtk_littleEndian_uint16_from_bin(instructions + 2),
             y = qtk_littleEndian_uint16_from_bin(instructions + 4);
    return multidirectional_broadcasting_shape_infer_(nv, a, b, y);
}

#ifdef __cplusplus
};
#endif
#endif
