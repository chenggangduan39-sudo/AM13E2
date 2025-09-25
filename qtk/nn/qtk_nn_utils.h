#ifndef QBL_NN_QBL_NN_UTILS_H
#define QBL_NN_QBL_NN_UTILS_H
#include "qtk/core/qtk_type.h"
#pragma once
#include "qtk/nn/vm/qtk_nn_vm.h"
#ifdef __cplusplus
extern "C" {
#endif

#define qtk_nn_pad_dim(in, padL, padR, ksize, stride)                          \
    (((in) + (padL) + (padR) - (ksize)) / (stride) + 1)

// please refer to qtk_nn_vm_tensor_elem_type_t
static uint8_t nn_vm_tensor_elem_sz_[] = {0, 4, 8, 4, 1, 1, 1};

qtk_maybe_unused static int nn_vm_tensor_get_elem_sz_(qtk_nn_vm_t *nv,
                                                      uint16_t repr) {
    return nn_vm_tensor_elem_sz_[QBL_NN_TENSOR_GET_ELEM_TYPE(repr)];
}

qtk_maybe_unused static uint32_t
nn_vm_tensor_get_nelem_from_repr_(qtk_nn_vm_t *nv, qtk_nn_vm_repr_t repr) {
    uint32_t nelem = 1;
    uint32_t *shape = qtk_nn_get_shape_from_repr(nv, repr);
    int rank = qtk_nn_get_rank_from_repr(nv, repr);
    for (int i = 0; i < rank; i++) {
        nelem *= shape[i];
    }
    return nelem;
}

qtk_maybe_unused static uint32_t
nn_vm_tensor_get_tensor_sz_(qtk_nn_vm_t *nv, qtk_nn_vm_repr_t repr) {
    int elem_sz = nn_vm_tensor_elem_sz_[QBL_NN_TENSOR_GET_ELEM_TYPE(repr)];
    return elem_sz * nn_vm_tensor_get_nelem_from_repr_(nv, repr);
}

qtk_maybe_unused static uint32_t nn_vm_multidirectional_broadcasting_real_idx_(
    uint32_t *a, uint32_t *std, uint32_t *idx, int rankA, int rankStd) {
    uint32_t alignedA[32] = {0};
    uint32_t idxA[32] = {0};
    uint32_t ldaA[32] = {0}, lda = 1;
    uint32_t result = 0;

    if (rankStd > rankA) {
        for (int i = 0; i < rankStd - rankA; i++) {
            alignedA[i] = 1;
        }
        for (int i = 0; i < rankA; i++) {
            alignedA[rankStd - rankA + i] = a[i];
        }
    } else {
        memcpy(alignedA, a, sizeof(a[0]) * rankA);
    }

    for (int i = 0; i < rankStd; i++) {
        idxA[i] = alignedA[i] == 1 ? 0 : idx[i];
        ldaA[rankStd - i - 1] = lda;
        lda *= alignedA[rankStd - i - 1];
    }

    for (int i = 0; i < rankStd; i++) {
        result += idxA[i] * ldaA[i];
    }

    return result;
}

qtk_maybe_unused static void
nn_vm_set_dynamic_shape_for_repr_(qtk_nn_vm_t *nv, uint32_t *shape, int rank,
                                  qtk_nn_vm_repr_t repr) {
    int tensor_idx = QBL_NN_TENSOR_GET_INDEX(repr);
    *(qtk_nn_get_dynamic_rank_ptr(nv, tensor_idx)) = rank;
    if (nv->tensor.dynamic_shape_pos + rank > nv->tensor.dynamic_shape_cap) {
        nv->tensor.dynamic_shape_cap = max(nv->tensor.dynamic_shape_pos + rank,
                                           nv->tensor.dynamic_shape_cap * 1.5);
        void *dynamic_shape = qtk_new_vec(qtk_nn_vm_tensor_shape_type_t,
                                          nv->tensor.dynamic_shape_cap);
        memcpy(dynamic_shape, nv->tensor.shape[QBL_NN_VM_TENSOR_DYNAMIC],
               sizeof(qtk_nn_vm_tensor_shape_type_t) *
                   nv->tensor.dynamic_shape_pos);
        wtk_free(nv->tensor.shape[QBL_NN_VM_TENSOR_DYNAMIC]);
        nv->tensor.shape[QBL_NN_VM_TENSOR_DYNAMIC] = dynamic_shape;
    }
    nv->tensor.shape_offset[QBL_NN_VM_TENSOR_DYNAMIC][tensor_idx] =
        nv->tensor.dynamic_shape_pos;
    if (rank > 0) {
        memcpy(nv->tensor.shape[QBL_NN_VM_TENSOR_DYNAMIC] +
                   nv->tensor.dynamic_shape_pos,
               shape, sizeof(shape[0]) * rank);
    }
    nv->tensor.dynamic_shape_pos += rank;
}

qtk_maybe_unused static void nn_vm_clone_shape_for_repr_(qtk_nn_vm_t *nv,
                                                         qtk_nn_vm_repr_t from,
                                                         qtk_nn_vm_repr_t to) {
    int rank = qtk_nn_get_rank_from_repr(nv, from);
    uint32_t *shape_from = qtk_nn_get_shape_from_repr(nv, from);
    nn_vm_set_dynamic_shape_for_repr_(nv, shape_from, rank, to);
}

qtk_maybe_unused static int ensure_workspace_(qtk_nn_vm_t *vm, int sz) {
    if (vm->workspace_cap >= sz) {
        return 0;
    }
    wtk_free(vm->workspace.raw);
    vm->workspace_cap = sz;
    vm->workspace.raw = wtk_malloc(sz);
    return 0;
}

#ifdef __cplusplus
};
#endif
#endif
