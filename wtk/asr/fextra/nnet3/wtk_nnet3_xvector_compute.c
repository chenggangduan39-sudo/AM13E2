#include "wtk_nnet3_xvector_compute.h"
#ifdef USE_NEON
#include "wtk/os/asm/arm/wtk_neon_math.h"
#endif

void wtk_nnet3_xvector_compute_kxparm_notity(wtk_nnet3_xvector_compute_t *x,wtk_kfeat_t *feat);
void wtk_nnet3_xvector_compute_kxparm_notity2(wtk_nnet3_xvector_compute_t *x,wtk_feat_t *feat);
void wtk_nnet3_xvector_compute_kxparm_notity_end(wtk_nnet3_xvector_compute_t *x);
// wtk_nnet3_xvector_compute_feat_cache_node_t* wtk_nnet3_xvector_compute_feat_cache_node_new(wtk_nnet3_xvector_compute_t *x);
// void wtk_nnet3_xvector_compute_feat_cache_node_delete(wtk_nnet3_xvector_compute_t *x,wtk_nnet3_xvector_compute_feat_cache_node_t *node);
void wtk_nnet3_xvector_compute_kxparm_kind_notify(wtk_nnet3_xvector_compute_t *x,wtk_kfeat_t *feat);
void wtk_nnet3_xvector_compute_nn_notify(wtk_nnet3_xvector_compute_t *x,qtk_blas_matrix_t *m);
void wtk_nnet3_xvector_compute_nn_notify_end(wtk_nnet3_xvector_compute_t *x);

void wtk_nnet3_xvector_compute_set_nn_notify(wtk_nnet3_xvector_compute_t *x)
{
	qtk_torchnn_set_notify(x->nn,x,(qtk_torchnn_notify_f)wtk_nnet3_xvector_compute_nn_notify);
	qtk_torchnn_set_notify_end(x->nn,(qtk_torchnn_notify_end_f)wtk_nnet3_xvector_compute_nn_notify_end);
}

wtk_nnet3_xvector_compute_t* wtk_nnet3_xvector_compute_new(wtk_nnet3_xvector_compute_cfg_t *cfg)
{
    wtk_nnet3_xvector_compute_t *x;

    x=wtk_malloc(sizeof(*x));
    x->cfg=cfg;
    x->kxparm=wtk_kxparm_new(&(cfg->kxparm));
    wtk_kxparm_set_notify(x->kxparm,x,(wtk_kxparm_notify_f)wtk_nnet3_xvector_compute_kxparm_notity);
    wtk_kxparm_set_notify_end(x->kxparm,x,(wtk_kxparm_notify_end_f)wtk_nnet3_xvector_compute_kxparm_notity_end);
#ifdef ONNX_DEC
    x->num_feats = 0;
    x->feat=wtk_strbuf_new(1024,1);
    x->wav = wtk_strbuf_new(1024, 1);
    x->onnx = qtk_onnxruntime_new(&(cfg->onnx));
#endif
    //x->spk_mean=wtk_vecf_new(x->cfg->kxparm.knn.output_dim);
    x->spk_cnt=0;
    //x->transform_out=wtk_vecf_new(x->cfg->transform->row);
    x->nn = NULL;
    x->xvec_in_buf = NULL;
    x->xvec_in = NULL;
    x->pool = NULL;
    x->in_idx = 0;
    x->xvec=wtk_vecf_new(256);
    wtk_kxparm_set_kind_notify(x->kxparm,x,(wtk_kxparm_notify_f)wtk_nnet3_xvector_compute_kxparm_kind_notify);
    // printf("nnet3 %p\n",x);
    return x;
}

