#include "qtk_durian_encoder_cfg.h"
#include "qtk_tts_comm.h"

int qtk_durian_encoder_cfg_init(qtk_durian_encoder_cfg_t *cfg)
{
    memset(cfg, 0, sizeof(*cfg));
    return 0;
}

int qtk_durian_encoder_cfg_clean(qtk_durian_encoder_cfg_t *cfg)
{
    int i = 0;

    for(i = 0; i < 2; ++i){
        wtk_free(cfg->gru_gate_weight_fn[i]);
        wtk_free(cfg->gru_gate_bias_fn[i]);
        wtk_free(cfg->gru_candidate_weight_fn[i]);
        wtk_free(cfg->gru_candidate_bias_fn[i]);
        wtk_free(cfg->gru_candidate_hh_weight_fn[i]);
        wtk_free(cfg->gru_candidate_hh_bias_fn[i]);
    }
    wtk_free(cfg->gru_gate_weight_fn);
    wtk_free(cfg->gru_gate_bias_fn);
    wtk_free(cfg->gru_candidate_weight_fn);
    wtk_free(cfg->gru_candidate_bias_fn);
    wtk_free(cfg->gru_candidate_hh_weight_fn);
    wtk_free(cfg->gru_candidate_hh_bias_fn);

    for(i = 0; i < cfg->cbhg_num_highway_layers; ++i){
        wtk_free(cfg->highway_T_weight_fn[i]);
        wtk_free(cfg->highway_T_bias_fn[i]);
        wtk_free(cfg->highway_H_weight_fn[i]);
        wtk_free(cfg->highway_H_bias_fn[i]);
    }
    wtk_free(cfg->highway_H_weight_fn);
    wtk_free(cfg->highway_H_bias_fn);
    wtk_free(cfg->highway_T_weight_fn);
    wtk_free(cfg->highway_T_bias_fn);
    
    for(i = 0; i < 2; ++i){
        wtk_free(cfg->conv1d_prodections_conv_fn[i]);
        wtk_free(cfg->conv1d_prodections_batchnorm_gamma_fn[i]);
        wtk_free(cfg->conv1d_prodections_batchnorm_beta_fn[i]);
        wtk_free(cfg->conv1d_prodections_batchnorm_mean_fn[i]);
        wtk_free(cfg->conv1d_prodections_batchnorm_var_fn[i]);
    }
    wtk_free(cfg->conv1d_prodections_conv_fn);
    wtk_free(cfg->conv1d_prodections_batchnorm_gamma_fn);
    wtk_free(cfg->conv1d_prodections_batchnorm_beta_fn);
    wtk_free(cfg->conv1d_prodections_batchnorm_mean_fn);
    wtk_free(cfg->conv1d_prodections_batchnorm_var_fn);

    for(i = 0; i < cfg->cbhg_max_kernel_size; ++i){
        wtk_free(cfg->conv1d_bank_conv_fn[i]);
        wtk_free(cfg->conv1d_bank_batchnorm_gamma_fn[i]);
        wtk_free(cfg->conv1d_bank_batchnorm_beta_fn[i]);
        wtk_free(cfg->conv1d_bank_batchnorm_mean_fn[i]);
        wtk_free(cfg->conv1d_bank_batchnorm_var_fn[i]);
    }
    wtk_free(cfg->conv1d_bank_conv_fn);
    wtk_free(cfg->conv1d_bank_batchnorm_gamma_fn);
    wtk_free(cfg->conv1d_bank_batchnorm_beta_fn);
    wtk_free(cfg->conv1d_bank_batchnorm_mean_fn);
    wtk_free(cfg->conv1d_bank_batchnorm_var_fn);
    return 0;
}

int qtk_durian_encoder_cfg_update_local(qtk_durian_encoder_cfg_t *cfg,wtk_local_cfg_t *main_lc)
{
    wtk_string_t *v = NULL;

    wtk_local_cfg_update_cfg_i(main_lc,cfg,embedding_input_size,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,cbhg_max_kernel_size,v);
    wtk_local_cfg_update_cfg_str(main_lc,cfg,conv1d_bank_fn_format,v);
    cfg->cbhg_projection_sizes = wtk_local_cfg_find_int_array_s(main_lc,"cbhg_projection_sizes");
    wtk_local_cfg_update_cfg_str(main_lc,cfg,conv1d_projections_fn_format,v);
    cfg->pre_highway_dim = wtk_local_cfg_find_int_array_s(main_lc,"pre_highway_dim");
    wtk_local_cfg_update_cfg_str(main_lc,cfg,pre_highway_fn,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,cbhg_num_highway_layers,v);
    wtk_local_cfg_update_cfg_str(main_lc,cfg,highway_fn_format,v);
    wtk_local_cfg_update_cfg_str(main_lc,cfg,rnn_fn_format,v);

    return 0;
}

