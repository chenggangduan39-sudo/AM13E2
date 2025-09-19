#ifndef __QTK_TTS_TAC2_SYN_ENC_H__
#define __QTK_TTS_TAC2_SYN_ENC_H__

#include "qtk_tts_tac2_syn_enc_cfg.h"
#include "tts-mer/wtk-extend/wtk_mat2.h"
#include "tts-mer/wtk-extend/nn/wtk_nn.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct qtk_tts_tac2_syn_enc{
    qtk_tts_tac2_syn_enc_cfg_t *enc_cfg;
    wtk_heap_t *heap;   //指针需要外部传入
    //enc   可能使用dfsnm替换原来的enc
    wtk_matf_t **enc_conv_kernel;
    wtk_vecf_t **enc_conv_bias;
    wtk_vecf_t **enc_gamma;
    wtk_vecf_t **enc_beta;
    wtk_vecf_t **enc_mean;
    wtk_vecf_t **enc_variance;
    wtk_nn_lstm_t *enc_lstm_fw;
    wtk_nn_lstm_t *enc_lstm_bw;
}qtk_tts_tac2_syn_enc_t;

qtk_tts_tac2_syn_enc_t* qtk_tts_tac2_syn_encoder_new(qtk_tts_tac2_syn_enc_cfg_t *cfg,wtk_heap_t *heap);
int qtk_tts_tac2_syn_encode_process(qtk_tts_tac2_syn_enc_t *syn,wtk_matf_t *in,wtk_matf_t *out);
void qtk_tts_tac2_syn_encode_delete(qtk_tts_tac2_syn_enc_t *syn);

#ifdef __cplusplus
};
#endif

#endif
