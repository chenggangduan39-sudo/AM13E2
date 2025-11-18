#include "qtk_tts_tac2_syn_enc.h"

#define QTK_EPSILON (1e-5f)

//******************encoder 

qtk_tts_tac2_syn_enc_t* qtk_tts_tac2_syn_encoder_new(qtk_tts_tac2_syn_enc_cfg_t *cfg,wtk_heap_t *heap)
{
    qtk_tts_tac2_syn_enc_t *enc = NULL;
    int num_layer = cfg->conv_layer;
    int *dim = NULL,i = 0;

    enc = wtk_malloc(sizeof(*enc));
    enc->enc_cfg = cfg;
    enc->heap = heap;
    wtk_source_loader_t sl;
    wtk_source_t source;
    
    wtk_source_loader_init_file(&sl);
    enc->enc_conv_kernel = wtk_malloc(sizeof(wtk_matf_t*)*num_layer);
    enc->enc_conv_bias = wtk_malloc(sizeof(wtk_vecf_t*)*num_layer);
    enc->enc_gamma = wtk_malloc(sizeof(wtk_vecf_t*)*num_layer);
    enc->enc_beta = wtk_malloc(sizeof(wtk_vecf_t*)*num_layer);
    enc->enc_mean = wtk_malloc(sizeof(wtk_vecf_t*)*num_layer);
    enc->enc_variance = wtk_malloc(sizeof(wtk_vecf_t*)*num_layer);
    dim = cfg->conv_dim_num->slot;
    for(i = 0; i < num_layer; ++i){
        enc->enc_conv_kernel[i] = wtk_matf_new(dim[0],dim[1]);
        enc->enc_conv_bias[i] = wtk_vecf_new(dim[0]);
        wtk_mer_source_loader_load_matf(&sl,&source,cfg->conv_kernel_fn[i],enc->enc_conv_kernel[i]);
        wtk_mer_source_loader_load_vecf(&sl,&source,cfg->conv_bias_fn[i],enc->enc_conv_bias[i]);
        enc->enc_gamma[i] = wtk_vecf_new(dim[0]);
        enc->enc_beta[i] = wtk_vecf_new(dim[0]);
        enc->enc_mean[i] = wtk_vecf_new(dim[0]);
        enc->enc_variance[i] = wtk_vecf_new(dim[0]);
        wtk_mer_source_loader_load_vecf(&sl,&source,cfg->gamma_fn[i],enc->enc_gamma[i]);
        wtk_mer_source_loader_load_vecf(&sl,&source,cfg->beta_fn[i],enc->enc_beta[i]);
        wtk_mer_source_loader_load_vecf(&sl,&source,cfg->moving_mean_fn[i],enc->enc_mean[i]);
        wtk_mer_source_loader_load_vecf(&sl,&source,cfg->moving_variance_fn[i],enc->enc_variance[i]);
        dim+=2;
    }

    dim = cfg->lstm_dim_num->slot;
    enc->enc_lstm_fw = wtk_nn_lstm_new(1,dim[0],dim[1],0.0f,tanhf,NULL);
    enc->enc_lstm_bw = wtk_nn_lstm_new(0,dim[0],dim[1],0.0f,tanhf,NULL);
    wtk_mer_source_loader_load_matf(&sl,&source,cfg->lstm_fw_kernel_fn,enc->enc_lstm_fw->kernel);
    wtk_mer_source_loader_load_vecf(&sl,&source,cfg->lstm_fw_bias_fn,enc->enc_lstm_fw->bias);
    wtk_mer_source_loader_load_matf(&sl,&source,cfg->lstm_bw_kernel_fn,enc->enc_lstm_bw->kernel);
    wtk_mer_source_loader_load_vecf(&sl,&source,cfg->lstm_bw_bias_fn,enc->enc_lstm_bw->bias);

    return enc;
}

