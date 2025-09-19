#ifndef QBL_NN_QBL_NN_QUANTIZE_H
#define QBL_NN_QBL_NN_QUANTIZE_H
#pragma once
#include "qtk/core/qtk_binary.h"
#include "qtk/core/qtk_type.h"
#include "qtk/nn/qtk_nn_utils.h"
#include "qtk/nn/vm/qtk_nn_vm.h"
#include "qtk/numeric/qtk_numeric_type.h"
#ifdef __cplusplus
extern "C" {
#endif

static float q_max(float *x, uint32_t nelem) {
    float ret = 0;
    for (uint32_t i = 0; i < nelem; i++,x++) {
        if(*x > ret){
            ret = *x;
        }
    }
    return ret;
}

static float q_min(float *x, uint32_t nelem) {
    float ret = 0;
    for (uint32_t i = 0; i < nelem; i++,x++) {
        if(*x < ret){
            ret = *x;
        }
    }
    return ret;
}

static void nn_quantize_(float *x, uint8_t *y, uint32_t nelem, float* scale, uint8_t* zero_point)
{
    float max = q_max(x, nelem);
    float min = q_min(x, nelem);
    *scale = (max - min)/255.0f;
    int tmp = roundf(-min / *scale);
    if(tmp > 255){
        *zero_point = 255;
    }else{
        *zero_point = (uint8_t)tmp;
    }
    for(int i = 0; i < nelem; i++){
        float val = x[i] / (*scale) + (*zero_point);
        val = fmaxf(0.0f, fminf(val, 255.0f));
        y[i] = (uint8_t)roundf(val);
        //y[i] = clamp(round(x[i] / *scale) + *zero_point, 0, 255);  // UINT8
    }
}

qtk_maybe_unused static void vm_DynamicQuantizeLinear_(qtk_nn_vm_t *nv, uint8_t **instructions) {
    uint16_t x = qtk_littleEndian_uint16_from_bin(*instructions),
             y = qtk_littleEndian_uint16_from_bin(*instructions + 2),
             y_scale = qtk_littleEndian_uint16_from_bin(*instructions + 4),
             y_zero_point = qtk_littleEndian_uint16_from_bin(*instructions + 6);
    qtk_numeric_data_t X, Y, YS, YZ;
    uint32_t nelem;

    X.raw = qtk_nn_get_loc_from_repr(nv, x);
    Y.raw = qtk_nn_get_loc_from_repr(nv, y);
    YS.raw = qtk_nn_get_loc_from_repr(nv, y_scale);
    YZ.raw = qtk_nn_get_loc_from_repr(nv, y_zero_point);
    *instructions += 8;

    uint16_t extra_index;

    nelem = nn_vm_tensor_get_nelem_from_repr_(nv, x);

    nn_quantize_(X.f32, Y.u8, nelem, YS.f32, YZ.u8);

}

qtk_maybe_unused static int vm_DynamicQuantizeLinear_infer_shape_(qtk_nn_vm_t *nv,
                                                 uint8_t *instructions) {
    uint16_t x = qtk_littleEndian_uint16_from_bin(instructions),
             y = qtk_littleEndian_uint16_from_bin(instructions + 2);
    nn_vm_clone_shape_for_repr_(nv, x, y);
    return 0;
}

#ifdef __cplusplus
};

#endif
#endif
