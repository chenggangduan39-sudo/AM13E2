#include "qtk_cache_feature.h"

qtk_cache_feature_t* qtk_cache_feature_new(void)
{
	qtk_cache_feature_t* cache = (qtk_cache_feature_t*)wtk_malloc(sizeof(qtk_cache_feature_t));

	cache->cache_ = qtk_array_new(2,sizeof(wtk_vector_t*));
	return cache;
}

void qtk_cache_feature_delete(qtk_cache_feature_t* cache)
{
	int i;
	float **val;
	for(i = 0; i < cache->cache_->dim; i++)
	{
		val = qtk_array_get(cache->cache_,i);
		wtk_free(*val);
	}
	qtk_array_delete(cache->cache_);
	wtk_free(cache);
}

void qtk_cache_feature_reset(qtk_cache_feature_t* cache)
{
	int i;
	float **val;
	for(i = 0; i < cache->cache_->dim; i++)
	{
		val = qtk_array_get(cache->cache_,i);
		wtk_free(*val);
	}
	qtk_array_clear(cache->cache_);
}

int qtk_cache_set_sourcer(
		qtk_cache_feature_t *pp, int (*frame_ready)(void *ud),
    void (*get_frame)(void *, int frame, float* feat,int dim),
	void (*get_frames)(void *, qtk_array_t *array, qtk_blas_matrix_t *mat),
    int (*is_last_frame)(void *ud, int frame),
	int (*dim)(void *),void *ud)
{
    pp->sourcer.ud = ud;
    pp->sourcer.frame_ready = frame_ready;
    pp->sourcer.get_frame = get_frame;
    pp->sourcer.get_frames = get_frames;
    pp->sourcer.is_last_frame = is_last_frame;
    pp->sourcer.dim = dim;

    return 0;
}

int qtk_cache_feature_dim(qtk_cache_feature_t *p)
{
//	int src_dim = p->sourcer.dim(p->sourcer.ud);
//	return src_dim * (1 + p->cfg->window);
	return p->sourcer.dim(p->sourcer.ud);
}

int qtk_cache_feature_is_last_frame(qtk_cache_feature_t *p,int frame)
{
	return p->sourcer.is_last_frame(p->sourcer.ud,frame);
}

int qtk_cache_feature_num_frames_ready(qtk_cache_feature_t *p)
{
//	int num_frames = p->sourcer.frame_ready(p->sourcer.ud);
//	int context = p->cfg->order * p->cfg->window;
//
//	if(num_frames > 0 && p->sourcer.is_last_frame(p->sourcer.ud,num_frames - 1))
//		return num_frames;
//	else
//		return max(0,num_frames - context);
	return p->sourcer.frame_ready(p->sourcer.ud);
}

void qtk_cache_feature_get_frames(qtk_cache_feature_t *p, qtk_array_t* frames, qtk_blas_matrix_t* feats)
{
	int num_frames = frames->dim;
	qtk_array_t* non_cached_frames = qtk_array_new(num_frames, sizeof(int));
	qtk_array_t* non_cached_indexes = qtk_array_new(num_frames, sizeof(int));
	int i,t;
	int *t1;
	wtk_vector_t **feat;
	wtk_vector_t *featx;
//	wtk_debug("hhhhhhhhhhh1\n");
	for(i = 0; i < num_frames; i++)
	{
		t1 = qtk_array_get(frames,i);
		t = *t1;
		if(t < p->cache_->dim && qtk_array_get(p->cache_,t) != NULL)
		{
			feat = qtk_array_get(p->cache_,t);
			memcpy(feats->m+feats->col*i,(*feat)+1,sizeof(float)*wtk_vector_size(*feat));
		}else
		{
			qtk_array_push(non_cached_frames,&t);
			qtk_array_push(non_cached_indexes,&t);
		}
	}

	if(non_cached_frames->dim <= 0)
	{
		return;
	}

	int num_non_cached_frames = non_cached_frames->dim;
	int dim = qtk_cache_feature_dim(p);
	qtk_blas_matrix_t *non_cached_feats = qtk_blas_matrix_new(num_non_cached_frames,dim);
	p->sourcer.get_frames(p->sourcer.ud,non_cached_frames,non_cached_feats);
	for(i = 0; i < num_non_cached_frames; i++)
	{
		t1 = qtk_array_get(non_cached_frames,i);
		t = *t1;
//		wtk_debug("hhhhhhhhhhhx %p %d %d\n",p->cache_,t,p->cache_->dim);
		if(t < p->cache_->dim && qtk_array_get(p->cache_,t) != NULL)
		{
			feat = qtk_array_get(non_cached_indexes,t);
			//wtk_debug("1111111\n");
			memcpy(feats->m+feats->col*i,*feat+1,sizeof(float)*wtk_vector_size(*feat));
		}else
		{
			memcpy(feats->m+feats->col*i,non_cached_feats->m+feats->col*i,sizeof(float)*feats->col);
//			if(t >= p->cache_->dim)
//			{
//				qtk_array_resize(p->cache_,t+1);
//			}
			featx = wtk_vector_new(feats->col);
			//wtk_debug("2222222\n");
			memcpy(featx+1,non_cached_feats->m+feats->col*i,sizeof(float)*feats->col);
			qtk_array_push(p->cache_,&featx);
		}
	}
//	wtk_debug("hhhhhh2\n");
//	qtk_blas_matrix_print(feats);
	qtk_blas_matrix_delete(non_cached_feats);
	qtk_array_delete(non_cached_frames);
	qtk_array_delete(non_cached_indexes);
}
