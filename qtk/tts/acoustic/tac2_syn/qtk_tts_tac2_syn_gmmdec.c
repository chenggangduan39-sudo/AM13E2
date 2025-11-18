#include "qtk_tts_tac2_syn_gmmdec.h"

int qtk_tts_tac2_syn_gmmdec_prenet_process(qtk_tts_tac2_syn_gmmdec_t *syn,wtk_matf_t *input,wtk_matf_t *output);
int qtk_tts_tac2_syn_gmmdec_atten_process(qtk_tts_tac2_syn_gmmdec_t *dec,wtk_matf_t *query);
wtk_matf_t* qtk_tts_tac2_syn_gmmdec_atten_getJ(wtk_heap_t *heap,int row);
void qtk_tts_tac2_syn_gmmdec_proj_view(wtk_matf_t *proj_o,wtk_matf_t *y);

qtk_tts_tac2_syn_gmmdec_t* qtk_tts_tac2_syn_gmmdec_new(qtk_tts_tac2_syn_gmmdec_cfg_t *cfg,wtk_heap_t *heap)
{
    qtk_tts_tac2_syn_gmmdec_t *gmm = NULL;
    int *dim = NULL,layer = 0;
    int i = 0;
    wtk_source_loader_t sl;
    wtk_source_t source;
    
    wtk_source_loader_init_file(&sl);
    gmm = wtk_malloc(sizeof(qtk_tts_tac2_syn_gmmdec_t));
    gmm->cfg = cfg;
    gmm->heap = heap;

    layer = cfg->prenet_layer;
    dim = cfg->prenet_dim_num->slot;
    gmm->prenet_weight = wtk_malloc(sizeof(wtk_matf_t*)*layer);
    gmm->prenet_bias = wtk_malloc(sizeof(wtk_vecf_t*)*layer);
    for(i  =0; i < layer; ++i){
        gmm->prenet_weight[i] = wtk_matf_new(dim[0],dim[1]);
        gmm->prenet_bias[i] = wtk_vecf_new(dim[0]);
        wtk_mer_source_loader_load_matf(&sl,&source,cfg->prenet_kernel_fn[i],gmm->prenet_weight[i]);
        wtk_mer_source_loader_load_vecf(&sl,&source,cfg->prenet_bias_fn[i],gmm->prenet_bias[i]);
        dim += 2;
    }
    dim = cfg->atten_rnn_dim_num->slot;
    gmm->atten_rnn = wtk_nn_lstm_new(1,dim[0],dim[1],0.0f,tanhf,NULL);
    wtk_mer_source_loader_load_matf(&sl,&source,cfg->atten_rnn_weight_fn,gmm->atten_rnn->kernel);
    wtk_mer_source_loader_load_vecf(&sl,&source,cfg->atten_rnn_bias_fn,gmm->atten_rnn->bias);
    dim = cfg->atten_layer_dim_num->slot;
    gmm->atten_query_weight = wtk_matf_new(dim[0],dim[1]);
    gmm->atten_query_bias = wtk_vecf_new(dim[0]);
    wtk_mer_source_loader_load_matf(&sl,&source,cfg->atten_query_weight_fn,gmm->atten_query_weight);
    wtk_mer_source_loader_load_vecf(&sl,&source,cfg->atten_query_bias_fn,gmm->atten_query_bias);
    gmm->atten_v_weight = wtk_matf_new(dim[2],dim[3]);
    wtk_mer_source_loader_load_matf(&sl,&source,cfg->atten_v_weight_fn,gmm->atten_v_weight);
    dim = cfg->linear_dim_num->slot;
    gmm->linear_weight = wtk_matf_new(dim[0],dim[1]);
    gmm->linear_bias = wtk_vecf_new(dim[0]);
    wtk_mer_source_loader_load_matf(&sl,&source,cfg->linear_weight_fn,gmm->linear_weight);
    wtk_mer_source_loader_load_vecf(&sl,&source,cfg->linear_bias_fn,gmm->linear_bias);
    gmm->lstm_rnn = wtk_malloc(sizeof(wtk_nn_lstm_t*)*cfg->rnn_n);
    dim = cfg->rnn_dim_num->slot;
    for(i = 0; i < cfg->rnn_n; ++i){
        gmm->lstm_rnn[i] = wtk_nn_lstm_new(1,dim[0],dim[1],0.0f,tanhf,NULL);
        wtk_mer_source_loader_load_matf(&sl,&source,cfg->rnn_weight_fn[i],gmm->lstm_rnn[i]->kernel);
        wtk_mer_source_loader_load_vecf(&sl,&source,cfg->rnn_bias_fn[i],gmm->lstm_rnn[i]->bias);
        dim += 2;
    }
    dim = cfg->proj_dim_num->slot;
    gmm->proj_weight = wtk_matf_new(dim[0],dim[1]);
    wtk_mer_source_loader_load_matf(&sl,&source,cfg->proj_weight_fn,gmm->proj_weight);
    return gmm;
}