wtk_nnet3_xvector_compute_t* wtk_nnet3_xvector_compute_new2(wtk_nnet3_xvector_compute_cfg_t *cfg)
{
    wtk_nnet3_xvector_compute_t *x;

    x=wtk_malloc(sizeof(*x));
    memset(x,0,sizeof(wtk_nnet3_xvector_compute_t));
    x->cfg=cfg;
    x->kxparm=wtk_kxparm_new(&(cfg->kxparm));
    wtk_kxparm_set_notify(x->kxparm,x,(wtk_kxparm_notify_f)wtk_nnet3_xvector_compute_kxparm_notity);
    wtk_kxparm_set_notify2(x->kxparm,x,(wtk_kxparm_notify_f2)wtk_nnet3_xvector_compute_kxparm_notity2);//for htk
    wtk_kxparm_set_notify_end(x->kxparm,x,(wtk_kxparm_notify_end_f)wtk_nnet3_xvector_compute_kxparm_notity_end);
#ifdef ONNX_DEC
    x->num_feats = 0;
    x->feat=wtk_strbuf_new(1024,1);
    x->wav = wtk_strbuf_new(1024, 1);
    x->onnx = qtk_onnxruntime_new(&(cfg->onnx));
    x->shape[0]=1;
    x->shape2=1;
    x->spk_mean=NULL;
#else
    x->spk_mean=wtk_vecf_new(x->cfg->kxparm.knn.output_dim);
#endif
    x->spk_cnt=0;
    x->transform_out=wtk_vecf_new(256);//wtk_vecf_new(x->cfg->transform->row);
    x->nn = NULL;
    x->xvec_in_buf = NULL;
    x->xvec_in = NULL;
    x->pool = NULL;
    x->in_idx = 0;
    x->xvec=wtk_vecf_new(256);
    wtk_kxparm_set_kind_notify(x->kxparm,x,(wtk_kxparm_notify_f)wtk_nnet3_xvector_compute_kxparm_kind_notify);
    // printf("nnet3 %p\n",x);
    return x;
}

void wtk_nnet3_xvector_compute_delete(wtk_nnet3_xvector_compute_t *x)
{
	if(x->pool)
	{
		wtk_nnet3_xvector_pool_delete(x->pool);
	}
    if(x->xvec_in_buf){
        wtk_strbuf_delete(x->xvec_in_buf);
    }
	if(x->xvec_in) //ssy
	{
        // wtk_debug("free %p\n",x->xvec_in);
		qtk_blas_matrix_delete(x->xvec_in);
        //wtk_free(x->xvec_in);
        x->xvec_in = NULL;
	}
#ifdef ONNX_DEC
    qtk_onnxruntime_delete(x->onnx);
    wtk_strbuf_delete(x->feat);
    wtk_strbuf_delete(x->wav);
#endif
    wtk_vecf_delete(x->transform_out);
    wtk_vecf_delete(x->spk_mean);
    wtk_kxparm_delete(x->kxparm);
    wtk_vecf_delete(x->xvec);
    wtk_free(x);
}

int wtk_nnet3_xvector_compute_start(wtk_nnet3_xvector_compute_t *x)
{
    wtk_kxparm_start(x->kxparm);
    return 0;
}

void wtk_nnet3_xvector_compute_reset(wtk_nnet3_xvector_compute_t *x)
{
	//wtk_debug("xvec reset\n");
	if(x->xvec_in)   //ssy
	{
		// qtk_blas_matrix_zero(x->xvec_in);
		// x->in_idx = 0;
        x->xvec_in->m = NULL;
        x->xvec_in->row = 0;
	}
    if(x->xvec_in_buf){
        wtk_strbuf_reset(x->xvec_in_buf);
    }
    wtk_kxparm_reset(x->kxparm);
    if(x->pool)
    {
    	qtk_blas_matrix_zero(x->pool->mu);
    	qtk_blas_matrix_zero(x->pool->rh);
    	qtk_blas_matrix_zero(x->pool->output);
    	qtk_blas_matrix_zero(x->pool->xvector);
    }
#ifdef ONNX_DEC
    x->num_feats = 0;
    wtk_strbuf_reset(x->feat);
    wtk_strbuf_reset(x->wav);
    qtk_onnxruntime_reset(x->onnx);
#endif
}

void wtk_nnet3_xvector_compute_reset2(wtk_nnet3_xvector_compute_t *x)
{
	//wtk_debug("xvec reset2\n");
	if(x->xvec_in)  //ssy
	{
		// qtk_blas_matrix_zero(x->xvec_in);
		// x->in_idx = 0;
        x->xvec_in->m = NULL;
        x->xvec_in->row = 0;
	}
    if(x->xvec_in_buf){
        wtk_strbuf_reset(x->xvec_in_buf);
    }
    wtk_kxparm_reset(x->kxparm);
#ifdef ONNX_DEC
    x->num_feats = 0;
    wtk_strbuf_reset(x->feat);
    wtk_strbuf_reset(x->wav);
    qtk_onnxruntime_reset(x->onnx);
#endif
}

