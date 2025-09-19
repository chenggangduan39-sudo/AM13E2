#ifndef __QTK_DURIAN_DECODER_H__
#define __QTK_DURIAN_DECODER_H__

#include "qtk_durian_decoder_cfg.h"
#include "wtk/core/math/wtk_mat.h"
#include "layer/qtk_nn_fc.h"
#include "layer/qtk_nn_conv1d.h"
#include "layer/qtk_nn_lstm.h"
#include "random/qtk_random.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct{
    qtk_durian_decoder_cfg_t *cfg;
    wtk_heap_t *heap;
    qtk_random_t *r;

    qtk_nn_fc_t **prenet_layer;
    qtk_nn_lstm_t **rnn_lstm;
    // qtk_nn_fc_t *projection;
    qtk_nn_conv1d_t *conv1d;
}qtk_durian_decoder_t;

qtk_durian_decoder_t *qtk_durian_decoder_new(qtk_durian_decoder_cfg_t *cfg,wtk_heap_t *heap);
int qtk_durian_decoder_delete(qtk_durian_decoder_t *decoder);
wtk_matf_t* qtk_durian_decoder_process(qtk_durian_decoder_t *dec,wtk_matf_t *aligned_features);

#ifdef __cplusplus
};
#endif

#endif
