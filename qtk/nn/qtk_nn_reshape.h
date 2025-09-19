#ifndef G_SQNSD2E3_T8TB_BBSP_OGNF_ZOGFEMGGWDAM
#define G_SQNSD2E3_T8TB_BBSP_OGNF_ZOGFEMGGWDAM
#pragma once
#include "qtk/core/qtk_binary.h"
#include "qtk/nn/qtk_nn_utils.h"
#include "qtk/nn/vm/qtk_nn_vm.h"
#ifdef __cplusplus
extern "C" {
#endif

qtk_maybe_unused static void vm_Reshape_(qtk_nn_vm_t *nv,
                                         uint8_t **instructions) {
    uint16_t x = qtk_littleEndian_uint16_from_bin(*instructions),
             y = qtk_littleEndian_uint16_from_bin(*instructions + 4);
    qtk_numeric_data_t X, Y;

    X.raw = qtk_nn_get_loc_from_repr(nv, x);
    Y.raw = qtk_nn_get_loc_from_repr(nv, y);

    memcpy(Y.raw, X.raw, nn_vm_tensor_get_tensor_sz_(nv, x));

    *instructions += 6;
}

qtk_maybe_unused static int vm_Reshape_infer_shape_(qtk_nn_vm_t *nv,
                                                    uint8_t *instructions) {
    uint16_t x = qtk_littleEndian_uint16_from_bin(instructions),
             shape = qtk_littleEndian_uint16_from_bin(instructions + 2),
             y = qtk_littleEndian_uint16_from_bin(instructions + 4);
    uint32_t out_shape[32], *shape_shape, *in_shape;
    uint32_t nelem = 1, nelem_result_determined = 1;
    int post = 0, post_idx = 0;
    int idx;
    int rankX, rankY;
    qtk_numeric_data_t SHAPE;

    SHAPE.raw = qtk_nn_get_loc_from_repr(nv, shape);
    shape_shape = qtk_nn_get_shape_from_repr(nv, shape);
    in_shape = qtk_nn_get_shape_from_repr(nv, x);

    rankX = qtk_nn_get_rank_from_repr(nv, x);

    for (idx = 0; idx < shape_shape[0]; idx++) {
        out_shape[idx] = SHAPE.i64[idx];
        if (SHAPE.i64[idx] < 0) {
            if (post == 1) {
                goto err;
            }
            post_idx = idx;
            post = 1;
        } else {
            nelem_result_determined *= out_shape[idx];
        }
    }

    if (post) {
        for (int i = 0; i < rankX; i++) {
            nelem *= in_shape[i];
        }
        out_shape[post_idx] = nelem / nelem_result_determined;
    }

    rankY = shape_shape[0];

    nn_vm_set_dynamic_shape_for_repr_(nv, out_shape, rankY, y);
    return 0;
err:
    return -1;
}

#ifdef __cplusplus
};
#endif
#endif /* G_SQNSD2E3_T8TB_BBSP_OGNF_ZOGFEMGGWDAM */
