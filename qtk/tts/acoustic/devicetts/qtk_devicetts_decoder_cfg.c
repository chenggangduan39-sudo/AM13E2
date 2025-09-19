#include "qtk_devicetts_decoder_cfg.h"
#include "qtk_tts_comm.h"

int qtk_devicetts_decoder_cfg_init(qtk_devicetts_decoder_cfg_t *cfg)
{
    int ret = 0;
    memset(cfg, 0, sizeof(qtk_devicetts_decoder_cfg_t));
    qtk_devicetts_postnet_cfg_init(&cfg->postnet);
    cfg->mgc_dim = 60;
    cfg->bap_dim = 1;
    cfg->lf0_dim = 1;
    cfg->vuv_dim = 2;

    return ret;
}

int qtk_devicetts_decoder_cfg_clean(qtk_devicetts_decoder_cfg_t *cfg)
{
    int ret = 0;
    int i = 0;

    qtk_devicetts_postnet_cfg_clean(&cfg->postnet);
    wtk_free(cfg->prenet_layer_weight_fn[0]);
    wtk_free(cfg->prenet_layer_weight_fn[1]);
    wtk_free(cfg->prenet_layer_weight_fn);
    for(i = 0; i < 2; ++i){
        wtk_free(cfg->gru_gate_weight_fn[i]);
        wtk_free(cfg->gru_candidate_weight_fn[i]);
        wtk_free(cfg->gru_candidate_hh_weight_fn[i]);
        wtk_free(cfg->gru_gate_bias_fn[i]);
        wtk_free(cfg->gru_candidate_bias_fn[i]);
        wtk_free(cfg->gru_candidate_hh_bias_fn[i]);
    }
    wtk_free(cfg->gru_gate_weight_fn);
    wtk_free(cfg->gru_candidate_weight_fn);
    wtk_free(cfg->gru_candidate_hh_weight_fn);
    wtk_free(cfg->gru_gate_bias_fn);
    wtk_free(cfg->gru_candidate_bias_fn);
    wtk_free(cfg->gru_candidate_hh_bias_fn);
    wtk_free(cfg->projection_weight_fn);
    wtk_free(cfg->projection_bias_fn);
    wtk_free(cfg->conv1d_weight_fn);
    wtk_free(cfg->conv1d_bias_fn);
    wtk_free(cfg->linear_bap_weight_fn);
    wtk_free(cfg->linear_bap_bias_fn);
    wtk_free(cfg->linear_lf0_weight_fn);
    wtk_free(cfg->linear_lf0_bias_fn);
    wtk_free(cfg->linear_vuv_weight_fn);
    wtk_free(cfg->linear_vuv_bias_fn);
    return ret;
}

int qtk_devicetts_decoder_cfg_update_local(qtk_devicetts_decoder_cfg_t *cfg,wtk_local_cfg_t *main_lc)
{
    int ret = 0;
    wtk_string_t *v = NULL;
    wtk_local_cfg_t *lc = NULL;
    
    wtk_local_cfg_update_cfg_i(main_lc,cfg,mix_resolution_factor,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,prenet_layers_dim,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,decoder_rnn_dim,v);
    wtk_local_cfg_update_cfg_str(main_lc,cfg,prenet_layer_fn_format,v);
    cfg->gru_dim_num = wtk_local_cfg_find_int_array_s(main_lc,"gru_dim_num");
    wtk_local_cfg_update_cfg_str(main_lc,cfg,gru_fn_format,v);
    cfg->projection_dim_num = wtk_local_cfg_find_int_array_s(main_lc,"projection_dim_num");
    wtk_local_cfg_update_cfg_str(main_lc,cfg,projection_fn_format,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,num_mels,v);
    wtk_local_cfg_update_cfg_str(main_lc,cfg,conv1d_fn_format,v);
    wtk_local_cfg_update_cfg_str(main_lc,cfg,linear_fn_format,v);

    wtk_local_cfg_update_cfg_i(main_lc,cfg,mgc_dim,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,bap_dim,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,lf0_dim,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,vuv_dim,v);

    lc = wtk_local_cfg_find_lc_s(main_lc,"postnet");
    if(lc){
        qtk_devicetts_postnet_cfg_update_local(&cfg->postnet,lc);
    }

    return ret;
}