int qtk_tts_tac2_syn_gmmdec_delete(qtk_tts_tac2_syn_gmmdec_t *gmm_dec)
{
    int i = 0,layer = gmm_dec->cfg->prenet_layer;
    for(i = 0; i < layer;++i){
        wtk_matf_delete(gmm_dec->prenet_weight[i]);
        wtk_vecf_delete(gmm_dec->prenet_bias[i]);
    }
    wtk_free(gmm_dec->prenet_weight);
    wtk_free(gmm_dec->prenet_bias);
    wtk_nn_lstm_delete(gmm_dec->atten_rnn);
    wtk_matf_delete(gmm_dec->atten_query_weight);
    wtk_vecf_delete(gmm_dec->atten_query_bias);
    wtk_matf_delete(gmm_dec->atten_v_weight);
    wtk_matf_delete(gmm_dec->linear_weight);
    wtk_vecf_delete(gmm_dec->linear_bias);
    for(i = 0; i < gmm_dec->cfg->rnn_n;++i){
        wtk_nn_lstm_delete(gmm_dec->lstm_rnn[i]);
    }
    wtk_free(gmm_dec->lstm_rnn);
    wtk_matf_delete(gmm_dec->proj_weight);
    wtk_free(gmm_dec);
    return 0;
}

int qtk_tts_tac2_syn_gmmdec_process(qtk_tts_tac2_syn_gmmdec_t *gmm_dec,wtk_matf_t *encoder_out,int is_end)
{
    wtk_heap_t *heap = gmm_dec->heap;
    int num_mels = gmm_dec->cfg->num_mels;
    int rnn_size = gmm_dec->cfg->rnn_size;
    int token_length = encoder_out->row;
    float threshold = token_length;//+ 0.1f;
    int max_length = 10000,i = 0;
    // int n = 0;
    int prenet_layer = gmm_dec->cfg->prenet_layer;
    wtk_matf_t *dec_out = wtk_matf_heap_new(heap,max_length,num_mels);  //输出的时候需要转下
    wtk_matf_t *y = wtk_matf_heap_new(heap,2,num_mels);
    wtk_matf_t yi;
    wtk_matf_t *proj_o = wtk_matf_heap_new(heap,1,num_mels*2);
    wtk_matf_t *c = wtk_matf_heap_new(heap,1,2*rnn_size);
    wtk_matf_t *prenet_out = wtk_matf_heap_new(heap,proj_o->row,gmm_dec->prenet_weight[prenet_layer-1]->row);
    wtk_matf_t *atten_rnn_in = wtk_matf_heap_new(heap,1,prenet_out->col+c->col);
    wtk_matf_t *atten_rnn_out = wtk_matf_heap_new(heap,atten_rnn_in->row,gmm_dec->atten_rnn->lstm_units);
    wtk_matf_t *linear_in = wtk_matf_heap_new(heap,atten_rnn_out->row,atten_rnn_out->col+c->col);
    wtk_matf_t *x = wtk_matf_heap_new(heap,linear_in->row,gmm_dec->linear_weight->row);
    wtk_matf_t *rnn1_h = wtk_matf_heap_new(heap,x->row,gmm_dec->lstm_rnn[0]->lstm_units);
    wtk_matf_t *rnn2_h = NULL;
    gmm_dec->query_p = wtk_matf_heap_new(heap,atten_rnn_out->row,gmm_dec->atten_query_weight->row);
    gmm_dec->omega_delta_sigma = wtk_matf_heap_new(heap,gmm_dec->query_p->row,gmm_dec->atten_v_weight->row);
    gmm_dec->atten_delata = wtk_heap_malloc(heap,sizeof(wtk_matf_t));
    gmm_dec->atten_sigma = wtk_heap_malloc(heap,sizeof(wtk_matf_t));

    gmm_dec->atten_delata->row = gmm_dec->omega_delta_sigma->row;
    gmm_dec->atten_delata->col = 1;
    gmm_dec->atten_delata->p = gmm_dec->omega_delta_sigma->p;

    gmm_dec->atten_sigma->row = gmm_dec->omega_delta_sigma->row;
    gmm_dec->atten_sigma->col = 1;
    gmm_dec->atten_sigma->p = gmm_dec->omega_delta_sigma->p+1;

    gmm_dec->mu_prev = wtk_matf_heap_new(heap,gmm_dec->atten_delata->row,gmm_dec->atten_delata->col);
    gmm_dec->J = qtk_tts_tac2_syn_gmmdec_atten_getJ(heap,encoder_out->row);
    gmm_dec->atten_alpha = wtk_matf_heap_new(heap,gmm_dec->J->row,gmm_dec->J->col);

    yi.row = y->row/2;
    yi.col = y->col;
    yi.p = y->p+(yi.row*yi.col);

    wtk_matf_zero(y);
    wtk_nn_lstm_reset(gmm_dec->atten_rnn);
    for(i = 0; i < gmm_dec->cfg->rnn_n; ++i){
        wtk_nn_lstm_reset(gmm_dec->lstm_rnn[i]);
    }
    if(gmm_dec->cfg->rnn_n == 2){
        rnn2_h = wtk_matf_heap_new(heap,x->row,gmm_dec->lstm_rnn[1]->lstm_units);
    }
    wtk_debug("decoder start\n");
    for(i = 0; i < max_length; i += 2){
        qtk_tts_tac2_syn_gmmdec_prenet_process(gmm_dec,&yi,prenet_out);
        wtk_mer_matf_concat(c,prenet_out,atten_rnn_in);
        wtk_nn_lstm_cell(gmm_dec->atten_rnn,atten_rnn_in,atten_rnn_out);
        qtk_tts_tac2_syn_gmmdec_atten_process(gmm_dec,atten_rnn_out);
        wtk_mer_unblas_sgemm_notrans(gmm_dec->atten_alpha,encoder_out,NULL,c);
        wtk_mer_matf_concat(c,atten_rnn_out,linear_in);
        wtk_nn_layer_dense(linear_in,gmm_dec->linear_weight,gmm_dec->linear_bias,NULL,x);
        wtk_nn_lstm_cell(gmm_dec->lstm_rnn[0],x,rnn1_h);
        wtk_matf_add(x,x,rnn1_h);
        if(gmm_dec->cfg->rnn_n == 2){
            wtk_nn_lstm_cell(gmm_dec->lstm_rnn[1],x,rnn2_h);
            wtk_matf_add(x,x,rnn2_h);
        }
        wtk_nn_layer_dense(x,gmm_dec->proj_weight,NULL,NULL,proj_o);
        if(gmm_dec->mu_prev->p[0] >= threshold){
            break;
        }
        qtk_tts_tac2_syn_gmmdec_proj_view(proj_o,y);
        memcpy(dec_out->p+i*dec_out->col,y->p,sizeof(float)*y->row*y->col);
    }
    if(max_length - i <= 2){
        wtk_debug("decoder error\n");
        return -1;
    }
    dec_out->row = i;

    if(gmm_dec->notify){
        gmm_dec->notify(gmm_dec->user_data,dec_out,is_end);
    }
    return 0;
}

