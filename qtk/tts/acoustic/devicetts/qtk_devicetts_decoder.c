#include "qtk_devicetts_decoder.h"
#include "tts-mer/wtk-extend/wtk_heap2.h"
#include "tts-mer/wtk-extend/wtk_mat2.h"

wtk_matf_t* qtk_devicetts_decoder_prenet(qtk_devicetts_decoder_t *dec, wtk_matf_t *decoder_input);

//load bin file
//static qtk_torchnn_linear_t* qtk_torchnn_linear_load_file_fordevice(char *weight_fn,char *bias_fn, int bin)
//{
//    int ret = 0;
//    wtk_source_t source, *src;
//	int col=0, row=0;
//	qtk_blas_matrix_t *m = 0;
//	qtk_torchnn_linear_t *linear = qtk_torchnn_linear_new();
//
//	src = &source;
//	wtk_source_clean_file(src);
//	ret=wtk_source_init_file(src, weight_fn);
//	if (ret!=0) goto end;
//	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
//	ret = wtk_source_read_int(src, &row, 1, bin);
//	if(ret!=0)goto end;
//	ret = wtk_source_read_int(src, &col, 1, bin);
//	if(ret!=0)goto end;
//	//wtk_debug("%d %d\n",row,col);
//	m=qtk_blas_matrix_new(row,col);
//	//wtk_debug("%d %d\n",row,col);
//	ret = wtk_source_read_float(src, m->m, row * col, bin);
//	if(ret!=0)goto end;
//	linear->weight=m;
//
//	wtk_source_clean_file(src);
//	ret=wtk_source_init_file(src, bias_fn);
//	if (ret!=0) goto end;
//	ret = wtk_source_read_int(src, &col, 1, bin);
//	if(ret!=0)goto end;
//	//wtk_debug("%d\n",col);
//	m=qtk_blas_matrix_new(1,col);
//	ret = wtk_source_read_float(src, m->m, 1 * col, bin);
//	if(ret!=0)goto end;
//	linear->bias=m;
//end:
//
//	return linear;
//}

qtk_devicetts_decoder_t *qtk_devicetts_decoder_new_world(qtk_devicetts_decoder_cfg_t *cfg,wtk_heap_t *heap)
{
    qtk_devicetts_decoder_t *dec = NULL;
    int num = 0,i = 0,*dim = NULL;
    wtk_source_loader_t sl;
    wtk_source_t source;
    
    wtk_source_loader_init_file(&sl);
    dec = wtk_calloc(1, sizeof(qtk_devicetts_decoder_t));
    dec->cfg = cfg;
    dec->heap = heap;
    dec->prenet_layer = wtk_calloc(1, sizeof(qtk_nn_fc_t*)*2);
    num = cfg->decoder_rnn_dim;
    dec->prenet_layer[0] = qtk_nn_fc_new(num,cfg->prenet_layers_dim,QTK_NN_ACTINATION_RULE,0,1);
    dec->prenet_layer[1] = qtk_nn_fc_new(cfg->prenet_layers_dim,cfg->prenet_layers_dim,QTK_NN_ACTINATION_RULE,0,1);
    qtk_nn_fc_load_file(dec->prenet_layer[0],cfg->prenet_layer_weight_fn[0],NULL);
    qtk_nn_fc_load_file(dec->prenet_layer[1],cfg->prenet_layer_weight_fn[1],NULL);
    
    dec->gru = wtk_malloc(sizeof(wtk_nn_rnngru_t*)*2);
    dim = cfg->gru_dim_num->slot;
    for(i = 0; i < 2;++i){
        dec->gru[i] = wtk_nn_rnngru_new2(wtk_nn_enum_type_pytorch,1,dim[0],dim[1],NULL);
        wtk_mer_source_loader_load_matf(&sl,&source,cfg->gru_gate_weight_fn[i],dec->gru[i]->gate_kernel);
        wtk_mer_source_loader_load_matf(&sl,&source,cfg->gru_candidate_weight_fn[i],dec->gru[i]->candidate_kernel);
        wtk_mer_source_loader_load_matf(&sl,&source,cfg->gru_candidate_hh_weight_fn[i],dec->gru[i]->candidate_kernel_hh);
        wtk_mer_source_loader_load_vecf(&sl,&source,cfg->gru_gate_bias_fn[i],dec->gru[i]->gate_bias);
        wtk_mer_source_loader_load_vecf(&sl,&source,cfg->gru_candidate_bias_fn[i],dec->gru[i]->candidate_bias);
        wtk_mer_source_loader_load_vecf(&sl,&source,cfg->gru_candidate_hh_bias_fn[i],dec->gru[i]->candidate_bias_hh);
        dim += 2;
    }
    dim = cfg->projection_dim_num->slot;
    dec->projection = qtk_nn_fc_new(dim[0],dim[1],QTK_NN_ACTINATION_NULL,1,1);
    qtk_nn_fc_load_file(dec->projection,cfg->projection_weight_fn,cfg->projection_bias_fn);

    dec->conv_mgc = qtk_devicetts_postnet_new(&cfg->postnet,dec->heap);

    if (cfg->linear_bap_weight_fn)
    {
    	dec->linear_bap = qtk_nn_fc_new(num,cfg->bap_dim,QTK_NN_ACTINATION_NULL,1,1);
    	qtk_nn_fc_load_file(dec->linear_bap,cfg->linear_bap_weight_fn,cfg->linear_bap_bias_fn);
    }

    if (cfg->linear_lf0_weight_fn)
    {
    	dec->linear_lf0 = qtk_nn_fc_new(num,cfg->lf0_dim,QTK_NN_ACTINATION_NULL,1,1);
    	qtk_nn_fc_load_file(dec->linear_lf0,cfg->linear_lf0_weight_fn,cfg->linear_lf0_bias_fn);
    }

    if (cfg->linear_vuv_weight_fn)
    {
    	dec->linear_vuv = qtk_nn_fc_new(num,cfg->vuv_dim,QTK_NN_ACTINATION_NULL,1,1);
    	qtk_nn_fc_load_file(dec->linear_vuv,cfg->linear_vuv_weight_fn,cfg->linear_vuv_bias_fn);
    }

    return dec;
}

