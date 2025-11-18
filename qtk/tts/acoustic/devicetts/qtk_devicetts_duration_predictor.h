#ifndef __QTK_DEVICETTS_DURATION_PREDICTOR_H__
#define __QTK_DEVICETTS_DURATION_PREDICTOR_H__

#include "qtk_devicetts_duration_predictor_cfg.h"
#include "tts-mer/wtk-extend/wtk_mat2.h"
#include "qtk/layer/qtk_nn_conv1d.h"
#include "qtk/layer/qtk_nn_layernorm.h"
#include "qtk/layer/qtk_nn_fc.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct qtk_devicetts_duration_predictor{
    qtk_devicetts_duration_predictor_cfg_t *cfg;
    wtk_heap_t *heap;

    qtk_nn_conv1d_t **conv;
    qtk_nn_layernorm_t **layernorm;
    qtk_nn_fc_t *fc;
    // int num_pad_for_mix_resolution;
}qtk_devicetts_duration_predictor_t;

qtk_devicetts_duration_predictor_t *qtk_devicetts_duration_predictor_new(qtk_devicetts_duration_predictor_cfg_t *cfg,wtk_heap_t *heap);
wtk_matf_t* qtk_devicetts_duration_predictor_forward(qtk_devicetts_duration_predictor_t *dur,wtk_matf_t *in);
int qtk_devicetts_duration_predictor_delete(qtk_devicetts_duration_predictor_t *);

#ifdef __cplusplus
};
#endif

#endif