wtk_svprint_pool_t* wtk_nnet3_xvector_pool_read(wtk_source_t *src,wtk_strbuf_t *buf,int bin)
{
	wtk_svprint_pool_t *pool = (wtk_svprint_pool_t*)wtk_malloc(sizeof(wtk_svprint_pool_t));
	int col=0;

	//asp
	wtk_source_read_string(src, buf);
	pool->asp = (qtk_torchnn_linear_t*)qtk_torchnn_linear_read(src,buf,bin);

	//attentiom
	wtk_source_read_string(src, buf);
	wtk_source_read_int(src, &col, 1, bin);
	pool->attention=qtk_blas_matrix_new(1,col);
	//wtk_debug("%d %d\n",row,col);
	wtk_source_read_float(src, pool->attention->m, col, bin);
	wtk_source_read_string(src, buf);

	//fc
	wtk_source_read_string(src, buf);
	pool->fc = (qtk_torchnn_linear_t*)qtk_torchnn_linear_read(src,buf,bin);

	//pool->w = qtk_blas_matrix_new();
	//pool->h = qtk_blas_matrix_new();
	pool->mu = qtk_blas_matrix_new(1,pool->asp->weight->row);
	pool->rh = qtk_blas_matrix_new(1,pool->asp->weight->row);
	pool->output = qtk_blas_matrix_new(1,pool->asp->weight->row*2);
	pool->xvector = qtk_blas_matrix_new(1,pool->fc->weight->row);

	return pool;
}

wtk_svprint_pool_t* wtk_nnet3_xvector_pool_read2(wtk_source_t *src,wtk_strbuf_t *buf,int bin)
{
	wtk_svprint_pool_t *pool = (wtk_svprint_pool_t*)wtk_malloc(sizeof(wtk_svprint_pool_t));
	int col=0;

	//asp
	wtk_source_read_string(src, buf);
	pool->asp = (qtk_torchnn_linear_t*)qtk_torchnn_linear_read(src,buf,bin);

	//attentiom
	wtk_source_read_string(src, buf);
	wtk_source_read_int(src, &col, 1, bin);
	pool->attention=qtk_blas_matrix_new(1,col);
	//wtk_debug("%d %d\n",row,col);
	wtk_source_read_float(src, pool->attention->m, col, bin);
	wtk_source_read_string(src, buf);

	//fc
	wtk_source_read_string(src, buf);
	pool->fc = (qtk_torchnn_linear_t*)qtk_torchnn_linear_read(src,buf,bin);

	//pool->w = qtk_blas_matrix_new();
	//pool->h = qtk_blas_matrix_new();
	pool->mu = qtk_blas_matrix_new(1,pool->asp->weight->row);
	pool->rh = qtk_blas_matrix_new(1,pool->asp->weight->row);
	pool->output = qtk_blas_matrix_new(1,pool->asp->weight->row);
	pool->xvector = qtk_blas_matrix_new(1,pool->fc->weight->row);

	return pool;
}

int xvector_pool_read(wtk_nnet3_xvector_compute_t *x,wtk_source_t *src)
{
    int ret = 0;
    wtk_strbuf_t *buf = wtk_strbuf_new(1024,1);

    x->pool = wtk_nnet3_xvector_pool_read(src,buf,0);
    x->xvec_in_buf = wtk_strbuf_new(100*x->pool->asp->bias->col*sizeof(float),1.0f);
    // x->xvec_in = qtk_blas_matrix_new(100,x->pool->asp->bias->col);
    x->xvec_in = wtk_calloc(1,sizeof(qtk_blas_matrix_t));
    //wtk_debug("%d\n",x->pool->asp->bias->col);
    x->transform_out=wtk_vecf_new(x->pool->fc->bias->col);

    wtk_strbuf_delete(buf);
    return ret;
}

void wtk_nnet3_xvector_pool_delete(wtk_svprint_pool_t *asp)
{
	qtk_torchnn_linear_delete(asp->asp);
	wtk_free(asp->asp);
	qtk_torchnn_linear_delete(asp->fc);
	wtk_free(asp->fc);
	qtk_blas_matrix_delete(asp->attention);
	//qtk_blas_matrix_delete(asp->h);
	//qtk_blas_matrix_delete(asp->w);
	qtk_blas_matrix_delete(asp->mu);
	qtk_blas_matrix_delete(asp->rh);
	qtk_blas_matrix_delete(asp->output);
	qtk_blas_matrix_delete(asp->xvector);
	wtk_free(asp);
}

