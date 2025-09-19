#include "qtk_ivector.h"
#include "wtk/asr/fextra/kcmn/qtk_kcmn.h"
//#ifdef USE_BLAS
//#   include "GotoBLAS2/common.h"
//#   include "GotoBLAS2/cblas.h"
//#endif
//void qtk_sp_logposdefdet(qtk_sp_matrix_t* sp)
//{
//
//}
//
//void qtk_ivector_extrac_ComputeDerivedVars(qtk_ivector_extractor_t *extractor)
//{
//	extractor->gconst = (double*)wtk_malloc(sizeof(double)*extractor->msize);
//	int i;
//	int dim = extractor->m[0]->row;
//	for(i = 0; i < extractor->msize;i ++)
//	{
//		double var_logdet =1; //extractor->sigma_inv[i]
//		extractor->gconst[i] = -0.5 * (var_logdet + dim*M_LOG_2PI);
//	}
//
//}

int qtk_ivector_post_set_lda_norm(
    qtk_ivector_t *pp, int (*frame_ready)(void *ud),
    void (*get_frame)(void *, int frame, float* feat,int dim),
	void (*get_frames)(void *, qtk_array_t *array, qtk_blas_matrix_t *mat),
    int (*is_last_frame)(void *ud, int frame),
	int (*dim)(void *),void *ud)
{
    pp->lda_normalized.ud = ud;
    pp->lda_normalized.frame_ready = frame_ready;
    pp->lda_normalized.get_frame = get_frame;
    pp->lda_normalized.get_frames = get_frames;
    pp->lda_normalized.is_last_frame = is_last_frame;
    pp->lda_normalized.dim = dim;
    return 0;
}

int qtk_ivector_post_set_lda(
		qtk_ivector_t *pp, int (*frame_ready)(void *ud),
    void (*get_frame)(void *, int frame, float* feat,int dim),
	void (*get_frames)(void *, qtk_array_t *array, qtk_blas_matrix_t *mat),
    int (*is_last_frame)(void *ud, int frame),
	int (*dim)(void *),void *ud)
{
    pp->lda.ud = ud;
    pp->lda.frame_ready = frame_ready;
    pp->lda.get_frame = get_frame;
    pp->lda.get_frames = get_frames;
    pp->lda.is_last_frame = is_last_frame;
    pp->lda_normalized.dim = dim;
    return 0;
}

qtk_ivector_estimation_stats_t* qtk_ivector_estimation_stats_new(int ivector_dim,float prior_offset,float max_count)
{
	qtk_ivector_estimation_stats_t* stats = (qtk_ivector_estimation_stats_t*)wtk_malloc(sizeof(qtk_ivector_estimation_stats_t));

	stats->prior_offset = prior_offset;
	stats->max_count = max_count;
	stats->num_frames = 0.0;
	stats->quadratic_term = qtk_sp_matrix_new(ivector_dim);
	stats->linear_term = qtk_blas_double_vector_new(ivector_dim);
	if(ivector_dim != 0)
	{
		stats->linear_term->m[0] += prior_offset;
		qtk_sp_matrix_diag(stats->quadratic_term,1.0);
	}
	return stats;
}