qtk_devicetts_decoder_t *qtk_devicetts_decoder_new_lpcnet(qtk_devicetts_decoder_cfg_t *cfg,wtk_heap_t *heap)
{
    qtk_devicetts_decoder_t *dec = NULL;
    int num = 0,i = 0,*dim = NULL;
    wtk_source_loader_t sl;
    wtk_source_t source;

    wtk_source_loader_init_file(&sl);
    dec = wtk_calloc(1, sizeof(qtk_devicetts_decoder_t));
    dec->cfg = cfg;
    dec->heap = heap;
    dec->prenet_layer = wtk_calloc(1, sizeof(qtk_nn_fc_t*)*2);
    num = cfg->mix_resolution_factor*cfg->decoder_rnn_dim;
    dec->prenet_layer[0] = qtk_nn_fc_new(num,cfg->prenet_layers_dim,QTK_NN_ACTINATION_RULE,0,1);
    dec->prenet_layer[1] = qtk_nn_fc_new(cfg->prenet_layers_dim,cfg->prenet_layers_dim,QTK_NN_ACTINATION_RULE,0,1);
    qtk_nn_fc_load_file(dec->prenet_layer[0],cfg->prenet_layer_weight_fn[0],NULL);
    qtk_nn_fc_load_file(dec->prenet_layer[1],cfg->prenet_layer_weight_fn[1],NULL);

    dec->gru = wtk_malloc(sizeof(wtk_nn_rnngru_t*)*2);
    dim = cfg->gru_dim_num->slot;
    for(i = 0; i < 2;++i){
        dec->gru[i] = wtk_nn_rnngru_new2(wtk_nn_enum_type_pytorch,1,dim[0],dim[1],NULL);
        wtk_mer_source_loader_load_matf(&sl,&source,cfg->gru_gate_weight_fn[i],dec->gru[i]->gate_kernel);
        wtk_mer_source_loader_load_matf(&sl,&source,cfg->gru_candidate_weight_fn[i],dec->gru[i]->candidate_kernel);
        wtk_mer_source_loader_load_matf(&sl,&source,cfg->gru_candidate_hh_weight_fn[i],dec->gru[i]->candidate_kernel_hh);
        wtk_mer_source_loader_load_vecf(&sl,&source,cfg->gru_gate_bias_fn[i],dec->gru[i]->gate_bias);
        wtk_mer_source_loader_load_vecf(&sl,&source,cfg->gru_candidate_bias_fn[i],dec->gru[i]->candidate_bias);
        wtk_mer_source_loader_load_vecf(&sl,&source,cfg->gru_candidate_hh_bias_fn[i],dec->gru[i]->candidate_bias_hh);
        dim += 2;
    }
    dim = cfg->projection_dim_num->slot;
    dec->projection = qtk_nn_fc_new(dim[0],dim[1],QTK_NN_ACTINATION_NULL,1,1);
    qtk_nn_fc_load_file(dec->projection,cfg->projection_weight_fn,cfg->projection_bias_fn);
    if (cfg->conv1d_weight_fn && cfg->conv1d_bias_fn)
    {
        dec->conv1d = qtk_nn_conv1d_new(cfg->decoder_rnn_dim,cfg->num_mels,1,0,1);
        qtk_nn_conv1d_load_file(dec->conv1d,cfg->conv1d_weight_fn,cfg->conv1d_bias_fn);
    }

    return dec;
}

