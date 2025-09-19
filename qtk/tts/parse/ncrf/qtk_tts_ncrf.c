#include "qtk_tts_ncrf.h"
#include "tts-mer/wtk-extend/wtk_mer_source_file.h"
#include "tts-mer/wtk-extend/wtk_blas.h"
#include "tts-mer/wtk-extend/wtk_heap2.h"

int qtk_tts_ncrf_charrep_new(qtk_tts_ncrf_t *crf);
int qtk_tts_ncrf_charrep_new2(qtk_tts_ncrf_t *crf, wtk_rbin2_t *rbin);
int qtk_tts_ncrf_pw_new(qtk_tts_ncrf_t *crf);
int qtk_tts_ncrf_pw_new2(qtk_tts_ncrf_t *crf, wtk_rbin2_t* rbin);
int qtk_tts_ncrf_pp_new(qtk_tts_ncrf_t *crf);
int qtk_tts_ncrf_pp_new2(qtk_tts_ncrf_t *crf, wtk_rbin2_t* rbin);
int qtk_tts_ncrf_ip_new(qtk_tts_ncrf_t *crf);
int qtk_tts_ncrf_ip_new2(qtk_tts_ncrf_t *crf, wtk_rbin2_t* rbin);
int qtk_tts_ncrf_charrep(qtk_tts_ncrf_t *ncrf,wtk_mati_t *in,wtk_matf_t *out);
int qtk_tts_ncrf_model_pw(qtk_tts_ncrf_t *ncrf,wtk_matf_t *input,wtk_matf_t*output);
int qtk_tts_ncrf_model_pp(qtk_tts_ncrf_t *ncrf,wtk_matf_t *input,wtk_matf_t*output);
int qtk_tts_ncrf_model_ip(qtk_tts_ncrf_t *ncrf,wtk_matf_t *input,wtk_matf_t*output);
void qtk_tts_ncrf_pw_delete(qtk_tts_ncrf_t *ncrf);
void qtk_tts_ncrf_pp_delete(qtk_tts_ncrf_t *ncrf);
void qtk_tts_ncrf_ip_delete(qtk_tts_ncrf_t *ncrf);
void qtk_tts_ncrf_charrep_delete(qtk_tts_ncrf_t *ncrf);
void qtk_tts_ncrf_pred_embedding(wtk_mati_t *in,wtk_matf_t *embedding,wtk_matf_t *out);
int qtk_tts_ncrf_viterbi_decode(qtk_tts_ncrf_t *ncrf,wtk_matf_t *tran,wtk_matf_t *in,wtk_mati_t *out);
void qtk_tts_ncrf_viterbi_decode_max(qtk_tts_ncrf_t *ncrf,wtk_matf_t *in,float *out,int *lists);
void qtk_tts_ncrf_fnn_pp_cat(wtk_matf_t *in1,wtk_matf_t *in2,wtk_matf_t *out);
void qtk_tts_ncrf_fnn_ip_cat(wtk_matf_t *in1,wtk_matf_t *in2,wtk_matf_t *in3,wtk_matf_t *out);
int qtk_tts_ncrf_combine(qtk_tts_ncrf_t *ncrf,wtk_mati_t *pwi,wtk_mati_t *ppi,wtk_mati_t *ipi,wtk_veci_t *out);

qtk_tts_ncrf_t* qtk_tts_ncrf_new(qtk_tts_ncrf_cfg_t *cfg)
{
    qtk_tts_ncrf_t *crf = NULL;
    int ret = -1;

    crf = wtk_malloc(sizeof(*crf));
    memset(crf,0,sizeof(qtk_tts_ncrf_t));
    crf->cfg = cfg;
    crf->uheap = wtk_heap_new(2048);

    ret = qtk_tts_ncrf_charrep_new(crf);
    if(ret != 0) goto end;
    ret = qtk_tts_ncrf_pw_new(crf);
    if(ret != 0) goto end;
    ret = qtk_tts_ncrf_pp_new(crf);
    if(ret != 0) goto end;
    ret = qtk_tts_ncrf_ip_new(crf);
    if(ret != 0) goto end;
    ret = 0;
end:
    if(ret != 0){
        qtk_tts_ncrf_delete(crf);
        crf = NULL;
    }
    return crf;
}
qtk_tts_ncrf_t* qtk_tts_ncrf_new2(qtk_tts_ncrf_cfg_t *cfg, wtk_rbin2_t *rbin)
{
    qtk_tts_ncrf_t *crf = NULL;
    int ret = -1;

    crf = wtk_malloc(sizeof(*crf));
    memset(crf,0,sizeof(qtk_tts_ncrf_t));
    crf->cfg = cfg;
    crf->uheap = wtk_heap_new(2048);

    ret = qtk_tts_ncrf_charrep_new2(crf, rbin);
    if(ret != 0) goto end;
    ret = qtk_tts_ncrf_pw_new2(crf, rbin);
    if(ret != 0) goto end;
    ret = qtk_tts_ncrf_pp_new2(crf, rbin);
    if(ret != 0) goto end;
    ret = qtk_tts_ncrf_ip_new2(crf, rbin);
    if(ret != 0) goto end;
    ret = 0;
end:
    if(ret != 0){
        qtk_tts_ncrf_delete(crf);
        crf = NULL;
    }
    return crf;
}

int qtk_tts_ncrf_delete(qtk_tts_ncrf_t *ncrf)
{
    if(ncrf == NULL) return -1;
    qtk_tts_ncrf_charrep_delete(ncrf);
    qtk_tts_ncrf_pw_delete(ncrf);
    qtk_tts_ncrf_pp_delete(ncrf);
    qtk_tts_ncrf_ip_delete(ncrf);
    wtk_heap_delete(ncrf->uheap);
    wtk_free(ncrf);
    return 0;
}

int qtk_tts_ncrf_process(qtk_tts_ncrf_t *ncrf,wtk_mati_t *in,wtk_veci_t *out)
{
    int ret = 0;
    wtk_matf_t *linear_pw_out=NULL,*charrep_out = NULL,
                        *pw_pred_out=NULL;
    wtk_heap_t *heap = NULL;
    wtk_mati_t *vatd = NULL,*vatdpp = NULL,*vatdip = NULL;
    wtk_matf_t  *pp_in = NULL,*pp_fnn_out = NULL,*linear_pp_out=NULL,*pp_pred_out=NULL;
    wtk_matf_t *ip_in = NULL,*ip_fnn_out = NULL,*linear_ip_out=NULL;

    heap = ncrf->uheap;
    //charrep
    charrep_out = wtk_matf_heap_new(heap,in->row,ncrf->linear_weight[1]->row);
    qtk_tts_ncrf_charrep(ncrf,in,charrep_out);    
    //pw
    linear_pw_out = wtk_matf_heap_new(heap,charrep_out->row,ncrf->linear_pw_weight->row);
    qtk_tts_ncrf_model_pw(ncrf,charrep_out,linear_pw_out);
    vatd = wtk_mati_heap_new(heap,1,linear_pw_out->row);
    qtk_tts_ncrf_viterbi_decode(ncrf,ncrf->crf_pw_transitions,linear_pw_out,vatd);
    pw_pred_out = wtk_matf_heap_new(heap,vatd->col*vatd->row,ncrf->pw_pred_embedding->col);
    qtk_tts_ncrf_pred_embedding(vatd,ncrf->pw_pred_embedding,pw_pred_out);
    //pp
    pp_in = wtk_matf_heap_new(heap,charrep_out->row,charrep_out->col+pw_pred_out->col);
    qtk_tts_ncrf_fnn_pp_cat(charrep_out,pw_pred_out,pp_in);
    pp_fnn_out = wtk_matf_heap_new(heap,pp_in->row,ncrf->pp_fnn_weight->row);
    wtk_nn_layer_dense(pp_in,ncrf->pp_fnn_weight,ncrf->pp_fnn_bias,NULL,pp_fnn_out);
    linear_pp_out = wtk_matf_heap_new(heap,pp_fnn_out->row,ncrf->linear_pp_weight->row);
    qtk_tts_ncrf_model_pp(ncrf,pp_fnn_out,linear_pp_out);
    vatdpp = wtk_mati_heap_new(heap,1,linear_pp_out->row);
    qtk_tts_ncrf_viterbi_decode(ncrf,ncrf->crf_pp_transitions,linear_pp_out,vatdpp);
    pp_pred_out = wtk_matf_heap_new(heap,vatdpp->col*vatdpp->row,ncrf->pp_pred_embedding->col);
    qtk_tts_ncrf_pred_embedding(vatdpp,ncrf->pp_pred_embedding,pp_pred_out);
    //ip
    ip_in = wtk_matf_heap_new(heap,charrep_out->row,charrep_out->col+pw_pred_out->col+pp_pred_out->col);
    qtk_tts_ncrf_fnn_ip_cat(charrep_out,pw_pred_out,pp_pred_out,ip_in);
    ip_fnn_out = wtk_matf_heap_new(heap,ip_in->row,ncrf->ip_fnn_weight->row);
    wtk_nn_layer_dense(ip_in,ncrf->ip_fnn_weight,ncrf->ip_fnn_bias,NULL,ip_fnn_out);
    linear_ip_out = wtk_matf_heap_new(heap,ip_fnn_out->row,ncrf->linear_ip_weight->row);
    qtk_tts_ncrf_model_ip(ncrf,ip_fnn_out,linear_ip_out);
    vatdip = wtk_mati_heap_new(heap,1,linear_ip_out->row);
    qtk_tts_ncrf_viterbi_decode(ncrf,ncrf->crf_ip_transitions,linear_ip_out,vatdip);
    //get combine
    qtk_tts_ncrf_combine(ncrf,vatd,vatdpp,vatdip,out);

    wtk_heap_reset(heap);
    return ret;
}

