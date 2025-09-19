#include "qtk_tts_tac2_syn_dec_cfg.h"
#include "qtk_tts_comm.h"

int qtk_tts_tac2_syn_dec_cfg_init(qtk_tts_tac2_syn_dec_cfg_t *cfg)
{
    memset(cfg, 0, sizeof(qtk_tts_tac2_syn_dec_cfg_t));
    cfg->use_gru = 1;
    cfg->atten_per_step = 1;
    return 0;
}

int qtk_tts_tac2_syn_dec_cfg_clean(qtk_tts_tac2_syn_dec_cfg_t *cfg)
{
    int i = 0;
    for(i = 0; i < cfg->prenet_layer;++i){
        wtk_free(cfg->prenet_kernel_fn[i]);
        wtk_free(cfg->prenet_bias_fn[i]);
    }
    wtk_free(cfg->prenet_kernel_fn);
    wtk_free(cfg->prenet_bias_fn);
    if(cfg->use_gru){
        for(i = 0; i < cfg->rnn_layer;++i){
            wtk_free(cfg->gru_kernel_gate[i]);
            wtk_free(cfg->gru_kernel_cand[i]);
            wtk_free(cfg->gru_kernel_cand_hh[i]);
            wtk_free(cfg->gru_bias_gate[i]);
            wtk_free(cfg->gru_bias_cand[i]);
            wtk_free(cfg->gru_bias_cand_hh[i]);
        }
        wtk_free(cfg->gru_kernel_gate);
        wtk_free(cfg->gru_kernel_cand);
        wtk_free(cfg->gru_kernel_cand_hh);
        wtk_free(cfg->gru_bias_gate);
        wtk_free(cfg->gru_bias_cand);
        wtk_free(cfg->gru_bias_cand_hh);
    }
    wtk_free(cfg->atten_query_kernel_fn);
    wtk_free(cfg->atten_query_bias_fn);
    wtk_free(cfg->atten_conv_kernel_fn);
    wtk_free(cfg->atten_conv_bias_fn);
    return 0;
}

int qtk_tts_tac2_syn_dec_cfg_update_local(qtk_tts_tac2_syn_dec_cfg_t *cfg,wtk_local_cfg_t *lc)
{
    wtk_string_t *v = NULL;

    cfg->prenet_dim_num = wtk_local_cfg_find_int_array_s(lc,"prenet_dim_num");
    wtk_local_cfg_update_cfg_str(lc,cfg,prenet_fn_format,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,prenet_layer,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_gru,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,rnn_layer,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,num_mels,v);
    cfg->rnn_dim_num = wtk_local_cfg_find_int_array_s(lc,"rnn_dim_num");
    if(cfg->use_gru){
        wtk_local_cfg_update_cfg_str(lc,cfg,gru_fn_format,v);
    }
    wtk_local_cfg_update_cfg_str(lc,cfg,linear_kernel_fn,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,linear_bias_fn,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,stop_kernel_fn,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,stop_bias_fn,v);
    cfg->linear_dim_num = wtk_local_cfg_find_int_array_s(lc,"linear_dim_num");
    cfg->stop_dim_num = wtk_local_cfg_find_int_array_s(lc,"stop_dim_num");

    wtk_local_cfg_update_cfg_str(lc,cfg,atten_query_fn_format,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,atten_channel,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,atten_dim,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,atten_kernel,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,atten_filter,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,atten_query_nrow,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,atten_memory_kernel_fn,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,atten_memory_bias_fn,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,atten_conv_fn_format,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,atten_features_kernel_fn,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,atten_var_project_fn,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,atten_var_project_bias_fn,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,atten_per_step,v);
    return 0;
}

int qtk_tts_tac2_syn_dec_cfg_update(qtk_tts_tac2_syn_dec_cfg_t *cfg)
{
    int i = 0;
    cfg->prenet_kernel_fn = wtk_malloc(sizeof(char*)*cfg->prenet_layer);
    cfg->prenet_bias_fn = wtk_malloc(sizeof(char*)*cfg->prenet_layer);
    for(i  =0; i < cfg->prenet_layer;++i){
        cfg->prenet_kernel_fn[i] = PATH_FORMAT_TO(cfg->prenet_fn_format,"prenet_kernel",i);
        cfg->prenet_bias_fn[i] = PATH_FORMAT_TO(cfg->prenet_fn_format,"prenet_bias",i);
    }
    if(cfg->use_gru){
        cfg->gru_kernel_gate = wtk_malloc(sizeof(char *)*cfg->rnn_layer);
        cfg->gru_kernel_cand = wtk_malloc(sizeof(char*)*cfg->rnn_layer);
        cfg->gru_kernel_cand_hh = wtk_malloc(sizeof(char*)*cfg->rnn_layer);
        cfg->gru_bias_gate = wtk_malloc(sizeof(char*)*cfg->rnn_layer);
        cfg->gru_bias_cand = wtk_malloc(sizeof(char*)*cfg->rnn_layer);
        cfg->gru_bias_cand_hh = wtk_malloc(sizeof(char*)*cfg->rnn_layer);
        for(i = 0; i < cfg->rnn_layer;++i){
            cfg->gru_kernel_gate[i] = PATH_FORMAT_TO(cfg->gru_fn_format,"kernel_gate",i);
            cfg->gru_kernel_cand[i] = PATH_FORMAT_TO(cfg->gru_fn_format,"kernel_candidate",i);
            cfg->gru_kernel_cand_hh[i] = PATH_FORMAT_TO(cfg->gru_fn_format,"kernel_candidate_hh",i);
            cfg->gru_bias_gate[i] = PATH_FORMAT_TO(cfg->gru_fn_format,"bias_gate",i);
            cfg->gru_bias_cand[i] = PATH_FORMAT_TO(cfg->gru_fn_format,"bias_candidate",i);
            cfg->gru_bias_cand_hh[i] = PATH_FORMAT_TO(cfg->gru_fn_format,"bias_candidate_hh",i);
        }
    }
    cfg->atten_query_kernel_fn = PATH_FORMAT_TO(cfg->atten_query_fn_format,"kernel");
    cfg->atten_query_bias_fn = PATH_FORMAT_TO(cfg->atten_query_fn_format,"bias");
    cfg->atten_conv_kernel_fn = PATH_FORMAT_TO(cfg->atten_conv_fn_format,"kernel");
    cfg->atten_conv_bias_fn = PATH_FORMAT_TO(cfg->atten_conv_fn_format,"bias");
    return 0;
}
