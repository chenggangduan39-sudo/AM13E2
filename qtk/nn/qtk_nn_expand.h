#ifndef G_D099GBCQ_LLWT_9H6I_US0O_KV3GLSMAKFD0
#define G_D099GBCQ_LLWT_9H6I_US0O_KV3GLSMAKFD0
#pragma once
#include "qtk/core/qtk_binary.h"
#include "qtk/nn/qtk_nn_shape_infer.h"
#include "qtk/nn/qtk_nn_utils.h"
#include "qtk/nn/vm/qtk_nn_vm.h"
#ifdef __cplusplus
extern "C" {
#endif

qtk_maybe_unused static int vm_Expand_infer_shape_(qtk_nn_vm_t *nv,
                                                   uint8_t *instructions) {
    uint32_t *shapeX, *shapeShape;
    uint16_t x = qtk_littleEndian_uint16_from_bin(instructions),
             shape = qtk_littleEndian_uint16_from_bin(instructions + 2),
             y = qtk_littleEndian_uint16_from_bin(instructions + 4);
    qtk_numeric_data_t Shape;
    int rankX;
    uint32_t shape_tmp[16] = {0, 0, 0, 0};

    shapeX = qtk_nn_get_shape_from_repr(nv, x);
    rankX = qtk_nn_get_rank_from_repr(nv, x);
    shapeShape = qtk_nn_get_shape_from_repr(nv, shape);

    Shape.raw = qtk_nn_get_loc_from_repr(nv, shape);
    for (int i = 0; i < shapeShape[0]; i++) {
        shape_tmp[i] = Shape.i64[i];
    }
    multidirectional_broadcasting_shape_infer_impl_(
        nv, shapeX, shape_tmp, rankX, cast(int, shapeShape[0]), y);
    return 0;
}

qtk_maybe_unused static void vm_Expand_(qtk_nn_vm_t *nv,
                                        uint8_t **instructions) {
    uint16_t x = qtk_littleEndian_uint16_from_bin(*instructions),
             y = qtk_littleEndian_uint16_from_bin(*instructions + 4);
    qtk_numeric_data_t Y, X;
    int tmp[5],nelem = 1;

    X.raw = qtk_nn_get_loc_from_repr(nv, x);
    Y.raw = qtk_nn_get_loc_from_repr(nv, y);

    int rankX = qtk_nn_get_rank_from_repr(nv, x);
    uint32_t *in_shape = qtk_nn_get_shape_from_repr(nv, x);
    uint32_t *oshape = qtk_nn_get_shape_from_repr(nv, y);
    int elem_sz = nn_vm_tensor_elem_sz_[QBL_NN_TENSOR_GET_ELEM_TYPE(x)];
    for(int i = 0; i < rankX; i++){
        tmp[i] = oshape[i]/in_shape[i];
    }

    if(tmp[1] != 0){
        for(int i = 2; i < rankX; i++){
            nelem *= in_shape[i];
        }
        if(elem_sz == 1){
            int8_t *to = Y.boolean;
            int8_t *from = X.boolean;
            for(int i = 0; i < oshape[0]; i++){
                for(int j =0; j < oshape[1]; j++){
                    memcpy(to, from, elem_sz * nelem);
                    to += nelem;
                }
                from += nelem;
            }
        }else if(elem_sz == 4){
            float *to = Y.f32;
            float *from = X.f32;
            for(int i = 0; i < oshape[0]; i++){
                for(int j =0; j < oshape[1]; j++){
                    memcpy(to, from, elem_sz * nelem);
                    to += nelem;
                }
                from += nelem;
            }
        }

    }

    *instructions += 6;
}

#ifdef __cplusplus
};
#endif
#endif /* G_D099GBCQ_LLWT_9H6I_US0O_KV3GLSMAKFD0 */
