#ifndef WTK_VITE_MATH_WTK_MAT_H_
#define WTK_VITE_MATH_WTK_MAT_H_
#include "wtk/core/wtk_type.h"
#include "wtk_matrix.h"
#include "wtk_math.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	signed char *p;
	unsigned int row;
	unsigned int col;
	unsigned short col_bytes;
	unsigned char align;
}wtk_matc_t;

#define wtk_mat_at(a,i,j) ((a)->p+i*(a)->col_bytes+j)
#define wtk_matc_at(a,i,j) ((a)->p+i*(a)->col_bytes+j)
//#define wtk_mati_at(a,_xi,_xj) (((a)->p)+((_xi)*((a)->col_bytes))+(_xj))

wtk_matc_t* wtk_matc_new_align(int row,int col,int align);
void wtk_matc_init_transpose(wtk_matc_t *dst,wtk_matc_t *src);

wtk_matc_t* wtk_matc_new(int row,int col);
void wtk_matc_zero(wtk_matc_t *m);
wtk_matc_t* wtk_matc_new2(wtk_matrix_t *m,float scale);
void wtk_matc_init(wtk_matc_t *mc,wtk_matrix_t *m,float scale);
void wtk_matc_cpy(wtk_matc_t *dst,wtk_matc_t *src);
void wtk_matc_delete(wtk_matc_t *m);
void wtk_matc_print(wtk_matc_t *mc);
wtk_matc_t* wtk_matc_transpose(wtk_matc_t *src);
/**
 * |---------- row -------------|
 *   |
 *  col
 *   |
 * |----------------------------|
 */
wtk_matc_t* wtk_matc_new3(wtk_matrix_t *m,float scale);

typedef struct
{
	unsigned char *p;
	unsigned int row;
	unsigned int col;
	unsigned short col_bytes;
	unsigned char align;
}wtk_matuc_t;

wtk_matuc_t* wtk_matuc_new_align(int row,int col,int align);

wtk_matuc_t* wtk_matuc_new(int row,int col);
wtk_matuc_t* wtk_matuc_new2(wtk_matrix_t *m,float scale);
wtk_matuc_t* wtk_matuc_new3(wtk_matc_t *src,int *pmin);
void wtk_matuc_init(wtk_matuc_t* cm,wtk_matrix_t *m,float scale);
void wtk_matuc_delete(wtk_matuc_t *mc);
void wtk_matuc_print(wtk_matuc_t *mc);

typedef struct
{
	int *p;
	float scale;
	unsigned int row;
	unsigned int col;
}wtk_mati_t;

#define wtk_mati_at(mi,i,j) (*((mi)->p+i*(mi)->col+j))

wtk_mati_t* wtk_mati_new(int row,int col);
void wtk_mati_cpy(wtk_mati_t *src,wtk_mati_t *dst);
void wtk_mati_zero(wtk_mati_t *m);
wtk_mati_t* wtk_mati_new2(wtk_matrix_t *m,float scale);
wtk_mati_t* wtk_mati_new3(wtk_matrix_t *m,float scale);
void wtk_mati_init(wtk_mati_t *im,wtk_matrix_t *m,float scale);
void wtk_mati_delete(wtk_mati_t *im);
void wtk_mati_print(wtk_mati_t *mi);
void wtk_mati_print2(wtk_mati_t *mi,int cnt);
void wtk_mati_scale(wtk_mati_t *mi,double scale);
void wtk_mati_set(wtk_mati_t *mi,int row,int col,int v);
int wtk_mati_get(wtk_mati_t *mi,int row,int col);
wtk_mati_t* wtk_mati_transpose(wtk_mati_t *mi);
int wtk_mati_value_count(wtk_mati_t *m,int v);
int wtk_mati_range_count(wtk_mati_t *m,int f1,int f2);
void wtk_mati_prune(wtk_mati_t *m,int f1,int f2);
int wtk_mati_max(wtk_mati_t *m);
int wtk_mati_min(wtk_mati_t *m);
int wtk_mati_avg(wtk_mati_t *m);

/*
 * m=|a*b|
 */
void wtk_mati_multi(wtk_mati_t *m,wtk_matuc_t *a,wtk_matc_t *b);
void wtk_mati_multi2(wtk_mati_t *m,wtk_mati_t *a,wtk_matc_t *b);
void wtk_mati_multi3(wtk_mati_t *m,wtk_mati_t *a,wtk_mati_t *b);
void wtk_mati_multi_dc(wtk_mati_t *m,wtk_mati_t *a,wtk_matc_t *b,int nx);
void wtk_mati_add(wtk_mati_t *a,wtk_mati_t *b);
void wtk_mati_multi4(wtk_mati_t *m,wtk_matc_t *a,wtk_matc_t *b);
void wtk_mati_multi4_2(wtk_mati_t *m,wtk_matc_t *a,wtk_matc_t *b);