qtk_ivector_t* qtk_ivector_new(qtk_ivector_cfg_t *cfg,void* cmn)
{
	qtk_ivector_t* i = (qtk_ivector_t*)wtk_malloc(sizeof(qtk_ivector_t));

	i->cfg = cfg;
	i->cmn = cmn;
	i->diag_gmm = cfg->diag_gmm;
	i->extractor = cfg->extractor;
	i->lda_m = cfg->lda_m;

	i->estimationstats = qtk_ivector_estimation_stats_new(i->extractor->m[0]->col,i->extractor->prior_offset,cfg->max_count);
    //wtk_debug("%d\n",i->extractor->m[0]->col);
	i->current_ivector = (double*)wtk_calloc((i->extractor->m[0]->col),sizeof(double));
	i->ivector_history = qtk_array_new(2,sizeof(float*));
	i->num_frames_stats = 0;
	i->most_recent_frame_with_weight = -1;
	i->tot_ubm_loglike = 0.0;
	i->delta_weights_provided = 0;
	i->updated_with_no_delta_weights = 0;
	i->heap = wtk_heap_new(4096);
	//lda sourcer:
	qtk_cache_feature_t *cache = qtk_cache_feature_new();
	qtk_splice_feature_t *splice = qtk_splice_feature_new(cfg->left_context,cfg->right_context);
	qtk_transform_feature_t *transform = qtk_transform_feature_new(i->lda_m);
	qtk_ivector_post_set_lda(
	        i, cast(int (*)(void *), qtk_cache_feature_num_frames_ready),
	        NULL,
			cast(void (*)(void *, qtk_array_t *, qtk_blas_matrix_t *), qtk_cache_feature_get_frames),
	        cast(int (*)(void *, int), qtk_cache_feature_is_last_frame),
			cast(int (*)(void *),qtk_cache_feature_dim),cache);

	qtk_cache_set_sourcer(
    		cache, cast(int (*)(void *), qtk_transform_feature_is_last_frame),
			NULL,
			cast(void (*)(void *, qtk_array_t *, qtk_blas_matrix_t *), qtk_transform_feature_get_frames),
			cast(int (*)(void *, int), qtk_transform_feature_num_frames_ready),
			cast(int (*)(void *),qtk_transform_feature_dim),transform);

	qtk_transform_set_sourcer(
    		transform, cast(int (*)(void *), qtk_splice_feature_num_frames_ready),
			cast(void (*)(void *, int, float *,int), qtk_splice_feature_get_frame),
			cast(void (*)(void *, qtk_array_t *, qtk_blas_matrix_t *), qtk_splice_feature_get_frames),
			cast(int (*)(void *, int), qtk_splice_feature_is_last_frame),
			cast(int (*)(void *),qtk_splice_feature_dim), splice);

	qtk_splice_set_sourcer(
        splice,cast(int (*)(void *), qtk_kcmn_frame_ready),
        cast(void (*)(void *, int, float *,int), qtk_kcmn_get_raw_frame),
        NULL,cast(int (*)(void *, int), qtk_kcmn_last_frame),
		cast(int (*)(void *),qtk_kcmn_dim),i->cmn);

	//lda normalize sourcer:
	qtk_cache_feature_t *cache2 = qtk_cache_feature_new();
	qtk_splice_feature_t *splice2 = qtk_splice_feature_new(cfg->left_context,cfg->right_context);
	qtk_transform_feature_t *transform2 = qtk_transform_feature_new(i->lda_m);
	qtk_ivector_post_set_lda_norm(
	        i, cast(int (*)(void *), qtk_cache_feature_num_frames_ready),
	        NULL,
			cast(void (*)(void *, qtk_array_t *, qtk_blas_matrix_t *), qtk_cache_feature_get_frames),
	        cast(int (*)(void *, int), qtk_cache_feature_is_last_frame),
			cast(int (*)(void *),qtk_cache_feature_dim), cache2);

	qtk_cache_set_sourcer(
    		cache2, cast(int (*)(void *), qtk_transform_feature_is_last_frame),
			NULL,
			cast(void (*)(void *, qtk_array_t *, qtk_blas_matrix_t *), qtk_transform_feature_get_frames),
			cast(int (*)(void *, int), qtk_transform_feature_num_frames_ready),
			cast(int (*)(void *),qtk_transform_feature_dim), transform2);

	qtk_transform_set_sourcer(
    		transform2, cast(int (*)(void *), qtk_splice_feature_num_frames_ready),
			cast(void (*)(void *, int, float *,int), qtk_splice_feature_get_frame),
			cast(void (*)(void *, qtk_array_t *, qtk_blas_matrix_t *), qtk_splice_feature_get_frames),
			cast(int (*)(void *, int), qtk_splice_feature_is_last_frame),
			cast(int (*)(void *),qtk_splice_feature_dim),splice2);

	qtk_splice_set_sourcer(
        splice2,cast(int (*)(void *), qtk_kcmn_frame_ready),
        cast(void (*)(void *, int, float *,int), qtk_kcmn_get_out_frame),
        NULL,cast(int (*)(void *, int), qtk_kcmn_last_frame),
		cast(int (*)(void *),qtk_kcmn_dim),i->cmn);

	i->source.cache1 = cache;
	i->source.cache2 = cache2;
	i->source.splice1 = splice;
	i->source.splice2 = splice2;
	i->source.trans1 = transform;
	i->source.trans2 = transform2;
	return i;
}

void qtk_ivector_pair_delete(qtk_array_t* array)
{
//	ivector_pair_t** p;
//	int i;
//	for(i = 0; i < array->dim; i++)
//	{
//		p = qtk_array_get(array,i);
//		wtk_free(*p);
//	}
	qtk_array_delete(array);
}


void qtk_ivector_gauss_info_delete(qtk_array_t* array)
{
	ivector_gauss_t** g;
	int i;
	for(i = 0; i < array->dim; i++)
	{
		g = qtk_array_get(array,i);
		qtk_ivector_pair_delete((*g)->pairs);
		wtk_free(*g);
	}
	qtk_array_delete(array);
}

void qtk_ivector_estimationstats_delete(qtk_ivector_estimation_stats_t* e)
{
	qtk_sp_matrix_delete(e->quadratic_term);
	qtk_blas_double_vector_delete(e->linear_term);
	wtk_free(e);
}

void qtk_ivector_estimationstats_reset(qtk_ivector_estimation_stats_t* e,int ivector_dim)
{
	int row = e->quadratic_term->num_rows;
	int num_elements = (row+1)*row/2;

	memset(e->linear_term->m,0,sizeof(double)*e->linear_term->dim);
	memset(e->quadratic_term->data,0,sizeof(double)*num_elements);
	if(ivector_dim != 0)
	{
		e->linear_term->m[0] += e->prior_offset;
		qtk_sp_matrix_diag(e->quadratic_term,1.0);
	}
}

void qtk_ivector_sourcer_delete(qtk_ivector_sourcers_t* s)
{
	qtk_cache_feature_delete(s->cache1);
	qtk_cache_feature_delete(s->cache2);
	qtk_splice_feature_delete(s->splice1);
	qtk_splice_feature_delete(s->splice2);
	qtk_transform_feature_delete(s->trans1);
	qtk_transform_feature_delete(s->trans2);
}

