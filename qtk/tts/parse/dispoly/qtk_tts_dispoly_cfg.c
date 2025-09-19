#include "qtk_tts_dispoly_cfg.h"
#include <limits.h>
#ifdef WIN32
#define PATH_MAX 4096
#endif

int qtk_tts_dispoly_cfg_init(qtk_tts_dispoly_cfg_t *cfg)
{
    int ret = 0;
    memset(cfg,0,sizeof(*cfg));
    return ret;
}

int qtk_tts_dispoly_cfg_clean(qtk_tts_dispoly_cfg_t *cfg)
{
    int ret = 0;
    int i = 0;
    for(i = 0; i < cfg->mfnn_num;++i){
        wtk_free(cfg->mfnn_weight_fn[i]);
        wtk_free(cfg->mfnn_bias_fn[i]);
    }
    wtk_free(cfg->mfnn_bias_fn);
    wtk_free(cfg->mfnn_weight_fn);
    for(i = 0; i < cfg->conv_num;++i){
        wtk_free(cfg->conv_weight_fn[i]);
        wtk_free(cfg->conv_bias_fn[i]);
        wtk_free(cfg->conv_layernorm_bias_fn[i]);
        wtk_free(cfg->conv_layernorm_weight_fn[i]);
    }
    wtk_free(cfg->conv_weight_fn);
    wtk_free(cfg->conv_bias_fn);
    wtk_free(cfg->conv_layernorm_bias_fn);
    wtk_free(cfg->conv_layernorm_weight_fn);
    for(i = 0; i < cfg->lstm_num;++i){
        wtk_free(cfg->lstm_weight_fn[i]);
        wtk_free(cfg->lstm_bias_fn[i]);
        wtk_free(cfg->lstm_weight_reverse_fn[i]);
        wtk_free(cfg->lstm_bias_reverse_fn[i]);
    }
    wtk_free(cfg->lstm_weight_fn);
    wtk_free(cfg->lstm_bias_fn);
    wtk_free(cfg->lstm_weight_reverse_fn);
    wtk_free(cfg->lstm_bias_reverse_fn);
    for(i = 0;i < cfg->fnn_num;++i){
        wtk_free(cfg->fnn_weight_fn[i]);
        wtk_free(cfg->fnn_bias_fn[i]);
    }
    wtk_free(cfg->fnn_weight_fn);
    wtk_free(cfg->fnn_bias_fn);

    return ret;
}

int qtk_tts_dispoly_cfg_update_local(qtk_tts_dispoly_cfg_t *cfg,wtk_local_cfg_t *lc)
{
    wtk_string_t *v;

    wtk_local_cfg_update_cfg_i(lc,cfg,mfnn_num,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,conv_num,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,lstm_num,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,fnn_num,v);
    cfg->word_embedding_dim = wtk_local_cfg_find_int_array_s(lc,"word_embedding_dim");
    cfg->cws_embedding_dim = wtk_local_cfg_find_int_array_s(lc,"cws_embedding_dim");
    cfg->pp_embedding_dim = wtk_local_cfg_find_int_array_s(lc,"pp_embedding_dim");
    cfg->flag_embedding_dim = wtk_local_cfg_find_int_array_s(lc,"flag_embedding_dim");
    cfg->poly_mask_dim = wtk_local_cfg_find_int_array_s(lc,"poly_mask_dim");
    cfg->mfnn_dim = wtk_local_cfg_find_int_array_s(lc,"mfnn_dim");
    cfg->conv_dim = wtk_local_cfg_find_int_array_s(lc,"conv_dim");
    cfg->conv_size_dim = wtk_local_cfg_find_int_array_s(lc,"conv_size_dim");
    cfg->lstm_dim = wtk_local_cfg_find_int_array_s(lc,"lstm_dim");
    cfg->fnn_dim = wtk_local_cfg_find_int_array_s(lc,"fnn_dim");
    wtk_local_cfg_update_cfg_str(lc,cfg,word_embedding_fn,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,cws_embedding_fn,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,pp_embedding_fn,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,flag_embedding_fn,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,poly_mask_fn,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,mfnn_fn_format,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,conv_fn_format,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,conv_layernorm_fn_format,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,lstm_fn_format,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,fnn_fn_format,v);
    return 0;
}