void wtk_nnet3_xvector_compute_pool(wtk_svprint_pool_t *asp,qtk_blas_matrix_t *in)
{
	int i;

	//memcpy(in->m,asp_in,sizeof(float)*64*14);
	//in->row = 14;in->col  = 64;
	//wtk_debug("notify end %d %d\n",in->row,in->col);
	//qtk_blas_matrix_print(in);
	asp->h = qtk_blas_matrix_new(in->row,asp->asp->weight->col);
	asp->w = qtk_blas_matrix_new(in->row,1);

	qtk_torchnn_linear_cal(asp->asp,in,asp->h);
	for(i=0;i<asp->h->row;i++)
	{
		wtk_tanh(asp->h->m+asp->h->col*i,asp->h->col);
	}
	//qtk_blas_matrix_print(asp->h);
	qtk_blas_matrix_mul(asp->h,asp->w,NULL,NULL,asp->attention,NULL);
	wtk_softmax(asp->w->m,asp->w->col*asp->w->row);
	//qtk_blas_matrix_print(asp->w);
	qtk_blas_matrix_sum_row(asp->mu,in,asp->w);
	//qtk_blas_matrix_print(asp->mu);
	qtk_blas_matrix_apply_power(in,2);
	qtk_blas_matrix_sum_row(asp->rh,in,asp->w);

	i = asp->mu->col*asp->mu->row;
	memcpy(asp->output->m,asp->mu->m,sizeof(float)*i);
	qtk_blas_matrix_apply_power(asp->mu,2);
	qtk_blas_matrix_scale(asp->mu,NULL,-1.0);
	qtk_blas_matrix_add_mat(asp->rh,asp->mu);
	qtk_blas_matrix_apply_power(asp->rh,0.5);
	//qtk_blas_matrix_print(asp->rh);
	memcpy(asp->output->m + i,asp->rh->m,sizeof(float)*asp->rh->col*asp->rh->row);
	qtk_torchnn_linear_cal(asp->fc,asp->output,asp->xvector);
	// wtk_debug("---------------xvector-----------------\n");
	//qtk_blas_matrix_print(asp->xvector);
	qtk_blas_matrix_delete(asp->w);
	qtk_blas_matrix_delete(asp->h);
}

void wtk_nnet3_xvector_compute_pool2(wtk_svprint_pool_t *asp,qtk_blas_matrix_t *in)
{
	int i;

	asp->h = qtk_blas_matrix_new(in->row,asp->asp->weight->col);
	asp->w = qtk_blas_matrix_new(in->row,1);

	qtk_torchnn_linear_cal(asp->asp,in,asp->h);
	for(i=0;i<asp->h->row;i++)
	{
		wtk_tanh(asp->h->m+asp->h->col*i,asp->h->col);
	}
	//wtk_debug("sap1\n");
	//qtk_blas_matrix_print(asp->h);
	qtk_blas_matrix_mul(asp->h,asp->w,NULL,NULL,asp->attention,NULL);
	wtk_softmax(asp->w->m,asp->w->col*asp->w->row);
	//wtk_debug("sap2\n");
	//qtk_blas_matrix_print(asp->w);
	qtk_blas_matrix_sum_row(asp->mu,in,asp->w);
	//wtk_debug("sap3\n");
	//qtk_blas_matrix_print(asp->mu);
	//qtk_blas_matrix_apply_power(in,2);
	//qtk_blas_matrix_sum_row(asp->rh,in,asp->w);

	i = asp->mu->col*asp->mu->row;
	memcpy(asp->output->m,asp->mu->m,sizeof(float)*i);
//	qtk_blas_matrix_apply_power(asp->mu,2);
//	qtk_blas_matrix_scale(asp->mu,NULL,-1.0);
//	qtk_blas_matrix_add_mat(asp->rh,asp->mu);
//	qtk_blas_matrix_apply_power(asp->rh,0.5);
//	//qtk_blas_matrix_print(asp->rh);
//	memcpy(asp->output->m + i,asp->rh->m,sizeof(float)*asp->rh->col*asp->rh->row);
	qtk_torchnn_linear_cal(asp->fc,asp->output,asp->xvector);
	//wtk_debug("sap4\n");
	//qtk_blas_matrix_print(asp->xvector);
	qtk_blas_matrix_delete(asp->w);
	qtk_blas_matrix_delete(asp->h);
}

