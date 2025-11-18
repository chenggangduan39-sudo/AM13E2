#ifndef QTK_SPLICE_FEATURE_H_
#define QTK_SPLICE_FEATURE_H_

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
	int left_context;
	int right_context;
	qtk_pitch_post_sourcer_t sourcer;
}qtk_splice_feature_t;

int qtk_splice_feature_dim(qtk_splice_feature_t *s);
int qtk_splice_feature_is_last_frame(qtk_splice_feature_t *s,int frame);
int qtk_splice_feature_num_frames_ready(qtk_splice_feature_t *s);
void qtk_splice_feature_get_frame(qtk_splice_feature_t *s, int frame, float* feats, int len);
void qtk_splice_feature_get_frames(qtk_splice_feature_t *s, qtk_array_t* frames, qtk_blas_matrix_t* feats);

int qtk_splice_set_sourcer(
		qtk_splice_feature_t *pp, int (*frame_ready)(void *ud),
    void (*get_frame)(void *, int frame, float* feat,int dim),
	void (*get_frames)(void *, qtk_array_t *array, qtk_blas_matrix_t *mat),
    int (*is_last_frame)(void *ud, int frame),
	int (*dim)(void *),void *ud);

qtk_splice_feature_t* qtk_splice_feature_new(int left, int right);
void qtk_splice_feature_delete(qtk_splice_feature_t* s);
#ifdef __cplusplus
};
#endif
#endif
