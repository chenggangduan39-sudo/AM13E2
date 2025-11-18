#ifndef __QTK_FEAT_QTK_PITCH_POST_H__
#define __QTK_FEAT_QTK_PITCH_POST_H__
#include "wtk/asr/fextra/pitch/core/qtk_array.h"
#include "wtk/asr/fextra/pitch/feat/qtk_feature.h"
#include "wtk/asr/fextra/pitch/feat/qtk_pitch_post_cfg.h"
#include "wtk/core/math/wtk_matrix.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_pitch_post qtk_pitch_post_t;
typedef struct qtk_pitch_post_sourcer qtk_pitch_post_sourcer_t;

struct qtk_pitch_post_sourcer {
    void *ud;
    int (*frame_ready)(void *);
    void (*get_frame)(void *, int frame, float* feat,int dim);
    void (*get_frames)(void *, qtk_array_t *array, qtk_blas_matrix_t *mat);
    int (*is_last_frame)(void *, int frame);
    int (*dim)(void *);
};

struct qtk_pitch_post {
    qtk_pitch_post_cfg_t *cfg;
    qtk_pitch_post_sourcer_t sourcer;
    qtk_feature_delta_t *delta;
    qtk_array_t *delta_feature_noise;
    qtk_array_t *normalization_stats;
    int feat_dim;
};

int qtk_pitch_post_init(qtk_pitch_post_t *pp, qtk_pitch_post_cfg_t *cfg);
int qtk_pitch_post_clean(qtk_pitch_post_t *pp);
int qtk_pitch_post_reset(qtk_pitch_post_t *pp);
int qtk_pitch_post_get_frame(qtk_pitch_post_t *pp, int frame, float *res,int dim);
int qtk_pitch_post_set_sourcer(
    qtk_pitch_post_t *pp, int (*frame_ready)(void *ud),
    void (*get_frame)(void *, int frame, float* feat,int dim),
    int (*is_last_frame)(void *ud, int frame), void *ud);
int qtk_pitch_post_nums_ready(qtk_pitch_post_t *pp);
int qtk_pitch_post_get_feat_dim(qtk_pitch_post_t *pp);

#ifdef __cplusplus
};
#endif
#endif
