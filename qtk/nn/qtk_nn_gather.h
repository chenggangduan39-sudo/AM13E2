#ifndef G_GHL99UFF_SMQT_DIGZ_VBPQ_39X9QF56XHSN
#define G_GHL99UFF_SMQT_DIGZ_VBPQ_39X9QF56XHSN
#pragma once
#include "qtk/core/qtk_binary.h"
#include "qtk/nn/qtk_nn_utils.h"
#include "qtk/nn/vm/qtk_nn_vm.h"
#ifdef __cplusplus
extern "C" {
#endif

qtk_maybe_unused static int vm_Gather_infer_shape_(qtk_nn_vm_t *nv,
                                                   uint8_t *instructions) {
    uint32_t *in_shape, *indices_shape;
    uint16_t x = qtk_littleEndian_uint16_from_bin(instructions),
             indices = qtk_littleEndian_uint16_from_bin(instructions + 2),
             y = qtk_littleEndian_uint16_from_bin(instructions + 4);
    uint16_t extra_index = qtk_littleEndian_uint16_from_bin(instructions + 6);
    int8_t axis;
    int rankX, rank_indices;
    int rankY;
    uint32_t out_shape[16];

    in_shape = qtk_nn_get_dynamic_shape(nv, QBL_NN_TENSOR_GET_INDEX(x));
    rankX = qtk_nn_get_rank_from_repr(nv, x);
    rank_indices = qtk_nn_get_rank_from_repr(nv, indices);
    indices_shape = qtk_nn_get_shape_from_repr(nv, indices);
    axis = cast(int8_t *, cast(char *, nv->extra) + extra_index)[0];

    rankY = 0;
    for (; rankY < axis; rankY++) {
        out_shape[rankY] = in_shape[rankY];
    }
    for (int i = 0; i < rank_indices; i++) {
        out_shape[rankY++] = indices_shape[i];
    }
    for (int i = axis + 1; i < rankX; i++) {
        out_shape[rankY++] = in_shape[i];
    }
    nn_vm_set_dynamic_shape_for_repr_(nv, out_shape, rankY, y);
    return 0;
}

qtk_maybe_unused static void vm_Gather_(qtk_nn_vm_t *nv,
                                        uint8_t **instructions) {
    uint32_t *in_shape;
    uint16_t x = qtk_littleEndian_uint16_from_bin(*instructions),
             indices = qtk_littleEndian_uint16_from_bin(*instructions + 2),
             y = qtk_littleEndian_uint16_from_bin(*instructions + 4);
    uint16_t extra_index = qtk_littleEndian_uint16_from_bin(*instructions + 6);
    qtk_numeric_data_t X, Y, INDICES;
    int8_t axis;
    int elem_sz;
    int rankX;
    uint32_t st[3] = {1, 1, 1};
    void *y_raw;
    qtk_nn_vm_tensor_elem_type_t indices_type;

    in_shape = qtk_nn_get_shape_from_repr(nv, x);
    rankX = qtk_nn_get_rank_from_repr(nv, x);
    axis = cast(int8_t *, cast(char *, nv->extra) + extra_index)[0];
    uint32_t nindices;

    for (int i = 0; i < axis; i++) {
        st[0] *= in_shape[i];
    }
    st[1] = in_shape[axis];
    for (int i = axis + 1; i < rankX; i++) {
        st[2] *= in_shape[i];
    }

    INDICES.raw = qtk_nn_get_loc_from_repr(nv, indices);
    Y.raw = qtk_nn_get_loc_from_repr(nv, y);
    X.raw = qtk_nn_get_loc_from_repr(nv, x);
    elem_sz = nn_vm_tensor_get_elem_sz_(nv, y);
    y_raw = Y.raw;

    indices_type = QBL_NN_TENSOR_GET_ELEM_TYPE(indices);
    nindices = nn_vm_tensor_get_nelem_from_repr_(nv, indices);

    if (indices_type == QBL_NN_VM_TENSOR_ELEM_I32) {
        for (int i = 0; i < st[0]; i++) {
            for (int j = 0; j < nindices; j++) {
                memcpy(y_raw,
                       X.chr + ((i * st[1] + INDICES.i32[j]) * st[2]) * elem_sz,
                       elem_sz * st[2]);
                y_raw = cast(char *, y_raw) + elem_sz * st[2];
            }
        }
    } else {
        qtk_assert(indices_type == QBL_NN_VM_TENSOR_ELEM_I64);
        for (int i = 0; i < st[0]; i++) {
            for (int j = 0; j < nindices; j++) {
                memcpy(y_raw,
                       X.chr + ((i * st[1] + INDICES.i64[j]) * st[2]) * elem_sz,
                       elem_sz * st[2]);
                y_raw = cast(char *, y_raw) + elem_sz * st[2];
            }
        }
    }

    *instructions += 8;
}

#ifdef __cplusplus
};
#endif
#endif /* G_GHL99UFF_SMQT_DIGZ_VBPQ_39X9QF56XHSN */