void wtk_int_matrix_multi(wtk_int_matrix_t *m,wtk_matc_t *a,wtk_matc_t *b);

//转置后
//
//|1*a| |b*a|=|1*b|
void wtk_mati_multi_cc_x(wtk_mati_t *m,wtk_matc_t *a,wtk_matc_t *b);
void wtk_mati_multi_cu_x(wtk_mati_t *m,wtk_matc_t *a,wtk_matuc_t *b,int min);
void wtk_mati_multi_cu_x_scale(wtk_mati_t *m,wtk_matc_t *a,wtk_matuc_t *b,int min,float scale);
void wtk_mati_multi_cc_x_scale(wtk_mati_t *m,wtk_matc_t *a,wtk_matc_t *b,float scale);
void wtk_mati_multi_uc_x(wtk_mati_t *m,wtk_matuc_t *a,wtk_matc_t *b);
void wtk_mati_multi_uc_x_scale(wtk_mati_t *m,wtk_matuc_t *a,wtk_matc_t *b,double scale,double min);
void wtk_mati_multi_ic_x(wtk_mati_t *m,wtk_mati_t *a,wtk_matc_t *b);

void wtk_mati_multi_x_raw(wtk_mati_t *m,wtk_matuc_t *a,wtk_matc_t *b);

typedef wtk_mati_t* (*wtk_mati_heap_new_f)(void *ths,int row,int col);
typedef void (*wtk_mati_heap_delete_f)(void *ths,wtk_mati_t *m);

typedef struct
{
	void *ths;
	wtk_mati_heap_new_f mat_new;
	wtk_mati_heap_delete_f mat_delete;
}wtk_mati_heap_t;

void wtk_mati_multi_dc_ext(wtk_mati_t *c,wtk_mati_t *a,wtk_matc_t *b,
		wtk_mati_heap_t *heap,int NX);
void wtk_mati_multi_dc2(wtk_mati_t *c,wtk_mati_t *a,wtk_mati_t *b,wtk_mati_heap_t *heap,int NX);
void wtk_mati_multi_dc_ext_cc(wtk_mati_t *c,wtk_matc_t *a,wtk_matc_t *b,
		wtk_mati_heap_t *heap,int NX);
void wtk_mati_multi_dc_cc(wtk_mati_t *c,wtk_matc_t *a,wtk_matc_t *b,int nx);

typedef struct
{
	float *p;
	int len;
}wtk_vecf_t;

wtk_vecf_t* wtk_vecf_new(int len);
wtk_vecf_t* wtk_vecf_heap_new(wtk_heap_t* heap,int len);
int wtk_vecf_bytes(wtk_vecf_t *f);
wtk_vecf_t* wtk_vecf_dup(wtk_vecf_t *src);
void wtk_vecf_cpy(wtk_vecf_t *dst,wtk_vecf_t *src);
void wtk_vecf_delete(wtk_vecf_t *v);
void wtk_vecf_zero(wtk_vecf_t *v);
void wtk_vecf_init(wtk_vecf_t *v,float f);
void wtk_vecf_add(wtk_vecf_t *v,float *f);
void wtk_vecf_scale(wtk_vecf_t *v,float scale);
float wtk_vecf_norm(wtk_vecf_t *v);
float wtk_vecf_norm2(wtk_vecf_t *v,wtk_vecf_t *src);
float wtk_vecf_norm_len(wtk_vecf_t *v);
float wtk_vecf_cos(wtk_vecf_t *v1,wtk_vecf_t *v2);
float wtk_vecf_cos2(wtk_vecf_t *v1,wtk_vecf_t *v2);
float wtk_vecf_value(wtk_vecf_t *v);
void wtk_vecf_print(wtk_vecf_t *v);
float wtk_vecf_dist(wtk_vecf_t *v1,wtk_vecf_t *v2);
float wtk_vecf_sum_log(wtk_vecf_t *vec);
float wtk_vecf_vecvec(wtk_vecf_t *v1,wtk_vecf_t *v2);
double wtk_vecf_vecvec2(wtk_vecf_t *v1,wtk_vecf_t *v2);
void wtk_vecf_invert(wtk_vecf_t *dst,wtk_vecf_t *src);

typedef struct
{
	unsigned char *p;
	int len;
}wtk_vecuc_t;
wtk_vecuc_t* wtk_vecuc_new(int len);
void wtk_vecuc_delete(wtk_vecuc_t *v);
void wtk_vecuc_zero(wtk_vecuc_t *v);
void wtk_vecuc_init(wtk_vecuc_t *v,float *pf,float scale);

