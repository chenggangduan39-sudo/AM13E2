#include "qtk_durian_encoder.h"


qtk_durian_encoder_t* qtk_durian_encoder_new(qtk_durian_encoder_cfg_t *cfg,wtk_heap_t *heap)
{
    qtk_durian_encoder_t *enc = NULL;
    int kernel_max_size = 0;
    int input_size = 0;
    int i = 0,*dim = NULL;
    qtk_durian_batchnormconv_t *bnc = NULL;

    enc = wtk_malloc(sizeof(*enc));
    assert(enc != NULL);

    enc->cfg = cfg;
    enc->heap = heap;
    kernel_max_size = cfg->cbhg_max_kernel_size;
    input_size = cfg->embedding_input_size;
    enc->conv1d_back = wtk_malloc(kernel_max_size*sizeof(qtk_durian_batchnormconv_t*));
    for(i = 0; i < kernel_max_size; ++i){
        bnc = qtk_durian_batchnormconv_new(input_size,input_size,i+1,1,(i+1)/2);
        qtk_durian_batchnormconv_loadfile(bnc,cfg->conv1d_bank_conv_fn[i],NULL,
                                                                                            cfg->conv1d_bank_batchnorm_gamma_fn[i],
                                                                                            cfg->conv1d_bank_batchnorm_beta_fn[i],
                                                                                            cfg->conv1d_bank_batchnorm_mean_fn[i],
                                                                                            cfg->conv1d_bank_batchnorm_var_fn[i]);
        enc->conv1d_back[i] = bnc;
    }
    enc->maxpool = qtk_nn_maxpool1d_new(2,1,1); //最大池化
    dim = cfg->cbhg_projection_sizes->slot;
    enc->conv1d_projections = wtk_malloc(sizeof(qtk_durian_batchnormconv_t*)*2); //算法限定了这个convbatchnome只能是2个
    enc->conv1d_projections[0] = qtk_durian_batchnormconv_new(kernel_max_size*input_size,dim[0],3,1,1); //kernel 之类的是默认的
    enc->conv1d_projections[1] = qtk_durian_batchnormconv_new(dim[1],dim[1],3,1,1);
    for(i = 0; i < 2; ++i){
        qtk_durian_batchnormconv_loadfile(enc->conv1d_projections[i],cfg->conv1d_prodections_conv_fn[i],NULL,
                                                                                        cfg->conv1d_prodections_batchnorm_gamma_fn[i],
                                                                                        cfg->conv1d_prodections_batchnorm_beta_fn[i],
                                                                                        cfg->conv1d_prodections_batchnorm_mean_fn[i],
                                                                                        cfg->conv1d_prodections_batchnorm_var_fn[i]);
    }
    dim = cfg->pre_highway_dim->slot;
    enc->pre_highway = qtk_nn_fc_new(dim[0],dim[1],QTK_NN_ACTINATION_NULL,0,1); //改变模型形状不引入非线性
    qtk_nn_fc_load_file(enc->pre_highway,cfg->pre_highway_fn,NULL);
    enc->highway = wtk_malloc(cfg->cbhg_num_highway_layers*sizeof(qtk_durian_highway_t*));
    for(i = 0; i < cfg->cbhg_num_highway_layers; ++i){
        enc->highway[i] = qtk_durian_highway_new(input_size,input_size);
        qtk_durian_highwav_loadfile(enc->highway[i],cfg->highway_H_weight_fn[i],cfg->highway_H_bias_fn[i],
                                                                                                        cfg->highway_T_weight_fn[i],cfg->highway_T_bias_fn[i]);
    }
    enc->bigru = qtk_nn_bigru_new(input_size,input_size,1);
    qtk_nn_bigru_loadfile(enc->bigru,cfg->gru_gate_weight_fn,cfg->gru_candidate_weight_fn,cfg->gru_candidate_hh_weight_fn,
                                                    cfg->gru_gate_bias_fn,cfg->gru_candidate_bias_fn,cfg->gru_candidate_hh_bias_fn);
    return enc;
}

