#include "qtk_splice_feature.h"

qtk_splice_feature_t* qtk_splice_feature_new(int left, int right)
{
	qtk_splice_feature_t* cache = (qtk_splice_feature_t*)wtk_malloc(sizeof(qtk_splice_feature_t));

	cache->left_context = left;
	cache->right_context = right;
	return cache;
}

void qtk_splice_feature_delete(qtk_splice_feature_t* s)
{
	wtk_free(s);
}

int qtk_splice_set_sourcer(
		qtk_splice_feature_t *pp, int (*frame_ready)(void *ud),
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

int qtk_splice_feature_dim(qtk_splice_feature_t *s)
{
	return (s->sourcer.dim(s->sourcer.ud))*(1 + s->left_context + s->right_context);
}

int qtk_splice_feature_is_last_frame(qtk_splice_feature_t *s,int frame)
{
	return s->sourcer.is_last_frame(s->sourcer.ud,frame);
}

int qtk_splice_feature_num_frames_ready(qtk_splice_feature_t *s)
{
		int num_frames = s->sourcer.frame_ready(s->sourcer.ud);
		int context = s->cfg->order * s->cfg->window;

		if(num_frames > 0 && s->sourcer.is_last_frame(s->sourcer.ud,num_frames - 1))
			return num_frames;
		else
			return max(0,num_frames - context);
}

void qtk_splice_feature_get_frame(qtk_splice_feature_t *s, int frame, float* feats, int len)
{
	//wtk_debug("splice get frame:%d\n",frame);
	int dim_in = s->sourcer.dim(s->sourcer.ud);
	int T = s->sourcer.frame_ready(s->sourcer.ud);
	int t2,t2_limited,n;
	float* part;

	for(t2 = frame - s->left_context; t2<= frame + s->right_context; t2++)
	{
		t2_limited = t2;
		if(t2_limited < 0)
			t2_limited = 0;
		if(t2_limited >= T)
			t2_limited = T; //- 1;
		n = t2 - (frame - s->left_context);
		part = feats + n*dim_in;
		//wtk_debug("%d %d %d\n",T,t2_limited,n);
		s->sourcer.get_frame(s->sourcer.ud,t2_limited,part,dim_in);
	}
}

void qtk_splice_feature_get_frames(qtk_splice_feature_t *s, qtk_array_t* frames, qtk_blas_matrix_t* feats)
{
	int i;
	int *frame;
	for(i = 0; i < frames->dim; i++)
	{
		frame = qtk_array_get(frames,i);
		qtk_splice_feature_get_frame(s,*frame,feats->m+feats->col*i,feats->col);
	}
}