void qtk_ivector_sourcer_reset(qtk_ivector_sourcers_t* s)
{
	qtk_cache_feature_reset(s->cache1);
	qtk_cache_feature_reset(s->cache2);
	//qtk_splice_feature_reset(s->splice1);
	//qtk_splice_feature_reset(s->splice2);
	//qtk_transform_feature_reset(s->trans1);
	//qtk_transform_feature_reset(s->trans2);
}

int qtk_ivector_delete(qtk_ivector_t *i)
{
	int id;
	float **f;
	qtk_ivector_sourcer_delete(&(i->source));
	qtk_ivector_estimationstats_delete(i->estimationstats);
	wtk_free(i->current_ivector);
	for(id = 0; id < i->ivector_history->dim; id++)
	{
		f = qtk_array_get(i->ivector_history,id);
		wtk_free(*f);
	}
	qtk_array_delete(i->ivector_history);
	wtk_heap_delete(i->heap);
	wtk_free(i);
	return 0;
}
int qtk_ivector_reset(qtk_ivector_t *i)
{
	int id;
	float **f;
	memset(i->current_ivector,0,sizeof(double)*(i->extractor->m[0]->col));
	i->num_frames_stats = 0;
	i->most_recent_frame_with_weight = -1;
	i->tot_ubm_loglike = 0.0;
	i->delta_weights_provided = 0;
	i->updated_with_no_delta_weights = 0;
	for(id = 0; id < i->ivector_history->dim; id++)
	{
		f = qtk_array_get(i->ivector_history,id);
		wtk_free(*f);
	}
	qtk_array_clear(i->ivector_history);
	wtk_heap_reset(i->heap);

	qtk_ivector_sourcer_reset(&(i->source));
	qtk_ivector_estimationstats_reset(i->estimationstats,i->extractor->m[0]->col);
	//qtk_ivector_extractor_reset(i->extractor);

	return 0;
}

void qtk_PackedMatrix_AddToDiag(qtk_sp_matrix_t* mat, double r)
{
  double *ptr = mat->data;
  int i;
  for (i = 2; i <= mat->num_rows+1; i++) {
    *ptr += r;
    ptr += i;
  }
}

void qtk_dspmv(float scale1,float scale2,double *in,int dim,double* s,double* out)
{
	int i,j;
	double *p,*p2;
	p = out;
	for(i=0;i<dim;i++,p++)
	{
		*p = *p * scale2;
	}

	p = in;
	p2 = out;
	for(i=0;i<dim;i++)
	{
		for(j=0;j<i+1;j++)
		{

			p2[i] += scale1*(*p)*s[j];
			if(i!=j)
			{
				//p2[i] += scale1*(*p)*s[j];
				p2[j] += scale1*(*p)*s[i];
			}
			p++;
		}
	}
}

