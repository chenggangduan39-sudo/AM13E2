#include "wtk_nn_activation.h"
#ifdef __SSE__
#include <xmmintrin.h>
#endif

#ifdef __AVX2__
#include <immintrin.h>
#define _mm256_fmadd_ps(a,b,c) _mm256_add_ps(_mm256_mul_ps(a, b), c)
#define _mm_fmadd_ps(a,b,c) _mm_add_ps(_mm_mul_ps(a, b), c)
static __m128 exp4_approx(__m128 X)
{
   const __m128 K0 = _mm_set1_ps(0.99992522f);
   const __m128 K1 = _mm_set1_ps(0.69583354f);
   const __m128 K2 = _mm_set1_ps(0.22606716f);
   const __m128 K3 = _mm_set1_ps(0.078024523f);
   const __m128 log2_E = _mm_set1_ps(1.44269504);
   const __m128 max_in = _mm_set1_ps(50.f);
   const __m128 min_in = _mm_set1_ps(-50.f);
   const __m128i mask = _mm_set1_epi32(0x7fffffff);
   __m128 XF, Y;
   __m128i I;
   X = _mm_mul_ps(X, log2_E);
   X = _mm_max_ps(min_in, _mm_min_ps(max_in, X));
   XF = _mm_floor_ps(X);
   I = _mm_cvtps_epi32(XF);
   X = _mm_sub_ps(X, XF);
   Y = _mm_fmadd_ps(_mm_fmadd_ps(_mm_fmadd_ps(K3, X, K2), X, K1), X, K0);
   I = _mm_slli_epi32(I, 23);
   Y = _mm_castsi128_ps(_mm_and_si128(mask, _mm_add_epi32(I, _mm_castps_si128(Y))));
   return Y;
}
static __m256 exp8_approx(__m256 X)
{
   __m256 Y;
   __m128 Xhi, Xlo, Yhi, Ylo;
   Xhi = _mm256_extractf128_ps(X, 1);
   Xlo = _mm256_extractf128_ps(X, 0);
   Yhi = exp4_approx(Xhi);
   Ylo = exp4_approx(Xlo);
   Y = _mm256_insertf128_ps(_mm256_setzero_ps(), Yhi, 1);
   Y = _mm256_insertf128_ps(Y, Ylo, 0);
   return Y;
}
#endif

/* max(0, x) */
void wtk_nn_relu(float *p, int len)
{
    float *e;
    e = p+len;

    while(p<e)
    {
        *p = max(0.0f, *p);
        p++;
    }
}

/* max(0, x) */
void wtk_nn_relu_d(double *p, int len)
{
    double *e;
    e = p+len;

    while(p<e)
    {
        *p = max(1e-10, *p);
        p++;
    }
}

// ---------------------- sigmoid tanh 近似求值
static const float tansig_table[201] = {
0.000000f, 0.039979f, 0.079830f, 0.119427f, 0.158649f,
0.197375f, 0.235496f, 0.272905f, 0.309507f, 0.345214f,
0.379949f, 0.413644f, 0.446244f, 0.477700f, 0.507977f,
0.537050f, 0.564900f, 0.591519f, 0.616909f, 0.641077f,
0.664037f, 0.685809f, 0.706419f, 0.725897f, 0.744277f,
0.761594f, 0.777888f, 0.793199f, 0.807569f, 0.821040f,
0.833655f, 0.845456f, 0.856485f, 0.866784f, 0.876393f,
0.885352f, 0.893698f, 0.901468f, 0.908698f, 0.915420f,
0.921669f, 0.927473f, 0.932862f, 0.937863f, 0.942503f,
0.946806f, 0.950795f, 0.954492f, 0.957917f, 0.961090f,
0.964028f, 0.966747f, 0.969265f, 0.971594f, 0.973749f,
0.975743f, 0.977587f, 0.979293f, 0.980869f, 0.982327f,
0.983675f, 0.984921f, 0.986072f, 0.987136f, 0.988119f,
0.989027f, 0.989867f, 0.990642f, 0.991359f, 0.992020f,
0.992631f, 0.993196f, 0.993718f, 0.994199f, 0.994644f,
0.995055f, 0.995434f, 0.995784f, 0.996108f, 0.996407f,
0.996682f, 0.996937f, 0.997172f, 0.997389f, 0.997590f,
0.997775f, 0.997946f, 0.998104f, 0.998249f, 0.998384f,
0.998508f, 0.998623f, 0.998728f, 0.998826f, 0.998916f,
0.999000f, 0.999076f, 0.999147f, 0.999213f, 0.999273f,
0.999329f, 0.999381f, 0.999428f, 0.999472f, 0.999513f,
0.999550f, 0.999585f, 0.999617f, 0.999646f, 0.999673f,
0.999699f, 0.999722f, 0.999743f, 0.999763f, 0.999781f,
0.999798f, 0.999813f, 0.999828f, 0.999841f, 0.999853f,
0.999865f, 0.999875f, 0.999885f, 0.999893f, 0.999902f,
0.999909f, 0.999916f, 0.999923f, 0.999929f, 0.999934f,
0.999939f, 0.999944f, 0.999948f, 0.999952f, 0.999956f,
0.999959f, 0.999962f, 0.999965f, 0.999968f, 0.999970f,
0.999973f, 0.999975f, 0.999977f, 0.999978f, 0.999980f,
0.999982f, 0.999983f, 0.999984f, 0.999986f, 0.999987f,
0.999988f, 0.999989f, 0.999990f, 0.999990f, 0.999991f,
0.999992f, 0.999992f, 0.999993f, 0.999994f, 0.999994f,
0.999994f, 0.999995f, 0.999995f, 0.999996f, 0.999996f,
0.999996f, 0.999997f, 0.999997f, 0.999997f, 0.999997f,
0.999997f, 0.999998f, 0.999998f, 0.999998f, 0.999998f,
0.999998f, 0.999998f, 0.999999f, 0.999999f, 0.999999f,
0.999999f, 0.999999f, 0.999999f, 0.999999f, 0.999999f,
0.999999f, 0.999999f, 0.999999f, 0.999999f, 0.999999f,
1.000000f, 1.000000f, 1.000000f, 1.000000f, 1.000000f,
1.000000f, 1.000000f, 1.000000f, 1.000000f, 1.000000f,
1.000000f,
};

