#ifndef QBL_NN_QBL_NN_TOPK_H
#define QBL_NN_QBL_NN_TOPK_H
#pragma once
#include "qtk/core/qtk_binary.h"
#include "qtk/core/qtk_min_heap.h"
#include "qtk/nn/qtk_nn_utils.h"
#include "qtk/nn/vm/qtk_nn_vm.h"
#ifdef __cplusplus
extern "C" {
#endif

qtk_maybe_unused static int vm_TopK_infer_shape_(qtk_nn_vm_t *nv,
                                                 uint8_t *instructions) {
    uint16_t x, k, t, i;
    uint32_t *shapeX, shapeT[32], shapeI[32];
    uint8_t *extra;
    int8_t axis;
    int rankX;
    x = qtk_littleEndian_uint16_from_bin(instructions);
    k = qtk_littleEndian_uint16_from_bin(instructions + 2);
    t = qtk_littleEndian_uint16_from_bin(instructions + 4);
    i = qtk_littleEndian_uint16_from_bin(instructions + 6);
    extra = instructions + 8;
    qtk_numeric_data_t K;

    axis = cast(int8_t *, extra)[0];

    shapeX = qtk_nn_get_shape_from_repr(nv, x);

    K.raw = qtk_nn_get_loc_from_repr(nv, k);

    rankX = qtk_nn_get_rank_from_repr(nv, x);
    if (axis < 0) {
        axis += rankX;
    }

    qtk_assert(axis == rankX - 1);

    memcpy(shapeT, shapeX, sizeof(shapeX[0]) * rankX);
    shapeT[rankX - 1] = K.i64[0];
    memcpy(shapeI, shapeT, sizeof(shapeX[0]) * rankX);

    nn_vm_set_dynamic_shape_for_repr_(nv, shapeT, rankX, t);
    nn_vm_set_dynamic_shape_for_repr_(nv, shapeI, rankX, i);

    return 0;
}

struct sitem_ {
    uint32_t idx;
    qtk_numeric_data_t data;
};

static int sitem_f32_cmp_(struct sitem_ *a, struct sitem_ *b) {
    if (a->data.f32[0] > b->data.f32[0]) {
        return -1;
    }
    if (a->data.f32[0] < b->data.f32[0]) {
        return 1;
    }
    return 0;
}

qtk_maybe_unused static void vm_TopK_(qtk_nn_vm_t *nv, uint8_t **instructions) {

    uint16_t x, k, t, i, extra_index;
    uint32_t *shapeX;
    int8_t axis, largest, is_sorted;
    uint32_t pre_dim = 1, post_dim;
    int min_heap_mem_require;
    int rankX;
    qtk_min_heap_t *mheap;
    qtk_nn_vm_tensor_elem_type_t elem_type;
    x = qtk_littleEndian_uint16_from_bin(*instructions);
    k = qtk_littleEndian_uint16_from_bin(*instructions + 2);
    t = qtk_littleEndian_uint16_from_bin(*instructions + 4);
    i = qtk_littleEndian_uint16_from_bin(*instructions + 6);
    extra_index = qtk_littleEndian_uint16_from_bin(*instructions + 8);
    qtk_numeric_data_t K, X, T, I;

    axis = cast(int8_t *, nv->extra)[extra_index];
    largest = cast(int8_t *, nv->extra)[extra_index + 1];
    is_sorted = cast(int8_t *, nv->extra)[extra_index + 2];

    shapeX = qtk_nn_get_shape_from_repr(nv, x);

    K.raw = qtk_nn_get_loc_from_repr(nv, k);
    X.raw = qtk_nn_get_loc_from_repr(nv, x);
    T.raw = qtk_nn_get_loc_from_repr(nv, t);
    I.raw = qtk_nn_get_loc_from_repr(nv, i);

    rankX = qtk_nn_get_rank_from_repr(nv, x);
    if (axis < 0) {
        axis += rankX;
    }

    qtk_assert(axis == rankX - 1 && largest == 1 && is_sorted == 1);

    for (int i = 0; i < rankX - 1; i++) {
        pre_dim *= shapeX[i];
    }
    post_dim = shapeX[rankX - 1];

    qtk_min_heap_init(NULL, &min_heap_mem_require, sizeof(struct sitem_),
                      post_dim, NULL);
    if (nv->workspace_cap < min_heap_mem_require) {
        wtk_free(nv->workspace.raw);
        nv->workspace.raw = wtk_malloc(min_heap_mem_require);
    }

    elem_type = QBL_NN_TENSOR_GET_ELEM_TYPE(x);
    qtk_assert(elem_type == QBL_NN_VM_TENSOR_ELEM_F32);

    for (uint32_t i = 0; i < pre_dim; i++) {
        mheap = qtk_min_heap_init(nv->workspace.raw, &min_heap_mem_require,
                                  sizeof(struct sitem_), post_dim,
                                  cast(qtk_min_heap_cmp_f, sitem_f32_cmp_));
        struct sitem_ item;
        for (uint32_t j = 0; j < post_dim; j++) {
            item.idx = j;
            item.data.f32 = X.f32 + i * post_dim + j;
            qtk_min_heap_push(mheap, &item);
        }
        for (uint32_t j = 0; j < K.i64[0]; j++) {
            qtk_min_heap_pop(mheap, &item);
            I.i64[i * K.i64[0] + j] = item.idx;
            T.f32[i * K.i64[0] + j] = item.data.f32[0];
        }
    }

    *instructions += 10;
}

#ifdef __cplusplus
};
#endif
#endif
