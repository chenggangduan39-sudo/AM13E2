#ifndef QBL_NN_QBL_CONV2D_H
#define QBL_NN_QBL_CONV2D_H
#pragma once
#include "qtk/core/qtk_type.h"
#include "qtk/linalg/qtk_gemm.h"
#include "qtk/nn/qtk_nn_im2col.h"
#include "qtk/nn/qtk_nn_utils.h"
#include "qtk/numeric/qtk_numeric_type.h"
#ifdef __cplusplus
extern "C" {
#endif
#define IN_RANGE(n, left, right) ((n) >= (left) && (n) <= (right))
static void nn_conv2d_pad(float *pv,float *pv_old, uint32_t *shape, int pad1, int pad2){
    int a, b, c, d;
    int h = shape[2] + pad1 * 2;
    int w = shape[3] + pad2 * 2;
    for (a = 0; a < shape[0]; a++) {
        for (b = 0; b < shape[1]; b++) {
            for (c = 0; c < h; c++) {
                for (d = 0; d < w; d++) {
                    int c1, d1;
                    c1 = c - pad1;
                    d1 = d - pad2;
                    if (IN_RANGE(c1, 0, shape[2] - 1) &&
                        IN_RANGE(d1, 0, shape[3] - 1)) {
                        *pv++ = *pv_old++;
                    } else {
                        *pv++ = 0.0;
                    }
                }
            }
        }
    }
}

void conv2d_acc(float *c, float a, float *b, int len, int stride){
    int i,j;
    for (i = 0, j = 0; i < len; i++,j+=stride){
        c[i] += a * b[j];
    }
}

void cov2d_sgemm(int M, int N, int K, float ALPHA, float *A,
    int lda, float **B, int ldb, float BETA, float *C, int ldc, int stride, int pstride) {
    int i, j, k;

    if(N % stride != 0){
        exit(0);
    }
    int n = N / stride;

    for (i = 0; i < M; i++) {
        for (j = 0; j < N; j++) {
            C[i * ldc + j] *= BETA;
        }
    }

    int xx;
    for (i = 0; i < M; ++i) {
        xx = 0;
        for (k = 0; k < K; ++k) {
            register float A_PART = ALPHA * A[i * lda + k];
            for (j = 0; j < n; ++j, ++xx) {
                conv2d_acc(C + i * ldc + j * stride, A_PART, B[xx], stride, pstride);
                //C[i * ldc + j] += A_PART * B[k * ldb + j];
            }
        }
    }
}

qtk_maybe_unused static void
nn_conv2d_(float *X, float **idx,qtk_numeric_data_t Y, qtk_numeric_data_t W,
           qtk_numeric_data_t B, int* ksize, int *stride,
           int out_channel, int groups, uint32_t *shape, int hasB) {
    int in_b = shape[0];
    int in_c = shape[1];
    int in_h = shape[2];
    int in_w = shape[3];
    int out_w = qtk_nn_pad_dim(in_w, 0, 0, ksize[1], stride[1]);
    int out_h = qtk_nn_pad_dim(in_h, 0, 0, ksize[0], stride[0]);

    int m = out_channel / groups;
    int k = ksize[0] * ksize[1] * in_c / groups;
    int n = out_w * out_h;
    float *x_f32 = X;
    float *w_f32 = W.f32;
    float *y_f32 = Y.f32;
    float *bias_f32 = B.f32;

    float **index = idx;
    int id_stride;

    for (int i = 0; i < in_b; i++) {
        for (int j = 0; j < groups;
             j++, y_f32 += n * m, x_f32 += in_c / groups * in_h * in_w) {
            float *a = w_f32 + m * k * j;
            float *b;
            float *c = y_f32;
            float *im = x_f32;
            if (ksize[0] == 1 && ksize[1] == 1) {
                b = im;
                qtk_sgemm(0, 0, m, n, k, 1.0, a, k, b, n, 0.0, c, n);
            } else {
                //im2col_cpu_(im, in_c / groups, in_h, in_w, ksize, stride, npad,
                //            b);
                im2col2_cpu_(im, in_c / groups, in_h, in_w, ksize, stride,
                            index, &id_stride);
                cov2d_sgemm(m, n, k, 1.0, a, k, index, n, 0.0, c, n, id_stride, stride[1]);
            }
        }
    }

    if(!hasB){
        return;
    }
    y_f32 = Y.f32;
    for (int i = 0; i < in_b; i++) {
        for (int j = 0; j < out_channel; j++) {
            for (int ii = 0; ii < n; ii++) {
                *y_f32++ += bias_f32[j];
            }
        }
    }
}

void Axpy4x4x(int k, float *a, int lda, float *b, int ldb, float *c, int ldc) {
    // loop parameters
    int i;
    // Point to the address at the beginning of the rows
    float
        /* Point to the current elements in the four rows of A */
        *a_0i_pntr,
        //*a_1i_pntr, *a_2i_pntr, *a_3i_pntr,
        /* Point to the current elements in the four rows of B */
        *b_0i_pntr, *b_1i_pntr, *b_2i_pntr, *b_3i_pntr;
    // register values
    register float
        /* C( 0, 0 ), C( 0, 1 ), C( 0, 2 ), C( 0, 3 )
           C( 1, 0 ), C( 1, 1 ), C( 1, 2 ), C( 1, 3 )
           C( 2, 0 ), C( 2, 1 ), C( 2, 2 ), C( 2, 3 )
           C( 3, 0 ), C( 3, 1 ), C( 3, 2 ), C( 3, 3 ) */
        c_00_reg,
        c_01_reg, c_02_reg, c_03_reg, c_10_reg, c_11_reg, c_12_reg, c_13_reg,
        c_20_reg, c_21_reg, c_22_reg, c_23_reg, c_30_reg, c_31_reg, c_32_reg,
        c_33_reg,
        /* A( 0, i ), A( 1, i ), A( 2, i ), A( 3, i ) */
        a_0i_reg, a_1i_reg, a_2i_reg, a_3i_reg,
        /* B( 0, i ), B( 1, i ), B( 2, i ), B( 3, i ) */
        b_0i_reg, b_1i_reg, b_2i_reg, b_3i_reg;

    c_00_reg = 0.0;
    c_01_reg = 0.0;
    c_02_reg = 0.0;
    c_03_reg = 0.0;
    c_10_reg = 0.0;
    c_11_reg = 0.0;
    c_12_reg = 0.0;
    c_13_reg = 0.0;
    c_20_reg = 0.0;
    c_21_reg = 0.0;
    c_22_reg = 0.0;
    c_23_reg = 0.0;
    c_30_reg = 0.0;
    c_31_reg = 0.0;
    c_32_reg = 0.0;
    c_33_reg = 0.0;

    a_0i_pntr = a;
    //a_1i_pntr = a + lda;
    //a_2i_pntr = a + 2 * lda;
    //a_3i_pntr = a + 3 * lda;

    b_0i_pntr = b;
    b_1i_pntr = b + ldb;
    b_2i_pntr = b + 2 * ldb;
    b_3i_pntr = b + 3 * ldb;
    int xx = k - k/4*4;
    //int xx2 = k/4 * 4;
    for (i = 0; i < k; i++) {
        //a_0i_reg = *a_0i_pntr++;
        //a_1i_reg = *a_1i_pntr++;
        //a_2i_reg = *a_2i_pntr++;
        //a_3i_reg = *a_3i_pntr++;

        if(i < k/4 * 4){
            //wtk_debug("%d\n",i/4 * 16 + i%4);
            a_0i_pntr = a + i/4 * 16 + i%4;
            a_0i_reg = *a_0i_pntr;
            a_1i_reg = *(a_0i_pntr+4);
            a_2i_reg = *(a_0i_pntr+8);
            a_3i_reg = *(a_0i_pntr+12);
            //a_1i_pntr = b + k/4 * 16;
            a_0i_pntr = a + k/4 * 16;
        }else{
            a_0i_reg = *a_0i_pntr;
            a_1i_reg = *(a_0i_pntr + xx);
            a_2i_reg = *(a_0i_pntr + xx * 2);
            a_3i_reg = *(a_0i_pntr + xx * 3);
            a_0i_pntr++;
        }

        b_0i_reg = *b_0i_pntr++;
        b_1i_reg = *b_1i_pntr++;
        b_2i_reg = *b_2i_pntr++;
        b_3i_reg = *b_3i_pntr++;
        // first row
        c_00_reg += a_0i_reg * b_0i_reg;
        //printf("%f %f\n",a_0i_reg,b_0i_reg);
        c_01_reg += a_0i_reg * b_1i_reg;
        //printf("%f %f\n",a_1i_reg,b_1i_reg);
        c_02_reg += a_0i_reg * b_2i_reg;
        //printf("%f %f\n",a_2i_reg,b_2i_reg);
        c_03_reg += a_0i_reg * b_3i_reg;
        //printf("%f %f\n",a_3i_reg,b_3i_reg);

        // second row
        c_10_reg += a_1i_reg * b_0i_reg;
        c_11_reg += a_1i_reg * b_1i_reg;
        c_12_reg += a_1i_reg * b_2i_reg;
        c_13_reg += a_1i_reg * b_3i_reg;

        // third row
        c_20_reg += a_2i_reg * b_0i_reg;
        c_21_reg += a_2i_reg * b_1i_reg;
        c_22_reg += a_2i_reg * b_2i_reg;
        c_23_reg += a_2i_reg * b_3i_reg;

        // four row
        c_30_reg += a_3i_reg * b_0i_reg;
        c_31_reg += a_3i_reg * b_1i_reg;
        c_32_reg += a_3i_reg * b_2i_reg;
        c_33_reg += a_3i_reg * b_3i_reg;
    }
    *c += c_00_reg;
    *(c + 1) += c_01_reg;
    *(c + 2) += c_02_reg;
    *(c + 3) += c_03_reg;

    *(c + ldc) += c_10_reg;
    *(c + 1 + ldc) += c_11_reg;
    *(c + 2 + ldc) += c_12_reg;
    *(c + 3 + ldc) += c_13_reg;

    *(c + 2 * ldc) += c_20_reg;
    *(c + 1 + 2 * ldc) += c_21_reg;
    *(c + 2 + 2 * ldc) += c_22_reg;
    *(c + 3 + 2 * ldc) += c_23_reg;

    *(c + 3 * ldc) += c_30_reg;
    *(c + 1 + 3 * ldc) += c_31_reg;
    *(c + 2 + 3 * ldc) += c_32_reg;
    *(c + 3 + 3 * ldc) += c_33_reg;
}

/* So, this routine computes a 4x4 block of matrix A

       C( 0, 0 ), C( 0, 1 ), C( 0, 2 ), C( 0, 3 ).

 Notice that this routine is called with c = C( i, j ) in the
 previous routine, so these are actually the elements

       C( i  , j ), C( i  , j+1 ), C( i  , j+2 ), C( i  , j+3 )

 in the original matrix C */
void Axpy1x4x(int k, float *a, int lda, float *b, int ldb, float *c, int ldc) {
    // loop parameters
    int i;
    // Point to the address at the beginning of the rows
    float
        /* Point to the current elements in the one rows of A */
        *a_0i_pntr,
        /* Point to the current elements in the four rows of B */
        *b_0i_pntr,*b_1i_pntr,*b_2i_pntr,*b_3i_pntr;
    // register values
    register float
        /* C( 0, 0 ), C( 0, 1 ), C( 0, 2 ), C( 0, 3 ) */
        c_00_reg,
        c_01_reg, c_02_reg, c_03_reg,
        /* A( 0, p ) */
        a_0i_reg,
        /* B( 0, p ), B( 1, p ), B( 2, p ), B( 3, p ) */
        b_0i_reg, b_1i_reg, b_2i_reg, b_3i_reg;

    c_00_reg = 0.0;
    c_01_reg = 0.0;
    c_02_reg = 0.0;
    c_03_reg = 0.0;

    a_0i_pntr = a;

    b_0i_pntr = b;
    b_1i_pntr = b + ldb;
    b_2i_pntr = b + 2 * ldb;
    b_3i_pntr = b + 3 * ldb;

    //int xx = k - k/4*4;
    for (i = 0; i < k; i++) {
        a_0i_reg = *a_0i_pntr++;

        b_0i_reg = *b_0i_pntr++;
        b_1i_reg = *b_1i_pntr++;
        b_2i_reg = *b_2i_pntr++;
        b_3i_reg = *b_3i_pntr++;
        // if(i < k/4 * 4){
        //     b_0i_pntr = b + i/4 * 16 + i%4;
        //     b_0i_reg = *b_0i_pntr;
        //     b_1i_reg = *(b_0i_pntr+4);
        //     b_2i_reg = *(b_0i_pntr+8);
        //     b_3i_reg = *(b_0i_pntr+12);
        //     b_0i_pntr = b + k/4 * 16;
        // }else{
        //     b_0i_reg = *b_0i_pntr;
        //     b_1i_reg = *(b_0i_pntr + xx);
        //     b_2i_reg = *(b_0i_pntr + xx * 2);
        //     b_3i_reg = *(b_0i_pntr + xx * 3);
        //     b_0i_pntr++;
        // }

        c_00_reg += a_0i_reg * b_0i_reg;
        c_01_reg += a_0i_reg * b_1i_reg;
        c_02_reg += a_0i_reg * b_2i_reg;
        c_03_reg += a_0i_reg * b_3i_reg;
    }

    *c += c_00_reg;
    *(c + 1) += c_01_reg;
    *(c + 2) += c_02_reg;
    *(c + 3) += c_03_reg;
}

/* So, this routine computes a 4x4 block of matrix A

       C( 0, 0 ).
       C( 1, 0 ).
       C( 2, 0 ).
       C( 3, 0 ).

 Notice that this routine is called with c = C( i, j ) in the
 previous routine, so these are actually the elements

       C( i  , j )
       C( i+1, j )
       C( i+2, j )
       C( i+3, j )

 in the original matrix C */
void Axpy4x1x(int k, float *a, int lda, float *b, int ldb, float *c, int ldc) {
    // loop parameters
    int i;
    // Point to the address at the beginning of the rows
    float
        /* Point to the current elements in the four rows of A */
        *a_0i_pntr,
        //*a_1i_pntr, *a_2i_pntr, *a_3i_pntr,
        /* Point to the current elements in the one rows of B */
        *b_0i_pntr;
    // register values
    register float
        /* C( 0, 0 )
           C( 1, 0 )
           C( 2, 0 )
           C( 3, 0 ) */
        c_00_reg,
        c_10_reg, c_20_reg, c_30_reg,
        /* A( 0, p ), A( 1, p ), A( 2, p ), A( 3, p ) */
        a_0i_reg, a_1i_reg, a_2i_reg, a_3i_reg,
        /* B( 0, p ) */
        b_0i_reg;

    c_00_reg = 0.0;
    c_10_reg = 0.0;
    c_20_reg = 0.0;
    c_30_reg = 0.0;

    a_0i_pntr = a;
    //a_1i_pntr = a + lda;
    //a_2i_pntr = a + 2 * lda;
    //a_3i_pntr = a + 3 * lda;

    b_0i_pntr = b;
    int xx = k - k/4*4;
    for (i = 0; i < k; i++) {
        //a_0i_reg = *a_0i_pntr++;
        //a_1i_reg = *a_1i_pntr++;
        //a_2i_reg = *a_2i_pntr++;
        //a_3i_reg = *a_3i_pntr++;
        if(i < k/4 * 4){
            a_0i_pntr = a + i/4 * 16 + i%4;
            a_0i_reg = *a_0i_pntr;
            a_1i_reg = *(a_0i_pntr+4);
            a_2i_reg = *(a_0i_pntr+8);
            a_3i_reg = *(a_0i_pntr+12);
            a_0i_pntr = a + k/4 * 16;
        }else{
            a_0i_reg = *a_0i_pntr;
            a_1i_reg = *(a_0i_pntr + xx);
            a_2i_reg = *(a_0i_pntr + xx * 2);
            a_3i_reg = *(a_0i_pntr + xx * 3);
            a_0i_pntr++;
        }

        b_0i_reg = *b_0i_pntr++;

        c_00_reg += a_0i_reg * b_0i_reg;
        c_10_reg += a_1i_reg * b_0i_reg;
        c_20_reg += a_2i_reg * b_0i_reg;
        c_30_reg += a_3i_reg * b_0i_reg;
    }

    *c += c_00_reg;
    *(c + ldc) += c_10_reg;
    *(c + 2 * ldc) += c_20_reg;
    *(c + 3 * ldc) += c_30_reg;
}

/* So, this routine computes a 4x4 block of matrix A

       C( 0, 0 ).

 Notice that this routine is called with c = C( i, j ) in the
 previous routine, so these are actually the elements

       C( i , j )

 in the original matrix C */
void Axpy1x1x(int k, float *a, int lda, float *b, int ldb, float *c, int ldc) {
    // loop parameters
    int i;
    // Point to the address at the beginning of the rows
    float
        /* Point to the current elements in the one rows of A */
        *a_0i_pntr,
        /* Point to the current elements in the one rows of B */
        *b_0i_pntr;
    // register values
    register float
        /* C( 0, 0 ) */
        c_00_reg,
        /* A( 0, p ) */
        a_0i_reg,
        /* B( 0, p ) */
        b_0i_reg;

    c_00_reg = 0.0;
    a_0i_pntr = a;
    b_0i_pntr = b;

    for (i = 0; i < k; i++) {
        a_0i_reg = *a_0i_pntr++;
        b_0i_reg = *b_0i_pntr++;

        c_00_reg += a_0i_reg * b_0i_reg;
    }

    *c += c_00_reg;
}

/*b matrix transpose is performed by default*/
void matrix_mul2(int m, int n, int k, float *a, int lda, float *b, int ldb,
                float *c, int ldc) {
    // loop parameters
    int i, j;
    // m_4 and n_4 is the multiple of four
    int m_4, n_4;
    // m_1 and n_1 is the remainder of one
    int m_1, n_1;
    // A, B and C pointer to the beginning of loop
    float *A, *B, *C;

    m_4 = (m >> 2) << 2;
    n_4 = (n >> 2) << 2;

    m_1 = m - m_4;
    n_1 = n - n_4;
    /*
            c(m, n) = a(m, k) * b(n, k)
                                    ||
            c(m_4, n_4) = a(m_4, n_4) * b(m_4, n_4)
            c(m_1, n_4) = a(m_1, n_4) * b(m_1, n_4)
            c(m_4, n_1) = a(m_4, n_1) * b(m_4, n_1)
            c(m_1, n_1) = a(m_1, n_1) * b(m_1, n_1)
    */

    /* A B C point to the beginning of memory to AddDot4x4 */
    //wtk_debug("%d %d\n",m_4,n_4);
    if (m_4 && n_4) {
        for (j = 0; j < n_4;
             j += 4) { // loop over the columns of c, unrolled by 4
            B = (b) + j * ldb;
            for (i = 0; i < m_4;
                 i += 4) { // loop over the rows of c, unrolled by 4
                A = (a) + i * lda;
                C = (c) + i * ldc + j;
#ifdef USE_NEON
                GemmNeonAxpy4x4_2(k, A, lda, B, ldb, C, ldc);
                // NeonAxpy4x4(k, A, lda, B, ldb, C, ldc);
#else
                Axpy4x4x(k, A, lda, B, ldb, C, ldc);
#endif
            }
        }
    }
    //exit(0);
    if (m_1 && n_4) {
        for (j = 0; j < n_4;
             j += 4) { // loop over the columns of c, unrolled by 4
            B = (b) + j * ldb;
            for (i = 0; i < m_1; i++) { // loop over the rows of c, unrolled by 1
                A = (a + m_4 * lda) + i * lda;
                C = (c + m_4 * ldc) + i * ldc + j;
                Axpy1x4x(k, A, lda, B, ldb, C, ldc);
            }
        }
    }

    if (m_4 && n_1) {
        for (j = 0; j < n_1; j++) { // loop over the rows of c, unrolled by 1
            B = (b + n_4 * ldb) + j * ldb;
            for (i = 0; i < m_4;
                 i += 4) { // loop over the rows of c, unrolled by 4
                A = (a) + i * lda;
                C = (c + n_4) + i * ldc + j;
                Axpy4x1x(k, A, lda, B, ldb, C, ldc);
            }
        }
    }

    if (m_1 && n_1) {
        for (j = 0; j < n_1; j++) { // loop over the rows of c, unrolled by 1
            B = (b + n_4 * ldb) + j * ldb;
            for (i = 0; i < m_1; i++) { // loop over the rows of c, unrolled by
                                        // 1
                A = (a + m_4 * lda) + i * lda;
                C = (c + m_4 * ldc + n_4) + i * ldc + j;
                Axpy1x1x(k, A, lda, B, ldb, C, ldc);
            }
        }
    }
}

qtk_maybe_unused static void
nn_conv2d_naive_(float *X, float* Y,
                 qtk_numeric_data_t W, qtk_numeric_data_t B, int *pad,
                 int *ksize, int *stride, int out_channel, int groups,
                 uint32_t *shape, uint8_t *dilations,
                 float* workspace, int hasB, int prelu) {
    int in_b = shape[0];
    int in_c = shape[1];
    int in_h = shape[2];
    int in_w = shape[3];
    const int out_h =
        (shape[2] + pad[0] + pad[2] - (dilations[0] * (ksize[0] - 1) + 1)) /
            stride[0] +
        1;
    const int out_w =
        (shape[3] + pad[1] + pad[3] - (dilations[1] * (ksize[1] - 1) + 1)) /
            stride[1] +
        1;

    int m = out_channel / groups;
    int k = ksize[0] * ksize[1] * in_c / groups;
    int n = out_w * out_h;
    float *x_f32 = X;
    float *w_f32 = W.f32;
    float *y_f32 = Y;
    float *bias_f32 = B.f32;

//    wtk_debug("%d %d %d %d %d\n",ksize[0],ksize[1],in_c,groups,k);
    for (int i = 0; i < in_b; i++) {
        for (int j = 0; j < groups;
             j++, y_f32 += n * m, x_f32 += in_c / groups * in_h * in_w) {
            float *a = w_f32 + m * k * j;
            float *b = workspace;
            float *c = y_f32;
            float *im = x_f32;
//            wtk_debug("%d %d\n",m,k);
            //TODO for fast matmul in im2col b is transposed
            //if (ksize[0] == 1 && ksize[1] == 1) {
            //    b = im;
            //} else {
//            im2col_cpu_re_(im, in_c / groups, in_h, in_w, ksize, stride, pad,
//                        b, dilations);
            im2col_cpu_(im, in_c / groups, in_h, in_w, ksize, stride, pad,
                        b, dilations);
            //}
            //wtk_debug("%d %d\n",m,k);
            //wtk_debug("%d %d\n",n,k);
            //print_float(b, n * k);
            memset(c, 0, sizeof(float) * m * n);
        //    qtk_sgemm(0, 1, m, n, k, 1.0, a, k, b, k, 0.0, c, n);
            matrix_mul2(m, n, k, a, k, b, k, c, n);
        }
    }
    //wtk_debug("conv2d\n");
    //print_float(Y, in_b * out_channel * out_h * out_w);
    //exit(0);
    if(prelu){
        float *slope = bias_f32 + out_channel;
        y_f32 = Y;
        for (int i = 0; i < in_b; i++) {
            for (int j = 0; j < out_channel; j++) {
                for (int ii = 0; ii < n; ii++, y_f32++) {
                    *y_f32 += bias_f32[j];
                    if(*y_f32 < 0){
                        *y_f32 *= slope[j];
                    }
                }
            }
        }
    }else if(hasB){
        y_f32 = Y;
        for (int i = 0; i < in_b; i++) {
            for (int j = 0; j < out_channel; j++) {
                for (int ii = 0; ii < n; ii++) {
                    *y_f32++ += bias_f32[j];
                }
            }
        }
    }
}

qtk_maybe_unused static void
nn_conv2d_naive2_(float *X, float* Y,
                 qtk_numeric_data_t W, qtk_numeric_data_t B, int *pad,
                 int *ksize, int *stride, int out_channel, int groups,
                 uint32_t *shape, uint8_t *dilations,
                 float* workspace, int hasB, int prelu) {
    int in_b = shape[0];
    int in_c = shape[1];
    int in_h = shape[2];
    int in_w = shape[3];
    const int out_h =
        (shape[2] + pad[0] + pad[2] - (dilations[0] * (ksize[0] - 1) + 1)) /
            stride[0] +
        1;
    const int out_w =
        (shape[3] + pad[1] + pad[3] - (dilations[1] * (ksize[1] - 1) + 1)) /
            stride[1] +
        1;

    int m = out_channel / groups;
    int k = ksize[0] * ksize[1] * in_c / groups;
    int n = out_w * out_h;
    float *x_f32 = X;
    float *w_f32 = W.f32;
    float *y_f32 = Y;
    float *bias_f32 = B.f32;

//    wtk_debug("%d %d %d %d %d\n",ksize[0],ksize[1],in_c,groups,k);
    for (int i = 0; i < in_b; i++) {
        for (int j = 0; j < groups;
             j++, y_f32 += n * m, x_f32 += in_c / groups * in_h * in_w) {
            float *a = w_f32 + m * k * j;
            float *b = workspace;
            float *c = y_f32;
            float *im = x_f32;
//            wtk_debug("%d %d\n",m,k);
            //TODO for fast matmul in im2col b is transposed
            //if (ksize[0] == 1 && ksize[1] == 1) {
            //    b = im;
            //} else {
//            im2col_cpu_re_(im, in_c / groups, in_h, in_w, ksize, stride, pad,
//                        b, dilations);
            im2col_cpu_(im, in_c / groups, in_h, in_w, ksize, stride, pad,
                        b, dilations);
            //}
            //wtk_debug("%d %d\n",m,k);
            //wtk_debug("%d %d\n",n,k);
            //print_float(b, n * k);
            memset(c, 0, sizeof(float) * m * n);
            qtk_sgemm(0, 1, m, n, k, 1.0, a, k, b, k, 0.0, c, n);
            //matrix_mul2(m, n, k, a, k, b, k, c, n);
        }
    }
    //wtk_debug("conv2d\n");
    //print_float(Y, in_b * out_channel * out_h * out_w);
    //exit(0);
    if(prelu){
        float *slope = bias_f32 + out_channel;
        y_f32 = Y;
        for (int i = 0; i < in_b; i++) {
            for (int j = 0; j < out_channel; j++) {
                for (int ii = 0; ii < n; ii++, y_f32++) {
                    *y_f32 += bias_f32[j];
                    if(*y_f32 < 0){
                        *y_f32 *= slope[j];
                    }
                }
            }
        }
    }else if(hasB){
        y_f32 = Y;
        for (int i = 0; i < in_b; i++) {
            for (int j = 0; j < out_channel; j++) {
                for (int ii = 0; ii < n; ii++) {
                    *y_f32++ += bias_f32[j];
                }
            }
        }
    }
}

#ifdef __cplusplus
};
#endif
#endif
