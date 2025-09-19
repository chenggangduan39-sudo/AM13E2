#include "qtk_tts_tac2_syn_dec.h"

//********** decoder

qtk_tts_tac2_syn_dec_t* qtk_tts_tac2_syn_decoder_new(qtk_tts_tac2_syn_dec_cfg_t *cfg,wtk_heap_t *heap)
{
    int layer = 0,i = 0,*dim = NULL,use_gru = cfg->use_gru;
    wtk_source_loader_t sl;
    wtk_source_t source;
    qtk_tts_tac2_syn_dec_t *syn = NULL;
    
    syn = wtk_malloc(sizeof(qtk_tts_tac2_syn_dec_t));
    syn->cfg = cfg;
    syn->heap = heap;
    wtk_source_loader_init_file(&sl);
    layer = cfg->prenet_layer;
    syn->dec_prenet_bias = wtk_malloc(sizeof(wtk_vecf_t*)*layer);
    syn->dec_prenet_kernel = wtk_malloc(sizeof(wtk_matf_t*)*layer);
    dim = cfg->prenet_dim_num->slot;
    for(i = 0; i < layer; ++i){
        syn->dec_prenet_kernel[i] = wtk_matf_new(dim[0],dim[1]);
        syn->dec_prenet_bias[i] = wtk_vecf_new(dim[0]);
        wtk_mer_source_loader_load_matf(&sl,&source,cfg->prenet_kernel_fn[i],syn->dec_prenet_kernel[i]);
        wtk_mer_source_loader_load_vecf(&sl,&source,cfg->prenet_bias_fn[i],syn->dec_prenet_bias[i]);
        dim+=2;
    }
    dim = cfg->prenet_dim_num->slot;
    syn->dec_prenet_catch = wtk_matf_new(1,dim[0]);
    layer = cfg->rnn_layer;
    dim = cfg->rnn_dim_num->slot;
    if(use_gru){
        syn->dec_gru = wtk_malloc(sizeof(wtk_nn_rnngru_t*)*layer);
        for(i = 0;i < layer; ++i){
            syn->dec_gru[i] = wtk_nn_rnngru_new2(wtk_nn_enum_type_pytorch,1,dim[0],dim[1],NULL);
            wtk_mer_source_loader_load_matf(&sl,&source,cfg->gru_kernel_gate[i],syn->dec_gru[i]->gate_kernel);
            wtk_mer_source_loader_load_matf(&sl,&source,cfg->gru_kernel_cand[i],syn->dec_gru[i]->candidate_kernel);
            wtk_mer_source_loader_load_matf(&sl,&source,cfg->gru_kernel_cand_hh[i],syn->dec_gru[i]->candidate_kernel_hh);
            wtk_mer_source_loader_load_vecf(&sl,&source,cfg->gru_bias_gate[i],syn->dec_gru[i]->gate_bias);
            wtk_mer_source_loader_load_vecf(&sl,&source,cfg->gru_bias_cand[i],syn->dec_gru[i]->candidate_bias);
            wtk_mer_source_loader_load_vecf(&sl,&source,cfg->gru_bias_cand_hh[i],syn->dec_gru[i]->candidate_bias_hh);
            dim+=2;
        }
    }
    dim = cfg->linear_dim_num->slot;
    syn->dec_linear_kernel = wtk_matf_new(dim[0],dim[1]);
    syn->dec_linear_bias = wtk_vecf_new(dim[0]);
    dim = cfg->stop_dim_num->slot;
    syn->dec_stop_kernel = wtk_matf_new(dim[0],dim[1]);
    syn->dec_stop_bias = wtk_vecf_new(dim[0]);
    wtk_mer_source_loader_load_matf(&sl,&source,cfg->linear_kernel_fn,syn->dec_linear_kernel);
    wtk_mer_source_loader_load_vecf(&sl,&source,cfg->linear_bias_fn,syn->dec_linear_bias);
    wtk_mer_source_loader_load_matf(&sl,&source,cfg->stop_kernel_fn,syn->dec_stop_kernel);
    wtk_mer_source_loader_load_vecf(&sl,&source,cfg->stop_bias_fn,syn->dec_stop_bias);
    //atten
    syn->dec_atten_mf = wtk_malloc(sizeof(wtk_matf_t));
    syn->dec_atten_memory_kernel = wtk_matf_new(cfg->atten_dim,cfg->atten_channel);
    syn->dec_atten_memory_bias = wtk_vecf_new(cfg->atten_dim);
    syn->dec_atten_quern_kernel = wtk_matf_new(cfg->atten_dim,cfg->atten_query_nrow);
    syn->dec_atten_quern_bias = wtk_vecf_new(cfg->atten_dim);
    syn->dec_atten_conv_kernel = wtk_matf_new(cfg->atten_filter,cfg->atten_kernel);
    syn->dec_atten_conv_bias = wtk_vecf_new(cfg->atten_filter);
    syn->dec_atten_features_kernel = wtk_matf_new(cfg->atten_dim,cfg->atten_filter);
    syn->dec_atten_var_project = wtk_vecf_new(cfg->atten_dim);
    syn->dec_atten_var_project_bias = wtk_vecf_new(1);

    syn->dec_atten_mo = wtk_matf_new(1024,cfg->atten_dim);
    syn->dec_atten_qo = wtk_matf_new(1,cfg->atten_dim);
    syn->dec_atten_fo = wtk_matf_new(1024, cfg->atten_dim);
    syn->dec_atten_conv_out = wtk_matf_new(1024,cfg->atten_filter);
    syn->dec_prev_align = wtk_vecf_new(1024);
    syn->dec_new_align = wtk_vecf_new(1024);

    wtk_mer_source_loader_load_matf(&sl,&source,cfg->atten_memory_kernel_fn,syn->dec_atten_memory_kernel);
    wtk_mer_source_loader_load_vecf(&sl,&source,cfg->atten_memory_bias_fn,syn->dec_atten_memory_bias);
    wtk_mer_source_loader_load_matf(&sl,&source,cfg->atten_query_kernel_fn,syn->dec_atten_quern_kernel);
    wtk_mer_source_loader_load_vecf(&sl,&source,cfg->atten_query_bias_fn,syn->dec_atten_quern_bias);
    wtk_mer_source_loader_load_matf(&sl,&source,cfg->atten_conv_kernel_fn,syn->dec_atten_conv_kernel);
    wtk_mer_source_loader_load_vecf(&sl,&source,cfg->atten_conv_bias_fn,syn->dec_atten_conv_bias);
    wtk_mer_source_loader_load_matf(&sl,&source,cfg->atten_features_kernel_fn,syn->dec_atten_features_kernel);
    wtk_mer_source_loader_load_vecf(&sl,&source,cfg->atten_var_project_fn,syn->dec_atten_var_project);
    wtk_mer_source_loader_load_vecf(&sl,&source,cfg->atten_var_project_bias_fn,syn->dec_atten_var_project_bias);

    return syn;
}