int wtk_nnet3_xvector_compute_feed(wtk_nnet3_xvector_compute_t *x, short *data,
                                   int len, int is_end) {
    if (x->cfg->use_parm) {
        wtk_kxparm_feed(x->kxparm, data, len, is_end);
    } else {
#ifdef ONNX_DEC
        wtk_strbuf_push(x->wav, (char *)data, len * sizeof(short));
        if (is_end) {
            if (x->cfg->use_onnx) {
                int end_len = (x->wav->pos) >> 1;
                x->shape[0] = (x->wav->pos) >> 1;
                short *s_wav = (short *)x->wav->data;
                float *f_wav = wtk_malloc(end_len * sizeof(float));
                int i;
		for (i = 0; i < end_len; i++) {
                    f_wav[i] = (float)(s_wav[i]) / 32768;
                }

                if(x->cfg->use_normalize){
                    float max = wtk_float_abs_max(f_wav,end_len);
                    if(max > 0.0){
                        for (i = 0; i < end_len; i++) {
                            f_wav[i] /= max;
                        }
                    }
                }

                qtk_onnxruntime_feed(x->onnx, f_wav, x->shape[0],
                                     cast(int64_t *, x->shape), 1, 0, 0);
                qtk_onnxruntime_run(x->onnx);
                float *v2;
                int64_t *shape, size;
                shape = qtk_onnxruntime_get_outshape(x->onnx, 0, &size);
                v2 = qtk_onnxruntime_getout(x->onnx, 0);
                memcpy(x->xvec->p, v2, sizeof(float) * shape[1] * shape[0]);
                wtk_free(shape);
                wtk_free(f_wav);
            }
        }
#endif
    }
    return 0;
}

void wtk_nnet3_xvector_compute_set_kind_notify(wtk_nnet3_xvector_compute_t *x,void *ths,wtk_nnet3_xvector_compute_kxparm_notify_f func)
{
    x->kxparm_func=func;
    x->ths=ths;
}

void wtk_nnet3_xvector_compute_kxparm_kind_notify(wtk_nnet3_xvector_compute_t *x,wtk_kfeat_t *feat)
{
    // printf("%d %p\n",__LINE__,x);
    x->kxparm_func(x->ths,feat);
}

void wtk_nnet3_xvector_compute_kxparm_notity_end(wtk_nnet3_xvector_compute_t *x)
{

	if(x->nn || x->cfg->use_onnx)
	{
#ifdef ONNX_DEC
	int n;
	if(x->kxparm->cfg->use_htk == 1)
	{
		n = x->kxparm->cfg->htk.NUMCHNAS;
	}else
	{
		n = x->kxparm->cfg->parm.melbank.num_bins;
	}
	x->shape[1] = x->num_feats;
	x->shape[2] = n;
	//wtk_debug("%d %d\n",x->num_feats,n);
        qtk_onnxruntime_feed(x->onnx, x->feat->data, x->num_feats * n,
                             cast(int64_t *, x->shape), 3, 0, 0);
        qtk_onnxruntime_run(x->onnx);

	float *v2;
	int64_t *shape,size;
	//int i,j;

	shape = qtk_onnxruntime_get_outshape(x->onnx,0,&size);
        v2 = qtk_onnxruntime_getout(x->onnx, 0);
        //wtk_debug("%d %d\n",shape[1],shape[0]);
	//wtk_strbuf_push(x->xvec_in_buf,(char*)v2,sizeof(float)*shape[1]*shape[0]);
	//x->pool->xvector->m = (float*)x->xvec_in_buf->data;
	memcpy(x->xvec->p,v2,sizeof(float)*shape[1]*shape[0]);
	//wtk_debug("onnx\n");
	//print_float(x->xvec->p,256);
	//x->xvec_in->m = (float*)x->xvec_in_buf->data;
	//x->xvec_in->col = x->pool->asp->bias->col;
	//x->xvec_in->row = x->xvec_in_buf->pos/(x->xvec_in->col*sizeof(float));
	//wtk_nnet3_xvector_compute_pool(x->pool,x->xvec_in);
	wtk_free(shape);
#else
	qtk_torchnn_flush(x->nn);
#endif
	}
}

