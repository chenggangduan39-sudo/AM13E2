#ifndef __QTK_TTS_TAC2_SYN_ENC_CFG_H__
#define __QTK_TTS_TAC2_SYN_ENC_CFG_H__

#include "wtk/core/cfg/wtk_local_cfg.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct qtk_tts_tac2_syn_enc_cfg{
    int conv_layer;
    char *conv_fn_format;
    char *lstm_fn_format;
    char *batch_norm_fn_format;
    wtk_array_t *conv_dim_num;
    wtk_array_t *conv_kernel_sizes;
    wtk_array_t *lstm_dim_num;
    char **conv_kernel_fn;
    char **conv_bias_fn;
    char **gamma_fn;
    char **beta_fn;
    char **moving_mean_fn;
    char **moving_variance_fn;
    char *lstm_fw_kernel_fn;
    char *lstm_fw_bias_fn;
    char *lstm_bw_kernel_fn;
    char *lstm_bw_bias_fn;
}qtk_tts_tac2_syn_enc_cfg_t;

int qtk_tts_tac2_syn_enc_cfg_init(qtk_tts_tac2_syn_enc_cfg_t *cfg);
int qtk_tts_tac2_syn_enc_cfg_clean(qtk_tts_tac2_syn_enc_cfg_t *cfg);
int qtk_tts_tac2_syn_enc_cfg_update_local(qtk_tts_tac2_syn_enc_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_tts_tac2_syn_enc_cfg_update(qtk_tts_tac2_syn_enc_cfg_t *cfg);


#ifdef __cplusplus
};
#endif

#endif