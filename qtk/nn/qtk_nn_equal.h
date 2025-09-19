#ifndef QBL_NN_QBL_NN_EQUAL_H
#define QBL_NN_QBL_NN_EQUAL_H
#pragma once
#include "qtk/core/qtk_binary.h"
#include "qtk/math/qtk_vector.h"
#include "qtk/nn/qtk_nn_shape_infer.h"
#include "qtk/nn/vm/qtk_nn_vm.h"
#ifdef __cplusplus
extern "C" {
#endif

qtk_maybe_unused static int vm_Equal_infer_shape_(qtk_nn_vm_t *nv,
                                                  uint8_t *instructions) {
    uint16_t a = qtk_littleEndian_uint16_from_bin(instructions),
             b = qtk_littleEndian_uint16_from_bin(instructions + 2),
             y = qtk_littleEndian_uint16_from_bin(instructions + 4);
    return multidirectional_broadcasting_shape_infer_(nv, a, b, y);
}

static void equal1_(qtk_nn_vm_t *nv, uint32_t *merge_shape,
                    qtk_numeric_data_t A, qtk_numeric_data_t B,
                    qtk_numeric_data_t Y,
                    qtk_nn_vm_tensor_elem_type_t elem_type) {
    uint32_t pre_dim = merge_shape[0];
    uint32_t last_dim = merge_shape[1];
    switch (elem_type) {
    case QBL_NN_VM_TENSOR_ELEM_F32:
        for (int i = 0; i < pre_dim; i++) {
            for (int j = 0; j < last_dim; j++) {
                Y.boolean[i * last_dim + j] =
                    qtk_fabs(A.f32[i * last_dim + j] - B.f32[j]) > 1e-6 ? 0 : 1;
            }
        }
        break;
    case QBL_NN_VM_TENSOR_ELEM_I64:
        for (int i = 0; i < pre_dim; i++) {
            for (int j = 0; j < last_dim; j++) {
                Y.boolean[i * last_dim + j] =
                    A.i64[i * last_dim + j] == B.i64[j] ? 1 : 0;
            }
        }
        break;
    default:
        qtk_assert(0);
    }
}

static void equal2_(qtk_nn_vm_t *nv, uint32_t *merge_shape,
                    qtk_numeric_data_t A, qtk_numeric_data_t B,
                    qtk_numeric_data_t Y,
                    qtk_nn_vm_tensor_elem_type_t elem_type) {
    uint32_t pre_dim = merge_shape[0];
    uint32_t last_dim = merge_shape[1];
    switch (elem_type) {
    case QBL_NN_VM_TENSOR_ELEM_F32:
        for (int i = 0; i < pre_dim; i++) {
            for (int j = 0; j < last_dim; j++) {
                Y.boolean[i * last_dim + j] =
                    qtk_fabs(A.f32[i * last_dim + j] - B.f32[i]) > 1e-6 ? 0 : 1;
            }
        }
        break;
    case QBL_NN_VM_TENSOR_ELEM_I64:
        for (int i = 0; i < pre_dim; i++) {
            for (int j = 0; j < last_dim; j++) {
                Y.boolean[i * last_dim + j] =
                    A.i64[i * last_dim + j] == B.i64[i] ? 1 : 0;
            }
        }
        break;
    default:
        qtk_assert(0);
    }
}

qtk_maybe_unused static void vm_Equal_(qtk_nn_vm_t *nv,
                                       uint8_t **instructions) {
    uint32_t *shapeB, *shapeA;
    uint16_t a = qtk_littleEndian_uint16_from_bin(*instructions),
             b = qtk_littleEndian_uint16_from_bin(*instructions + 2),
             y = qtk_littleEndian_uint16_from_bin(*instructions + 4);
    qtk_numeric_data_t A, B, Y;
    int rankA;
    int rankB;
    qtk_nn_vm_tensor_elem_type_t elem_type;
    uint32_t merge_shape[2] = {1, 1};

    shapeB = qtk_nn_get_shape_from_repr(nv, b);
    elem_type = QBL_NN_TENSOR_GET_ELEM_TYPE(a);

    A.raw = qtk_nn_get_loc_from_repr(nv, a);
    B.raw = qtk_nn_get_loc_from_repr(nv, b);
    Y.raw = qtk_nn_get_dynamic_loc(nv, QBL_NN_TENSOR_GET_INDEX(y));
    shapeA = qtk_nn_get_shape_from_repr(nv, a);

    rankA = qtk_nn_get_rank_from_repr(nv, a);
    rankB = qtk_nn_get_rank_from_repr(nv, b);

    if (rankA < rankB) {
        qtk_numeric_data_t tmp;
        int rank_tmp;
        uint32_t *tmp_shape;
        tmp_shape = shapeA;
        shapeA = shapeB;
        shapeB = tmp_shape;
        tmp = A;
        A = B;
        B = tmp;
        rank_tmp = rankA;
        rankA = rankB;
        rankB = rank_tmp;
    }

    if (rankB < rankA) {
        int last_merged = 0;
        for (last_merged = 0; last_merged < rankB; last_merged++) {
            if (shapeB[last_merged] != shapeA[rankA - rankB + last_merged]) {
                break;
            }
            merge_shape[1] *= shapeB[last_merged];
        }
        for (int i = 0; i < rankA - rankB; i++) {
            merge_shape[0] *= shapeA[i];
        }
        if (last_merged == rankB) {
            equal1_(nv, merge_shape, A, B, Y, elem_type);
        } else {
            qtk_assert(last_merged == rankB - 1);
            merge_shape[0] *= shapeA[rankA - rankB];
            equal1_(nv, merge_shape, A, B, Y, elem_type);
        }
    } else if (rankA == rankB) {
        int rankMerge = 0;
        for (rankMerge = 0;
             rankMerge < rankA && shapeA[rankMerge] == shapeB[rankMerge];
             rankMerge++) {
            merge_shape[0] *= shapeA[rankMerge];
        }
        if (rankMerge == rankA) {
            merge_shape[1] = merge_shape[0];
            merge_shape[0] = 1;
            equal1_(nv, merge_shape, A, B, Y, elem_type);
        } else {
            qtk_assert(rankMerge == rankA - 1);
            if (shapeA[rankA - 1] == shapeB[rankB - 1]) {
                merge_shape[1] = shapeA[rankA - 1];
                equal1_(nv, merge_shape, A, B, Y, elem_type);
            } else {
                if (shapeA[rankA - 1] == 1) {
                    merge_shape[1] = shapeB[rankA - 1];
                    equal2_(nv, merge_shape, B, A, Y, elem_type);
                } else if (shapeB[rankA - 1] == 1) {
                    merge_shape[1] = shapeA[rankA - 1];
                    equal2_(nv, merge_shape, A, B, Y, elem_type);
                }
            }
        }
    }

    *instructions += 6;
}

#ifdef __cplusplus
};
#endif
#endif
