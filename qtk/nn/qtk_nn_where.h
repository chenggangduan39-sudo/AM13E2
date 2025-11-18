#ifndef QBL_NN_QBL_NN_WHERE_H
#define QBL_NN_QBL_NN_WHERE_H
#pragma once
#include "qtk/core/qtk_binary.h"
#include "qtk/math/qtk_vector.h"
#include "qtk/nn/qtk_nn_shape_infer.h"
#include "qtk/nn/qtk_nn_utils.h"
#include "qtk/nn/vm/qtk_nn_vm.h"
#include "qtk/numeric/qtk_ndarray_iter.h"
#ifdef __cplusplus
extern "C" {
#endif

qtk_maybe_unused static int vm_Where_infer_shape_(qtk_nn_vm_t *nv,
                                                  uint8_t *instructions) {
    uint16_t a = qtk_littleEndian_uint16_from_bin(instructions + 2),
             b = qtk_littleEndian_uint16_from_bin(instructions + 4),
             y = qtk_littleEndian_uint16_from_bin(instructions + 6);
    return multidirectional_broadcasting_shape_infer_(nv, a, b, y);
}

qtk_maybe_unused static void vm_Where_(qtk_nn_vm_t *nv,
                                       uint8_t **instructions) {
    uint16_t cond = qtk_littleEndian_uint16_from_bin(*instructions),
             a = qtk_littleEndian_uint16_from_bin(*instructions + 2),
             b = qtk_littleEndian_uint16_from_bin(*instructions + 4),
             y = qtk_littleEndian_uint16_from_bin(*instructions + 6);
    uint32_t *out_shape = qtk_nn_get_shape_from_repr(nv, y);
    uint32_t *cond_shape = qtk_nn_get_shape_from_repr(nv, cond);
    uint32_t *shapeA = qtk_nn_get_shape_from_repr(nv, a);
    uint32_t *shapeB = qtk_nn_get_shape_from_repr(nv, b);
    qtk_ndarray_iter_t iter;
    int rankStd, rankA, rankB, rank_cond;

    qtk_numeric_data_t A, B, Y, Cond;
    qtk_nn_vm_tensor_elem_type_t elem_type;
    int elem_sz;
    rankStd = qtk_nn_get_rank_from_repr(nv, y);
    rankA = qtk_nn_get_rank_from_repr(nv, a);
    rankB = qtk_nn_get_rank_from_repr(nv, b);
    rank_cond = qtk_nn_get_rank_from_repr(nv, cond);

    A.raw = qtk_nn_get_loc_from_repr(nv, a);
    B.raw = qtk_nn_get_loc_from_repr(nv, b);
    Cond.raw = qtk_nn_get_loc_from_repr(nv, cond);
    Y.raw = qtk_nn_get_dynamic_loc(nv, QBL_NN_TENSOR_GET_INDEX(y));
    elem_type = QBL_NN_TENSOR_GET_ELEM_TYPE(y);
    elem_sz = nn_vm_tensor_elem_sz_[elem_type];

    int rank[4] = {rankA, rankB, rank_cond, rankStd};
    uint32_t *shape[4] = {shapeA, shapeB, cond_shape, out_shape};
    qtk_numeric_data_t data[4] = {A, B, Cond, Y};
    uint8_t elem_szs[4] = {elem_sz, elem_sz, 1, elem_sz};
    qtk_ndarray_iter_init(&iter, data, rank, shape, out_shape, rankStd, 4,
                          elem_szs);

    do {
        qtk_numeric_data_t a, b, cond, y;
        a = iter.data[0];
        b = iter.data[1];
        cond = iter.data[2];
        y = iter.data[3];
        if (cond.boolean[0]) {
            memcpy(y.raw, a.raw, elem_sz);
        } else {
            memcpy(y.raw, b.raw, elem_sz);
        }
    } while (qtk_ndarray_iter_next(&iter));

    *instructions += 8;
}

#ifdef __cplusplus
};
#endif
#endif
