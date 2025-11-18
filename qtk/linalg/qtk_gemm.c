#ifdef USE_NEON
#include <arm_neon.h>

static void gemm_nn_neon(int M, int N, int K, float ALPHA, 
                         float *A, int lda, float *B, int ldb, 
                         float *C, int ldc) {
    for (int i = 0; i < M; ++i) {
        for (int k = 0; k < K; ++k) {
            // 1. 计算标量A_PART并广播到NEON向量
            float A_PART = ALPHA * A[i * lda + k];
            float32x4_t a_vec = vdupq_n_f32(A_PART); // 复制到4个通道

            // 2. 主循环：每次处理4个元素（N维度）
            int j = 0;
            for (; j <= N - 4; j += 4) {
                // 加载B的4个连续元素
                float32x4_t b_vec = vld1q_f32(&B[k * ldb + j]);
                // 加载C的当前值
                float32x4_t c_vec = vld1q_f32(&C[i * ldc + j]);
                // 融合乘加：C_vec += A_PART * B_vec
                c_vec = vmlaq_f32(c_vec, a_vec, b_vec);
                // 存回C矩阵
                vst1q_f32(&C[i * ldc + j], c_vec);
            }

            // 3. 尾部处理：剩余1-3个元素
            for (; j < N; ++j) {
                C[i * ldc + j] += A_PART * B[k * ldb + j];
            }
        }
    }
}

static void gemm_tn_neon(int M, int N, int K, float ALPHA, 
                         float *A, int lda, float *B, int ldb, 
                         float *C, int ldc) {
    for (int i = 0; i < M; ++i) {
        for (int k = 0; k < K; ++k) {
            // 标量计算并广播到NEON向量
            float A_PART = ALPHA * A[k * lda + i];
            float32x4_t a_vec = vdupq_n_f32(A_PART);

            // 主循环：每次处理4个元素（128位寄存器）
            int j = 0;
            for (; j <= N - 4; j += 4) {
                // 加载B的4个连续元素
                float32x4_t b_vec = vld1q_f32(&B[k * ldb + j]);
                // 加载C的当前值
                float32x4_t c_vec = vld1q_f32(&C[i * ldc + j]);
                // 融合乘加：C += A_PART * B
                c_vec = vmlaq_f32(c_vec, a_vec, b_vec);
                // 存回C矩阵
                vst1q_f32(&C[i * ldc + j], c_vec);
            }

            // 处理尾部剩余元素（不足4个）
            for (; j < N; ++j) {
                C[i * ldc + j] += A_PART * B[k * ldb + j];
            }
        }
    }
}

/* this routine computes a 4x4 block of matrix A

       C( 0, 0 ), C( 0, 1 ), C( 0, 2 ), C( 0, 3 ).
       C( 1, 0 ), C( 1, 1 ), C( 1, 2 ), C( 1, 3 ).
       C( 2, 0 ), C( 2, 1 ), C( 2, 2 ), C( 2, 3 ).
       C( 3, 0 ), C( 3, 1 ), C( 3, 2 ), C( 3, 3 ).

 Notice that this routine is called with c = C( i, j ) in the
 previous routine, so these are actually the elements

       C( i  , j ), C( i  , j+1 ), C( i  , j+2 ), C( i  , j+3 )
       C( i+1, j ), C( i+1, j+1 ), C( i+1, j+2 ), C( i+1, j+3 )
       C( i+2, j ), C( i+2, j+1 ), C( i+2, j+2 ), C( i+2, j+3 )
       C( i+3, j ), C( i+3, j+1 ), C( i+3, j+2 ), C( i+3, j+3 )

 in the original matrix C */
