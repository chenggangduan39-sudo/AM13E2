#include "qtk/numeric/qtk_ndarray_iter.h"

static void ndarray_set_strides_(qtk_ndarray_iter_t *iter) {
    for (int i = 0; i < iter->nop; i++) {
        uint32_t stride = 1;
        for (int j = iter->ndim - 1; j >= 0; j--) {
            iter->strides[i][j] = stride;
            stride *= iter->aligned_shape[i][j];
        }
        for (int j = 0; j < iter->ndim; j++) {
            iter->backstrides[i][j] =
                iter->strides[i][j] * iter->aligned_shape[i][j] - 1;
        }

        for (int j = 0; j < iter->ndim; j++) {
            if (iter->aligned_shape[i][j] == 1) {
                iter->strides[i][j] = 0;
            }
        }
    }
}

static void ndarray_set_aligned_shape_(qtk_ndarray_iter_t *iter,
                                       uint32_t **shape, int *rank) {
    for (int i = 0; i < iter->nop; i++) {
        for (int odim = 0; odim < iter->ndim - rank[i]; odim++) {
            iter->aligned_shape[i][odim] = 1;
        }
        for (int odim = iter->ndim - rank[i]; odim < iter->ndim; odim++) {
            iter->aligned_shape[i][odim] =
                shape[i][odim - iter->ndim + rank[i]];
        }
    }
}

int qtk_ndarray_iter_init(qtk_ndarray_iter_t *iter, qtk_numeric_data_t *data,
                          int *rank, uint32_t **shape,
                          uint32_t *broadcast_shape, int ndim, int nop,
                          uint8_t *elem_sz) {
    memcpy(iter->broadcast_shape, broadcast_shape, sizeof(uint32_t) * ndim);
    memset(iter->coordinates, 0, sizeof(iter->coordinates));
    memcpy(iter->data, data, sizeof(qtk_numeric_data_t) * nop);
    iter->nop = nop;
    iter->ndim = ndim;
    memcpy(iter->elem_sz, elem_sz, sizeof(iter->elem_sz[0]) * nop);

    for (int i = 0; i < ndim; i++) {
        iter->broadcast_shape_m1[i] = iter->broadcast_shape[i] - 1;
    }

    ndarray_set_aligned_shape_(iter, shape, rank);
    ndarray_set_strides_(iter);
    iter->cur_dim = iter->ndim - 1;
    iter->dim_cursor = iter->ndim - 1;

    return 0;
}

int qtk_ndarray_iter_next(qtk_ndarray_iter_t *iter) {
    if (++iter->coordinates[iter->cur_dim] <
        iter->broadcast_shape[iter->cur_dim]) {
        for (int i = 0; i < iter->nop; i++) {
            iter->data[i].chr +=
                iter->strides[i][iter->cur_dim] * iter->elem_sz[i];
        }
    } else {
        while (1) {
            iter->cur_dim--;
            if (iter->cur_dim < 0) {
                goto iter_end;
            }
            if (++iter->coordinates[iter->cur_dim] <
                iter->broadcast_shape[iter->cur_dim]) {
                for (int i = 0; i < iter->nop; i++) {
                    iter->data[i].chr -=
                        iter->backstrides[i][iter->cur_dim + 1] *
                        iter->elem_sz[i];
                    iter->data[i].chr +=
                        iter->strides[i][iter->cur_dim] * iter->elem_sz[i];
                }
                for (int i = iter->cur_dim + 1; i < iter->ndim; i++) {
                    iter->coordinates[i] = 0;
                }
                iter->cur_dim = iter->ndim - 1;
                return 1;
            }
        }
    }

    return 1;
iter_end:
    return 0;
}