void qtk_ivector_linear_cgd(qtk_ivector_t* i,int num_cg_iters,double* x)
{
	qtk_sp_matrix_t* A = i->estimationstats->quadratic_term;
	qtk_blas_double_vector_t*b = i->estimationstats->linear_term;
	int M = A->num_rows;
	int min;
	qtk_blas_double_matrix_t *storage = qtk_blas_double_matrix_new(4,M);
	qtk_blas_double_vector_t *r = (qtk_blas_double_vector_t*)wtk_malloc(sizeof(qtk_blas_double_vector_t));
	qtk_blas_double_vector_t *p = (qtk_blas_double_vector_t*)wtk_malloc(sizeof(qtk_blas_double_vector_t));
	qtk_blas_double_vector_t *Ap = (qtk_blas_double_vector_t*)wtk_malloc(sizeof(qtk_blas_double_vector_t));
	qtk_blas_double_vector_t *x_org = (qtk_blas_double_vector_t*)wtk_malloc(sizeof(qtk_blas_double_vector_t));
	r->dim = p->dim = Ap->dim = x_org->dim = M;
	r->m = storage->m;p->m = storage->m + storage->col;Ap->m = storage->m + storage->col*2;x_org->m = storage->m + storage->col*3;

	min = min(p->dim,b->dim);
	memcpy(p->m,b->m,min*sizeof(double));
	//TODO p.AddSpVec(-1.0, A, *x, 1.0);  // p_0 = b - A x_0
//#ifdef USE_BLAS
	//cblas_dspmv(CblasRowMajor,CblasLower,A->num_rows,-1.0,A->data,x,1,1.0,p->m,1);
//#else
	qtk_dspmv(-1.0,1.0,A->data,A->num_rows,x,p->m);
//#endif
	//wtk_debug("%d %d\n",A->num_rows,p->dim);
	//print_double(p->m,p->dim);
	//print_double(A->data,p->dim);
	//r.AddVec(-1.0, p);  // r_0 = - p_0
	qtk_blas_addvecd(r,-1.0,p->m);
//	wtk_debug("!!!!!!!!!!\n");
//	int xx;
//	for(xx=0;xx<100;xx++)
//	{
//		wtk_debug("%d:%f\n",xx,*(x+xx));
//	}
	memcpy(x_org->m,x,min*sizeof(double));

	double r_cur_norm_sq = qtk_blas_vecvec(r,r);
	double r_initial_norm_sq = r_cur_norm_sq;
	double r_recompute_norm_sq = r_cur_norm_sq;

	double max_error_sq = 0.0;
	double residual_factor = 0.01 * 0.01;
	double inv_residual_factor = 1.0/residual_factor;
	int t;
	int k = 0;
//	wtk_debug("%d %d %d\n",k,M,num_cg_iters);
	for(; k < M+5 && k != num_cg_iters; k++)
	{
	    //Ap.AddSpVec(1.0, A, p, 0.0);  // Ap = A p
//#ifdef USE_BLAS
		//cblas_dspmv(CblasRowMajor,CblasLower,A->num_rows,1.0,A->data,p->m,1,0.0,Ap->m,1);
	    qtk_dspmv(1.0,0.0,A->data,A->num_rows,p->m,Ap->m);
//#endif
//		wtk_debug("qqqqqq\n");
//		for(xx=0;xx<Ap->dim;xx++)
//		{
//			wtk_debug("%d:%f\n",xx,*(Ap->m+xx));
//		}

		double alpha = -qtk_blas_vecvec(p,r)/qtk_blas_vecvec(p,Ap);
		//qtk_blas_addvecd(x,alpha,p->m);
		for(t = 0; t < p->dim; t++)
		{
			*(x+t) += (*(p->m+t))*alpha;
		}
		qtk_blas_addvecd(r,alpha,Ap->m);
		double r_next_norm_sq = qtk_blas_vecvec(r,r);

	    if (r_next_norm_sq < residual_factor * r_recompute_norm_sq ||
	        r_next_norm_sq > inv_residual_factor * r_recompute_norm_sq) {
	      // r_{k+1} = A x_{k+1} - b
	      //r.AddSpVec(1.0, A, *x, 0.0);
//#ifdef USE_BLAS
		  //cblas_dspmv(CblasRowMajor,CblasLower,A->num_rows,1.0,A->data,x,1,0.0,r->m,1);
	      qtk_dspmv(1.0,0.0,A->data,A->num_rows,x,r->m);
//#endif
	      qtk_blas_addvecd(r,-1.0,b->m);
	      r_next_norm_sq = qtk_blas_vecvec(r, r);
	      r_recompute_norm_sq = r_next_norm_sq;
	    }

	    if (r_next_norm_sq <= max_error_sq)
	      break;
	    double beta_next = r_next_norm_sq / r_cur_norm_sq;
	    // next lines: p_{k+1} = -r_{k+1} + \beta_{k+1} p_k
	    //qtk_blas_double_vector_t *p_old = (qtk_blas_double_vector_t*)wtk_malloc(sizeof(qtk_blas_double_vector_t));
	    //p_old->dim = p->dim;
	    //memcpy(p_old->m,p->m;sizeof(double)*p->dim);
//	    p.Scale(beta_next);
//	    p.AddVec(-1.0, r);
		for(t = 0; t < p->dim; t++)
		{
			(*(p->m+t)) *= beta_next;
		}

		for(t = 0; t < p->dim; t++)
		{
			(*(p->m+t)) += (*(r->m+t))*(-1.0);
		}
	    r_cur_norm_sq = r_next_norm_sq;
	}

	  if (r_cur_norm_sq > r_initial_norm_sq &&
	      r_cur_norm_sq > r_initial_norm_sq + 1.0e-10 * qtk_blas_vecvec(b, b)) {
	    //SolverOptions opts("called-from-linearCGD");
	    memcpy(x,x_org->m,x_org->dim*sizeof(double));
	    //SolveQuadraticProblem(A, b, opts, x); TODO
	  }
	  wtk_free(r);
	  wtk_free(p);
	  wtk_free(Ap);
	  wtk_free(x_org);
	  qtk_blas_double_matrix_delete(storage);
}

void qtk_ivector_stats_get_ivector(qtk_ivector_t* i,int num_cg_iters,double* ivector)
{
	int dim = i->extractor->m[0]->col;
	if(i->estimationstats->num_frames > 0.0)
	{
		if(*ivector == 0.0)
			*ivector = i->estimationstats->prior_offset;
		qtk_ivector_linear_cgd(i,num_cg_iters,ivector);
//		wtk_debug("zzzzzzzz\n");
//		int x;
//		for(x=0;x<100;x++)
//		{
//			wtk_debug("%d:%f\n",x,*(ivector+x));
//		}
	}else
	{
		memset(ivector,0,sizeof(double)*dim);
		*ivector = i->estimationstats->prior_offset;
	}
}

ivector_gauss_t* qtk_ivector_gauss_find(qtk_array_t* gauss_infos,int gauss_idx)
{
	int i;
	ivector_gauss_t* gauss_info=NULL;
	ivector_gauss_t** tmp;
	for(i = 0; i<gauss_infos->dim; i++)
	{
		tmp = qtk_array_get(gauss_infos,i);
		if((*tmp)->gauss_idx == gauss_idx)
		{
			gauss_info = *tmp;
			break;
		}
	}
	return gauss_info;
}

