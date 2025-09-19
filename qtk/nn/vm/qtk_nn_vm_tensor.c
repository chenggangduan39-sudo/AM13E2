#include "qtk/nn/vm/qtk_nn_vm_tensor.h"
#include "wtk/core/wtk_alloc.h"

#define qtk_new_vec(t, n) wtk_malloc(sizeof(t) * (n))

void qtk_nn_vm_tensor_clean(qtk_nn_vm_tensor_t *nt) {
    wtk_free(nt->loc[QBL_NN_VM_TENSOR_INIT]);
    wtk_free(nt->loc[QBL_NN_VM_TENSOR_DYNAMIC]);
    wtk_free(nt->rank[QBL_NN_VM_TENSOR_INIT]);
    wtk_free(nt->rank[QBL_NN_VM_TENSOR_DYNAMIC]);
    wtk_free(nt->shape[QBL_NN_VM_TENSOR_INIT]);
    wtk_free(nt->shape[QBL_NN_VM_TENSOR_DYNAMIC]);
    wtk_free(nt->shape_offset[QBL_NN_VM_TENSOR_INIT]);
    wtk_free(nt->shape_offset[QBL_NN_VM_TENSOR_DYNAMIC]);
    for (int i = 0; i < nt->ninit_block; i++) {
        wtk_free(nt->data[QBL_NN_VM_TENSOR_INIT][i].raw);
    }
    for (int i = 0; i < nt->dynamic_data_pos; i++) {
        if (nt->data[QBL_NN_VM_TENSOR_DYNAMIC][i].raw) {
            wtk_free(nt->data[QBL_NN_VM_TENSOR_DYNAMIC][i].raw);
            nt->data[QBL_NN_VM_TENSOR_DYNAMIC][i].raw = NULL;
        }
    }
    wtk_free(nt->data[QBL_NN_VM_TENSOR_INIT]);
    wtk_free(nt->data[QBL_NN_VM_TENSOR_DYNAMIC]);
}

void qtk_nn_vm_tensor_reset(qtk_nn_vm_tensor_t *nt) {
    for (int i = nt->fixed_dynamic_data_pos; i < nt->dynamic_data_pos; i++) {
        if (nt->data[QBL_NN_VM_TENSOR_DYNAMIC][i].raw) {
            wtk_free(nt->data[QBL_NN_VM_TENSOR_DYNAMIC][i].raw);
            nt->data[QBL_NN_VM_TENSOR_DYNAMIC][i].raw = NULL;
        }
    }
    nt->dynamic_data_pos = nt->fixed_dynamic_data_pos;
    nt->dynamic_shape_pos = nt->fixed_dynamic_shape_pos;
}

int qtk_nn_vm_tensor_load_initializer(qtk_nn_vm_tensor_t *nt,
                                      qtk_io_reader reader, void *upval) {
    uint32_t nblock, sz, ntensor;
    uint16_t shape_cap;
    if (qtk_io_readn(upval, reader, cast(char *, &nblock), sizeof(nblock))) {
        goto err;
    }
    nt->ninit_block = nblock;
    nt->data[QBL_NN_VM_TENSOR_INIT] = qtk_new_vec(qtk_numeric_data_t, nblock);
    for (int i = 0; i < nblock; i++) {
        if (qtk_io_readn(upval, reader, cast(char *, &sz), sizeof(sz))) {
            goto err;
        }
        nt->data[QBL_NN_VM_TENSOR_INIT][i].raw = wtk_malloc(sz);
        if (qtk_io_readn(upval, reader, nt->data[QBL_NN_VM_TENSOR_INIT][i].chr,
                         sz)) {
            goto err;
        }
    }
    if (qtk_io_readn(upval, reader, cast(char *, &ntensor), sizeof(ntensor))) {
        goto err;
    }
    nt->ninit_tensor = ntensor;

    nt->shape_offset[QBL_NN_VM_TENSOR_INIT] =
        qtk_new_vec(qtk_nn_vm_tensor_shape_offset_type_t, ntensor);
    nt->rank[QBL_NN_VM_TENSOR_INIT] =
        qtk_new_vec(qtk_nn_vm_tensor_rank_type_t, ntensor);
    if (qtk_io_readn(upval, reader, cast(char *, &shape_cap),
                     sizeof(shape_cap))) {
        goto err;
    }
    nt->shape[QBL_NN_VM_TENSOR_INIT] =
        qtk_new_vec(qtk_nn_vm_tensor_shape_type_t, shape_cap);
    nt->loc[QBL_NN_VM_TENSOR_INIT] =
        qtk_new_vec(qtk_nn_vm_tensor_loc_t, ntensor);

    if (qtk_io_readn(upval, reader,
                     cast(char *, nt->rank[QBL_NN_VM_TENSOR_INIT]),
                     sizeof(qtk_nn_vm_tensor_rank_type_t) * ntensor) ||
        qtk_io_readn(upval, reader,
                     cast(char *, nt->shape[QBL_NN_VM_TENSOR_INIT]),
                     sizeof(qtk_nn_vm_tensor_shape_type_t) * shape_cap) ||
        qtk_io_readn(upval, reader,
                     cast(char *, nt->shape_offset[QBL_NN_VM_TENSOR_INIT]),
                     sizeof(qtk_nn_vm_tensor_shape_offset_type_t) * ntensor) ||
        qtk_io_readn(upval, reader,
                     cast(char *, nt->loc[QBL_NN_VM_TENSOR_INIT]),
                     ntensor * sizeof(qtk_nn_vm_tensor_loc_t))) {
        goto err;
    }
    // for(int i = 0 ;i<ntensor;i++){
    //     if(nt->loc[i]){
    //         printf("loc%d :%d %d\n",i,nt->loc[0][i][0],nt->loc[0][i][1]);
    //         printf("loc%d :%d %d\n",i,nt->loc[0][i][0],nt->loc[0][i][1]);
    //     }
    // }

    // exit(0);
    return 0;
err:
    wtk_debug("Failed\n");
    return -1;
}

