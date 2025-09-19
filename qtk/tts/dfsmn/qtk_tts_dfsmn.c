#include "qtk_tts_dfsmn.h"

qtk_tts_dfsmn_t* qtk_tts_dfsmn_new(qtk_tts_dfsmn_cfg_t *cfg,wtk_heap_t *heap)
{
    qtk_tts_dfsmn_t *dfs = NULL;
    int layer = cfg->dfsmn_layer,i = 0;
    dfs = wtk_calloc(1, sizeof(*dfs));
    int *dim = NULL;
    wtk_source_loader_t sl;
    wtk_source_t source;
    
    wtk_source_loader_init_file(&sl);
    dfs->cfg = cfg;
    dfs->heap = heap;
    //先linear
    dfs->linear_affine_weight = wtk_malloc(sizeof(wtk_matf_t*)*layer);
    dfs->linear_affine_bias = wtk_malloc(sizeof(wtk_vecf_t*)*layer);
    dfs->linear_projection_weight = wtk_malloc(sizeof(wtk_matf_t*)*layer);
    dfs->linear_projection_bias = wtk_malloc(sizeof(wtk_vecf_t*)*layer);
    dfs->conv_left_kernel_weight = wtk_malloc(sizeof(wtk_matf_t*)*layer);
    dfs->conv_right_kernel_weight = wtk_malloc(sizeof(wtk_matf_t*)*layer);
    dfs->batch_norm_gamma = wtk_malloc(sizeof(wtk_vecf_t*)*layer);
    dfs->batch_norm_beta = wtk_malloc(sizeof(wtk_vecf_t*)*layer);
    dfs->batch_norm_mean = wtk_malloc(sizeof(wtk_vecf_t*)*layer);
    dfs->batch_norm_var = wtk_malloc(sizeof(wtk_vecf_t*)*layer);
    for(i = 0; i < layer; ++i){
        dim = cfg->linear_affine_dim_num->slot;
        dim += i*2;
        dfs->linear_affine_weight[i] = wtk_matf_new(dim[0],dim[1]);
        dfs->linear_affine_bias[i] = wtk_vecf_new(dim[0]);
        wtk_mer_source_loader_load_matf(&sl,&source,cfg->linear_affine_weight_fn[i],dfs->linear_affine_weight[i]);
        wtk_mer_source_loader_load_vecf(&sl,&source,cfg->linear_affine_bias_fn[i],dfs->linear_affine_bias[i]);
        dim = cfg->linear_projection_dim_num->slot;
        dim += i*2;
        dfs->linear_projection_weight[i] = wtk_matf_new(dim[0],dim[1]);
        dfs->linear_projection_bias[i] = wtk_vecf_new(dim[0]);
        wtk_mer_source_loader_load_matf(&sl,&source,cfg->linear_projection_weight_fn[i],dfs->linear_projection_weight[i]);
        wtk_mer_source_loader_load_vecf(&sl,&source,cfg->linear_projection_bias_fn[i],dfs->linear_projection_bias[i]);
        dim = cfg->left_weight_dim_num->slot;
        dim += i*2;
        dfs->conv_left_kernel_weight[i] = wtk_matf_new(dim[0],dim[1]);
        wtk_mer_source_loader_load_matf(&sl,&source,cfg->conv_left_weight_fn[i],dfs->conv_left_kernel_weight[i]);
        dim = cfg->right_weight_dim_num->slot;
        dim += i*2;
        dfs->conv_right_kernel_weight[i] = wtk_matf_new(dim[0],dim[1]);
        wtk_mer_source_loader_load_matf(&sl,&source,cfg->conv_right_weight_fn[i],dfs->conv_right_kernel_weight[i]);
        dfs->batch_norm_gamma[i] = wtk_vecf_new(dim[0]);
        dfs->batch_norm_beta[i] = wtk_vecf_new(dim[0]);
        dfs->batch_norm_mean[i] = wtk_vecf_new(dim[0]);
        dfs->batch_norm_var[i] = wtk_vecf_new(dim[0]);
        wtk_mer_source_loader_load_vecf(&sl,&source,cfg->batch_norm_gamma_fn[i],dfs->batch_norm_gamma[i]);
        wtk_mer_source_loader_load_vecf(&sl,&source,cfg->batch_norm_beta_fn[i],dfs->batch_norm_beta[i]);
        wtk_mer_source_loader_load_vecf(&sl,&source,cfg->batch_norm_mean_fn[i],dfs->batch_norm_mean[i]);
        wtk_mer_source_loader_load_vecf(&sl,&source,cfg->batch_norm_var_fn[i],dfs->batch_norm_var[i]);
    }
    //双向gru
    dim = cfg->rnn_gru_dim_num->slot;
    dfs->rnn_gru = wtk_malloc(sizeof(wtk_nn_rnngru_t*)*2);
    dfs->rnn_gru[0] = wtk_nn_rnngru_new2(wtk_nn_enum_type_pytorch,1,dim[0],dim[1],NULL);
    dfs->rnn_gru[1] = wtk_nn_rnngru_new2(wtk_nn_enum_type_pytorch,0,dim[2],dim[3],NULL);
    for(i = 0; i < 2; ++i){
        wtk_mer_source_loader_load_matf(&sl,&source,cfg->gru_weight_gate_fn[i],dfs->rnn_gru[i]->gate_kernel);
        wtk_mer_source_loader_load_matf(&sl,&source,cfg->gru_weight_cand_fn[i],dfs->rnn_gru[i]->candidate_kernel);
        wtk_mer_source_loader_load_matf(&sl,&source,cfg->gru_weight_cand_hh_fn[i],dfs->rnn_gru[i]->candidate_kernel_hh);
        wtk_mer_source_loader_load_vecf(&sl,&source,cfg->gru_bias_gate_fn[i],dfs->rnn_gru[i]->gate_bias);
        wtk_mer_source_loader_load_vecf(&sl,&source,cfg->gru_bias_cand_fn[i],dfs->rnn_gru[i]->candidate_bias);
        wtk_mer_source_loader_load_vecf(&sl,&source,cfg->gru_bias_cand_hh_fn[i],dfs->rnn_gru[i]->candidate_bias_hh);
    }
    return dfs;
}

