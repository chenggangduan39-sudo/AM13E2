#include "qtk_ivector_cfg.h"

int qtk_ivector_cfg_init(qtk_ivector_cfg_t *cfg)
{
	cfg->lda_fn=0;
	cfg->ubm_fn=0;
	cfg->extractor_fn=0;
	cfg->lda_m=0;
	cfg->max_count=0;
	cfg->max_remembered_frames=1000;
	cfg->num_gselect=5;
	cfg->ivector_period=10;
	cfg->left_context=3;
	cfg->right_context=3;
	cfg->min_post=0.025;
	cfg->posterior_scale=0.1;
	cfg->num_cg_iters = 15;
	cfg->extractor = 0;
	cfg->diag_gmm = 0;
	return 0;
}

void qtk_ivector_cfg_extractor_delete(qtk_ivector_extractor_t* e)
{
	int i;
	wtk_free(e->gconst);
	wtk_free(e->w_vec);
	for(i = 0; i < e->msize; i++)
	{
		qtk_blas_double_matrix_delete(e->m[i]);
		qtk_sp_matrix_delete(e->sigma_inv[i]);
		qtk_blas_double_matrix_delete(e->sigma_inv_m[i]);
	}
	wtk_free(e->m);
	wtk_free(e->sigma_inv);
	wtk_free(e->sigma_inv_m);
	wtk_debug("%p\n",e);
	qtk_blas_double_matrix_delete(e->u);
	qtk_blas_double_matrix_delete(e->w);
	free(e);
}

void qtk_ivector_cfg_diag_delete(qtk_diaggmm_t *d)
{
	wtk_free(d->gconsts);
	wtk_free(d->weights);
	qtk_blas_matrix_delete(d->inv_vars);
	qtk_blas_matrix_delete(d->means_invvars);
	free(d);
}

int qtk_ivector_cfg_clean(qtk_ivector_cfg_t *cfg)
{
	if(cfg->lda_m)
	{
		qtk_blas_matrix_delete(cfg->lda_m);
	}
	if(cfg->extractor)
	{
		qtk_ivector_cfg_extractor_delete(cfg->extractor);
	}
	if(cfg->diag_gmm)
	{
		qtk_ivector_cfg_diag_delete(cfg->diag_gmm);
	}
	return 0;
}

int qtk_ivector_cfg_update_local(qtk_ivector_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_str(lc,cfg,lda_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,ubm_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,extractor_fn,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,max_count,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,max_remembered_frames,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,num_gselect,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,ivector_period,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,left_context,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,right_context,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,min_post,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,posterior_scale,v);


	wtk_local_cfg_update_cfg_b(lc,cfg,use_hist,v);
	return 0;
}

/*
int wtk_zmean_cfg_update(wtk_zmean_cfg_t *cfg)
{
	return wtk_zmean_cfg_update2(cfg,0);
}
*/