//********************
int qtk_tts_ncrf_charrep_new(qtk_tts_ncrf_t *crf)
{
    int ret = -1;
    qtk_tts_ncrf_cfg_t *cfg = crf->cfg;
    wtk_source_loader_t sl;
    wtk_source_t source;
    wtk_array_t *arr = NULL;
    int *diml = NULL,i = 0;
    int feature_num = cfg->feature_num;
    
    wtk_source_loader_init_file(&sl);
    arr = cfg->char_embedding_dim;
    if(NULL == arr || arr->nslot != 2){
        wtk_debug("get char_embedding error\n");
        goto end;
    }
    diml = (int*)arr->slot;
    crf->char_embedding = wtk_matf_new(diml[0],diml[1]);
    wtk_mer_source_loader_load_matf(&sl,&source,cfg->char_embedding_fn,crf->char_embedding);

    arr = cfg->feature_embedding_dim;
    if(NULL == arr || arr->nslot != 2*feature_num){
        wtk_debug("get feature embedding error\n");
        goto end;
    }
    crf->feature_embedding = wtk_malloc(sizeof(wtk_matf_t*)*feature_num);
    diml = arr->slot;
    for(i = 0; i < feature_num; ++i){
        crf->feature_embedding[i] = wtk_matf_new(diml[0],diml[1]);
        wtk_mer_source_loader_load_matf(&sl,&source,cfg->feature_embedding_fn[i],crf->feature_embedding[i]);
        diml += 2;
    }
    arr = cfg->linear_dim;
    if(NULL == arr || arr->nslot != 2*cfg->linear_num){
        wtk_debug("get linear error\n");
        goto end;
    }
    crf->linear_bias = wtk_malloc(sizeof(wtk_vecf_t*)*cfg->linear_num);
    crf->linear_weight = wtk_malloc(sizeof(wtk_matf_t*)*cfg->linear_num);
    diml = arr->slot;
    for(i = 0; i < cfg->linear_num;++i){
        crf->linear_weight[i] = wtk_matf_new(diml[0],diml[1]);
        wtk_mer_source_loader_load_matf(&sl,&source,cfg->linear_weight_fn[i],crf->linear_weight[i]);
        crf->linear_bias[i] = wtk_vecf_new(diml[0]);
        wtk_mer_source_loader_load_vecf(&sl,&source,cfg->linear_bias_fn[i],crf->linear_bias[i]);
        diml+=2;
    }

    ret = 0;
end:
    return ret;
}

int qtk_tts_ncrf_charrep_new2(qtk_tts_ncrf_t *crf, wtk_rbin2_t *rbin)
{
    int ret = -1;
    qtk_tts_ncrf_cfg_t *cfg = crf->cfg;
    wtk_source_loader_t sl;
    wtk_source_t src;
    wtk_array_t *arr = NULL;
    int *diml = NULL,i = 0;
    int feature_num = cfg->feature_num;
    FILE *f;

    wtk_source_loader_init_file(&sl);
    arr = cfg->char_embedding_dim;
    if(NULL == arr || arr->nslot != 2){
        wtk_debug("get char_embedding error\n");
        goto end;
    }
    diml = (int*)arr->slot;
    crf->char_embedding = wtk_matf_new(diml[0],diml[1]);
    //
	if (rbin)
	{
		f=wtk_rbin2_get_file(rbin, cfg->char_embedding_fn);
		wtk_source_init_fd(&src,f, ftell(f));
	    wtk_mer_source_read_float(&src, crf->char_embedding->p, crf->char_embedding->row*crf->char_embedding->col, 1);
		wtk_source_clean_fd(&src);
		if (f)fclose(f);
	}
	else
	{
		wtk_mer_source_loader_load_matf(&sl,&src,cfg->char_embedding_fn,crf->char_embedding);
	}

    arr = cfg->feature_embedding_dim;
    if(NULL == arr || arr->nslot != 2*feature_num){
        wtk_debug("get feature embedding error\n");
        goto end;
    }
    crf->feature_embedding = wtk_malloc(sizeof(wtk_matf_t*)*feature_num);
    diml = arr->slot;
    for(i = 0; i < feature_num; ++i){
        crf->feature_embedding[i] = wtk_matf_new(diml[0],diml[1]);
        if (rbin)
        {
    		f=wtk_rbin2_get_file(rbin, cfg->feature_embedding_fn[i]);
    		wtk_source_init_fd(&src,f, ftell(f));
    	    wtk_mer_source_read_float(&src, crf->feature_embedding[i]->p, crf->feature_embedding[i]->row*crf->feature_embedding[i]->col, 1);
    		wtk_source_clean_fd(&src);
    		if (f)fclose(f);
        }else
        {
        	wtk_mer_source_loader_load_matf(&sl,&src,cfg->feature_embedding_fn[i],crf->feature_embedding[i]);
        }
        diml += 2;
    }
    arr = cfg->linear_dim;
    if(NULL == arr || arr->nslot != 2*cfg->linear_num){
        wtk_debug("get linear error\n");
        goto end;
    }
    crf->linear_bias = wtk_malloc(sizeof(wtk_vecf_t*)*cfg->linear_num);
    crf->linear_weight = wtk_malloc(sizeof(wtk_matf_t*)*cfg->linear_num);
    diml = arr->slot;
    for(i = 0; i < cfg->linear_num;++i){
        crf->linear_weight[i] = wtk_matf_new(diml[0],diml[1]);
        if (rbin){
    		f=wtk_rbin2_get_file(rbin, cfg->linear_weight_fn[i]);
    		wtk_source_init_fd(&src,f, ftell(f));
    	    wtk_mer_source_read_float(&src, crf->linear_weight[i]->p, crf->linear_weight[i]->row*crf->linear_weight[i]->col, 1);
    		wtk_source_clean_fd(&src);
    		if (f)fclose(f);
        }else
        {
        	wtk_mer_source_loader_load_matf(&sl,&src,cfg->linear_weight_fn[i],crf->linear_weight[i]);
        }

        crf->linear_bias[i] = wtk_vecf_new(diml[0]);
        if (rbin)
        {
    		f=wtk_rbin2_get_file(rbin, cfg->linear_bias_fn[i]);
    		wtk_source_init_fd(&src,f, ftell(f));
    	    wtk_mer_source_read_float(&src, crf->linear_bias[i]->p, crf->linear_bias[i]->len, 1);
    		wtk_source_clean_fd(&src);
    		if (f)fclose(f);
        }else
        {
        	wtk_mer_source_loader_load_vecf(&sl,&src,cfg->linear_bias_fn[i],crf->linear_bias[i]);
        }

        diml+=2;
    }

    ret = 0;
end:
    return ret;
}

