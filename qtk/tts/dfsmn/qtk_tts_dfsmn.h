#ifndef __QTK_TTS_DFSMN_H__
#define __QTK_TTS_DFSMN_H__

#include "qtk_tts_dfsmn_cfg.h"
#include "tts-mer/wtk-extend/wtk_mat2.h"
#include "tts-mer/wtk-extend/nn/wtk_nn.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct qtk_tts_dfsmn{
    qtk_tts_dfsmn_cfg_t *cfg;
    wtk_heap_t *heap;
    //block
    wtk_matf_t **linear_affine_weight;
    wtk_vecf_t **linear_affine_bias;
    wtk_matf_t **linear_projection_weight;
    wtk_vecf_t **linear_projection_bias;
    wtk_matf_t **conv_left_kernel_weight;
    wtk_matf_t **conv_right_kernel_weight;
    wtk_vecf_t **batch_norm_gamma;
    wtk_vecf_t **batch_norm_beta;
    wtk_vecf_t **batch_norm_mean;
    wtk_vecf_t **batch_norm_var;
    wtk_nn_rnngru_t **rnn_gru;
}qtk_tts_dfsmn_t;

qtk_tts_dfsmn_t* qtk_tts_dfsmn_new(qtk_tts_dfsmn_cfg_t *cfg,wtk_heap_t *heap);
int qtk_tts_dfsmn_process(qtk_tts_dfsmn_t *dfs,wtk_matf_t *in,wtk_matf_t *out);
int qtk_tts_dfsmn_delete(qtk_tts_dfsmn_t *dfs);

#ifdef __cplusplus
};
#endif

#endif
