#ifndef C5ACAC9E_4D1F_3205_DA60_D0640D9BFD8D
#define C5ACAC9E_4D1F_3205_DA60_D0640D9BFD8D

#include "qtk/core/qtk_binary.h"
#include "qtk/nn/qtk_nn_utils.h"

qtk_maybe_unused static void vm_Pad_(qtk_nn_vm_t *nv, uint8_t **instructions) {
    uint16_t x = qtk_littleEndian_uint16_from_bin(*instructions),
             p = qtk_littleEndian_uint16_from_bin(*instructions + 2),
             y = qtk_littleEndian_uint16_from_bin(*instructions + 4);
    uint32_t *shapeX;
    int rank = qtk_nn_get_rank_from_repr(nv, x);
    shapeX = qtk_nn_get_shape_from_repr(nv, x);
    qtk_numeric_data_t P, Y, X;
    X.raw = qtk_nn_get_loc_from_repr(nv, x);
    P.raw = qtk_nn_get_loc_from_repr(nv, p);
    Y.raw = qtk_nn_get_loc_from_repr(nv, y);
    uint32_t rows = 1;
    for (int i = 0; i < rank - 1; i++) {
        assert(P.i32[i] == 0);
        assert(P.i32[i + rank] == 0);
        rows *= shapeX[i];
    }
    uint32_t cols = shapeX[rank - 1];
    int padL = P.i32[rank - 1];
    int padR = P.i32[rank - 1 + rank];
    int elem_sz = nn_vm_tensor_elem_sz_[QBL_NN_TENSOR_GET_ELEM_TYPE(y)];

    char *x_ptr;
    char *y_ptr;
    uint32_t r;
    for (r = 0, x_ptr = X.chr, y_ptr = Y.chr; r < rows; r++,
        x_ptr += cols * elem_sz, y_ptr += (cols + padL + padR) * elem_sz) {
        memset(y_ptr, 0, padL * elem_sz);
        memcpy(y_ptr + padL * elem_sz, x_ptr, cols * elem_sz);
        memset(y_ptr + (padL + cols) * elem_sz, 0, padR * elem_sz);
    }
    *instructions += 6;
}

#endif /* C5ACAC9E_4D1F_3205_DA60_D0640D9BFD8D */
