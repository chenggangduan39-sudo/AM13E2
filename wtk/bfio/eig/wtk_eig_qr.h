#ifndef WTK_BFIO_EIG_WTK_EIG_QR
#define WTK_BFIO_EIG_WTK_EIG_QR
#include "wtk/core/wtk_complex.h"
#ifdef __cplusplus
extern "C" {
#endif

// Hermite  eig
int wtk_eig_qr_householder_process(wtk_complex_t *a,double *fv,double *b,double *c,int n,wtk_complex_t *fvij,double *val);

#ifdef __cplusplus
}
#endif
#endif
