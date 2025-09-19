#include "qtk/linalg/qtk_linalg_determinant.h"
#include "qtk/math/qtk_math.h"
#include "qtk/math/qtk_matrix.h"

extern int sgesvd_(char *jobu, char *jobvt, int *m, int *n, float *a, int *lda,
                   float *s, float *u, int *ldu, float *vt, int *ldvt,
                   float *work, int *lwork, int *info);

static int umeyama_(float *src, float *dst, int num, int dim, float *result,
                    int estimate_scale) {
#define MAX_DIM 4
    if (dim > MAX_DIM) {
        goto err;
    }

    float src_mean[MAX_DIM] = {0};
    float tmp_a[MAX_DIM] = {0};
    float src_demean_mean[MAX_DIM] = {0};
    float src_demean_var[MAX_DIM] = {0};
    float dst_mean[MAX_DIM] = {0};
    float A[MAX_DIM * MAX_DIM];
    float AT[MAX_DIM * MAX_DIM];
    float D[MAX_DIM * 1];
    float *T = result;
    memset(T, 0, sizeof(float) * (dim + 1) * (dim + 1));
    float VT[MAX_DIM * MAX_DIM];
    float S[MAX_DIM];
    int WORK[MAX_DIM * 5];
    float sum;
    int i, j, m;
    int succ;
    float det, det1;
    char jobA = 'A';
    char jobO = 'O';
    float *U;
    int lwork = sizeof(WORK) / sizeof(float);
    int rank;
    int s;
    float scale = 1;

    for (i = 0; i < num; i++) {
        for (j = 0; j < dim; j++) {
            src_mean[j] += src[i * dim + j];
            dst_mean[j] += dst[i * dim + j];
        }
    }
    for (j = 0; j < dim; j++) {
        src_mean[j] /= num;
        dst_mean[j] /= num;
    }
    for (i = 0; i < num; i++) {
        for (j = 0; j < dim; j++) {
            src_demean_mean[j] += src[i * dim + j] - src_mean[j];
        }
    }
    for (j = 0; j < dim; j++) {
        src_demean_mean[j] /= num;
    }

    for (i = 0; i < dim; i++) {
        for (j = 0; j < dim; j++) {
            sum = 0;
            for (m = 0; m < num; m++) {
                sum += (dst[m * dim + i] - dst_mean[i]) *
                       (src[m * dim + j] - src_mean[j]);
            }
            A[i * dim + j] = sum / num;
        }
        D[i] = 1.0;
    }

    memcpy(AT, A, sizeof(float) * dim * dim);
    det = qtk_linalg_determinant(AT, dim, WORK, &succ);
    if (succ != 0) {
        goto err;
    }
    if (det < 0) {
        D[dim - 1] = -1;
    }
    qtk_matrix_eye(T, dim + 1);

    qtk_matrix_transposef(A, AT, dim, dim);

    sgesvd_(&jobO, &jobA, &dim, &dim, AT, &dim, S, NULL, &dim, VT, &dim,
            (float *)WORK, &lwork, &succ);
    if (succ != 0) {
        goto err;
    }
    U = AT;
    for (i = 0; i < dim; i++) {
        if (S[i] < 0.0001) {
            break;
        }
    }
    rank = i;
    if (rank == 0) {
        goto err;
    }
    if (rank == dim - 1) {
        det = qtk_linalg_determinant(U, dim, WORK, &succ);
        if (succ != 0) {
            goto err;
        }
        det1 = qtk_linalg_determinant(U, dim, WORK, &succ);
        if (succ != 0) {
            goto err;
        }
        if (det * det1 > 0) {
            for (i = 0; i < dim; i++) {
                for (j = 0; j < dim; j++) {
                    sum = 0;
                    for (m = 0; m < dim; m++) {
                        sum += U[i * dim + m] * VT[m * dim + j];
                    }
                    T[i * (dim + 1) + j] = sum;
                }
            }
        } else {
            s = D[dim - 1];
            D[dim - 1] = -1;
            for (i = 0; i < dim; i++) {
                for (j = 0; j < dim; j++) {
                    sum = 0;
                    for (m = 0; m < dim; m++) {
                        sum += U[i * dim + m] * VT[m * dim + j] * D[i];
                    }
                    T[i * (dim + 1) + j] = sum;
                }
            }
            D[dim - 1] = s;
        }
    } else {
        for (i = 0; i < dim; i++) {
            for (j = 0; j < dim; j++) {
                sum = 0;
                for (m = 0; m < dim; m++) {
                    sum += U[i * dim + m] * VT[m * dim + j] * D[i];
                }
                T[i * (dim + 1) + j] = sum;
            }
        }
    }

    if (estimate_scale) {
        float tmp;
        for (i = 0; i < num; i++) {
            for (j = 0; j < dim; j++) {
                tmp = src[i * dim + j] - src_demean_mean[j] - src_mean[j];
                src_demean_var[j] += tmp * tmp;
            }
        }
        scale = 0;
        for (j = 0; j < dim; j++) {
            src_demean_var[j] /= num;
            scale += src_demean_var[j];
        }
        tmp = 0;
        for (j = 0; j < dim; j++) {
            tmp += D[j] * S[j];
        }
        scale = 1.0 / scale * tmp;
    }

    for (i = 0; i < dim; i++) {
        for (j = 0; j < dim; j++) {
            T[i * (dim + 1) + j] *= scale;
        }
    }

    for (i = 0; i < dim; i++) {
        for (j = 0; j < dim; j++) {
            tmp_a[i] += T[i * (dim + 1) + j] * src_mean[j];
        }
    }
    for (i = 0; i < dim; i++) {
        T[i * (dim + 1) + dim] = dst_mean[i] - tmp_a[i];
    }

#undef MAX_DIM
    return 0;
err:
    return -1;
}

