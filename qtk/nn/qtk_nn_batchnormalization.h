#ifndef F1C01EDA_C9D9_6466_A5FC_98F6328367DC
#define F1C01EDA_C9D9_6466_A5FC_98F6328367DC

#include "qtk/core/qtk_binary.h"
#include "qtk/nn/vm/qtk_nn_vm.h"

qtk_maybe_unused static void vm_BatchNormalization_(qtk_nn_vm_t *nv,
                                                    uint8_t **instructions) {
    uint16_t x = qtk_littleEndian_uint16_from_bin(*instructions),
             scale = qtk_littleEndian_uint16_from_bin(*instructions + 2),
             b = qtk_littleEndian_uint16_from_bin(*instructions + 4),
             mean = qtk_littleEndian_uint16_from_bin(*instructions + 6),
             var = qtk_littleEndian_uint16_from_bin(*instructions + 8),
             y = qtk_littleEndian_uint16_from_bin(*instructions + 10);
    uint16_t extra_index = qtk_littleEndian_uint16_from_bin(*instructions + 12);
    float eps;
    memcpy(&eps, cast(uint8_t *, nv->extra) + extra_index, sizeof(float));
    *instructions += 14;
    qtk_numeric_data_t X, Scale, B, Mean, Var, Y;

    X.raw = qtk_nn_get_loc_from_repr(nv, x);
    Scale.raw = qtk_nn_get_loc_from_repr(nv, scale);
    B.raw = qtk_nn_get_loc_from_repr(nv, b);
    Mean.raw = qtk_nn_get_loc_from_repr(nv, mean);
    Var.raw = qtk_nn_get_loc_from_repr(nv, var);
    Y.raw = qtk_nn_get_loc_from_repr(nv, y);

    qtk_assert(QBL_NN_TENSOR_GET_ELEM_TYPE(x) == QBL_NN_VM_TENSOR_ELEM_F32);

    uint32_t *shape = qtk_nn_get_shape_from_repr(nv, x);
    qtk_assert(qtk_nn_get_rank_from_repr(nv, x) == 4);

    float *x_ptr = X.f32;
    float *y_ptr = Y.f32;

    for (int b = 0; b < shape[0]; b++) {
        for (int c = 0; c < shape[1]; c++) {
            for (int h = 0; h < shape[2]; h++) {
                for (int w = 0; w < shape[3]; w++) {
                    *y_ptr++ = (*x_ptr++ - Mean.f32[c]) *
                                   (1.0 / sqrtf(Var.f32[c] + eps)) *
                                   Scale.f32[c] +
                               B.f32[c];
                }
            }
        }
    }
}

#endif /* F1C01EDA_C9D9_6466_A5FC_98F6328367DC */
