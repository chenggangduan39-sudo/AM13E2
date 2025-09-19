#ifndef __QBL_MATH_QBL_MATH_H__
#define __QBL_MATH_QBL_MATH_H__
#include "qtk/core/qtk_type.h"
#include "wtk/core/math/wtk_math.h"
#include <math.h>
#ifdef __cplusplus
extern "C" {
#endif

#define QBL_USAT8(v) ((v) > 255 ? 255 : (v) < 0 ? 0 : (v))
#define qtk_sqrtf(f) sqrtf(f)
#define qtk_fabs(f) fabs(f)
#define qtk_powf(x, y) powf(x, y)
#define qtk_expf(f) expf(f)
#define qtk_logf(f) logf(f)
#define qtk_cosf(f) cosf(f)
#define qtk_floorf(f) floorf(f)

#define QTK_PI 3.1415926535897932384626433832795

#ifndef FLT_MAX
#define FLT_MAX 3.402823466e+38F
#endif
#ifndef FLT_MIN
#define FLT_MIN 1.175494351e-38F
#endif

qtk_maybe_unused QTK_INLINE static int qtk_roundf(float f) {
    return cast(int, f + (f >= 0 ? 0.5f : -0.5f));
}

qtk_maybe_unused QTK_INLINE static int qtk_approx_eqf(float a, float b,
                                                      float tolerance) {
    if (a == b) {
        return 1;
    }
    float diff = qtk_fabs(a - b);
    if (isnan(diff) || isinf(diff)) {
        return 0;
    }
    return diff <= tolerance * (qtk_fabs(a) + qtk_fabs(b));
}

qtk_maybe_unused QTK_INLINE static float qtk_rand_uniform() {
    return cast(float, (rand() + 1.0) / (RAND_MAX + 2.0));
}

qtk_maybe_unused QTK_INLINE static float qtk_rand_gauss() {
    return cast(float, qtk_sqrtf(-2 * qtk_logf(qtk_rand_uniform())) *
                           qtk_cosf(2 * M_PI * qtk_rand_uniform()));
}

qtk_maybe_unused QTK_INLINE static void qtk_gen_gauss_vecf(float *d, int len) {
    int i;
    for (i = 0; i < len; i++) {
        d[i] = qtk_rand_gauss();
    }
}

#ifdef __cplusplus
};
#endif
#endif
