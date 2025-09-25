#ifndef QTK_CACHE_FEATURE_H_
#define QTK_CACHE_FEATURE_H_

#include "wtk/asr/fextra/pitch/core/qtk_array.h"
#include "wtk/asr/fextra/pitch/feat/qtk_pitch_post.h"
#include "qtk_ivector_cfg.h"
#include "wtk/core/math/wtk_matrix.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	qtk_array_t* cache_;//float **
	qtk_pitch_post_sourcer_t sourcer;
}qtk_cache_feature_t;

int qtk_cache_feature_dim(qtk_cache_feature_t *p);
int qtk_cache_feature_is_last_frame(qtk_cache_feature_t *p,int frame);
int qtk_cache_feature_num_frames_ready(qtk_cache_feature_t *p);
void qtk_cache_feature_get_frames(qtk_cache_feature_t *p, qtk_array_t* frames, qtk_blas_matrix_t* feats);

int qtk_cache_set_sourcer(
		qtk_cache_feature_t *pp, int (*frame_ready)(void *ud),
    void (*get_frame)(void *, int frame, float* feat,int dim),
	void (*get_frames)(void *, qtk_array_t *array, qtk_blas_matrix_t *mat),
    int (*is_last_frame)(void *ud, int frame),
	int (*dim)(void *),void *ud);
qtk_cache_feature_t* qtk_cache_feature_new(void);
void qtk_cache_feature_delete(qtk_cache_feature_t* cache);
void qtk_cache_feature_reset(qtk_cache_feature_t* cache);

#ifdef __cplusplus
};
#endif
#endif