int qtk_ivector_extractor_load(qtk_ivector_extractor_t *extractor,wtk_source_t *s)
{
	wtk_strbuf_t *buf;
	int ret,n,row,col,i;

	buf=wtk_strbuf_new(32,1);
	ret=wtk_source_read_string(s,buf);//<IvectorExtractor>
//	wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if(ret!=0){goto end;}

	ret=wtk_source_read_string(s,buf);//<w>
//	wtk_debug("[%.*s]\n",buf->pos,buf->data);
	ret=wtk_source_read_string(s,buf);
//	wtk_debug("[%.*s]\n",buf->pos,buf->data);
	ret = wtk_source_read_char(s);
	ret = wtk_source_read_char(s);
	ret = wtk_source_read_int_little(s, &row, 1, 1);
	ret = wtk_source_read_char(s);
	ret = wtk_source_read_int_little(s, &col, 1, 1);
	extractor->w=qtk_blas_double_matrix_new(row,col);
	ret = wtk_source_read_double_little(s, extractor->w->m, row * col, 1);

	ret=wtk_source_read_string(s,buf);//<w_vec>
//	wtk_debug("[%.*s]\n",buf->pos,buf->data);
	ret=wtk_source_read_string(s,buf);
//	wtk_debug("[%.*s]\n",buf->pos,buf->data);
	ret = wtk_source_read_char(s);
	ret = wtk_source_read_char(s);
	ret = wtk_source_read_int_little(s, &row, 1, 1);
	extractor->w_vec=wtk_double_vector_new(row);
	ret = wtk_source_read_double_little(s, extractor->w_vec+1, row, 1);

	ret=wtk_source_read_string(s,buf);//<M>
//	wtk_debug("[%.*s]\n",buf->pos,buf->data);
	ret = wtk_source_read_char(s);
	ret = wtk_source_read_char(s);
	ret = wtk_source_read_int_little(s, &n, 1, 1);
	extractor->msize = n;
	extractor->m=(qtk_blas_double_matrix_t**)wtk_calloc(n,sizeof(qtk_blas_double_matrix_t*));
	//wtk_debug("%d\n",n);
	for(i=0;i<n;i++)
	{
		ret=wtk_source_read_string(s,buf);
		//wtk_debug("[%.*s]\n",buf->pos,buf->data);
		ret = wtk_source_read_char(s);
		ret = wtk_source_read_char(s);
		ret = wtk_source_read_int_little(s, &row, 1, 1);
		ret = wtk_source_read_char(s);
		ret = wtk_source_read_int_little(s, &col, 1, 1);
		extractor->m[i]=qtk_blas_double_matrix_new(row,col);
		ret = wtk_source_read_double_little(s, extractor->m[i]->m, row * col, 1);
	}

	ret=wtk_source_read_string(s,buf);//<SigmaInv>
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	extractor->sigma_inv=(qtk_sp_matrix_t**)wtk_calloc(n,sizeof(qtk_sp_matrix_t*));
	for(i=0;i<n;i++)
	{
		ret=wtk_source_read_string(s,buf);//DP
		ret = wtk_source_read_char(s);
		ret = wtk_source_read_char(s);
		ret = wtk_source_read_int_little(s, &row, 1, 1);
		int num_elements = (row+1)*row/2;
		extractor->sigma_inv[i]=qtk_sp_matrix_new(row);
		ret = wtk_source_read_double_little(s, extractor->sigma_inv[i]->data, num_elements, 1);
	}
	//TODO read sparse matrix

	ret=wtk_source_read_string(s,buf);//<IvectorOffset>
//	wtk_debug("[%.*s]\n",buf->pos,buf->data);
	ret = wtk_source_read_char(s);
	ret = wtk_source_read_char(s);
	wtk_source_read_double_little(s,&(extractor->prior_offset),1,1);

	//ret=wtk_source_read_string(s,buf);//</IvectorExtractor>
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	//TODO ComputeDerivedVars
	//qtk_ivector_extrac_ComputeDerivedVars(extractor);
	//change to read file
	ret=wtk_source_read_string(s,buf);//<Gconsts>
//	wtk_debug("[%.*s]\n",buf->pos,buf->data);
	ret=wtk_source_read_string(s,buf);
	ret = wtk_source_read_char(s);
	ret = wtk_source_read_char(s);
	ret = wtk_source_read_int_little(s, &row, 1, 1);
	extractor->gconst=(double*)wtk_malloc(sizeof(double)*row);//wtk_double_vector_new(row);
	ret = wtk_source_read_double_little(s, extractor->gconst, row, 1);

	ret=wtk_source_read_string(s,buf);//<U>
//	wtk_debug("[%.*s]\n",buf->pos,buf->data);
	ret=wtk_source_read_string(s,buf);
	ret = wtk_source_read_char(s);
	ret = wtk_source_read_char(s);
	ret = wtk_source_read_int_little(s, &row, 1, 1);
	ret = wtk_source_read_char(s);
	ret = wtk_source_read_int_little(s, &col, 1, 1);
	extractor->u=qtk_blas_double_matrix_new(row,col);
	wtk_debug("%p\n",extractor);
	ret = wtk_source_read_double_little(s, extractor->u->m, row * col, 1);

	ret=wtk_source_read_string(s,buf);//<SigmaInvM>
//	wtk_debug("[%.*s]\n",buf->pos,buf->data);
	ret = wtk_source_read_char(s);
	ret = wtk_source_read_char(s);
	ret = wtk_source_read_int_little(s, &n, 1, 1);
	extractor->ssize = n;
	extractor->sigma_inv_m=(qtk_blas_double_matrix_t**)wtk_calloc(n,sizeof(qtk_blas_double_matrix_t*));
	for(i=0;i<n;i++)
	{
		ret=wtk_source_read_string(s,buf);
		ret = wtk_source_read_char(s);
		ret = wtk_source_read_char(s);
		ret = wtk_source_read_int_little(s, &row, 1, 1);
		ret = wtk_source_read_char(s);
		ret = wtk_source_read_int_little(s, &col, 1, 1);
		extractor->sigma_inv_m[i]=qtk_blas_double_matrix_new(row,col);
		ret = wtk_source_read_double_little(s, extractor->sigma_inv_m[i]->m, row * col, 1);
	}

	ret=wtk_source_read_string(s,buf);//<SIvectorEtractor>
//	wtk_debug("[%.*s]\n",buf->pos,buf->data);

	end:
		wtk_strbuf_delete(buf);
		return ret;
}