void qtk_ivector_acc_stats(qtk_ivector_t* i,qtk_blas_matrix_t* features,qtk_array_t** gauss_post,int num_frames)
{
	int feat_dim = features->col;
	qtk_blas_double_vector_t* weighted_feats = qtk_blas_double_vector_new(feat_dim);
	double tot_weight = 0.0;
	int ivector_dim = i->estimationstats->linear_term->dim;
	int quadratic_term_dim = (ivector_dim * (ivector_dim + 1)) / 2;
	qtk_blas_double_vector_t*quadratic_term_vec = (qtk_blas_double_vector_t*)wtk_malloc(sizeof(qtk_blas_double_vector_t));
	quadratic_term_vec->dim = quadratic_term_dim;
	quadratic_term_vec->m = i->estimationstats->quadratic_term->data;
	ivector_pair_t **pair;
	ivector_pair_t *p;
	qtk_array_t* a;
	int gauss_idx,idx,gidx;

	//covert to gauss info
	qtk_array_t* gauss_infos = qtk_array_new(2,sizeof(ivector_gauss_t*));
	ivector_gauss_t *gauss_info=NULL;
	ivector_gauss_t **g;
	for(gauss_idx = 0; gauss_idx < num_frames; gauss_idx++)
	{
		a = gauss_post[gauss_idx];
		for(idx = 0; idx < a->dim; idx++)
		{
			pair = qtk_array_get(a, idx);
        	int t = (*pair)->stats;
        	float weight = (*pair)->frame_weight;
        	gauss_info = qtk_ivector_gauss_find(gauss_infos,t);
        	//wtk_debug("%p\n",gauss_info);
			p = (ivector_pair_t*)wtk_heap_malloc(i->heap,sizeof(ivector_pair_t));
			p->stats = gauss_idx;
			p->frame_weight = weight;
			if(gauss_info)
			{
				gauss_info->tot_weight += weight;
				qtk_array_push(gauss_info->pairs,&p);
			}else
			{
				gauss_info = (ivector_gauss_t*)wtk_malloc(sizeof(ivector_gauss_t));
				gauss_info->tot_weight = weight;
				gauss_info->gauss_idx = t;
				gauss_info->pairs = qtk_array_new(2,sizeof(ivector_pair_t*));
				qtk_array_push(gauss_info->pairs,&p);
				qtk_array_push(gauss_infos,&gauss_info);
			}
		}
	}

    for(gauss_idx = 0; gauss_idx < gauss_infos->dim; gauss_idx++)
    {
    	g = qtk_array_get(gauss_infos,gauss_idx);
    	a = (*g)->pairs;
    	gidx = (*g)->gauss_idx;
    	//wtk_debug("%d\n",gidx);
    	memset(weighted_feats->m,0,sizeof(double)*weighted_feats->dim);
        for(idx = 0; idx < a->dim; idx++)
        {
        	pair = qtk_array_get(a, idx);
        	int t = (*pair)->stats;
        	float weight = (*pair)->frame_weight;
        	//wtk_debug("%d %f\n",t,weight);
        	qtk_blas_addvec(weighted_feats,weight,features->m+features->col*t);
        }
        //wtk_debug("111111111 %d\n",gidx);
        //qtk_blas_double_vec_print(weighted_feats);
        float this_tot_weight = (*g)->tot_weight;
        qtk_blas_addmatvec(i->estimationstats->linear_term,1.0,i->extractor->sigma_inv_m[gidx],weighted_feats);

    	qtk_blas_double_vector_t*U_g = (qtk_blas_double_vector_t*)wtk_malloc(sizeof(qtk_blas_double_vector_t));
    	U_g->dim = i->extractor->u->col;
    	U_g->m = i->extractor->u->m+i->extractor->u->col*gidx;
    	qtk_blas_addvecd(quadratic_term_vec,this_tot_weight,U_g->m);
    	tot_weight += this_tot_weight;
    	wtk_free(U_g);
    	//wtk_debug("%f %f\n",tot_weight,this_tot_weight);
    }

    if(i->estimationstats->max_count > 0.0)
    {
    	//wtk_debug("????????\n");
        double old_num_frames = i->estimationstats->num_frames;
        double new_num_frames = i->estimationstats->num_frames + tot_weight;
        double old_prior_scale = max(old_num_frames, i->estimationstats->max_count) / i->estimationstats->max_count;
        double new_prior_scale = max(new_num_frames, i->estimationstats->max_count) / i->estimationstats->max_count;
        double prior_scale_change = new_prior_scale - old_prior_scale;
        if (prior_scale_change != 0.0) {
        	//wtk_debug("????????\n");
        	*(i->estimationstats->linear_term->m) += i->estimationstats->prior_offset * prior_scale_change;
          qtk_PackedMatrix_AddToDiag(i->estimationstats->quadratic_term,prior_scale_change); //TODO
        }
    }
    i->estimationstats->num_frames += tot_weight;
    //wtk_debug("accst %f\n",i->estimationstats->num_frames);
    //qtk_blas_double_vec_print(i->estimationstats->linear_term);
    //qtk_blas_double_vec_print(quadratic_term_vec);
    qtk_blas_double_vector_delete(weighted_feats);
    wtk_free(quadratic_term_vec);
    qtk_ivector_gauss_info_delete(gauss_infos);
}