#define A(i, j) A[(i)*lda + (j)]
#define B(i, j) B[(i)*ldb + (j)]
#define C(i, j) C[(i)*ldc + (j)]
#define BLOCK 4
void GemmNeonAxpy4x4(int K, float *A, int lda, float *B, int ldb, float *C,
                     int ldc) {
    // loop parameters
    int i;
    // k_4 and n_1 is the multiple of four and one
    int k_4, k_1;
    // Neon values
    float32x4_t
        /* C( 0, 0 ), C( 0, 1 ), C( 0, 2 ), C( 0, 3 )
           C( 1, 0 ), C( 1, 1 ), C( 1, 2 ), C( 1, 3 )
           C( 2, 0 ), C( 2, 1 ), C( 2, 2 ), C( 2, 3 )
           C( 3, 0 ), C( 3, 1 ), C( 3, 2 ), C( 3, 3 ) */
        c_00_neon,
        c_01_neon, c_02_neon, c_03_neon, c_10_neon, c_11_neon, c_12_neon,
        c_13_neon, c_20_neon, c_21_neon, c_22_neon, c_23_neon, c_30_neon,
        c_31_neon, c_32_neon, c_33_neon,
        /* A( 0, i ), A( 0, i + 1 ), A( 0, i + 2 ), A( 0, i + 3 ) */
        a_0i_neon,
        /* A( 1, i ), A( 1, i + 1 ), A( 1, i + 2 ), A( 1, i + 3 ) */
        a_1i_neon,
        /* A( 2, i ), A( 2, i + 1 ), A( 2, i + 2 ), A( 2, i + 3 ) */
        a_2i_neon,
        /* A( 3, i ), A( 3, i + 1 ), A( 3, i + 2 ), A( 3, i + 3 ) */
        a_3i_neon,
        /* B( 0, i ), B( 0, i + 1 ), B( 0, i + 2 ), B( 0, i + 3 ) */
        b_0i_neon,
        /* B( 1, i ), B( 1, i + 1 ), B( 1, i + 2 ), B( 1, i + 3 ) */
        b_1i_neon,
        /* B( 2, i ), B( 2, i + 1 ), B( 2, i + 2 ), B( 2, i + 3 ) */
        b_2i_neon,
        /* B( 3, i ), B( 3, i + 1 ), B( 3, i + 2 ), B( 3, i + 3 ) */
        b_3i_neon;

    k_4 = K / BLOCK;
    k_1 = K - 4 * k_4;

    // if out neon register not load zero, the result is error
    c_00_neon = vmovq_n_f32(0);
    c_01_neon = vmovq_n_f32(0);
    c_02_neon = vmovq_n_f32(0);
    c_03_neon = vmovq_n_f32(0);
    c_10_neon = vmovq_n_f32(0);
    c_11_neon = vmovq_n_f32(0);
    c_12_neon = vmovq_n_f32(0);
    c_13_neon = vmovq_n_f32(0);
    c_20_neon = vmovq_n_f32(0);
    c_21_neon = vmovq_n_f32(0);
    c_22_neon = vmovq_n_f32(0);
    c_23_neon = vmovq_n_f32(0);
    c_30_neon = vmovq_n_f32(0);
    c_31_neon = vmovq_n_f32(0);
    c_32_neon = vmovq_n_f32(0);
    c_33_neon = vmovq_n_f32(0);

    if (k_4) {
        /* the loop-for is divided into four because cache line if the size of
           64 bytes(The possible reason needs to be confirmed later) */
        for (i = 0; i < k_4; i++) {
            // first row
            a_0i_neon = vld1q_f32(&A(0, i * BLOCK));

            b_0i_neon = vld1q_f32(&B(0, i * BLOCK));
            b_1i_neon = vld1q_f32(&B(1, i * BLOCK));
            b_2i_neon = vld1q_f32(&B(2, i * BLOCK));
            b_3i_neon = vld1q_f32(&B(3, i * BLOCK));

            c_00_neon = vfmaq_f32(c_00_neon, a_0i_neon, b_0i_neon);
            c_01_neon = vfmaq_f32(c_01_neon, a_0i_neon, b_1i_neon);
            c_02_neon = vfmaq_f32(c_02_neon, a_0i_neon, b_2i_neon);
            c_03_neon = vfmaq_f32(c_03_neon, a_0i_neon, b_3i_neon);
        //}

        //for (i = 0; i < k_4; i++) {
            // second row
            a_1i_neon = vld1q_f32(&A(1, i * BLOCK));

            // b_0i_neon = vld1q_f32(&B(0, i * BLOCK));
            // b_1i_neon = vld1q_f32(&B(1, i * BLOCK));
            // b_2i_neon = vld1q_f32(&B(2, i * BLOCK));
            // b_3i_neon = vld1q_f32(&B(3, i * BLOCK));

            c_10_neon = vfmaq_f32(c_10_neon, a_1i_neon, b_0i_neon);
            c_11_neon = vfmaq_f32(c_11_neon, a_1i_neon, b_1i_neon);
            c_12_neon = vfmaq_f32(c_12_neon, a_1i_neon, b_2i_neon);
            c_13_neon = vfmaq_f32(c_13_neon, a_1i_neon, b_3i_neon);
        //}

        //for (i = 0; i < k_4; i++) {
            // third row
            a_2i_neon = vld1q_f32(&A(2, i * BLOCK));

            // b_0i_neon = vld1q_f32(&B(0, i * BLOCK));
            // b_1i_neon = vld1q_f32(&B(1, i * BLOCK));
            // b_2i_neon = vld1q_f32(&B(2, i * BLOCK));
            // b_3i_neon = vld1q_f32(&B(3, i * BLOCK));

            c_20_neon = vfmaq_f32(c_20_neon, a_2i_neon, b_0i_neon);
            c_21_neon = vfmaq_f32(c_21_neon, a_2i_neon, b_1i_neon);
            c_22_neon = vfmaq_f32(c_22_neon, a_2i_neon, b_2i_neon);
            c_23_neon = vfmaq_f32(c_23_neon, a_2i_neon, b_3i_neon);
        //}

        //for (i = 0; i < k_4; i++) {
            // four row
            a_3i_neon = vld1q_f32(&A(3, i * BLOCK));

            // b_0i_neon = vld1q_f32(&B(0, i * BLOCK));
            // b_1i_neon = vld1q_f32(&B(1, i * BLOCK));
            // b_2i_neon = vld1q_f32(&B(2, i * BLOCK));
            // b_3i_neon = vld1q_f32(&B(3, i * BLOCK));

            c_30_neon = vfmaq_f32(c_30_neon, a_3i_neon, b_0i_neon);
            c_31_neon = vfmaq_f32(c_31_neon, a_3i_neon, b_1i_neon);
            c_32_neon = vfmaq_f32(c_32_neon, a_3i_neon, b_2i_neon);
            c_33_neon = vfmaq_f32(c_33_neon, a_3i_neon, b_3i_neon);
        }

        C(0, 0) += vgetq_lane_f32(c_00_neon, 0) + vgetq_lane_f32(c_00_neon, 1) +
                   vgetq_lane_f32(c_00_neon, 2) + vgetq_lane_f32(c_00_neon, 3);
        C(0, 1) += vgetq_lane_f32(c_01_neon, 0) + vgetq_lane_f32(c_01_neon, 1) +
                   vgetq_lane_f32(c_01_neon, 2) + vgetq_lane_f32(c_01_neon, 3);
        C(0, 2) += vgetq_lane_f32(c_02_neon, 0) + vgetq_lane_f32(c_02_neon, 1) +
                   vgetq_lane_f32(c_02_neon, 2) + vgetq_lane_f32(c_02_neon, 3);
        C(0, 3) += vgetq_lane_f32(c_03_neon, 0) + vgetq_lane_f32(c_03_neon, 1) +
                   vgetq_lane_f32(c_03_neon, 2) + vgetq_lane_f32(c_03_neon, 3);

        C(1, 0) += vgetq_lane_f32(c_10_neon, 0) + vgetq_lane_f32(c_10_neon, 1) +
                   vgetq_lane_f32(c_10_neon, 2) + vgetq_lane_f32(c_10_neon, 3);
        C(1, 1) += vgetq_lane_f32(c_11_neon, 0) + vgetq_lane_f32(c_11_neon, 1) +
                   vgetq_lane_f32(c_11_neon, 2) + vgetq_lane_f32(c_11_neon, 3);
        C(1, 2) += vgetq_lane_f32(c_12_neon, 0) + vgetq_lane_f32(c_12_neon, 1) +
                   vgetq_lane_f32(c_12_neon, 2) + vgetq_lane_f32(c_12_neon, 3);
        C(1, 3) += vgetq_lane_f32(c_13_neon, 0) + vgetq_lane_f32(c_13_neon, 1) +
                   vgetq_lane_f32(c_13_neon, 2) + vgetq_lane_f32(c_13_neon, 3);

        C(2, 0) += vgetq_lane_f32(c_20_neon, 0) + vgetq_lane_f32(c_20_neon, 1) +
                   vgetq_lane_f32(c_20_neon, 2) + vgetq_lane_f32(c_20_neon, 3);
        C(2, 1) += vgetq_lane_f32(c_21_neon, 0) + vgetq_lane_f32(c_21_neon, 1) +
                   vgetq_lane_f32(c_21_neon, 2) + vgetq_lane_f32(c_21_neon, 3);
        C(2, 2) += vgetq_lane_f32(c_22_neon, 0) + vgetq_lane_f32(c_22_neon, 1) +
                   vgetq_lane_f32(c_22_neon, 2) + vgetq_lane_f32(c_22_neon, 3);
        C(2, 3) += vgetq_lane_f32(c_23_neon, 0) + vgetq_lane_f32(c_23_neon, 1) +
                   vgetq_lane_f32(c_23_neon, 2) + vgetq_lane_f32(c_23_neon, 3);

        C(3, 0) += vgetq_lane_f32(c_30_neon, 0) + vgetq_lane_f32(c_30_neon, 1) +
                   vgetq_lane_f32(c_30_neon, 2) + vgetq_lane_f32(c_30_neon, 3);
        C(3, 1) += vgetq_lane_f32(c_31_neon, 0) + vgetq_lane_f32(c_31_neon, 1) +
                   vgetq_lane_f32(c_31_neon, 2) + vgetq_lane_f32(c_31_neon, 3);
        C(3, 2) += vgetq_lane_f32(c_32_neon, 0) + vgetq_lane_f32(c_32_neon, 1) +
                   vgetq_lane_f32(c_32_neon, 2) + vgetq_lane_f32(c_32_neon, 3);
        C(3, 3) += vgetq_lane_f32(c_33_neon, 0) + vgetq_lane_f32(c_33_neon, 1) +
                   vgetq_lane_f32(c_33_neon, 2) + vgetq_lane_f32(c_33_neon, 3);
    }

    /* when k is not a multiple of four, k_1 that is the remainder of four need
       to be calculate */
    if (k_1) {
        // the offset of k_4
        int offset;
        float
            /* C( 0, 0 ), C( 0, 1 ), C( 0, 2 ), C( 0, 3 )
            C( 1, 0 ), C( 1, 1 ), C( 1, 2 ), C( 1, 3 )
            C( 2, 0 ), C( 2, 1 ), C( 2, 2 ), C( 2, 3 )
            C( 3, 0 ), C( 3, 1 ), C( 3, 2 ), C( 3, 3 ) */
            c_00,
            c_01, c_02, c_03, c_10, c_11, c_12, c_13, c_20, c_21, c_22, c_23,
            c_30, c_31, c_32, c_33,
            /* Point to the current elements in the four rows of A */
            *a_0i_pntr, *a_1i_pntr, *a_2i_pntr, *a_3i_pntr,
            /* Point to the current elements in the four rows of B */
            *b_0i_pntr, *b_1i_pntr, *b_2i_pntr, *b_3i_pntr;

        offset = k_4 * 4;

        c_00 = 0.0;
        c_01 = 0.0;
        c_02 = 0.0;
        c_03 = 0.0;
        c_10 = 0.0;
        c_11 = 0.0;
        c_12 = 0.0;
        c_13 = 0.0;
        c_20 = 0.0;
        c_21 = 0.0;
        c_22 = 0.0;
        c_23 = 0.0;
        c_30 = 0.0;
        c_31 = 0.0;
        c_32 = 0.0;
        c_33 = 0.0;

        a_0i_pntr = &A(0, 0) + offset;
        a_1i_pntr = &A(1, 0) + offset;
        a_2i_pntr = &A(2, 0) + offset;
        a_3i_pntr = &A(3, 0) + offset;

        b_0i_pntr = &B(0, 0) + offset;
        b_1i_pntr = &B(1, 0) + offset;
        b_2i_pntr = &B(2, 0) + offset;
        b_3i_pntr = &B(3, 0) + offset;

        for (i = 0; i < k_1; i++) {
            // first row
            c_00 += *a_0i_pntr * *b_0i_pntr;
            c_01 += *a_0i_pntr * *b_1i_pntr;
            c_02 += *a_0i_pntr * *b_2i_pntr;
            c_03 += *a_0i_pntr * *b_3i_pntr;

            // second row
            c_10 += *a_1i_pntr * *b_0i_pntr;
            c_11 += *a_1i_pntr * *b_1i_pntr;
            c_12 += *a_1i_pntr * *b_2i_pntr;
            c_13 += *a_1i_pntr * *b_3i_pntr;

            // third row
            c_20 += *a_2i_pntr * *b_0i_pntr;
            c_21 += *a_2i_pntr * *b_1i_pntr;
            c_22 += *a_2i_pntr * *b_2i_pntr;
            c_23 += *a_2i_pntr * *b_3i_pntr;

            // four row
            c_30 += *a_3i_pntr * *b_0i_pntr;
            c_31 += *a_3i_pntr * *b_1i_pntr;
            c_32 += *a_3i_pntr * *b_2i_pntr;
            c_33 += *a_3i_pntr * *b_3i_pntr;

            a_0i_pntr++;
            a_1i_pntr++;
            a_2i_pntr++;
            a_3i_pntr++;
            b_0i_pntr++;
            b_1i_pntr++;
            b_2i_pntr++;
            b_3i_pntr++;
        }

        C(0, 0) += c_00;
        C(0, 1) += c_01;
        C(0, 2) += c_02;
        C(0, 3) += c_03;

        C(1, 0) += c_10;
        C(1, 1) += c_11;
        C(1, 2) += c_12;
        C(1, 3) += c_13;

        C(2, 0) += c_20;
        C(2, 1) += c_21;
        C(2, 2) += c_22;
        C(2, 3) += c_23;

        C(3, 0) += c_30;
        C(3, 1) += c_31;
        C(3, 2) += c_32;
        C(3, 3) += c_33;
    }
}