int qtk_geometric_similarity_transform(float *src, float *dst, int num, int dim,
                                       float *result) {
    return umeyama_(src, dst, num, dim, result, 1);
}

int qtk_geometric_euclidean_transform(float *src, float *dst, int num, int dim,
                                      float *result) {
    return umeyama_(src, dst, num, dim, result, 0);
}

int qtk_geometric_invert_affine_transform(const float *src, float *dst) {
#define F(x, y) src[(x)*3 + (y)]
#define S(x, y, value) dst[(x)*3 + (y)] = value

    float A11, A22, A12, A21, b1, b2;
    float D = F(0, 0) * F(1, 1) - F(0, 1) * F(1, 0);
    if (D != 0) {
        D = 1.0 / D;
    }

    A11 = F(1, 1) * D;
    A22 = F(0, 0) * D;
    A12 = -F(0, 1) * D;
    A21 = -F(1, 0) * D;
    b1 = -A11 * F(0, 2) - A12 * F(1, 2);
    b2 = -A21 * F(0, 2) - A22 * F(1, 2);

    S(0, 0, A11);
    S(0, 1, A12);
    S(0, 2, b1);

    S(1, 0, A21);
    S(1, 1, A22);
    S(1, 2, b2);

#undef F
#undef S
    return 0;
}

static uint8_t get_pixel_(uint8_t *src, int y, int x, int k, int h, int w,
                          float *mat, int channel) {
#define M(x, y) mat[(x)*3 + (y)]
#define GETP(x, y) src[(y) * (w)*channel + (x)*channel + k]
    float fx = x * M(0, 0) + y * M(0, 1) + M(0, 2);
    float fy = x * M(1, 0) + y * M(1, 1) + M(1, 2);
    int low_x = floorf(fx);
    int high_x = ceilf(fx);
    int low_y = floorf(fy);
    int high_y = ceilf(fy);
    float pos_x = fx - low_x;
    float pos_y = fy - low_y;
    if (low_x >= 0 && high_x < w && low_y >= 0 && high_y < h) {
        float p0_area = (1 - pos_x) * (1 - pos_y);
        float p1_area = (pos_x) * (1 - pos_y);
        float p2_area = (1 - pos_x) * (pos_y);
        float p3_area = (pos_x) * (pos_y);
        // p0        p1
        //       p
        // p2        p3
        float pixel =
            GETP(low_x, low_y) * p0_area + GETP(high_x, low_y) * p1_area +
            GETP(low_x, high_y) * p2_area + GETP(high_x, high_y) * p3_area;
        return QBL_USAT8(pixel);
    }
#undef M
#undef GETP
    return 0;
}

int qtk_geometric_apply_affine_transform(uint8_t *src, uint8_t *dst, int h,
                                         int w, int new_h, int new_w,
                                         int channel, float *mat) {
    int i, j, k;
    float inv_mat[6];
    qtk_geometric_invert_affine_transform(mat, inv_mat);
    for (i = 0; i < new_h; i++) {
        for (j = 0; j < new_w; j++) {
            for (k = 0; k < channel; k++) {
                *dst++ = get_pixel_(src, i, j, k, h, w, inv_mat, channel);
            }
        }
    }
    return 0;
}
