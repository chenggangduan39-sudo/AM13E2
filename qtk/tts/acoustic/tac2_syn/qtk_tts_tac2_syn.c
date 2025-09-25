#include "qtk_tts_tac2_syn.h"
#include "tts-mer/wtk-extend/wtk_mer_source_file.h"
#include "tts-mer/wtk-extend/wtk_heap2.h"

void qtk_tts_tac2_syn_embedding(wtk_veci_t *vec,wtk_matf_t *embedding_in,wtk_matf_t *embedding_out);
int qtk_tts_tac2_syn_postnet_new(qtk_tts_tac2_syn_t *syn,qtk_tts_tac2_syn_cfg_t *cfg);
int qtk_tts_tac2_syn_postnet_delete(qtk_tts_tac2_syn_t *syn);
int qtk_tts_tac2_syn_postnet_process(qtk_tts_tac2_syn_t *syn,wtk_matf_t *decoder_out);
void qtk_tts_tac2_syn_on_notify(qtk_tts_tac2_syn_t *syn, wtk_matf_t *decode_out,int is_end);

qtk_tts_tac2_syn_t *qtk_tts_tac2_syn_new(qtk_tts_tac2_syn_cfg_t *cfg)
{
    qtk_tts_tac2_syn_t *syn = NULL;
    syn = wtk_calloc(1, sizeof(qtk_tts_tac2_syn_t));
    int *dim = NULL,ret = -1;
    wtk_source_loader_t sl;
    wtk_source_t source;

    syn->cfg = cfg;

    wtk_source_loader_init_file(&sl);
    syn->heap = wtk_heap_new(4096);
    if(cfg->embedding_dim->nslot < 2){
        goto end;
    }
    dim = cfg->embedding_dim->slot;
    syn->embedding = wtk_matf_new(dim[0],dim[1]);
    wtk_mer_source_loader_load_matf(&sl,&source,cfg->embedding_fn,syn->embedding);

    if(cfg->use_dfsmn == 0){
        syn->enc = qtk_tts_tac2_syn_encoder_new(&cfg->enc_cfg,syn->heap);
    }else{
        syn->dfsmn_enc = qtk_tts_dfsmn_new(&cfg->dfsmn_enc_cfg,syn->heap);
    }
    if(cfg->use_gmmdec == 0){
        syn->dec = qtk_tts_tac2_syn_decoder_new(&cfg->dec_cfg,syn->heap);
        qtk_tts_tac2_syn_decoder_set_notify(syn->dec,syn,(qtk_tts_tac2_syn_dec_notify_f)qtk_tts_tac2_syn_on_notify);
    }else{
        syn->gmm_dec = qtk_tts_tac2_syn_gmmdec_new(&cfg->gmmdec_cfg,syn->heap);
        qtk_tts_tac2_syn_gmmdec_set_notify(syn->gmm_dec,syn,(qtk_tts_tac2_syn_dec_notify_f)qtk_tts_tac2_syn_on_notify);
    }
    if(cfg->use_postnet){
        ret = qtk_tts_tac2_syn_postnet_new(syn,cfg);
    }
    ret = 0;
end:
    if(ret < 0){
        qtk_tts_tac2_syn_delete(syn);
        syn = NULL;
    }
    return syn;
}

void qtk_tts_tac2_syn_set_notify(qtk_tts_tac2_syn_t *syn,void *user_data,qtk_tts_tac2_syn_notify_f cb)
{
    syn->user_data = user_data;
    syn->notify = cb;
}

int qtk_tts_tac2_syn_process(qtk_tts_tac2_syn_t *syn,wtk_veci_t *vec,int is_end)
{
    int ret = -1;
    wtk_heap_t *heap = syn->heap;
    wtk_matf_t *embedding_in = NULL,*embedding_out = NULL,*encode_out = NULL;
    
    gettimeofday(&syn->time_s,NULL);
    wtk_heap_reset(syn->heap);

    srand(999);
    embedding_in = syn->embedding;
    embedding_out = wtk_matf_heap_new( heap, vec->len, embedding_in->col);
    qtk_tts_tac2_syn_embedding(vec,embedding_in,embedding_out);
    if(syn->cfg->use_dfsmn == 0){
        encode_out = wtk_matf_heap_new(heap,embedding_out->row,syn->enc->enc_lstm_fw->lstm_units*2);
        qtk_tts_tac2_syn_encode_process(syn->enc,embedding_out,encode_out);
    }else{
        encode_out = wtk_matf_heap_new(heap,embedding_out->row,syn->dfsmn_enc->rnn_gru[0]->num_units*2);
        qtk_tts_dfsmn_process(syn->dfsmn_enc,embedding_out,encode_out);
    }
    if(syn->cfg->use_gmmdec == 0){
        qtk_tts_tac2_syn_decoder_process(syn->dec,encode_out,is_end);
    }else{
        qtk_tts_tac2_syn_gmmdec_process(syn->gmm_dec,encode_out,is_end);
    }
    ret = 0;
    return ret;
}

