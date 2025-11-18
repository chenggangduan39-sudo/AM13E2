#include "qtk_tts_dfsmn_cfg.h"
#include "qtk_tts_comm.h"

int qtk_tts_dfsmn_cfg_init(qtk_tts_dfsmn_cfg_t *cfg)
{
    memset(cfg, 0, sizeof(qtk_tts_dfsmn_cfg_t));
    return 0;
}

int qtk_tts_dfsmn_cfg_clean(qtk_tts_dfsmn_cfg_t *cfg)
{
    int i = 0,n = cfg->dfsmn_layer;
    for(i = 0; i < n; ++i){
        wtk_free(cfg->linear_affine_weight_fn[i]);
        wtk_free(cfg->linear_affine_bias_fn[i]);
        wtk_free(cfg->linear_projection_weight_fn[i]);
        wtk_free(cfg->linear_projection_bias_fn[i]);
        wtk_free(cfg->conv_left_weight_fn[i]);
        wtk_free(cfg->conv_right_weight_fn[i]);
        wtk_free(cfg->batch_norm_gamma_fn[i]);
        wtk_free(cfg->batch_norm_beta_fn[i]);
        wtk_free(cfg->batch_norm_mean_fn[i]);
        wtk_free(cfg->batch_norm_var_fn[i]);
    }
    wtk_free(cfg->batch_norm_var_fn);
    wtk_free(cfg->batch_norm_mean_fn);
    wtk_free(cfg->batch_norm_beta_fn);
    wtk_free(cfg->batch_norm_gamma_fn);
    wtk_free(cfg->conv_left_weight_fn);
    wtk_free(cfg->conv_right_weight_fn);
    wtk_free(cfg->linear_affine_weight_fn);
    wtk_free(cfg->linear_affine_bias_fn);
    wtk_free(cfg->linear_projection_weight_fn);
    wtk_free(cfg->linear_projection_bias_fn);
    for(i = 0; i < 2; ++i){
        wtk_free(cfg->gru_weight_gate_fn[i]);
        wtk_free(cfg->gru_weight_cand_fn[i]);
        wtk_free(cfg->gru_weight_cand_hh_fn[i]);
        wtk_free(cfg->gru_bias_gate_fn[i]);
        wtk_free(cfg->gru_bias_cand_fn[i]);
        wtk_free(cfg->gru_bias_cand_hh_fn[i]);
    }
    wtk_free(cfg->gru_weight_gate_fn);
    wtk_free(cfg->gru_weight_cand_fn);
    wtk_free(cfg->gru_weight_cand_hh_fn);
    wtk_free(cfg->gru_bias_gate_fn);
    wtk_free(cfg->gru_bias_cand_fn);
    wtk_free(cfg->gru_bias_cand_hh_fn);
    return 0;
}

int qtk_tts_dfsmn_cfg_update_local(qtk_tts_dfsmn_cfg_t *cfg,wtk_local_cfg_t *lc)
{
    wtk_string_t *v = NULL;
    wtk_local_cfg_t *main_lc = lc;

    wtk_local_cfg_update_cfg_i(main_lc,cfg,dfsmn_layer,v);
    wtk_local_cfg_update_cfg_str(main_lc,cfg,linear_fn_format,v);
    cfg->linear_affine_dim_num = wtk_local_cfg_find_int_array_s(main_lc,"linear_affine_dim_num");
    cfg->linear_projection_dim_num = wtk_local_cfg_find_int_array_s(main_lc,"linear_projection_dim_num");
    wtk_local_cfg_update_cfg_str(main_lc,cfg,conv_fn_format,v);
    cfg->left_pad_num = wtk_local_cfg_find_int_array_s(main_lc,"conv_left_pad_dim_num");
    cfg->right_pad_num = wtk_local_cfg_find_int_array_s(main_lc,"conv_right_pad_dim_num");
    cfg->left_weight_dim_num = wtk_local_cfg_find_int_array_s(main_lc,"conv_left_kernel_dim_num");
    cfg->left_kernel_size_num = wtk_local_cfg_find_int_array_s(main_lc,"conv_left_kernel_size");
    cfg->right_weight_dim_num = wtk_local_cfg_find_int_array_s(main_lc,"conv_right_kernel_dim_num");
    cfg->right_kernel_size_num = wtk_local_cfg_find_int_array_s(main_lc,"conv_right_kernel_size");
    wtk_local_cfg_update_cfg_i(main_lc,cfg,conv_left_groups,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,conv_right_groups,v);
    wtk_local_cfg_update_cfg_str(main_lc,cfg,batch_norm_fn_format,v);
    cfg->rnn_gru_dim_num = wtk_local_cfg_find_int_array_s(main_lc,"rnn_dim_num");
    wtk_local_cfg_update_cfg_str(main_lc,cfg,gru_fn_format,v);
    return 0;
}

