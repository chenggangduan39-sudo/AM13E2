#ifndef WTK_MER_BLAS_H
#define WTK_MER_BLAS_H
#include "wtk/tts-mer/wtk-extend/wtk_tts_common.h"
#ifdef USE_MKL
// #include "third/mkl/include/mkl.h"
#include "mkl.h"
#elif defined  USE_GO2BLAS
#include "third/GotoBLAS2/common.h"
#include "third/GotoBLAS2/cblas.h"
#endif
#ifdef __cplusplus
extern "C" {
#endif

void wtk_mer_blas_sgemm(wtk_matf_t *a, wtk_matf_t *b, wtk_vecf_t *vf, wtk_matf_t *c);
void wtk_mer_blas_sgemm2(wtk_matf_t *a, wtk_matf_t *b, wtk_vecf_t *vf, wtk_matf_t *c);
void wtk_mer_blas_sgemm3(wtk_matdf_t *a, wtk_matf_t *b, wtk_vecf_t *vf, wtk_matdf_t *c);
void wtk_mer_unblas_sgemm(wtk_matf_t *a, wtk_matf_t *b, wtk_vecf_t *vf, wtk_matf_t *c);
void wtk_mer_unblas_sgemm_notrans(wtk_matf_t *a, wtk_matf_t *b, wtk_vecf_t *vf, wtk_matf_t *c);

#ifdef __cplusplus
}
#endif
#endif