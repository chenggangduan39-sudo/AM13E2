#ifndef __QTK_TTS_NCRF_CFG_H__
#define __QTK_TTS_NCRF_CFG_H__

#include "wtk/core/cfg/wtk_local_cfg.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct {
    int feature_num;
    int linear_num;
    int lstm_pw_num;
    int lstm_pp_num;
    int lstm_ip_num;
    int char_id_;
    char *char_embedding_fn;
    char *feature_fn_format;
    char *linear_weight_format;
    char *linear_bias_format;
    char *lstm_pw_weight_format;
    char *lstm_pw_bias_format;
    char *linear_pw_weight_fn;
    char *linear_pw_bias_fn;
    char *crf_pw_transitions_fn;
    char *pw_pred_embedding_fn;
    char *pp_fnn_weight_fn;
    char *pp_fnn_bias_fn;
    char *lstm_pp_weight_format;
    char *lstm_pp_bias_format;
    char *linear_pp_weight_fn;
    char *linear_pp_bias_fn;
    char *crf_pp_transitions_fn;
    char *pp_pred_embedding_fn;
    char *ip_fnn_weight_fn;
    char *ip_fnn_bias_fn;
    char *lstm_ip_weight_format;
    char *lstm_ip_bias_format;
    char *linear_ip_weight_fn;
    char *linear_ip_bias_fn;
    char *crf_ip_transitions_fn;
    char **feature_embedding_fn;
    char **linear_weight_fn;
    char **linear_bias_fn;
    char **lstm_pw_weight_fn;
    char **lstm_pw_bias_fn;
    char **lstm_pp_weight_fn;
    char **lstm_pp_bias_fn;
    char **lstm_ip_weight_fn;
    char **lstm_ip_bias_fn;
    wtk_array_t *char_embedding_dim;
    wtk_array_t *feature_embedding_dim;
    wtk_array_t *linear_dim;
    wtk_array_t *lstm_pw_dim;
    wtk_array_t *linear_pw_dim;
    wtk_array_t *pw_pred_embedding_dim;
    wtk_array_t *pp_fnn_dim;
    wtk_array_t *lstm_pp_dim;
    wtk_array_t *linear_pp_dim;
    wtk_array_t *pp_pred_embedding_dim;
    wtk_array_t *ip_fnn_dim;
    wtk_array_t *lstm_ip_dim;
    wtk_array_t *linear_ip_dim;
    wtk_array_t *id;
    wtk_array_t *id_;

}qtk_tts_ncrf_cfg_t;

int qtk_tts_ncrf_cfg_init(qtk_tts_ncrf_cfg_t *cfg);
int qtk_tts_ncrf_cfg_clean(qtk_tts_ncrf_cfg_t *cfg);
int qtk_tts_ncrf_cfg_update_local(qtk_tts_ncrf_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_tts_ncrf_cfg_update(qtk_tts_ncrf_cfg_t *cfg);

#ifdef __cplusplus
};
#endif

#endif
