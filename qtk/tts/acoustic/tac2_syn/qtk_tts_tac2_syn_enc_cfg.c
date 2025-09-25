#include "qtk_tts_tac2_syn_enc_cfg.h"
#include "wtk/core/wtk_alloc.h"
#include "qtk_tts_comm.h"

int qtk_tts_tac2_syn_enc_cfg_init(qtk_tts_tac2_syn_enc_cfg_t *cfg)
{
    memset(cfg,0, sizeof(qtk_tts_tac2_syn_enc_cfg_t));
    return 0;
}

int qtk_tts_tac2_syn_enc_cfg_clean(qtk_tts_tac2_syn_enc_cfg_t *cfg)
{
    int conv_layer_num = cfg->conv_layer;
    int i = 0;
    for(i = 0; i < conv_layer_num; ++i){
        wtk_free(cfg->conv_kernel_fn[i]);
        wtk_free(cfg->conv_bias_fn[i]);
        wtk_free(cfg->gamma_fn[i]);
        wtk_free(cfg->beta_fn[i]);
        wtk_free(cfg->moving_mean_fn[i]);
        wtk_free(cfg->moving_variance_fn[i]);
    }
    wtk_free(cfg->conv_kernel_fn);
    wtk_free(cfg->conv_bias_fn);
    wtk_free(cfg->gamma_fn);
    wtk_free(cfg->beta_fn);
    wtk_free(cfg->moving_mean_fn);
    wtk_free(cfg->moving_variance_fn);
    wtk_free(cfg->lstm_fw_kernel_fn);
    wtk_free(cfg->lstm_fw_bias_fn);
    wtk_free(cfg->lstm_bw_kernel_fn);
    wtk_free(cfg->lstm_bw_bias_fn);
    return 0;
}

int qtk_tts_tac2_syn_enc_cfg_update_local(qtk_tts_tac2_syn_enc_cfg_t *cfg,wtk_local_cfg_t *lc)
{
    wtk_string_t *v = NULL;

    wtk_local_cfg_update_cfg_i(lc,cfg,conv_layer,v);
    cfg->conv_dim_num = wtk_local_cfg_find_int_array_s(lc,"conv_dim_num");
    cfg->conv_kernel_sizes = wtk_local_cfg_find_int_array_s(lc,"conv_kernel_sizes");
    cfg->lstm_dim_num = wtk_local_cfg_find_int_array_s(lc,"lstm_dim_num");
    wtk_local_cfg_update_cfg_str(lc,cfg,conv_fn_format,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,lstm_fn_format,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,batch_norm_fn_format,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,lstm_fn_format,v);
    return 0;
}

int qtk_tts_tac2_syn_enc_cfg_update(qtk_tts_tac2_syn_enc_cfg_t *cfg)
{
    int i = 0;
    int conv_layer_num = cfg->conv_layer;
    
    cfg->conv_kernel_fn = wtk_malloc(sizeof(char*)*conv_layer_num);
    cfg->conv_bias_fn = wtk_malloc(sizeof(char*)*conv_layer_num);
    cfg->gamma_fn = wtk_malloc(sizeof(char*)*conv_layer_num);
    cfg->beta_fn = wtk_malloc(sizeof(char*)*conv_layer_num);
    cfg->moving_mean_fn = wtk_malloc(sizeof(char*)*conv_layer_num);
    cfg->moving_variance_fn = wtk_malloc(sizeof(char*)*conv_layer_num);
    for(i = 0; i < conv_layer_num; ++i){
        cfg->conv_kernel_fn[i] = PATH_FORMAT_TO(cfg->conv_fn_format,"kernel",i);
        cfg->conv_bias_fn[i] = PATH_FORMAT_TO(cfg->conv_fn_format,"bias",i);
        cfg->gamma_fn[i] = PATH_FORMAT_TO(cfg->batch_norm_fn_format,"gamma",i);
        cfg->beta_fn[i] = PATH_FORMAT_TO(cfg->batch_norm_fn_format,"beta",i);
        cfg->moving_mean_fn[i] = PATH_FORMAT_TO(cfg->batch_norm_fn_format,"moving_mean",i);
        cfg->moving_variance_fn[i] = PATH_FORMAT_TO(cfg->batch_norm_fn_format,"moving_variance",i);
    }
    cfg->lstm_fw_kernel_fn = PATH_FORMAT_TO(cfg->lstm_fn_format,"fw_kernel");
    cfg->lstm_fw_bias_fn = PATH_FORMAT_TO(cfg->lstm_fn_format,"fw_bias");
    cfg->lstm_bw_kernel_fn = PATH_FORMAT_TO(cfg->lstm_fn_format,"bw_kernel");;
    cfg->lstm_bw_bias_fn = PATH_FORMAT_TO(cfg->lstm_fn_format,"bw_bias");;
    return 0;
}
