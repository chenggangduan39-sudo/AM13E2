#include "qtk_durian_decoder_cfg.h"
#include "qtk_tts_comm.h"

int qtk_durian_decoder_cfg_init(qtk_durian_decoder_cfg_t *cfg)
{
    int ret = 0;
    memset(cfg, 0, sizeof(qtk_durian_decoder_cfg_t));
    return ret;
}

int qtk_durian_decoder_cfg_clean(qtk_durian_decoder_cfg_t *cfg)
{
    int ret = 0;
    int i = 0;

    wtk_free(cfg->prenet_layer_weight_fn[0]);
    wtk_free(cfg->prenet_layer_weight_fn[1]);
    wtk_free(cfg->prenet_layer_weight_fn);
    for(i = 0; i < 2; ++i){
        wtk_free(cfg->lstm_weight_fn[i]);
        wtk_free(cfg->lstm_bias_fn[i]);
    }
    wtk_free(cfg->lstm_weight_fn);
    wtk_free(cfg->lstm_bias_fn);
    wtk_free(cfg->conv1d_weight_fn);
    wtk_free(cfg->conv1d_bias_fn);
    return ret;
}

int qtk_durian_decoder_cfg_update_local(qtk_durian_decoder_cfg_t *cfg,wtk_local_cfg_t *main_lc)
{
    int ret = 0;
    wtk_string_t *v = NULL;
    
    wtk_local_cfg_update_cfg_i(main_lc,cfg,prenet_layers_dim,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,decoder_rnn_dim,v);
    wtk_local_cfg_update_cfg_str(main_lc,cfg,prenet_layer_fn_format,v);
    cfg->lstm_dim_num = wtk_local_cfg_find_int_array_s(main_lc,"lstm_dim_num");
    wtk_local_cfg_update_cfg_str(main_lc,cfg,lstm_fn_format,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,num_mels,v);
    wtk_local_cfg_update_cfg_str(main_lc,cfg,conv1d_fn_format,v);

    return ret;
}

int qtk_durian_decoder_cfg_update(qtk_durian_decoder_cfg_t *cfg)
{
    int ret = 0;
    cfg->decoder_output_size = cfg->decoder_rnn_dim;
    cfg->prenet_layer_weight_fn = wtk_malloc(sizeof(char*)*2);
    cfg->prenet_layer_weight_fn[0] = PATH_FORMAT_TO(cfg->prenet_layer_fn_format,"weight",0);
    cfg->prenet_layer_weight_fn[1] = PATH_FORMAT_TO(cfg->prenet_layer_fn_format,"weight",1);
    int i = 0; 
    cfg->lstm_weight_fn = wtk_malloc(sizeof(char*)*2);
    cfg->lstm_bias_fn = wtk_malloc(sizeof(char*)*2);
    for(i = 0; i < 2; ++i){
        cfg->lstm_weight_fn[i] = PATH_FORMAT_TO(cfg->lstm_fn_format,"weight",i);
        cfg->lstm_bias_fn[i] = PATH_FORMAT_TO(cfg->lstm_fn_format,"bias",i);
    }
    cfg->conv1d_weight_fn = PATH_FORMAT_TO(cfg->conv1d_fn_format,"weight");
    cfg->conv1d_bias_fn = PATH_FORMAT_TO(cfg->conv1d_fn_format,"bias");
    return ret;
}
