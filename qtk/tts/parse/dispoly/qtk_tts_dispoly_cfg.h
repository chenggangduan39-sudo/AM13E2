#ifndef __QTK_TTS_DISPOLY_CFG_H__
#define __QTK_TTS_DISPOLY_CFG_H__

#include "wtk/core/cfg/wtk_local_cfg.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct qtk_tts_dispoly_cfg{
    int mfnn_num;
    int conv_num;
    int lstm_num;
    int fnn_num;
    char *word_embedding_fn;
    char *cws_embedding_fn;
    char *flag_embedding_fn;
    char *pp_embedding_fn;
    char *poly_mask_fn;
    char *mfnn_fn_format;
    char *conv_fn_format;
    char *conv_layernorm_fn_format;
    char *lstm_fn_format;
    char *fnn_fn_format;
    char **mfnn_weight_fn;
    char **mfnn_bias_fn;
    char **conv_weight_fn;
    char **conv_bias_fn;
    char **conv_layernorm_weight_fn;
    char **conv_layernorm_bias_fn;
    char **lstm_weight_fn;
    char **lstm_bias_fn;
    char **lstm_weight_reverse_fn;
    char **lstm_bias_reverse_fn;
    char **fnn_weight_fn;
    char **fnn_bias_fn;
    wtk_array_t *word_embedding_dim;
    wtk_array_t *cws_embedding_dim;
    wtk_array_t *flag_embedding_dim;
    wtk_array_t *pp_embedding_dim;
    wtk_array_t *poly_mask_dim;
    wtk_array_t *mfnn_dim;
    wtk_array_t *conv_dim;
    wtk_array_t *conv_size_dim;
    wtk_array_t *lstm_dim;
    wtk_array_t *fnn_dim;
}qtk_tts_dispoly_cfg_t;

int qtk_tts_dispoly_cfg_init(qtk_tts_dispoly_cfg_t *cfg);
int qtk_tts_dispoly_cfg_clean(qtk_tts_dispoly_cfg_t *cfg);
int qtk_tts_dispoly_cfg_update_local(qtk_tts_dispoly_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_tts_dispoly_cfg_update(qtk_tts_dispoly_cfg_t *cfg);

#ifdef __cplusplus
};
#endif

#endif