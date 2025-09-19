#include "qtk_devicetts.h"
#include "tts-mer/wtk-extend/wtk_mat2.h"
#include "tts-mer/wtk-extend/wtk_heap2.h"
#include "tts-mer/wtk-extend/wtk_mer_source_file.h"

wtk_matf_t* qtk_devicetts_embedding_process(qtk_devicetts_t *dev,wtk_veci_t *token);

qtk_devicetts_normalizer_t* qtk_devicetts_normalizer_new(char* mean_path, char* std_path, int mgc_dim, int lf0_dim, int bap_dim)
{
	qtk_devicetts_normalizer_t* normalizer;
    wtk_source_loader_t sl;
    wtk_source_t source;

	normalizer = wtk_calloc(1, sizeof(*normalizer));
	normalizer->mgc_order = mgc_dim;
	normalizer->lf0_order = lf0_dim;
	normalizer->bap_order = bap_dim;
	normalizer->feature_dim = mgc_dim + lf0_dim + bap_dim;

	wtk_source_loader_init_file(&sl); //fixed bug
	normalizer->mean_vector = wtk_calloc(normalizer->feature_dim * 3, sizeof(float));
	wtk_mer_source_loader_load_float(&sl,&source, mean_path, normalizer->mean_vector, normalizer->feature_dim * 3);
//	print_float(normalizer->mean_vector, 3*62);
	normalizer->std_vector = wtk_calloc(normalizer->feature_dim * 3, sizeof(float));
	wtk_mer_source_loader_load_float(&sl,&source, std_path, normalizer->std_vector, normalizer->feature_dim * 3);

	normalizer->mgc_mean_vector = normalizer->mean_vector;                      //len->mgc_dim, 60
	normalizer->lf0_mean_vector = &(normalizer->mean_vector[3 * mgc_dim]);      //len->lf0_dim, 1
	normalizer->bap_mean_vector = &(normalizer->mean_vector[3 * mgc_dim + 3 * lf0_dim]);    //len->bap_dim, 1

	normalizer->mgc_std_vector = normalizer->std_vector;                        //len->mgc_dim, 60
	normalizer->lf0_std_vector = &(normalizer->std_vector[3 * mgc_dim]);        //len->lf0_dim, 1
	normalizer->bap_std_vector = &(normalizer->std_vector[3 * mgc_dim + 3 * lf0_dim]);      //len->bap_dim, 1
	return normalizer;
}

int qtk_devicetts_normalizer_denormal(float* mean, float* std, wtk_matf_t* feat)
{
	int i,j;
	float *p;

	p = feat->p;
	for(i=0; i < feat->row; i++)
		for(j=0; j < feat->col; j++)
		{
			*p = (*p) *  std[j] + mean[j];
			p++;
		}
	return 0;
}
void qtk_devicetts_normalizer_delete(qtk_devicetts_normalizer_t* normalizer)
{
	if (normalizer->mean_vector)
		wtk_free(normalizer->mean_vector);
	if (normalizer->std_vector)
		wtk_free(normalizer->std_vector);
	wtk_free(normalizer);
}

qtk_devicetts_t *qtk_devicetts_new_lpcnet(qtk_devicetts_cfg_t *cfg)
{
    qtk_devicetts_t *dev = NULL;
    int *dim = NULL;
    wtk_source_loader_t sl;
    wtk_source_t source;
    dev = wtk_malloc(sizeof(qtk_devicetts_t));
    memset(dev,0, sizeof(*dev));
    
    wtk_source_loader_init_file(&sl);
    dev->cfg = cfg;
    dev->heap = wtk_heap_new(4096);
    dim = (int*)cfg->embedding_dim_num->slot;
    dev->embedding_table = wtk_matf_new(dim[0],dim[1]);
    wtk_mer_source_loader_load_matf(&sl,&source,cfg->embedding_fn,dev->embedding_table);
    
    dev->encoder = qtk_tts_dfsmn_new(&cfg->encoder,dev->heap);
    dev->dur = qtk_devicetts_duration_predictor_new(&cfg->dur,dev->heap);
    dev->dec = qtk_devicetts_decoder_new_lpcnet(&cfg->dec,dev->heap);
    dev->postnet = qtk_devicetts_postnet_new(&cfg->postnet,dev->heap);

    return dev;
}

qtk_devicetts_t *qtk_devicetts_new_world(qtk_devicetts_cfg_t *cfg)
{
    qtk_devicetts_t *dev = NULL;
    int *dim = NULL;
    wtk_source_loader_t sl;
    wtk_source_t source;
    dev = wtk_malloc(sizeof(qtk_devicetts_t));
    memset(dev,0, sizeof(*dev));

    wtk_source_loader_init_file(&sl);
    dev->cfg = cfg;
    dev->heap = wtk_heap_new(4096);
    dim = (int*)cfg->embedding_dim_num->slot;
    dev->embedding_table = wtk_matf_new(dim[0],dim[1]);
    wtk_mer_source_loader_load_matf(&sl,&source,cfg->embedding_fn,dev->embedding_table);

    dev->encoder = qtk_tts_dfsmn_new(&cfg->encoder,dev->heap);
    dev->dur = qtk_devicetts_duration_predictor_new(&cfg->dur,dev->heap);
    dev->dec = qtk_devicetts_decoder_new_world(&cfg->dec,dev->heap);
    dev->normalizer = qtk_devicetts_normalizer_new(cfg->mean_fn, cfg->std_fn, cfg->dec.mgc_dim, cfg->dec.lf0_dim, cfg->dec.bap_dim);

    return dev;
}


