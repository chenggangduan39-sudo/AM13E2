#ifndef QTK_TRANSFORM_FEATURE_H_
#define QTK_TRANSFORM_FEATURE_H_

#include "wtk/asr/fextra/pitch/core/qtk_array.h"
#include "wtk/asr/fextra/pitch/feat/qtk_pitch_post.h"
#include "qtk_ivector_cfg.h"
#include "wtk/core/math/wtk_matrix.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	qtk_ivector_cfg_t* cfg;
	qtk_blas_matrix_t* linear_term_;
	qtk_blas_matrix_t* offset_;//float vector
	qtk_pitch_post_sourcer_t sourcer;
}qtk_transform_feature_t;

int qtk_transform_feature_dim(qtk_transform_feature_t *t);
int qtk_transform_feature_is_last_frame(qtk_transform_feature_t *t,int frame);
int qtk_transform_feature_num_frames_ready(qtk_transform_feature_t *t);
void qtk_transform_feature_get_frames(qtk_transform_feature_t *t, qtk_array_t* frames, qtk_blas_matrix_t* feats);

int qtk_transform_set_sourcer(
		qtk_transform_feature_t *pp, int (*frame_ready)(void *ud),
    void (*get_frame)(void *, int frame, float* feat,int dim),
	void (*get_frames)(void *, qtk_array_t *array, qtk_blas_matrix_t *mat),
    int (*is_last_frame)(void *ud, int frame),
	int (*dim)(void *),void *ud);

qtk_transform_feature_t* qtk_transform_feature_new(qtk_blas_matrix_t *val);
void qtk_transform_feature_delete(qtk_transform_feature_t* t);
void qtk_transform_feature_reset(qtk_transform_feature_t* t);
#ifdef __cplusplus
};
#endif
#endif