int qtk_tts_tac2_syn_decoder_delete(qtk_tts_tac2_syn_dec_t *syn)
{
    int layer = syn->cfg->prenet_layer,use_gru = syn->cfg->use_gru;
    int i = 0;

    for(i = 0; i < layer; ++i) {
        wtk_matf_delete(syn->dec_prenet_kernel[i]);
        wtk_vecf_delete(syn->dec_prenet_bias[i]);
    }
    wtk_matf_delete(syn->dec_prenet_catch);
    wtk_free(syn->dec_prenet_kernel);
    wtk_free(syn->dec_prenet_bias);
    layer = syn->cfg->rnn_layer;
    if(use_gru){
        for(i = 0; i < layer; ++i){
            wtk_nn_rnngru_delete(syn->dec_gru[i]);
        }
        wtk_free(syn->dec_gru);
    }
    wtk_free(syn->dec_atten_mf);
    wtk_matf_delete(syn->dec_linear_kernel);
    wtk_vecf_delete(syn->dec_linear_bias);
    wtk_matf_delete(syn->dec_stop_kernel);
    wtk_vecf_delete(syn->dec_stop_bias);

    wtk_matf_delete(syn->dec_atten_memory_kernel);
    wtk_vecf_delete(syn->dec_atten_memory_bias);
    wtk_matf_delete(syn->dec_atten_quern_kernel);
    wtk_vecf_delete(syn->dec_atten_quern_bias);
    wtk_matf_delete(syn->dec_atten_conv_kernel);
    wtk_vecf_delete(syn->dec_atten_conv_bias);
    wtk_matf_delete(syn->dec_atten_features_kernel);
    wtk_vecf_delete(syn->dec_atten_var_project);
    wtk_vecf_delete(syn->dec_atten_var_project_bias);

    wtk_matf_delete(syn->dec_atten_mo);
    wtk_matf_delete(syn->dec_atten_qo);
    wtk_matf_delete(syn->dec_atten_fo);
    wtk_matf_delete(syn->dec_atten_conv_out);
    wtk_vecf_delete(syn->dec_prev_align);
    wtk_vecf_delete(syn->dec_new_align);
    return 0;
}