int qtk_devicetts_decoder_delete(qtk_devicetts_decoder_t *dec)
{
    int i = 0;
    qtk_nn_fc_delete(dec->prenet_layer[1]);
    qtk_nn_fc_delete(dec->prenet_layer[0]);
    for(i = 0; i < 2;++i){
        wtk_nn_rnngru_delete(dec->gru[i]);
    }
    if (dec->gru)
    	wtk_free(dec->gru);
    if (dec->prenet_layer)
    	wtk_free(dec->prenet_layer);
    if (dec->projection)
    	qtk_nn_fc_delete(dec->projection);
    if (dec->conv1d)
    	qtk_nn_conv1d_delete(dec->conv1d);
    if (dec->conv_mgc)
    	qtk_devicetts_postnet_delete(dec->conv_mgc);
    if (dec->linear_bap)
    	qtk_nn_fc_delete(dec->linear_bap);
    if (dec->linear_lf0)
    	qtk_nn_fc_delete(dec->linear_lf0);
    if (dec->linear_vuv)
    	qtk_nn_fc_delete(dec->linear_vuv);
    wtk_free(dec);
    return 0;
}

wtk_matf_t* qtk_devicetts_decoder_process_lpcnet(qtk_devicetts_decoder_t *dec,wtk_matf_t *aligned_features)
{
    wtk_heap_t *heap = dec->heap;
    int alf_row = aligned_features->row;
    int alf_col = aligned_features->col;
    wtk_matf_t *prenet_input = wtk_matf_heap_new(heap,1,dec->cfg->decoder_output_size);
    wtk_matf_t *prenet_out = NULL;
    wtk_matf_t *decoder_in = wtk_matf_heap_new(heap,1,aligned_features->col+dec->prenet_layer[1]->kernel->col);
    float *alfp = aligned_features->p;
    int i = 0;
    wtk_matf_t *gru_out1 = NULL,*gru_out2 = NULL;
    wtk_matf_t *out = NULL;
    gru_out1 = wtk_matf_heap_new(heap,1,dec->gru[0]->num_units);
    gru_out2 = wtk_matf_heap_new(heap,1,dec->gru[1]->num_units);
    int projection_krow = dec->projection->kernel->row;
    out = wtk_matf_heap_new(heap,alf_row,projection_krow);
    wtk_matf_t projection_out;
    projection_out.row = 1;
    projection_out.col = projection_krow;
    for(i = 0; i < 2; ++i){
        wtk_nn_rnngru_reset(dec->gru[i]);
    }
    float *prop = out->p;
    for(i = 0; i < alf_row; ++i){
        prenet_out = qtk_devicetts_decoder_prenet(dec,prenet_input);
        memcpy(decoder_in->p,prenet_out->p,sizeof(float)*prenet_out->col);
        memcpy(decoder_in->p+prenet_out->col,alfp+alf_col*i,sizeof(float)*alf_col);
        wtk_nn_rnngru_cell(dec->gru[0],decoder_in,gru_out1);
        wtk_nn_rnngru_cell(dec->gru[1],gru_out1,gru_out2);
        projection_out.p = prop+i*projection_krow;
        qtk_nn_fc_forward(dec->projection,gru_out2,&projection_out);
        memcpy(prenet_input->p,projection_out.p,sizeof(float)*projection_krow);
    }
    wtk_matf_reshape(out,-1,dec->cfg->decoder_rnn_dim);
    wtk_matf_t *out_mel = wtk_matf_heap_new(heap,out->row,dec->conv1d->out_dim);
    qtk_nn_conv1d_forward(dec->conv1d,out,out_mel);


    return out_mel;
}

wtk_matf_t* qtk_devicetts_decode_getlinear(qtk_devicetts_decoder_t *dec, wtk_matf_t* in, qtk_nn_fc_t* fc)
{
    wtk_matf_t *out = wtk_matf_heap_new(dec->heap,in->row, fc->kernel->row);
    qtk_nn_fc_forward(fc, in, out);

    return out;
}