int qtk_tts_tac2_syn_encode_process_1(qtk_tts_tac2_syn_enc_t *syn,wtk_matf_t *input,wtk_matf_t *output)
{
    int layer = syn->enc_cfg->conv_layer;
    int ret = -1,i = 0,mem_len = 0;
    wtk_heap_t *heap = syn->heap;
    int *kernel_size = syn->enc_cfg->conv_kernel_sizes->slot;
    wtk_matf_t *conv_out = NULL;
    //这边的kernel size 和 channel 一样 只申请一块内存
    conv_out = wtk_matf_heap_new(heap,input->row,syn->enc_conv_kernel[0]->row);
    mem_len = input->row*syn->enc_conv_kernel[0]->row;
    for(i = 0; i < layer; ++i){
        printf("encoder conv %d\n",i);
        wtk_nn_conv1d2(syn->enc_conv_kernel[i],kernel_size[i],enum_conv_same,input,conv_out);
        wtk_matf_vecf_add(conv_out,syn->enc_conv_bias[i]);
        wtk_nn_batch_norm(conv_out,syn->enc_gamma[i],syn->enc_beta[i],syn->enc_mean[i],syn->enc_variance[i],QTK_EPSILON);
        wtk_nn_relu(conv_out->p,mem_len);
        memcpy(input->p,conv_out->p,sizeof(float)*mem_len);
    }
    memcpy(output->p,conv_out->p,sizeof(float)*mem_len);
    ret = 0;
    return ret;
}

int qtk_tts_tac2_syn_encode_process_2(qtk_tts_tac2_syn_enc_t *syn,wtk_matf_t *input,wtk_matf_t *output)
{
    int ret = -1;
    wtk_nn_lstm_t *fw_cell = syn->enc_lstm_fw;
    wtk_nn_lstm_t *bw_cell = syn->enc_lstm_bw;
    int nrow = input->row, lstm_units = fw_cell->lstm_units,i;
    float *op, *fwp, *bwp;
    size_t lstm_stlen = sizeof(float)*lstm_units;
    wtk_heap_t *heap = syn->heap;
    wtk_matf_t *fw_out = NULL, *bw_out = NULL;
    
    fw_out = wtk_matf_heap_new( heap, nrow, lstm_units),
    bw_out = wtk_matf_heap_new( heap, nrow, lstm_units);
    
    wtk_nn_lstm_reset(fw_cell);
    wtk_nn_lstm_reset(bw_cell);
    wtk_nn_lstm(fw_cell, input, fw_out);
    wtk_nn_lstm(bw_cell, input, bw_out);

    op = output->p;
    fwp = fw_out->p;
    bwp = bw_out->p;
    for (i=0; i<nrow; ++i)
    {
        memcpy(op, fwp, lstm_stlen);
        memcpy(op+lstm_units, bwp, lstm_stlen);
        op+=output->col;
        fwp+=lstm_units;
        bwp+=lstm_units;
    }
    ret = 0;
    return ret;
}

int qtk_tts_tac2_syn_encode_process(qtk_tts_tac2_syn_enc_t *syn,wtk_matf_t *in,wtk_matf_t *out)
{
    int ret = -1;
    wtk_matf_t *conv_out = NULL;
    wtk_heap_t *heap = syn->heap;

    conv_out = wtk_matf_heap_new(heap,in->row,syn->enc_conv_kernel[0]->row);
    qtk_tts_tac2_syn_encode_process_1(syn,in,conv_out);
    // lstm_out = wtk_matf_heap_new(heap,conv_out->row,syn->enc_lstm_fw->kernel->col*2); 
    qtk_tts_tac2_syn_encode_process_2(syn,conv_out,out);
    return ret;
}

void qtk_tts_tac2_syn_encode_delete(qtk_tts_tac2_syn_enc_t *syn)
{
    int num_layer = syn->enc_cfg->conv_layer;
    int i = 0;

    for(i = 0; i < num_layer; ++i){
        wtk_matf_delete(syn->enc_conv_kernel[i]);
        wtk_vecf_delete(syn->enc_conv_bias[i]);
        wtk_vecf_delete(syn->enc_gamma[i]);
        wtk_vecf_delete(syn->enc_beta[i]);
        wtk_vecf_delete(syn->enc_mean[i]);
        wtk_vecf_delete(syn->enc_variance[i]);
    }
    wtk_nn_lstm_delete(syn->enc_lstm_bw);
    wtk_nn_lstm_delete(syn->enc_lstm_fw);
    wtk_free(syn->enc_conv_kernel);
    wtk_free(syn->enc_conv_bias);
    wtk_free(syn->enc_gamma);
    wtk_free(syn->enc_beta);
    wtk_free(syn->enc_mean);
    wtk_free(syn->enc_variance);
    wtk_free(syn);
    return;
}
