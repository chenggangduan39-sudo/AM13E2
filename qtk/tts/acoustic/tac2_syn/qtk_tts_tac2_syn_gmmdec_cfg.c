#include "qtk_tts_tac2_syn_gmmdec_cfg.h"
#include "qtk_tts_comm.h"

int qtk_tts_tac2_syn_gmmdec_cfg_init(qtk_tts_tac2_syn_gmmdec_cfg_t *cfg)
{
    memset(cfg,0,sizeof(qtk_tts_tac2_syn_gmmdec_cfg_t));
    return 0;
}

int qtk_tts_tac2_syn_gmmdec_cfg_clean(qtk_tts_tac2_syn_gmmdec_cfg_t *cfg)
{
    int i = 0, n = cfg->prenet_layer;
    for(i = 0; i < n; ++i){
        wtk_free(cfg->prenet_kernel_fn[i]);
        wtk_free(cfg->prenet_bias_fn[i]);
    }
    wtk_free(cfg->prenet_kernel_fn);
    wtk_free(cfg->prenet_bias_fn);
    wtk_free(cfg->atten_rnn_weight_fn);
    wtk_free(cfg->atten_rnn_bias_fn);
    wtk_free(cfg->atten_query_weight_fn);
    wtk_free(cfg->atten_query_bias_fn);
    wtk_free(cfg->atten_v_weight_fn);
    wtk_free(cfg->linear_weight_fn);
    wtk_free(cfg->linear_bias_fn);
    for(i = 0; i < cfg->rnn_n; ++i){
        wtk_free(cfg->rnn_weight_fn[i]);
        wtk_free(cfg->rnn_bias_fn[i]);
    }
    wtk_free(cfg->rnn_weight_fn);
    wtk_free(cfg->rnn_bias_fn);
    wtk_free(cfg->proj_weight_fn);
    return 0;
}

int qtk_tts_tac2_syn_gmmdec_cfg_update_local(qtk_tts_tac2_syn_gmmdec_cfg_t *cfg,wtk_local_cfg_t *lc)
{
    wtk_string_t *v = NULL;
    
    wtk_local_cfg_update_cfg_i(lc,cfg,prenet_layer,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,prenet_fn_format,v);
    cfg->prenet_dim_num = wtk_local_cfg_find_int_array_s(lc,"prenet_dim_num");
    wtk_local_cfg_update_cfg_i(lc,cfg,num_mels,v);
    cfg->atten_rnn_dim_num = wtk_local_cfg_find_int_array_s(lc,"atten_rnn_dim_num");
    wtk_local_cfg_update_cfg_str(lc,cfg,atten_fn_format,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,rnn_size,v);
    cfg->atten_layer_dim_num = wtk_local_cfg_find_int_array_s(lc,"atten_layer_dim_num");
    cfg->linear_dim_num = wtk_local_cfg_find_int_array_s(lc,"linear_dim_num");
    wtk_local_cfg_update_cfg_str(lc,cfg,linear_fn_format,v);
    cfg->rnn_dim_num = wtk_local_cfg_find_int_array_s(lc,"rnn_dim_num");
    wtk_local_cfg_update_cfg_str(lc,cfg,rnn_fn_format,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,rnn_n,v);
    cfg->proj_dim_num = wtk_local_cfg_find_int_array_s(lc,"proj_dim_num");
    wtk_local_cfg_update_cfg_str(lc,cfg,proj_fn_format,v);
    return 0;
}

int qtk_tts_tac2_syn_gmmdec_cfg_update(qtk_tts_tac2_syn_gmmdec_cfg_t *cfg)
{
    int i = 0,n = cfg->prenet_layer;
    cfg->prenet_kernel_fn = wtk_malloc(sizeof(char*)*n);
    cfg->prenet_bias_fn = wtk_malloc(sizeof(char*)*n);
    for(i = 0; i < n; ++i){
        cfg->prenet_kernel_fn[i] = PATH_FORMAT_TO(cfg->prenet_fn_format,"weight",i);
        cfg->prenet_bias_fn[i] = PATH_FORMAT_TO(cfg->prenet_fn_format,"bias",i);
    }
    cfg->atten_rnn_weight_fn = PATH_FORMAT_TO(cfg->atten_fn_format,"atten_rnn_weight");
    cfg->atten_rnn_bias_fn = PATH_FORMAT_TO(cfg->atten_fn_format,"atten_rnn_bias");
    cfg->atten_query_weight_fn = PATH_FORMAT_TO(cfg->atten_fn_format,"atten_query_weight");
    cfg->atten_query_bias_fn = PATH_FORMAT_TO(cfg->atten_fn_format,"atten_query_bias");
    cfg->atten_v_weight_fn = PATH_FORMAT_TO(cfg->atten_fn_format,"atten_v_weight");
    cfg->linear_weight_fn = PATH_FORMAT_TO(cfg->linear_fn_format,"weight");
    cfg->linear_bias_fn = PATH_FORMAT_TO(cfg->linear_fn_format,"bias");
    cfg->rnn_weight_fn = wtk_malloc(sizeof(char*)*cfg->rnn_n);
    cfg->rnn_bias_fn = wtk_malloc(sizeof(char*)*cfg->rnn_n);
    for(i = 0; i < cfg->rnn_n; ++i){
        cfg->rnn_weight_fn[i] = PATH_FORMAT_TO(cfg->rnn_fn_format,"weight",i);
        cfg->rnn_bias_fn[i] = PATH_FORMAT_TO(cfg->rnn_fn_format,"bias",i);
    }
    cfg->proj_weight_fn = PATH_FORMAT_TO(cfg->proj_fn_format,"weight");
    return 0;
}
