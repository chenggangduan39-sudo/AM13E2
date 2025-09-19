#ifndef WTK_BFIO_AHS_QTK_CRNNOISE_NNRT
#define WTK_BFIO_AHS_QTK_CRNNOISE_NNRT
#include "wtk/core/wtk_complex.h"
#include "qtk/nnrt/qtk_nnrt.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_crnnoise_nnrt qtk_crnnoise_nnrt_t;

struct qtk_crnnoise_nnrt {
    qtk_nnrt_cfg_t *cfg;
    qtk_nnrt_t *nnrt;

    qtk_nnrt_value_t cache[7];
    int64_t shape[8][5];
    float *input;

    int num_cache;
};

qtk_crnnoise_nnrt_t *qtk_crnnoise_new(qtk_nnrt_cfg_t *cfg);
void qtk_crnnoise_delete(qtk_crnnoise_nnrt_t *nrt);
void qtk_crnnoise_reset(qtk_crnnoise_nnrt_t *nrt);
void qtk_crnnoise_run(qtk_crnnoise_nnrt_t *nrt, wtk_complex_t *x, wtk_complex_t *y);
#ifdef __cplusplus
};
#endif
#endif