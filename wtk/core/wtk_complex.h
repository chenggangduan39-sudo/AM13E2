#ifndef WTK_CORE_WTK_COMPLEX
#define WTK_CORE_WTK_COMPLEX
#include "wtk/core/wtk_type.h" 
#include "wtk/core/math/wtk_math.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_complex wtk_complex_t;
struct wtk_complex
{
	float a;
	float b;
};

typedef struct wtk_dcomplex wtk_dcomplex_t;
struct wtk_dcomplex
{
	double a;
	double b;
};

void wtk_complex_check(wtk_complex_t *c,int n);

double*** wtk_double_new_p3(int n1,int n2,int n3);
void wtk_double_delete_p3(double ***pf,int n1,int n2);

double** wtk_double_new_p2(int n1,int n2);
void wtk_double_zero_p2(double **p,int n1,int n2);
void wtk_double_delete_p2(double **pf,int n1);

float*** wtk_float_new_p3(int n1,int n2,int n3);
void wtk_float_delete_p3(float ***pf,int n1,int n2);

float** wtk_float_new_p2(int n1,int n2);
int wtk_float_bytes_p2(int n1,int n2);
void wtk_float_zero_p2(float **p,int n1,int n2);
void wtk_float_delete_p2(float **pf,int n1);

short **wtk_short_new_p2(int n1, int n2);
void wtk_short_zero_p2(short **ps, int n1, int n2);
void wtk_short_delete_p2(short **ps,int n1);

wtk_complex_t**** wtk_complex_new_p4(int n1,int n2,int n3,int n4);
void wtk_complex_delete_p4(wtk_complex_t ****p,int n1,int n2,int n3);

wtk_complex_t*** wtk_complex_new_p3(int n1,int n2,int n3);
void wtk_complex_delete_p3(wtk_complex_t ***p3,int n1,int n2);
void wtk_complex_cpy_p3(wtk_complex_t ***dst,wtk_complex_t ***src,int n1,int n2,int n3);
void wtk_complex_zero_p3(wtk_complex_t ***p,int n1,int n2,int n3);

wtk_complex_t** wtk_complex_new_p2(int n1,int n2);
void wtk_complex_delete_p2(wtk_complex_t **p2,int n1);
void wtk_complex_cpy_p2(wtk_complex_t **dst,wtk_complex_t **src,int n1,int n2);
void wtk_complex_zero_p2(wtk_complex_t **p,int n1,int n2);
int wtk_complex_bytes_p2(int n1,int n2);

void wtk_complex_zero(wtk_complex_t *c,int n);

int wtk_complex_invx4(wtk_complex_t *input,wtk_dcomplex_t *a,int nx,wtk_complex_t *b,int sym);
int wtk_complex_invx_and_det(wtk_complex_t *input,wtk_dcomplex_t *a,int nx,wtk_complex_t *b,int sym,double *det);
int wtk_complex_guass_elimination_p1(wtk_complex_t *input,wtk_complex_t *b,wtk_dcomplex_t *a,int nx,wtk_complex_t *out);
int wtk_complex_guass_elimination_p2(wtk_complex_t *input,wtk_complex_t *b,wtk_dcomplex_t *a,int nx,wtk_complex_t *out);
int wtk_complex_guass_elimination_p1_f(wtk_complex_t *input,wtk_complex_t *b,wtk_complex_t *a,int nx,wtk_complex_t *out);
void wtk_complex_itereig(wtk_complex_t *w,wtk_complex_t *u,wtk_complex_t *v,int n,float eps,int max_iter);


wtk_dcomplex_t*** wtk_dcomplex_new_p3(int n1,int n2,int n3);
void wtk_dcomplex_zero_p3(wtk_dcomplex_t ***p,int n1,int n2,int n3);
void wtk_dcomplex_delete_p3(wtk_dcomplex_t ***p3,int n1,int n2);
void wtk_dcomplex_cpy_p3(wtk_dcomplex_t ***dst,wtk_complex_t ***src,int n1,int n2,int n3);

wtk_dcomplex_t** wtk_dcomplex_new_p2(int n1,int n2);
void wtk_dcomplex_zero_p2(wtk_dcomplex_t **p,int n1,int n2);
void wtk_dcomplex_delete_p2(wtk_dcomplex_t **p2,int n1);
void wtk_dcomplex_cpy_p2(wtk_dcomplex_t **dst,wtk_dcomplex_t **src,int n1,int n2);

wtk_complex_t wtk_complex_mul(wtk_complex_t *a, wtk_complex_t *b);
void wtk_complex_fft(wtk_complex_t* fpix,int n);
void wtk_complex_ifft(wtk_complex_t* f,wtk_complex_t *x,int n);

void wtk_complex_exp(wtk_complex_t *f);

#ifdef __cplusplus
};
#endif
#endif
