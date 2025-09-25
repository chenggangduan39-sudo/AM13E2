#ifndef __QTK_DEVICETTS_DECODER_H__
#define __QTK_DEVICETTS_DECODER_H__

#include "qtk_devicetts_decoder_cfg.h"
#include "wtk/core/math/wtk_mat.h"
#include "layer/qtk_nn_fc.h"
#include "tts-mer/wtk-extend/nn/wtk_nn.h"
#include "layer/qtk_nn_conv1d.h"
#include "wtk/asr/fextra/torchnn/qtk_torchnn_cal.h"
#include "qtk_devicetts_postnet.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct{
    qtk_devicetts_decoder_cfg_t *cfg;
    wtk_heap_t *heap;

    qtk_nn_fc_t **prenet_layer;
    wtk_nn_rnngru_t **gru;
    qtk_nn_fc_t *projection;
    qtk_nn_conv1d_t *conv1d;
    qtk_devicetts_postnet_t *conv_mgc;
    qtk_nn_fc_t *linear_bap;
    qtk_nn_fc_t *linear_lf0;
    qtk_nn_fc_t *linear_vuv;
}qtk_devicetts_decoder_t;

qtk_devicetts_decoder_t *qtk_devicetts_decoder_new_lpcnet(qtk_devicetts_decoder_cfg_t *cfg,wtk_heap_t *heap);
wtk_matf_t* qtk_devicetts_decoder_process_lpcnet(qtk_devicetts_decoder_t *dec,wtk_matf_t *aligned_features);
int qtk_devicetts_decoder_delete(qtk_devicetts_decoder_t *decoder);
qtk_devicetts_decoder_t *qtk_devicetts_decoder_new_world(qtk_devicetts_decoder_cfg_t *cfg,wtk_heap_t *heap);
wtk_matf_t* qtk_devicetts_decoder_process_world(qtk_devicetts_decoder_t *dec,wtk_matf_t *aligned_features);

wtk_matf_t* qtk_devicetts_decode_getlinear(qtk_devicetts_decoder_t *dec, wtk_matf_t* in, qtk_nn_fc_t* fc);
wtk_matf_t* qtk_devicetts_decode_getPostnet(qtk_devicetts_decoder_t *dec, wtk_matf_t* in, qtk_devicetts_postnet_t * postnet);
#ifdef __cplusplus
};
#endif

#endif