int qtk_tts_ncrf_pw_new(qtk_tts_ncrf_t *crf)
{
    int ret = -1;
    qtk_tts_ncrf_cfg_t *cfg = crf->cfg;
    wtk_array_t *arr = NULL;
    int *diml = NULL,i = 0,n = 0;
    wtk_source_loader_t sl;
    wtk_source_t source;

    wtk_source_loader_init_file(&sl);
    arr = cfg->lstm_pw_dim;
    if(NULL == arr || arr->nslot != 2*cfg->lstm_pw_num){
        wtk_debug("get lstm pw error\n");
        goto end;
    }
    crf->lstm_pw = wtk_malloc(sizeof(wtk_nn_lstm_t*)*cfg->lstm_pw_num*2);
    diml = arr->slot;
    for(i=0;i<cfg->lstm_pw_num;++i){
        n = i*2;
        crf->lstm_pw[n] = wtk_nn_lstm_new(1,diml[0],diml[1],0.0f,NULL,NULL);
        wtk_mer_source_loader_load_matf(&sl,&source,cfg->lstm_pw_weight_fn[n],crf->lstm_pw[n]->kernel);
        wtk_mer_source_loader_load_vecf(&sl,&source,cfg->lstm_pw_bias_fn[n],crf->lstm_pw[n]->bias);
        n = i*2+1;
        crf->lstm_pw[n] = wtk_nn_lstm_new(0,diml[0],diml[1],0.0f,NULL,NULL);
        wtk_mer_source_loader_load_matf(&sl,&source,cfg->lstm_pw_weight_fn[n],crf->lstm_pw[n]->kernel);
        wtk_mer_source_loader_load_vecf(&sl,&source,cfg->lstm_pw_bias_fn[n],crf->lstm_pw[n]->bias);
        diml+=2;
    }
    arr = cfg->linear_pw_dim;
    if(NULL == arr || arr->nslot != 2){
        wtk_debug("get linear pw error\n");
        goto end;
    }
    diml = arr->slot;
    crf->linear_pw_weight = wtk_matf_new(diml[0],diml[1]);
    wtk_mer_source_loader_load_matf(&sl,&source,cfg->linear_pw_weight_fn,crf->linear_pw_weight);
    crf->linear_pw_bias = wtk_vecf_new(diml[0]);
    wtk_mer_source_loader_load_vecf(&sl,&source,cfg->linear_pw_bias_fn,crf->linear_pw_bias);

    crf->crf_pw_transitions = wtk_matf_new(cfg->feature_num,cfg->feature_num);
    wtk_mer_source_loader_load_matf(&sl,&source,cfg->crf_pw_transitions_fn,crf->crf_pw_transitions);
    arr = cfg->pw_pred_embedding_dim;
    diml = cfg->pw_pred_embedding_dim->slot;
    crf->pw_pred_embedding = wtk_matf_new(diml[0],diml[1]);
    wtk_mer_source_loader_load_matf(&sl,&source,cfg->pw_pred_embedding_fn,crf->pw_pred_embedding);
    ret = 0;
end:
    return ret;
}

int qtk_tts_ncrf_pw_new2(qtk_tts_ncrf_t *crf, wtk_rbin2_t* rbin)
{
    int ret = -1;
    qtk_tts_ncrf_cfg_t *cfg = crf->cfg;
    wtk_array_t *arr = NULL;
    int *diml = NULL,i = 0,n = 0;
    wtk_source_loader_t sl;
    wtk_source_t src;
    FILE *f;

    wtk_source_loader_init_file(&sl);
    arr = cfg->lstm_pw_dim;
    if(NULL == arr || arr->nslot != 2*cfg->lstm_pw_num){
        wtk_debug("get lstm pw error\n");
        goto end;
    }
    crf->lstm_pw = wtk_malloc(sizeof(wtk_nn_lstm_t*)*cfg->lstm_pw_num*2);
    diml = arr->slot;
    for(i=0;i<cfg->lstm_pw_num;++i){
        n = i*2;
        crf->lstm_pw[n] = wtk_nn_lstm_new(1,diml[0],diml[1],0.0f,NULL,NULL);
        if (rbin)
        {
    		f=wtk_rbin2_get_file(rbin, cfg->lstm_pw_weight_fn[n]);
    		wtk_source_init_fd(&src,f, ftell(f));
    	    wtk_mer_source_read_float(&src, crf->lstm_pw[n]->kernel->p, crf->lstm_pw[n]->kernel->row*crf->lstm_pw[n]->kernel->col, 1);
    		wtk_source_clean_fd(&src);
    		if (f)fclose(f);
        }else
        {
        	wtk_mer_source_loader_load_matf(&sl,&src,cfg->lstm_pw_weight_fn[n],crf->lstm_pw[n]->kernel);
        }

        if (rbin)
        {
    		f=wtk_rbin2_get_file(rbin, cfg->lstm_pw_bias_fn[n]);
    		wtk_source_init_fd(&src,f, ftell(f));
    	    wtk_mer_source_read_float(&src, crf->lstm_pw[n]->bias->p, crf->lstm_pw[n]->bias->len, 1);
    		wtk_source_clean_fd(&src);
    		if (f)fclose(f);
        }else
        {
        	wtk_mer_source_loader_load_vecf(&sl,&src,cfg->lstm_pw_bias_fn[n],crf->lstm_pw[n]->bias);
        }

        n = i*2+1;
        crf->lstm_pw[n] = wtk_nn_lstm_new(0,diml[0],diml[1],0.0f,NULL,NULL);
        if (rbin)
        {
    		f=wtk_rbin2_get_file(rbin, cfg->lstm_pw_weight_fn[n]);
    		wtk_source_init_fd(&src,f, ftell(f));
    	    wtk_mer_source_read_float(&src, crf->lstm_pw[n]->kernel->p, crf->lstm_pw[n]->kernel->row*crf->lstm_pw[n]->kernel->col, 1);
    		wtk_source_clean_fd(&src);
    		if (f)fclose(f);
        }else
        {
        	wtk_mer_source_loader_load_matf(&sl,&src,cfg->lstm_pw_weight_fn[n],crf->lstm_pw[n]->kernel);
        }

        if (rbin)
        {
    		f=wtk_rbin2_get_file(rbin, cfg->lstm_pw_bias_fn[n]);
    		wtk_source_init_fd(&src,f, ftell(f));
    	    wtk_mer_source_read_float(&src, crf->lstm_pw[n]->bias->p, crf->lstm_pw[n]->bias->len, 1);
    		wtk_source_clean_fd(&src);
    		if (f)fclose(f);
        }else
        {
        	wtk_mer_source_loader_load_vecf(&sl,&src,cfg->lstm_pw_bias_fn[n],crf->lstm_pw[n]->bias);
        }

        diml+=2;
    }
    arr = cfg->linear_pw_dim;
    if(NULL == arr || arr->nslot != 2){
        wtk_debug("get linear pw error\n");
        goto end;
    }
    diml = arr->slot;
    crf->linear_pw_weight = wtk_matf_new(diml[0],diml[1]);
    if (rbin)
    {
		f=wtk_rbin2_get_file(rbin, cfg->linear_pw_weight_fn);
		wtk_source_init_fd(&src,f, ftell(f));
	    wtk_mer_source_read_float(&src, crf->linear_pw_weight->p, crf->linear_pw_weight->row * crf->linear_pw_weight->col, 1);
		wtk_source_clean_fd(&src);
		if (f)fclose(f);
    }else
    {
    	wtk_mer_source_loader_load_matf(&sl,&src,cfg->linear_pw_weight_fn,crf->linear_pw_weight);
    }

    crf->linear_pw_bias = wtk_vecf_new(diml[0]);
    if (rbin)
    {
		f=wtk_rbin2_get_file(rbin, cfg->linear_pw_bias_fn);
		wtk_source_init_fd(&src,f, ftell(f));
	    wtk_mer_source_read_float(&src, crf->linear_pw_bias->p, crf->linear_pw_bias->len, 1);
		wtk_source_clean_fd(&src);
		if (f)fclose(f);
    }else
    {
    	wtk_mer_source_loader_load_vecf(&sl,&src,cfg->linear_pw_bias_fn,crf->linear_pw_bias);
    }


    crf->crf_pw_transitions = wtk_matf_new(cfg->feature_num,cfg->feature_num);
    if (rbin)
    {
		f=wtk_rbin2_get_file(rbin, cfg->crf_pw_transitions_fn);
		wtk_source_init_fd(&src,f, ftell(f));
	    wtk_mer_source_read_float(&src, crf->crf_pw_transitions->p, crf->crf_pw_transitions->row * crf->crf_pw_transitions->col, 1);
		wtk_source_clean_fd(&src);
		if (f)fclose(f);
    }else
    {
    	wtk_mer_source_loader_load_matf(&sl,&src,cfg->crf_pw_transitions_fn,crf->crf_pw_transitions);
    }

    arr = cfg->pw_pred_embedding_dim;
    diml = cfg->pw_pred_embedding_dim->slot;
    crf->pw_pred_embedding = wtk_matf_new(diml[0],diml[1]);
    if (rbin)
    {
		f=wtk_rbin2_get_file(rbin, cfg->pw_pred_embedding_fn);
		wtk_source_init_fd(&src,f, ftell(f));
	    wtk_mer_source_read_float(&src, crf->pw_pred_embedding->p, crf->pw_pred_embedding->row * crf->pw_pred_embedding->col, 1);
		wtk_source_clean_fd(&src);
		if (f)fclose(f);
    }else
    {
    	wtk_mer_source_loader_load_matf(&sl,&src,cfg->pw_pred_embedding_fn,crf->pw_pred_embedding);
    }

    ret = 0;
end:
    return ret;
}