void GemmNeonAxpy4x4_2(int K, float *A, int lda, float *B, int ldb, float *C,
                     int ldc) {
    // loop parameters
    int i;
    // k_4 and n_1 is the multiple of four and one
    int k_4, k_1;
    // Neon values
    float32x4_t
        /* C( 0, 0 ), C( 0, 1 ), C( 0, 2 ), C( 0, 3 )
           C( 1, 0 ), C( 1, 1 ), C( 1, 2 ), C( 1, 3 )
           C( 2, 0 ), C( 2, 1 ), C( 2, 2 ), C( 2, 3 )
           C( 3, 0 ), C( 3, 1 ), C( 3, 2 ), C( 3, 3 ) */
        c_00_neon,
        c_01_neon, c_02_neon, c_03_neon, c_10_neon, c_11_neon, c_12_neon,
        c_13_neon, c_20_neon, c_21_neon, c_22_neon, c_23_neon, c_30_neon,
        c_31_neon, c_32_neon, c_33_neon,
        /* A( 0, i ), A( 0, i + 1 ), A( 0, i + 2 ), A( 0, i + 3 ) */
        a_0i_neon,
        /* A( 1, i ), A( 1, i + 1 ), A( 1, i + 2 ), A( 1, i + 3 ) */
        a_1i_neon,
        /* A( 2, i ), A( 2, i + 1 ), A( 2, i + 2 ), A( 2, i + 3 ) */
        a_2i_neon,
        /* A( 3, i ), A( 3, i + 1 ), A( 3, i + 2 ), A( 3, i + 3 ) */
        a_3i_neon,
        /* B( 0, i ), B( 0, i + 1 ), B( 0, i + 2 ), B( 0, i + 3 ) */
        b_0i_neon,
        /* B( 1, i ), B( 1, i + 1 ), B( 1, i + 2 ), B( 1, i + 3 ) */
        b_1i_neon,
        /* B( 2, i ), B( 2, i + 1 ), B( 2, i + 2 ), B( 2, i + 3 ) */
        b_2i_neon,
        /* B( 3, i ), B( 3, i + 1 ), B( 3, i + 2 ), B( 3, i + 3 ) */
        b_3i_neon;

    k_4 = K / BLOCK;
    k_1 = K - 4 * k_4;

    // if out neon register not load zero, the result is error
    c_00_neon = vmovq_n_f32(0);
    c_01_neon = vmovq_n_f32(0);
    c_02_neon = vmovq_n_f32(0);
    c_03_neon = vmovq_n_f32(0);
    c_10_neon = vmovq_n_f32(0);
    c_11_neon = vmovq_n_f32(0);
    c_12_neon = vmovq_n_f32(0);
    c_13_neon = vmovq_n_f32(0);
    c_20_neon = vmovq_n_f32(0);
    c_21_neon = vmovq_n_f32(0);
    c_22_neon = vmovq_n_f32(0);
    c_23_neon = vmovq_n_f32(0);
    c_30_neon = vmovq_n_f32(0);
    c_31_neon = vmovq_n_f32(0);
    c_32_neon = vmovq_n_f32(0);
    c_33_neon = vmovq_n_f32(0);
    //float *pB = B;
    float *pA = A;
    if (k_4) {
        /* the loop-for is divided into four because cache line if the size of
           64 bytes(The possible reason needs to be confirmed later) */
        for (i = 0; i < k_4; i++, pA += 16) {
            // first row
            a_0i_neon = vld1q_f32(pA);
            b_0i_neon = vld1q_f32(&B(0, i * BLOCK));
            b_1i_neon = vld1q_f32(&B(1, i * BLOCK));
            b_2i_neon = vld1q_f32(&B(2, i * BLOCK));
            b_3i_neon = vld1q_f32(&B(3, i * BLOCK));
            // b_0i_neon = vld1q_f32(pB);
            // b_1i_neon = vld1q_f32(pB+4);
            // b_2i_neon = vld1q_f32(pB+8);
            // b_3i_neon = vld1q_f32(pB+12);

            c_00_neon = vfmaq_f32(c_00_neon, a_0i_neon, b_0i_neon);
            c_01_neon = vfmaq_f32(c_01_neon, a_0i_neon, b_1i_neon);
            c_02_neon = vfmaq_f32(c_02_neon, a_0i_neon, b_2i_neon);
            c_03_neon = vfmaq_f32(c_03_neon, a_0i_neon, b_3i_neon);

            // second row
            a_1i_neon = vld1q_f32(pA+4);

            c_10_neon = vfmaq_f32(c_10_neon, a_1i_neon, b_0i_neon);
            c_11_neon = vfmaq_f32(c_11_neon, a_1i_neon, b_1i_neon);
            c_12_neon = vfmaq_f32(c_12_neon, a_1i_neon, b_2i_neon);
            c_13_neon = vfmaq_f32(c_13_neon, a_1i_neon, b_3i_neon);

            // third row
            a_2i_neon = vld1q_f32(pA+8);

            c_20_neon = vfmaq_f32(c_20_neon, a_2i_neon, b_0i_neon);
            c_21_neon = vfmaq_f32(c_21_neon, a_2i_neon, b_1i_neon);
            c_22_neon = vfmaq_f32(c_22_neon, a_2i_neon, b_2i_neon);
            c_23_neon = vfmaq_f32(c_23_neon, a_2i_neon, b_3i_neon);

            // four row
            a_3i_neon = vld1q_f32(pA+12);

            c_30_neon = vfmaq_f32(c_30_neon, a_3i_neon, b_0i_neon);
            c_31_neon = vfmaq_f32(c_31_neon, a_3i_neon, b_1i_neon);
            c_32_neon = vfmaq_f32(c_32_neon, a_3i_neon, b_2i_neon);
            c_33_neon = vfmaq_f32(c_33_neon, a_3i_neon, b_3i_neon);
        }

        C(0, 0) += vgetq_lane_f32(c_00_neon, 0) + vgetq_lane_f32(c_00_neon, 1) +
                   vgetq_lane_f32(c_00_neon, 2) + vgetq_lane_f32(c_00_neon, 3);
        C(0, 1) += vgetq_lane_f32(c_01_neon, 0) + vgetq_lane_f32(c_01_neon, 1) +
                   vgetq_lane_f32(c_01_neon, 2) + vgetq_lane_f32(c_01_neon, 3);
        C(0, 2) += vgetq_lane_f32(c_02_neon, 0) + vgetq_lane_f32(c_02_neon, 1) +
                   vgetq_lane_f32(c_02_neon, 2) + vgetq_lane_f32(c_02_neon, 3);
        C(0, 3) += vgetq_lane_f32(c_03_neon, 0) + vgetq_lane_f32(c_03_neon, 1) +
                   vgetq_lane_f32(c_03_neon, 2) + vgetq_lane_f32(c_03_neon, 3);

        C(1, 0) += vgetq_lane_f32(c_10_neon, 0) + vgetq_lane_f32(c_10_neon, 1) +
                   vgetq_lane_f32(c_10_neon, 2) + vgetq_lane_f32(c_10_neon, 3);
        C(1, 1) += vgetq_lane_f32(c_11_neon, 0) + vgetq_lane_f32(c_11_neon, 1) +
                   vgetq_lane_f32(c_11_neon, 2) + vgetq_lane_f32(c_11_neon, 3);
        C(1, 2) += vgetq_lane_f32(c_12_neon, 0) + vgetq_lane_f32(c_12_neon, 1) +
                   vgetq_lane_f32(c_12_neon, 2) + vgetq_lane_f32(c_12_neon, 3);
        C(1, 3) += vgetq_lane_f32(c_13_neon, 0) + vgetq_lane_f32(c_13_neon, 1) +
                   vgetq_lane_f32(c_13_neon, 2) + vgetq_lane_f32(c_13_neon, 3);

        C(2, 0) += vgetq_lane_f32(c_20_neon, 0) + vgetq_lane_f32(c_20_neon, 1) +
                   vgetq_lane_f32(c_20_neon, 2) + vgetq_lane_f32(c_20_neon, 3);
        C(2, 1) += vgetq_lane_f32(c_21_neon, 0) + vgetq_lane_f32(c_21_neon, 1) +
                   vgetq_lane_f32(c_21_neon, 2) + vgetq_lane_f32(c_21_neon, 3);
        C(2, 2) += vgetq_lane_f32(c_22_neon, 0) + vgetq_lane_f32(c_22_neon, 1) +
                   vgetq_lane_f32(c_22_neon, 2) + vgetq_lane_f32(c_22_neon, 3);
        C(2, 3) += vgetq_lane_f32(c_23_neon, 0) + vgetq_lane_f32(c_23_neon, 1) +
                   vgetq_lane_f32(c_23_neon, 2) + vgetq_lane_f32(c_23_neon, 3);

        C(3, 0) += vgetq_lane_f32(c_30_neon, 0) + vgetq_lane_f32(c_30_neon, 1) +
                   vgetq_lane_f32(c_30_neon, 2) + vgetq_lane_f32(c_30_neon, 3);
        C(3, 1) += vgetq_lane_f32(c_31_neon, 0) + vgetq_lane_f32(c_31_neon, 1) +
                   vgetq_lane_f32(c_31_neon, 2) + vgetq_lane_f32(c_31_neon, 3);
        C(3, 2) += vgetq_lane_f32(c_32_neon, 0) + vgetq_lane_f32(c_32_neon, 1) +
                   vgetq_lane_f32(c_32_neon, 2) + vgetq_lane_f32(c_32_neon, 3);
        C(3, 3) += vgetq_lane_f32(c_33_neon, 0) + vgetq_lane_f32(c_33_neon, 1) +
                   vgetq_lane_f32(c_33_neon, 2) + vgetq_lane_f32(c_33_neon, 3);
    }

    /* when k is not a multiple of four, k_1 that is the remainder of four need
       to be calculate */
    if (k_1) {
        // the offset of k_4
        int offset;
        //pB = B + k_4 * 16;
        pA = A + k_4 * 16;
        float
            /* C( 0, 0 ), C( 0, 1 ), C( 0, 2 ), C( 0, 3 )
            C( 1, 0 ), C( 1, 1 ), C( 1, 2 ), C( 1, 3 )
            C( 2, 0 ), C( 2, 1 ), C( 2, 2 ), C( 2, 3 )
            C( 3, 0 ), C( 3, 1 ), C( 3, 2 ), C( 3, 3 ) */
            c_00,
            c_01, c_02, c_03, c_10, c_11, c_12, c_13, c_20, c_21, c_22, c_23,
            c_30, c_31, c_32, c_33,
            /* Point to the current elements in the four rows of A */
            *a_0i_pntr, *a_1i_pntr, *a_2i_pntr, *a_3i_pntr,
            /* Point to the current elements in the four rows of B */
            *b_0i_pntr, *b_1i_pntr, *b_2i_pntr, *b_3i_pntr;

        offset = k_4 * 4;

        c_00 = 0.0;
        c_01 = 0.0;
        c_02 = 0.0;
        c_03 = 0.0;
        c_10 = 0.0;
        c_11 = 0.0;
        c_12 = 0.0;
        c_13 = 0.0;
        c_20 = 0.0;
        c_21 = 0.0;
        c_22 = 0.0;
        c_23 = 0.0;
        c_30 = 0.0;
        c_31 = 0.0;
        c_32 = 0.0;
        c_33 = 0.0;

        // a_0i_pntr = &A(0, 0) + offset;
        // a_1i_pntr = &A(1, 0) + offset;
        // a_2i_pntr = &A(2, 0) + offset;
        // a_3i_pntr = &A(3, 0) + offset;
        a_0i_pntr = pA;
        a_1i_pntr = pA + k_1;
        a_2i_pntr = pA + k_1 * 2;
        a_3i_pntr = pA + k_1 * 3;
        // b_0i_pntr = pB;
        // b_1i_pntr = pB + k_1;
        // b_2i_pntr = pB + k_1 * 2;
        // b_3i_pntr = pB + k_1 * 3;
        b_0i_pntr = &B(0, 0) + offset;
        b_1i_pntr = &B(1, 0) + offset;
        b_2i_pntr = &B(2, 0) + offset;
        b_3i_pntr = &B(3, 0) + offset;        

        for (i = 0; i < k_1; i++) {
            // first row
            c_00 += *a_0i_pntr * *b_0i_pntr;
            c_01 += *a_0i_pntr * *b_1i_pntr;
            c_02 += *a_0i_pntr * *b_2i_pntr;
            c_03 += *a_0i_pntr * *b_3i_pntr;

            // second row
            c_10 += *a_1i_pntr * *b_0i_pntr;
            c_11 += *a_1i_pntr * *b_1i_pntr;
            c_12 += *a_1i_pntr * *b_2i_pntr;
            c_13 += *a_1i_pntr * *b_3i_pntr;

            // third row
            c_20 += *a_2i_pntr * *b_0i_pntr;
            c_21 += *a_2i_pntr * *b_1i_pntr;
            c_22 += *a_2i_pntr * *b_2i_pntr;
            c_23 += *a_2i_pntr * *b_3i_pntr;

            // four row
            c_30 += *a_3i_pntr * *b_0i_pntr;
            c_31 += *a_3i_pntr * *b_1i_pntr;
            c_32 += *a_3i_pntr * *b_2i_pntr;
            c_33 += *a_3i_pntr * *b_3i_pntr;

            a_0i_pntr++;
            a_1i_pntr++;
            a_2i_pntr++;
            a_3i_pntr++;
            b_0i_pntr++;
            b_1i_pntr++;
            b_2i_pntr++;
            b_3i_pntr++;
        }

        C(0, 0) += c_00;
        C(0, 1) += c_01;
        C(0, 2) += c_02;
        C(0, 3) += c_03;

        C(1, 0) += c_10;
        C(1, 1) += c_11;
        C(1, 2) += c_12;
        C(1, 3) += c_13;

        C(2, 0) += c_20;
        C(2, 1) += c_21;
        C(2, 2) += c_22;
        C(2, 3) += c_23;

        C(3, 0) += c_30;
        C(3, 1) += c_31;
        C(3, 2) += c_32;
        C(3, 3) += c_33;
    }
}

