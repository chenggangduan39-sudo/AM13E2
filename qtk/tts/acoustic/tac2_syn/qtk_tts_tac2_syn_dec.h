#ifndef __QTK_TTS_TAC2_SYN_DEC_H__
#define __QTK_TTS_TAC2_SYN_DEC_H__

#include "qtk_tts_tac2_syn_dec_cfg.h"
#include "tts-mer/wtk-extend/wtk_mat2.h"
#include "tts-mer/wtk-extend/nn/wtk_nn.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef void(*qtk_tts_tac2_syn_dec_notify_f)(void *user_data,wtk_matf_t *m,int is_end);

typedef struct {
    qtk_tts_tac2_syn_dec_cfg_t *cfg;
    wtk_heap_t *heap;
    //dec
    wtk_matf_t **dec_prenet_kernel;
    wtk_vecf_t **dec_prenet_bias;
    wtk_matf_t *dec_prenet_catch;

    wtk_nn_rnngru_t **dec_gru;
    wtk_matf_t *dec_linear_kernel;
    wtk_vecf_t *dec_linear_bias;
    wtk_matf_t *dec_stop_kernel;
    wtk_vecf_t *dec_stop_bias;
    //atten
    wtk_matf_t *dec_atten_mf;
    wtk_matf_t *dec_enc_transpose;
    wtk_matf_t *dec_atten_memory_kernel;
    wtk_vecf_t *dec_atten_memory_bias;
    wtk_matf_t *dec_atten_quern_kernel;
    wtk_vecf_t *dec_atten_quern_bias;
    wtk_matf_t *dec_atten_conv_kernel;
    wtk_vecf_t *dec_atten_conv_bias;
    wtk_matf_t *dec_atten_conv_out;
    wtk_matf_t *dec_atten_features_kernel;
    wtk_vecf_t *dec_atten_var_project;
    wtk_vecf_t *dec_atten_var_project_bias;
    wtk_matf_t *dec_atten_mo;   //memory out
    wtk_matf_t *dec_atten_qo;    //quern out
    wtk_matf_t *dec_atten_fo;      //feature out
    wtk_vecf_t *dec_prev_align;
    wtk_vecf_t *dec_new_align;
    
    void *use_data;
    qtk_tts_tac2_syn_dec_notify_f notify;
}qtk_tts_tac2_syn_dec_t;

qtk_tts_tac2_syn_dec_t* qtk_tts_tac2_syn_decoder_new(qtk_tts_tac2_syn_dec_cfg_t *cfg,wtk_heap_t *heap);
int qtk_tts_tac2_syn_decoder_delete(qtk_tts_tac2_syn_dec_t *syn);
int qtk_tts_tac2_syn_decoder_process(qtk_tts_tac2_syn_dec_t *syn,wtk_matf_t *in,int is_end);
int qtk_tts_tac2_syn_decoder_set_notify(qtk_tts_tac2_syn_dec_t *syn,void *user_data,qtk_tts_tac2_syn_dec_notify_f notify);

#ifdef __cplusplus
};
#endif

#endif
