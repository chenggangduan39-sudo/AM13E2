#ifndef WTK_BFIO_MASKASPEC_WTK_MASKASPEC
#define WTK_BFIO_MASKASPEC_WTK_MASKASPEC
#include "wtk_maskaspec_cfg.h"
#include "wtk_maskmvdrspec.h"
#include "wtk_maskdsspec.h"
#include "wtk_maskzdsspec.h"
#include "wtk_maskgccspec.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_maskaspec wtk_maskaspec_t;
struct wtk_maskaspec
{
    wtk_maskaspec_cfg_t *cfg;
    
    int nbin;
    int start_ang_num;

    wtk_maskmvdrspec_t *maskmvdrspec;
    wtk_maskdsspec_t *maskdsspec;
    wtk_maskzdsspec_t *maskzdsspec;
    wtk_maskgccspec_t *maskgccspec;

    unsigned need_cov:1;
    unsigned need_inv_ncov:1;
};

wtk_maskaspec_t* wtk_maskaspec_new(wtk_maskaspec_cfg_t *cfg, int nbin, int ang_num);

void wtk_maskaspec_delete(wtk_maskaspec_t *maskaspec);

void wtk_maskaspec_reset(wtk_maskaspec_t *maskaspec);

void wtk_maskaspec_start(wtk_maskaspec_t *maskaspec, float theta, float phi, int ang_idx);

void wtk_maskaspec_start2(wtk_maskaspec_t *maskaspec, float theta, float phi, int ang_idx);

float wtk_maskaspec_flush_spec_k(wtk_maskaspec_t *maskaspec, wtk_complex_t *fft, wtk_complex_t *scov,wtk_complex_t *ncov, wtk_complex_t *inv_cov, float prob, int k, int ang_idx);

void wtk_maskaspec_flush_spec_k2(wtk_maskaspec_t *maskaspec, wtk_complex_t *fft, wtk_complex_t *scov,wtk_complex_t *ncov, wtk_complex_t *inv_cov, float prob, int k, int ang_idx, float *spec);
#ifdef __cplusplus
};
#endif
#endif