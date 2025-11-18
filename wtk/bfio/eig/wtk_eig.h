#ifndef WTK_BFIO_EIG_WTK_EIG
#define WTK_BFIO_EIG_WTK_EIG
#include "wtk/core/wtk_complex.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_eig wtk_eig_t;

typedef struct { float r, i; } f2c_complex;
typedef struct { double r, i; } f2c_doublecomplex;
typedef struct { double real, imag; } npy_cdouble;
typedef int               fortran_int;
typedef float             fortran_real;
typedef double            fortran_doublereal;
typedef f2c_complex       fortran_complex;
typedef f2c_doublecomplex fortran_doublecomplex;

typedef union {
    fortran_doublecomplex f;
    npy_cdouble npy;
    double array[2];
} DOUBLECOMPLEX_t;


struct wtk_eig
{
    void *A;
    void *WR; /* RWORK in complex versions, REAL W buffer for (sd)geev*/
    void *WI;
    void *VLR; /* REAL VL buffers for _geev where _ is s, d */
    void *VRR; /* REAL VR buffers for _geev hwere _ is s, d */
    void *WORK;
    void *W;  /* final w */
    void *VR; /* final vr */

    fortran_int N;
    fortran_int LDA;
    fortran_int LDVL;
    fortran_int LDVR;
    fortran_int LWORK;

    char JOBVL;
    char JOBVR;
};

wtk_eig_t* wtk_eig_new(int n);
void wtk_eig_delete(wtk_eig_t *eig);

int wtk_eig_process_maxfv(wtk_eig_t *eig,wtk_complex_t *v,wtk_complex_t *vector);
int wtk_eig_process_fv(wtk_eig_t *eig,wtk_complex_t *v,wtk_complex_t *fv, float *val);
int wtk_eig_process_fv2(wtk_eig_t *eig,wtk_complex_t *v,wtk_complex_t *fv, float *val);
int wtk_eig_process2_matlab(wtk_eig_t *eig,wtk_complex_t *v,wtk_complex_t *fv);

int wtk_eig_process_all_eig3(wtk_eig_t *eig,wtk_complex_t *v,wtk_complex_t *eig_v,float *rk_v,float *rk_vi);

#ifdef __cplusplus
};
#endif
#endif
