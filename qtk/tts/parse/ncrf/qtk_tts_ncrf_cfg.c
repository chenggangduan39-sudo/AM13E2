#include "qtk_tts_ncrf_cfg.h"
#include <limits.h>
#ifdef WIN32
#define PATH_MAX 4096
#endif


int qtk_tts_ncrf_cfg_init(qtk_tts_ncrf_cfg_t *cfg)
{
    int ret = 0;
    memset(cfg,0,sizeof(*cfg));
    cfg->feature_num = 5;
    cfg->linear_num = 2;
    cfg->lstm_pw_num = 2;
    cfg->lstm_pp_num = 2;

    return ret;
}

int qtk_tts_ncrf_cfg_clean(qtk_tts_ncrf_cfg_t *cfg)
{
    int ret = 0,i = 0;
    for(i = 0; i < cfg->feature_num; ++i){
        wtk_free(cfg->feature_embedding_fn[i]);
    }
    wtk_free(cfg->feature_embedding_fn);
    for(i = 0; i < cfg->linear_num;++i){
        wtk_free(cfg->linear_weight_fn[i]);
        wtk_free(cfg->linear_bias_fn[i]);
    }
    wtk_free(cfg->linear_weight_fn);
    wtk_free(cfg->linear_bias_fn);
    for(i = 0; i < cfg->lstm_pw_num*2;++i){
        wtk_free(cfg->lstm_pw_bias_fn[i]);
        wtk_free(cfg->lstm_pw_weight_fn[i]);
    }
    wtk_free(cfg->lstm_pw_weight_fn);
    wtk_free(cfg->lstm_pw_bias_fn);
    for(i = 0; i < cfg->lstm_pp_num*2;++i){
        wtk_free(cfg->lstm_pp_bias_fn[i]);
        wtk_free(cfg->lstm_pp_weight_fn[i]);
    }
    wtk_free(cfg->lstm_pp_weight_fn);
    wtk_free(cfg->lstm_pp_bias_fn);
    for(i = 0; i < cfg->lstm_ip_num*2;++i){
        wtk_free(cfg->lstm_ip_bias_fn[i]);
        wtk_free(cfg->lstm_ip_weight_fn[i]);
    }
    wtk_free(cfg->lstm_ip_weight_fn);
    wtk_free(cfg->lstm_ip_bias_fn);
    return ret;
}

int qtk_tts_ncrf_cfg_update_local(qtk_tts_ncrf_cfg_t *cfg,wtk_local_cfg_t *lc)
{
    int ret = 0;
    wtk_string_t *v = NULL;

    wtk_local_cfg_update_cfg_i(lc,cfg,feature_num,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,linear_num,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,lstm_pw_num,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,lstm_pp_num,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,lstm_ip_num,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,char_id_,v);
    cfg->feature_embedding_dim = wtk_local_cfg_find_int_array_s(lc,"feature_embedding_dim");
    cfg->char_embedding_dim = wtk_local_cfg_find_int_array_s(lc,"char_embedding_dim");
    cfg->linear_dim = wtk_local_cfg_find_int_array_s(lc,"linear_dim");
    cfg->lstm_pw_dim = wtk_local_cfg_find_int_array_s(lc,"lstm_pw_dim");
    cfg->linear_pw_dim = wtk_local_cfg_find_int_array_s(lc,"linear_pw_dim");
    cfg->pw_pred_embedding_dim = wtk_local_cfg_find_int_array_s(lc,"pw_pred_embedding_dim");
    cfg->pp_fnn_dim = wtk_local_cfg_find_int_array_s(lc,"pp_fnn_dim");
    cfg->lstm_pp_dim = wtk_local_cfg_find_int_array_s(lc,"lstm_pp_dim");
    cfg->linear_pp_dim = wtk_local_cfg_find_int_array_s(lc,"linear_pp_dim");
    cfg->pp_pred_embedding_dim = wtk_local_cfg_find_int_array_s(lc,"pp_pred_embedding_dim");
    cfg->ip_fnn_dim = wtk_local_cfg_find_int_array_s(lc,"ip_fnn_dim");
    cfg->lstm_ip_dim = wtk_local_cfg_find_int_array_s(lc,"lstm_ip_dim");
    cfg->linear_ip_dim = wtk_local_cfg_find_int_array_s(lc,"linear_ip_dim");
    cfg->id = wtk_local_cfg_find_int_array_s(lc,"id");
    cfg->id_ = wtk_local_cfg_find_int_array_s(lc,"id_");
    wtk_local_cfg_update_cfg_str(lc,cfg,feature_fn_format,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,char_embedding_fn,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,linear_weight_format,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,linear_bias_format,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,lstm_pw_weight_format,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,lstm_pw_bias_format,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,linear_pw_weight_fn,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,linear_pw_bias_fn,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,crf_pw_transitions_fn,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,pw_pred_embedding_fn,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,pp_fnn_weight_fn,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,pp_fnn_bias_fn,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,lstm_pp_weight_format,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,lstm_pp_bias_format,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,linear_pp_weight_fn,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,linear_pp_bias_fn,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,crf_pp_transitions_fn,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,pp_pred_embedding_fn,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,ip_fnn_weight_fn,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,ip_fnn_bias_fn,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,lstm_ip_weight_format,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,lstm_ip_bias_format,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,linear_ip_weight_fn,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,linear_ip_bias_fn,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,crf_ip_transitions_fn,v);

    return ret;
}

