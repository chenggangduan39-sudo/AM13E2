#ifndef WTK_BFIO_AFILTER_WTK_RLS
#define WTK_BFIO_AFILTER_WTK_RLS
#include "wtk/core/wtk_complex.h"
#include "wtk_rls_cfg.h"
#ifdef USE_NEON
#include <arm_neon.h>
#endif
#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_rls wtk_rls_t;

struct wtk_rls
{
    wtk_rls_cfg_t *cfg;
    wtk_complex_t *xld;
#ifdef USE_NEON
    wtk_complex_t *G;
    wtk_complex_t *Q;
    wtk_complex_t *K;
    wtk_complex_t *tmp;
#else
    wtk_dcomplex_t *G;
    wtk_dcomplex_t *Q;
    wtk_dcomplex_t *K;
    wtk_dcomplex_t *tmp;
#endif
    wtk_complex_t *wx;
	float *x2;
	float *x3;
	float *phrr;
	float *phru;
	float **phrx;
	float *phrd;

    wtk_complex_t *u;
	wtk_complex_t *a;
	wtk_complex_t *z;

    float nframe;
    wtk_complex_t *out;
    wtk_complex_t *lsty;

    float lambda;
};

wtk_rls_t *wtk_rls_new(wtk_rls_cfg_t *cfg);
void wtk_rls_delete(wtk_rls_t *rls);

void wtk_rls_init(wtk_rls_t *rls, wtk_rls_cfg_t *cfg);
void wtk_rls_clean(wtk_rls_t *rls);

void wtk_rls_reset(wtk_rls_t *rls);
void wtk_rls_reset2(wtk_rls_t *rls);

void wtk_rls_feed(wtk_rls_t *rls, wtk_complex_t *in, wtk_complex_t *u);
void wtk_rls_feed2(wtk_rls_t *rls, wtk_complex_t *in, wtk_complex_t *xld);

void wtk_rls_feed3(wtk_rls_t *rls, wtk_complex_t *in, wtk_complex_t *u, int switch_b);
#ifdef __cplusplus
};
#endif
#endif