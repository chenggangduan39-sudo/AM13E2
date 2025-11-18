#ifndef __QTK_DEVICETTS_POSTNET_H__
#define __QTK_DEVICETTS_POSTNET_H__

#include "qtk_devicetts_postnet_cfg.h"
#include "wtk/core/math/wtk_mat.h"
#include "tts-mer/wtk-extend/nn/wtk_nn.h"
#include "qtk/layer/qtk_nn_conv1d.h"
#include "qtk/layer/qtk_nn_batchnorm.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct qtk_devicetts_postnet{
    qtk_devicetts_postnet_cfg_t *cfg;
    wtk_heap_t *heap;
    qtk_nn_conv1d_t **convrb_conv1d;
    qtk_nn_batchnorm_t **convrb_batchnorm;
    qtk_nn_conv1d_t **convrb_residual_conv1d;
    qtk_nn_conv1d_t *conv1d;
}qtk_devicetts_postnet_t;

qtk_devicetts_postnet_t *qtk_devicetts_postnet_new(qtk_devicetts_postnet_cfg_t *cfg,wtk_heap_t *heap);
int qtk_devicetts_postnet_delete(qtk_devicetts_postnet_t *decoder);
wtk_matf_t* qtk_devicetts_postnet_process(qtk_devicetts_postnet_t *dec,wtk_matf_t *in);

#ifdef __cplusplus
};
#endif

#endif