int qtk_tts_ncrf_pp_new(qtk_tts_ncrf_t *crf)
{
    int ret = -1;
    qtk_tts_ncrf_cfg_t *cfg = crf->cfg;
    wtk_array_t *arr = NULL;
    int *diml = NULL,i = 0,n = 0;
    wtk_source_loader_t sl;
    wtk_source_t source;

    wtk_source_loader_init_file(&sl);
    arr = cfg->pp_fnn_dim;
    if(arr == NULL || arr->nslot != 2){
        wtk_debug("pp get dim error");
        goto end;
    }
    diml = arr->slot;
    crf->pp_fnn_weight = wtk_matf_new(diml[0],diml[1]);
    wtk_mer_source_loader_load_matf(&sl,&source,cfg->pp_fnn_weight_fn,crf->pp_fnn_weight);
    crf->pp_fnn_bias = wtk_vecf_new(diml[0]);
    wtk_mer_source_loader_load_vecf(&sl,&source,cfg->pp_fnn_bias_fn,crf->pp_fnn_bias);
    
    arr = cfg->lstm_pw_dim;
    if(NULL == arr || arr->nslot != 2*cfg->lstm_pp_num){
        wtk_debug("get lstm pp error\n");
        goto end;
    }
    crf->lstm_pp = wtk_malloc(sizeof(wtk_nn_lstm_t*)*cfg->lstm_pp_num*2);
    diml = arr->slot;
    for(i=0;i<cfg->lstm_pp_num;++i){
        n = i*2;
        crf->lstm_pp[n] = wtk_nn_lstm_new(1,diml[0],diml[1],0.0f,NULL,NULL);
        wtk_mer_source_loader_load_matf(&sl,&source,cfg->lstm_pp_weight_fn[n],crf->lstm_pp[n]->kernel);
        wtk_mer_source_loader_load_vecf(&sl,&source,cfg->lstm_pp_bias_fn[n],crf->lstm_pp[n]->bias);
        n = i*2+1;
        crf->lstm_pp[n] = wtk_nn_lstm_new(0,diml[0],diml[1],0.0f,NULL,NULL);
        wtk_mer_source_loader_load_matf(&sl,&source,cfg->lstm_pp_weight_fn[n],crf->lstm_pp[n]->kernel);
        wtk_mer_source_loader_load_vecf(&sl,&source,cfg->lstm_pp_bias_fn[n],crf->lstm_pp[n]->bias);
        diml+=2;
    }
    arr = cfg->linear_pp_dim;
    if(NULL == arr || arr->nslot != 2){
        wtk_debug("get linear pp error\n");
        goto end;
    }
    diml = arr->slot;
    crf->linear_pp_weight = wtk_matf_new(diml[0],diml[1]);
    wtk_mer_source_loader_load_matf(&sl,&source,cfg->linear_pp_weight_fn,crf->linear_pp_weight);
    crf->linear_pp_bias = wtk_vecf_new(diml[0]);
    wtk_mer_source_loader_load_vecf(&sl,&source,cfg->linear_pp_bias_fn,crf->linear_pp_bias);

    crf->crf_pp_transitions = wtk_matf_new(cfg->feature_num,cfg->feature_num);
    wtk_mer_source_loader_load_matf(&sl,&source,cfg->crf_pp_transitions_fn,crf->crf_pp_transitions);
    arr = cfg->pp_pred_embedding_dim;
    diml = cfg->pp_pred_embedding_dim->slot;
    crf->pp_pred_embedding = wtk_matf_new(diml[0],diml[1]);
    wtk_mer_source_loader_load_matf(&sl,&source,cfg->pp_pred_embedding_fn,crf->pp_pred_embedding);
    ret = 0;
end:
    return ret;
}

int qtk_tts_ncrf_pp_new2(qtk_tts_ncrf_t *crf, wtk_rbin2_t* rbin)
{
    int ret = -1;
    qtk_tts_ncrf_cfg_t *cfg = crf->cfg;
    wtk_array_t *arr = NULL;
    int *diml = NULL,i = 0,n = 0;
    wtk_source_loader_t sl;
    wtk_source_t src;
    FILE *f;

    wtk_source_loader_init_file(&sl);
    arr = cfg->pp_fnn_dim;
    if(arr == NULL || arr->nslot != 2){
        wtk_debug("pp get dim error");
        goto end;
    }
    diml = arr->slot;
    crf->pp_fnn_weight = wtk_matf_new(diml[0],diml[1]);
    if (rbin)
    {
		f=wtk_rbin2_get_file(rbin, cfg->pp_fnn_weight_fn);
		wtk_source_init_fd(&src,f, ftell(f));
	    wtk_mer_source_read_float(&src, crf->pp_fnn_weight->p, crf->pp_fnn_weight->row * crf->pp_fnn_weight->col, 1);
		wtk_source_clean_fd(&src);
		if (f)fclose(f);
    }else
    {
    	wtk_mer_source_loader_load_matf(&sl,&src,cfg->pp_fnn_weight_fn,crf->pp_fnn_weight);
    }

    crf->pp_fnn_bias = wtk_vecf_new(diml[0]);
    if (rbin)
    {
		f=wtk_rbin2_get_file(rbin, cfg->pp_fnn_bias_fn);
		wtk_source_init_fd(&src,f, ftell(f));
	    wtk_mer_source_read_float(&src, crf->pp_fnn_bias->p, crf->pp_fnn_bias->len, 1);
		wtk_source_clean_fd(&src);
		if (f)fclose(f);
    }else
    {
    	wtk_mer_source_loader_load_vecf(&sl,&src,cfg->pp_fnn_bias_fn,crf->pp_fnn_bias);
    }

    arr = cfg->lstm_pw_dim;
    if(NULL == arr || arr->nslot != 2*cfg->lstm_pp_num){
        wtk_debug("get lstm pp error\n");
        goto end;
    }
    crf->lstm_pp = wtk_malloc(sizeof(wtk_nn_lstm_t*)*cfg->lstm_pp_num*2);
    diml = arr->slot;
    for(i=0;i<cfg->lstm_pp_num;++i){
        n = i*2;
        crf->lstm_pp[n] = wtk_nn_lstm_new(1,diml[0],diml[1],0.0f,NULL,NULL);
        if (rbin)
        {
    		f=wtk_rbin2_get_file(rbin, cfg->lstm_pp_weight_fn[n]);
    		wtk_source_init_fd(&src,f, ftell(f));
    	    wtk_mer_source_read_float(&src, crf->lstm_pp[n]->kernel->p, crf->lstm_pp[n]->kernel->row * crf->lstm_pp[n]->kernel->col, 1);
    		wtk_source_clean_fd(&src);
    		if (f)fclose(f);
        }else
        {
        	wtk_mer_source_loader_load_matf(&sl,&src,cfg->lstm_pp_weight_fn[n],crf->lstm_pp[n]->kernel);
        }

        if (rbin)
        {
    		f=wtk_rbin2_get_file(rbin, cfg->lstm_pp_bias_fn[n]);
    		wtk_source_init_fd(&src,f, ftell(f));
    	    wtk_mer_source_read_float(&src, crf->lstm_pp[n]->bias->p, crf->lstm_pp[n]->bias->len, 1);
    		wtk_source_clean_fd(&src);
    		if (f)fclose(f);
        }else
        {
        	wtk_mer_source_loader_load_vecf(&sl,&src,cfg->lstm_pp_bias_fn[n],crf->lstm_pp[n]->bias);
        }

        n = i*2+1;
        crf->lstm_pp[n] = wtk_nn_lstm_new(0,diml[0],diml[1],0.0f,NULL,NULL);
        if (rbin)
        {
    		f=wtk_rbin2_get_file(rbin, cfg->lstm_pp_weight_fn[n]);
    		wtk_source_init_fd(&src,f, ftell(f));
    	    wtk_mer_source_read_float(&src, crf->lstm_pp[n]->kernel->p, crf->lstm_pp[n]->kernel->row * crf->lstm_pp[n]->kernel->col, 1);
    		wtk_source_clean_fd(&src);
    		if (f)fclose(f);
        }else
        {
        	wtk_mer_source_loader_load_matf(&sl,&src,cfg->lstm_pp_weight_fn[n],crf->lstm_pp[n]->kernel);
        }

        if (rbin)
        {
    		f=wtk_rbin2_get_file(rbin, cfg->lstm_pp_bias_fn[n]);
    		wtk_source_init_fd(&src,f, ftell(f));
    	    wtk_mer_source_read_float(&src, crf->lstm_pp[n]->bias->p, crf->lstm_pp[n]->bias->len, 1);
    		wtk_source_clean_fd(&src);
    		if (f)fclose(f);
        }else
        {
        	wtk_mer_source_loader_load_vecf(&sl,&src,cfg->lstm_pp_bias_fn[n],crf->lstm_pp[n]->bias);
        }

        diml+=2;
    }
    arr = cfg->linear_pp_dim;
    if(NULL == arr || arr->nslot != 2){
        wtk_debug("get linear pp error\n");
        goto end;
    }
    diml = arr->slot;
    crf->linear_pp_weight = wtk_matf_new(diml[0],diml[1]);
    if (rbin)
    {
		f=wtk_rbin2_get_file(rbin, cfg->linear_pp_weight_fn);
		wtk_source_init_fd(&src,f, ftell(f));
	    wtk_mer_source_read_float(&src, crf->linear_pp_weight->p, crf->linear_pp_weight->row * crf->linear_pp_weight->col, 1);
		wtk_source_clean_fd(&src);
		if (f)fclose(f);
    }else
    {
    	wtk_mer_source_loader_load_matf(&sl,&src,cfg->linear_pp_weight_fn,crf->linear_pp_weight);
    }

    crf->linear_pp_bias = wtk_vecf_new(diml[0]);
    if (rbin)
    {
		f=wtk_rbin2_get_file(rbin, cfg->linear_pp_bias_fn);
		wtk_source_init_fd(&src,f, ftell(f));
	    wtk_mer_source_read_float(&src, crf->linear_pp_bias->p, crf->linear_pp_bias->len, 1);
		wtk_source_clean_fd(&src);
		if (f)fclose(f);
    }else
    {
    	wtk_mer_source_loader_load_vecf(&sl,&src,cfg->linear_pp_bias_fn,crf->linear_pp_bias);
    }

    crf->crf_pp_transitions = wtk_matf_new(cfg->feature_num,cfg->feature_num);
    if (rbin)
    {
		f=wtk_rbin2_get_file(rbin, cfg->crf_pp_transitions_fn);
		wtk_source_init_fd(&src,f, ftell(f));
	    wtk_mer_source_read_float(&src, crf->crf_pp_transitions->p, crf->crf_pp_transitions->row * crf->crf_pp_transitions->col, 1);
		wtk_source_clean_fd(&src);
		if (f)fclose(f);
    }else
    {
    	wtk_mer_source_loader_load_matf(&sl,&src,cfg->crf_pp_transitions_fn,crf->crf_pp_transitions);
    }

    arr = cfg->pp_pred_embedding_dim;
    diml = cfg->pp_pred_embedding_dim->slot;
    crf->pp_pred_embedding = wtk_matf_new(diml[0],diml[1]);
    if (rbin)
    {
		f=wtk_rbin2_get_file(rbin, cfg->pp_pred_embedding_fn);
		wtk_source_init_fd(&src,f, ftell(f));
	    wtk_mer_source_read_float(&src, crf->pp_pred_embedding->p, crf->pp_pred_embedding->row * crf->pp_pred_embedding->col, 1);
		wtk_source_clean_fd(&src);
		if (f)fclose(f);
    }else
    {
    	wtk_mer_source_loader_load_matf(&sl,&src,cfg->pp_pred_embedding_fn,crf->pp_pred_embedding);
    }

    ret = 0;
end:
    return ret;
}

