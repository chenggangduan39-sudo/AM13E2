#ifndef __QBL_MATH_QBL_VECTOR_H__
#define __QBL_MATH_QBL_VECTOR_H__
#pragma once
#include "qtk/core/qtk_type.h"
#include "qtk/math/qtk_math.h"
#include "wtk/core/wtk_complex.h"
#ifdef USE_AVX2
#include <immintrin.h>
#endif
#ifdef QTK_USE_NEON
#include <arm_neon.h>
typedef float32x4_t f32x4_t;
#endif
#ifdef __cplusplus
extern "C" {
#endif

#ifdef USE_AVX2
static inline float hsum_ps_sse3(__m128 v) {
    __m128 shuf = _mm_movehdup_ps(v);
    __m128 sums = _mm_add_ps(v, shuf);
    shuf = _mm_movehl_ps(shuf, sums);
    sums = _mm_add_ss(sums, shuf);
    return _mm_cvtss_f32(sums);
}

static inline float hsum256_ps_avx(__m256 v) {
    __m128 vlow = _mm256_castps256_ps128(v);
    __m128 vhigh = _mm256_extractf128_ps(v, 1);
    vlow = _mm_add_ps(vlow, vhigh);
    return hsum_ps_sse3(vlow);
}
#endif

/* multiply–accumulate operation */
QTK_INLINE static float qtk_vector_dotf(float *a, float *b, int len) {
    float accum = 0;
#ifdef USE_AVX2
    int cnt, c;

    __m256 sum = _mm256_setzero_ps();
    __m256 sum1 = _mm256_setzero_ps();
    __m256 sum2 = _mm256_setzero_ps();
    __m256 sum3 = _mm256_setzero_ps();

    cnt = len / 8;
    c = cnt / 4;
    while (c-- > 0) {
        __m256 x = _mm256_loadu_ps(a);
        __m256 y = _mm256_loadu_ps(b);
        sum = _mm256_fmadd_ps(x, y, sum);

        __m256 xx = _mm256_loadu_ps(a + 8);
        __m256 yy = _mm256_loadu_ps(b + 8);
        sum1 = _mm256_fmadd_ps(xx, yy, sum1);

        __m256 xxx = _mm256_loadu_ps(a + 8 * 2);
        __m256 yyy = _mm256_loadu_ps(b + 8 * 2);
        sum2 = _mm256_fmadd_ps(xxx, yyy, sum2);

        __m256 xxxx = _mm256_loadu_ps(a + 8 * 3);
        __m256 yyyy = _mm256_loadu_ps(b + 8 * 3);
        sum3 = _mm256_fmadd_ps(xxxx, yyyy, sum3);

        a += 32;
        b += 32;
    }

    sum = _mm256_add_ps(sum, sum1);
    sum2 = _mm256_add_ps(sum2, sum3);
    sum = _mm256_add_ps(sum, sum2);

    c = cnt % 4;
    while (c-- > 0) {
        __m256 x = _mm256_loadu_ps(a);
        __m256 y = _mm256_loadu_ps(b);
        sum = _mm256_fmadd_ps(x, y, sum);
        a += 8;
        b += 8;
    }

    accum = hsum256_ps_avx(sum);

    cnt = len % 8;
    while (cnt-- > 0) {
        accum += *a++ * *b++;
    }
#else
    while (len-- > 0) {
        accum += *a++ * *b++;
    }
#endif
    return accum;
}

qtk_maybe_unused QTK_INLINE static float qtk_vector_normf(float *a, int len) {
    return qtk_sqrtf(qtk_vector_dotf(a, a, len));
}

qtk_maybe_unused QTK_INLINE static void
qtk_vector_multipy_elewise(float *a, float *b, float *dst, int len) {
#ifdef QTK_USE_NEON
    uint32_t blkCnt; /* Loop counter */
    f32x4_t vec1;
    f32x4_t vec2;
    f32x4_t res;

    /* Compute 4 outputs at a time */
    blkCnt = len >> 2U;

    while (blkCnt > 0U) {
        /* C = A * B */

        /* Multiply the inputs and then store the results in the destination
         * buffer. */
        vec1 = vld1q_f32(a);
        vec2 = vld1q_f32(b);
        res = vmulq_f32(vec1, vec2);
        vst1q_f32(dst, res);

        /* Increment pointers */
        a += 4;
        b += 4;
        dst += 4;

        /* Decrement the loop counter */
        blkCnt--;
    }

    /* Tail */
    blkCnt = len & 0x3;
    while (blkCnt > 0U) {
        /* C = A * B */

        /* Multiply input and store result in destination buffer. */
        *dst++ = (*a++) * (*b++);

        /* Decrement loop counter */
        blkCnt--;
    }
#else
    for (int i = 0; i < len; i++) {
        dst[i] = a[i] * b[i];
    }
#endif
}

qtk_maybe_unused static void qtk_vector_sub_elewise(float *a, float *b,
                                                    float *dst, int len) {
    for (int i = 0; i < len; i++) {
        dst[i] = a[i] - b[i];
    }
}

qtk_maybe_unused static void qtk_vector_add_elewise(float *a, float *b,
                                                    float *dst, int len) {
    for (int i = 0; i < len; i++) {
        dst[i] = a[i] + b[i];
    }
}

qtk_maybe_unused static void qtk_vector_add_elewise_i32(int32_t *a, int32_t *b,
                                                        int32_t *dst, int len) {
    for (int i = 0; i < len; i++) {
        dst[i] = a[i] + b[i];
    }
}

qtk_maybe_unused static void qtk_vector_div_elewise(float *a, float *b,
                                                    float *dst, int len) {
    for (int i = 0; i < len; i++) {
        dst[i] = a[i] / b[i];
    }
}

qtk_maybe_unused static void qtk_vector_pow_elewise(float *a, float *b,
                                                    float *dst, int len) {
    for (int i = 0; i < len; i++) {
        dst[i] = qtk_powf(a[i], b[i]);
    }
}

qtk_maybe_unused static void qtk_vector_sqrt_elewise(float *a, float *dst,
                                                     int len) {
    for (int i = 0; i < len; i++) {
        dst[i] = qtk_sqrtf(a[i]);
    }
}

qtk_maybe_unused QTK_INLINE static void
qtk_vector_multipy_elewise_depth(float *multi, float *single, float *dst,
                                 int len, int depth) {
    for (int i = 0; i < len; i++, single++) {
        for (int j = 0; j < depth; j++, multi++, dst++) {
            *dst = *multi * *single;
        }
    }
}

qtk_maybe_unused QTK_INLINE static void
qtk_vector_update(float *hist, float *cur, int dim, float alpha) {
    for (int i = 0; i < dim; i++) {
        hist[i] = (1.0 - alpha) * hist[i] + alpha * cur[i];
    }
}

qtk_maybe_unused QTK_INLINE static float qtk_vector_sum(float *d, int len) {
    float sum = 0;

    for (int i = 0; i < len; i++) {
        sum += d[i];
    }
    return sum;
}

qtk_maybe_unused QTK_INLINE static void qtk_vector_scale(float *d, float *dst,
                                                         int len, float scale) {
#ifdef QTK_USE_NEON
    uint32_t blockSize = len;
    uint32_t blkCnt; /* Loop counter */
    float *pSrc = d;
    float *pDst = dst;
    f32x4_t vec1;
    f32x4_t res;

    /* Compute 4 outputs at a time */
    blkCnt = blockSize >> 2U;

    while (blkCnt > 0U) {
        /* C = A * scale */

        /* Scale the input and then store the results in the destination buffer.
         */
        vec1 = vld1q_f32(pSrc);
        res = vmulq_f32(vec1, vdupq_n_f32(scale));
        vst1q_f32(pDst, res);

        /* Increment pointers */
        pSrc += 4;
        pDst += 4;

        /* Decrement the loop counter */
        blkCnt--;
    }

    /* Tail */
    blkCnt = blockSize & 0x3;
    while (blkCnt > 0U) {
        /* C = A * scale */

        /* Scale input and store result in destination buffer. */
        *pDst++ = (*pSrc++) * scale;

        /* Decrement loop counter */
        blkCnt--;
    }
#else
    for (int i = 0; i < len; i++) {
        dst[i] = d[i] * scale;
    }
#endif
}

qtk_maybe_unused QTK_INLINE static void qtk_vector_add(float *d, float *dst,
                                                       int len, float added) {
    for (int i = 0; i < len; i++) {
        dst[i] = d[i] + added;
    }
}

qtk_maybe_unused QTK_INLINE static float qtk_vector_mse(float *a, float *b,
                                                        int len) {
    float sum = 0;
    for (int i = 0; i < len; i++) {
        sum += (a[i] - b[i]) * (a[i] - b[i]);
    }
    return sum / len;
}

qtk_maybe_unused QTK_INLINE static void
qtk_vector_argmax(float *d, int len, float *max_val, int *max_idx) {
    *max_val = d[0];
    *max_idx = 0;
    for (int i = 1; i < len; i++) {
        if (d[i] > *max_val) {
            *max_val = d[i];
            *max_idx = i;
        }
    }
}

#ifdef QTK_USE_NEON
qtk_maybe_unused QTK_INLINE static float32x4_t
__arm_vec_sqrt_f32_neon(float32x4_t x) {
    float32x4_t x1 = vmaxq_f32(x, vdupq_n_f32(FLT_MIN));
    float32x4_t e = vrsqrteq_f32(x1);
    e = vmulq_f32(vrsqrtsq_f32(vmulq_f32(x1, e), e), e);
    e = vmulq_f32(vrsqrtsq_f32(vmulq_f32(x1, e), e), e);
    return vmulq_f32(x, e);
}

qtk_maybe_unused QTK_INLINE static void
qtk_vector_cpx_mag(wtk_complex_t *a, float *d, int numSamples) {
    uint32_t blkCnt; /* loop counter */
    float32_t real, imag;
    float32x4x2_t vecA;
    float32x4_t vRealA;
    float32x4_t vImagA;
    float32x4_t vMagSqA;

    float32x4x2_t vecB;
    float32x4_t vRealB;
    float32x4_t vImagB;
    float32x4_t vMagSqB;
    float *pSrc = (float *)a;
    float *pDst = d;

    /* Loop unrolling: Compute 8 outputs at a time */
    blkCnt = numSamples >> 3;

    while (blkCnt > 0U) {
        /* out = sqrt((real * real) + (imag * imag)) */

        vecA = vld2q_f32(pSrc);
        pSrc += 8;

        vecB = vld2q_f32(pSrc);
        pSrc += 8;

        vRealA = vmulq_f32(vecA.val[0], vecA.val[0]);
        vImagA = vmulq_f32(vecA.val[1], vecA.val[1]);
        vMagSqA = vaddq_f32(vRealA, vImagA);

        vRealB = vmulq_f32(vecB.val[0], vecB.val[0]);
        vImagB = vmulq_f32(vecB.val[1], vecB.val[1]);
        vMagSqB = vaddq_f32(vRealB, vImagB);

        /* Store the result in the destination buffer. */
        vst1q_f32(pDst, __arm_vec_sqrt_f32_neon(vMagSqA));
        pDst += 4;

        vst1q_f32(pDst, __arm_vec_sqrt_f32_neon(vMagSqB));
        pDst += 4;

        /* Decrement the loop counter */
        blkCnt--;
    }

    blkCnt = numSamples & 7;
    while (blkCnt > 0U) {
        /* C[0] = sqrt(A[0] * A[0] + A[1] * A[1]) */

        real = *pSrc++;
        imag = *pSrc++;

        /* store result in destination buffer. */
        *pDst++ = sqrtf((real * real) + (imag * imag));

        /* Decrement loop counter */
        blkCnt--;
    }
}

qtk_maybe_unused QTK_INLINE static void
qtk_vector_cpx_mag_squared(wtk_complex_t *a, float *d, int numSamples) {
    uint32_t blkCnt; /* Loop counter */
    float32_t real, imag;
    float32x4x2_t vecA;
    float32x4_t vRealA;
    float32x4_t vImagA;
    float32x4_t vMagSqA;

    float32x4x2_t vecB;
    float32x4_t vRealB;
    float32x4_t vImagB;
    float32x4_t vMagSqB;
    float *pSrc = (float *)a;
    float *pDst = d;

    /* Loop unrolling: Compute 8 outputs at a time */
    blkCnt = numSamples >> 3;

    while (blkCnt > 0U) {
        /* out = sqrt((real * real) + (imag * imag)) */

        vecA = vld2q_f32(pSrc);
        pSrc += 8;

        vRealA = vmulq_f32(vecA.val[0], vecA.val[0]);
        vImagA = vmulq_f32(vecA.val[1], vecA.val[1]);
        vMagSqA = vaddq_f32(vRealA, vImagA);

        vecB = vld2q_f32(pSrc);
        pSrc += 8;

        vRealB = vmulq_f32(vecB.val[0], vecB.val[0]);
        vImagB = vmulq_f32(vecB.val[1], vecB.val[1]);
        vMagSqB = vaddq_f32(vRealB, vImagB);

        /* Store the result in the destination buffer. */
        vst1q_f32(pDst, vMagSqA);
        pDst += 4;

        vst1q_f32(pDst, vMagSqB);
        pDst += 4;

        /* Decrement the loop counter */
        blkCnt--;
    }

    blkCnt = numSamples & 7;
    while (blkCnt > 0U) {
        /* C[0] = (A[0] * A[0] + A[1] * A[1]) */

        real = *pSrc++;
        imag = *pSrc++;

        /* store result in destination buffer. */
        *pDst++ = (real * real) + (imag * imag);

        /* Decrement loop counter */
        blkCnt--;
    }
}

qtk_maybe_unused QTK_INLINE static float
qtk_vector_cpx_mag_squared_sum(wtk_complex_t *a, int numSamples) {
    uint32_t blkCnt; /* Loop counter */
    float32_t real, imag;
    float32x4x2_t vecA;
    float32x4_t vRealA;
    float32x4_t vImagA;
    float32x4_t vMagSqA;

    float32x4x2_t vecB;
    float32x4_t vRealB;
    float32x4_t vImagB;
    float32x4_t vMagSqB;
    float sum, tmp;
    float *pSrc = (float *)a;
    f32x4_t accum = vdupq_n_f32(0);

    /* Loop unrolling: Compute 8 outputs at a time */
    blkCnt = numSamples >> 3;

    while (blkCnt > 0U) {
        /* out = sqrt((real * real) + (imag * imag)) */

        vecA = vld2q_f32(pSrc);
        pSrc += 8;

        vRealA = vmulq_f32(vecA.val[0], vecA.val[0]);
        vImagA = vmulq_f32(vecA.val[1], vecA.val[1]);
        vMagSqA = vaddq_f32(vRealA, vImagA);

        vecB = vld2q_f32(pSrc);
        pSrc += 8;

        vRealB = vmulq_f32(vecB.val[0], vecB.val[0]);
        vImagB = vmulq_f32(vecB.val[1], vecB.val[1]);
        vMagSqB = vaddq_f32(vRealB, vImagB);

        accum = vmlaq_f32(accum, vMagSqA, vMagSqB);
        /* Decrement the loop counter */
        blkCnt--;
    }

#if defined(__aarch64__)
    sum = vpadds_f32(vpadd_f32(vget_low_f32(accum), vget_high_f32(accum)));
#else
    tmp = vpadd_f32(vget_low_f32(accum), vget_high_f32(accum));
    sum = vget_lane_f32(tmp, 0) + vget_lane_f32(tmp, 1);
#endif

    blkCnt = numSamples & 7;
    while (blkCnt > 0U) {
        /* C[0] = (A[0] * A[0] + A[1] * A[1]) */

        real = *pSrc++;
        imag = *pSrc++;

        /* store result in destination buffer. */
        sum += (real * real) + (imag * imag);

        /* Decrement loop counter */
        blkCnt--;
    }
    return sum;
}
#else
qtk_maybe_unused QTK_INLINE static void
qtk_vector_cpx_mag(wtk_complex_t *a, float *d, int numSamples) {
    int i;
    for (i = 0; i < numSamples; i++) {
        d[i] = sqrtf(a[i].a * a[i].a + a[i].b * a[i].b);
    }
}

qtk_maybe_unused QTK_INLINE static void
qtk_vector_cpx_mag_squared(wtk_complex_t *a, float *d, int numSamples) {
    int i;
    for (i = 0; i < numSamples; i++) {
        d[i] = a[i].a * a[i].a + a[i].b * a[i].b;
    }
}

qtk_maybe_unused QTK_INLINE static float
qtk_vector_cpx_mag_squared_sum(wtk_complex_t *a, int numSamples) {
    float sum = 0;
    int i;
    for (i = 0; i < numSamples; i++) {
        sum += a[i].a * a[i].a + a[i].b * a[i].b;
    }
    return sum;
}

#endif

#ifdef __cplusplus
};
#endif
#endif
