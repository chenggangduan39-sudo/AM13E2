#ifndef QBL_NN_QBL_NN_SFE_H
#define QBL_NN_QBL_NN_SFE_H
#pragma once
#include "qtk/core/qtk_binary.h"
#include "qtk/core/qtk_type.h"
#include "qtk/nn/qtk_nn_utils.h"
#include "qtk/nn/vm/qtk_nn_vm.h"
#include "qtk/numeric/qtk_numeric_type.h"
#ifdef __cplusplus
extern "C" {
#endif

qtk_maybe_unused static void vm_Sfe_(qtk_nn_vm_t *nv, uint8_t **instructions) {
    uint16_t x = qtk_littleEndian_uint16_from_bin(*instructions),
             y = qtk_littleEndian_uint16_from_bin(*instructions + 2);
    qtk_numeric_data_t X, Y;
    uint32_t nelem;

    X.raw = qtk_nn_get_loc_from_repr(nv, x);
    Y.raw = qtk_nn_get_loc_from_repr(nv, y);
    uint32_t *shapeX = qtk_nn_get_shape_from_repr(nv, x);
    nelem = nn_vm_tensor_get_nelem_from_repr_(nv, y);

    //F.unfold(x, kernel_size=(1, 3), padding=(0, 1))
    memset(Y.f32, 0, nelem * sizeof(float));
    float *o = Y.f32;
    float *p = X.f32;
    for (int i = 0; i < shapeX[0] * shapeX[1]; i++) {
        p = X.f32 + i * shapeX[3];
        o = Y.f32 + i * shapeX[3] * 3;
        for (int j = 0; j < shapeX[3]; j+=3, o+=3) {
            if(j == 0){
                *o = 0;
                memcpy(o + 1, p, 2 * sizeof(float));
                memcpy(o + shapeX[3], p, 3 * sizeof(float));
                memcpy(o + shapeX[3] * 2, p + 1, 3 * sizeof(float));
                p = p + 2;
            }else if (j == shapeX[3] - 2) {
                memcpy(o , p, 2 * sizeof(float));
                memcpy(o + shapeX[3], p + 1, 2 * sizeof(float));
                memcpy(o + shapeX[3] * 2, p + 2, 1 * sizeof(float));
                *(o + shapeX[3] * 2 + 1) = 0;
            }else{
                memcpy(o, p, 3 * sizeof(float));
                memcpy(o + shapeX[3], p + 1, 3 * sizeof(float));
                memcpy(o + shapeX[3] * 2, p + 2, 3 * sizeof(float));
                p = p + 3;
            }
        }
    }

    *instructions += 4;
}

qtk_maybe_unused static int vm_Sfe_infer_shape_(qtk_nn_vm_t *nv,
                                                 uint8_t *instructions) {
    // uint16_t x = qtk_littleEndian_uint16_from_bin(instructions),
    //          y = qtk_littleEndian_uint16_from_bin(instructions + 2);
    // nn_vm_clone_shape_for_repr_(nv, x, y);
    return 0;
}

#ifdef __cplusplus
};

#endif
#endif
