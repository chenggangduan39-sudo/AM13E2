#ifndef G_S3279JPK_APZK_NK7S_3N75_4N3JL0I3IKRU
#define G_S3279JPK_APZK_NK7S_3N75_4N3JL0I3IKRU
#pragma once
#include "qtk/numeric/qtk_ndarray.h"
#include "qtk/numeric/qtk_numeric_type.h"
#ifdef __cplusplus
extern "C" {
#endif

#define QBL_NDARRAY_ITER_MAX_NOP 8

typedef struct qtk_ndarray_iter qtk_ndarray_iter_t;

struct qtk_ndarray_iter {
    qtk_numeric_data_t data[QBL_NDARRAY_ITER_MAX_NOP];
    uint32_t aligned_shape[QBL_NDARRAY_ITER_MAX_NOP][QBL_NDARRAY_MAX_RANK];
    uint32_t strides[QBL_NDARRAY_ITER_MAX_NOP][QBL_NDARRAY_MAX_RANK];
    uint32_t backstrides[QBL_NDARRAY_ITER_MAX_NOP][QBL_NDARRAY_MAX_RANK];
    uint32_t coordinates[QBL_NDARRAY_MAX_RANK];
    uint32_t broadcast_shape[QBL_NDARRAY_MAX_RANK];
    uint32_t broadcast_shape_m1[QBL_NDARRAY_MAX_RANK];
    int8_t cur_dim;
    int8_t dim_cursor;
    uint8_t elem_sz[QBL_NDARRAY_ITER_MAX_NOP];
    int ndim;
    int nop;
};

int qtk_ndarray_iter_init(qtk_ndarray_iter_t *iter, qtk_numeric_data_t *data,
                          int *rank, uint32_t **shape,
                          uint32_t *broadcast_shape, int ndim, int nop,
                          uint8_t *elem_sz);
int qtk_ndarray_iter_next(qtk_ndarray_iter_t *iter);

#ifdef __cplusplus
};
#endif
#endif /* G_S3279JPK_APZK_NK7S_3N75_4N3JL0I3IKRU */
