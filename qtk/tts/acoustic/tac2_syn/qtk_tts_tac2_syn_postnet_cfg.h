#ifndef __QTK_TTS_TAC2_SYN_POSTNET_CFG_H__
#define __QTK_TTS_TAC2_SYN_POSTNET_CFG_H__

#include "wtk/core/cfg/wtk_local_cfg.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct qtk_tts_tac2_syn_postnet_cfg{
    int num_layer;
    int num_mels;
    wtk_array_t *conv_dim_num;
    wtk_array_t *conv_kernel_size;
    char *conv_fn_format;
    char *batch_norm_fn_format;
    char **conv_kernel_fn;
    char **conv_bias_fn;
    char **batch_norm_gamma_fn;
    char **batch_norm_beta_fn;
    char **batch_norm_mean_fn;
    char **batch_norm_var_fn;
}qtk_tts_tac2_syn_postnet_cfg_t;

int qtk_tts_tac2_syn_postnet_cfg_init(qtk_tts_tac2_syn_postnet_cfg_t *cfg);
int qtk_tts_tac2_syn_postnet_cfg_clean(qtk_tts_tac2_syn_postnet_cfg_t *cfg);
int qtk_tts_tac2_syn_postnet_cfg_update_local(qtk_tts_tac2_syn_postnet_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_tts_tac2_syn_postnet_cfg_update(qtk_tts_tac2_syn_postnet_cfg_t *cfg);

#ifdef __cplusplus
};
#endif


#endif