int qtk_tts_tac2_syn_delete(qtk_tts_tac2_syn_t *syn)
{
    if(syn == NULL)
        goto end;
    if(syn->embedding)
        wtk_matf_delete(syn->embedding);
    if(syn->cfg->use_dfsmn){
        qtk_tts_dfsmn_delete(syn->dfsmn_enc);
    }else{
        qtk_tts_tac2_syn_encode_delete(syn->enc);
    }
    if(syn->cfg->use_gmmdec == 0){
        qtk_tts_tac2_syn_decoder_delete(syn->dec);
    }else{
        qtk_tts_tac2_syn_gmmdec_delete(syn->gmm_dec);
    }
    if(syn->cfg->use_postnet){
        qtk_tts_tac2_syn_postnet_delete(syn);
    }
    if(syn->heap){
        wtk_heap_delete(syn->heap);
    }
    wtk_free(syn);
end:
    return 0;
}

//*****************embedding
void qtk_tts_tac2_syn_embedding(wtk_veci_t *vec,wtk_matf_t *embedding_in,wtk_matf_t *embedding_out)
{
    float *mfp = NULL,*emp = NULL;
    int i = 0,input_len = vec->len,embedding_dim = embedding_out->col;
    mfp = embedding_out->p;
    emp = embedding_in->p;
    for (i=0; i < input_len; ++i, mfp+=embedding_dim)
    {
        memcpy(mfp, emp + vec->p[i]*embedding_dim, sizeof(float)*embedding_dim);  //转成了一个(i,embedding_dim)
    }
    return;
}

//***************** postnet
int qtk_tts_tac2_syn_postnet_new(qtk_tts_tac2_syn_t *syn,qtk_tts_tac2_syn_cfg_t *cfg)
{
    int ret = 0;
    int layer = cfg->postnet.num_layer,i=0;
    int *dim = NULL;
    wtk_source_loader_t sl;
    wtk_source_t source;
    
    wtk_source_loader_init_file(&sl);
    syn->postnet_conv_kernel = wtk_malloc(sizeof(wtk_matf_t*)*layer);
    syn->postnet_conv_bias = wtk_malloc(sizeof(wtk_vecf_t*)*layer);
    syn->postnet_gamma = wtk_malloc(sizeof(wtk_vecf_t*)*layer);
    syn->postnet_beta = wtk_malloc(sizeof(wtk_vecf_t*)*layer);
    syn->postnet_mean = wtk_malloc(sizeof(wtk_vecf_t*)*layer);
    syn->postnet_var = wtk_malloc(sizeof(wtk_vecf_t*)*layer);
    dim = cfg->postnet.conv_dim_num->slot;
    for(i = 0; i < layer; ++i){
        syn->postnet_conv_kernel[i] = wtk_matf_new(dim[0],dim[1]);
        syn->postnet_conv_bias[i] = wtk_vecf_new(dim[0]);
        syn->postnet_gamma[i] = wtk_vecf_new(dim[0]);
        syn->postnet_beta[i] = wtk_vecf_new(dim[0]);
        syn->postnet_mean[i] = wtk_vecf_new(dim[0]);
        syn->postnet_var[i] = wtk_vecf_new(dim[0]);
        wtk_mer_source_loader_load_matf(&sl,&source,cfg->postnet.conv_kernel_fn[i],syn->postnet_conv_kernel[i]);
        wtk_mer_source_loader_load_vecf(&sl,&source,cfg->postnet.conv_bias_fn[i],syn->postnet_conv_bias[i]);
        wtk_mer_source_loader_load_vecf(&sl,&source,cfg->postnet.batch_norm_gamma_fn[i],syn->postnet_gamma[i]);
        wtk_mer_source_loader_load_vecf(&sl,&source,cfg->postnet.batch_norm_beta_fn[i],syn->postnet_beta[i]);
        wtk_mer_source_loader_load_vecf(&sl,&source,cfg->postnet.batch_norm_mean_fn[i],syn->postnet_mean[i]);
        wtk_mer_source_loader_load_vecf(&sl,&source,cfg->postnet.batch_norm_var_fn[i],syn->postnet_var[i]);
        dim+=2;
    }
    
    return ret;
}

int qtk_tts_tac2_syn_postnet_delete(qtk_tts_tac2_syn_t *syn)
{
    int ret = 0;
    int i = 0,layer = syn->cfg->postnet.num_layer;
    
    for(i = 0; i < layer; ++i){
        wtk_matf_delete(syn->postnet_conv_kernel[i]);
        wtk_vecf_delete(syn->postnet_conv_bias[i]);
        wtk_vecf_delete(syn->postnet_gamma[i]);
        wtk_vecf_delete(syn->postnet_beta[i]);
        wtk_vecf_delete(syn->postnet_mean[i]);
        wtk_vecf_delete(syn->postnet_var[i]);
    }
    wtk_free(syn->postnet_conv_kernel);
    wtk_free(syn->postnet_conv_bias);
    wtk_free(syn->postnet_gamma);
    wtk_free(syn->postnet_beta);
    wtk_free(syn->postnet_mean);
    wtk_free(syn->postnet_var);
    return ret;
}

