#ifndef WTK_BFIO_AHS_QTK_CRNNOISE_NNRTV2
#define WTK_BFIO_AHS_QTK_CRNNOISE_NNRTV2
#include "wtk/core/wtk_complex.h"
#include "qtk/nnrt/qtk_nnrt.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_crnnoise_nnrt2 qtk_crnnoise_nnrt2_t;

struct qtk_crnnoise_nnrt2 {
    qtk_nnrt_cfg_t *cfg;
    qtk_nnrt_t *nnrt;

    qtk_nnrt_value_t cache[8];
    int64_t shape[9][5];
    float *input;
    int num_cache;
};

qtk_crnnoise_nnrt2_t *qtk_crnnoise2_new(qtk_nnrt_cfg_t *cfg);
void qtk_crnnoise2_delete(qtk_crnnoise_nnrt2_t *nrt);
void qtk_crnnoise2_reset(qtk_crnnoise_nnrt2_t *nrt);
void qtk_crnnoise2_run(qtk_crnnoise_nnrt2_t *nrt, wtk_complex_t *x, wtk_complex_t *z);
#ifdef __cplusplus
};
#endif
#endif