#ifndef __QTK_DURIAN_DURATION_PREDICTOR_CFG_H__
#define __QTK_DURIAN_DURATION_PREDICTOR_CFG_H__

#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/wtk_array.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct qtk_durian_duration_predictor_cfg{
    int conv1d_layer;
    float speed_factor;
    // int mix_resolution_factor;
    wtk_array_t *conv1d_dim_num;
    wtk_array_t *layernorm_dim_num;
    wtk_array_t *linear_dim_num;
    char *conv1d_fn_format;
    char **conv1d_kernel_fn;
    char **conv1d_bias_fn;
    char *layernorm_fn_format;
    char **layernorm_gamm_fn;
    char **layernorm_beta_fn;
    char *linear_weight_fn;
    char *linear_bias_fn;
}qtk_durian_duration_predictor_cfg_t;

int qtk_durian_duration_predictor_cfg_init(qtk_durian_duration_predictor_cfg_t *cfg);
int qtk_durian_duration_predictor_cfg_clean(qtk_durian_duration_predictor_cfg_t *cfg);
int qtk_durian_duration_predictor_cfg_update_local(qtk_durian_duration_predictor_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_durian_duration_predictor_cfg_update(qtk_durian_duration_predictor_cfg_t *cfg);


#ifdef __cplusplus
};
#endif

#endif