int qtk_durian_encoder_cfg_update(qtk_durian_encoder_cfg_t *cfg)
{
    int i = 0,n = cfg->cbhg_max_kernel_size;

    cfg->conv1d_bank_conv_fn = wtk_malloc(n*sizeof(char*));
    cfg->conv1d_bank_batchnorm_gamma_fn = wtk_malloc(n*sizeof(char*));
    cfg->conv1d_bank_batchnorm_beta_fn = wtk_malloc(n*sizeof(char*));
    cfg->conv1d_bank_batchnorm_mean_fn = wtk_malloc(n*sizeof(char*));
    cfg->conv1d_bank_batchnorm_var_fn = wtk_malloc(n*sizeof(char*));
    for(i = 0; i < n; ++i){
        cfg->conv1d_bank_conv_fn[i] = PATH_FORMAT_TO(cfg->conv1d_bank_fn_format,"conv1d",i);
        cfg->conv1d_bank_batchnorm_gamma_fn[i] = PATH_FORMAT_TO(cfg->conv1d_bank_fn_format,"batchnorm_gamma",i);
        cfg->conv1d_bank_batchnorm_beta_fn[i] = PATH_FORMAT_TO(cfg->conv1d_bank_fn_format,"batchnorm_beta",i);
        cfg->conv1d_bank_batchnorm_mean_fn[i] = PATH_FORMAT_TO(cfg->conv1d_bank_fn_format,"batchnorm_mean",i);
        cfg->conv1d_bank_batchnorm_var_fn[i] = PATH_FORMAT_TO(cfg->conv1d_bank_fn_format,"batchnorm_var",i);
    }

    cfg->conv1d_prodections_conv_fn = wtk_malloc(2*sizeof(char*));
    cfg->conv1d_prodections_batchnorm_gamma_fn = wtk_malloc(2*sizeof(char*));
    cfg->conv1d_prodections_batchnorm_beta_fn = wtk_malloc(2*sizeof(char*));
    cfg->conv1d_prodections_batchnorm_mean_fn = wtk_malloc(2*sizeof(char*));
    cfg->conv1d_prodections_batchnorm_var_fn = wtk_malloc(2*sizeof(char*));
    for(i = 0; i < 2; ++i){
        cfg->conv1d_prodections_conv_fn[i] = PATH_FORMAT_TO(cfg->conv1d_projections_fn_format,"conv1d",i);
        cfg->conv1d_prodections_batchnorm_gamma_fn[i] = PATH_FORMAT_TO(cfg->conv1d_projections_fn_format,"batchnorm_gamma",i);
        cfg->conv1d_prodections_batchnorm_beta_fn[i] = PATH_FORMAT_TO(cfg->conv1d_projections_fn_format,"batchnorm_beta",i);
        cfg->conv1d_prodections_batchnorm_mean_fn[i] = PATH_FORMAT_TO(cfg->conv1d_projections_fn_format,"batchnorm_mean",i);
        cfg->conv1d_prodections_batchnorm_var_fn[i] = PATH_FORMAT_TO(cfg->conv1d_projections_fn_format,"batchnorm_var",i);
    }
    n = cfg->cbhg_num_highway_layers;
    cfg->highway_H_weight_fn = wtk_malloc(n*sizeof(char*));
    cfg->highway_H_bias_fn = wtk_malloc(n*sizeof(char*));
    cfg->highway_T_weight_fn = wtk_malloc(n*sizeof(char*));
    cfg->highway_T_bias_fn = wtk_malloc(n*sizeof(char*));
    for(i = 0; i < n; ++i){
        cfg->highway_T_weight_fn[i] = PATH_FORMAT_TO(cfg->highway_fn_format,"T_weight",i);
        cfg->highway_T_bias_fn[i] = PATH_FORMAT_TO(cfg->highway_fn_format,"T_bias",i);
        cfg->highway_H_weight_fn[i] = PATH_FORMAT_TO(cfg->highway_fn_format,"H_weight",i);
        cfg->highway_H_bias_fn[i] = PATH_FORMAT_TO(cfg->highway_fn_format,"H_bias",i);
    }
    //bigru
    cfg->gru_gate_weight_fn = wtk_malloc(2*sizeof(char*));
    cfg->gru_gate_bias_fn = wtk_malloc(2*sizeof(char*));
    cfg->gru_candidate_weight_fn = wtk_malloc(2*sizeof(char*));
    cfg->gru_candidate_bias_fn = wtk_malloc(2*sizeof(char*));
    cfg->gru_candidate_hh_weight_fn = wtk_malloc(2*sizeof(char*));
    cfg->gru_candidate_hh_bias_fn = wtk_malloc(2*sizeof(char*));
    for(i = 0; i < 2; ++i){
        char *r = NULL;
        r = i==0?"":"_reverse";
        cfg->gru_gate_weight_fn[i] = PATH_FORMAT_TO(cfg->rnn_fn_format,"gate",r,"weight");
        cfg->gru_candidate_weight_fn[i] = PATH_FORMAT_TO(cfg->rnn_fn_format,"candidate",r,"weight");
        cfg->gru_candidate_hh_weight_fn[i] = PATH_FORMAT_TO(cfg->rnn_fn_format,"candidate_hh",r,"weight");
        cfg->gru_gate_bias_fn[i] = PATH_FORMAT_TO(cfg->rnn_fn_format,"gate",r,"bias");
        cfg->gru_candidate_bias_fn[i] = PATH_FORMAT_TO(cfg->rnn_fn_format,"candidate",r,"bias");
        cfg->gru_candidate_hh_bias_fn[i] = PATH_FORMAT_TO(cfg->rnn_fn_format,"candidate_hh",r,"bias");
    }

    return 0;
}