void qtk_dlag_gmm_compute_gconst(qtk_diaggmm_t *dlag)
{
	int num_mix=wtk_vector_size(dlag->weights);
	int dim=dlag->means_invvars->col;
	float offset=-0.5*M_LOG_2PI*dim;
	qtk_blas_matrix_t *inv_vars=dlag->inv_vars;
	qtk_blas_matrix_t *means_invvars=dlag->means_invvars;

	if(num_mix!=wtk_vector_size(dlag->gconsts))
	{
		dlag->gconsts[0]=num_mix;
	}

	int mix,d;
	float gc,mean,vars;
	for(mix=0;mix<num_mix;mix++)
	{
		gc=logf(dlag->weights[mix+1])+offset;
		for(d=0;d<dim;d++)
		{
			mean=*(means_invvars->m+means_invvars->col*mix+d);
			vars=*(inv_vars->m+inv_vars->col*mix+d);
			gc+=0.5*logf(vars)-0.5*mean*mean/vars;
		}
		//TODO gc is none
		dlag->gconsts[1+mix]=gc;
	}
	dlag->valid_gconst=1;
}

int qtk_diag_gmm_load(qtk_diaggmm_t *dlag,wtk_source_t *s)
{
	wtk_strbuf_t *buf;
	int ret,row,col;

	buf=wtk_strbuf_new(32,1);
	ret=wtk_source_read_string(s,buf);//<DiagGMMBegin> or <DiagGMM>
	if(ret!=0){goto end;}
	ret=wtk_source_read_string(s,buf);
	if(wtk_str_equal_s(buf->data,buf->pos,"<GCONSTS>"))
	{
		ret=wtk_source_read_string(s,buf);
		ret = wtk_source_read_char(s);
		ret = wtk_source_read_char(s);
		ret = wtk_source_read_int_little(s, &row, 1, 1);
		dlag->gconsts=wtk_vector_new(row);
		ret = wtk_source_read_float_little(s, dlag->gconsts+1, 1 * row, 1);

		ret=wtk_source_read_string(s,buf);
	}
//	wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if(!wtk_str_equal_s(buf->data,buf->pos,"<WEIGHTS>"))
	{
		if(ret!=0){goto end;}
	}
	ret=wtk_source_read_string(s,buf);
//	wtk_debug("[%.*s]\n",buf->pos,buf->data);
	ret = wtk_source_read_char(s);
	ret = wtk_source_read_char(s);
	ret = wtk_source_read_int_little(s, &row, 1, 1);
	dlag->weights=wtk_vector_new(row);
	ret = wtk_source_read_float_little(s, dlag->weights+1, 1 * row, 1);

	ret=wtk_source_read_string(s,buf);//<MEANS_INVVARS>
//	wtk_debug("[%.*s]\n",buf->pos,buf->data);
	ret=wtk_source_read_string(s,buf);
//	wtk_debug("[%.*s]\n",buf->pos,buf->data);
	ret = wtk_source_read_char(s);
	ret = wtk_source_read_char(s);
	ret = wtk_source_read_int_little(s, &row, 1, 1);
	ret = wtk_source_read_char(s);
	ret = wtk_source_read_int_little(s, &col, 1, 1);
	dlag->means_invvars=qtk_blas_matrix_new(row,col);
	ret = wtk_source_read_float_little(s, dlag->means_invvars->m, row * col, 1);

	ret=wtk_source_read_string(s,buf);//<INV_VARS>
//	wtk_debug("[%.*s]\n",buf->pos,buf->data);
	ret=wtk_source_read_string(s,buf);
	ret = wtk_source_read_char(s);
	ret = wtk_source_read_char(s);
	ret = wtk_source_read_int_little(s, &row, 1, 1);
	ret = wtk_source_read_char(s);
	ret = wtk_source_read_int_little(s, &col, 1, 1);
	dlag->inv_vars=qtk_blas_matrix_new(row,col);
	ret = wtk_source_read_float_little(s, dlag->inv_vars->m, row * col, 1);

	ret=wtk_source_read_string(s,buf);//<DiagGMMEnd> or </DiagGMM>
//	wtk_debug("[%.*s]\n",buf->pos,buf->data);

	qtk_dlag_gmm_compute_gconst(dlag);

end:
	wtk_strbuf_delete(buf);
	return ret;
}