int qtk_tts_ncrf_ip_new(qtk_tts_ncrf_t *crf)
{
    int ret = -1;
    qtk_tts_ncrf_cfg_t *cfg = crf->cfg;
    wtk_array_t *arr = NULL;
    int *diml = NULL,i = 0,n = 0;
    wtk_source_loader_t sl;
    wtk_source_t source;

    wtk_source_loader_init_file(&sl);
    arr = cfg->ip_fnn_dim;
    if(arr == NULL || arr->nslot != 2){
        wtk_debug("ip get dim error");
        goto end;
    }
    diml = arr->slot;
    crf->ip_fnn_weight = wtk_matf_new(diml[0],diml[1]);
    wtk_mer_source_loader_load_matf(&sl,&source,cfg->ip_fnn_weight_fn,crf->ip_fnn_weight);
    crf->ip_fnn_bias = wtk_vecf_new(diml[0]);
    wtk_mer_source_loader_load_vecf(&sl,&source,cfg->ip_fnn_bias_fn,crf->ip_fnn_bias);
    
    arr = cfg->lstm_pw_dim;
    if(NULL == arr || arr->nslot != 2*cfg->lstm_ip_num){
        wtk_debug("get lstm ip error\n");
        goto end;
    }
    crf->lstm_ip = wtk_malloc(sizeof(wtk_nn_lstm_t*)*cfg->lstm_ip_num*2);
    diml = arr->slot;
    for(i=0;i<cfg->lstm_ip_num;++i){
        n = i*2;
        crf->lstm_ip[n] = wtk_nn_lstm_new(1,diml[0],diml[1],0.0f,NULL,NULL);
        wtk_mer_source_loader_load_matf(&sl,&source,cfg->lstm_ip_weight_fn[n],crf->lstm_ip[n]->kernel);
        wtk_mer_source_loader_load_vecf(&sl,&source,cfg->lstm_ip_bias_fn[n],crf->lstm_ip[n]->bias);
        n = i*2+1;
        crf->lstm_ip[n] = wtk_nn_lstm_new(0,diml[0],diml[1],0.0f,NULL,NULL);
        wtk_mer_source_loader_load_matf(&sl,&source,cfg->lstm_ip_weight_fn[n],crf->lstm_ip[n]->kernel);
        wtk_mer_source_loader_load_vecf(&sl,&source,cfg->lstm_ip_bias_fn[n],crf->lstm_ip[n]->bias);
        diml+=2;
    }
    arr = cfg->linear_ip_dim;
    if(NULL == arr || arr->nslot != 2){
        wtk_debug("get linear ip error\n");
        goto end;
    }
    diml = arr->slot;
    crf->linear_ip_weight = wtk_matf_new(diml[0],diml[1]);
    wtk_mer_source_loader_load_matf(&sl,&source,cfg->linear_ip_weight_fn,crf->linear_ip_weight);
    crf->linear_ip_bias = wtk_vecf_new(diml[0]);
    wtk_mer_source_loader_load_vecf(&sl,&source,cfg->linear_ip_bias_fn,crf->linear_ip_bias);

    crf->crf_ip_transitions = wtk_matf_new(cfg->feature_num,cfg->feature_num);
    wtk_mer_source_loader_load_matf(&sl,&source,cfg->crf_ip_transitions_fn,crf->crf_ip_transitions);

    ret = 0;
end:
    return ret;
}

