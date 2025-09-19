#include "qtk_durian_decoder.h"
#include "tts-mer/wtk-extend/wtk_heap2.h"
#include "tts-mer/wtk-extend/wtk_mat2.h"

wtk_matf_t* qtk_durian_decoder_prenet(qtk_durian_decoder_t *dec, wtk_matf_t *decoder_input);

qtk_durian_decoder_t *qtk_durian_decoder_new(qtk_durian_decoder_cfg_t *cfg,wtk_heap_t *heap)
{
    qtk_durian_decoder_t *dec = NULL;
    int num = 0,i = 0,*dim = NULL;

    dec = wtk_malloc(sizeof(qtk_durian_decoder_t));
    dec->cfg = cfg;
    dec->heap = heap;
    dec->prenet_layer = wtk_malloc(sizeof(qtk_nn_fc_t*)*2);
    num = cfg->decoder_rnn_dim;
    dec->prenet_layer[0] = qtk_nn_fc_new(num,cfg->prenet_layers_dim,QTK_NN_ACTINATION_RULE,0,1);
    dec->prenet_layer[1] = qtk_nn_fc_new(cfg->prenet_layers_dim,cfg->prenet_layers_dim,QTK_NN_ACTINATION_RULE,0,1);
    qtk_nn_fc_load_file(dec->prenet_layer[0],cfg->prenet_layer_weight_fn[0],NULL);
    qtk_nn_fc_load_file(dec->prenet_layer[1],cfg->prenet_layer_weight_fn[1],NULL);
    
    dec->rnn_lstm = wtk_malloc(sizeof(qtk_nn_lstm_t*)*2);
    dim = cfg->lstm_dim_num->slot;
    for(i = 0; i < 2;++i){
        dec->rnn_lstm[i] = qtk_nn_lstm_new(dim[0],dim[1]);
        qtk_nn_lstm_loadfile(dec->rnn_lstm[i],cfg->lstm_weight_fn[i],cfg->lstm_bias_fn[i]);
        dim += 2;
    }
    dec->conv1d = qtk_nn_conv1d_new(cfg->decoder_rnn_dim,cfg->num_mels,1,0,1);
    qtk_nn_conv1d_load_file(dec->conv1d,cfg->conv1d_weight_fn,cfg->conv1d_bias_fn);
    // dec->r = qtk_random_new(1024);
    return dec;
}

int qtk_durian_decoder_delete(qtk_durian_decoder_t *dec)
{
    int i = 0;
    // qtk_random_delete(dec->r);
    qtk_nn_fc_delete(dec->prenet_layer[1]);
    qtk_nn_fc_delete(dec->prenet_layer[0]);
    for(i = 0; i < 2;++i){
        qtk_nn_lstm_delete(dec->rnn_lstm[i]);
    }
    wtk_free(dec->rnn_lstm);
    wtk_free(dec->prenet_layer);
    qtk_nn_conv1d_delete(dec->conv1d);
    wtk_free(dec);
    return 0;
}


wtk_matf_t* qtk_durian_decoder_process(qtk_durian_decoder_t *dec,wtk_matf_t *aligned_features)
{
    wtk_heap_t *heap = dec->heap;
    int alf_row = aligned_features->row;
    int alf_col = aligned_features->col;
    wtk_matf_t *prenet_input = wtk_matf_heap_new(heap,1,dec->cfg->decoder_output_size);
    wtk_matf_t *prenet_out = NULL;
    wtk_matf_t *decoder_in = wtk_matf_heap_new(heap,1,aligned_features->col+dec->prenet_layer[1]->kernel->col);
    float *alfp = aligned_features->p;
    int i = 0;
    wtk_matf_t *lstm_out1 = NULL,lstm_out2;
    wtk_matf_t *out = NULL;

    lstm_out1 = wtk_matf_heap_new(heap,1,dec->rnn_lstm[0]->lstm_units);
    // lstm_out2 = wtk_matf_heap_new(heap,1,dec->rnn_lstm[1]->lstm_units);
    out = wtk_matf_heap_new(heap,alf_row,dec->rnn_lstm[1]->lstm_units);
    for(i = 0; i < 2; ++i){
        qtk_nn_lstm_reset(dec->rnn_lstm[i]);
    }
    lstm_out2.row = 1;
    lstm_out2.col = dec->rnn_lstm[1]->lstm_units;
    srand(1024);
    for(i = 0; i < alf_row; ++i){
        prenet_out = qtk_durian_decoder_prenet(dec,prenet_input);
        memcpy(decoder_in->p,prenet_out->p,sizeof(float)*prenet_out->col);
        memcpy(decoder_in->p+prenet_out->col,alfp+alf_col*i,sizeof(float)*alf_col);
        qtk_nn_lstm_cell_forward(dec->rnn_lstm[0],decoder_in,lstm_out1);
        lstm_out2.p = out->p+i*out->col;
        qtk_nn_lstm_cell_forward(dec->rnn_lstm[1],lstm_out1,&lstm_out2);
        memcpy(prenet_input->p,lstm_out2.p,sizeof(float)*lstm_out2.col);

    }
    wtk_matf_reshape(out,-1,dec->cfg->decoder_rnn_dim);
    wtk_matf_t *out_mel = wtk_matf_heap_new(heap,out->row,dec->conv1d->out_dim);
    qtk_nn_conv1d_forward(dec->conv1d,out,out_mel);

    return out_mel;
}

// decoder process
#define BERNOULLI(p) (random() < ((p)*RAND_MAX)?1.0f:0.0f)
// #define DUPOUT(r,p) (qtk_random_rand(r) < ((p)*RAND_MAX)?1.0f:0.0f)

wtk_matf_t* qtk_durian_decoder_prenet(qtk_durian_decoder_t *dec, wtk_matf_t *decoder_input)
{
    wtk_heap_t *heap = dec->heap;
    qtk_nn_fc_t **layer = dec->prenet_layer;
    wtk_matf_t *layer_out1 = wtk_matf_heap_new(heap,decoder_input->row,layer[0]->kernel->row);
    wtk_matf_t *layer_out2 = wtk_matf_heap_new(heap,decoder_input->row,layer[1]->kernel->row);
    int i = 0,n = 0;

    qtk_nn_fc_forward(layer[0],decoder_input,layer_out1);
    n = layer_out1->row*layer_out1->col;
    for(i = 0; i < n; ++i){
        layer_out1->p[i] = BERNOULLI(0.5f) * layer_out1->p[i];
        // layer_out1->p[i] = DUPOUT(dec->r,0.5f) * layer_out1->p[i];
        // layer_out1->p[i] = 1.0f * layer_out1->p[i];
    }
    qtk_nn_fc_forward(layer[1],layer_out1,layer_out2);
    n = layer_out2->row * layer_out2->col;
    for(i = 0; i < n; ++i){
        layer_out2->p[i] = BERNOULLI(0.5f) * layer_out2->p[i];
        // layer_out2->p[i] = DUPOUT(dec->r,0.5f) * layer_out2->p[i];
        // layer_out2->p[i] = 1.0f * layer_out2->p[i];
    }

    return layer_out2;
}