int qtk_tts_dfsmn_cfg_update(qtk_tts_dfsmn_cfg_t *cfg)
{
    int n = cfg->dfsmn_layer,i = 0;
    cfg->linear_affine_weight_fn = wtk_malloc(n*sizeof(char*));
    cfg->linear_affine_bias_fn = wtk_malloc(n*sizeof(char*));
    cfg->linear_projection_weight_fn = wtk_malloc(n*sizeof(char*));
    cfg->linear_projection_bias_fn = wtk_malloc(n*sizeof(char*));
    cfg->conv_left_weight_fn = wtk_malloc(n*sizeof(char*));
    cfg->conv_right_weight_fn = wtk_malloc(n*sizeof(char*));
    cfg->batch_norm_beta_fn = wtk_malloc(n*sizeof(char*));
    cfg->batch_norm_gamma_fn = wtk_malloc(n*sizeof(char*));
    cfg->batch_norm_mean_fn = wtk_malloc(n*sizeof(char*));
    cfg->batch_norm_var_fn = wtk_malloc(n*sizeof(char*));
    for(i = 0; i < n; ++i){
        cfg->linear_affine_weight_fn[i] = PATH_FORMAT_TO(cfg->linear_fn_format,"affine_weight",i);
        cfg->linear_affine_bias_fn[i] = PATH_FORMAT_TO(cfg->linear_fn_format,"affine_bias",i);
        cfg->linear_projection_weight_fn[i] = PATH_FORMAT_TO(cfg->linear_fn_format,"projection_weight",i);
        cfg->linear_projection_bias_fn[i] = PATH_FORMAT_TO(cfg->linear_fn_format,"projection_bias",i);
        cfg->conv_right_weight_fn[i] = PATH_FORMAT_TO(cfg->conv_fn_format,"right_conv1d_kernel",i);
        cfg->conv_left_weight_fn[i] = PATH_FORMAT_TO(cfg->conv_fn_format,"left_conv1d_kernel",i);
        cfg->batch_norm_gamma_fn[i] = PATH_FORMAT_TO(cfg->batch_norm_fn_format,"gamma",i);
        cfg->batch_norm_beta_fn[i] = PATH_FORMAT_TO(cfg->batch_norm_fn_format,"beta",i);
        cfg->batch_norm_mean_fn[i] = PATH_FORMAT_TO(cfg->batch_norm_fn_format,"mean",i);
        cfg->batch_norm_var_fn[i] = PATH_FORMAT_TO(cfg->batch_norm_fn_format,"var",i);
    }
    //是一个
    cfg->gru_weight_gate_fn = wtk_malloc(sizeof(char*)*2);
    cfg->gru_weight_cand_fn = wtk_malloc(sizeof(char*)*2);
    cfg->gru_weight_cand_hh_fn = wtk_malloc(sizeof(char*)*2);
    cfg->gru_bias_gate_fn = wtk_malloc(sizeof(char*)*2);
    cfg->gru_bias_cand_fn = wtk_malloc(sizeof(char*)*2);
    cfg->gru_bias_cand_hh_fn = wtk_malloc(sizeof(char*)*2);
    cfg->gru_weight_gate_fn[0] = PATH_FORMAT_TO(cfg->gru_fn_format,"weight_gate");
    cfg->gru_weight_cand_fn[0] = PATH_FORMAT_TO(cfg->gru_fn_format,"weight_candidate");
    cfg->gru_weight_cand_hh_fn[0] = PATH_FORMAT_TO(cfg->gru_fn_format,"weight_candidate_hh");
    cfg->gru_bias_gate_fn[0] = PATH_FORMAT_TO(cfg->gru_fn_format,"bias_gate");
    cfg->gru_bias_cand_fn[0] = PATH_FORMAT_TO(cfg->gru_fn_format,"bias_candidate");
    cfg->gru_bias_cand_hh_fn[0] = PATH_FORMAT_TO(cfg->gru_fn_format,"bias_candidate_hh");
    cfg->gru_weight_gate_fn[1] = PATH_FORMAT_TO(cfg->gru_fn_format,"weight_reverse_gate");
    cfg->gru_weight_cand_fn[1] = PATH_FORMAT_TO(cfg->gru_fn_format,"weight_reverse_candidate");
    cfg->gru_weight_cand_hh_fn[1] = PATH_FORMAT_TO(cfg->gru_fn_format,"weight_reverse_candidate_hh");
    cfg->gru_bias_gate_fn[1] = PATH_FORMAT_TO(cfg->gru_fn_format,"bias_reverse_gate");
    cfg->gru_bias_cand_fn[1] = PATH_FORMAT_TO(cfg->gru_fn_format,"bias_reverse_candidate");
    cfg->gru_bias_cand_hh_fn[1] = PATH_FORMAT_TO(cfg->gru_fn_format,"bias_reverse_candidate_hh");
    return 0;
}