static inline float tansig_approx(float x)
{/* 
2/(1+exp(-2x))-1
 */
    int i;
    float y, dy;
    float sign=1;
    if (x<0)
    {
       x=-x;
       sign=-1;
    }
    i = (int)floor(.5f+25*x);
    i = max(0, min(200, i));
    x -= .04f*i;
    y = tansig_table[i];
    dy = 1-y*y;
    y = y + x*dy*(1 - y*x);
    return sign*y;
    // return (exp(x)-exp(-x))/(exp(x)+exp(-x));
}

static inline float sigmoid_approx(float x)
{/* 近似求值 */
    return .5f + .5f*tansig_approx(.5f*x);
}

inline float wtk_nn_sigmoid_inline(float f)
{
    // return 1.0/(1.0+expf(-f));
    return sigmoid_approx(f);
}

inline float wtk_nn_tanh_inline(float f)
{
    return tansig_approx(f);
}

#ifdef __AVX__
/* 指令版激活函数 */
static float celt_exp(float x)
{
   float out[8];
   __m256 X, Y;
   X = _mm256_set1_ps(x);
   Y = exp8_approx(X);
   _mm256_storeu_ps(out, Y);
   return out[0];
}
void wtk_nn_sigmoid(float *a, int len)
{
    int i;
    for (i=0;i<len-7;i+=8)
    {
        const __m256 one = _mm256_set1_ps(1.f);
        __m256 X, Y;
        X = _mm256_loadu_ps(&a[i]);
        Y = exp8_approx(X);
        Y = _mm256_mul_ps(Y,  _mm256_rcp_ps(_mm256_add_ps(Y, one)));
        _mm256_storeu_ps(&a[i], Y);
    }
    for (; i<len; i++)
    {
        float ex;
        ex = celt_exp(a[i]);
        a[i] = (ex)/(ex+1);
    }
}
void wtk_nn_tanh(float *a, int len)
{
    int i;
    for (i=0;i<len-7;i+=8)
    {
        const __m256 two = _mm256_set1_ps(2.f);
        const __m256 one = _mm256_set1_ps(1.f);
        __m256 X, Y;
        X = _mm256_loadu_ps(&a[i]);
        X = _mm256_mul_ps(X, two);
        Y = exp8_approx(X);
        Y = _mm256_mul_ps(_mm256_sub_ps(Y, one),  _mm256_rcp_ps(_mm256_add_ps(Y, one)));
        _mm256_storeu_ps(&a[i], Y);
    }
    for (;i<len;i++)
    {
        float ex2;
        ex2 = celt_exp(2*a[i]);
        a[i] = (ex2-1)/(ex2+1);
    }
}
void wtk_nn_softmax(float *a, int len)
{
    int i;
    for (i=0;i<len-7;i+=8)
    {
        __m256 X, Y;
        X = _mm256_loadu_ps(&a[i]);
        Y = exp8_approx(X);
        _mm256_storeu_ps(&a[i], Y);
    }
    for (; i<len; i++)
        a[i] = celt_exp(a[i]);
}
void wtk_nn_softplus(float *a, int len)
{
    int i;
    for(i = 0; i < len; ++i){
        a[i] = logf(1+celt_exp(a[i]));
    }
}
#else
/* 普通版激活函数 */
/* np.exp(p) / np.sum(np.exp(p))
归一化, sum(p)==1
 */
void wtk_nn_softmax(float *a, int len)
{
    float max,sum=0;
	float *p,*e;

    p=a;e=a+len;
    max=wtk_math_max(a, len);
    while (p<e)
    {
        sum+=*p=expf(*p-max);
        p++;
    }

    p=a;
    sum=1.0/sum;
    while (p<e)
    {
        (*p)*=sum;
        p++;
    }
}
void wtk_nn_sigmoid(float *a, int len)
{
    int i;
    for (i=0; i<len; ++i)
    {
        a[i]=sigmoid_approx(a[i]);
    }
}
void wtk_nn_tanh(float *a, int len)
{
    int i;
    for (i=0;i<len;i++)
    {
        a[i] = tansig_approx(a[i]);
    }
}
void wtk_nn_softplus(float *a, int len)
{
    int i;
    for(i = 0; i < len; ++i){
        a[i] = logf(1+expf(a[i]));
    }
}
#endif
