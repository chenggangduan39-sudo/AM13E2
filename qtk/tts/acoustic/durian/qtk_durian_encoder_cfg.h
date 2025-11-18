#ifndef __QTK_DURIAN_ENCODER_CFG_H__
#define __QTK_DURIAN_ENCODER_CFG_H__

#include "wtk/core/cfg/wtk_local_cfg.h"

//encode of durian

#ifdef __cplusplus
extern "C"{
#endif

typedef struct qtk_durian_encoder_cfg{
    //暂时没有必要把cbhg搞开
    int embedding_input_size;   //embedding dim
    int cbhg_max_kernel_size;   //cbhg的最大kernel数
    int cbhg_num_highway_layers;
    wtk_array_t *cbhg_projection_sizes;
    wtk_array_t *pre_highway_dim;
    char *conv1d_bank_fn_format;
    char **conv1d_bank_conv_fn;
    char **conv1d_bank_batchnorm_gamma_fn;
    char **conv1d_bank_batchnorm_beta_fn;
    char **conv1d_bank_batchnorm_var_fn;
    char **conv1d_bank_batchnorm_mean_fn;
    char *conv1d_projections_fn_format;
    char **conv1d_prodections_conv_fn;
    char **conv1d_prodections_batchnorm_gamma_fn;
    char **conv1d_prodections_batchnorm_beta_fn;
    char **conv1d_prodections_batchnorm_var_fn;
    char **conv1d_prodections_batchnorm_mean_fn;
    char *pre_highway_fn;
    char *highway_fn_format;
    char **highway_H_weight_fn;
    char **highway_H_bias_fn;
    char **highway_T_weight_fn;
    char **highway_T_bias_fn;
    char *rnn_fn_format;
    char **gru_gate_weight_fn;
    char **gru_candidate_weight_fn;
    char **gru_candidate_hh_weight_fn;
    char **gru_gate_bias_fn;
    char **gru_candidate_bias_fn;
    char **gru_candidate_hh_bias_fn;
}qtk_durian_encoder_cfg_t;

int qtk_durian_encoder_cfg_init(qtk_durian_encoder_cfg_t *cfg);
int qtk_durian_encoder_cfg_clean(qtk_durian_encoder_cfg_t *cfg);
int qtk_durian_encoder_cfg_update_local(qtk_durian_encoder_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_durian_encoder_cfg_update(qtk_durian_encoder_cfg_t *cfg);

#ifdef __cplusplus
};
#endif

#endif