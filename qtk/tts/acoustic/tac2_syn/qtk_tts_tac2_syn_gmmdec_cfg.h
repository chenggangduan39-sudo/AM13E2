#ifndef __QTK_TTS_TAC2_SYN_GMMDEC_CFG_H__
#define __QTK_TTS_TAC2_SYN_GMMDEC_CFG_H__

#include "wtk/core/cfg/wtk_local_cfg.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct {
    int prenet_layer;
    int num_mels;
    int rnn_size;
    int rnn_n;
    wtk_array_t *prenet_dim_num;
    wtk_array_t *atten_rnn_dim_num;
    wtk_array_t *atten_layer_dim_num;
    wtk_array_t *linear_dim_num;
    wtk_array_t *rnn_dim_num;
    wtk_array_t *proj_dim_num;
    char *prenet_fn_format;
    char **prenet_kernel_fn;
    char **prenet_bias_fn;
    char *atten_fn_format;
    char *atten_rnn_weight_fn;
    char *atten_rnn_bias_fn;
    char *atten_query_weight_fn;
    char *atten_query_bias_fn;
    char *atten_v_weight_fn;
    char *linear_fn_format;
    char *linear_weight_fn;
    char *linear_bias_fn;
    char *rnn_fn_format;
    char **rnn_weight_fn;
    char **rnn_bias_fn;
    char *proj_fn_format;
    char *proj_weight_fn;
}qtk_tts_tac2_syn_gmmdec_cfg_t;

int qtk_tts_tac2_syn_gmmdec_cfg_init(qtk_tts_tac2_syn_gmmdec_cfg_t *cfg);
int qtk_tts_tac2_syn_gmmdec_cfg_clean(qtk_tts_tac2_syn_gmmdec_cfg_t *cfg);
int qtk_tts_tac2_syn_gmmdec_cfg_update_local(qtk_tts_tac2_syn_gmmdec_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_tts_tac2_syn_gmmdec_cfg_update(qtk_tts_tac2_syn_gmmdec_cfg_t *cfg);

#ifdef __cplusplus
};
#endif

#endif