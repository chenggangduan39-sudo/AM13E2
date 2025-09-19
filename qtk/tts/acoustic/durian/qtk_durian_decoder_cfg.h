#ifndef __QTK_DURIAN_DECODER_CFG_H__
#define __QTK_DURIAN_DECODER_CFG_H__

#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/wtk_array.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct{
    int prenet_layers_dim;
    int decoder_rnn_dim;
    int decoder_output_size;
    int num_mels;
    // wtk_array_t *gru_dim_num;
    // wtk_array_t *projection_dim_num;
    wtk_array_t *lstm_dim_num;
    char *prenet_layer_fn_format;
    char **prenet_layer_weight_fn;
    // char *gru_fn_format;
    // char **gru_gate_weight_fn;
    // char **gru_candidate_weight_fn;
    // char **gru_candidate_hh_weight_fn;
    // char **gru_gate_bias_fn;
    // char **gru_candidate_bias_fn;
    // char **gru_candidate_hh_bias_fn;
    char *lstm_fn_format;
    char **lstm_weight_fn;
    char **lstm_bias_fn;
    // char *projection_fn_format;
    // char *projection_weight_fn;
    // char *projection_bias_fn;
    char *conv1d_fn_format;
    char *conv1d_weight_fn;
    char *conv1d_bias_fn;
}qtk_durian_decoder_cfg_t;

int qtk_durian_decoder_cfg_init(qtk_durian_decoder_cfg_t *cfg);
int qtk_durian_decoder_cfg_clean(qtk_durian_decoder_cfg_t *cfg);
int qtk_durian_decoder_cfg_update_local(qtk_durian_decoder_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_durian_decoder_cfg_update(qtk_durian_decoder_cfg_t *cfg);



#ifdef __cplusplus
};
#endif


#endif