#ifndef FAF1C2A4_DE25_0B0F_1B2A_847BFB6A79B6
#define FAF1C2A4_DE25_0B0F_1B2A_847BFB6A79B6

#include "qtk/core/qtk_binary.h"
#include "qtk/nn/qtk_nn_utils.h"

qtk_maybe_unused static int vm_ScatterND_infer_shape_(qtk_nn_vm_t *nv,
                                                      uint8_t *instructions) {
    // TODO
    qtk_assert(0);
    return 0;
}

qtk_maybe_unused static void vm_ScatterND_(qtk_nn_vm_t *nv,
                                           uint8_t **instructions) {
    uint16_t data = qtk_littleEndian_uint16_from_bin(*instructions),
             indices = qtk_littleEndian_uint16_from_bin(*instructions + 2),
             updates = qtk_littleEndian_uint16_from_bin(*instructions + 4),
             y = qtk_littleEndian_uint16_from_bin(*instructions + 6);
    *instructions += 8;
    qtk_numeric_data_t D, I, U, Y;

    D.raw = qtk_nn_vm_tensor_get_loc_from_repr(&nv->tensor, data);
    I.raw = qtk_nn_vm_tensor_get_loc_from_repr(&nv->tensor, indices);
    U.raw = qtk_nn_vm_tensor_get_loc_from_repr(&nv->tensor, updates);
    Y.raw = qtk_nn_vm_tensor_get_loc_from_repr(&nv->tensor, y);

    uint32_t *shapeD = qtk_nn_get_shape_from_repr(nv, data);
    uint32_t *shapeU = qtk_nn_get_shape_from_repr(nv, updates);
    int rankD = qtk_nn_get_rank_from_repr(nv, data);
    int rankI = qtk_nn_get_rank_from_repr(nv, indices);
    int rankU = qtk_nn_get_rank_from_repr(nv, updates);
    uint32_t *shapeI = qtk_nn_get_shape_from_repr(nv, indices);

    int k = shapeI[rankI - 1];
    qtk_assert(k == rankD && k == rankU);
    uint32_t m = 1;
    for (int i = 0; i < rankI - 1; i++) {
        m *= shapeI[i];
    }

    qtk_assert(nn_vm_tensor_get_elem_sz_(nv, y) == 4);

    uint32_t ldy[32];
    uint32_t ldu[32];

    ldy[k - 1] = 1;
    ldu[k - 1] = 1;
    for (int i = k - 2; i >= 0; i--) {
        ldy[i] = ldy[i + 1] * shapeD[i + 1];
        ldu[i] = ldu[i + 1] * shapeU[i + 1];
    }

    uint32_t nelem = nn_vm_tensor_get_nelem_from_repr_(nv, y);
    if (Y.raw != D.raw) {
        memcpy(Y.raw, D.raw, 4 * nelem);
    }

    for (uint32_t i = 0; i < m; i++) {
        int32_t *idx = I.i32 + i * k;
        uint32_t offset_y = 0, offset_u = 0;
        for (int j = 0; j < k; j++) {
            offset_y += idx[j] * ldy[j];
            offset_u += idx[j] * ldu[j];
        }
        Y.i32[offset_y] = U.i32[offset_u];
    }
}

#endif /* FAF1C2A4_DE25_0B0F_1B2A_847BFB6A79B6 */
