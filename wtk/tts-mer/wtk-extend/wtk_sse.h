#ifndef WTK_MER_SSE_H
#define WTK_MER_SSE_H
#ifdef __cplusplus
extern "C" {
#endif

/* 
mmintrin.h------>MMX
xmmintrin.h------>SSE
emmintrin.h------>SSE2
pmmintrin.h------>SSE3
 */
#ifdef USE_NENO64
#define USE_NEON
#endif

#ifdef __AVX__
    #include <immintrin.h>
    #define sse256_float32_t __m256
    #define sse256_set _mm256_broadcast_ss
    #define sse256_mul _mm256_mul_ps
    #define sse256_add _mm256_add_ps
    #define sse256_loadu _mm256_loadu_ps
    #define sse256_load _mm256_load_ps
    #define sse256_storeu _mm256_storeu_ps
#endif

#if defined(__ANDROID__) || defined(USE_NENO)
    #include <arm_neon.h>
    #define sse_float32_t float32x4_t
    #define sse_mul vmulq_f32
    #define sse_add vaddq_f32
    #define sse_loadu vld1q_f32
#elif  defined(__SSE__)
    #include <xmmintrin.h>
    #define sse_float32_t __m128
    #define sse_set _mm_set1_ps
    #define sse_mul _mm_mul_ps
    #define sse_add _mm_add_ps
    #define sse_loadu _mm_loadu_ps
    #define sse_load _mm_load_ps
    #define sse_storeu _mm_storeu_ps
#endif

#ifdef __cplusplus
}
#endif
#endif