int qtk_tts_dispoly_cfg_update(qtk_tts_dispoly_cfg_t *cfg)
{
    int i = 0;
    cfg->mfnn_bias_fn = wtk_malloc(sizeof(char*)*cfg->mfnn_num);
    cfg->mfnn_weight_fn = wtk_malloc(sizeof(char*)*cfg->mfnn_num);
    for(i = 0; i < cfg->mfnn_num;++i){
        cfg->mfnn_weight_fn[i] = wtk_malloc(PATH_MAX);
        sprintf(cfg->mfnn_weight_fn[i],cfg->mfnn_fn_format,i,"weight");
        cfg->mfnn_bias_fn[i] = wtk_malloc(PATH_MAX);
        sprintf(cfg->mfnn_bias_fn[i],cfg->mfnn_fn_format,i,"bias");
    }
    cfg->conv_bias_fn = wtk_malloc(sizeof(char*)*cfg->conv_num);
    cfg->conv_weight_fn = wtk_malloc(sizeof(char*)*cfg->conv_num);
    cfg->conv_layernorm_bias_fn = wtk_malloc(sizeof(char*)*cfg->conv_num);
    cfg->conv_layernorm_weight_fn = wtk_malloc(sizeof(char*)*cfg->conv_num);
    for(i = 0; i < cfg->conv_num;++i){
        cfg->conv_bias_fn[i] = wtk_malloc(PATH_MAX);
        sprintf(cfg->conv_bias_fn[i],cfg->conv_fn_format,i,"bias");
        cfg->conv_weight_fn[i] = wtk_malloc(PATH_MAX);
        sprintf(cfg->conv_weight_fn[i],cfg->conv_fn_format,i,"weight");
        cfg->conv_layernorm_weight_fn[i] = wtk_malloc(PATH_MAX);
        sprintf(cfg->conv_layernorm_weight_fn[i],cfg->conv_layernorm_fn_format,i,"weight");
        cfg->conv_layernorm_bias_fn[i] = wtk_malloc(PATH_MAX);
        sprintf(cfg->conv_layernorm_bias_fn[i],cfg->conv_layernorm_fn_format,i,"bias");
    }
    cfg->lstm_weight_fn = wtk_malloc(sizeof(char*)*cfg->lstm_num);
    cfg->lstm_bias_fn = wtk_malloc(sizeof(char*)*cfg->lstm_num);
    cfg->lstm_weight_reverse_fn = wtk_malloc(sizeof(char*)*cfg->lstm_num);
    cfg->lstm_bias_reverse_fn = wtk_malloc(sizeof(char*)*cfg->lstm_num);
    for(i = 0; i < cfg->lstm_num;++i){
        cfg->lstm_weight_fn[i] = wtk_malloc(PATH_MAX);
        sprintf(cfg->lstm_weight_fn[i],cfg->lstm_fn_format,"",i,"weight");
        cfg->lstm_bias_fn[i] = wtk_malloc(PATH_MAX);
        sprintf(cfg->lstm_bias_fn[i],cfg->lstm_fn_format,"",i,"bias");
        cfg->lstm_weight_reverse_fn[i] = wtk_malloc(PATH_MAX);
        sprintf(cfg->lstm_weight_reverse_fn[i],cfg->lstm_fn_format,"_reverse",i,"weight");
        cfg->lstm_bias_reverse_fn[i] = wtk_malloc(PATH_MAX);
        sprintf(cfg->lstm_bias_reverse_fn[i],cfg->lstm_fn_format,"_reverse",i,"bias");
    }
    cfg->fnn_weight_fn = wtk_malloc(sizeof(char*)*cfg->fnn_num);
    cfg->fnn_bias_fn = wtk_malloc(sizeof(char*)*cfg->fnn_num);
    for(i = 0; i < cfg->fnn_num;++i){
        cfg->fnn_weight_fn[i] = wtk_malloc(PATH_MAX);
        sprintf(cfg->fnn_weight_fn[i],cfg->fnn_fn_format,i,"weight");
        cfg->fnn_bias_fn[i] = wtk_malloc(PATH_MAX);
        sprintf(cfg->fnn_bias_fn[i],cfg->fnn_fn_format,i,"bias");
    }
    return 0;
}