int qtk_nn_vm_tensor_load_dynamic(qtk_nn_vm_tensor_t *nt, qtk_io_reader reader,
                                  void *upval) {

    uint32_t nblock, sz, ntensor, ncomptime_tensor, nfixshape_tensor;
    qtk_numeric_data_t *dynamic;
    uint16_t shape_pos;

    if (qtk_io_readn(upval, reader, cast(char *, &ntensor), sizeof(ntensor))) {
        goto err;
    }
    nt->ndyn_tensor = ntensor;

    if (qtk_io_readn(upval, reader, cast(char *, &nblock), sizeof(nblock))) {
        goto err;
    }
    nt->dynamic_data_cap = nblock;
    nt->dynamic_data_pos = nblock;
    nt->fixed_dynamic_data_pos = nblock;
    nt->data[QBL_NN_VM_TENSOR_DYNAMIC] = dynamic =
        qtk_new_vec(qtk_numeric_data_t, nblock);
    for (int i = 0; i < nblock; i++) {
        if (qtk_io_readn(upval, reader, cast(char *, &sz), sizeof(sz))) {
            goto err;
        }
        dynamic[i].raw = wtk_calloc(sz, 1);
    }
    if (qtk_io_readn(upval, reader, cast(char *, &nfixshape_tensor),
                     sizeof(nfixshape_tensor))) {
        goto err;
    }
    if (qtk_io_readn(upval, reader, cast(char *, &ncomptime_tensor),
                     sizeof(ncomptime_tensor))) {
        goto err;
    }

    nt->shape_offset[QBL_NN_VM_TENSOR_DYNAMIC] =
        qtk_new_vec(qtk_nn_vm_tensor_shape_offset_type_t, ntensor);
    nt->rank[QBL_NN_VM_TENSOR_DYNAMIC] =
        qtk_new_vec(qtk_nn_vm_tensor_rank_type_t, ntensor);
    if (qtk_io_readn(upval, reader, cast(char *, &shape_pos),
                     sizeof(shape_pos))) {
        goto err;
    }

    nt->loc[QBL_NN_VM_TENSOR_DYNAMIC] =
        qtk_new_vec(qtk_nn_vm_tensor_loc_t, ntensor);
    nt->dynamic_shape_cap = shape_pos;
    nt->shape[QBL_NN_VM_TENSOR_DYNAMIC] =
        qtk_new_vec(qtk_nn_vm_tensor_shape_type_t, nt->dynamic_shape_cap);
    nt->dynamic_shape_pos = shape_pos;
    nt->fixed_dynamic_shape_pos = shape_pos;
    nt->ncomptime_tensor = ncomptime_tensor;

    if (qtk_io_readn(upval, reader,
                     cast(char *, nt->rank[QBL_NN_VM_TENSOR_DYNAMIC]),
                     sizeof(qtk_nn_vm_tensor_rank_type_t) * nfixshape_tensor) ||
        qtk_io_readn(upval, reader,
                     cast(char *, nt->shape[QBL_NN_VM_TENSOR_DYNAMIC]),
                     sizeof(qtk_nn_vm_tensor_shape_type_t) * shape_pos) ||
        qtk_io_readn(upval, reader,
                     cast(char *, nt->shape_offset[QBL_NN_VM_TENSOR_DYNAMIC]),
                     sizeof(qtk_nn_vm_tensor_shape_offset_type_t) *
                         nfixshape_tensor)) {
        goto err;
    }

    if (ncomptime_tensor > 0) {
        if (qtk_io_readn(upval, reader,
                         cast(char *, nt->loc[QBL_NN_VM_TENSOR_DYNAMIC]),
                         ncomptime_tensor * sizeof(qtk_nn_vm_tensor_loc_t))) {
            goto err;
        }
    }

    return 0;
err:
    wtk_debug("Failed\n");
    return -1;
}
