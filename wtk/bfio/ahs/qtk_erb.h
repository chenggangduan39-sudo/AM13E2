#ifndef WTK_BFIO_AHS_QTK_ERB
#define WTK_BFIO_AHS_QTK_ERB
#include "wtk/core/math/wtk_math.h"
#include "wtk/bfio/maskdenoise/wtk_drft.h"
#include "qtk_erb_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_ahs_erb qtk_ahs_erb_t;

struct qtk_ahs_erb {
    qtk_ahs_erb_cfg_t *cfg;
    float *filters;
    float *erb_fc;
    int erb_fc_shape[2];
    float *ierb_fc;
    int ierb_fc_shape[2];
    int n;
    int nfreqs;
};

qtk_ahs_erb_t *qtk_ahs_erb_new(qtk_ahs_erb_cfg_t *cfg);
void qtk_ahs_erb_delete(qtk_ahs_erb_t *freq_shift);
void qtk_ahs_erb_reset(qtk_ahs_erb_t *freq_shift);
void qtk_ahs_erb_bm(qtk_ahs_erb_t *erb, float *data, float * out, int cnt);
void qtk_ahs_erb_bs(qtk_ahs_erb_t *erb, float *data, float * out, int cnt);
#ifdef __cplusplus
};
#endif
#endif