//使用prenet
#define BERNOULLI(p) (random() < ((p)*RAND_MAX)?1.0f:0.0f)

int qtk_tts_tac2_syn_gmmdec_prenet_process(qtk_tts_tac2_syn_gmmdec_t *syn,wtk_matf_t *input,wtk_matf_t *output)
{
    int i = 0,j = 0;
    wtk_heap_t *heap = syn->heap;
    wtk_vecf_t *bernoulli_random = NULL;
    int layer_len = syn->cfg->prenet_layer;
    wtk_matf_t **kernel = syn->prenet_weight;
    wtk_vecf_t **bias = syn->prenet_bias;
    wtk_matf_t *out = NULL;

    //这边选择的prenet模型是一样的  所以可以使用同一个output
    for (i=0; i<layer_len; ++i){
        out = wtk_matf_heap_new(heap,input->row,kernel[i]->row);
        bernoulli_random = wtk_vecf_heap_new(heap,out->col);
        wtk_nn_layer_dense(input, kernel[i], bias[i], wtk_nn_relu, out);
        for(j = 0; j < bernoulli_random->len;++j){
            bernoulli_random->p[j] = BERNOULLI(0.5f)*2;
            // bernoulli_random->p[j] = 1.0f;
        }
        wtk_matf_vecf_multi(out,bernoulli_random);
        input = out;
    }
    wtk_matf_cpy(out,output);
    return 0;   
}