int qtk_tts_ncrf_ip_new2(qtk_tts_ncrf_t *crf, wtk_rbin2_t* rbin)
{
    int ret = -1;
    qtk_tts_ncrf_cfg_t *cfg = crf->cfg;
    wtk_array_t *arr = NULL;
    int *diml = NULL,i = 0,n = 0;
    wtk_source_loader_t sl;
    wtk_source_t src;
    FILE *f;

    wtk_source_loader_init_file(&sl);
    arr = cfg->ip_fnn_dim;
    if(arr == NULL || arr->nslot != 2){
        wtk_debug("ip get dim error");
        goto end;
    }
    diml = arr->slot;
    crf->ip_fnn_weight = wtk_matf_new(diml[0],diml[1]);
    if (rbin)
    {
		f=wtk_rbin2_get_file(rbin, cfg->ip_fnn_weight_fn);
		wtk_source_init_fd(&src,f, ftell(f));
	    wtk_mer_source_read_float(&src, crf->ip_fnn_weight->p, crf->ip_fnn_weight->row * crf->ip_fnn_weight->col, 1);
		wtk_source_clean_fd(&src);
		if (f)fclose(f);
    }else
    {
    	wtk_mer_source_loader_load_matf(&sl,&src,cfg->ip_fnn_weight_fn,crf->ip_fnn_weight);
    }

    crf->ip_fnn_bias = wtk_vecf_new(diml[0]);
    if (rbin)
    {
		f=wtk_rbin2_get_file(rbin, cfg->ip_fnn_bias_fn);
		wtk_source_init_fd(&src,f, ftell(f));
	    wtk_mer_source_read_float(&src, crf->ip_fnn_bias->p, crf->ip_fnn_bias->len, 1);
		wtk_source_clean_fd(&src);
		if (f)fclose(f);
    }else
    {
    	wtk_mer_source_loader_load_vecf(&sl,&src,cfg->ip_fnn_bias_fn,crf->ip_fnn_bias);
    }

    arr = cfg->lstm_pw_dim;
    if(NULL == arr || arr->nslot != 2*cfg->lstm_ip_num){
        wtk_debug("get lstm ip error\n");
        goto end;
    }
    crf->lstm_ip = wtk_malloc(sizeof(wtk_nn_lstm_t*)*cfg->lstm_ip_num*2);
    diml = arr->slot;
    for(i=0;i<cfg->lstm_ip_num;++i){
        n = i*2;
        crf->lstm_ip[n] = wtk_nn_lstm_new(1,diml[0],diml[1],0.0f,NULL,NULL);
        if (rbin)
        {
    		f=wtk_rbin2_get_file(rbin, cfg->lstm_ip_weight_fn[n]);
    		wtk_source_init_fd(&src,f, ftell(f));
    	    wtk_mer_source_read_float(&src, crf->lstm_ip[n]->kernel->p, crf->lstm_ip[n]->kernel->row * crf->lstm_ip[n]->kernel->col, 1);
    		wtk_source_clean_fd(&src);
    		if (f)fclose(f);
        }else
        {
        	wtk_mer_source_loader_load_matf(&sl,&src,cfg->lstm_ip_weight_fn[n],crf->lstm_ip[n]->kernel);
        }

        if (rbin)
        {
    		f=wtk_rbin2_get_file(rbin, cfg->lstm_ip_bias_fn[n]);
    		wtk_source_init_fd(&src,f, ftell(f));
    	    wtk_mer_source_read_float(&src, crf->lstm_ip[n]->bias->p, crf->lstm_ip[n]->bias->len, 1);
    		wtk_source_clean_fd(&src);
    		if (f)fclose(f);
        }else
        {
        	wtk_mer_source_loader_load_vecf(&sl,&src,cfg->lstm_ip_bias_fn[n],crf->lstm_ip[n]->bias);
        }

        n = i*2+1;
        crf->lstm_ip[n] = wtk_nn_lstm_new(0,diml[0],diml[1],0.0f,NULL,NULL);
        if (rbin)
        {
    		f=wtk_rbin2_get_file(rbin, cfg->lstm_ip_weight_fn[n]);
    		wtk_source_init_fd(&src,f, ftell(f));
    	    wtk_mer_source_read_float(&src, crf->lstm_ip[n]->kernel->p, crf->lstm_ip[n]->kernel->row * crf->lstm_ip[n]->kernel->col, 1);
    		wtk_source_clean_fd(&src);
    		if (f)fclose(f);
        }else
        {
        	wtk_mer_source_loader_load_matf(&sl,&src,cfg->lstm_ip_weight_fn[n],crf->lstm_ip[n]->kernel);
        }

        if (rbin)
        {
    		f=wtk_rbin2_get_file(rbin, cfg->lstm_ip_bias_fn[n]);
    		wtk_source_init_fd(&src,f, ftell(f));
    	    wtk_mer_source_read_float(&src, crf->lstm_ip[n]->bias->p, crf->lstm_ip[n]->bias->len, 1);
    		wtk_source_clean_fd(&src);
    		if (f)fclose(f);
        }else
        {
        	wtk_mer_source_loader_load_vecf(&sl,&src,cfg->lstm_ip_bias_fn[n],crf->lstm_ip[n]->bias);
        }

        diml+=2;
    }
    arr = cfg->linear_ip_dim;
    if(NULL == arr || arr->nslot != 2){
        wtk_debug("get linear ip error\n");
        goto end;
    }
    diml = arr->slot;
    crf->linear_ip_weight = wtk_matf_new(diml[0],diml[1]);
    if (rbin)
    {
		f=wtk_rbin2_get_file(rbin, cfg->linear_ip_weight_fn);
		wtk_source_init_fd(&src,f, ftell(f));
	    wtk_mer_source_read_float(&src, crf->linear_ip_weight->p, crf->linear_ip_weight->row * crf->linear_ip_weight->col, 1);
		wtk_source_clean_fd(&src);
		if (f)fclose(f);
    }else
    {
    	wtk_mer_source_loader_load_matf(&sl,&src,cfg->linear_ip_weight_fn,crf->linear_ip_weight);
    }

    crf->linear_ip_bias = wtk_vecf_new(diml[0]);
    if (rbin)
    {
		f=wtk_rbin2_get_file(rbin, cfg->linear_ip_bias_fn);
		wtk_source_init_fd(&src,f, ftell(f));
	    wtk_mer_source_read_float(&src, crf->linear_ip_bias->p, crf->linear_ip_bias->len, 1);
		wtk_source_clean_fd(&src);
		if (f)fclose(f);
    }else
    {
    	wtk_mer_source_loader_load_vecf(&sl,&src,cfg->linear_ip_bias_fn,crf->linear_ip_bias);
    }

    crf->crf_ip_transitions = wtk_matf_new(cfg->feature_num,cfg->feature_num);
    if (rbin)
    {
		f=wtk_rbin2_get_file(rbin, cfg->crf_ip_transitions_fn);
		wtk_source_init_fd(&src,f, ftell(f));
	    wtk_mer_source_read_float(&src, crf->crf_ip_transitions->p,crf->crf_ip_transitions->row * crf->crf_ip_transitions->col, 1);
		wtk_source_clean_fd(&src);
		if (f)fclose(f);
    }else
    {
    	wtk_mer_source_loader_load_matf(&sl,&src,cfg->crf_ip_transitions_fn,crf->crf_ip_transitions);
    }

    ret = 0;
end:
    return ret;
}

int qtk_tts_ncrf_charrep_embedding(qtk_tts_ncrf_t *ncrf,wtk_mati_t *in,wtk_matf_t *emb_out)
{
    int ret = 0, row = in->row, i = 0,j = 0;
    int featurn_num = ncrf->cfg->feature_num;
    float *to = NULL,*src = NULL;
    int *p = NULL;
    wtk_matf_t *char_embedding = ncrf->char_embedding;
    // wtk_mer_matf_shape_print(in);
    for(i = 0; i < row; ++i){
        p = wtk_matf_at(in,i,0);    //wtk_mati_at()only get value，no address
        to = wtk_matf_at(emb_out,i,0);
        src = wtk_matf_at(char_embedding,p[0],0);
        memcpy(to,src,char_embedding->col*sizeof(float));
        to+=char_embedding->col;
        p+=1;
        for(j = 0; j < featurn_num; ++j){
            src = wtk_matf_at(ncrf->feature_embedding[j],p[0],0);
            memcpy(to,src,ncrf->feature_embedding[j]->col * sizeof(float));
            to+=ncrf->feature_embedding[j]->col;
            p+=1;
        }
    }
    return ret;
}

int qtk_tts_ncrf_charrep(qtk_tts_ncrf_t *ncrf,wtk_mati_t *in,wtk_matf_t *out)
{
    int ret = -1, i = 0;
    int featurn_num = ncrf->cfg->feature_num;
    wtk_matf_t *embedding_out = NULL,*linear_out1=NULL;
    wtk_heap_t *heap = NULL;
    int embedding_row = in->row,embedding_col = 0;

    heap = ncrf->uheap;
    //ncrf embedding
    embedding_col += ncrf->char_embedding->col;
    for(i = 0; i < featurn_num; ++i){
        embedding_col += ncrf->feature_embedding[i]->col;
    }
    embedding_out = wtk_matf_heap_new(heap,embedding_row,embedding_col);
    qtk_tts_ncrf_charrep_embedding(ncrf,in,embedding_out);
    linear_out1 = wtk_matf_heap_new(heap,embedding_out->row,ncrf->linear_weight[0]->row);
    wtk_nn_layer_dense(embedding_out,ncrf->linear_weight[0],ncrf->linear_bias[0],wtk_nn_relu,linear_out1);
    wtk_nn_layer_dense(linear_out1,ncrf->linear_weight[1],ncrf->linear_bias[1],wtk_nn_relu,out);

    ret = 0;
    return ret;
}

