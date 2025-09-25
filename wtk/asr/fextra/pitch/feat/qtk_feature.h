#ifndef __QTK_FEAT_QTK_FEATURE_H__
#define __QTK_FEAT_QTK_FEATURE_H__
#include "wtk/asr/fextra/pitch/feat/qtk_feature_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_feature_delta qtk_feature_delta_t;

struct qtk_feature_delta {
    qtk_feature_delta_cfg_t *cfg;
    float **scales;
    int *scales_dim;
};

qtk_feature_delta_t *qtk_feature_delta_new(qtk_feature_delta_cfg_t *cfg);
void qtk_feature_delta_delete(qtk_feature_delta_t *delta);
int qtk_feature_delta_process(qtk_feature_delta_t *delta, float *input,
                              float *output, int irow, int icol, int frame);

#ifdef __cplusplus
};
#endif
#endif