typedef struct
{
	char *p;
	int len;
}wtk_vecc_t;
wtk_vecc_t* wtk_vecc_new(int len);
void wtk_vecc_delete(wtk_vecc_t *v);
void wtk_vecc_zero(wtk_vecc_t *v);
void wtk_vecc_init(wtk_vecc_t *v,float *pf,float scale);




typedef struct
{
	int *p;
	int len;
}wtk_veci_t;
wtk_veci_t* wtk_veci_new(int len);
void wtk_veci_delete(wtk_veci_t *v);
void wtk_veci_zero(wtk_veci_t *v);
void wtk_veci_scale(wtk_veci_t *v,double scale);
void wtk_veci_init(wtk_veci_t *v,float *pf,float scale);
void wtk_veci_print(wtk_veci_t *v);

typedef struct
{
	float *p;
	int row;
	int col;
}wtk_matf_t;

#define wtk_matf_at(a,_xi,_xj) (((a)->p)+((_xi)*((a)->col))+(_xj))
#define wtk_matf_bytes(a) ((a)->row*(a)->col*sizeof(float))


float* wtk_matf_at2(wtk_matf_t *m,int row,int col);

wtk_matf_t* wtk_matf_new(int row,int col);
wtk_range_t wtk_matf_range(wtk_matf_t *f);
void wtk_matf_cpy(wtk_matf_t *src,wtk_matf_t *dst);
void wtk_matf_cpy2(wtk_matf_t *dst,float *fp,int n);
void wtk_matf_delete(wtk_matf_t *m);
void wtk_matf_zero(wtk_matf_t *m);
float* wtk_matf_row(wtk_matf_t *m,int row);
void wtk_matf_scale(wtk_matf_t *m,float scale);

/**
 *   |x*N|*|y*N|=|x*y|
 * c=a*b
 */
void wtk_matf_mul(wtk_matf_t *a,wtk_matf_t *b,wtk_matf_t *c);

void wtk_matf_multvm(wtk_matf_t *src_v,wtk_matf_t *m,wtk_matf_t *dst_v);

wtk_matf_t* wtk_matf_new2(wtk_matrix_t *m);
wtk_matf_t* wtk_matf_transpose(wtk_matf_t *mf);
void wtk_matf_print(wtk_matf_t *f);
void wtk_matf_mul_raw(wtk_matf_t *m,wtk_matf_t *a,wtk_matf_t *b);
void wtk_mati_multi_x_raw(wtk_mati_t *m,wtk_matuc_t *a,wtk_matc_t *b);

void wtk_matf_add(wtk_matf_t *dst,wtk_matf_t *a,wtk_matf_t *b);
void wtk_matf_add_scale(wtk_matf_t *dst,float *a,float *b,float alpha,float beta);
void wtk_matf_add2(wtk_matf_t *dst,wtk_matf_t *b);
void wtk_matf_normalize_row(wtk_matf_t *m);
void wtk_matf_scale2(wtk_matf_t *dst,wtk_matf_t *src,double scale);
void wtk_matf_set_init(wtk_matf_t *m,float v);
void wtk_matf_vm_raw(float *dst,float *src,wtk_matf_t *m);
void wtk_matf_vm(float *dst,float *src,wtk_matf_t *m);
void wtk_matf_vm_add(float *dst,float *src,wtk_matf_t *m);
void wtk_float_add(float *dst, float *src, int n);
void wtk_int_add(int *dst,int *src,int n);
void wtk_float_vm(float *dst,float *src,float *m,int row,int col);
void wtk_float_vm_add(float *dst,float *src,float *m,int row,int col);
void wtk_float_vm_raw(float *dst,float *src,float *m,int row,int col);
void wtk_matf_save(wtk_matf_t *m,FILE *f,char *sym);
void wtk_matf_vm_hid(float *dst,float *src,wtk_matf_t *m,int add);
void wtk_matf_vm_hid_x(float *dst,float *src,wtk_matf_t *m,int add);

typedef struct
{
	double *p;
	int row;
	int col;
}wtk_matdf_t;

wtk_matdf_t* wtk_matdf_new(int row,int col);
void wtk_matdf_delete(wtk_matdf_t *m);
void wtk_matdf_print(wtk_matdf_t *f);
void wtk_matdf_cpy(wtk_matdf_t *src,wtk_matdf_t *dst);
void wtk_matdf_zero(wtk_matdf_t *m);
void wtk_matdf_add(wtk_matdf_t *dst,wtk_matdf_t *a,wtk_matdf_t *b);
void wtk_matdf_add2(wtk_matdf_t *dst,wtk_matdf_t *b);
void wtk_matdf_scale(wtk_matdf_t *dst,wtk_matdf_t *src,double scale);
void wtk_matdf_normalize_row(wtk_matdf_t *m);
void wtk_float_normalize(float *fp,int n);