void qtk_ivector_diagubm_loglikehoods(qtk_diaggmm_t *diag, qtk_blas_matrix_t* data, qtk_blas_matrix_t* dst )
{
    qtk_sub_matrix_t sub_dst;
    qtk_sub_matrix_init2(&sub_dst, dst->m, dst->row, dst->col, dst->col);
    qtk_blas_matrix_t temp;
    temp.row = 1;
    temp.col = wtk_vector_size(diag->gconsts);
    temp.m = diag->gconsts + 1;
    qtk_matrix_copy_rows_fromvec(&sub_dst, &temp);
    qtk_blas_matrix_t *data_sq = qtk_blas_matrix_new(data->row, data->col);
    memcpy(data_sq->m, data->m, sizeof(float) * data->row * data->col);
    qtk_blas_matrix_apply_power(data_sq, 2.0);
    qtk_sub_matrix_t sub_data;
    qtk_sub_matrix_init2(&sub_data, data->m, data->row, data->col, data->col);
    qtk_sub_matrix_t sub_datasq;
    qtk_sub_matrix_init2(&sub_datasq, data_sq->m, data_sq->row, data_sq->col,
                         data_sq->col);
    qtk_sub_matrix_t means;
    qtk_sub_matrix_init2(&means, diag->means_invvars->m,
                         diag->means_invvars->row, diag->means_invvars->col,
                         diag->means_invvars->col);
    qtk_sub_matrix_t vars;
    qtk_sub_matrix_init2(&vars, diag->inv_vars->m, diag->inv_vars->row,
                         diag->inv_vars->col, diag->inv_vars->col);

    qtk_matrix_add_matmat(&sub_dst, &sub_data, &means, 1.0);
    qtk_matrix_add_matmat(&sub_dst, &sub_datasq, &vars, -0.5);
    // wtk_debug("loooooooog\n");
    // qtk_blas_matrix_print(dst);
    qtk_blas_matrix_delete(data_sq);
}

void qtk_ivector_pair_sort(qtk_array_t* array, int n)
{
	int idx,i;
	ivector_pair_t **pair1,**pair2;
	int tmp;
	float tmp_f;
	for(idx = 1; idx < n; idx++)
	{
		pair1 = qtk_array_get(array,idx);
		for(i = 0; i  < idx; i++)
		{
			pair2 = qtk_array_get(array,i);
			if((*pair1)->frame_weight > (*pair2)->frame_weight)
			{
				tmp = (*pair2)->stats;
				tmp_f = (*pair2)->frame_weight;
				(*pair2)->frame_weight = (*pair1)->frame_weight;
				(*pair2)->stats = (*pair1)->stats;
				(*pair1)->frame_weight = tmp_f;
				(*pair1)->stats = tmp;
			}
		}
	}
}

float qtk_ivector_get_total_posterior(qtk_array_t* posterior)
{
	float tot = 0.0;
	int idx;
	ivector_pair_t **pair;

    for(idx = 0; idx < posterior->dim; idx++)
    {
    	pair = qtk_array_get(posterior, idx);
    	tot += (*pair)->frame_weight;
    }
    return tot;
}