#define BERNOULLI(p) (random() < ((p)*RAND_MAX)?1.0f:0.0f)

int qtk_tts_tac2_syn_decoder_prenet(qtk_tts_tac2_syn_dec_t *syn,wtk_matf_t *input,wtk_matf_t *output)
{
    int i = 0,j = 0;
    wtk_vecf_t *bernoulli_random = wtk_vecf_new(output->col);
    int layer_len = syn->cfg->prenet_layer;
    wtk_matf_t *cache_mf = syn->dec_prenet_catch;
    wtk_matf_t **kernel = syn->dec_prenet_kernel;
    wtk_vecf_t **bias = syn->dec_prenet_bias;

    //这边选择的prenet模型是一样的  所以可以使用同一个output
    for (i=0; i<layer_len; ++i){
        // cache_mf->col = layer_size[i];
        wtk_nn_layer_dense(input, kernel[i], bias[i], wtk_nn_relu, output);
        for(j = 0; j < bernoulli_random->len;++j){
            bernoulli_random->p[j] = BERNOULLI(0.5f)*2;
            // bernoulli_random->p[j] = 1.0f;
        }
        wtk_matf_vecf_multi(output,bernoulli_random);
        wtk_matf_cpy(output, cache_mf);
        input = cache_mf;
    }
    wtk_vecf_delete(bernoulli_random);
    return 0;   
}

int qtk_tts_tac2_syn_decoder_atten_step1(qtk_tts_tac2_syn_dec_t *syn,wtk_matf_t *enc_out)
{
    int ret = -1;
    wtk_heap_t *heap = syn->heap;
    int enc_row = enc_out->row;

    if (enc_row > 1024) //大于1024个参数层
    { 
        wtk_debug("atten 设计不合理 enc_row 超出最大限制 %d > %d \n", enc_row, 1024);
        goto end;
    }
    syn->dec_enc_transpose = wtk_matf_heap_new(heap, enc_out->col, enc_out->row);
    syn->dec_atten_mo->row = enc_row;
    syn->dec_atten_fo->row = enc_row;
    syn->dec_atten_conv_out->row = enc_row;
    syn->dec_prev_align->len = enc_row;
    syn->dec_new_align->len = enc_row;

    wtk_matf_init_transpose(enc_out, syn->dec_enc_transpose);
    wtk_nn_layer_dense(enc_out, syn->dec_atten_memory_kernel, syn->dec_atten_memory_bias, NULL, syn->dec_atten_mo);
    wtk_vecf_zero(syn->dec_prev_align);
    wtk_vecf_zero(syn->dec_new_align);
    wtk_matf_zero(syn->dec_atten_mf);
    ret = 0;
end:
    return ret;
}

