#ifndef WTK_BFIO_AFILTER_WTK_RLS2
#define WTK_BFIO_AFILTER_WTK_RLS2
#include "wtk/core/wtk_complex.h"
#include "wtk_rls2_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_rls2 wtk_rls2_t;

struct wtk_rls2
{
    wtk_rls2_cfg_t *cfg;
    wtk_complex_t *xld;
    wtk_complex_t *G;
    wtk_complex_t *Q;
    wtk_complex_t *K;
    wtk_complex_t *tmp;

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

wtk_rls2_t *wtk_rls2_new(wtk_rls2_cfg_t *cfg);
void wtk_rls2_delete(wtk_rls2_t *rls2);

void wtk_rls2_init(wtk_rls2_t *rls2, wtk_rls2_cfg_t *cfg);
void wtk_rls2_clean(wtk_rls2_t *rls2);

void wtk_rls2_reset(wtk_rls2_t *rls2);
void wtk_rls2_reset2(wtk_rls2_t *rls2);

void wtk_rls2_feed(wtk_rls2_t *rls2, wtk_complex_t *in, wtk_complex_t *u);
void wtk_rls2_feed2(wtk_rls2_t *rls2, wtk_complex_t *in, wtk_complex_t *xld);
#ifdef __cplusplus
};
#endif
#endif