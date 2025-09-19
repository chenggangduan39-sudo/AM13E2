#ifndef G_DWGVFD9G_EGW7_SRRS_0WAO_BNLSEO45KB0R
#define G_DWGVFD9G_EGW7_SRRS_0WAO_BNLSEO45KB0R
#pragma once
#include "qtk/core/qtk_io.h"
#include "qtk/core/qtk_type.h"
#include "qtk/numeric/qtk_numeric_type.h"
#ifdef __cplusplus
extern "C" {
#endif

#define QBL_NN_TENSOR_TYPE_MASK 0x8000
#define QBL_NN_TENSOR_ELEM_TYPE_MASK 0x7000
#define QBL_NN_TENSOR_INDEX_MASK 0x0FFF

#define QBL_NN_TENSOR_GET_INDEX(x) ((x)&QBL_NN_TENSOR_INDEX_MASK)
#define QBL_NN_TENSOR_GET_TYPE(x) ((x) >> 15)
#define QBL_NN_TENSOR_GET_ELEM_TYPE(x)                                         \
    (((x)&QBL_NN_TENSOR_ELEM_TYPE_MASK) >> 12)

typedef enum {
    QBL_NN_VM_TENSOR_INIT,
    QBL_NN_VM_TENSOR_DYNAMIC
} qtk_nn_vm_tensor_type_t;

typedef enum {
    QBL_NN_VM_TENSOR_ELEM_EMPTY,
    QBL_NN_VM_TENSOR_ELEM_F32,
    QBL_NN_VM_TENSOR_ELEM_I64,
    QBL_NN_VM_TENSOR_ELEM_I32,
    QBL_NN_VM_TENSOR_ELEM_BOOL,
    QBL_NN_VM_TENSOR_ELEM_U8,
    QBL_NN_VM_TENSOR_ELEM_I8
} qtk_nn_vm_tensor_elem_type_t;

typedef struct qtk_nn_vm_tensor qtk_nn_vm_tensor_t;
typedef uint16_t qtk_nn_vm_tensor_shape_offset_type_t;
typedef uint32_t qtk_nn_vm_tensor_shape_type_t;
typedef uint8_t qtk_nn_vm_tensor_rank_type_t;
typedef uint32_t qtk_nn_vm_tensor_loc_t[2];

struct qtk_nn_vm_tensor {
    qtk_nn_vm_tensor_loc_t *loc[2];
    qtk_nn_vm_tensor_rank_type_t *rank[2];
    qtk_nn_vm_tensor_shape_type_t *shape[2];
    qtk_nn_vm_tensor_shape_offset_type_t *shape_offset[2];
    qtk_numeric_data_t *data[2];
    uint32_t dynamic_shape_cap;
    uint32_t dynamic_shape_pos;
    uint32_t fixed_dynamic_shape_pos;
    uint32_t dynamic_data_cap;
    uint32_t dynamic_data_pos;
    uint32_t fixed_dynamic_data_pos;
    uint32_t ninit_block;
    uint32_t ninit_tensor;
    uint32_t ndyn_tensor;
    uint32_t ncomptime_tensor;
};

#define qtk_nn_vm_tensor_get_loc_ptr(nt, tt, idx)                              \
    cast(qtk_nn_vm_tensor_loc_t *, (nt)->loc[tt] + idx)

#define qtk_nn_vm_tensor_get_loc(nt, tt, idx)                                  \
    cast(qtk_numeric_data_t *,                                                 \
         (nt)->data[tt][(nt)->loc[tt][idx][0]].chr + (nt)->loc[tt][idx][1])
#define qtk_nn_vm_tensor_get_shape(nt, tt, idx)                                \
    ((nt)->shape[tt] + (nt)->shape_offset[tt][idx])
#define qtk_nn_vm_tensor_get_rank(nt, tt, idx) ((nt)->rank[tt][idx])
#define qtk_nn_vm_tensor_get_rank_ptr(nt, tt, idx) ((nt)->rank[tt] + idx)

#define qtk_nn_vm_tensor_get_loc_ptr_from_repr(nt, repr)                       \
    qtk_nn_vm_tensor_get_loc_ptr(nt, QBL_NN_TENSOR_GET_TYPE(repr),             \
                                 QBL_NN_TENSOR_GET_INDEX(repr))
#define qtk_nn_vm_tensor_get_loc_from_repr(nt, repr)                           \
    qtk_nn_vm_tensor_get_loc(nt, QBL_NN_TENSOR_GET_TYPE(repr),                 \
                             QBL_NN_TENSOR_GET_INDEX(repr))
#define qtk_nn_vm_tensor_get_shape_from_repr(nt, repr)                         \
    qtk_nn_vm_tensor_get_shape(nt, QBL_NN_TENSOR_GET_TYPE(repr),               \
                               QBL_NN_TENSOR_GET_INDEX(repr))
#define qtk_nn_vm_tensor_get_rank_from_repr(nt, repr)                          \
    qtk_nn_vm_tensor_get_rank(nt, QBL_NN_TENSOR_GET_TYPE(repr),                \
                              QBL_NN_TENSOR_GET_INDEX(repr))
#define qtk_nn_vm_tensor_get_rank_ptr_from_repr(nt, repr)                      \
    qtk_nn_vm_tensor_get_rank_ptr(nt, QBL_NN_TENSOR_GET_TYPE(repr),            \
                                  QBL_NN_TENSOR_GET_INDEX(repr))

void qtk_nn_vm_tensor_clean(qtk_nn_vm_tensor_t *nt);
void qtk_nn_vm_tensor_reset(qtk_nn_vm_tensor_t *nt);

int qtk_nn_vm_tensor_load_dynamic(qtk_nn_vm_tensor_t *nt, qtk_io_reader reader,
                                  void *upval);
int qtk_nn_vm_tensor_load_initializer(qtk_nn_vm_tensor_t *nt,
                                      qtk_io_reader reader, void *upval);

#ifdef __cplusplus
};
#endif
#endif /* G_DWGVFD9G_EGW7_SRRS_0WAO_BNLSEO45KB0R */
