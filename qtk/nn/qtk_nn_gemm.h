#ifndef B9E03382_3B03_95B7_B484_53362A402F22
#define B9E03382_3B03_95B7_B484_53362A402F22

#include "qtk/core/qtk_binary.h"
#include "qtk/linalg/qtk_gemm.h"
#include "qtk/nn/qtk_nn_utils.h"

qtk_maybe_unused static int vm_Gemm_infer_shape_(qtk_nn_vm_t *nv,
                                                 uint8_t *instructions) {
    // TODO
    qtk_assert(0);
    return 0;
}

qtk_maybe_unused static void vm_Gemm_(qtk_nn_vm_t *nv, uint8_t **instructions) {
    uint16_t a = qtk_littleEndian_uint16_from_bin(*instructions),
             b = qtk_littleEndian_uint16_from_bin(*instructions + 2),
             c = qtk_littleEndian_uint16_from_bin(*instructions + 4),
             y = qtk_littleEndian_uint16_from_bin(*instructions + 6),
             extra_index = qtk_littleEndian_uint16_from_bin(*instructions + 8);
    qtk_numeric_data_t A, B, C, Y;
    int m, n, k;
    int lda, ldb, ldc;
    uint8_t transA, transB;
    float alpha, beta;
    *instructions += 10;
    transA = ((uint8_t *)nv->extra)[extra_index];
    transB = ((uint8_t *)nv->extra)[extra_index + 1];
    memcpy(&alpha, (char *)nv->extra + 2 + extra_index, sizeof(alpha));
    memcpy(&beta, (char *)nv->extra + 2 + 4 + extra_index, sizeof(alpha));

    uint32_t *shapeA = qtk_nn_get_shape_from_repr(nv, a);
    uint32_t *shapeB = qtk_nn_get_shape_from_repr(nv, b);
    uint32_t *shapeY = qtk_nn_get_shape_from_repr(nv, y);

    A.raw = qtk_nn_vm_tensor_get_loc_from_repr(&nv->tensor, a);
    B.raw = qtk_nn_vm_tensor_get_loc_from_repr(&nv->tensor, b);
    C.raw = qtk_nn_vm_tensor_get_loc_from_repr(&nv->tensor, c);
    Y.raw = qtk_nn_vm_tensor_get_loc_from_repr(&nv->tensor, y);

    qtk_nn_vm_tensor_elem_type_t elem_type = QBL_NN_TENSOR_GET_ELEM_TYPE(y);
    qtk_assert(elem_type == QBL_NN_VM_TENSOR_ELEM_F32);
    uint32_t nelem = nn_vm_tensor_get_nelem_from_repr_(nv, y);
    qtk_assert(nelem == nn_vm_tensor_get_nelem_from_repr_(nv, c));
    memcpy(Y.raw, C.raw, nelem * sizeof(float));

    lda = shapeA[1];
    ldb = shapeB[1];
    ldc = shapeY[1];

    m = shapeY[0];
    n = shapeY[1];
    k = transA ? shapeA[0] : shapeA[1];

    qtk_sgemm(transA, transB, m, n, k, alpha, A.f32, lda, B.f32, ldb, beta,
              Y.f32, ldc);
}

#endif /* B9E03382_3B03_95B7_B484_53362A402F22 */
