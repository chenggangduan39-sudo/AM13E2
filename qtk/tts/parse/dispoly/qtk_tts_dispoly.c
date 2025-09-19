#include "qtk_tts_dispoly.h"
#include "tts-mer/wtk-extend/wtk_heap2.h"
#include "tts-mer/wtk-extend/wtk_mer_source_file.h"
#include "tts-mer/wtk-extend/nn/wtk_nn.h"

#define judge_dim(dimi,n) \
    if(dimi != n){ \
        wtk_debug(#dimi" error %d:%d\n",dimi,n); \
        goto end; \
    }

int qtk_tts_dispoly_mfnn(qtk_tts_dispoly_t *ply,wtk_matf_t *in,wtk_matf_t *out);
int qtk_tts_dispoly_conv(qtk_tts_dispoly_t *ply,wtk_matf_t *in,wtk_matf_t *out);
int qtk_tts_dispoly_lstm(qtk_tts_dispoly_t *ply,wtk_matf_t *in,wtk_matf_t *out);
int qtk_tts_disply_fnn(qtk_tts_dispoly_t *disply,wtk_matf_t *in,wtk_matf_t *out);

qtk_tts_dispoly_t *qtk_tts_dispoly_new(qtk_tts_dispoly_cfg_t *cfg)
{
    qtk_tts_dispoly_t *disply = NULL;
    int *arr = NULL,i = 0;
    wtk_source_loader_t sl;
    wtk_source_t source;

    disply = wtk_malloc(sizeof(qtk_tts_dispoly_t));
    disply->cfg = cfg;
    disply->heap = wtk_heap_new(2048);
    wtk_source_loader_init_file(&sl);
    //embedding
    arr = cfg->word_embedding_dim->slot;
    judge_dim(cfg->word_embedding_dim->nslot,2);
    disply->word_embedding = wtk_matf_new(arr[0],arr[1]);
    wtk_mer_source_loader_load_matf(&sl,&source,disply->cfg->word_embedding_fn,disply->word_embedding);
    arr = cfg->cws_embedding_dim->slot;
    judge_dim(cfg->cws_embedding_dim->nslot,2);
    disply->cws_embedding = wtk_matf_new(arr[0],arr[1]);
    wtk_mer_source_loader_load_matf(&sl,&source,disply->cfg->cws_embedding_fn,disply->cws_embedding);
    arr = cfg->pp_embedding_dim->slot;
    judge_dim(cfg->pp_embedding_dim->nslot,2);
    disply->pp_embedding = wtk_matf_new(arr[0],arr[1]);
    wtk_mer_source_loader_load_matf(&sl,&source,disply->cfg->pp_embedding_fn,disply->pp_embedding);
    arr = cfg->flag_embedding_dim->slot;
    judge_dim(cfg->flag_embedding_dim->nslot,2);
    disply->flag_embedding = wtk_matf_new(arr[0],arr[1]);
    wtk_mer_source_loader_load_matf(&sl,&source,disply->cfg->flag_embedding_fn,disply->flag_embedding);
    arr = cfg->poly_mask_dim->slot;
    judge_dim(cfg->poly_mask_dim->nslot,2);
    disply->poly_mask = wtk_matf_new(arr[0],arr[1]);
    wtk_mer_source_loader_load_matf(&sl,&source,disply->cfg->poly_mask_fn,disply->poly_mask);
    //mfnn
    arr = cfg->mfnn_dim->slot;
    judge_dim(cfg->mfnn_dim->nslot,(cfg->mfnn_num*2));
    disply->mfnn_weight = wtk_malloc(sizeof(wtk_matf_t*)*cfg->mfnn_num);
    disply->mfnn_bias = wtk_malloc(sizeof(wtk_vecf_t*)*cfg->mfnn_num);
    for(i = 0; i < cfg->mfnn_num;++i){
        disply->mfnn_weight[i] = wtk_matf_new(arr[0],arr[1]);
        wtk_mer_source_loader_load_matf(&sl,&source,disply->cfg->mfnn_weight_fn[i],disply->mfnn_weight[i]);
        disply->mfnn_bias[i] =wtk_vecf_new(arr[0]);
        wtk_mer_source_loader_load_vecf(&sl,&source,disply->cfg->mfnn_bias_fn[i],disply->mfnn_bias[i]);
        arr+=2;
    }
    //conv
    arr = cfg->conv_dim->slot;
    judge_dim(cfg->conv_dim->nslot,(cfg->conv_num*2));
    disply->conv_weight = wtk_malloc(sizeof(wtk_matf_t*)*cfg->conv_num);
    disply->conv_bias = wtk_malloc(sizeof(wtk_vecf_t*)*cfg->conv_num);
    disply->conv_layernorm_bias = wtk_malloc(sizeof(wtk_vecf_t*)*cfg->conv_num);
    disply->conv_layernorm_weight = wtk_malloc(sizeof(wtk_vecf_t*)*cfg->conv_num);
    for(i = 0; i < cfg->conv_num;++i){
        disply->conv_weight[i] = wtk_matf_new(arr[0],arr[1]);
        wtk_mer_source_loader_load_matf(&sl,&source,disply->cfg->conv_weight_fn[i],disply->conv_weight[i]);
        disply->conv_bias[i] = wtk_vecf_new(arr[0]);
        wtk_mer_source_loader_load_vecf(&sl,&source,disply->cfg->conv_bias_fn[i],disply->conv_bias[i]);
        disply->conv_layernorm_bias[i] = wtk_vecf_new(arr[0]);
        wtk_mer_source_loader_load_vecf(&sl,&source,disply->cfg->conv_layernorm_bias_fn[i],disply->conv_layernorm_bias[i]);
        disply->conv_layernorm_weight[i] = wtk_vecf_new(arr[0]);
        wtk_mer_source_loader_load_vecf(&sl,&source,disply->cfg->conv_layernorm_weight_fn[i],disply->conv_layernorm_weight[i]);
        arr+=2;
    }
    //lstm
    arr = cfg->lstm_dim->slot;
    judge_dim(cfg->lstm_dim->nslot,(cfg->lstm_num*2));  //双向lstm
    disply->lstm = wtk_malloc(sizeof(wtk_nn_lstm_t*)*disply->cfg->lstm_num);
    disply->lstm_reverse = wtk_malloc(sizeof(wtk_nn_lstm_t*)*disply->cfg->lstm_num);
    for(i = 0; i < cfg->lstm_num;++i){
        disply->lstm[i] = wtk_nn_lstm_new(1,arr[0],arr[1],0.0,NULL,NULL);
        wtk_mer_source_loader_load_matf(&sl,&source,cfg->lstm_weight_fn[i],disply->lstm[i]->kernel);
        wtk_mer_source_loader_load_vecf(&sl,&source,cfg->lstm_bias_fn[i],disply->lstm[i]->bias);
        disply->lstm_reverse[i] = wtk_nn_lstm_new(0,arr[0],arr[1],0.0,NULL,NULL);
        wtk_mer_source_loader_load_matf(&sl,&source,cfg->lstm_weight_reverse_fn[i],disply->lstm_reverse[i]->kernel);
        wtk_mer_source_loader_load_vecf(&sl,&source,cfg->lstm_bias_reverse_fn[i],disply->lstm_reverse[i]->bias);
        arr+=2;
    }
    //fnn
    arr = cfg->fnn_dim->slot;
    judge_dim(cfg->fnn_dim->nslot,(cfg->fnn_num*2));
    disply->fnn_weight = wtk_malloc(sizeof(wtk_matf_t*)*cfg->fnn_num);
    disply->fnn_bias = wtk_malloc(sizeof(wtk_vecf_t*)*cfg->fnn_num);
    for(i = 0; i < cfg->fnn_num;++i){
        disply->fnn_weight[i] = wtk_matf_new(arr[0],arr[1]);
        wtk_mer_source_loader_load_matf(&sl,&source,cfg->fnn_weight_fn[i],disply->fnn_weight[i]);
        disply->fnn_bias[i] = wtk_vecf_new(arr[0]);
        wtk_mer_source_loader_load_vecf(&sl,&source,cfg->fnn_bias_fn[i],disply->fnn_bias[i]);
        arr+=2;
    }
end:
    return disply;
}
qtk_tts_dispoly_t *qtk_tts_dispoly_new2(qtk_tts_dispoly_cfg_t *cfg, wtk_rbin2_t* rbin)
{
    qtk_tts_dispoly_t *disply = NULL;
    int *arr = NULL,i = 0;
    wtk_source_loader_t sl;
    wtk_source_t src;
    FILE* f;

    disply = wtk_malloc(sizeof(qtk_tts_dispoly_t));
    disply->cfg = cfg;
    disply->heap = wtk_heap_new(2048);
    wtk_source_loader_init_file(&sl);
    //embedding
    arr = cfg->word_embedding_dim->slot;
    judge_dim(cfg->word_embedding_dim->nslot,2);
    disply->word_embedding = wtk_matf_new(arr[0],arr[1]);
    if (rbin)
    {
		f=wtk_rbin2_get_file(rbin, disply->cfg->word_embedding_fn);
		wtk_source_init_fd(&src,f, ftell(f));
	    wtk_mer_source_read_float(&src, disply->word_embedding->p, disply->word_embedding->row*disply->word_embedding->col, 1);
		wtk_source_clean_fd(&src);
		if (f)fclose(f);
    }else
    {
    	wtk_mer_source_loader_load_matf(&sl,&src,disply->cfg->word_embedding_fn,disply->word_embedding);
    }

    arr = cfg->cws_embedding_dim->slot;
    judge_dim(cfg->cws_embedding_dim->nslot,2);
    disply->cws_embedding = wtk_matf_new(arr[0],arr[1]);
    if (rbin)
    {
		f=wtk_rbin2_get_file(rbin, disply->cfg->cws_embedding_fn);
		wtk_source_init_fd(&src,f, ftell(f));
	    wtk_mer_source_read_float(&src, disply->cws_embedding->p, disply->cws_embedding->row*disply->cws_embedding->col, 1);
		wtk_source_clean_fd(&src);
		if (f)fclose(f);
    }else
    {
    	wtk_mer_source_loader_load_matf(&sl,&src,disply->cfg->cws_embedding_fn,disply->cws_embedding);
    }

    arr = cfg->pp_embedding_dim->slot;
    judge_dim(cfg->pp_embedding_dim->nslot,2);
    disply->pp_embedding = wtk_matf_new(arr[0],arr[1]);
    if (rbin)
    {
		f=wtk_rbin2_get_file(rbin, disply->cfg->pp_embedding_fn);
		wtk_source_init_fd(&src,f, ftell(f));
	    wtk_mer_source_read_float(&src, disply->pp_embedding->p, disply->pp_embedding->row*disply->pp_embedding->col, 1);
		wtk_source_clean_fd(&src);
		if (f)fclose(f);
    }else
    {
    	wtk_mer_source_loader_load_matf(&sl,&src,disply->cfg->pp_embedding_fn,disply->pp_embedding);
    }

    arr = cfg->flag_embedding_dim->slot;
    judge_dim(cfg->flag_embedding_dim->nslot,2);
    disply->flag_embedding = wtk_matf_new(arr[0],arr[1]);
    if (rbin)
    {
		f=wtk_rbin2_get_file(rbin, disply->cfg->flag_embedding_fn);
		wtk_source_init_fd(&src,f, ftell(f));
	    wtk_mer_source_read_float(&src, disply->flag_embedding->p, disply->flag_embedding->row*disply->flag_embedding->col, 1);
		wtk_source_clean_fd(&src);
		if (f)fclose(f);
    }else
    {
    	wtk_mer_source_loader_load_matf(&sl,&src,disply->cfg->flag_embedding_fn,disply->flag_embedding);
    }

    arr = cfg->poly_mask_dim->slot;
    judge_dim(cfg->poly_mask_dim->nslot,2);
    disply->poly_mask = wtk_matf_new(arr[0],arr[1]);
    if (rbin)
    {
		f=wtk_rbin2_get_file(rbin, disply->cfg->poly_mask_fn);
		wtk_source_init_fd(&src,f, ftell(f));
	    wtk_mer_source_read_float(&src, disply->poly_mask->p, disply->poly_mask->row*disply->poly_mask->col, 1);
		wtk_source_clean_fd(&src);
		if (f)fclose(f);
    }else
    {
    	wtk_mer_source_loader_load_matf(&sl,&src,disply->cfg->poly_mask_fn,disply->poly_mask);
    }

    //mfnn
    arr = cfg->mfnn_dim->slot;
    judge_dim(cfg->mfnn_dim->nslot,(cfg->mfnn_num*2));
    disply->mfnn_weight = wtk_malloc(sizeof(wtk_matf_t*)*cfg->mfnn_num);
    disply->mfnn_bias = wtk_malloc(sizeof(wtk_vecf_t*)*cfg->mfnn_num);
    for(i = 0; i < cfg->mfnn_num;++i){
        disply->mfnn_weight[i] = wtk_matf_new(arr[0],arr[1]);
        if (rbin)
        {
    		f=wtk_rbin2_get_file(rbin, disply->cfg->mfnn_weight_fn[i]);
    		wtk_source_init_fd(&src,f, ftell(f));
    	    wtk_mer_source_read_float(&src, disply->mfnn_weight[i]->p, disply->mfnn_weight[i]->row*disply->mfnn_weight[i]->col, 1);
    		wtk_source_clean_fd(&src);
    		if (f)fclose(f);
        }else
        {
        	wtk_mer_source_loader_load_matf(&sl,&src,disply->cfg->mfnn_weight_fn[i],disply->mfnn_weight[i]);
        }

        disply->mfnn_bias[i] =wtk_vecf_new(arr[0]);
        if (rbin)
        {
    		f=wtk_rbin2_get_file(rbin, disply->cfg->mfnn_bias_fn[i]);
    		wtk_source_init_fd(&src,f, ftell(f));
    	    wtk_mer_source_read_float(&src, disply->mfnn_bias[i]->p, disply->mfnn_bias[i]->len, 1);
    		wtk_source_clean_fd(&src);
    		if (f)fclose(f);
        }else
        {
        	wtk_mer_source_loader_load_vecf(&sl,&src,disply->cfg->mfnn_bias_fn[i],disply->mfnn_bias[i]);
        }

        arr+=2;
    }
    //conv
    arr = cfg->conv_dim->slot;
    judge_dim(cfg->conv_dim->nslot,(cfg->conv_num*2));
    disply->conv_weight = wtk_malloc(sizeof(wtk_matf_t*)*cfg->conv_num);
    disply->conv_bias = wtk_malloc(sizeof(wtk_vecf_t*)*cfg->conv_num);
    disply->conv_layernorm_bias = wtk_malloc(sizeof(wtk_vecf_t*)*cfg->conv_num);
    disply->conv_layernorm_weight = wtk_malloc(sizeof(wtk_vecf_t*)*cfg->conv_num);
    for(i = 0; i < cfg->conv_num;++i){
        disply->conv_weight[i] = wtk_matf_new(arr[0],arr[1]);
        if (rbin)
        {
    		f=wtk_rbin2_get_file(rbin, disply->cfg->conv_weight_fn[i]);
    		wtk_source_init_fd(&src,f, ftell(f));
    	    wtk_mer_source_read_float(&src, disply->conv_weight[i]->p, disply->conv_weight[i]->row*disply->conv_weight[i]->col, 1);
    		wtk_source_clean_fd(&src);
    		if (f)fclose(f);
        }else
        {
        	wtk_mer_source_loader_load_matf(&sl,&src,disply->cfg->conv_weight_fn[i],disply->conv_weight[i]);
        }

        disply->conv_bias[i] = wtk_vecf_new(arr[0]);
        if (rbin)
        {
    		f=wtk_rbin2_get_file(rbin, disply->cfg->conv_bias_fn[i]);
    		wtk_source_init_fd(&src,f, ftell(f));
    	    wtk_mer_source_read_float(&src, disply->conv_bias[i]->p, disply->conv_bias[i]->len, 1);
    		wtk_source_clean_fd(&src);
    		if (f)fclose(f);
        }else
        {
        	wtk_mer_source_loader_load_vecf(&sl,&src,disply->cfg->conv_bias_fn[i],disply->conv_bias[i]);
        }

        disply->conv_layernorm_bias[i] = wtk_vecf_new(arr[0]);
        if (rbin)
        {
    		f=wtk_rbin2_get_file(rbin, disply->cfg->conv_layernorm_bias_fn[i]);
    		wtk_source_init_fd(&src,f, ftell(f));
    	    wtk_mer_source_read_float(&src, disply->conv_layernorm_bias[i]->p, disply->conv_layernorm_bias[i]->len, 1);
    		wtk_source_clean_fd(&src);
    		if (f)fclose(f);
        }else
        {
        	wtk_mer_source_loader_load_vecf(&sl,&src,disply->cfg->conv_layernorm_bias_fn[i],disply->conv_layernorm_bias[i]);
        }

        disply->conv_layernorm_weight[i] = wtk_vecf_new(arr[0]);
        if (rbin)
        {
    		f=wtk_rbin2_get_file(rbin, disply->cfg->conv_layernorm_weight_fn[i]);
    		wtk_source_init_fd(&src,f, ftell(f));
    	    wtk_mer_source_read_float(&src, disply->conv_layernorm_weight[i]->p, disply->conv_layernorm_weight[i]->len, 1);
    		wtk_source_clean_fd(&src);
    		if (f)fclose(f);
        }else
        {
        	wtk_mer_source_loader_load_vecf(&sl,&src,disply->cfg->conv_layernorm_weight_fn[i],disply->conv_layernorm_weight[i]);
        }

        arr+=2;
    }
    //lstm
    arr = cfg->lstm_dim->slot;
    judge_dim(cfg->lstm_dim->nslot,(cfg->lstm_num*2));  //双向lstm
    disply->lstm = wtk_malloc(sizeof(wtk_nn_lstm_t*)*disply->cfg->lstm_num);
    disply->lstm_reverse = wtk_malloc(sizeof(wtk_nn_lstm_t*)*disply->cfg->lstm_num);
    for(i = 0; i < cfg->lstm_num;++i){
        disply->lstm[i] = wtk_nn_lstm_new(1,arr[0],arr[1],0.0,NULL,NULL);
        if (rbin)
        {
    		f=wtk_rbin2_get_file(rbin, disply->cfg->lstm_weight_fn[i]);
    		wtk_source_init_fd(&src,f, ftell(f));
    	    wtk_mer_source_read_float(&src, disply->lstm[i]->kernel->p, disply->lstm[i]->kernel->row*disply->lstm[i]->kernel->col, 1);
    		wtk_source_clean_fd(&src);
    		if (f)fclose(f);
        }else
        {
        	wtk_mer_source_loader_load_matf(&sl,&src,cfg->lstm_weight_fn[i],disply->lstm[i]->kernel);
        }

        if (rbin)
        {
    		f=wtk_rbin2_get_file(rbin, disply->cfg->lstm_bias_fn[i]);
    		wtk_source_init_fd(&src,f, ftell(f));
    	    wtk_mer_source_read_float(&src, disply->lstm[i]->bias->p, disply->lstm[i]->bias->len, 1);
    		wtk_source_clean_fd(&src);
    		if (f)fclose(f);
        }else
        {
        	wtk_mer_source_loader_load_vecf(&sl,&src,cfg->lstm_bias_fn[i],disply->lstm[i]->bias);
        }

        disply->lstm_reverse[i] = wtk_nn_lstm_new(0,arr[0],arr[1],0.0,NULL,NULL);
        if (rbin)
        {
    		f=wtk_rbin2_get_file(rbin, disply->cfg->lstm_weight_reverse_fn[i]);
    		wtk_source_init_fd(&src,f, ftell(f));
    	    wtk_mer_source_read_float(&src, disply->lstm_reverse[i]->kernel->p, disply->lstm_reverse[i]->kernel->row*disply->lstm_reverse[i]->kernel->col, 1);
    		wtk_source_clean_fd(&src);
    		if (f)fclose(f);
        }else
        {
        	wtk_mer_source_loader_load_matf(&sl,&src,cfg->lstm_weight_reverse_fn[i],disply->lstm_reverse[i]->kernel);
        }
        if (rbin)
        {
    		f=wtk_rbin2_get_file(rbin, disply->cfg->lstm_bias_reverse_fn[i]);
    		wtk_source_init_fd(&src,f, ftell(f));
    	    wtk_mer_source_read_float(&src, disply->lstm_reverse[i]->bias->p, disply->lstm_reverse[i]->bias->len, 1);
    		wtk_source_clean_fd(&src);
    		if (f)fclose(f);
        }else
        {
        	wtk_mer_source_loader_load_vecf(&sl,&src,cfg->lstm_bias_reverse_fn[i],disply->lstm_reverse[i]->bias);
        }

        arr+=2;
    }
    //fnn
    arr = cfg->fnn_dim->slot;
    judge_dim(cfg->fnn_dim->nslot,(cfg->fnn_num*2));
    disply->fnn_weight = wtk_malloc(sizeof(wtk_matf_t*)*cfg->fnn_num);
    disply->fnn_bias = wtk_malloc(sizeof(wtk_vecf_t*)*cfg->fnn_num);
    for(i = 0; i < cfg->fnn_num;++i){
        disply->fnn_weight[i] = wtk_matf_new(arr[0],arr[1]);
        if (rbin)
        {
    		f=wtk_rbin2_get_file(rbin, disply->cfg->fnn_weight_fn[i]);
    		wtk_source_init_fd(&src,f, ftell(f));
    	    wtk_mer_source_read_float(&src, disply->fnn_weight[i]->p, disply->fnn_weight[i]->row*disply->fnn_weight[i]->col, 1);
    		wtk_source_clean_fd(&src);
    		if (f)fclose(f);
        }else
        {
        	wtk_mer_source_loader_load_matf(&sl,&src,cfg->fnn_weight_fn[i],disply->fnn_weight[i]);
        }

        disply->fnn_bias[i] = wtk_vecf_new(arr[0]);
        if (rbin)
        {
    		f=wtk_rbin2_get_file(rbin, disply->cfg->fnn_bias_fn[i]);
    		wtk_source_init_fd(&src,f, ftell(f));
    	    wtk_mer_source_read_float(&src, disply->fnn_bias[i]->p, disply->fnn_bias[i]->len, 1);
    		wtk_source_clean_fd(&src);
    		if (f)fclose(f);
        }else
        {
        	wtk_mer_source_loader_load_vecf(&sl,&src,cfg->fnn_bias_fn[i],disply->fnn_bias[i]);
        }

        arr+=2;
    }
end:
    return disply;
}

int qtk_tts_dispoly_process(qtk_tts_dispoly_t *disply,wtk_veci_t **vecs,wtk_matf_t *out)
{
    int ret = 0;
    wtk_veci_t *label = NULL;
    wtk_heap_t *heap = disply->heap;
    int charl = vecs[0]->len,i = 0;
    static int pp_id_other = 1,poly_to_pred_flag = 2;
    int lp = 0,embedding_col = 0;
    wtk_matf_t *embedding_out = NULL,*mfnn_out = NULL,*conv_out = NULL,
            *lstm_out = NULL,*poly_mask = NULL;
    float *p = NULL,*p2 = NULL,*po = NULL;
    wtk_matf_t *fnn_input = NULL,*fnn_out = NULL;;

    label = wtk_veci_heap_new(heap,charl);
    for(i = 0;i < charl;++i){
        lp = vecs[2]->p[i];
        if(vecs[3]->p[i] != poly_to_pred_flag){
            lp = pp_id_other;
        }
        if(vecs[2]->p[i] == 0){
            lp = 0;
        }
        label->p[i] = lp;
    }
    //embedding
    embedding_col = disply->word_embedding->col+disply->cws_embedding->col
                                        +disply->pp_embedding->col+disply->flag_embedding->col
                                        +disply->poly_mask->col;
    embedding_out = wtk_matf_heap_new(heap,charl,embedding_col);
    poly_mask = wtk_matf_heap_new(heap,charl,disply->poly_mask->col);
    for(i = 0; i < charl; ++i){
        p = wtk_matf_at(embedding_out,i,0);
        memcpy(p,wtk_matf_at(disply->word_embedding,vecs[0]->p[i],0),sizeof(float)*disply->word_embedding->col);
        p+=disply->word_embedding->col;
        memcpy(p,wtk_matf_at(disply->cws_embedding,vecs[1]->p[i],0),sizeof(float)*disply->cws_embedding->col);
        p+=disply->cws_embedding->col;
        memcpy(p,wtk_matf_at(disply->pp_embedding,vecs[2]->p[i],0),sizeof(float)*disply->pp_embedding->col);
        p+=disply->pp_embedding->col;
        memcpy(p,wtk_matf_at(disply->flag_embedding,vecs[3]->p[i],0),sizeof(float)*disply->flag_embedding->col);
        p+=disply->flag_embedding->col;
        memcpy(p,wtk_matf_at(disply->poly_mask,label->p[i],0),sizeof(float)*disply->poly_mask->col);
        memcpy(wtk_matf_at(poly_mask,i,0),wtk_matf_at(disply->poly_mask,label->p[i],0),sizeof(float)*disply->poly_mask->col);
    }
    mfnn_out = wtk_matf_heap_new(heap,embedding_out->row,disply->mfnn_bias[disply->cfg->mfnn_num-1]->len);
    qtk_tts_dispoly_mfnn(disply,embedding_out,mfnn_out);
    
    conv_out = wtk_matf_heap_new(heap,mfnn_out->row,disply->conv_weight[disply->cfg->conv_num-1]->row);
    qtk_tts_dispoly_conv(disply,mfnn_out,conv_out);
    lstm_out = wtk_matf_heap_new(heap,mfnn_out->row,mfnn_out->col);
    qtk_tts_dispoly_lstm(disply,mfnn_out,lstm_out);
    //cat
    fnn_input = wtk_matf_heap_new(heap,conv_out->row,conv_out->col+lstm_out->col);
    po = fnn_input->p;
    p = conv_out->p;
    p2 = lstm_out->p;
    for(i = 0; i < conv_out->row;++i){
        memcpy(po,p2,lstm_out->col*sizeof(float));
        memcpy(po+lstm_out->col,p,conv_out->col*sizeof(float));
        po+=fnn_input->col;
        p += conv_out->col;
        p2 += lstm_out->col;
    }
    //fnn
    fnn_out = wtk_matf_heap_new(heap,fnn_input->row,disply->fnn_weight[disply->cfg->fnn_num-1]->row);
    qtk_tts_disply_fnn(disply,fnn_input,fnn_out);
    //weighted_softmax
    wtk_float_mult(fnn_out->p,poly_mask->p,fnn_out->p,fnn_out->col*fnn_out->row);
    for(i = 0; i < fnn_out->row;++i){
        wtk_nn_softmax(wtk_matf_at(fnn_out,i,0),fnn_out->col);
    }
    memcpy(out->p,fnn_out->p,sizeof(float)*fnn_out->row*fnn_out->col);
    
    wtk_heap_reset(heap);
    return ret;
}

int qtk_tts_dispoly_delete(qtk_tts_dispoly_t *disply)
{
    int ret = 0,i = 0;
    if(disply->heap)
        wtk_heap_delete(disply->heap);
    //embedding
    if(disply->word_embedding)
        wtk_matf_delete(disply->word_embedding);
    if(disply->cws_embedding)
        wtk_matf_delete(disply->cws_embedding);
    if(disply->pp_embedding)
        wtk_matf_delete(disply->pp_embedding);
    if(disply->flag_embedding)
        wtk_matf_delete(disply->flag_embedding);
    if(disply->poly_mask)
        wtk_matf_delete(disply->poly_mask);
    for(i = 0; i < disply->cfg->mfnn_num;++i){
        wtk_matf_delete(disply->mfnn_weight[i]);
        wtk_vecf_delete(disply->mfnn_bias[i]);
    }
    wtk_free(disply->mfnn_bias);
    wtk_free(disply->mfnn_weight);
    for(i = 0; i < disply->cfg->conv_num;++i){
        wtk_matf_delete(disply->conv_weight[i]);
        wtk_vecf_delete(disply->conv_bias[i]);
        wtk_vecf_delete(disply->conv_layernorm_bias[i]);
        wtk_vecf_delete(disply->conv_layernorm_weight[i]);
    }
    wtk_free(disply->conv_weight);
    wtk_free(disply->conv_bias);
    wtk_free(disply->conv_layernorm_bias);
    wtk_free(disply->conv_layernorm_weight);

    for(i = 0; i < disply->cfg->lstm_num;++i){
        wtk_nn_lstm_delete(disply->lstm[i]);
        wtk_nn_lstm_delete(disply->lstm_reverse[i]);
    }
    wtk_free(disply->lstm);
    wtk_free(disply->lstm_reverse);
    for(i = 0; i < disply->cfg->fnn_num;++i){
        wtk_matf_delete(disply->fnn_weight[i]);
        wtk_vecf_delete(disply->fnn_bias[i]);
    }
    wtk_free(disply->fnn_weight);
    wtk_free(disply->fnn_bias);

    wtk_free(disply);
    return ret;
}


int qtk_tts_dispoly_mfnn(qtk_tts_dispoly_t *ply,wtk_matf_t *in,wtk_matf_t *out)
{
    int ret = 0,i = 0;
    wtk_heap_t *heap = ply->heap;
    wtk_matf_t *tmp = NULL;

    for(i=0;i < ply->cfg->mfnn_num;++i){
        tmp = wtk_matf_heap_new(heap,in->row,ply->mfnn_weight[i]->row);
        wtk_nn_layer_dense(in,ply->mfnn_weight[i],ply->mfnn_bias[i],wtk_nn_relu,tmp);
        in = tmp;
    }
    memcpy(out->p,tmp->p,sizeof(float)*tmp->row*tmp->col);
    return ret;
}

int qtk_tts_dispoly_conv(qtk_tts_dispoly_t *ply,wtk_matf_t *in,wtk_matf_t *out)
{
    wtk_heap_t *heap = ply->heap;
    int ret = 0,i = 0;
    wtk_matf_t *dst = NULL;
    int *k_size = NULL;

    k_size = ply->cfg->conv_size_dim->slot;
    for(i = 0; i < ply->cfg->conv_num;++i){
        dst = wtk_matf_heap_new(heap,in->row,ply->conv_weight[i]->row);
        wtk_nn_conv1d2(ply->conv_weight[i],k_size[i],enum_conv_same,in,dst);
        wtk_matf_vecf_add(dst,ply->conv_bias[i]);
        wtk_nn_relu(dst->p,dst->row*dst->col);
        wtk_nn_layer_norm_1dim(dst,ply->conv_layernorm_weight[i],ply->conv_layernorm_bias[i],1E-5);
        in = dst;
    }
    memcpy(out->p,dst->p,sizeof(float)*dst->row*dst->col);
    return ret;
}

int qtk_tts_dispoly_lstm(qtk_tts_dispoly_t *ply,wtk_matf_t *in,wtk_matf_t *out)
{
    int ret = 0,num = 0,i = 0,j = 0;
    wtk_heap_t *heap = ply->heap;
    wtk_nn_lstm_t **lstm = ply->lstm;
    wtk_nn_lstm_t **lstm_r = ply->lstm_reverse;
    wtk_matf_t *moutf = NULL,*moutr = NULL,*mout = NULL;
    int row = in->row;
    float *fp = NULL,*rp = NULL,*op = NULL;

    num = ply->cfg->lstm_num;
    moutf = wtk_matf_heap_new(heap,row,in->col/2);
    moutr = wtk_matf_heap_new(heap,row,in->col/2);
    mout = wtk_matf_heap_new(heap,row,in->col);
    for(i = 0; i < num; ++i){
        wtk_nn_lstm_reset(lstm[i]);
        wtk_nn_lstm(lstm[i],in,moutf);
        wtk_nn_lstm_reset(lstm[i]);
        wtk_nn_lstm(lstm_r[i],in,moutr);
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
        in = mout;
    }
    memcpy(out->p,mout->p,sizeof(float)*out->row*out->col);
    return ret;
}

int qtk_tts_disply_fnn(qtk_tts_dispoly_t *disply,wtk_matf_t *in,wtk_matf_t *out)
{
    int ret = 0;
    int i = 0;
    wtk_matf_t *dst = NULL;
    wtk_heap_t *heap = disply->heap;

    for(i = 0;i < disply->cfg->fnn_num;++i){
        dst = wtk_matf_heap_new(heap,in->row,disply->fnn_weight[i]->row);
        wtk_nn_layer_dense(in,disply->fnn_weight[i],disply->fnn_bias[i],i==(disply->cfg->fnn_num-1)?NULL:wtk_nn_relu,dst);
        in = dst;
    } 
    memcpy(out->p,dst->p,sizeof(float)*dst->row*dst->col);
    return ret;
}