/* this routine computes a 4x4 block of matrix A

       C( 0, 0 ), C( 0, 1 ), C( 0, 2 ), C( 0, 3 ).
       C( 1, 0 ), C( 1, 1 ), C( 1, 2 ), C( 1, 3 ).
       C( 2, 0 ), C( 2, 1 ), C( 2, 2 ), C( 2, 3 ).
       C( 3, 0 ), C( 3, 1 ), C( 3, 2 ), C( 3, 3 ).

 Notice that this routine is called with c = C( i, j ) in the
 previous routine, so these are actually the elements

       C( i  , j ), C( i  , j+1 ), C( i  , j+2 ), C( i  , j+3 )
       C( i+1, j ), C( i+1, j+1 ), C( i+1, j+2 ), C( i+1, j+3 )
       C( i+2, j ), C( i+2, j+1 ), C( i+2, j+2 ), C( i+2, j+3 )
       C( i+3, j ), C( i+3, j+1 ), C( i+3, j+2 ), C( i+3, j+3 )

 in the original matrix C */
void NeonAxpy4x4(int k, float *a, int lda, float *b, int ldb, float *c,
                 int ldc) {
    // loop parameters
    int i;
    // k_4 and n_1 is the multiple of four and one
    int k_4, k_1;
    // Neon values
    float32x4_t
        /* C( 0, 0 ), C( 0, 1 ), C( 0, 2 ), C( 0, 3 )
           C( 1, 0 ), C( 1, 1 ), C( 1, 2 ), C( 1, 3 )
           C( 2, 0 ), C( 2, 1 ), C( 2, 2 ), C( 2, 3 )
           C( 3, 0 ), C( 3, 1 ), C( 3, 2 ), C( 3, 3 ) */
        c_00_neon,
        c_01_neon, c_02_neon, c_03_neon, c_10_neon, c_11_neon, c_12_neon,
        c_13_neon, c_20_neon, c_21_neon, c_22_neon, c_23_neon, c_30_neon,
        c_31_neon, c_32_neon, c_33_neon,
        /* A( 0, i ), A( 0, i + 1 ), A( 0, i + 2 ), A( 0, i + 3 ) */
        *a_0i_neon,
        /* A( 1, i ), A( 1, i + 1 ), A( 1, i + 2 ), A( 1, i + 3 ) */
        *a_1i_neon,
        /* A( 2, i ), A( 2, i + 1 ), A( 2, i + 2 ), A( 2, i + 3 ) */
        *a_2i_neon,
        /* A( 3, i ), A( 3, i + 1 ), A( 3, i + 2 ), A( 3, i + 3 ) */
        *a_3i_neon,
        /* B( 0, i ), B( 0, i + 1 ), B( 0, i + 2 ), B( 0, i + 3 ) */
        *b_0i_neon,
        /* B( 1, i ), B( 1, i + 1 ), B( 1, i + 2 ), B( 1, i + 3 ) */
        *b_1i_neon,
        /* B( 2, i ), B( 2, i + 1 ), B( 2, i + 2 ), B( 2, i + 3 ) */
        *b_2i_neon,
        /* B( 3, i ), B( 3, i + 1 ), B( 3, i + 2 ), B( 3, i + 3 ) */
        *b_3i_neon;

    k_4 = k >> 2;
    k_1 = k - 4 * k_4;

    // if out neon register not load zero, the result is error
    c_00_neon = vmovq_n_f32(0);
    c_01_neon = vmovq_n_f32(0);
    c_02_neon = vmovq_n_f32(0);
    c_03_neon = vmovq_n_f32(0);
    c_10_neon = vmovq_n_f32(0);
    c_11_neon = vmovq_n_f32(0);
    c_12_neon = vmovq_n_f32(0);
    c_13_neon = vmovq_n_f32(0);
    c_20_neon = vmovq_n_f32(0);
    c_21_neon = vmovq_n_f32(0);
    c_22_neon = vmovq_n_f32(0);
    c_23_neon = vmovq_n_f32(0);
    c_30_neon = vmovq_n_f32(0);
    c_31_neon = vmovq_n_f32(0);
    c_32_neon = vmovq_n_f32(0);
    c_33_neon = vmovq_n_f32(0);

    // Forced type conversion instead of loading to neon register to speed up
    a_0i_neon = (float32x4_t *)a;
    a_1i_neon = (float32x4_t *)(a + lda);
    a_2i_neon = (float32x4_t *)(a + 2 * lda);
    a_3i_neon = (float32x4_t *)(a + 3 * lda);

    b_0i_neon = (float32x4_t *)b;
    b_1i_neon = (float32x4_t *)(b + ldb);
    b_2i_neon = (float32x4_t *)(b + 2 * ldb);
    b_3i_neon = (float32x4_t *)(b + 3 * ldb);

    if (k_4) {
        /* the loop-for is divided into four because cache line if the size of
           64 bytes(The possible reason needs to be confirmed later) */
        for (i = 0; i < k_4; i++) {
            // first row
            c_00_neon = vfmaq_f32(c_00_neon, a_0i_neon[i], b_0i_neon[i]);
            c_01_neon = vfmaq_f32(c_01_neon, a_0i_neon[i], b_1i_neon[i]);
            c_02_neon = vfmaq_f32(c_02_neon, a_0i_neon[i], b_2i_neon[i]);
            c_03_neon = vfmaq_f32(c_03_neon, a_0i_neon[i], b_3i_neon[i]);
        }

        for (i = 0; i < k_4; i++) {
            // second row
            c_10_neon = vfmaq_f32(c_10_neon, a_1i_neon[i], b_0i_neon[i]);
            c_11_neon = vfmaq_f32(c_11_neon, a_1i_neon[i], b_1i_neon[i]);
            c_12_neon = vfmaq_f32(c_12_neon, a_1i_neon[i], b_2i_neon[i]);
            c_13_neon = vfmaq_f32(c_13_neon, a_1i_neon[i], b_3i_neon[i]);
        }

        for (i = 0; i < k_4; i++) {
            // third row
            c_20_neon = vfmaq_f32(c_20_neon, a_2i_neon[i], b_0i_neon[i]);
            c_21_neon = vfmaq_f32(c_21_neon, a_2i_neon[i], b_1i_neon[i]);
            c_22_neon = vfmaq_f32(c_22_neon, a_2i_neon[i], b_2i_neon[i]);
            c_23_neon = vfmaq_f32(c_23_neon, a_2i_neon[i], b_3i_neon[i]);
        }

        for (i = 0; i < k_4; i++) {
            // four row
            c_30_neon = vfmaq_f32(c_30_neon, a_3i_neon[i], b_0i_neon[i]);
            c_31_neon = vfmaq_f32(c_31_neon, a_3i_neon[i], b_1i_neon[i]);
            c_32_neon = vfmaq_f32(c_32_neon, a_3i_neon[i], b_2i_neon[i]);
            c_33_neon = vfmaq_f32(c_33_neon, a_3i_neon[i], b_3i_neon[i]);
        }

        /* Forced type conversion for the pointer instead of neon API
           (eg:vgetq_lane_f32) */
        float *neon_values;// = NULL;

        neon_values = (float *)(&c_00_neon);
        *c = neon_values[0] + neon_values[1] + neon_values[2] + neon_values[3];
        neon_values = (float *)(&c_01_neon);
        *(c + 1) =
            neon_values[0] + neon_values[1] + neon_values[2] + neon_values[3];
        neon_values = (float *)(&c_02_neon);
        *(c + 2) =
            neon_values[0] + neon_values[1] + neon_values[2] + neon_values[3];
        neon_values = (float *)(&c_03_neon);
        *(c + 3) =
            neon_values[0] + neon_values[1] + neon_values[2] + neon_values[3];

        neon_values = (float *)(&c_10_neon);
        *(c + ldc) =
            neon_values[0] + neon_values[1] + neon_values[2] + neon_values[3];
        neon_values = (float *)(&c_11_neon);
        *(c + 1 + ldc) =
            neon_values[0] + neon_values[1] + neon_values[2] + neon_values[3];
        neon_values = (float *)(&c_12_neon);
        *(c + 2 + ldc) =
            neon_values[0] + neon_values[1] + neon_values[2] + neon_values[3];
        neon_values = (float *)(&c_13_neon);
        *(c + 3 + ldc) =
            neon_values[0] + neon_values[1] + neon_values[2] + neon_values[3];

        neon_values = (float *)(&c_20_neon);
        *(c + 2 * ldc) =
            neon_values[0] + neon_values[1] + neon_values[2] + neon_values[3];
        neon_values = (float *)(&c_21_neon);
        *(c + 1 + 2 * ldc) =
            neon_values[0] + neon_values[1] + neon_values[2] + neon_values[3];
        neon_values = (float *)(&c_22_neon);
        *(c + 2 + 2 * ldc) =
            neon_values[0] + neon_values[1] + neon_values[2] + neon_values[3];
        neon_values = (float *)(&c_23_neon);
        *(c + 3 + 2 * ldc) =
            neon_values[0] + neon_values[1] + neon_values[2] + neon_values[3];

        neon_values = (float *)(&c_30_neon);
        *(c + 3 * ldc) =
            neon_values[0] + neon_values[1] + neon_values[2] + neon_values[3];
        neon_values = (float *)(&c_31_neon);
        *(c + 1 + 3 * ldc) =
            neon_values[0] + neon_values[1] + neon_values[2] + neon_values[3];
        neon_values = (float *)(&c_32_neon);
        *(c + 2 + 3 * ldc) =
            neon_values[0] + neon_values[1] + neon_values[2] + neon_values[3];
        neon_values = (float *)(&c_33_neon);
        *(c + 3 + 3 * ldc) =
            neon_values[0] + neon_values[1] + neon_values[2] + neon_values[3];
    }

    /* when k is not a multiple of four, k_1 that is the remainder of four need
       to be calculate */
    if (k_1) {
        // the offset of k_4
        int offset;
        float
            /* C( 0, 0 ), C( 0, 1 ), C( 0, 2 ), C( 0, 3 )
            C( 1, 0 ), C( 1, 1 ), C( 1, 2 ), C( 1, 3 )
            C( 2, 0 ), C( 2, 1 ), C( 2, 2 ), C( 2, 3 )
            C( 3, 0 ), C( 3, 1 ), C( 3, 2 ), C( 3, 3 ) */
            c_00,
            c_01, c_02, c_03, c_10, c_11, c_12, c_13, c_20, c_21, c_22, c_23,
            c_30, c_31, c_32, c_33,
            /* Point to the current elements in the four rows of A */
            *a_0i_pntr, *a_1i_pntr, *a_2i_pntr, *a_3i_pntr,
            /* Point to the current elements in the four rows of B */
            *b_0i_pntr, *b_1i_pntr, *b_2i_pntr, *b_3i_pntr;

        offset = k_4 * 4;

        a_0i_pntr = a + offset;
        a_1i_pntr = a + lda + offset;
        a_2i_pntr = a + 2 * lda + offset;
        a_3i_pntr = a + 3 * lda + offset;

        b_0i_pntr = b + offset;
        b_1i_pntr = b + ldb + offset;
        b_2i_pntr = b + 2 * ldb + offset;
        b_3i_pntr = b + 3 * ldb + offset;

        for (i = 0; i < k_1; i++) {
            // first row
            c_00 += *a_0i_pntr * *b_0i_pntr;
            c_01 += *a_0i_pntr * *b_1i_pntr;
            c_02 += *a_0i_pntr * *b_2i_pntr;
            c_03 += *a_0i_pntr * *b_3i_pntr;

            // second row
            c_10 += *a_1i_pntr * *b_0i_pntr;
            c_11 += *a_1i_pntr * *b_1i_pntr;
            c_12 += *a_1i_pntr * *b_2i_pntr;
            c_13 += *a_1i_pntr * *b_3i_pntr;

            // third row
            c_20 += *a_2i_pntr * *b_0i_pntr;
            c_21 += *a_2i_pntr * *b_1i_pntr;
            c_22 += *a_2i_pntr * *b_2i_pntr;
            c_23 += *a_2i_pntr * *b_3i_pntr;

            // four row
            c_30 += *a_3i_pntr * *b_0i_pntr;
            c_31 += *a_3i_pntr * *b_1i_pntr;
            c_32 += *a_3i_pntr * *b_2i_pntr;
            c_33 += *a_3i_pntr * *b_3i_pntr;

            a_0i_pntr++;
            a_1i_pntr++;
            a_2i_pntr++;
            a_3i_pntr++;
            b_0i_pntr++;
            b_1i_pntr++;
            b_2i_pntr++;
            b_3i_pntr++;
        }

        *c += c_00;
        *(c + 1) += c_01;
        *(c + 2) += c_02;
        *(c + 3) += c_03;

        *(c + ldc) += c_10;
        *(c + 1 + ldc) += c_11;
        *(c + 2 + ldc) += c_12;
        *(c + 3 + ldc) += c_13;

        *(c + 2 * ldc) += c_20;
        *(c + 1 + 2 * ldc) += c_21;
        *(c + 2 + 2 * ldc) += c_22;
        *(c + 3 + 2 * ldc) += c_23;

        *(c + 3 * ldc) += c_30;
        *(c + 1 + 3 * ldc) += c_31;
        *(c + 2 + 3 * ldc) += c_32;
        *(c + 3 + 3 * ldc) += c_33;
    }
}
#endif

