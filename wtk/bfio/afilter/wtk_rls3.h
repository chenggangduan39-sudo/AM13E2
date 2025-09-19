#ifndef WTK_BFIO_AFILTER_WTK_RLS3
#define WTK_BFIO_AFILTER_WTK_RLS3
#include "wtk/core/wtk_complex.h"
#include "wtk_rls3_cfg.h"
#ifdef USE_NEON
#include <arm_neon.h>
#endif
#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_rls3 wtk_rls3_t;

struct wtk_rls3
{
    wtk_rls3_cfg_t *cfg;
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

    double *w;

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

    float *lambda;
    float *fa_1;
    float *Q_eye_n;
};

wtk_rls3_t *wtk_rls3_new(wtk_rls3_cfg_t *cfg, int nbin);
void wtk_rls3_delete(wtk_rls3_t *rls);

void wtk_rls3_init(wtk_rls3_t *rls, wtk_rls3_cfg_t *cfg, int nbin);
void wtk_rls3_clean(wtk_rls3_t *rls);

void wtk_rls3_reset(wtk_rls3_t *rls, int nbin);
void wtk_rls3_reset2(wtk_rls3_t *rls, int nbin);

void wtk_rls3_feed(wtk_rls3_t *rls, wtk_complex_t *in, wtk_complex_t *u, int nbin);
void wtk_rls3_feed2(wtk_rls3_t *rls, wtk_complex_t *in, wtk_complex_t *xld, int nbin);

void wtk_rls3_feed3(wtk_rls3_t *rls, wtk_complex_t *in, wtk_complex_t *u, int switch_b, int nbin);
#ifdef __cplusplus
};
#endif
#endif