typedef struct
{
	double *p;
	int len;
}wtk_vecdf_t;

wtk_vecdf_t* wtk_vecdf_new(int len);
void wtk_vecdf_zero(wtk_vecdf_t *v);
void wtk_vecdf_delete(wtk_vecdf_t *v);

typedef struct
{
	signed char *p;
	float scale;
	int row;
	int col;
}wtk_matb_t;

wtk_matb_t *wtk_matb_new(int row,int col);
void wtk_matb_delete(wtk_matb_t *m);
void wtk_matb_fix(wtk_matb_t *m,wtk_matf_t *src);
void  wtk_matb_fix_matf(wtk_matb_t *mc,wtk_matf_t *mf,float max);
int wtk_matb_bytes(wtk_matb_t *b);

typedef struct
{
	short *p;
	float scale;
	int row;
	int col;
}wtk_mats_t;

wtk_mats_t *wtk_mats_new(int row,int col);
int wtk_mats_bytes(wtk_mats_t *s);
void wtk_mats_zero(wtk_mats_t *m);
void wtk_mats_delete(wtk_mats_t *m);
void  wtk_mats_fix_matf(wtk_mats_t *mc,wtk_matf_t *mf,float max);

typedef struct
{
	short* p;
	int stride;
	int col;
	int row;
}wtk_sub_mats_t;

void wtk_sub_mats_init(wtk_sub_mats_t *sub, short *p, int row_offset,
                       int num_rows, int col_offset, int num_cols, int stride);
void wtk_sub_mats_init2(wtk_sub_mats_t *sub, short *p, int row, int col,
                        int stride);

typedef struct
{
	char *p;
	int len;
	float scale;
}wtk_vecb_t;
wtk_vecb_t* wtk_vecb_new(int len);
void wtk_vecb_delete(wtk_vecb_t *v);
void  wtk_vecb_fix_vecf(wtk_vecb_t *mc,wtk_vecf_t *mf,float max);

typedef struct
{
	short *p;
	int len;
	float scale;
	float *c_scale;
}wtk_vecs_t;
wtk_vecs_t* wtk_vecs_new(int len);
wtk_vecs_t* wtk_vecs_new_qlas(int len,int c_size);
void wtk_vecs_delete(wtk_vecs_t *v);
void  wtk_vecs_fix_vecf(wtk_vecs_t *mc,wtk_vecf_t *mf,float max);
void  wtk_vecs_fix_vecf_qlas(wtk_vecs_t *mc,wtk_vecf_t *mf,float max,int len,int cache_size);

typedef struct
{
	short *p;
	int len;
	int shift;
}wtk_vecs2_t;
int wtk_vecs2_bytes(wtk_vecs2_t *v);
wtk_vecs2_t* wtk_vecs2_new(int len);
void wtk_vecs2_delete(wtk_vecs2_t *v);


/*
量化mat
//0-8 char
// 0-16 i16
*/
// typedef enum{
// 	WTK_MATQ_8,
// 	WTK_MATQ_U8,
// 	WTK_MATQ_16,
// 	WTK_MATQ_U16,
// }wtk_matq_type_t;

typedef struct{
	char *p;
	int row;
	int col;
	float scale;
}wtk_matq8_t;

wtk_matq8_t *wtk_matq8_new(int row, int col);
wtk_matq8_t *wtk_matq8_heap_new(wtk_heap_t *heap, int row, int col);
int wtk_matq8_delete(wtk_matq8_t *);
int wtk_mat_f32toi8_transfer(wtk_matf_t *matf,wtk_matq8_t *matq8);
void wtk_matq8_print(wtk_matq8_t *matq8);

#include "wtk/core/cfg/wtk_source.h"
wtk_matf_t* wtk_matf_read(wtk_source_t *src);
wtk_vecf_t* wtk_vecf_read(wtk_source_t *src);
wtk_veci_t* wtk_veci_read(wtk_source_t *src);
void wtk_matf_write(wtk_matf_t *m,FILE *f);
void wtk_vecf_write(wtk_vecf_t *v,FILE *f);
void wtk_veci_write(wtk_veci_t *v,FILE *f);
int wtk_veci_bytes(wtk_veci_t *v);
int wtk_mati_bytes(wtk_mati_t *m);

void* wtk_matfix_new(int row,int col, int fixed_nbytes);
void wtk_matfix_delete(void* mfix, int fixed_nbytes);
#ifdef __cplusplus
};
#endif
#endif