int qtk_devicetts_decoder_cfg_update(qtk_devicetts_decoder_cfg_t *cfg)
{
    int ret = 0;
    cfg->decoder_output_size = cfg->decoder_rnn_dim * cfg->mix_resolution_factor;
    cfg->prenet_layer_weight_fn = wtk_malloc(sizeof(char*)*2);
    cfg->prenet_layer_weight_fn[0] = PATH_FORMAT_TO(cfg->prenet_layer_fn_format,"weight",0);
    cfg->prenet_layer_weight_fn[1] = PATH_FORMAT_TO(cfg->prenet_layer_fn_format,"weight",1);
    int i = 0; 
    cfg->gru_gate_weight_fn = wtk_malloc(sizeof(char*)*2);
    cfg->gru_candidate_weight_fn = wtk_malloc(sizeof(char*)*2);
    cfg->gru_candidate_hh_weight_fn = wtk_malloc(sizeof(char*)*2);
    cfg->gru_gate_bias_fn = wtk_malloc(sizeof(char*)*2);
    cfg->gru_candidate_bias_fn = wtk_malloc(sizeof(char*)*2);
    cfg->gru_candidate_hh_bias_fn = wtk_malloc(sizeof(char*)*2);
    for(i = 0; i < 2; ++i){
        cfg->gru_gate_weight_fn[i] = PATH_FORMAT_TO(cfg->gru_fn_format,"gate_weight",i);
        cfg->gru_candidate_weight_fn[i] = PATH_FORMAT_TO(cfg->gru_fn_format,"candidate_weight",i);
        cfg->gru_candidate_hh_weight_fn[i] = PATH_FORMAT_TO(cfg->gru_fn_format,"candidate_hh_weight",i);
        cfg->gru_gate_bias_fn[i] = PATH_FORMAT_TO(cfg->gru_fn_format,"gate_bias",i);
        cfg->gru_candidate_bias_fn[i] = PATH_FORMAT_TO(cfg->gru_fn_format,"candidate_bias",i);
        cfg->gru_candidate_hh_bias_fn[i] = PATH_FORMAT_TO(cfg->gru_fn_format,"candidate_hh_bias",i);
    }
    cfg->projection_weight_fn = PATH_FORMAT_TO(cfg->projection_fn_format,"weight");
    cfg->projection_bias_fn = PATH_FORMAT_TO(cfg->projection_fn_format,"bias");
    cfg->conv1d_weight_fn = PATH_FORMAT_TO(cfg->conv1d_fn_format,"weight");
    cfg->conv1d_bias_fn = PATH_FORMAT_TO(cfg->conv1d_fn_format,"bias");

    cfg->linear_bap_weight_fn = PATH_FORMAT_TO(cfg->linear_fn_format,"bap", "weight");
    cfg->linear_bap_bias_fn = PATH_FORMAT_TO(cfg->linear_fn_format,"bap", "bias");

    cfg->linear_lf0_weight_fn = PATH_FORMAT_TO(cfg->linear_fn_format,"lf0", "weight");
    cfg->linear_lf0_bias_fn = PATH_FORMAT_TO(cfg->linear_fn_format,"lf0", "bias");

    cfg->linear_vuv_weight_fn = PATH_FORMAT_TO(cfg->linear_fn_format,"vuv", "weight");
    cfg->linear_vuv_bias_fn = PATH_FORMAT_TO(cfg->linear_fn_format,"vuv", "bias");

    qtk_devicetts_postnet_cfg_update(&cfg->postnet);

    return ret;
}
