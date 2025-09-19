#ifndef WTK_BFIO_ASPEC_WTK_ASPEC
#define WTK_BFIO_ASPEC_WTK_ASPEC
#include "wtk_aspec_cfg.h"
#include "wtk_gccspec.h"
#include "wtk_ml.h"
#include "wtk_gccspec2.h"
#include "wtk_ngccspec2.h"
#include "wtk_dnmspec2.h"
#include "wtk_dnmspec.h"
#include "wtk_mvdrspec.h"
#include "wtk_mvdrspec2.h"
#include "wtk_mvdrwspec.h"
#include "wtk_mvdrwspec2.h"
#include "wtk_dsspec.h"
#include "wtk_dsspec2.h"
#include "wtk_dswspec.h"
#include "wtk_dswspec2.h"
#include "wtk_musicspec2.h"
#include "wtk_zdsspec.h"
#include "wtk_zdswspec.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef float (*wtk_aspec_flush_spec_k_f)(void *ths, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx);

typedef void (*wtk_aspec_flush_spec_k_f2)(void *ths, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx, float *spec);

typedef struct wtk_aspec wtk_aspec_t;
struct wtk_aspec
{
    wtk_aspec_cfg_t *cfg;
    
    int nbin;
    int start_ang_num;

    wtk_gccspec_t *gccspec;
    wtk_ml_t *ml;
    wtk_gccspec2_t *gccspec2;
    wtk_ngccspec2_t *ngccspec2;
    wtk_dnmspec_t *dnmspec;
    wtk_dnmspec2_t *dnmspec2;
    wtk_mvdrspec_t *mvdrspec;
    wtk_mvdrspec2_t *mvdrspec2;
    wtk_mvdrwspec_t *mvdrwspec;
    wtk_mvdrwspec2_t *mvdrwspec2;
    wtk_dsspec_t *dsspec;
    wtk_dsspec2_t *dsspec2;
    wtk_dswspec_t *dswspec;
    wtk_dswspec2_t *dswspec2;
    wtk_musicspec2_t *musicspec2;
    wtk_zdsspec_t *zdsspec;
    wtk_zdswspec_t *zdswspec;

    void *ths;
    wtk_aspec_flush_spec_k_f flush_spec_f;
    wtk_aspec_flush_spec_k_f2 flush_spec_f2;

    unsigned need_cov:1;
    unsigned need_cov_travg:1;
    unsigned need_inv_cov:1;
};

wtk_aspec_t* wtk_aspec_new(wtk_aspec_cfg_t *cfg, int nbin, int ang_num);

void wtk_aspec_delete(wtk_aspec_t *aspec);

void wtk_aspec_reset(wtk_aspec_t *aspec);

void wtk_aspec_start(wtk_aspec_t *aspec, float theta, float phi, int ang_idx);

void wtk_aspec_start2(wtk_aspec_t *aspec, float x, float y, float z, int ang_idx);

float wtk_aspec_flush_spec_k(wtk_aspec_t *aspec, wtk_complex_t **fft, float fftabs2, float cov_travg, wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx);

void wtk_aspec_flush_spec_k2(wtk_aspec_t *aspec, wtk_complex_t **fft, float fftabs2, float cov_travg, wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx, float *spec);

#ifdef __cplusplus
};
#endif
#endif