//双重加法 in1 + in2 + in3
int qtk_tts_dfsmn_matf_add_second(wtk_matf_t *dst,wtk_matf_t *in1,wtk_matf_t *in2,wtk_matf_t *in3)
{
    if(in1->col != in2->col || in2->col != in3->col){
        wtk_debug("add dim error");
        return -1;
    }
    if(in1->row != in2->row || in2->row != in3->row){
        wtk_debug("add dim error");
        return -1;
    }
    int n = in1->row * in1->col;
    int i = 0;
    float *dp = dst->p,*inp1 = in1->p,*inp2 = in2->p,*inp3 = in3->p;
    for(i = 0; i < n; ++i){
        dp[i] = inp1[i] + inp2[i] + inp3[i];
    }
    return 0;
}

int qtk_tts_dfsmn_process(qtk_tts_dfsmn_t *dfs,wtk_matf_t *in,wtk_matf_t *out)
{
    int ret = -1;
    int layer = dfs->cfg->dfsmn_layer, i = 0;
    wtk_heap_t *heap = dfs->heap;
    int *pad_dim = NULL,*kernel_size = NULL;
    int pad_row = 0;
    wtk_matf_t *dout = wtk_matf_heap_new(heap,in->row,dfs->linear_affine_weight[i]->row);
    //left pad之后的值一样
    pad_dim = dfs->cfg->left_pad_num->slot;
    pad_row = in->row + pad_dim[0] + pad_dim[1];
    wtk_matf_t *left_pad = wtk_matf_heap_new(heap,pad_row,in->col);
    pad_dim = dfs->cfg->right_pad_num->slot;
    pad_row = in->row + pad_dim[0] + pad_dim[1];
    wtk_matf_t *right_pad = wtk_matf_heap_new(heap,pad_row,in->col);
    wtk_matf_t *out_left_conv = wtk_matf_heap_new(heap,in->row,dfs->conv_left_kernel_weight[0]->row);   //在tac中输出的列向量是一样的
    wtk_matf_t *out_right_conv = wtk_matf_heap_new(heap,in->row,dfs->conv_left_kernel_weight[0]->row);
    wtk_matf_t *add_out = wtk_matf_heap_new(heap,out_right_conv->row,out_right_conv->col);
    wtk_matf_t *out_tmp = wtk_matf_heap_new(heap,in->row,in->col);
    
    //dfsmn block
    for(i = 0; i < layer; ++i){
        wtk_nn_layer_dense(in,dfs->linear_affine_weight[i],dfs->linear_affine_bias[i],wtk_nn_relu,dout);
        wtk_nn_layer_dense(dout,dfs->linear_projection_weight[i],dfs->linear_projection_bias[i],NULL,out_tmp);
        pad_dim = dfs->cfg->left_pad_num->slot;
        pad_dim += i * 2;
        wtk_nn_constant_pad1d(out_tmp,left_pad,pad_dim[0],pad_dim[1]);
        kernel_size = dfs->cfg->left_kernel_size_num->slot;
        wtk_nn_conv1d_group(dfs->conv_left_kernel_weight[i],kernel_size[i],dfs->cfg->conv_left_groups,left_pad,out_left_conv);
        pad_dim = dfs->cfg->right_pad_num->slot;
        pad_dim += i * 2;
        wtk_nn_constant_pad1d(out_tmp,right_pad,pad_dim[0],pad_dim[1]);
        kernel_size = dfs->cfg->right_kernel_size_num->slot;
        wtk_nn_conv1d_group(dfs->conv_right_kernel_weight[i],kernel_size[i],dfs->cfg->conv_right_groups,right_pad,out_right_conv);
        qtk_tts_dfsmn_matf_add_second(add_out,out_tmp,out_left_conv,out_right_conv);
        wtk_nn_batch_norm(add_out,dfs->batch_norm_gamma[i],dfs->batch_norm_beta[i],
                                                    dfs->batch_norm_mean[i],dfs->batch_norm_var[i],NORM_EPSILON);
        wtk_matf_add(in,in,add_out);
    }
    wtk_matf_t *gru_fw = wtk_matf_heap_new(heap,in->row,dfs->rnn_gru[0]->num_units);
    wtk_matf_t *gru_bw = wtk_matf_heap_new(heap,in->row,dfs->rnn_gru[1]->num_units);
    wtk_nn_rnngru_reset(dfs->rnn_gru[0]);
    wtk_nn_rnngru_reset(dfs->rnn_gru[1]);
    wtk_nn_rnngru(dfs->rnn_gru[0],in,gru_fw);
    wtk_nn_rnngru(dfs->rnn_gru[1],in,gru_bw);
    for(i = 0;i < in->row; ++i){
        memcpy(out->p+out->col*i,gru_fw->p+gru_fw->col*i,gru_fw->col*sizeof(float));
        memcpy(out->p+out->col*i+gru_fw->col,gru_bw->p+gru_bw->col*i,gru_bw->col*sizeof(float));
    }
    return ret;
}

