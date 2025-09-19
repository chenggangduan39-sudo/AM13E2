#ifndef WTK_ASR_FEXTRA_FNN_QLAS_WTK_QLASASM
#define WTK_ASR_FEXTRA_FNN_QLAS_WTK_QLASASM
#include "wtk/core/wtk_type.h" 
#include "wtk/core/math/wtk_mat.h"
#include "wtk/core/math/wtk_math.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * reference: https://gcc.gnu.org/onlinedocs/gcc/Extended-Asm.html  ASM
 * http://infocenter.arm.com/help/topic/com.arm.doc.dui0491c/DUI0491C_arm_compiler_reference.pdf
 */
int wtk_qlas_mats_mul_row_asm_value(short *p1,short *p2,int col);
void wtk_qlas_matb_mul_neon64(wtk_vecb_t *a,wtk_matb_t *b,wtk_veci_t *c);
int wtk_qlas_mats_mul_row_asm_value(short *p1,short *p2,int col);
void wtk_qlas_mats_mul_neon32(wtk_vecs_t *a,wtk_mats_t *b,wtk_veci_t *c);
void wtk_qlas_matb_mul_neon32(wtk_vecb_t *a,wtk_matb_t *b,wtk_veci_t *c);
int wtk_short_multi_neon(short *p1,short *p2,int col);
int wtk_char_mult_neon(char *p1,char *p2,int col);
void wtk_qlas_mat_cache_doverflow4(wtk_vecs_t *a1,wtk_mats_t *b,wtk_veci_t *c);
#ifdef __cplusplus
};;
#endif
#endif