int qtk_tts_ncrf_model_pw(qtk_tts_ncrf_t *ncrf,wtk_matf_t *input,wtk_matf_t*output)
{
    int ret = 0,num = 0,i = 0,j = 0;
    wtk_heap_t *heap = ncrf->uheap;
    wtk_nn_lstm_t **lstm = ncrf->lstm_pw;
    wtk_matf_t *moutf = NULL,*moutr = NULL,*mout = NULL;
    int row = input->row;
    float *fp = NULL,*rp = NULL,*op = NULL;

    num = ncrf->cfg->lstm_pw_num*2;
    moutf = wtk_matf_heap_new(heap,row,input->col/2);
    moutr = wtk_matf_heap_new(heap,row,input->col/2);
    mout = wtk_matf_heap_new(heap,row,input->col);
    for(i = 0; i < num; ++i){
        wtk_nn_lstm_reset(lstm[i]);
        wtk_nn_lstm(lstm[i],input,moutf);
        ++i;
        wtk_nn_lstm_reset(lstm[i]);
        wtk_nn_lstm(lstm[i],input,moutr);
        op = mout->p;
        fp = moutf->p;
        rp = moutr->p;
        for(j = 0; j < row;++j){
            memcpy(op,fp,moutf->col*sizeof(float));
            memcpy(op+moutf->col,rp,moutr->col*sizeof(float));
            fp+=moutf->col;
            rp+=moutr->col;
            op += mout->col;
        }
        input = mout;
    }
    wtk_nn_layer_dense(mout,ncrf->linear_pw_weight,ncrf->linear_pw_bias,NULL,output);
    return ret;
}

int qtk_tts_ncrf_model_pp(qtk_tts_ncrf_t *ncrf,wtk_matf_t *input,wtk_matf_t*output)
{
    int ret = 0,num = 0,i = 0,j = 0;
    wtk_heap_t *heap = ncrf->uheap;
    wtk_nn_lstm_t **lstm = ncrf->lstm_pp;
    wtk_matf_t *moutf = NULL,*moutr = NULL,*mout = NULL;
    int row = input->row;
    float *fp = NULL,*rp = NULL,*op = NULL;

    num = ncrf->cfg->lstm_pp_num*2;
    moutf = wtk_matf_heap_new(heap,row,input->col/2);
    moutr = wtk_matf_heap_new(heap,row,input->col/2);
    mout = wtk_matf_heap_new(heap,row,input->col);
    for(i = 0; i < num; ++i){
        wtk_nn_lstm_reset(lstm[i]);
        wtk_nn_lstm(lstm[i],input,moutf);
        ++i;
        wtk_nn_lstm_reset(lstm[i]);
        wtk_nn_lstm(lstm[i],input,moutr);
        op = mout->p;
        fp = moutf->p;
        rp = moutr->p;
        for(j = 0; j < row;++j){
            memcpy(op,fp,moutf->col*sizeof(float));
            memcpy(op+moutf->col,rp,moutr->col*sizeof(float));
            fp+=moutf->col;
            rp+=moutr->col;
            op += mout->col;
        }
        input = mout;
    }
    wtk_nn_layer_dense(mout,ncrf->linear_pp_weight,ncrf->linear_pp_bias,NULL,output);
    return ret;
}

int qtk_tts_ncrf_model_ip(qtk_tts_ncrf_t *ncrf,wtk_matf_t *input,wtk_matf_t*output)
{
    int ret = 0,num = 0,i = 0,j = 0;
    wtk_heap_t *heap = ncrf->uheap;
    wtk_nn_lstm_t **lstm = ncrf->lstm_ip;
    wtk_matf_t *moutf = NULL,*moutr = NULL,*mout = NULL;
    int row = input->row;
    float *fp = NULL,*rp = NULL,*op = NULL;

    num = ncrf->cfg->lstm_ip_num*2;
    moutf = wtk_matf_heap_new(heap,row,input->col/2);
    moutr = wtk_matf_heap_new(heap,row,input->col/2);
    mout = wtk_matf_heap_new(heap,row,input->col);
    for(i = 0; i < num; ++i){
        wtk_nn_lstm_reset(lstm[i]);
        wtk_nn_lstm(lstm[i],input,moutf);
        ++i;
        wtk_nn_lstm_reset(lstm[i]);
        wtk_nn_lstm(lstm[i],input,moutr);
        op = mout->p;
        fp = moutf->p;
        rp = moutr->p;
        for(j = 0; j < row;++j){
            memcpy(op,fp,moutf->col*sizeof(float));
            memcpy(op+moutf->col,rp,moutr->col*sizeof(float));
            fp+=moutf->col;
            rp+=moutr->col;
            op += mout->col;
        }
        input = mout;
    }
    wtk_nn_layer_dense(mout,ncrf->linear_ip_weight,ncrf->linear_ip_bias,NULL,output);
    return ret;
}

int qtk_tts_ncrf_viterbi_decode(qtk_tts_ncrf_t *ncrf,wtk_matf_t *tran,wtk_matf_t *in,wtk_mati_t *out)
{
    int ret = 0,i = 0,j = 0,n = 0;
    // wtk_matf_t *tran = ncrf->crf_pw_transitions;
    wtk_heap_t *heap = ncrf->uheap;
    wtk_matf_t *scores = NULL,*in_all = NULL,*tran_all = NULL,
                            *tmp = NULL,*partition_history = NULL;
    float *sp = NULL,*ip = NULL,*tp = NULL;
    int **history = NULL,stop_point = 0,*dp = NULL;

    scores = wtk_matf_heap_new(heap,in->row,in->col*in->col);
    in_all = wtk_matf_heap_new(heap,in->row,in->col*in->col);
    tran_all = wtk_matf_heap_new(heap,in->row,tran->row*tran->col);
    history = wtk_heap_malloc(heap,sizeof(int*)*in->row);
    partition_history = wtk_matf_heap_new(heap,in->row+1,in->col);
    
    for(i = 0; i < in->row; ++i){
        history[i] = wtk_heap_malloc(heap,sizeof(int)*in->col);
    }

    for(i = 0; i < in->row;++i){
        sp = wtk_matf_at(in_all,i,0);
        ip = wtk_matf_at(in,i,0);
        for(j = 0; j < in->col;++j){
            memcpy(sp,ip,sizeof(float)*in->col);
            sp+=in->col;
        }
    }
    ip = tran->p;
    tp = tran_all->p;
    n = tran->col*tran->row;
    for(i = 0; i < in->row; ++i){
        memcpy(tp,ip,sizeof(float)*n);
        tp+=n;
    }
    wtk_matf_add(scores,in_all,tran_all);
    //tmp 作为暂时存储的内存 start_decoder
    tmp = wtk_matf_heap_new(heap,in->col,in->col);
    sp = tmp->p;
    ip = wtk_matf_at(scores,0,scores->col-in->col*2);   //开始的时候取第一层的倒数第二个作为第一层的选择概率
    for(i = 0; i < in->col;++i){
        for(j = 0; j < in->col;++j){
            *sp = ip[i];
            sp+=1;
        }
    }
    memcpy(partition_history->p,ip,sizeof(float)*in->col);
    int num = tmp->col*tmp->row;
    for(i = 1; i < in->row; ++i){
        ip = wtk_matf_at(scores,i,0);
        wtk_float_add(tmp->p,ip,num);
        ip = wtk_matf_at(partition_history,i,0);
        qtk_tts_ncrf_viterbi_decode_max(ncrf,tmp,ip,history[i-1]);
        sp = tmp->p;
        for(j = 0; j < in->col; ++j){
            for(n = 0; n < in->col; ++n){
                *sp = ip[j];
                sp+=1;
            }
        }
    }
    //end point
    ip = tran->p;
    wtk_float_add(tmp->p,ip,num);
    ip = wtk_matf_at(partition_history,in->row,0);
    qtk_tts_ncrf_viterbi_decode_max(ncrf,tmp,ip,history[in->row-1]);
    stop_point = history[in->row-1][in->col-1]; //根据python 选择最后一个数字 ??不知道为啥
    for(i = 0; i < in->col; ++i){
        history[in->row-1][i] = stop_point;
    }
    //回溯
    dp = out->p;
    dp[out->col-1] = stop_point;
    for(i = in->row-2;i > -1; i--){
        stop_point = history[i][stop_point];
        dp[i] = stop_point;
    }
    ret=0;
    return ret;
}