wtk_matf_t* qtk_devicetts_decode_getPostnet(qtk_devicetts_decoder_t *dec, wtk_matf_t* in, qtk_devicetts_postnet_t * postnet)
{
	return qtk_devicetts_postnet_process(postnet, in);
}

wtk_matf_t* qtk_devicetts_decoder_process_world(qtk_devicetts_decoder_t *dec,wtk_matf_t *aligned_features)
{
    wtk_heap_t *heap = dec->heap;
    int alf_row = aligned_features->row;
    int alf_col = aligned_features->col;
    wtk_matf_t *prenet_input = wtk_matf_heap_new(heap,1,dec->cfg->decoder_rnn_dim);
    wtk_matf_t *prenet_out = NULL;
    wtk_matf_t *decoder_in = wtk_matf_heap_new(heap,1,aligned_features->col+dec->prenet_layer[1]->kernel->col);
//    wtk_matf_t *mgc = NULL;
    float *alfp = aligned_features->p;
    int i = 0;
    wtk_matf_t *gru_out1 = NULL,*gru_out2 = NULL;
    wtk_matf_t *out = NULL;
    gru_out1 = wtk_matf_heap_new(heap,1,dec->gru[0]->num_units);
    gru_out2 = wtk_matf_heap_new(heap,1,dec->gru[1]->num_units);
    int projection_krow = dec->projection->kernel->row;
    out = wtk_matf_heap_new(heap,alf_row/dec->cfg->mix_resolution_factor,projection_krow);
    wtk_matf_t projection_out;
    projection_out.row = 1;
    projection_out.col = projection_krow;
    for(i = 0; i < 2; ++i){
        wtk_nn_rnngru_reset(dec->gru[i]);
    }
    float *prop = out->p;
    for(i = 0; i < alf_row; i+=dec->cfg->mix_resolution_factor){
        prenet_out = qtk_devicetts_decoder_prenet(dec,prenet_input);
        memcpy(decoder_in->p,prenet_out->p,sizeof(float)*prenet_out->col);
        memcpy(decoder_in->p+prenet_out->col,alfp+alf_col*i,sizeof(float)*alf_col);
        wtk_nn_rnngru_cell(dec->gru[0],decoder_in,gru_out1); 
        wtk_nn_rnngru_cell(dec->gru[1],gru_out1,gru_out2);
        wtk_matf_reshape(&projection_out, 1, projection_krow);
        projection_out.p = prop+i/dec->cfg->mix_resolution_factor*projection_krow;
        qtk_nn_fc_forward(dec->projection,gru_out2,&projection_out);
        wtk_matf_reshape(&projection_out, dec->cfg->mix_resolution_factor, dec->cfg->decoder_rnn_dim);
        memcpy(prenet_input->p,projection_out.p+(dec->cfg->mix_resolution_factor-1)*dec->cfg->decoder_rnn_dim,sizeof(float)*dec->cfg->decoder_rnn_dim);
    }
    wtk_matf_reshape(out,-1,dec->cfg->decoder_rnn_dim);

//    mgc = qtk_devicetts_postnet_process(dec->conv_mgc,out);

//    wtk_nn_tanh(out->p, out->row * out->col);
//    //bap
//    wtk_matf_t *out_bap = wtk_matf_heap_new(heap,out->row, dec->linear_bap->kernel->col);
//    qtk_nn_fc_forward(dec->linear_bap, out, out_bap);
//    //lf0
//    wtk_matf_t *out_lf0 = wtk_matf_heap_new(heap,out->row, dec->linear_lf0->kernel->col);
//    qtk_nn_fc_forward(dec->linear_lf0, out, out_lf0);
//
//    //vuv
//    wtk_matf_t *out_vuv = wtk_matf_heap_new(heap,out->row, dec->linear_vuv->kernel->col);
//    qtk_nn_fc_forward(dec->linear_vuv, out, out_vuv);

    return out;
}

// decoder process
#define BERNOULLI(p) (random() < ((p)*RAND_MAX)?1.0f:0.0f)

wtk_matf_t* qtk_devicetts_decoder_prenet(qtk_devicetts_decoder_t *dec, wtk_matf_t *decoder_input)
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
    }
    qtk_nn_fc_forward(layer[1],layer_out1,layer_out2);
    n = layer_out2->row * layer_out2->col;
    for(i = 0; i < n; ++i){
        layer_out2->p[i] = BERNOULLI(0.5f) * layer_out2->p[i];
    }

    return layer_out2;
}
