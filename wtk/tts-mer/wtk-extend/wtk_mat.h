#ifndef WTK_MER_MAT_H_
#define WTK_MER_MAT_H_
#include "wtk/tts-mer/wtk-extend/wtk_tts_common.h"
#include <assert.h>
#ifdef __cplusplus
extern "C" {
#endif

/* matrix vector write to file */
void wtk_mer_matdf_save(wtk_matdf_t *m, FILE *f, char *fn);
void wtk_mer_matf_save(wtk_matf_t *m, FILE *f, char *fn);
int wtk_mer_matf_write_file(wtk_matf_t *m, char *fn, int is_bin);
void wtk_mer_matf_read_file(wtk_matf_t *m, char *fn);
void wtk_mer_matf_read_file2(wtk_matf_t *m, char *fn, int isbin);
wtk_vecf_t* wtk_mer_vecf_read_binfile( char *in_fn);
void wtk_mer_vecf_read_file( wtk_vecf_t *dst, char *fn);
void wtk_mer_vecf_read_file2( wtk_vecf_t *dst, char *fn, int isbin);
int wtk_mer_matdf_write_file(wtk_matdf_t *m, char *fn, int is_bin);

wtk_matf_t* wtk_matf_slice(wtk_matf_t *src, int srow, int nrow, int scol, int ncol);
void wtk_matf_slice2(wtk_matf_t *src, wtk_matf_t *dst, int srow, int nrow, int scol, int ncol);
wtk_matf_t* wtk_matf_row_slice(wtk_matf_t *src, int srow, int nrow);
void wtk_matf_init_transpose(wtk_matf_t *src, wtk_matf_t *dst);
void wtk_matf_reshape(wtk_matf_t *src, int nr, int nc);
void wtk_mer_matf_concat( wtk_matf_t *a, wtk_matf_t *b, wtk_matf_t *dst);
void wtk_mer_matf_shape_print( wtk_matf_t *a);
#define wtk_mer_matf_shape_print(a) {wtk_debug( "%s.shape=(%d,%d) \n", #a, (a)->row, (a)->col);}


void wtk_float_set(float *dst, float f, int n);
void wtk_float_tanh(float *dst, int n);
void wtk_float_set_bound(float min, float max, float *p, int len);

wtk_vecf_t* wtk_vecf_new_concat( wtk_vecf_t *a, wtk_vecf_t *b);
void wtk_vecf_concat( wtk_vecf_t *dst, wtk_vecf_t *a, wtk_vecf_t *b);
void wtk_vecf_minus(wtk_vecf_t *dst, wtk_vecf_t *a, wtk_vecf_t *b);

void wtk_matf_vecf_multi(wtk_matf_t *dst, wtk_vecf_t *vec);
void wtk_matf_vecf_add(wtk_matf_t *dst, wtk_vecf_t *vec);
void wtk_matf_vecf_cpy(wtk_matf_t *dst, wtk_vecf_t *vec);

void wtk_matdf_init_transpose(wtk_matdf_t *src, wtk_matdf_t *dst);
void wtk_double_set_bound(double min, double max, double *p, int len);
#ifdef __cplusplus
}
#endif
#endif