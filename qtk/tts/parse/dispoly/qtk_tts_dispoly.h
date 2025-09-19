#ifndef __QTK_TTS_DISPOLY_H__
#define __QTK_TTS_DISPOLY_H__

#include "qtk_tts_dispoly_cfg.h"
#include "tts-mer/wtk-extend/wtk_mat2.h"
#include "tts-mer/wtk-extend/nn/wtk_nn.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct qtk_tts_dispoly{
    wtk_heap_t *heap;
    
    qtk_tts_dispoly_cfg_t *cfg;
    wtk_matf_t *word_embedding;
    wtk_matf_t *cws_embedding;
    wtk_matf_t *pp_embedding;
    wtk_matf_t *flag_embedding;
    wtk_matf_t *poly_mask;
    wtk_matf_t **mfnn_weight;
    wtk_vecf_t **mfnn_bias;
    wtk_matf_t **conv_weight;
    wtk_vecf_t **conv_bias;
    wtk_vecf_t **conv_layernorm_weight;
    wtk_vecf_t **conv_layernorm_bias;
    wtk_nn_lstm_t **lstm;
    wtk_nn_lstm_t **lstm_reverse;
    wtk_matf_t **fnn_weight;
    wtk_vecf_t **fnn_bias;
}qtk_tts_dispoly_t;

qtk_tts_dispoly_t *qtk_tts_dispoly_new(qtk_tts_dispoly_cfg_t *cfg);
qtk_tts_dispoly_t *qtk_tts_dispoly_new2(qtk_tts_dispoly_cfg_t *cfg, wtk_rbin2_t* rbin);
int qtk_tts_dispoly_process(qtk_tts_dispoly_t *disply,wtk_veci_t **vecs,wtk_matf_t *out);
int qtk_tts_dispoly_delete(qtk_tts_dispoly_t *disply);


#ifdef __cplusplus
};
#endif

#endif