//注意力机制
int qtk_tts_tac2_syn_gmmdec_atten_process(qtk_tts_tac2_syn_gmmdec_t *dec,wtk_matf_t *query)
{
    int ret = 0;
    int i = 0,n = dec->J->row * dec->J->col;
    float tmp = 0.0f,*alphap = dec->atten_alpha->p;

    wtk_nn_layer_dense(query,dec->atten_query_weight,dec->atten_query_bias,wtk_nn_tanh,dec->query_p);
    wtk_nn_layer_dense(dec->query_p,dec->atten_v_weight,NULL,NULL,dec->omega_delta_sigma);
    wtk_nn_softplus(dec->atten_delata->p,dec->atten_delata->row*dec->atten_delata->col);
    wtk_nn_softplus(dec->atten_sigma->p,dec->atten_sigma->row*dec->atten_sigma->col);
    wtk_float_clamp(dec->omega_delta_sigma->p,dec->omega_delta_sigma->row*dec->omega_delta_sigma->col,0,0.0f,1,1.0f);
    wtk_matf_add(dec->mu_prev,dec->mu_prev,dec->atten_delata);
    for(i = 0; i < n; ++i,++alphap){
        tmp = (dec->J->p[i] - dec->mu_prev->p[0])/dec->atten_sigma->p[0];
        tmp *= tmp;
        *alphap = expf(-1*0.5f*tmp);
    }
    return ret;
}

wtk_matf_t* qtk_tts_tac2_syn_gmmdec_atten_getJ(wtk_heap_t *heap,int row)
{
    wtk_matf_t *J = wtk_matf_heap_new(heap,1,row);
    int n = row,i = 0;
    for(i = 0; i < n; ++i){
        J->p[i] = i;
    }
    return J;
}

void qtk_tts_tac2_syn_gmmdec_proj_view(wtk_matf_t *proj_o,wtk_matf_t *y)
{
    int i = 0,j = 0,k = 0, n = 0;
    if(proj_o->row*proj_o->col != y->row * y->col){
        wtk_debug("error dim\n");
        return;
    }
    float *p = y->p;
    n = proj_o->col * proj_o->row;
    for(k = 0; k < n; ++k){
        if(i == y->row){
            i = 0;
            j += 1;
        }
        *(p+i*y->col+j) = proj_o->p[k];
        ++i;
    }
    return;
}

void qtk_tts_tac2_syn_gmmdec_set_notify(qtk_tts_tac2_syn_gmmdec_t *gmm_dec,void *user_data,qtk_tts_tac2_syn_gmmdec_notify_f notify)
{
    gmm_dec->user_data = user_data;
    gmm_dec->notify = notify;
    
    return;
}
