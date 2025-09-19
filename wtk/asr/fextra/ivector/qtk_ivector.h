#ifndef QTK_IVECTOR_H_
#define QTK_IVECTOR_H_
#include "wtk/core/math/wtk_matrix.h"
#include "wtk/core/math/wtk_vector.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_robin.h"
#include "../wtk_feat.h"
#include "qtk_ivector_cfg.h"
#include "wtk/asr/fextra/pitch/core/qtk_array.h"
#include "wtk/asr/fextra/pitch/feat/qtk_pitch_post.h"
#include "qtk_cache_feature.h"
#include "qtk_transform_feature.h"
#include "qtk_splice_feature.h"


#ifdef __cplusplus
extern "C" {
#endif
/**
 * cepstral mean normalization
 */

typedef struct qtk_ivector qtk_ivector_t;
typedef struct qtk_ivector_estimation_stats qtk_ivector_estimation_stats_t;

struct wtk_fextra;

typedef struct
{
	qtk_array_t* pairs;
	float tot_weight;
	int gauss_idx;
}ivector_gauss_t;

typedef struct
{
	int stats;
	float frame_weight;
}ivector_pair_t;

struct qtk_ivector_estimation_stats
{
	double prior_offset;
	double max_count;
	double num_frames;
	qtk_sp_matrix_t *quadratic_term;
	qtk_blas_double_vector_t *linear_term;
};

typedef struct
{
	qtk_cache_feature_t *cache1;
	qtk_cache_feature_t *cache2;
	qtk_splice_feature_t *splice1;
	qtk_splice_feature_t *splice2;
	qtk_transform_feature_t *trans1;
	qtk_transform_feature_t *trans2;
}qtk_ivector_sourcers_t;

struct qtk_ivector
{
	qtk_ivector_cfg_t *cfg;
	struct wtk_fextra *parm;
	qtk_ivector_extractor_t *extractor;
	qtk_diaggmm_t *diag_gmm;
	qtk_ivector_estimation_stats_t *estimationstats;
	qtk_pitch_post_sourcer_t lda_normalized;
	qtk_pitch_post_sourcer_t lda;//cache frame
	qtk_ivector_sourcers_t source;
	qtk_blas_matrix_t *lda_m;
	void* cmn;
	wtk_heap_t *heap;
	double *current_ivector;
	qtk_array_t *ivector_history;//float
	int num_frames_stats;
	int most_recent_frame_with_weight;
	double tot_ubm_loglike;
	unsigned delta_weights_provided:1;
	unsigned updated_with_no_delta_weights:1;
};

qtk_ivector_t* qtk_ivector_new(qtk_ivector_cfg_t *cfg,void* cmn);
int qtk_ivector_delete(qtk_ivector_t *i);
int qtk_ivector_reset(qtk_ivector_t *i);

int qtk_ivector_post_set_lda_norm(
		qtk_ivector_t *pp, int (*frame_ready)(void *ud),
    void (*get_frame)(void *, int frame, float* feat,int dim),
	void (*get_frames)(void *, qtk_array_t *array, qtk_blas_matrix_t *mat),
    int (*is_last_frame)(void *ud, int frame),
	int (*dim)(void *),void *ud);

int qtk_ivector_post_set_lda(
		qtk_ivector_t *pp, int (*frame_ready)(void *ud),
    void (*get_frame)(void *, int frame, float* feat,int dim),
	void (*get_frames)(void *, qtk_array_t *array, qtk_blas_matrix_t *mat),
    int (*is_last_frame)(void *ud, int frame),
	int (*dim)(void *),void *ud);
void qtk_ivector_get_frame(qtk_ivector_t *i,int frame,float *feat,int len);
//void qtk_ivector_set_notify(qtk_ivector_t *i,void *ths,wtk_ivector_notify_f notify);
#ifdef __cplusplus
};
#endif
#endif
