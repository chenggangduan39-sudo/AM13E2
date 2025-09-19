#ifndef WTK_MER_BANDMAT_H
#define WTK_MER_BANDMAT_H
#include "tts-mer/wtk_mer_common.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    int l;
    int u;
    wtk_matf_t *matf;
    int is_transposed;
} wtk_mer_bandmat_t;

wtk_mer_bandmat_t* wtk_mer_bandmat_new(int l, int u, int frame_num, int is_transposed);
wtk_mer_bandmat_t* wtk_mer_bandmat_heap_new(wtk_heap_t *heap, int l, int u, int frame_num, int is_transposed);
void wtk_mer_bandmat_init(wtk_mer_bandmat_t *b_mt, int l, int u, wtk_matf_t *mf, int is_transposed);
void wtk_mer_bandmat_delete(wtk_mer_bandmat_t *b_mt);
void wtk_mer_bandmat_print(wtk_mer_bandmat_t *bmt);
void wtk_mer_bandmat_print2(wtk_mer_bandmat_t *bmt, int is_print_matf);
void wtk_mer_bandmat_T(wtk_mer_bandmat_t *bm);

void wtk_mer_bandmat_dot_mv_plus_equals(wtk_mer_bandmat_t *a_bm, int transposed_a, wtk_vecf_t *b_vf, wtk_vecf_t *dst);
void wtk_mer_bandmat_dot_mm_plus_equals(wtk_mer_bandmat_t *a_bm, int transposed_a, wtk_mer_bandmat_t *b_bm, int transposed_b, wtk_mer_bandmat_t *dst, int transposed_c, wtk_vecf_t *diag);
void wtk_mer_bandmat_cholesky_banded(wtk_matf_t *mat, int overwrite_ab, int lower);
wtk_mer_bandmat_t* wtk_mer_bandmat_cholesky(wtk_mer_bandmat_t *mat_bm, int lower, int alternative);

void wtk_mer_bandmat_solve_triangular_banded(wtk_matf_t *a_rect, wtk_vecf_t *b, int transposed, int lower);
void wtk_mer_bandmat_solve_triangular(wtk_mer_bandmat_t *a_bm, wtk_vecf_t *b);
void wtk_mer_bandmat_cho_solve(wtk_mer_bandmat_t *chol_bm, wtk_vecf_t *b);
void wtk_mer_bandmat_solveh(wtk_mer_bandmat_t *a_bm, wtk_vecf_t *b);
#ifdef __cplusplus
}
#endif
#endif
