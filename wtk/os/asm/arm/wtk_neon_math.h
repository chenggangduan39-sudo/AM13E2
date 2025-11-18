#ifndef _WTK_NEON_MATH_C_H_
#define _WTK_NEON_MATH_C_H_
#include "wtk/core/wtk_type.h"
#include "wtk/core/math/wtk_mat.h"
#include "wtk/core/math/wtk_math.h"
#ifdef __cplusplus
extern "C" {
#endif

void wtk_neon_math_vecf_muti_matf(float *vec_c,float *mat_a,float *vec_b,int row,int col);
void wtk_neon_math_vecf_muti_matf_add_vec(float *vec_c,float *mat_a,float *vec_b,float *vec_d,int row,int col);
void wtk_neon_math_vecf_muti_matf_1(float *vec_c,float *mat_a,float *vec_b,int row,int col);
void wtk_neon_math_vecf_muti_matf2(float *vec_c,float *mat_a,float *vec_b,int row,int col);
void wtk_neon_math_vecf_muti_matf3(float *vec_c,float *mat_a,float *vec_b,int row,int col);
wtk_matf_t* wtk_neon_math_mat_transf_8float(wtk_matf_t *a);
wtk_matf_t* wtk_neon_math_mat_transf_16float(wtk_matf_t *a);
void wtk_neon_math_vecf_muti_matf_transf(float *vec_c,float *mat_a,float *vec_b,int row,int col);
void wtk_neon_math_vecf_muti_matf_transf_1(float *vec_c,float *mat_a,float *vec_b,int row,int col);
void wtk_neon_math_vecf_muti_matf_transf2(float *vec_c,float *mat_a,float *vec_b,int row,int col);
void wtk_neon_math_vecf_muti_matf_transf2_add_vec(float *vec_c,float *mat_a,float *vec_b,float *vec_d,int row,int col);
void wtk_neon_math_vec_sub(float *c,float *a,float *b,int len);
void wtk_neon_math_vec_add(float *c,float *a,float *b,int len);
void wtk_neon_math_vec_add_const(float *c,float *a,float const_b,int len);
void wtk_neon_math_vec_mul(float *c,float *a,float *b,int len);
void wtk_neon_math_vec_sub_square(float *c,float *a,float *b,int len);
void wtk_neon_math_vec_sub_square2(float *c,float *a,float *b,int len);
void wtk_neon_math_vec_mul_sum(float *c,float *a,float *b,int len);
void wtk_neon_math_vec_square(float *c,float *a,int len);
void wtk_neon_math_vec_mul_const(float *c,float *a,float const_b,int len);
void wtk_neon_math_vec_add_mul(float *c,float *a,float *b,float *d,int len);
void wtk_neon_math_memcpy(float *a,float *b,int len);
void wtk_neon_math_memset(float *a,float const_b,int len);
#ifdef __cplusplus
};;
#endif
#endif