void wtk_nnet3_xvector_feed_feat(wtk_nnet3_xvector_compute_t *x,float *f,int num_frames, int frame_col)
{
#ifdef ONNX_DEC
	int n;
	float *v2;
	int64_t *shape,size;

	n = frame_col;
	x->shape[1] = num_frames;
	x->shape[2] = n;
        qtk_onnxruntime_feed(x->onnx, f, num_frames * n,
                             cast(int64_t *, x->shape), 3, 0, 0);
        qtk_onnxruntime_run(x->onnx);

	shape = qtk_onnxruntime_get_outshape(x->onnx,0,&size);
	v2 = qtk_onnxruntime_getout(x->onnx,0);
	memcpy(x->xvec->p,v2,sizeof(float)*shape[1]*shape[0]);
	wtk_free(shape);
#endif
}

void wtk_nnet3_xvector_compute_kxparm_notity2(wtk_nnet3_xvector_compute_t *x,
                                              wtk_feat_t *feat) {
#ifdef USE_NEON
    if(x->nn != NULL)
    {
    	wtk_neon_math_vec_add(x->spk_mean->p,x->spk_mean->p,feat->v+1,x->spk_mean->len);
    }else
    {

    }
#else
    if(x->nn == NULL && !x->cfg->use_onnx)
    {
        x->spk_cnt++;
    	wtk_vecf_add(x->spk_mean,feat->v+1);
    }else
    {
    	if(x->kxparm->cfg->use_htk == 1)
    	{
#ifdef ONNX_DEC
            wtk_strbuf_push_float(x->feat,feat->v+1,x->kxparm->cfg->htk.NUMCHNAS);
            x->num_feats++;
#endif
    	}else
    	{
#ifdef ONNX_DEC
            wtk_strbuf_push_float(x->feat,feat->v+1,x->kxparm->cfg->parm.melbank.num_bins);
            x->num_feats++;
#endif
    	}
    }
#endif
}

void wtk_nnet3_xvector_compute_kxparm_notity(wtk_nnet3_xvector_compute_t *x,
                                             wtk_kfeat_t *feat) {
#ifdef USE_NEON
    if(x->nn != NULL)
    {
    	wtk_neon_math_vec_add(x->spk_mean->p,x->spk_mean->p,feat->v,x->spk_mean->len);
    }else
    {

    }
#else
    if(x->nn == NULL)
    {
#ifdef ONNX_DEC
       wtk_strbuf_push_float(x->feat,feat->v,x->kxparm->cfg->parm.melbank.num_bins);
       x->num_feats++;
#else
        x->spk_cnt++;
    	wtk_vecf_add(x->spk_mean,feat->v);
#endif
    }
#endif
}

void wtk_nnet3_xvector_compute_nn_notify(wtk_nnet3_xvector_compute_t *x,qtk_blas_matrix_t *m)
{
	// wtk_debug("-----------torchnn----------------\n");
	// qtk_blas_matrix_print(m);
	// memcpy(x->xvec_in->m + x->xvec_in->col*x->in_idx,m->m,sizeof(float)*m->col*m->row);
	// x->in_idx++;
	// x->xvec_in->row = x->in_idx;
	//exit(0);
    wtk_strbuf_push(x->xvec_in_buf,(char*)m->m,sizeof(float)*m->col*m->row);
}

void wtk_nnet3_xvector_compute_nn_notify_end(wtk_nnet3_xvector_compute_t *x)
{
    // wtk_debug("%d %d\n",x->xvec_in->row,x->xvec_in->col);
    x->xvec_in->m = (float*)x->xvec_in_buf->data;
    x->xvec_in->col = x->pool->asp->bias->col;
    x->xvec_in->row = x->xvec_in_buf->pos/(x->xvec_in->col*sizeof(float));
	wtk_nnet3_xvector_compute_pool(x->pool,x->xvec_in);
}

void wtk_nnet3_xvector_compute_normalize_reset(wtk_nnet3_xvector_compute_t *x)
{
    //x->spk_cnt=0;
    //wtk_vecf_zero(x->spk_mean);
    //wtk_debug("%p %p\n",x,x->transform_out);
    wtk_vecf_zero(x->transform_out);
}

