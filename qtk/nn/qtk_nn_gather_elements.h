#ifndef QBL_NN_QBL_NN_GATHER_ELEMENTS_H
#define QBL_NN_QBL_NN_GATHER_ELEMENTS_H
#pragma once
#include "qtk/core/qtk_binary.h"
#include "qtk/core/qtk_type.h"
#include "qtk/nn/qtk_nn_utils.h"
#include "qtk/nn/vm/qtk_nn_vm.h"
#include "qtk/numeric/qtk_numeric_type.h"
#ifdef __cplusplus
extern "C" {
#endif

static void nn_gather_elements_f32_(float *x, float *y, int32_t *indice, int shapeA, int shapeB, int col){
    float *p;
    for(int i = 0; i < shapeA; i++){
        p = x + i*col;
        for(int j = 0; j < shapeB; j++){
            *y = p[*indice];
            y++;
            indice++;
        }
    }
}

qtk_maybe_unused static void vm_GatherElements_(qtk_nn_vm_t *nv, uint8_t **instructions) {
    uint16_t x = qtk_littleEndian_uint16_from_bin(*instructions),
             indices = qtk_littleEndian_uint16_from_bin(*instructions + 2),
             y = qtk_littleEndian_uint16_from_bin(*instructions + 4);
    qtk_numeric_data_t X, I, Y;
    int8_t axis;
    uint16_t extra_index;
    int rankX;
    uint32_t *shape,*shapeX;

    X.raw = qtk_nn_get_loc_from_repr(nv, x);
    I.raw = qtk_nn_get_loc_from_repr(nv, indices);
    Y.raw = qtk_nn_get_loc_from_repr(nv, y);
    rankX = qtk_nn_get_rank_from_repr(nv, x);
    *instructions += 6;

    extra_index = qtk_littleEndian_uint16_from_bin(*instructions);
    *instructions += 2;
    uint8_t *extra = cast(uint8_t *, nv->extra) + extra_index;
    axis = cast(int8_t *, nv->extra)[extra_index];

    shape = qtk_nn_get_shape_from_repr(nv, indices);
    shapeX = qtk_nn_get_shape_from_repr(nv, x);
    if (axis < 0) {
        //TODO
    }else{
        nn_gather_elements_f32_(X.f32, Y.f32, I.i32, shape[0], shape[1], shapeX[1]);
    }
}

qtk_maybe_unused static int vm_GatherElements_infer_shape_(qtk_nn_vm_t *nv,
                                                 uint8_t *instructions) {
    uint16_t x = qtk_littleEndian_uint16_from_bin(instructions),
             indices = qtk_littleEndian_uint16_from_bin(*instructions + 2),
             y = qtk_littleEndian_uint16_from_bin(instructions + 4);
    nn_vm_clone_shape_for_repr_(nv, indices, y);
    return 0;
}

#ifdef __cplusplus
};

#endif
#endif
