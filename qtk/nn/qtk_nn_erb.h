#ifndef QBL_NN_QBL_NN_ERB_H
#define QBL_NN_QBL_NN_ERB_H
#pragma once
#include "qtk/core/qtk_binary.h"
#include "qtk/core/qtk_type.h"
#include "qtk/nn/qtk_nn_utils.h"
#include "qtk/nn/vm/qtk_nn_vm.h"
#include "qtk/numeric/qtk_numeric_type.h"
#ifdef __cplusplus
extern "C" {
#endif

qtk_maybe_unused static void vm_Erb_(qtk_nn_vm_t *nv, uint8_t **instructions) {
    uint16_t x = qtk_littleEndian_uint16_from_bin(*instructions),
             xx = qtk_littleEndian_uint16_from_bin(*instructions + 2),
             x2 = qtk_littleEndian_uint16_from_bin(*instructions + 4),
             x3 = qtk_littleEndian_uint16_from_bin(*instructions + 6),
             y = qtk_littleEndian_uint16_from_bin(*instructions + 8);
    qtk_numeric_data_t X1, X2, X3, Y;
    uint32_t nelem;

    X1.raw = qtk_nn_get_loc_from_repr(nv, x);//input
    X2.raw = qtk_nn_get_loc_from_repr(nv, x2);//erb_bank
    X3.raw = qtk_nn_get_loc_from_repr(nv, x3);//erb_index  cnt start, cnt2 start2
    Y.raw = qtk_nn_get_loc_from_repr(nv, y);//output

    uint32_t *shapex1 = qtk_nn_get_shape_from_repr(nv, x);
    uint32_t *shapex3 = qtk_nn_get_shape_from_repr(nv, x3);
    uint32_t *shapey = qtk_nn_get_shape_from_repr(nv, y);
    nelem = nn_vm_tensor_get_nelem_from_repr_(nv, y);
    memset(Y.f32, 0, nelem * sizeof(float));

    int split = shapey[3] - shapex3[0];
    float *erb_bank = X2.f32;
    float *p;
    float *out;
    int row = shapex1[0] * shapex1[1];
    int stride_in = shapex1[2] * shapex1[3];
    for(int i = 0; i < row; i++){
        p = X1.f32 + i * stride_in + split;
        erb_bank = X2.f32;
        out = Y.f32 + i * shapey[3] + split;
        for(int j = 0; j < shapex3[0]; j++, out++){
            int index = *(X3.i32 + j * 2);
            int cnt = *(X3.i32 + j * 2 + 1);
            for(int k = 0; k < cnt; k++, erb_bank++){
                *out += p[index + k] * *erb_bank;
            }
        }
        memcpy(Y.f32 + i * shapey[3], X1.f32 + i * stride_in, split * sizeof(float));
    }

    *instructions += 10;
}

qtk_maybe_unused static int vm_Erb_infer_shape_(qtk_nn_vm_t *nv,
                                                 uint8_t *instructions) {
    // uint16_t x = qtk_littleEndian_uint16_from_bin(instructions),
    //          y = qtk_littleEndian_uint16_from_bin(instructions + 2);
    // nn_vm_clone_shape_for_repr_(nv, x, y);
    wtk_debug("not implement\n");
    exit(0);
    return 0;
}

#ifdef __cplusplus
};

#endif
#endif