/* So, this routine computes a 4x4 block of matrix A

       C( 0, 0 ), C( 0, 1 ), C( 0, 2 ), C( 0, 3 ).
       C( 1, 0 ), C( 1, 1 ), C( 1, 2 ), C( 1, 3 ).
       C( 2, 0 ), C( 2, 1 ), C( 2, 2 ), C( 2, 3 ).
       C( 3, 0 ), C( 3, 1 ), C( 3, 2 ), C( 3, 3 ).

 Notice that this routine is called with c = C( i, j ) in the
 previous routine, so these are actually the elements

       C( i  , j ), C( i  , j+1 ), C( i  , j+2 ), C( i  , j+3 )
       C( i+1, j ), C( i+1, j+1 ), C( i+1, j+2 ), C( i+1, j+3 )
       C( i+2, j ), C( i+2, j+1 ), C( i+2, j+2 ), C( i+2, j+3 )
       C( i+3, j ), C( i+3, j+1 ), C( i+3, j+2 ), C( i+3, j+3 )

 in the original matrix C */
void Axpy4x4(int k, float *a, int lda, float *b, int ldb, float *c, int ldc) {
    // loop parameters
    int i;
    // Point to the address at the beginning of the rows
    float
        /* Point to the current elements in the four rows of A */
        *a_0i_pntr,
        *a_1i_pntr, *a_2i_pntr, *a_3i_pntr,
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
    a_1i_pntr = a + lda;
    a_2i_pntr = a + 2 * lda;
    a_3i_pntr = a + 3 * lda;

    b_0i_pntr = b;
    b_1i_pntr = b + ldb;
    b_2i_pntr = b + 2 * ldb;
    b_3i_pntr = b + 3 * ldb;

    for (i = 0; i < k; i++) {
        a_0i_reg = *a_0i_pntr++;
        a_1i_reg = *a_1i_pntr++;
        a_2i_reg = *a_2i_pntr++;
        a_3i_reg = *a_3i_pntr++;

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
void Axpy1x4(int k, float *a, int lda, float *b, int ldb, float *c, int ldc) {
    // loop parameters
    int i;
    // Point to the address at the beginning of the rows
    float
        /* Point to the current elements in the one rows of A */
        *a_0i_pntr,
        /* Point to the current elements in the four rows of B */
        *b_0i_pntr, *b_1i_pntr, *b_2i_pntr, *b_3i_pntr;
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

    for (i = 0; i < k; i++) {
        a_0i_reg = *a_0i_pntr++;

        b_0i_reg = *b_0i_pntr++;
        b_1i_reg = *b_1i_pntr++;
        b_2i_reg = *b_2i_pntr++;
        b_3i_reg = *b_3i_pntr++;

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
void Axpy4x1(int k, float *a, int lda, float *b, int ldb, float *c, int ldc) {
    // loop parameters
    int i;
    // Point to the address at the beginning of the rows
    float
        /* Point to the current elements in the four rows of A */
        *a_0i_pntr,
        *a_1i_pntr, *a_2i_pntr, *a_3i_pntr,
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
    a_1i_pntr = a + lda;
    a_2i_pntr = a + 2 * lda;
    a_3i_pntr = a + 3 * lda;

    b_0i_pntr = b;

    for (i = 0; i < k; i++) {
        a_0i_reg = *a_0i_pntr++;
        a_1i_reg = *a_1i_pntr++;
        a_2i_reg = *a_2i_pntr++;
        a_3i_reg = *a_3i_pntr++;

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
void Axpy1x1(int k, float *a, int lda, float *b, int ldb, float *c, int ldc) {
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
void matrix_mul(int m, int n, int k, float *a, int lda, float *b, int ldb,
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
    if (m_4 && n_4) {
        for (j = 0; j < n_4;
             j += 4) { // loop over the columns of c, unrolled by 4
            B = (b) + j * ldb;
            for (i = 0; i < m_4;
                 i += 4) { // loop over the rows of c, unrolled by 4
                A = (a) + i * lda;
                C = (c) + i * ldc + j;
#ifdef USE_NEON
                GemmNeonAxpy4x4(k, A, lda, B, ldb, C, ldc);
                // NeonAxpy4x4(k, A, lda, B, ldb, C, ldc);
#else
                Axpy4x4(k, A, lda, B, ldb, C, ldc);
#endif
            }
        }
    }
    //print_float(c, m * n);
    if (m_1 && n_4) {
        for (j = 0; j < n_4;
             j += 4) { // loop over the columns of c, unrolled by 4
            B = (b) + j * ldb;
            for (i = 0; i < m_1; i++) { // loop over the rows of c, unrolled by
                                        // 1
                A = (a + m_4 * lda) + i * lda;
                C = (c + m_4 * ldc) + i * ldc + j;
                Axpy1x4(k, A, lda, B, ldb, C, ldc);
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
                Axpy4x1(k, A, lda, B, ldb, C, ldc);
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
                Axpy1x1(k, A, lda, B, ldb, C, ldc);
            }
        }
    }
}


static void gemm_nn_(int M, int N, int K, float ALPHA, float *A, int lda,
                     float *B, int ldb, float *C, int ldc) {
    int i, j, k;
    for (i = 0; i < M; ++i) {
        for (k = 0; k < K; ++k) {
            register float A_PART = ALPHA * A[i * lda + k];
            for (j = 0; j < N; ++j) {
                C[i * ldc + j] += A_PART * B[k * ldb + j];
            }
        }
    }
}

static void gemm_nt_(int M, int N, int K, float ALPHA, float *A, int lda,
                     float *B, int ldb, float *C, int ldc) {
    int i, j, k;
    for (i = 0; i < M; ++i) {
        for (j = 0; j < N; ++j) {
            register float sum = 0;
            for (k = 0; k < K; ++k) {
                sum += ALPHA * A[i * lda + k] * B[j * ldb + k];
            }
            C[i * ldc + j] += sum;
        }
    }
}

static void gemm_tn_(int M, int N, int K, float ALPHA, float *A, int lda,
                     float *B, int ldb, float *C, int ldc) {
    int i, j, k;
    for (i = 0; i < M; ++i) {
        for (k = 0; k < K; ++k) {
            register float A_PART = ALPHA * A[k * lda + i];
            for (j = 0; j < N; ++j) {
                C[i * ldc + j] += A_PART * B[k * ldb + j];
            }
        }
    }
}

static void gemm_tt_(int M, int N, int K, float ALPHA, float *A, int lda,
                     float *B, int ldb, float *C, int ldc) {
    int i, j, k;
    for (i = 0; i < M; ++i) {
        for (j = 0; j < N; ++j) {
            register float sum = 0;
            for (k = 0; k < K; ++k) {
                sum += ALPHA * A[i + k * lda] * B[k + j * ldb];
            }
            C[i * ldc + j] += sum;
        }
    }
}

void qtk_sgemm(int TA, int TB, int M, int N, int K, float ALPHA, float *A,
               int lda, float *B, int ldb, float BETA, float *C, int ldc) {
    int i, j;
    for (i = 0; i < M; i++) {
        for (j = 0; j < N; j++) {
            C[i * ldc + j] *= BETA;
        }
    }

    if (!TA && !TB) {
#ifdef USE_NEON
        gemm_nn_neon(M, N, K, ALPHA, A, lda, B, ldb, C, ldc);
#else
        gemm_nn_(M, N, K, ALPHA, A, lda, B, ldb, C, ldc);
#endif
    } else if (TA && !TB) {
#ifdef USE_NEON
        gemm_tn_neon(M, N, K, ALPHA, A, lda, B, ldb, C, ldc);
#else
        gemm_tn_(M, N, K, ALPHA, A, lda, B, ldb, C, ldc);
#endif
    } else if (!TA && TB) {
        if(ALPHA == 1.0){
            matrix_mul(M,N,K,A,lda,B,ldb,C,ldc);
        }else{
            gemm_nt_(M, N, K, ALPHA, A, lda, B, ldb, C, ldc);
        }

    } else {
        gemm_tt_(M, N, K, ALPHA, A, lda, B, ldb, C, ldc);
    }
}