int qtk_durian_encoder_delete(qtk_durian_encoder_t *enc)
{
    int i = 0;
    
    qtk_nn_bigru_delete(enc->bigru);
    for(i = 0; i < enc->cfg->cbhg_num_highway_layers; ++i){
        qtk_durian_highway_delete(enc->highway[i]);
    }
    wtk_free(enc->highway);
    qtk_nn_fc_delete(enc->pre_highway);
    for(i = 0; i < 2; ++i){
        qtk_durian_batchnormconv_delete(enc->conv1d_projections[i]);
    }
    wtk_free(enc->conv1d_projections);
    qtk_nn_maxpool1d_delete(enc->maxpool);
    for(i = 0; i < enc->cfg->cbhg_max_kernel_size; ++i){
        qtk_durian_batchnormconv_delete(enc->conv1d_back[i]);
    }
    wtk_free(enc->conv1d_back);
    wtk_free(enc);
    return 0;
}

wtk_matf_t* qtk_durian_encoder_bank_process(qtk_durian_encoder_t *enc,wtk_matf_t *input)
{
    wtk_heap_t *heap = enc->heap;
    int i = 0,j = 0;
    int n = enc->cfg->cbhg_max_kernel_size;
    int out_col = 0;
    int conv_row = 0;

    wtk_matf_t **conv1d_bank_out_tmp = wtk_heap_malloc(heap,sizeof(wtk_matf_t*)*n);
    for(i = 0; i < n; ++i){
        conv_row = qtk_nn_conv1d_out_row(enc->conv1d_back[i]->conv1d,input->row);
        conv1d_bank_out_tmp[i] = wtk_matf_heap_new(heap,conv_row,enc->conv1d_back[i]->conv1d->out_dim);
        qtk_durian_batchnormconv_process(enc->conv1d_back[i],input,conv1d_bank_out_tmp[i]);
        out_col += enc->conv1d_back[i]->conv1d->out_dim;
    }
    wtk_matf_t *conv1d_bank_out = wtk_matf_heap_new(heap,input->row,out_col);
    //cat
    float *p = conv1d_bank_out->p;
    wtk_matf_t *tmp = NULL;
    for(j = 0;j < input->row; ++j){
        for(i = 0; i < n; ++i){
            tmp = conv1d_bank_out_tmp[i];
            memcpy(p, tmp->p+j*tmp->col,sizeof(float)*tmp->col);
            p+=tmp->col;
        }
    }
    // wtk_matf_print(conv1d_bank_out);
    return conv1d_bank_out;
}

wtk_matf_t* qtk_durian_encoder_projections_process(qtk_durian_encoder_t *enc,wtk_matf_t *input)
{
    wtk_matf_t *output = NULL;
    int i = 0;
    wtk_heap_t *heap = enc->heap;
    int i_row = 0;

    for(i = 0; i < 2; ++i){
        i_row = qtk_nn_conv1d_out_row(enc->conv1d_projections[i]->conv1d,input->row);
        output = wtk_matf_heap_new(heap,i_row,enc->conv1d_projections[i]->conv1d->out_dim);
        qtk_durian_batchnormconv_process(enc->conv1d_projections[i],input,output);
        input = output;
    }
    return output;
}

wtk_matf_t* qtk_durian_encoder_highway_process(qtk_durian_encoder_t *enc,wtk_matf_t *in)
{
    int i = 0,n = enc->cfg->cbhg_num_highway_layers;
    wtk_matf_t *tmp = NULL;
    wtk_heap_t *heap = enc->heap;

    for(i = 0; i < n; ++i){
        tmp = wtk_matf_heap_new(heap,in->row,enc->highway[i]->H->kernel->row);
        qtk_durian_highway_process(enc->highway[i],in,tmp);
        in = tmp;
    }
    return tmp;
}

wtk_matf_t* qtk_durian_encoder_process(qtk_durian_encoder_t *enc,wtk_matf_t *input)
{
    wtk_matf_t *enc_out = NULL;
    wtk_matf_t *bank_out = NULL;

    bank_out = qtk_durian_encoder_bank_process(enc,input);
    wtk_matf_t *maxpool_out = wtk_matf_heap_new(enc->heap,qtk_nn_maxpool1d_out_row(enc->maxpool,bank_out->row),bank_out->col);
    qtk_nn_maxpool1d_forward(enc->maxpool,bank_out,maxpool_out);
    //更改maxpool的维度
    wtk_matf_t project_in;
    project_in.p = maxpool_out->p;
    project_in.row = input->row;
    project_in.col = maxpool_out->col;
    wtk_matf_t *project_out = qtk_durian_encoder_projections_process(enc,&project_in);
    wtk_matf_t *pre_highway_out = wtk_matf_heap_new(enc->heap,project_out->row,enc->pre_highway->kernel->row);
    qtk_nn_fc_forward(enc->pre_highway,project_out,pre_highway_out);
    wtk_matf_add(pre_highway_out,pre_highway_out,input);
    wtk_matf_t *highway_out = qtk_durian_encoder_highway_process(enc,pre_highway_out);
    enc_out = wtk_matf_heap_new(enc->heap,highway_out->row,enc->bigru->forward->num_units*2);
    qtk_nn_bigru_forward(enc->bigru,highway_out,enc_out);
    return enc_out;
}

