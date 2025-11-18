#ifndef QBL_NN_QBL_NN_ADD_H
#define QBL_NN_QBL_NN_ADD_H
#pragma once
#include "qtk/core/qtk_binary.h"
#include "qtk/math/qtk_vector.h"
#include "qtk/nn/qtk_nn_shape_infer.h"
#include "qtk/nn/vm/qtk_nn_vm.h"
#ifdef __cplusplus
extern "C" {
#endif

qtk_maybe_unused static int vm_Add_infer_shape_(qtk_nn_vm_t *nv,
                                                uint8_t *instructions) {
    uint16_t a = qtk_littleEndian_uint16_from_bin(instructions),
             b = qtk_littleEndian_uint16_from_bin(instructions + 2),
             y = qtk_littleEndian_uint16_from_bin(instructions + 4);
    return multidirectional_broadcasting_shape_infer_(nv, a, b, y);
}

qtk_maybe_unused static void vm_Add_(qtk_nn_vm_t *nv, uint8_t **instructions) {
    uint32_t *shapeB, *shapeA;
    uint16_t a = qtk_littleEndian_uint16_from_bin(*instructions),
             b = qtk_littleEndian_uint16_from_bin(*instructions + 2),
             y = qtk_littleEndian_uint16_from_bin(*instructions + 4);
    qtk_numeric_data_t A, B, Y;
    uint32_t nelem = 1;
    uint32_t idx;
    int rankA;
    int rankB;
    qtk_nn_vm_tensor_elem_type_t elem_type;

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

    if (rankB == 0 || (rankB == 1 && shapeB[0] == 1)) {
        for (int i = 0; i < rankA; i++) {
            nelem *= shapeA[i];
        }
        switch (elem_type) {
        case QBL_NN_VM_TENSOR_ELEM_F32:
            for (idx = 0; idx < nelem; idx++) {
                Y.f32[idx] = A.f32[idx] + B.f32[0];
            }
            break;
        case QBL_NN_VM_TENSOR_ELEM_I64:
            for (idx = 0; idx < nelem; idx++) {
                Y.i64[idx] = A.i64[idx] + B.i64[0];
            }
            break;
        default:
            qtk_debug("%d\n", QBL_NN_TENSOR_GET_ELEM_TYPE(b));
            qtk_assert(0);
        }
    } else if (rankB == 1 && shapeB[0] == shapeA[rankA - 1]) {
        uint32_t mat_shape[2] = {1, shapeB[0]};
        for (int i = 0; i < rankA - 1; i++) {
            mat_shape[0] *= shapeA[i];
        }

        switch (elem_type) {
        case QBL_NN_VM_TENSOR_ELEM_F32:
            for (int i = 0; i < mat_shape[0]; i++) {
                qtk_vector_add_elewise(A.f32 + i * mat_shape[1], B.f32,
                                       Y.f32 + i * mat_shape[1], mat_shape[1]);
            }
            break;
        case QBL_NN_VM_TENSOR_ELEM_I64:
        default:
            qtk_debug("%d\n", QBL_NN_TENSOR_GET_ELEM_TYPE(b));
            qtk_assert(0);
        }

    } else {
        uint32_t last_dim = 1, pre_dim = 1;
        int flag  = 0;
        for (int i = 0; i < rankB; i++) {
            if(shapeB[i] != shapeA[rankA - rankB + i]){
                flag = 1;
            }
            if(flag == 0){
                last_dim *= shapeB[i];
            }else{
                pre_dim *= shapeA[i];
            }
        }

        if(flag == 0){
            for (int i = 0; i < rankA - rankB; i++) {
                pre_dim *= shapeA[i];
            }

            if (elem_type == QBL_NN_VM_TENSOR_ELEM_F32) {
                for (int i = 0; i < pre_dim; i++) {
                    qtk_vector_add_elewise(A.f32 + i * last_dim, B.f32,
                                           Y.f32 + i * last_dim, last_dim);
                }
            } else if (elem_type == QBL_NN_VM_TENSOR_ELEM_I32) {
                for (int i = 0; i < pre_dim; i++) {
                    qtk_vector_add_elewise_i32(A.i32 + i * last_dim, B.i32,
                                               Y.i32 + i * last_dim, last_dim);
                }
            }
        }else{
            float *po = Y.f32;
            float *pi = A.f32;
            for(int i = 0; i < last_dim; i++){
                for(int j = 0; j < pre_dim; j++){
                    *po++ = *pi++ + B.f32[i];
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
