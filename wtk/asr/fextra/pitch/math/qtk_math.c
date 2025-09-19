#include "wtk/asr/fextra/pitch/math/qtk_math.h"

float qtk_vec_xdot(float *m, float *n, int l) {
    int idx;
    float res = 0;

    for (idx = 0; idx < l; idx++) {
        res += m[idx] * n[idx];
    }

    return res;
}

float qtk_vec_sum(float *d, int l) {
    float res = 0;
    int idx;

    for (idx = 0; idx < l; idx++) {
        res += d[idx];
    }

    return res;
}

float qtk_vec_min(float *d, int l) {
    int idx;
    float m = d[0];

    for (idx = 1; idx < l; idx++) {
        if (d[idx] < m) {
            m = d[idx];
        }
    }

    return m;
}

void qtk_vec_add_inplace(float *d, int l, float parm) {
    int idx;
    for (idx = 0; idx < l; idx++) {
        d[idx] += parm;
    }
}

void qtk_vec_add_vec(float *d, int l, float *v, float alpha) {
    int idx;

    for (idx = 0; idx < l; idx++) {
        d[idx] += v[idx] * alpha;
    }
}

void qtk_vec_set(float *d, int l, float v) {
    int idx;

    for (idx = 0; idx < l; idx++) {
        d[idx] = v;
    }
}

void qtk_vec_add_vecvec(float *d, int l, float *v, float *r, float alpha,
                        float beta) {
    int idx;

    for (idx = 0; idx < l; idx++) {
        d[idx] = d[idx] * beta + alpha * v[idx] * r[idx];
    }
}

void qtk_vec_scale(float *d, int l, float alpha) {
    int idx;

    for (idx = 0; idx < l; idx++) {
        d[idx] *= alpha;
    }
}

float qtk_vec_min1(float *d, int l, int *where) {
    int idx;
    float m = d[0];
    int w = 0;

    for (idx = 1; idx < l; idx++) {
        if (d[idx] < m) {
            m = d[idx];
            w = idx;
        }
    }
    *where = w;
    return m;
}

#ifdef HAVE_STDIO_H
void qtk_vec_print(const char *hint, float *d, int l) {
    int idx;

    printf("\n%s: ", hint);
    for (idx = 0; idx < l; idx++) {
        printf("%f ", d[idx]);
    }
    printf("\n");
}

void qtk_mat_print(const char *hint, float *d, int row, int col) {
    int i;
    char buf[32];

    printf("\n========== <%s> %d X %d ===========\n", hint, row, col);
    for (i = 0; i < row; i++) {
        snprintf(buf, sizeof(buf), "[%d]", i);
        qtk_vec_print(buf, d + i * col, col);
    }
    printf("==============================\n");
}
#endif
