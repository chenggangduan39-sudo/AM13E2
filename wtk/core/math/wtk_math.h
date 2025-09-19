#ifndef WTK_MATH_WTK_MATH_H_
#define WTK_MATH_WTK_MATH_H_
#include <math.h>
#include "wtk_vector.h"
#include "wtk/core/cfg/wtk_source.h"
#include "wtk_matrix.h"
#ifdef __cplusplus
extern "C" {
#endif
#ifdef PI
#undef PI                /* PI is defined in Linux */
#endif
#define FIXLZERO  (-1.0E5)   /* ~log(0) */
#define PI   3.14159265358979
#define M_LOG_2PI 1.8378770664093454835606594728112
#define TPI  6.28318530717959     /* PI*2 */
#define WTK_TPI  6.28318530717959     /* PI*2 */
#define LZERO  (-1.0E10)   /* ~log(0) */
#define LSMALL (-0.5E10)   /* log values < LSMALL are set to LZERO */
#define MINEARG (-708.3)   /* lowest exp() arg  = log(MINLARG) */
#define MINLARG 2.45E-308  /* lowest log() arg  = exp(MINEARG) */
#define QLOGE2 0.693147180559945309417232121458176568

/* ZeroMeanFrame: remove dc offset from given vector */
void wtk_vector_zero_mean_frame(wtk_vector_t* v);
void wtk_vector_pre_emphasise(wtk_vector_t* v,float k);
wtk_vector_t* wtk_math_create_ham_window(int frame_size);
wtk_vector_t* wtk_math_create_povey_window(int frame_size);
float* wtk_math_create_float_ham_window(int frame_size);
float* wtk_math_create_bartlett_window(int win);
float* wtk_math_create_bartlett_hann_window(int win);
float* wtk_math_create_blackman_window(int win);
void wtk_math_init_blackman_window(float *f,int n);
float * wtk_math_create_sine_window(int win);
void wtk_math_init_hanning_window(float *f,int n);
float* wtk_math_create_hanning_window(int n);
float* wtk_math_create_conj_window(int len);
void wtk_realft (wtk_vector_t* s);
void wtk_math_do_diff(wtk_vector_t** pv,int window_size,double sigma,int start_pos,int step);
void wtk_math_do_simple_diff(wtk_vector_t** pv,int window_size,int start_pos,int step);

int wtk_source_read_vector(wtk_source_t* s,wtk_vector_t* v,int bin);
int wtk_source_read_vector_little(wtk_source_t* s,wtk_vector_t* v,int bin);
int wtk_source_read_matrix(wtk_source_t *s,wtk_matrix_t *m,int bin);
int wtk_source_read_matrix_little(wtk_source_t *s,wtk_matrix_t *m,int bin);
int wtk_source_read_double_vector(wtk_source_t* s,wtk_double_vector_t* v,int bin);
int wtk_source_read_double_matrix(wtk_source_t *s,wtk_double_matrix_t *m,int bin);
int wtk_source_read_double_vector_little(wtk_source_t* s,wtk_double_vector_t* v,int bin);
int wtk_source_read_double_matrix_little(wtk_source_t *s,wtk_double_matrix_t *m,int bin);
int wtk_source_read_hlda(wtk_source_t *s,wtk_matrix_t **pm);
int wtk_hlda_read(wtk_matrix_t **pm,wtk_source_t *s);
int wtk_source_read_hlda_bin(wtk_matrix_t **pm,wtk_source_t *s);
void wtk_matrix_multiply_vector(wtk_vector_t *dst,wtk_matrix_t *m,wtk_vector_t *src);
int wtk_floatfix_q(float max,float min,int n);

//c=a*f
void wtk_float_scale(float *a,float f,float *c,int n);
void wtk_short_scale(short *a,float f,float *c,int n);
//c+=a*f;
void wtk_float_scale_add(float *a,float f,float *c,int n);
void wtk_short_scale_add(short *a,float f,float *c,int n);

void wtk_float_mult(float *a,float *b,float *c,int n);
//c+=a*b;
void wtk_float_mult_add(float *a,float *b,float *c,int n);

float wtk_float_max(float *a,int n);
float wtk_float_min(float *a,int n);
int wtk_float_argmax(float *a,int n);

float wtk_float_abs_mean(float *pv,int len);
float wtk_float_mean(float *pv,int len);
float wtk_float_sum(float *a,int n);
float wtk_int_sum(int *a,int n);
float wtk_int_power(int *a,int n);
float wtk_short_power(short *a,int n);
float wtk_float_abs_max(float *a,int n);
float wtk_float_median(float *a,int n);
int wtk_float_median_index(float *a,int n);
float wtk_float_weighted_median(float *a, float *weight, int n);
float wtk_float_mode(float *a,int n);
int wtk_int_mode(int *a,int n);
int wtk_int_cnt_mode(int *a, int n, int cnt);
void ifft(float x[], float y[], int n); //数组x存储时域序列的实部，数组y存储时域序列的虚部,n代表N点FFT.;

void wtk_relu(float* a,float* alpha,float* beta,int len);
void wtk_relu2(float* a,int len);
void wtk_raw_relu(float* a,int len);
void wtk_softmax(float* a,int len);
void wtk_torch_softmax(float* a,int len);
void wtk_add_log(float *p,int n);
void wtk_pnorm(float *f,float *f2,int len,int next_len);
void wtk_sigmoid(float *f,int len);
void wtk_tanh(float *f,int len);

float wtk_scalar_sigmoid(float a);
float wtk_scalar_tanh(float a);

float* wtk_math_create_hanning_window2(int n);
float* wtk_math_create_hanning_window_torch(int n);

float wtk_float_abs_max(float *pf,int n);
void wtk_float_mul(float *a,int n,float f);
float wtk_short_sum(short *pv,int len);
float wtk_short_abs_sum(short *pv,int len);
float wtk_short_abs_mean(short *pv,int len);
float wtk_int_abs_mean(int *pv,int len);
int wtk_short_max(short *pv,int len);
int wtk_short_abs_max(short *pv,int len);
float wtk_short_energy(short *pv,int len);
float wtk_float_energy(float *a,int n);

float wtk_float_max2(float *a,int n,float add);

double wtk_double_sum(double *a,int n);
/*
 * ham;hann;conj;sine
 */
float* wtk_math_create_win(char *win,int len);
void wtk_float_clamp(float *a,int n,int use_min, float min, int use_max, float max);

float wtk_float_std(float *pv, int len);
/*
 * limit a float number to between min and max
 */
float wtk_float_clip(float value, float min, float max);
int wtk_math_rand_r(unsigned int *seed);
void wtk_float_add_dither(float *data, int len);

/*
 * log add exp based on numpy/libtorch
 */
float wtk_logaddexp(float x,float y);
int qtk_gcd(int m, int n);
int qtk_lcm(int m, int n);

#define QTK_SSAT16f(f)                                                         \
    ((f) > 32767 ? 32767                                                       \
                 : (f) < -32768 ? -32768                                       \
                                : cast(int, cast(int, (f) + 32768.5) - 32768))
#ifndef M_LN2
#define M_LN2 0.693147180559945309417 // ln(2)
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifdef __cplusplus
};
#endif
#endif
