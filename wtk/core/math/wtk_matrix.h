#ifndef WTK_MATH_WTK_MATRIX_H_
#define WTK_MATH_WTK_MATRIX_H_
#include "wtk_vector.h"
#include "wtk_fastm.h"
#ifdef __cplusplus
extern "C" {
#endif
/**
 * m[3][3]	   |<-   double *    ->|<----------------         double     --------------------------->|
 *	   memory: | c0 | c1 | c2 | c3 | c10 | c11 | c12 | c13 | c21 | c22 | c23 | c30 | c31 | c32 | c33 |
 *	        => | c0 | c1 | c2 | c3 |   //c0 =rows(3) (double**)
 *	    	   | c10| c11| c12| c13|   //c1 ->pointer c10, c10=cols(3),(double*)
 *	    	   | c20| c21| c22| c23|
 *	    	   | c30| c31| c32| c33|
 *
 *	c0-c3: double array index,c0(int)=3, c1 -> c10, c10(int) =3
 *	c11-c12: array;
 */

///// fast exp() implementation
#if 0
static union {
	double d;
	struct {
		int j, i;
	} n;
} d2i;
#define EXP_A (1048576/M_LN2)
#define EXP_C 60801
#define FAST_EXP(y) (d2i.n.i = EXP_A*(y)+(1072693248-EXP_C),d2i.d)
#endif
#define FAST_EXP(y) wtk_fast_exp(y)

typedef double* wtk_double_matrix_t;
typedef float* wtk_matrix_t;
typedef float* wtk_smatrix_t;
typedef wtk_matrix_t wtk_trimat_t;
typedef int* wtk_int_matrix_t;
typedef char* wtk_char_matrix_t;
typedef struct qtk_sub_matrix qtk_sub_matrix_t;
typedef struct qtk_nnet3_submatrix_info qtk_nnet3_submatrix_info_t;

#define wtk_double_matrix_bytes(r,c) wtk_round_word(wtk_double_vector_bytes(c)*r+wtk_round_word((r+1)*sizeof(wtk_double_vector_t*)))
#define wtk_matrix_bytes(r,c) wtk_round_word(wtk_vector_bytes(c)*r+wtk_round_word((r+1)*sizeof(wtk_vector_t*)))
#define wtk_smatrix_bytes(r,c) wtk_round_word(wtk_svector_bytes(c)*r+wtk_round_word((r+3)*sizeof(wtk_vector_t*)))
#define wtk_matrix_rows(m) (*(int*)m)
#define wtk_matrix_cols(m) (*(int*)(m[1]))
#define wtk_matrix_delete(m) free(m)
#define wtk_double_matrix_delete(m) wtk_free(m)

typedef struct
{
	unsigned int dim;
	double *m;
}qtk_blas_double_vector_t;

typedef struct
{
	unsigned int row;
	unsigned int col;
	//double *m;
	float *m;
}qtk_blas_matrix_t;

typedef struct
{
	unsigned int row;
	unsigned int col;
	//double *m;
	double *m;
}qtk_blas_double_matrix_t;

struct qtk_nnet3_submatrix_info
{
	int matrix_index;  // index into "matrices": the underlying matrix.
	int row_offset;
	int num_rows;
	int col_offset;
	int num_cols;
};

struct qtk_sub_matrix
{
	float* f;
	int stride;
	int col;
	int row;
};

#ifdef USE_AVX
void qtk_matrix_add_matmat_avx(qtk_sub_matrix_t* dst,qtk_sub_matrix_t* src,qtk_sub_matrix_t* linear);
#endif

typedef struct
{
	double *data;
	int num_rows;
}qtk_sp_matrix_t;

qtk_sp_matrix_t* qtk_sp_matrix_new(int row);
void qtk_sp_matrix_delete(qtk_sp_matrix_t* sp);
void qtk_sp_matrix_diag(qtk_sp_matrix_t* sp,double r);
void qtk_sub_matrix_init(qtk_sub_matrix_t *sub, float *f, int row_offset,
                         int num_rows, int col_offset, int num_cols,
                         int stride);
void qtk_sub_matrix_init2(qtk_sub_matrix_t *sub, float *f, int row, int col,
                          int stide);
void qtk_sub_matrix_set(qtk_sub_matrix_t* sub,float *f,int row_offset,int num_rows,int col_offset,int num_cols,int stride);
void qtk_sub_matrix_set2(qtk_sub_matrix_t* sub,float *f,int row,int col,int stride);
void qtk_sub_matrix_print(qtk_sub_matrix_t* sub);
void qtk_sub_matrix_mul_element(qtk_sub_matrix_t *dst,qtk_sub_matrix_t *src1,qtk_sub_matrix_t *src2);
void qtk_matirx_copy_cols(qtk_sub_matrix_t* dst,qtk_sub_matrix_t* src,int *vec);
void qtk_matrix_copy_from_mat(qtk_sub_matrix_t* dst,qtk_sub_matrix_t* src);
void qtk_matrix_add_matmat(qtk_sub_matrix_t* dst,qtk_sub_matrix_t* src,qtk_sub_matrix_t* linear,float alpha);
void qtk_matrix_copy_rows_fromvec(qtk_sub_matrix_t* dst,qtk_blas_matrix_t* bias);
qtk_blas_double_vector_t* qtk_blas_double_vector_new(int dim);
void qtk_blas_double_vector_delete(qtk_blas_double_vector_t* v);
void qtk_blas_addvec(qtk_blas_double_vector_t* dst,float alpha,float *src);
void qtk_blas_double_vec_print(qtk_blas_double_vector_t* a);
void qtk_blas_addvecd(qtk_blas_double_vector_t* dst,float alpha,double *src);
double qtk_blas_vecvec(qtk_blas_double_vector_t* a,qtk_blas_double_vector_t* b);
void qtk_blas_addmatvec(qtk_blas_double_vector_t *dst,float alpha,qtk_blas_double_matrix_t *a,qtk_blas_double_vector_t *b);
void qtk_blas_matrix_aver(qtk_blas_matrix_t *aver,qtk_blas_matrix_t *in,int cnt);
qtk_blas_matrix_t* qtk_blas_matrix_new(int row,int col);
void qtk_blas_matrix_scale(qtk_blas_matrix_t* m,qtk_nnet3_submatrix_info_t *info,float alpha);
void qtk_blas_matrix_scale2(qtk_blas_matrix_t *m,float val);
qtk_blas_double_matrix_t* qtk_blas_double_matrix_new(int row,int col);
void qtk_blas_double_matrix_delete(qtk_blas_double_matrix_t* m);
void qtk_blas_matrix_zero(qtk_blas_matrix_t *m);
void qtk_blas_matrix_delete(qtk_blas_matrix_t *m);
void qtk_blas_matrix_trans(qtk_blas_matrix_t *src,qtk_blas_matrix_t *dst);
void qtk_blas_matrix_apply_floor(qtk_blas_matrix_t *m,float floor);
//void qtk_blas_matrix_scale(qtk_blas_matrix_t *m,float val);
void qtk_blas_matrix_add(qtk_blas_matrix_t *m,float val);
void qtk_blas_matrix_mul_col(qtk_blas_matrix_t *dst, qtk_blas_matrix_t *src,qtk_blas_matrix_t *src2);
void qtk_blas_matrix_sum_col(qtk_blas_matrix_t *dst,qtk_blas_matrix_t *src);
void qtk_blas_matrix_sum_row(qtk_blas_matrix_t *dst,qtk_blas_matrix_t *src,qtk_blas_matrix_t *scale);
void qtk_blas_matrix_add_mat(qtk_blas_matrix_t *m,qtk_blas_matrix_t *n);
void qtk_blas_matrix_add_matrow(qtk_blas_matrix_t *m,qtk_blas_matrix_t *n);
void qtk_blas_matrix_apply_power(qtk_blas_matrix_t *m,float pow);
void qtk_blas_matrix_mul(qtk_blas_matrix_t *input,qtk_blas_matrix_t *output, qtk_nnet3_submatrix_info_t *input_info,
		qtk_nnet3_submatrix_info_t *output_info,qtk_blas_matrix_t *weight,qtk_blas_matrix_t *bias);
void qtk_blas_matrix_mul_element(qtk_blas_matrix_t *m,qtk_blas_matrix_t *b);
void qtk_blas_matrix_print(qtk_blas_matrix_t *m);

void qtk_add_spmatrix_vec(void);

wtk_double_matrix_t* wtk_double_matrix_new(int nrows,int ncols);
wtk_double_matrix_t* wtk_double_matrix_new_h(wtk_heap_t *heap,int nrows,int ncols);
void wtk_double_matrix_zero(wtk_double_matrix_t *m);
void wtk_double_matrix_cpy(wtk_double_matrix_t *src,wtk_double_matrix_t *dst);
void wtk_double_matrix_init_identity(wtk_double_matrix_t *A);
void wtk_double_matrix_cpy(wtk_double_matrix_t *src,wtk_double_matrix_t *dst);
wtk_matrix_t* wtk_matrix_new(int nrows,int ncols);
int wtk_matrix_bytes2(wtk_matrix_t *m);
wtk_matrix_t* wtk_matrix_newh(wtk_heap_t* h,int nrows,int ncols);
wtk_smatrix_t* wtk_smatrix_newh(wtk_heap_t *h,int nrows,int ncols);
void wtk_matrix_add(wtk_matrix_t *m,wtk_matrix_t *a);

/**
 * m=|a|*|b|
 */
void wtk_matrix_multi(wtk_matrix_t *m,wtk_matrix_t *a,wtk_matrix_t *b);
void wtk_matrix_to_vector(wtk_matrix_t *m,wtk_vector_t *v);
void wtk_matrix_print(wtk_matrix_t *m);

void wtk_matrix_transpose(wtk_matrix_t *dst,wtk_matrix_t *src);
void wtk_matrix_cpy(wtk_matrix_t *src,wtk_matrix_t *dst);
//void wtk_sub_matrix_cpy(wtk_sub_matrix_t *src,wtk_sub_matrix_t *dst);
void wtk_sub_matrix_cpy2(qtk_blas_matrix_t  *dst,qtk_blas_matrix_t *src,qtk_nnet3_submatrix_info_t *dst_info,qtk_nnet3_submatrix_info_t *src_info);
void wtk_sub_matrix_add_rows(qtk_blas_matrix_t  *dst,qtk_blas_matrix_t *src,qtk_nnet3_submatrix_info_t *dst_info,qtk_nnet3_submatrix_info_t *src_info,int *index,float alpha);
void wtk_sub_matrix_cpy_rows(qtk_blas_matrix_t  *dst,qtk_blas_matrix_t *src,qtk_nnet3_submatrix_info_t *dst_info,qtk_nnet3_submatrix_info_t *src_info,int *index);
//void wtk_sub_matrix_cpy_relu(wtk_sub_matrix_t *src,wtk_sub_matrix_t *dst,float floor);
void wtk_sub_matrix_cpy_relu2(qtk_blas_matrix_t  *dst,qtk_blas_matrix_t *src,
		qtk_nnet3_submatrix_info_t *dst_info,qtk_nnet3_submatrix_info_t *src_info,float floor);
//void wtk_sub_matrix_add(wtk_sub_matrix_t *src,wtk_sub_matrix_t *dst,float alpha);
wtk_matrix_t* wtk_matrix_transpose2(wtk_matrix_t *a);
void wtk_matrix_scale(wtk_matrix_t *m,float scale);

double wtk_matrix_max(wtk_matrix_t *m);
double wtk_matrix_min(wtk_matrix_t *m);
double wtk_matrix_max_abs(wtk_matrix_t *m);

wtk_int_matrix_t* wtk_int_matrix_new(int r,int c);
void wtk_int_matrix_print(wtk_int_matrix_t *m);
wtk_matrix_t* wtk_matrix_new2(int r,int col);
void wtk_matrix_multi6(wtk_matrix_t *m,wtk_matrix_t *a,wtk_matrix_t *b);

wtk_char_matrix_t* wtk_char_matrix_new(int r,int c);
void wtk_matrix_multi2(wtk_matrix_t *m,wtk_matrix_t *a,wtk_matrix_t *b);
void wtk_matrix_multi3(wtk_matrix_t *m,wtk_matrix_t *a,wtk_matrix_t *b);
float wtk_matrix_avg(wtk_matrix_t *m);

typedef double(*wtk_matrix_random_f)();
void wtk_matrix_set_random(wtk_matrix_t *m,wtk_matrix_random_f random);
void wtk_matrix_zero(wtk_matrix_t *m);
void wtk_matrix_set_init_value(wtk_matrix_t *m,double f);

/**
 *	|1*src| *|src* dst| = |1*dst|
 */
void wtk_vector_mult_matrix(wtk_vector_t *dst,wtk_vector_t *src,wtk_matrix_t *m);
/**
 * [s,e)
 */
void wtk_vector_mult_matrix2(wtk_vector_t *dst,wtk_vector_t *src,wtk_matrix_t *m,int s,int e);


/**
 *	|dst * src| *|src*1|=|dst*1|
 */
void wtk_vector_mult_matrix_rev(wtk_vector_t *dst,wtk_vector_t *src,wtk_matrix_t *m,int add);
void wtk_vector_mult_matrix_rev2(wtk_vector_t *dst,wtk_vector_t *src,wtk_matrix_t *m,int s,int e);
void wtk_matrix_add2(wtk_matrix_t *dst,wtk_matrix_t *src,float f1,float f2);

double wtk_fast_exp(double y);
float wtk_fast_exp2(float y);
#ifdef __cplusplus
};
#endif
#endif