////////////////// batchnormconv1d

qtk_durian_batchnormconv_t* qtk_durian_batchnormconv_new(int in_channels,int out_channels,
                                int kernel_size,int stride,int padding)
{
    qtk_durian_batchnormconv_t *bnc = NULL;
    bnc = wtk_malloc(sizeof(*bnc));
    
    bnc->conv1d = qtk_nn_conv1d_new(in_channels,out_channels,kernel_size,padding,0);
    bnc->batchnorm = qtk_nn_batchnorm_new(out_channels,1e-5);
    return bnc;
}

int qtk_durian_batchnormconv_loadfile(qtk_durian_batchnormconv_t *bnc,
                                        char *conv1_kenrel_fn,char *conv1d_bias_fn,
                                        char *batchnorm_gamma_fn,char *batchnorm_beta_fn,
                                        char *batchnorm_mean_fn,char *batchnorm_var_fn)
{
    qtk_nn_conv1d_load_file(bnc->conv1d,conv1_kenrel_fn,conv1d_bias_fn);
    qtk_nn_batchnorm_load_file(bnc->batchnorm,batchnorm_gamma_fn,batchnorm_beta_fn,
                                                       batchnorm_mean_fn, batchnorm_var_fn);
    return 0;
}

int qtk_durian_batchnormconv_delete(qtk_durian_batchnormconv_t *bnc)
{
    qtk_nn_conv1d_delete(bnc->conv1d);
    qtk_nn_batchnorm_delete(bnc->batchnorm);

    wtk_free(bnc);
    return 0;
}

int qtk_durian_batchnormconv_process(qtk_durian_batchnormconv_t *bnc, wtk_matf_t *in, 
                                                                                    wtk_matf_t *out)
{
    qtk_nn_conv1d_forward(bnc->conv1d,in,out);
    wtk_nn_relu(out->p,out->row*out->col);
    qtk_nn_batchnorm_forward_inplace(bnc->batchnorm,out);
    return 0;
}

//////////////////////////// highway
qtk_durian_highway_t *qtk_durian_highway_new(int in_size,int out_size)
{
    qtk_durian_highway_t *highway = NULL;
    highway = wtk_malloc(sizeof(*highway));
    highway->H = qtk_nn_fc_new(in_size,out_size,QTK_NN_ACTINATION_RULE,1,1);
    highway->T = qtk_nn_fc_new(in_size,out_size,QTK_NN_ACTINATION_SIGMOID,1,1);
    return highway;
}

int qtk_durian_highway_delete(qtk_durian_highway_t *highway)
{
    qtk_nn_fc_delete(highway->H);
    qtk_nn_fc_delete(highway->T);
    wtk_free(highway);
    return 0;   
}

int qtk_durian_highwav_loadfile(qtk_durian_highway_t *highway, char *H_wfn,char *H_bfn,char *T_wfn,char *T_bfn)
{
    qtk_nn_fc_load_file(highway->H,H_wfn,H_bfn);
    qtk_nn_fc_load_file(highway->T,T_wfn,T_bfn);
    return 0;
}

int qtk_durian_highway_process(qtk_durian_highway_t *highway,wtk_matf_t *in,wtk_matf_t *out)
{
    wtk_matf_t *H = wtk_matf_new(in->row,highway->H->kernel->row);  //use transe
    wtk_matf_t *T = wtk_matf_new(in->row,highway->T->kernel->row);
    qtk_nn_fc_forward(highway->H,in,H);
    qtk_nn_fc_forward(highway->T,in,T);
    
    //(H-in)T+in
    wtk_float_scale_add(in->p,-1.0f,H->p,H->row*H->col);
    wtk_float_mult(H->p,T->p,H->p,H->row*H->col);
    wtk_matf_add(out,H,in);
    wtk_matf_delete(H);
    wtk_matf_delete(T);
    return 0;
}
