#ifndef G_17I28LDH_YN5D_9N88_VUOM_OSNMX19Q9HIY
#define G_17I28LDH_YN5D_9N88_VUOM_OSNMX19Q9HIY
#pragma once
#include "qtk/core/qtk_binary.h"
#include "qtk/nn/qtk_nn_utils.h"
#include "qtk/nn/vm/qtk_nn_vm.h"
#ifdef __cplusplus
extern "C" {
#endif

// TODO more sophisticate memory management
qtk_maybe_unused static int vm_TensorAlloc_(qtk_nn_vm_t *nv,
                                            uint8_t **instructions) {
    uint16_t x = qtk_littleEndian_uint16_from_bin(*instructions);
    uint16_t x_idx = QBL_NN_TENSOR_GET_INDEX(x);
    int blockid;
    qtk_numeric_data_t *dynamic;

    dynamic = nv->tensor.data[QBL_NN_VM_TENSOR_DYNAMIC];

    if (nv->tensor.dynamic_data_pos == nv->tensor.dynamic_data_cap) {
        qtk_numeric_data_t *new_data;
        nv->tensor.dynamic_data_cap = max(nv->tensor.dynamic_data_pos + 1,
                                          nv->tensor.dynamic_data_cap * 1.5);
        new_data =
            wtk_malloc(sizeof(qtk_numeric_data_t) * nv->tensor.dynamic_shape_cap);
        memcpy(new_data, dynamic,
               sizeof(qtk_numeric_data_t) * nv->tensor.dynamic_data_pos);
        nv->tensor.data[QBL_NN_VM_TENSOR_DYNAMIC] = new_data;
        wtk_free(dynamic);
        dynamic = new_data;
    }

    blockid = nv->tensor.dynamic_data_pos++;
    nv->tensor.loc[QBL_NN_VM_TENSOR_DYNAMIC][x_idx][0] = blockid;
    nv->tensor.loc[QBL_NN_VM_TENSOR_DYNAMIC][x_idx][1] = 0;

    dynamic[blockid].raw = wtk_calloc(1,nn_vm_tensor_get_tensor_sz_(nv, x));
    *instructions += 2;

    return 0;
}

qtk_maybe_unused static int vm_TensorFree_(qtk_nn_vm_t *nv,
                                           uint8_t **instructions) {
    uint16_t x = qtk_littleEndian_uint16_from_bin(*instructions);
    qtk_numeric_data_t *dynamic;
    qtk_nn_vm_tensor_loc_t *loc = qtk_nn_get_loc_ptr_from_repr(nv, x);

    dynamic = nv->tensor.data[QBL_NN_VM_TENSOR_DYNAMIC];
    wtk_free(dynamic[*loc[0]].raw);
    dynamic[*loc[0]].raw = NULL;
    *instructions += 2;
    return 0;
}

qtk_maybe_unused static int vm_MemRef_(qtk_nn_vm_t *nv,
                                       uint8_t **instructions) {
    uint16_t ref = qtk_littleEndian_uint16_from_bin(*instructions);
    uint16_t val = qtk_littleEndian_uint16_from_bin(*instructions + 2);
    qtk_nn_vm_tensor_loc_t *ref_loc = qtk_nn_get_loc_ptr_from_repr(nv, ref);
    qtk_nn_vm_tensor_loc_t *val_loc = qtk_nn_get_loc_ptr_from_repr(nv, val);
    memcpy(val_loc, ref_loc, sizeof(qtk_nn_vm_tensor_loc_t));
    *instructions += 4;
    return 0;
}

#ifdef __cplusplus
};
#endif
#endif /* G_17I28LDH_YN5D_9N88_VUOM_OSNMX19Q9HIY */
