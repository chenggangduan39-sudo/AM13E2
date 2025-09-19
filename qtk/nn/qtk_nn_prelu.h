#ifndef QBL_NN_QBL_NN_PRELU_H
#define QBL_NN_QBL_NN_PRELU_H
#pragma once
#include "qtk/core/qtk_binary.h"
#include "qtk/core/qtk_type.h"
#include "qtk/nn/qtk_nn_utils.h"
#include "qtk/nn/vm/qtk_nn_vm.h"
#include "qtk/numeric/qtk_ndarray_iter.h"
#include "qtk/numeric/qtk_numeric_type.h"
#ifdef __cplusplus
extern "C" {
#endif

qtk_maybe_unused static void vm_PRelu_(qtk_nn_vm_t *nv, uint8_t **instructions) {
    uint16_t x = qtk_littleEndian_uint16_from_bin(*instructions),
             s = qtk_littleEndian_uint16_from_bin(*instructions + 2),
             y = qtk_littleEndian_uint16_from_bin(*instructions + 4);
    qtk_numeric_data_t X, Y, slope;

    X.raw = qtk_nn_get_loc_from_repr(nv, x);
    slope.raw = qtk_nn_get_loc_from_repr(nv, s);
    Y.raw = qtk_nn_get_loc_from_repr(nv, y);

    int rankX = qtk_nn_get_rank_from_repr(nv, x);
    int rankS = qtk_nn_get_rank_from_repr(nv, s);
    int rankY = qtk_nn_get_rank_from_repr(nv, y);

    uint32_t *shapeX = qtk_nn_get_shape_from_repr(nv, x);
    uint32_t *shapeS = qtk_nn_get_shape_from_repr(nv, s);
    uint32_t *shapeY = qtk_nn_get_shape_from_repr(nv, y);

    qtk_assert(QBL_NN_TENSOR_GET_ELEM_TYPE(x) == QBL_NN_VM_TENSOR_ELEM_F32);
    int rank[3] = {rankX, rankS, rankY};
    uint32_t *shape[3] = {shapeX, shapeS, shapeY};
    qtk_numeric_data_t data[3] = {X, slope, Y};
    uint8_t elem_szs[3] = {4, 4, 4};

    qtk_ndarray_iter_t iter;
    qtk_ndarray_iter_init(&iter, data, rank, shape, shapeY, rankY, 3, elem_szs);

    if (X.raw == Y.raw) {
        do {
            qtk_numeric_data_t xx, ss, yy;
            xx = iter.data[0];
            ss = iter.data[1];
            yy = iter.data[2];
            if (*xx.f32 < 0) {
                *yy.f32 *= *ss.f32;
            }
        } while (qtk_ndarray_iter_next(&iter));
    } else {
        do {
            qtk_numeric_data_t xx, ss, yy;
            xx = iter.data[0];
            ss = iter.data[1];
            yy = iter.data[2];
            if (*xx.f32 < 0) {
                *yy.f32 = *ss.f32 * *xx.f32;
            } else {
                *yy.f32 = *xx.f32;
            }
        } while (qtk_ndarray_iter_next(&iter));
    }

    *instructions += 6;
}

qtk_maybe_unused static int vm_PRelu_infer_shape_(qtk_nn_vm_t *nv,
                                                 uint8_t *instructions) {
    uint16_t x = qtk_littleEndian_uint16_from_bin(instructions),
             y = qtk_littleEndian_uint16_from_bin(instructions + 4);
    nn_vm_clone_shape_for_repr_(nv, x, y);
    return 0;
}

#ifdef __cplusplus
};

#endif
#endif