int qtk_tts_dfsmn_delete(qtk_tts_dfsmn_t *dfs)
{
    int layer = dfs->cfg->dfsmn_layer;
    int i = 0;
    for(i = 0; i < layer; ++i){
        wtk_matf_delete(dfs->linear_affine_weight[i]);
        wtk_vecf_delete(dfs->linear_affine_bias[i]);
        wtk_matf_delete(dfs->linear_projection_weight[i]);
        wtk_vecf_delete(dfs->linear_projection_bias[i]);
        wtk_matf_delete(dfs->conv_left_kernel_weight[i]);
        wtk_matf_delete(dfs->conv_right_kernel_weight[i]);
        wtk_vecf_delete(dfs->batch_norm_gamma[i]);
        wtk_vecf_delete(dfs->batch_norm_beta[i]);
        wtk_vecf_delete(dfs->batch_norm_var[i]);
        wtk_vecf_delete(dfs->batch_norm_mean[i]);
    }
    wtk_free(dfs->batch_norm_gamma);
    wtk_free(dfs->batch_norm_beta);
    wtk_free(dfs->batch_norm_mean);
    wtk_free(dfs->batch_norm_var);
    wtk_free(dfs->conv_left_kernel_weight);
    wtk_free(dfs->conv_right_kernel_weight);
    wtk_free(dfs->linear_affine_weight);
    wtk_free(dfs->linear_affine_bias);
    wtk_free(dfs->linear_projection_weight);
    wtk_free(dfs->linear_projection_bias);
    for(i = 0; i < 2; ++i){
        wtk_nn_rnngru_delete(dfs->rnn_gru[i]);
    }
    wtk_free(dfs->rnn_gru);
    wtk_free(dfs);
    return 0;
}
