#ifndef __QTK_DEVICETTS_POSTNET_CFG_H__
#define __QTK_DEVICETTS_POSTNET_CFG_H__

#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/wtk_array.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct qtk_devicetts_postnet_cfg{
    int convrbn;
    wtk_array_t *convrb_dim_num;
    wtk_array_t *conv1d_dim_num;
    char *convrb_fn_format;
    char *conv1d_fn_format;
    char **convrb_conv_weight_fn;
    char **convrb_conv_bias_fn;
    char **convrb_batchnorm_gamma_fn;
    char **convrb_batchnorm_beta_fn;
    char **convrb_batchnorm_mean_fn;
    char **convrb_batchnorm_var_fn;
    char **convrb_conv1d_residual_weight_fn;
    char **convrb_conv1d_residual_bias_fn;
    char *conv1d_weight_fn;
    char *conv1d_bias_fn;
    unsigned int useblk:1;
}qtk_devicetts_postnet_cfg_t;

int qtk_devicetts_postnet_cfg_init(qtk_devicetts_postnet_cfg_t *cfg);
int qtk_devicetts_postnet_cfg_clean(qtk_devicetts_postnet_cfg_t *cfg);
int qtk_devicetts_postnet_cfg_update_local(qtk_devicetts_postnet_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_devicetts_postnet_cfg_update(qtk_devicetts_postnet_cfg_t *cfg);


#ifdef __cplusplus
};
#endif

#endif