int qtk_tts_tac2_syn_decoder_atten_step2(qtk_tts_tac2_syn_dec_t *syn,wtk_matf_t *rnn_out)
{
    int ret = -1;
    wtk_vecf_t *prev_align = syn->dec_prev_align, *new_align = syn->dec_new_align,
                            *cumulated_align = syn->dec_prev_align;
    wtk_matf_t *w_query = NULL,*conv_out = NULL, *w_fil = NULL, *w_keys = NULL;
    int i, j, nrow = prev_align->len, dim = syn->cfg->atten_dim;
    double t;
    float *ap = NULL, *wq = NULL, *wf = NULL, *wk = NULL,
            *va = syn->dec_atten_var_project->p;

    w_query = syn->dec_atten_qo;
    conv_out = syn->dec_atten_conv_out;
    wq = w_query->p;
    w_fil = syn->dec_atten_fo;
    wf = w_fil->p;
    w_keys = syn->dec_atten_mo;
    wk = w_keys->p;
    ap = new_align->p;

    wtk_matf_t prev_mf,new_align_mf;
    prev_mf.p = prev_align->p;
    prev_mf.row = prev_align->len;
    prev_mf.col = 1;
    new_align_mf.p = new_align->p;
    new_align_mf.row = 1;
    new_align_mf.col = new_align->len;

    wtk_mer_blas_sgemm2(rnn_out, syn->dec_atten_quern_kernel,syn->dec_atten_quern_bias, w_query);    //ssy
    wtk_nn_conv1d2(syn->dec_atten_conv_kernel, syn->cfg->atten_kernel, enum_conv_same, &prev_mf, conv_out);
    wtk_matf_vecf_add(conv_out, syn->dec_atten_conv_bias);
    wtk_mer_blas_sgemm2(conv_out, syn->dec_atten_features_kernel, NULL, w_fil);

    for (i=0; i<nrow; ++i)
    {
        t=0;
        for (j=0; j<dim; ++j)
        {
             t += va[j] * wtk_nn_tanh_inline(wq[j] + wf[j] + wk[j]);
        }
        ap[i] = t+syn->dec_atten_var_project_bias->p[0]; //ssy
        wf+=dim;
        wk+=dim;
    }

    wtk_softmax(ap, new_align->len);
    wtk_vecf_add(cumulated_align, new_align->p);
    wtk_mer_blas_sgemm(&new_align_mf, syn->dec_enc_transpose, NULL, syn->dec_atten_mf);
    ret = 0;

    return ret;
}

