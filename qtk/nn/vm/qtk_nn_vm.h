#ifndef G_50NN4X9U_4F7J_7884_IJ8B_PJTUQ0YZHSAU
#define G_50NN4X9U_4F7J_7884_IJ8B_PJTUQ0YZHSAU
#pragma once
#include "qtk/core/qtk_heap.h"
#include "qtk/core/qtk_io.h"
#include "wtk/core/wtk_strbuf.h"
#include "qtk/core/qtk_type.h"
#include "qtk/nn/qtk_nn_op.h"
#include "qtk/nn/qtk_nn_profiler.h"
#include "qtk/nn/vm/qtk_nn_vm_tensor.h"
#include "qtk/numeric/qtk_numeric_type.h"
#ifdef QBL_DEBUG
#include "wtk/core/wtk_str.h"
#endif
#ifdef __cplusplus
extern "C" {
#endif

typedef uint16_t qtk_nn_vm_repr_t;
typedef struct qtk_nn_vm qtk_nn_vm_t;

typedef struct qtk_nn_vm_transform qtk_nn_vm_transform_t;

struct qtk_nn_vm_transform {
    qtk_heap_t *heap;

    wtk_strbuf_t *initializer_shape;
    wtk_strbuf_t *initializer_loc;

    wtk_strbuf_t *dynamic_shape;
    wtk_strbuf_t *dynamic_loc;
    wtk_strbuf_t *instructions;

    qtk_nn_vm_t *vm;
};

struct qtk_nn_vm {
    qtk_nn_vm_tensor_t tensor;

    void *instructions;
    void *extra;
    uint32_t nin;
    uint32_t nout;
    uint16_t *input_idx;
    uint16_t *output_idx;
    qtk_numeric_data_t workspace;
    int workspace_cap;

    qtk_nn_vm_transform_t trans;
    qtk_nn_profiler_t profiler;

    unsigned use_transform : 1;
    unsigned use_profile : 1;

    void *pc;

#ifdef QBL_DEBUG
    wtk_string_t **initializer_symbols;
    wtk_string_t **dynamic_symbols;
#endif
};

#define qtk_nn_get_dynamic_loc(nv, idx)                                        \
    qtk_nn_vm_tensor_get_loc(&(nv)->tensor, QBL_NN_VM_TENSOR_DYNAMIC, idx)

#define qtk_nn_get_init_loc(nv, idx)                                           \
    qtk_nn_vm_tensor_get_loc(&(nv)->tensor, QBL_NN_VM_TENSOR_INIT, idx)

#define qtk_nn_get_dynamic_shape(nv, idx)                                      \
    qtk_nn_vm_tensor_get_shape(&(nv)->tensor, QBL_NN_VM_TENSOR_DYNAMIC, idx)

#define qtk_nn_get_init_shape(nv, idx)                                         \
    qtk_nn_vm_tensor_get_shape(&(nv)->tensor, QBL_NN_VM_TENSOR_INIT, idx)

#define qtk_nn_get_dynamic_rank(nv, idx)                                       \
    qtk_nn_vm_tensor_get_rank(&(nv)->tensor, QBL_NN_VM_TENSOR_DYNAMIC, idx)
#define qtk_nn_get_init_rank(nv, idx)                                          \
    qtk_nn_vm_tensor_get_rank(&(nv)->tensor, QBL_NN_VM_TENSOR_INIT, idx)

#define qtk_nn_get_dynamic_rank_ptr(nv, idx)                                   \
    qtk_nn_vm_tensor_get_rank_ptr(&(nv)->tensor, QBL_NN_VM_TENSOR_DYNAMIC, idx)
#define qtk_nn_get_init_rank_ptr(nv, idx)                                      \
    qtk_nn_vm_tensor_get_rank_ptr(&(nv)->tensor, QBL_NN_VM_TENSOR_INIT, idx)

#define qtk_nn_get_loc_from_repr(nv, x)                                        \
    qtk_nn_vm_tensor_get_loc_from_repr(&(nv)->tensor, x)

#define qtk_nn_get_loc_ptr_from_repr(nv, x)                                    \
    qtk_nn_vm_tensor_get_loc_ptr_from_repr(&(nv)->tensor, x)

#define qtk_nn_get_shape_from_repr(nv, x)                                      \
    qtk_nn_vm_tensor_get_shape_from_repr(&(nv)->tensor, x)

#define qtk_nn_get_rank_from_repr(nv, x)                                       \
    qtk_nn_vm_tensor_get_rank_from_repr(&(nv)->tensor, x)

#define qtk_nn_get_rank_ptr_from_repr(nv, x)                                   \
    qtk_nn_vm_tensor_get_rank_ptr_from_repr(&(nv)->tensor, x)

#define qtk_nn_get_shape_bytes_from_repr(nv, x)                                \
    (sizeof((nv)->tensor.shape[0]) * qtk_nn_get_rank_from_repr(nv, x))

int qtk_nn_vm_load(qtk_nn_vm_t *nv, qtk_io_reader reader, void *upval);
void qtk_nn_vm_clean(qtk_nn_vm_t *nv);
int qtk_nn_vm_run(qtk_nn_vm_t *nv);
int qtk_nn_vm_shape_propagation(qtk_nn_vm_t *nv, int **input_shape);
int qtk_nn_vm_reset(qtk_nn_vm_t *nv);

int qtk_nn_vm_prepare(qtk_nn_vm_t *nv, int **input_shape);
int qtk_nn_vm_get_input(qtk_nn_vm_t *nv, int *nin, void **in_addr);
int qtk_nn_vm_get_output(qtk_nn_vm_t *nv, int *nout, void **out_addr);
uint32_t *qtk_nn_vm_get_output_shape(qtk_nn_vm_t *nv, int out_idx, int *rank);
uint32_t *qtk_nn_vm_get_input_shape(qtk_nn_vm_t *nv, int in_idx, int *rank);
qtk_nn_vm_tensor_elem_type_t qtk_nn_vm_get_input_elem_type(qtk_nn_vm_t *nv,
                                                           int in_idx);
qtk_nn_vm_tensor_elem_type_t qtk_nn_vm_get_output_elem_type(qtk_nn_vm_t *nv,
                                                            int out_idx);
void qtk_nn_vm_enable_profile(qtk_nn_vm_t *nv);

#ifdef __cplusplus
};
#endif
#endif // G_50NN4X9U_4F7J_7884_IJ8B_PJTUQ0YZHSAU
