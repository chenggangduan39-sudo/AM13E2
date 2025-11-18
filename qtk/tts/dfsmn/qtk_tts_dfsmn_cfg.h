#ifndef __QTK_TTS_DFSMN_CFG_H__
#define __QTK_TTS_DFSMN_CFG_H__

#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/wtk_array.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct qtk_tts_dfsmn_cfg{
    int dfsmn_layer;
    int conv_left_groups;
    int conv_right_groups;
    wtk_array_t *linear_affine_dim_num;
    wtk_array_t *linear_projection_dim_num;
    char *linear_fn_format;
    wtk_array_t *left_pad_num;
    wtk_array_t *left_kernel_size_num;
    wtk_array_t *left_weight_dim_num;
    wtk_array_t *right_pad_num;
    wtk_array_t *right_kernel_size_num;
    wtk_array_t *right_weight_dim_num;
    wtk_array_t *rnn_gru_dim_num;
    char *conv_fn_format;
    char *batch_norm_fn_format;
    char *gru_fn_format;
    char **linear_affine_weight_fn;
    char **linear_affine_bias_fn;
    char **linear_projection_weight_fn;
    char **linear_projection_bias_fn;
    char **conv_left_weight_fn;
    char **conv_right_weight_fn;
    char **batch_norm_gamma_fn;
    char **batch_norm_beta_fn;
    char **batch_norm_mean_fn;
    char **batch_norm_var_fn;
    char **gru_weight_gate_fn;
    char **gru_weight_cand_fn;
    char **gru_weight_cand_hh_fn;
    char **gru_bias_gate_fn;
    char **gru_bias_cand_fn;
    char **gru_bias_cand_hh_fn;
}qtk_tts_dfsmn_cfg_t;

int qtk_tts_dfsmn_cfg_init(qtk_tts_dfsmn_cfg_t *cfg);
int qtk_tts_dfsmn_cfg_clean(qtk_tts_dfsmn_cfg_t *cfg);
int qtk_tts_dfsmn_cfg_update_local(qtk_tts_dfsmn_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_tts_dfsmn_cfg_update(qtk_tts_dfsmn_cfg_t *cfg);


#ifdef __cplusplus
};
#endif

#endif