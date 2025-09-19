#ifndef __QTK_MATH_QTK_MATH_H__
#define __QTK_MATH_QTK_MATH_H__
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_type.h"
#ifdef __cplusplus
extern "C" {
#endif

#ifndef M_2PI
#define M_2PI 6.283185307179586476925286766559005
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

float qtk_vec_xdot(float *m, float *n, int l);
float qtk_vec_sum(float *d, int l);
void qtk_vec_add_inplace(float *d, int l, float parm);
void qtk_vec_add_vec(float *d, int l, float *v, float alpha);
void qtk_vec_set(float *d, int l, float v);
void qtk_vec_add_vecvec(float *d, int l, float *v, float *r, float alpha,
                        float beta);
float qtk_vec_min(float *d, int l);
float qtk_vec_min1(float *d, int l, int *where);
void qtk_vec_scale(float *d, int l, float alpha);
#ifdef HAVE_STDIO_H
void qtk_vec_print(const char *hint, float *d, int l);
void qtk_mat_print(const char *hint, float *d, int row, int col);
#endif

#define MAT_GET(mat, ncol, row, col) ((mat) + (row) * (ncol) + (col))
#define MAT_GET_ROW(mat, ncol, row) ((mat) + (row) * (ncol))

static int qtk_approx_eqf(float a, float b, float tolerance) qtk_maybe_unused;
static inline int qtk_approx_eqf(float a, float b, float tolerance) {
    if (a == b) {
        return 1;
    }
    float diff = fabs(a - b);
    if (isnan(diff) || isinf(diff)) {
        return 0;
    }
    return diff <= tolerance * (fabs(a) + fabs(b));
}

static float qtk_rand_uniform() qtk_maybe_unused;
static inline float qtk_rand_uniform() {
    return cast(float, (rand() + 1.0) / (RAND_MAX + 2.0));
}

static float qtk_rand_gauss() qtk_maybe_unused;
static inline float qtk_rand_gauss(void) {
    return cast(float, sqrt(-2 * log(qtk_rand_uniform())) *
                           cos(2 * M_PI * qtk_rand_uniform()));
}

#ifdef __cplusplus
};
#endif
#endif