void qtk_tts_ncrf_pred_embedding(wtk_mati_t *in,wtk_matf_t *embedding,wtk_matf_t *out)
{
    int irow = in->row,icol = in->col,n = 0;
    int embedding_col = embedding->col;
    int i = 0;
    float *sp = NULL,*ep = NULL;
    int *ip = NULL;
    n = irow*icol;
    ip = in->p;
    sp = out->p;
    for(i = 0; i < n;++i){
        ep = wtk_matf_at(embedding,ip[i],0);
        memcpy(sp,ep,sizeof(float)*embedding_col);
        sp+=embedding_col;
    }

    return;
}

void qtk_tts_ncrf_viterbi_decode_max(qtk_tts_ncrf_t *ncrf,wtk_matf_t *in,float *out,int *lists)
{
    int irow = in->row,i = 0;
    int icol = in->col,j = 0;
    float *p=NULL,max = 0;
    int maxi;
    
    for(i = 0; i < icol; ++i){
        p = wtk_matf_at(in,0,i);
        max = p[0];
        maxi = 0;
        for(j = 0; j < irow; ++j){
                if(*p>max){
                    max = *p;
                    maxi = j;
                }
                p+=icol;
        }
        out[i] = max;
        lists[i] = maxi;
    }
    return;
}

void qtk_tts_ncrf_charrep_delete(qtk_tts_ncrf_t *ncrf)
{
    int i = 0;
    
    if(ncrf->char_embedding) wtk_matf_delete(ncrf->char_embedding);
    if(ncrf->feature_embedding){
        for(i = 0;i < ncrf->cfg->feature_num; ++i){
            if(ncrf->feature_embedding[i]) wtk_matf_delete(ncrf->feature_embedding[i]);
        }
        wtk_free(ncrf->feature_embedding);
    }
    if(ncrf->linear_bias && ncrf->linear_weight){
        for(i =0; i < ncrf->cfg->linear_num;++i){
            if(ncrf->linear_weight[i]) wtk_matf_delete(ncrf->linear_weight[i]);
            if(ncrf->linear_bias[i]) wtk_vecf_delete(ncrf->linear_bias[i]);
        }
        wtk_free(ncrf->linear_bias);
        wtk_free(ncrf->linear_weight);
    }
    return;
}

void qtk_tts_ncrf_pw_delete(qtk_tts_ncrf_t *ncrf)
{
    int i = 0;

    if(ncrf->lstm_pw){
        for(i = 0; i < ncrf->cfg->lstm_pw_num*2;++i){
            wtk_nn_lstm_delete(ncrf->lstm_pw[i]);
        }
        wtk_free(ncrf->lstm_pw);
    }
    if(ncrf->linear_pw_weight)  wtk_matf_delete(ncrf->linear_pw_weight);
    if(ncrf->linear_pw_bias)    wtk_vecf_delete(ncrf->linear_pw_bias);
    if(ncrf->crf_pw_transitions)    wtk_matf_delete(ncrf->crf_pw_transitions);
    if(ncrf->pw_pred_embedding)     wtk_matf_delete(ncrf->pw_pred_embedding);
    return;
}

void qtk_tts_ncrf_pp_delete(qtk_tts_ncrf_t *ncrf)
{
    int i = 0;

    if(ncrf->pp_fnn_weight) wtk_matf_delete(ncrf->pp_fnn_weight);
    if(ncrf->pp_fnn_bias) wtk_vecf_delete(ncrf->pp_fnn_bias);
    if(ncrf->lstm_pp){
        for(i = 0; i < ncrf->cfg->lstm_pp_num*2;++i){
            wtk_nn_lstm_delete(ncrf->lstm_pp[i]);
        }
        wtk_free(ncrf->lstm_pp);
    }
    if(ncrf->linear_pp_weight)  wtk_matf_delete(ncrf->linear_pp_weight);
    if(ncrf->linear_pp_bias)    wtk_vecf_delete(ncrf->linear_pp_bias);
    if(ncrf->crf_pp_transitions)    wtk_matf_delete(ncrf->crf_pp_transitions);
    if(ncrf->pp_pred_embedding)     wtk_matf_delete(ncrf->pp_pred_embedding);
    return;
}

void qtk_tts_ncrf_ip_delete(qtk_tts_ncrf_t *ncrf)
{
    int i = 0;

    if(ncrf->ip_fnn_weight) wtk_matf_delete(ncrf->ip_fnn_weight);
    if(ncrf->ip_fnn_bias) wtk_vecf_delete(ncrf->ip_fnn_bias);
    if(ncrf->lstm_ip){
        for(i = 0; i < ncrf->cfg->lstm_ip_num*2;++i){
            wtk_nn_lstm_delete(ncrf->lstm_ip[i]);
        }
        wtk_free(ncrf->lstm_ip);
    }
    if(ncrf->linear_ip_weight)  wtk_matf_delete(ncrf->linear_ip_weight);
    if(ncrf->linear_ip_bias)    wtk_vecf_delete(ncrf->linear_ip_bias);
    if(ncrf->crf_ip_transitions)    wtk_matf_delete(ncrf->crf_ip_transitions);
    return;
}

void qtk_tts_ncrf_fnn_pp_cat(wtk_matf_t *in1,wtk_matf_t *in2,wtk_matf_t *out)
{
    int i = 0,row = in1->row,col1 = in1->col,col2 = in2->col;
    float *sp = NULL,*ip = NULL,*ip2 = NULL;

    if(in1->row != in2->row){
        wtk_debug("cat error \n");
        exit(1);
    }
    sp = out->p;
    ip = in1->p;
    ip2 = in2->p;
    for(i = 0; i < row; ++i){
        memcpy(sp,ip,sizeof(float)*col1);
        sp+=col1;
        memcpy(sp,ip2,sizeof(float)*col2);
        sp+=col2;
        ip+=col1;
        ip2+=col2;
    }
    return;
}

void qtk_tts_ncrf_fnn_ip_cat(wtk_matf_t *in1,wtk_matf_t *in2,wtk_matf_t *in3,wtk_matf_t *out)
{
    int i = 0,row = in1->row,col1 = in1->col,col2 = in2->col,col3 = in3->col;
    float *sp = NULL,*ip = NULL,*ip2 = NULL,*ip3 = NULL;

    if(in1->row != in2->row || in2->row != in3->row){
        wtk_debug("cat error \n");
        exit(1);
    }
    sp = out->p;
    ip = in1->p;
    ip2 = in2->p;
    ip3 = in3->p;
    for(i = 0; i < row; ++i){
        memcpy(sp,ip,sizeof(float)*col1);
        sp+=col1;
        memcpy(sp,ip2,sizeof(float)*col2);
        sp+=col2;
        memcpy(sp,ip3,sizeof(float)*col3);
        sp+=col3;
        ip+=col1;ip2+=col2;ip3+=col3;
    }
    return;
}

int qtk_tts_ncrf_combine(qtk_tts_ncrf_t *ncrf,wtk_mati_t *pwi,wtk_mati_t *ppi,wtk_mati_t *ipi,wtk_veci_t *out)
{
    wtk_array_t *id = NULL,*id_=NULL;
    int ret = -1,*idl = NULL,pw_id,pp_id,ip_id;
    int pw_id_,pp_id_,ip_id_,char_id_;
    int i = 0,n = 0,*wl = NULL,*pl=NULL,*il=NULL,*o=NULL;

    id = ncrf->cfg->id;
    id_=ncrf->cfg->id_;
    if(id->nslot != id_->nslot){
        wtk_debug("combine id error\n");
        goto end;
    }
    idl = id->slot;
    pw_id = idl[0];pp_id = idl[1];ip_id = idl[2];
    idl = id_->slot;
    pw_id_ = idl[0];pp_id_ = idl[1];ip_id_ = idl[2];
    char_id_=ncrf->cfg->char_id_;
    
    n = pwi->col;
    o = out->p;
    wl = pwi->p;
    pl=ppi->p;
    il=ipi->p;
    for(i = 0;i < n; ++i){
        if(il[i]==ip_id){
            o[i] = ip_id_;
        }else if(pl[i]==pp_id){
            o[i] = pp_id_;
        }else if(wl[i]==pw_id){
            o[i] = pw_id_;
        }else{
            o[i] = char_id_;
        }
    }
end:
    ret = 0;
    return ret;
}