wtk_vecf_t *wtk_nnet3_xvector_compute(wtk_nnet3_xvector_compute_t *x) {
    int i, j;
    float *pm;

    if (x->nn || x->cfg->use_onnx) {
        float *p = NULL;
        if (x->pool) {
            p = x->pool->xvector->m;
        } else {
            p = x->xvec->p;
        }
        float *p2 = x->transform_out->p;
        for (i = 0; i < x->transform_out->len; ++i) {
            *p2 = *p;
            p++;
            p2++;
        }
        // memcpy(x->transform_out->p,x->pool->xvector->m,sizeof(float)*x->transform_out->len);
        return x->transform_out;
    }

    for (i = 0; i < x->spk_mean->len; ++i) // test not need
    {
        x->spk_mean->p[i] /= x->spk_cnt;
        // printf("%d %f\n",i,x->spk_mean->p[i]);
    }

    if (x->cfg->mean_vec) {
#ifdef USE_NEON
        wtk_neon_math_vec_sub(x->spk_mean->p, x->spk_mean->p,
                              x->cfg->mean_vec->p, x->spk_mean->len);
#else
        for (i = 0; i < x->spk_mean->len; ++i) {
            x->spk_mean->p[i] -= x->cfg->mean_vec->p[i];
            // printf("%d %f\n",i,x->spk_mean->p[i]);
        }
#endif
    }

    if (x->cfg->transform) {
#ifdef USE_NEON
        wtk_neon_math_vecf_muti_matf_transf2(
            x->transform_out->p, x->cfg->transform->p, x->spk_mean->p,
            x->cfg->transform->row, x->cfg->transform->col);
#else
        pm = x->cfg->transform->p;
        for (i = 0; i < x->cfg->transform->row; ++i) {
            for (j = 0; j < x->spk_mean->len; ++j) {
                x->transform_out->p[i] += pm[j] * x->spk_mean->p[j];
            }
            // printf("%f\n",x->transform_out->p[i]);
            pm += x->cfg->transform->col;
        }
#endif
    }

    if (x->cfg->use_ivector_norm_len) {
        wtk_vecf_norm_len(x->transform_out);
        // wtk_vecf_print(x->transform_out);
    }

    return x->transform_out;
}

wtk_vecf_t* wtk_nnet3_xvector_compute_normalize(wtk_nnet3_xvector_compute_t *x)
{
    int i,j;
    float *pm;

    if(x->nn || x->cfg->use_onnx)
    {
        float *p = NULL;
        if(x->pool)
        {
        	p = x->pool->xvector->m;
        }else
        {
        	p = x->xvec->p;
        }
        float *p2 = x->transform_out->p;
        float scale = 0.0;

        //qtk_blas_matrix_print(x->pool->xvector);
        for(i=0;i<x->transform_out->len;++i)
        {
        	scale += *p * (*p);
        	p++;
        }
        if(scale < 0)
        {
        	scale = 0.0;
        }
        scale = pow(scale,0.5);

        if(x->pool)
        {
            p = x->pool->xvector->m;
        }else
        {
        	p = x->xvec->p;
        }

        for(i=0;i<x->transform_out->len;++i)
        {
        	*p2 = *p / scale;
        	p++;
        	p2++;
        }
    	//memcpy(x->transform_out->p,x->pool->xvector->m,sizeof(float)*x->transform_out->len);
        return x->transform_out;
    }

    for(i=0;i<x->spk_mean->len;++i) //test not need
    {
        x->spk_mean->p[i]/=x->spk_cnt;
        // printf("%d %f\n",i,x->spk_mean->p[i]);
    }

    if(x->cfg->mean_vec)
    {
#ifdef USE_NEON
    	wtk_neon_math_vec_sub(x->spk_mean->p,x->spk_mean->p,x->cfg->mean_vec->p,x->spk_mean->len);
#else
        for(i=0;i<x->spk_mean->len;++i)
        {
            x->spk_mean->p[i]-=x->cfg->mean_vec->p[i];
            // printf("%d %f\n",i,x->spk_mean->p[i]);
        }
#endif
    }

    if(x->cfg->transform)
    {
#ifdef USE_NEON
    	wtk_neon_math_vecf_muti_matf_transf2(x->transform_out->p,x->cfg->transform->p,\
    										x->spk_mean->p,x->cfg->transform->row,\
											x->cfg->transform->col);
#else
        pm=x->cfg->transform->p;
        for(i=0;i<x->cfg->transform->row;++i)
        {
            for(j=0;j<x->spk_mean->len;++j)
            {
               x->transform_out->p[i]+=pm[j]*x->spk_mean->p[j];
            }
            // printf("%f\n",x->transform_out->p[i]);
            pm+=x->cfg->transform->col;
        }
#endif
    }

    if(x->cfg->use_ivector_norm_len)
    {
        wtk_vecf_norm_len(x->transform_out);
        // wtk_vecf_print(x->transform_out);
    }

    return x->transform_out;
}