float qtk_ivector_vec_to_posterior_entry(qtk_ivector_t* i,float* log_likes,int num_gselect,float min_post,qtk_array_t* post_entry,int log_dim)
{
	//wtk_debug("entry:%d %f\n",num_gselect,min_post);
	int num_gauss = log_dim;
	if(num_gselect > num_gauss)
		num_gselect = num_gauss;
	qtk_array_t *temp_post = qtk_array_new(2,sizeof(ivector_pair_t*));
	float max_like = wtk_math_max(log_likes,log_dim);
	ivector_pair_t **pair;
	ivector_pair_t *pair2;
	if(min_post != 0.0)
	{
		float like_cutoff = max_like + logf(min_post);
		int g;
		for(g = 0; g < num_gauss; g++)
		{
			float like = *(log_likes+g);
			if(like >like_cutoff)
			{
				float post = exp(like - max_like);
				pair2 = (ivector_pair_t*)wtk_heap_malloc(i->heap,sizeof(ivector_pair_t));
				pair2->frame_weight = post;
				pair2->stats = g;
				//wtk_debug("entryx:%d %f\n",g,post);
				qtk_array_push(temp_post,&pair2);
			}
		}
	}
	if(temp_post->dim == 0)
	{
		//qtk_array_resize(temp_post,num_gauss);
		int g;
		for(g = 0; g < num_gauss; g++)
		{
			pair2 = (ivector_pair_t*)wtk_heap_malloc(i->heap,sizeof(ivector_pair_t));
			pair2->frame_weight = expf(*(log_likes+g) - max_like);
			pair2->stats = g;
			qtk_array_push(temp_post,&pair2);
		}
	}

	//if(temp_post->dim > num_gselect*2)
	//{
	//	qtk_ivector_pair_sort(temp_post,num_gselect);
	//}else
	//{
		qtk_ivector_pair_sort(temp_post,temp_post->dim);
	//}
	//wtk_debug("entryx:%d\n",temp_post->dim);
	int idx;
	int num_to_insert = min(temp_post->dim,num_gselect);
    for (idx = 0; idx < post_entry->dim; idx++) {
        pair = qtk_array_get(post_entry, idx);
        wtk_free(*pair);
    }
    qtk_array_clear(post_entry);

    for(idx = 0; idx < num_to_insert; idx++)
    {
    	pair = qtk_array_get(temp_post, idx);
		pair2 = (ivector_pair_t*)wtk_heap_malloc(i->heap,sizeof(ivector_pair_t));
		pair2->frame_weight = ((*pair)->frame_weight);
		pair2->stats = ((*pair)->stats);
        //wtk_debug("haha %d %f\n",(*pair)->stats,(*pair)->frame_weight);
    	qtk_array_push(post_entry,&pair2);
    }

    float tot_post = qtk_ivector_get_total_posterior(post_entry);
    //wtk_debug("%f\n",tot_post);
    float cutoff = min_post * tot_post;

    //TODO attention may cause error
    int dimx=0;
    for(idx = 0; idx < post_entry->dim; idx++)
    {
    	pair = qtk_array_get(post_entry, idx);
    	if((*pair)->frame_weight < cutoff)
    	{
    		tot_post -= (*pair)->frame_weight;
    		dimx++;
    	}
    }
    post_entry->dim -= dimx;
//    if(!post_entry->tag)
//        post_entry->tag = (float*)wtk_malloc(sizeof(float));
//    *(post_entry->tag) = 0.0;
    float inv_tot = 1.0/tot_post;
    for(idx = 0; idx < post_entry->dim; idx++)
    {
    	pair = qtk_array_get(post_entry, idx);
    	(*pair)->frame_weight *= inv_tot;
 //   	*(post_entry->tag) += (*pair)->frame_weight;
    }
    //wtk_debug("heiha %f\n",max_like + log(tot_post));
    qtk_ivector_pair_delete(temp_post);
    //qtk_array_delete(temp_post);
	return max_like + log(tot_post);
}

float qtk_ivector_get_minpost(qtk_ivector_t* i,float weight)
{
	float min_post = i->cfg->min_post;
	float abs_weight = fabs(weight);

	if(abs_weight == 0.0)
		return 0.99;
	min_post /= abs_weight;
	if(min_post > 0.99)
		min_post = 0.99;
	return min_post;
}

void qtk_ivector_posterior_delete(qtk_array_t** posteriors,int num)
{
	int i;
	qtk_array_t *a;
	for(i = 0; i < num; i++)
	{
		a = posteriors[i];
		qtk_ivector_pair_delete(a);
	}
	wtk_free(posteriors);
}

void qtk_ivector_update_stats_for_frames(qtk_ivector_t* i,qtk_array_t* frame_weights)
{
	int num_frames = frame_weights->dim;
	int feat_dim = i->lda_normalized.dim(i->lda_normalized.ud);
	qtk_blas_matrix_t *feats = qtk_blas_matrix_new(num_frames,feat_dim);
	qtk_blas_matrix_t *log_likes;
	qtk_array_t* frames = qtk_array_new(frame_weights->dim,sizeof(int));
	ivector_pair_t **pair,**pair2;
	int idx,idx2;
	float weight;

    for (idx = 0; idx < frame_weights->dim; idx++) {
        pair = qtk_array_get(frame_weights, idx);
        qtk_array_push(frames,&((*pair)->stats));
    }
    i->lda_normalized.get_frames(i->lda_normalized.ud,frames,feats);
    //wtk_debug("aaaaaa1\n");
    //qtk_blas_matrix_print(feats);

    log_likes = qtk_blas_matrix_new(feats->row,wtk_vector_size(i->diag_gmm->gconsts));
    qtk_ivector_diagubm_loglikehoods(i->diag_gmm,feats,log_likes);

    qtk_array_t** posteriors = (qtk_array_t**)wtk_calloc(num_frames,sizeof(qtk_array_t*));
    qtk_array_t* posterior;
    for(idx = 0; idx < num_frames; idx++)
    {
    	posterior = qtk_array_new(2,sizeof(ivector_pair_t*));
    	posteriors[idx] = posterior;
        pair = qtk_array_get(frame_weights, idx);
    	weight = (*pair)->frame_weight;
    	if(weight != 0.0)
    	{
    		float a1 = qtk_ivector_get_minpost(i,weight);
    		float a2 = qtk_ivector_vec_to_posterior_entry(i,log_likes->m+log_likes->col*idx,
    				i->cfg->num_gselect,a1,posterior,log_likes->col);
    		i->tot_ubm_loglike += weight * a2 ;
    		//wtk_debug("tot ubm:%f %d\n",i->tot_ubm_loglike,posterior->dim);
    		for(idx2 = 0; idx2 < posterior->dim; idx2++ )
    		{
    			pair2 = qtk_array_get(posterior, idx2);
    			(*pair2)->frame_weight *= weight * (i->cfg->posterior_scale);
    			//wtk_debug("%d %f\n",(*pair2)->stats,(*pair2)->frame_weight);
    		}
        	//*(posterior->tag) *= weight * (i->cfg->posterior_scale);
    	}
    }
    i->lda.get_frames(i->lda.ud,frames,feats);
    //wtk_debug("aaaaaa2\n");
    //qtk_blas_matrix_print(feats);
    qtk_ivector_acc_stats(i,feats,posteriors,num_frames);

    qtk_ivector_posterior_delete(posteriors,num_frames);
    qtk_array_delete(frames);
    qtk_blas_matrix_delete(log_likes);
    qtk_blas_matrix_delete(feats);
}