int qtk_ivector_lda_load(qtk_ivector_cfg_t *i,wtk_source_t *s)
{
	int ret,row,col;
	wtk_strbuf_t *buf = wtk_strbuf_new(1024,1);

	ret = wtk_source_read_char(s);
	ret = wtk_source_read_char(s);
	ret=wtk_source_read_string(s,buf);//FM
	ret = wtk_source_read_char(s);
	ret = wtk_source_read_char(s);
	ret = wtk_source_read_int_little(s, &row, 1, 1);
	ret = wtk_source_read_char(s);
	ret = wtk_source_read_int_little(s, &col, 1, 1);
	i->lda_m = qtk_blas_matrix_new(row,col);
	ret = wtk_source_read_float_little(s, i->lda_m->m, row * col, 1);
//	wtk_debug("%d %d\n",row,col);
	wtk_strbuf_delete(buf);
	return ret;
}

int qtk_ivector_cfg_update(qtk_ivector_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret=0;

	if(!cfg->extractor_fn)
	{
		ret=0;
		goto end;
	}
	cfg->extractor = (qtk_ivector_extractor_t*)wtk_malloc(sizeof(qtk_ivector_extractor_t));
	ret=wtk_source_loader_load(sl, (cfg->extractor),(wtk_source_load_handler_t)qtk_ivector_extractor_load, cfg->extractor_fn);

	if(!cfg->ubm_fn)
	{
		ret=0;
		goto end;
	}
	cfg->diag_gmm = (qtk_diaggmm_t*)wtk_malloc(sizeof(qtk_diaggmm_t));
	ret=wtk_source_loader_load(sl, (cfg->diag_gmm),(wtk_source_load_handler_t)qtk_diag_gmm_load, cfg->ubm_fn);

	if(ret!=0){goto end;}

	if(!cfg->lda_fn)
	{
		ret=0;
		goto end;
	}
	ret=wtk_source_loader_load(sl, cfg,(wtk_source_load_handler_t)qtk_ivector_lda_load, cfg->lda_fn);

	if(ret!=0){goto end;}
end:
	return ret;
}
