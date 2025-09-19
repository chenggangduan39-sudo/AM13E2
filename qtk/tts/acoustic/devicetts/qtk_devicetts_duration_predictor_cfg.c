#include "qtk_devicetts_duration_predictor_cfg.h"
#include "qtk_tts_comm.h"

int qtk_devicetts_duration_predictor_cfg_init(qtk_devicetts_duration_predictor_cfg_t *cfg)
{
    memset(cfg, 0, sizeof(qtk_devicetts_duration_predictor_cfg_t));
    cfg->speed_factor = 1.0f;
    return 0;
}

int qtk_devicetts_duration_predictor_cfg_clean(qtk_devicetts_duration_predictor_cfg_t *cfg)
{
    int i = 0;
    for(i = 0; i < cfg->conv1d_layer; ++i){
        wtk_free(cfg->conv1d_kernel_fn[i]);
        wtk_free(cfg->conv1d_bias_fn[i]);
        wtk_free(cfg->layernorm_gamm_fn[i]);
        wtk_free(cfg->layernorm_beta_fn[i]);
    }
    wtk_free(cfg->layernorm_gamm_fn);
    wtk_free(cfg->layernorm_beta_fn);
    wtk_free(cfg->conv1d_bias_fn);
    wtk_free(cfg->conv1d_kernel_fn);
    return 0;
}

int qtk_devicetts_duration_predictor_cfg_update_local(qtk_devicetts_duration_predictor_cfg_t *cfg,wtk_local_cfg_t *main_lc)
{
    wtk_string_t *v = NULL;

    wtk_local_cfg_update_cfg_b(main_lc,cfg,use_blk,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,conv1d_layer,v);
    cfg->conv1d_dim_num = wtk_local_cfg_find_int_array_s(main_lc,"conv1d_dim_num");
    wtk_local_cfg_update_cfg_str(main_lc,cfg,conv1d_fn_format,v);
    cfg->layernorm_dim_num = wtk_local_cfg_find_int_array_s(main_lc,"layernorm_dim_num");
    wtk_local_cfg_update_cfg_str(main_lc,cfg,layernorm_fn_format,v);
    cfg->linear_dim_num = wtk_local_cfg_find_int_array_s(main_lc,"linear_dim_num");
    wtk_local_cfg_update_cfg_str(main_lc,cfg,linear_weight_fn,v);
    wtk_local_cfg_update_cfg_str(main_lc,cfg,linear_bias_fn,v);
    wtk_local_cfg_update_cfg_f(main_lc,cfg,speed_factor,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,mix_resolution_factor,v);

    return 0;
}

int qtk_devicetts_duration_predictor_cfg_update(qtk_devicetts_duration_predictor_cfg_t *cfg)
{
    int i = 0;
    cfg->conv1d_kernel_fn = wtk_malloc(sizeof(char*)*cfg->conv1d_layer);
    cfg->conv1d_bias_fn = wtk_malloc(sizeof(char*)*cfg->conv1d_layer);
    cfg->layernorm_gamm_fn = wtk_malloc(sizeof(char*)*cfg->conv1d_layer);
    cfg->layernorm_beta_fn = wtk_malloc(sizeof(char*)*cfg->conv1d_layer);

    for(i = 0; i < cfg->conv1d_layer; ++i){
        cfg->conv1d_kernel_fn[i] = PATH_FORMAT_TO(cfg->conv1d_fn_format,"kernel",i);
        cfg->conv1d_bias_fn[i] = PATH_FORMAT_TO(cfg->conv1d_fn_format,"bias",i);
        cfg->layernorm_gamm_fn[i] = PATH_FORMAT_TO(cfg->layernorm_fn_format,"gamm",i);
        cfg->layernorm_beta_fn[i] = PATH_FORMAT_TO(cfg->layernorm_fn_format,"beta",i);
    }
    
    return 0;
}
