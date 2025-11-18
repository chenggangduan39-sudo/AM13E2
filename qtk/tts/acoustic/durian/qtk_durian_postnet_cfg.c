#include "qtk_durian_postnet_cfg.h"
#include "qtk_tts_comm.h"

int qtk_durian_postnet_cfg_init(qtk_durian_postnet_cfg_t *cfg)
{
    memset(cfg, 0, sizeof(qtk_durian_postnet_cfg_t));
    return 0;
}

int qtk_durian_postnet_cfg_clean(qtk_durian_postnet_cfg_t *cfg)
{
    int n = cfg->convrbn;
    int i = 0;
    for(i = 0; i < n;++i){
        wtk_free(cfg->convrb_conv_weight_fn[i]);
        wtk_free(cfg->convrb_conv_bias_fn[i]);
        wtk_free(cfg->convrb_batchnorm_gamma_fn[i]);
        wtk_free(cfg->convrb_batchnorm_beta_fn[i]);
        wtk_free(cfg->convrb_batchnorm_mean_fn[i]);
        wtk_free(cfg->convrb_batchnorm_var_fn[i]);
        wtk_free(cfg->convrb_conv1d_residual_weight_fn[i]);
        wtk_free(cfg->convrb_conv1d_residual_bias_fn[i]);
    }
    wtk_free(cfg->convrb_conv_weight_fn);
    wtk_free(cfg->convrb_conv_bias_fn);
    wtk_free(cfg->convrb_batchnorm_gamma_fn);
    wtk_free(cfg->convrb_batchnorm_beta_fn);
    wtk_free(cfg->convrb_batchnorm_mean_fn);
    wtk_free(cfg->convrb_batchnorm_var_fn);
    wtk_free(cfg->convrb_conv1d_residual_weight_fn);
    wtk_free(cfg->convrb_conv1d_residual_bias_fn);
    wtk_free(cfg->conv1d_weight_fn);
    wtk_free(cfg->conv1d_bias_fn);
    return 0;
}

int qtk_durian_postnet_cfg_update_local(qtk_durian_postnet_cfg_t *cfg,wtk_local_cfg_t *main_lc)
{
    int ret = 0;
    wtk_string_t *v = NULL;
    
    wtk_local_cfg_update_cfg_i(main_lc,cfg,convrbn,v);   
    cfg->convrb_dim_num = wtk_local_cfg_find_int_array_s(main_lc,"convrb_dim_num");
    cfg->conv1d_dim_num = wtk_local_cfg_find_int_array_s(main_lc,"conv1d_dim_num");
    wtk_local_cfg_update_cfg_str(main_lc,cfg,convrb_fn_format,v);
    wtk_local_cfg_update_cfg_str(main_lc,cfg,conv1d_fn_format,v);
    
    return ret;
}

int qtk_durian_postnet_cfg_update(qtk_durian_postnet_cfg_t *cfg)
{
    int n = cfg->convrbn;
    int i = 0;
    
    cfg->convrb_batchnorm_gamma_fn = wtk_malloc(sizeof(char*)*n);
    cfg->convrb_batchnorm_beta_fn = wtk_malloc(sizeof(char*)*n);
    cfg->convrb_batchnorm_mean_fn = wtk_malloc(sizeof(char*)*n);
    cfg->convrb_batchnorm_var_fn = wtk_malloc(sizeof(char*)*n);
    cfg->convrb_conv_weight_fn = wtk_malloc(sizeof(char*)*n);
    cfg->convrb_conv_bias_fn = wtk_malloc(sizeof(char*)*n);
    cfg->convrb_conv1d_residual_weight_fn = wtk_malloc(sizeof(char*)*n);
    cfg->convrb_conv1d_residual_bias_fn = wtk_malloc(sizeof(char*)*n);

    for(i = 0; i < n; ++i){
        cfg->convrb_conv_weight_fn[i] = PATH_FORMAT_TO(cfg->convrb_fn_format,"conv1d","weight",i);
        cfg->convrb_conv_bias_fn[i] = PATH_FORMAT_TO(cfg->convrb_fn_format,"conv1d","bias",i);
        cfg->convrb_batchnorm_gamma_fn[i] = PATH_FORMAT_TO(cfg->convrb_fn_format,"batchnorm","gamma",i);
        cfg->convrb_batchnorm_beta_fn[i] = PATH_FORMAT_TO(cfg->convrb_fn_format,"batchnorm","beta",i);
        cfg->convrb_batchnorm_mean_fn[i] = PATH_FORMAT_TO(cfg->convrb_fn_format,"batchnorm","mean",i);
        cfg->convrb_batchnorm_var_fn[i] = PATH_FORMAT_TO(cfg->convrb_fn_format,"batchnorm","var",i);
        cfg->convrb_conv1d_residual_weight_fn[i] = PATH_FORMAT_TO(cfg->convrb_fn_format,"residual","weight",i);
        cfg->convrb_conv1d_residual_bias_fn[i] = PATH_FORMAT_TO(cfg->convrb_fn_format,"residual","bias",i);
    }
    cfg->conv1d_weight_fn = PATH_FORMAT_TO(cfg->conv1d_fn_format,"weight");
    cfg->conv1d_bias_fn = PATH_FORMAT_TO(cfg->conv1d_fn_format,"bias");
    return 0;
}
