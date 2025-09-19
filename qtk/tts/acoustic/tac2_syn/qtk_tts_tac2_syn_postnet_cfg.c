#include "qtk_tts_tac2_syn_postnet_cfg.h"
#include "qtk_tts_comm.h"

int qtk_tts_tac2_syn_postnet_cfg_init(qtk_tts_tac2_syn_postnet_cfg_t *cfg)
{
    int ret = 0;
    memset(cfg, 0, sizeof(*cfg));
    return ret;
}

int qtk_tts_tac2_syn_postnet_cfg_clean(qtk_tts_tac2_syn_postnet_cfg_t *cfg)
{
    int ret = 0;
    int layer = cfg->num_layer,i = 0;
    for(i = 0; i < layer; ++i){
        wtk_free(cfg->conv_kernel_fn[i]);
        wtk_free(cfg->conv_bias_fn[i]);
        wtk_free(cfg->batch_norm_gamma_fn[i]);
        wtk_free(cfg->batch_norm_beta_fn[i]);
        wtk_free(cfg->batch_norm_mean_fn[i]);
        wtk_free(cfg->batch_norm_var_fn[i]);
    }
    wtk_free(cfg->conv_kernel_fn);
    wtk_free(cfg->conv_bias_fn);
    wtk_free(cfg->batch_norm_gamma_fn);
    wtk_free(cfg->batch_norm_beta_fn);
    wtk_free(cfg->batch_norm_mean_fn);
    wtk_free(cfg->batch_norm_var_fn);
    return ret;
}

int qtk_tts_tac2_syn_postnet_cfg_update_local(qtk_tts_tac2_syn_postnet_cfg_t *cfg,wtk_local_cfg_t *lc)
{
    int ret = 0;
    wtk_string_t *v = NULL;
    wtk_local_cfg_update_cfg_i(lc,cfg,num_layer,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,num_mels,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,conv_fn_format,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,batch_norm_fn_format,v);
    cfg->conv_dim_num = wtk_local_cfg_find_int_array_s(lc,"conv_dim_num");
    cfg->conv_kernel_size = wtk_local_cfg_find_int_array_s(lc,"conv_kernel_size");
    return ret;
}

int qtk_tts_tac2_syn_postnet_cfg_update(qtk_tts_tac2_syn_postnet_cfg_t *cfg)
{
    int ret = 0;
    int layer = cfg->num_layer,i = 0;

    cfg->conv_kernel_fn = wtk_malloc(sizeof(char*)*layer);
    cfg->conv_bias_fn = wtk_malloc(sizeof(char*)*layer);
    cfg->batch_norm_gamma_fn = wtk_malloc(sizeof(char*)*layer);
    cfg->batch_norm_beta_fn = wtk_malloc(sizeof(char*)*layer);
    cfg->batch_norm_mean_fn = wtk_malloc(sizeof(char*)*layer);
    cfg->batch_norm_var_fn = wtk_malloc(sizeof(char*)*layer);
    for(i = 0; i < layer; ++i){
        cfg->conv_kernel_fn[i] = PATH_FORMAT_TO(cfg->conv_fn_format,"kernel",i);
        cfg->conv_bias_fn[i] = PATH_FORMAT_TO(cfg->conv_fn_format,"bias",i);
        cfg->batch_norm_gamma_fn[i] = PATH_FORMAT_TO(cfg->batch_norm_fn_format,"gamma",i);
        cfg->batch_norm_beta_fn[i] = PATH_FORMAT_TO(cfg->batch_norm_fn_format,"beta",i);
        cfg->batch_norm_mean_fn[i] = PATH_FORMAT_TO(cfg->batch_norm_fn_format,"moving_mean",i);
        cfg->batch_norm_var_fn[i] = PATH_FORMAT_TO(cfg->batch_norm_fn_format,"moving_variance",i);
    }

    return ret;
}