int qtk_tts_tac2_syn_decoder_process(qtk_tts_tac2_syn_dec_t *syn,wtk_matf_t *in,int is_end)
{
    int ret = -1,layer = 0,i = 0;
    wtk_heap_t *heap = syn->heap;
    wtk_matf_t *prenet_out = NULL,*prenet_in = NULL,*lstm_in = NULL;
    int use_gru = syn->cfg->use_gru;
    // int atten_channel = syn->enc->enc_conv_kernel[0]->row,atten_channel_len = 0;
    int atten_channel = syn->cfg->atten_channel,atten_channel_len = 0;
    wtk_matf_t *atten_mf = NULL,*project_in = NULL,*lstm_out = NULL;
    wtk_matf_t *linear_kernel = NULL,*stop_kernel = NULL;
    wtk_matf_t *lstm_in2 = NULL;
    wtk_vecf_t *linear_bias = NULL,*stop_bias = NULL;
    int lstm_units = 0;
    int atten_per_step = syn->cfg->atten_per_step;
    wtk_matf_t *linear_out = NULL, *stop_out = NULL;
    float *dec_outp = NULL, *cellp = NULL;
    wtk_matf_t dec_out;

    layer = syn->cfg->rnn_layer;
    prenet_out = wtk_heap_malloc(heap,sizeof(wtk_matf_t));
    prenet_in = wtk_matf_heap_new(heap,1,syn->dec_prenet_kernel[0]->col);
    lstm_in = wtk_matf_heap_new(heap,1,syn->dec_prenet_kernel[layer-1]->col+atten_channel);
    lstm_out = wtk_heap_malloc(heap,sizeof(wtk_matf_t));
    atten_mf = syn->dec_atten_mf;
    wtk_matf_zero(prenet_in);
    
    if(use_gru){
        lstm_units = syn->dec_gru[0]->num_units;
    }

    project_in = wtk_matf_heap_new( heap, 1, lstm_units +atten_channel);
    wtk_matf_zero(project_in);

    lstm_in2 = wtk_matf_heap_new(heap,1,lstm_units+atten_channel);

    prenet_out->p = lstm_in->p;
    prenet_out->row = 1;
    prenet_out->col = syn->dec_prenet_kernel[layer-1]->col;
    
    atten_mf->p = lstm_in->p+syn->dec_prenet_kernel[layer-1]->col;
    atten_mf->row = 1;
    atten_mf->col = atten_channel;

    lstm_out->p = project_in->p;
    lstm_out->row = 1;
    lstm_out->col = lstm_units;

    atten_channel_len = atten_channel * sizeof(float);
    qtk_tts_tac2_syn_decoder_atten_step1(syn,in);

    layer = syn->cfg->rnn_layer;
    for(i = 0; i < layer; ++i){
        if(use_gru){
            wtk_nn_rnngru_reset(syn->dec_gru[i]);
        }
    }

    linear_kernel = syn->dec_linear_kernel;
    linear_bias = syn->dec_linear_bias;
    stop_kernel = syn->dec_stop_kernel;
    stop_bias = syn->dec_stop_bias;
    linear_out = wtk_matf_heap_new( heap, 1, linear_kernel->row);
    stop_out = wtk_matf_heap_new( heap, 1, atten_per_step);
    dec_outp = wtk_heap_malloc( heap, sizeof(float)*1000*linear_out->col);
    cellp = dec_outp;
    int linear_out_stlen = sizeof(float)*linear_out->col;
    float *linear_endp = linear_out->p + linear_kernel->row;
    int num_mels = syn->cfg->num_mels;
    int prenet_in_stlen = sizeof(float)*num_mels;
    int time_i = 0;
    printf("decoder start\n");
    do{
        qtk_tts_tac2_syn_decoder_prenet(syn,prenet_in,prenet_out);
        if(use_gru){
            wtk_nn_rnngru_cell(syn->dec_gru[0],lstm_in,lstm_out);
            memcpy(lstm_in2->p,lstm_out->p,sizeof(float)*lstm_units);
            memcpy(lstm_in2->p+lstm_units,atten_mf->p,atten_channel_len);
            wtk_nn_rnngru_cell(syn->dec_gru[1],lstm_in2,lstm_out);
        }
        qtk_tts_tac2_syn_decoder_atten_step2(syn,lstm_out);

        memcpy(project_in->p+lstm_units, atten_mf->p, atten_channel_len);
        wtk_nn_layer_dense(project_in, linear_kernel, linear_bias, NULL, linear_out);
        wtk_nn_layer_dense(project_in, stop_kernel, stop_bias, wtk_nn_sigmoid, stop_out);

        for (i=0; i<atten_per_step; ++i){
            if (stop_out->p[i] > 0.5){
                goto end;
            }
        }
        memcpy(cellp, linear_out->p, linear_out_stlen);
        memcpy(prenet_in->p, linear_endp-num_mels, prenet_in_stlen);
        time_i += 1;
        cellp += linear_out->col;
        if (time_i == 1000)
        {
            wtk_debug("decoder 解码超出限制 max_time: %d \n", 1000);
            goto end;
        }
    }while(1);

end:

    dec_out.p = dec_outp;
    dec_out.row = time_i;
    dec_out.col = linear_out->col;
    if(syn->notify){
        syn->notify(syn->use_data,&dec_out,is_end);
    }
    
    ret = 0;
    return ret;
}

int qtk_tts_tac2_syn_decoder_set_notify(qtk_tts_tac2_syn_dec_t *syn,void *user_data,qtk_tts_tac2_syn_dec_notify_f notify)
{
    syn->use_data = user_data;
    syn->notify = notify;
    return 0;
}
