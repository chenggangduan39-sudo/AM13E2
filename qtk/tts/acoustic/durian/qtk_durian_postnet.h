#ifndef __QTK_DURIAN_POSTNET_H__
#define __QTK_DURIAN_POSTNET_H__

#include "qtk_durian_postnet_cfg.h"
#include "wtk/core/math/wtk_mat.h"
#include "tts-mer/wtk-extend/nn/wtk_nn.h"
#include "layer/qtk_nn_conv1d.h"
#include "layer/qtk_nn_batchnorm.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct qtk_durian_postnet{
    qtk_durian_postnet_cfg_t *cfg;
    wtk_heap_t *heap;
    qtk_nn_conv1d_t **convrb_conv1d;
    qtk_nn_batchnorm_t **convrb_batchnorm;
    qtk_nn_conv1d_t **convrb_residual_conv1d;
    qtk_nn_conv1d_t *conv1d;
}qtk_durian_postnet_t;

qtk_durian_postnet_t *qtk_durian_postnet_new(qtk_durian_postnet_cfg_t *cfg,wtk_heap_t *heap);
int qtk_durian_postnet_delete(qtk_durian_postnet_t *decoder);
wtk_matf_t* qtk_durian_postnet_process(qtk_durian_postnet_t *dec,wtk_matf_t *in);

#ifdef __cplusplus
};
#endif

#endif