/*
wtk_nn_conv1d()
if (is_after) activation()
wtk_nn_batch_norm( gamma, beta, moving_mean, moving_variance)
if (is_before) activation()
if (is_training) drop_out()
 */
void qtk_tts_tac2_syn_postnet_conv1d(wtk_matf_t *kernel, int kernel_size, wtk_vecf_t *bias, 
                                                                                    wtk_vecf_t *gamma, wtk_vecf_t *beta, wtk_vecf_t *moving_mean, wtk_vecf_t *moving_variance, 
                                                                                    float epsilon, void activation(float*, int), wtk_matf_t *in, wtk_matf_t *dst)
{
    int dst_len = dst->row*dst->col;
    wtk_nn_conv1d2( kernel, kernel_size, enum_conv_same, in, dst);

    if (bias){
        wtk_matf_vecf_add( dst, bias);
    }
    wtk_nn_batch_norm(dst, gamma, beta, moving_mean, moving_variance, epsilon);
    if (activation){
        activation(dst->p, dst_len);
     }
}

int qtk_tts_tac2_syn_postnet_process(qtk_tts_tac2_syn_t *syn,wtk_matf_t *decoder_out)
{
    int ret = -1,i = 0;
    wtk_heap_t *heap = syn->heap;
    int irow = decoder_out->row, layer = syn->cfg->postnet.num_layer;
    int *kernel_size = syn->cfg->postnet.conv_kernel_size->slot, channel = syn->postnet_conv_kernel[0]->row;
    wtk_matf_t *residual = NULL, *cache_mf = NULL,*input = NULL;
    wtk_matf_t **conv_kernel = syn->postnet_conv_kernel;
    wtk_vecf_t **conv_bias = syn->postnet_conv_bias;
    wtk_vecf_t **gamma = syn->postnet_gamma;
    wtk_vecf_t **beta = syn->postnet_beta;
    wtk_vecf_t **mean = syn->postnet_mean;
    wtk_vecf_t **variance = syn->postnet_var;
    wtk_matf_t *dst = wtk_matf_heap_new( heap, decoder_out->row, syn->cfg->postnet.num_mels);
    wtk_matf_t *mel_out = NULL;

    residual = wtk_matf_heap_new( heap, irow, channel),
    cache_mf = wtk_matf_heap_new( heap, irow, channel);

    input = decoder_out;
    for (i=0; i<layer-1; ++i)
    {
        wtk_debug("postnet %d\n",i);
        qtk_tts_tac2_syn_postnet_conv1d(conv_kernel[i], kernel_size[i], conv_bias[i], 
                                                                                gamma[i], beta[i], mean[i], variance[i], 1E-5, wtk_nn_tanh, input, residual);
        wtk_matf_cpy(residual, cache_mf);
        input=cache_mf;
    }
    wtk_debug("postnet %d\n",i);
    qtk_tts_tac2_syn_postnet_conv1d(conv_kernel[i], kernel_size[i], conv_bias[i], 
                                                                            gamma[i], beta[i], mean[i], variance[i], 1E-5, NULL, input, dst);
    mel_out = wtk_matf_heap_new(heap, decoder_out->row, decoder_out->col);
    wtk_matf_add(mel_out, decoder_out, dst);
    // wtk_matf_print(mel_out);
    if(syn->notify){
         struct timeval time_e;
         gettimeofday(&time_e,NULL);
         float t = (time_e.tv_sec*1000+time_e.tv_usec/1000)-(syn->time_s.tv_sec*1000+syn->time_s.tv_usec/1000);
         printf("tac2 use time %fs\n",t/1000);
        syn->notify(syn->user_data,dst,1);
    }

    ret = 0;
    return ret;
}

//dec notify
void qtk_tts_tac2_syn_on_notify(qtk_tts_tac2_syn_t *syn, wtk_matf_t *decode_out,int is_end)
{
    if(syn->cfg->use_postnet){
        qtk_tts_tac2_syn_postnet_process(syn,decode_out);
    }else if(syn->notify){
         //return mel
         struct timeval time_e;
         gettimeofday(&time_e,NULL);
         float t = (time_e.tv_sec*1000+time_e.tv_usec/1000)-(syn->time_s.tv_sec*1000+syn->time_s.tv_usec/1000);
         printf("tac2 use time %fs\n",t/1000);
         syn->notify(syn->user_data,decode_out,1);
    }else{
        wtk_debug("mel dont out\n");
    }
    return;
} 
