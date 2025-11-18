#ifndef WTK_BFIO_AFILTER_WTK_NLMS
#define WTK_BFIO_AFILTER_WTK_NLMS
#include "wtk/core/wtk_complex.h"
#include "wtk_nlms_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_nlms wtk_nlms_t;
struct wtk_nlms
{
    wtk_nlms_cfg_t *cfg;
    wtk_complex_t *far_x;
    wtk_complex_t *W;
    float *prop;
    float power_x;
    float power_l;

    float nframe;
    wtk_complex_t *out;
    wtk_complex_t *lsty;
    float *lsty_power;

    float *Se;
    float *Sd;
    wtk_complex_t *Sed;
    float *leak;
    float *orth_agc;
};

wtk_nlms_t *wtk_nlms_new(wtk_nlms_cfg_t *cfg);
void wtk_nlms_delete(wtk_nlms_t *nlms);

void wtk_nlms_init(wtk_nlms_t *nlms, wtk_nlms_cfg_t *cfg);
void wtk_nlms_clean(wtk_nlms_t *nlms);

void wtk_nlms_reset(wtk_nlms_t *nlms);
void wtk_nlms_reset2(wtk_nlms_t *nlms);
void wtk_nlms_feed(wtk_nlms_t *nlms, wtk_complex_t *n_near, wtk_complex_t *n_far);

void wtk_nlms_feed3(wtk_nlms_t *nlms, wtk_complex_t *n_near, wtk_complex_t *n_far, int switch_b);

#ifdef __cplusplus
};
#endif
#endif