int qtk_tts_ncrf_cfg_update(qtk_tts_ncrf_cfg_t *cfg)
{
    int ret = 0, i = 0,n = 0;

    cfg->feature_embedding_fn = wtk_malloc(sizeof(char*)*cfg->feature_num);
    for(i = 0; i < cfg->feature_num; ++i){
        cfg->feature_embedding_fn[i] = wtk_calloc(1,PATH_MAX);
        sprintf(cfg->feature_embedding_fn[i],cfg->feature_fn_format,i);
    }
    cfg->linear_weight_fn = wtk_malloc(sizeof(char*)*cfg->linear_num);
    cfg->linear_bias_fn = wtk_malloc(sizeof(char*)*cfg->linear_num);
    for(i =0; i < cfg->linear_num; ++i){
        cfg->linear_weight_fn[i] = wtk_calloc(1,PATH_MAX);
        sprintf(cfg->linear_weight_fn[i],cfg->linear_weight_format,i);
        cfg->linear_bias_fn[i] = wtk_calloc(1,PATH_MAX);
        sprintf(cfg->linear_bias_fn[i],cfg->linear_bias_format,i);
    }
    //pw bilstm
    cfg->lstm_pw_weight_fn = wtk_malloc(sizeof(char*)*cfg->lstm_pw_num*2);
    cfg->lstm_pw_bias_fn = wtk_malloc(sizeof(char*)*cfg->lstm_pw_num*2);
    for(i = 0; i < cfg->lstm_pw_num;++i){
        n = i*2;
        cfg->lstm_pw_weight_fn[n] = wtk_calloc(1,PATH_MAX);
        sprintf(cfg->lstm_pw_weight_fn[n],cfg->lstm_pw_weight_format,"",i);
        cfg->lstm_pw_bias_fn[n] = wtk_calloc(1,PATH_MAX);
        sprintf(cfg->lstm_pw_bias_fn[n],cfg->lstm_pw_bias_format,"",i);
        //reverse
        n = i*2+1;
        cfg->lstm_pw_weight_fn[n] = wtk_calloc(1,PATH_MAX);
        sprintf(cfg->lstm_pw_weight_fn[n],cfg->lstm_pw_weight_format,"_reverse",i);
        cfg->lstm_pw_bias_fn[n] = wtk_calloc(1,PATH_MAX);
        sprintf(cfg->lstm_pw_bias_fn[n],cfg->lstm_pw_bias_format,"_reverse",i);
    }
    //pp
    cfg->lstm_pp_weight_fn = wtk_malloc(sizeof(char*)*cfg->lstm_pp_num*2);
    cfg->lstm_pp_bias_fn = wtk_malloc(sizeof(char*)*cfg->lstm_pp_num*2);
    for(i = 0; i < cfg->lstm_pp_num;++i){
        n = i*2;
        cfg->lstm_pp_weight_fn[n] = wtk_calloc(1,PATH_MAX);
        sprintf(cfg->lstm_pp_weight_fn[n],cfg->lstm_pp_weight_format,"",i);
        cfg->lstm_pp_bias_fn[n] = wtk_calloc(1,PATH_MAX);
        sprintf(cfg->lstm_pp_bias_fn[n],cfg->lstm_pp_bias_format,"",i);
        n = i*2+1;
        cfg->lstm_pp_weight_fn[n] = wtk_calloc(1,PATH_MAX);
        sprintf(cfg->lstm_pp_weight_fn[n],cfg->lstm_pp_weight_format,"_reverse",i);
        cfg->lstm_pp_bias_fn[n] = wtk_calloc(1,PATH_MAX);
        sprintf(cfg->lstm_pp_bias_fn[n],cfg->lstm_pp_bias_format,"_reverse",i);
    }
    //ip
    cfg->lstm_ip_weight_fn = wtk_malloc(sizeof(char*)*cfg->lstm_ip_num*2);
    cfg->lstm_ip_bias_fn = wtk_malloc(sizeof(char*)*cfg->lstm_ip_num*2);
    for(i = 0; i < cfg->lstm_ip_num;++i){
        n = i*2;
        cfg->lstm_ip_weight_fn[n] = wtk_calloc(1,PATH_MAX);
        sprintf(cfg->lstm_ip_weight_fn[n],cfg->lstm_ip_weight_format,"",i);
        cfg->lstm_ip_bias_fn[n] = wtk_calloc(1,PATH_MAX);
        sprintf(cfg->lstm_ip_bias_fn[n],cfg->lstm_ip_bias_format,"",i);
        n = i*2+1;
        cfg->lstm_ip_weight_fn[n] = wtk_calloc(1,PATH_MAX);
        sprintf(cfg->lstm_ip_weight_fn[n],cfg->lstm_ip_weight_format,"_reverse",i);
        cfg->lstm_ip_bias_fn[n] = wtk_calloc(1,PATH_MAX);
        sprintf(cfg->lstm_ip_bias_fn[n],cfg->lstm_ip_bias_format,"_reverse",i);
    }

    ret = 0;
    return ret;
}