void qtk_ivector_update_stats_until_frame(qtk_ivector_t *i,int frame)
{
	i->updated_with_no_delta_weights=1;
	int ivector_period = i->cfg->ivector_period;
	int num_cg_iters = i->cfg->num_cg_iters;
	qtk_array_t *frame_weight = qtk_array_new(2,sizeof(ivector_pair_t*));
	ivector_pair_t *pairx;
	int num_frames_stats = i->num_frames_stats;
	int t,idx;

	for(; num_frames_stats <= frame; num_frames_stats++,i->num_frames_stats++)
	{
		t = num_frames_stats;
		pairx = (ivector_pair_t*)wtk_heap_malloc(i->heap,sizeof(ivector_pair_t));
		pairx->frame_weight = 1.0;
		pairx->stats = t;
		qtk_array_push(frame_weight,&pairx);
		if((!i->cfg->use_most_recent_ivector && t%ivector_period == 0) ||
				(i->cfg->use_most_recent_ivector && t == frame))
		{
			qtk_ivector_update_stats_for_frames(i,frame_weight);

//		    for (idx = 0; idx < frame_weight->dim; idx++) {
//		        pair = qtk_array_get(frame_weight, idx);
//		        wtk_free(*pair);
//		    }
		    qtk_array_clear(frame_weight);
		    qtk_ivector_stats_get_ivector(i,num_cg_iters,i->current_ivector);

		    if(!i->cfg->use_most_recent_ivector)
		    {
		    	float *tmp = (float*)wtk_malloc(sizeof(float)*(i->extractor->m[0]->col));
		    	for (idx = 0; idx < i->extractor->m[0]->col; idx++) {
		    		tmp[idx] = i->current_ivector[idx];
		    	}
		    	qtk_array_push(i->ivector_history,&tmp);
		    }
		}
	}
	if(frame_weight->dim > 0)
		qtk_ivector_update_stats_for_frames(i,frame_weight);
	qtk_ivector_pair_delete(frame_weight);
}

float mivec[]={-2.65033,4.42765,3.09324,1.51133,-3.47148,-3.87757,-0.877793,0.884614,0.319233,0.945464,2.23522,-1.87646,1.80171,-1.74278,0.324095,-1.18253,-0.158202,0.467155,-1.10919,1.12259,-0.368726,0.2483,0.294768,1.2972,1.45118,-1.4169,-0.655,-0.232625,0.111016,-1.08898,-0.513207,-0.768039,-0.240228,0.727158,0.625668,-0.976587,0.693227,-0.803318,-0.496249,-1.58142,0.732517,-0.40243,0.494049,0.0600505,-0.198136,1.53709,-0.312813,-0.350923,-0.750178,-0.559334,-0.0471629,-0.462422,-0.31826,-0.758343,0.0188819,0.0657779,0.787923,-0.123203,-0.645718,0.614604,0.374464,-0.223992,0.175248,0.418484,-0.987072,-0.0490506,0.149812,0.469804,-0.033544,-0.223671,-0.71844,-0.905475,-0.298607,0.737132,0.223828,0.277493,-0.402545,-0.561838,0.287715,0.444609,0.0401142,0.286834,-0.184417,0.332339,0.141898,-0.0894829,0.474447,0.182256,-0.0102325,-0.750015,-0.0826612,-0.695744,-0.302041,-0.0597047,0.07571,-0.15347,0.301309,0.0395566,0.450642,-0.164545};

void qtk_ivector_get_frame(qtk_ivector_t *i,int frame,float *feat,int len)
{
	//wtk_debug("ivector get frame:%d\n",frame);
	int frame_to_update_until = (i->cfg->greedy_ivector_extractor?
			(i->lda.frame_ready(i->lda.ud)-1) : frame);
	int idx;
	if(!i->delta_weights_provided)
		qtk_ivector_update_stats_until_frame(i,frame_to_update_until);
	if(i->cfg->use_most_recent_ivector)
	{
    	for (idx = 0; idx < len; idx++) {
    		feat[idx] = i->current_ivector[idx];
    	}
    	feat[0] -= i->extractor->prior_offset;
	}else
	{
		int x = frame/(i->cfg->ivector_period);
		float **val = qtk_array_get(i->ivector_history,x);
		memcpy(feat,(*val),sizeof(float)*len);
    	feat[0] -= i->extractor->prior_offset;
	}
	//memcpy(feat,mivec,sizeof(float)*len);
}