float emb[]={
#include "data/tmp.emb.ctxt"
};
int qtk_devicetts_process_world(qtk_devicetts_t *dev,wtk_veci_t *token, int is_end)
{
    wtk_heap_t *heap = dev->heap;
    wtk_matf_t *embedding = NULL;
    wtk_matf_t *encoder_out = NULL;
    wtk_matf_t *decoder_out = NULL;

    qtk_devicetts_reset(dev);
    embedding = qtk_devicetts_embedding_process(dev,token);

    //test
//    embedding = wtk_matf_heap_new(heap,56,128);
//    embedding->p = emb;

    encoder_out = wtk_matf_heap_new(heap,embedding->row,dev->encoder->rnn_gru[0]->num_units*2);
    qtk_tts_dfsmn_process(dev->encoder,embedding,encoder_out);
    wtk_matf_t *LR = qtk_devicetts_duration_predictor_forward(dev->dur,encoder_out);
    //reshape
    wtk_matf_t *aligned_features = wtk_heap_malloc(heap,sizeof(wtk_matf_t));
    aligned_features->col = LR->col;
    aligned_features->row = LR->row;
    aligned_features->p = LR->p;
    decoder_out = qtk_devicetts_decoder_process_world(dev->dec,aligned_features);

    if(dev->notify){
        dev->notify(dev->user_data,decoder_out, is_end);
    }

    return 0;
}

int qtk_devicetts_process_lpcnet(qtk_devicetts_t *dev,wtk_veci_t *token, int is_end)
{
    wtk_heap_t *heap = dev->heap;
    wtk_matf_t *embedding = NULL;
    wtk_matf_t *encoder_out = NULL;
    wtk_matf_t *decoder_out = NULL;
    wtk_matf_t *postnet_out = NULL;
    wtk_matf_t *out_mel;

    qtk_devicetts_reset(dev);
    embedding = qtk_devicetts_embedding_process(dev,token);
    encoder_out = wtk_matf_heap_new(heap,embedding->row,dev->encoder->rnn_gru[0]->num_units*2);
    qtk_tts_dfsmn_process(dev->encoder,embedding,encoder_out);
    wtk_matf_t *LR = qtk_devicetts_duration_predictor_forward(dev->dur,encoder_out);
    //reshape
    int ndata = LR->col*LR->row;
    wtk_matf_t *aligned_features = wtk_heap_malloc(heap,sizeof(wtk_matf_t));
    aligned_features->col = dev->dur->cfg->mix_resolution_factor*257;
    aligned_features->row = ndata/aligned_features->col;
    aligned_features->p = LR->p;
    decoder_out = qtk_devicetts_decoder_process_lpcnet(dev->dec,aligned_features);

    //printf("out row=%d col=%d\n", decoder_out->row, decoder_out->col);
    out_mel = wtk_matf_heap_new(heap,decoder_out->row,dev->dec->conv1d->out_dim);
    qtk_nn_conv1d_forward(dev->dec->conv1d,decoder_out,out_mel);
    //printf("out_mel row=%d col=%d\n", out_mel->row, out_mel->col);
    postnet_out = qtk_devicetts_postnet_process(dev->postnet,out_mel);
    wtk_matf_add(postnet_out,out_mel,postnet_out);
    //printf("raw=%d col=%d\n", postnet_out->row, postnet_out->col);

    if(dev->notify){
        dev->notify(dev->user_data,postnet_out, is_end);
    }
    return 0;
}

int qtk_devicetts_delete(qtk_devicetts_t *dev)
{
	if (dev->heap)
		wtk_heap_delete(dev->heap);
    if (dev->embedding_table)
    	wtk_matf_delete(dev->embedding_table);
    if (dev->encoder)
    	qtk_tts_dfsmn_delete(dev->encoder);
    if (dev->dur)
    	qtk_devicetts_duration_predictor_delete(dev->dur);
    if (dev->dec)
    	qtk_devicetts_decoder_delete(dev->dec);
    if (dev->postnet)
    	qtk_devicetts_postnet_delete(dev->postnet);
    if (dev->normalizer)
    	qtk_devicetts_normalizer_delete(dev->normalizer);
    wtk_free(dev);
    return 0;
}

int qtk_devicetts_reset(qtk_devicetts_t *dev)
{
    wtk_heap_reset(dev->heap);
    return 0;
}

wtk_matf_t* qtk_devicetts_embedding_process(qtk_devicetts_t *dev,wtk_veci_t *token)
{
    int tkl = token->len,*tk = token->p;
    int i = 0;
    wtk_matf_t *embedding = dev->embedding_table;
    int col = embedding->col;
    wtk_matf_t *out = wtk_matf_heap_new(dev->heap,tkl,col);

    assert(out->col == col);

    for(i = 0; i < tkl; ++i){
        memcpy(out->p+col * i,embedding->p + col *tk[i],sizeof(float)*col);
    }
    return out;
}

int qtk_devicetts_set_notify(qtk_devicetts_t *dev, qtk_devicetts_notify_f notify, void *user_data)
{
    dev->notify = notify;
    dev->user_data = user_data;
    return 0;
}
