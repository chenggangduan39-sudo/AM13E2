#ifndef __QTK_TTS_TAC2_SYN_DEC_CFG_H__
#define __QTK_TTS_TAC2_SYN_DEC_CFG_H__

#include "wtk/core/cfg/wtk_local_cfg.h"

typedef struct qtk_tts_tac2_syn_dec_cfg{
    int prenet_layer;
    int rnn_layer;
    int num_mels;
    wtk_array_t *prenet_dim_num;
    char *prenet_fn_format;
    char **prenet_kernel_fn;
    char **prenet_bias_fn;
    wtk_array_t *rnn_dim_num;
    char *gru_fn_format;
    char **gru_kernel_gate;
    char **gru_kernel_cand;
    char **gru_kernel_cand_hh;
    char **gru_bias_gate;
    char **gru_bias_cand;
    char **gru_bias_cand_hh;
    wtk_array_t *linear_dim_num;
    char *linear_kernel_fn;
    char *linear_bias_fn;
    wtk_array_t *stop_dim_num;
    char *stop_kernel_fn;
    char *stop_bias_fn;
    //atten
    int atten_channel;
    int atten_dim;
    int atten_kernel;
    int atten_filter;
    int atten_query_nrow;
    int atten_per_step;
    char *atten_query_fn_format;
    char *atten_query_kernel_fn;
    char *atten_query_bias_fn;
    char *atten_memory_kernel_fn;
    char *atten_memory_bias_fn;
    char *atten_conv_fn_format;
    char *atten_conv_kernel_fn;
    char *atten_conv_bias_fn;
    char *atten_features_kernel_fn;
    char *atten_var_project_fn;
    char *atten_var_project_bias_fn;

    unsigned int use_gru:1;
}qtk_tts_tac2_syn_dec_cfg_t;

int qtk_tts_tac2_syn_dec_cfg_init(qtk_tts_tac2_syn_dec_cfg_t *cfg);
int qtk_tts_tac2_syn_dec_cfg_clean(qtk_tts_tac2_syn_dec_cfg_t *cfg);
int qtk_tts_tac2_syn_dec_cfg_update_local(qtk_tts_tac2_syn_dec_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_tts_tac2_syn_dec_cfg_update(qtk_tts_tac2_syn_dec_cfg_t *cfg);

#endif