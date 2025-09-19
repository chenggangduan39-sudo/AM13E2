#include "qtk_transform_feature.h"

qtk_transform_feature_t* qtk_transform_feature_new(qtk_blas_matrix_t *val)
{
	qtk_transform_feature_t* cache = (qtk_transform_feature_t*)wtk_malloc(sizeof(qtk_transform_feature_t));
//  linear linear linear linear offset
	cache->linear_term_ = qtk_blas_matrix_new(val->row,val->col-1);
	cache->offset_ = qtk_blas_matrix_new(1,val->row);
	int i;
	for(i = 0; i < val->row; i++)
	{
		memcpy(cache->linear_term_->m+cache->linear_term_->col*i,val->m+val->col*i,sizeof(float)*(val->col-1));
		*(cache->offset_->m+i) = *(val->m+val->col*i+(val->col-1));
	}

	return cache;
}

void qtk_transform_feature_delete(qtk_transform_feature_t* t)
{
	qtk_blas_matrix_delete(t->linear_term_);
	qtk_blas_matrix_delete(t->offset_);
	wtk_free(t);
}
void qtk_transform_feature_reset(qtk_transform_feature_t* t)
{
	//qtk_blas_matrix_zero(t->linear_term_);
	//qtk_blas_matrix_zero(t->offset_);
}

int qtk_transform_set_sourcer(
		qtk_transform_feature_t *pp, int (*frame_ready)(void *ud),
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

int qtk_transform_feature_dim(qtk_transform_feature_t *t)
{
	return t->offset_->col;
}

int qtk_transform_feature_is_last_frame(qtk_transform_feature_t *t,int frame)
{
	return t->sourcer.is_last_frame(t->sourcer.ud,frame);
}

int qtk_transform_feature_num_frames_ready(qtk_transform_feature_t *t)
{
	return t->sourcer.frame_ready(t->sourcer.ud);
}

void qtk_transform_feature_get_frames(qtk_transform_feature_t *t, qtk_array_t* frames, qtk_blas_matrix_t* feats)
{
	int num_frames = feats->row;
	int input_dim = t->linear_term_->col;
	qtk_blas_matrix_t *input_feats = qtk_blas_matrix_new(num_frames,input_dim);
	t->sourcer.get_frames(t->sourcer.ud,frames,input_feats);

        qtk_sub_matrix_t feats2;
        qtk_sub_matrix_init2(&feats2, feats->m, feats->row, feats->col,
                             feats->col);
        qtk_sub_matrix_t linear;
        qtk_sub_matrix_init2(&linear, t->linear_term_->m, t->linear_term_->row,
                             t->linear_term_->col, t->linear_term_->col);
        qtk_sub_matrix_t input_feats2;
        qtk_sub_matrix_init2(&input_feats2, input_feats->m, input_feats->row,
                             input_feats->col, input_feats->col);

        //	wtk_debug("ffffff3\n");
        // qtk_blas_matrix_print(input_feats);
        qtk_matrix_copy_rows_fromvec(&feats2, t->offset_);
        qtk_matrix_add_matmat(&feats2, &input_feats2, &linear, 1.0);
        //	qtk_blas_matrix_print(feats);
        qtk_blas_matrix_delete(input_feats);
}
