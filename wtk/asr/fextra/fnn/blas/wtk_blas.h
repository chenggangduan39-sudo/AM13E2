#ifdef USE_BLAS
#ifndef WTK_VITE_PARM_POST_DNN_BLAS_WTK_BLAS_H_
#define WTK_VITE_PARM_POST_DNN_BLAS_WTK_BLAS_H_
#include "wtk/core/wtk_type.h"
#include "wtk/core/wtk_robin.h"
#include "wtk/asr/fextra/wtk_feat.h"
#include "wtk_blas_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_blas wtk_blas_t;

struct wtk_fnn;

struct wtk_blas
{
	wtk_blas_cfg_t *cfg;
	wtk_blas_vector_t *feat;
	wtk_blas_matrix_t *tmp_m;
	wtk_blas_matrix_t *tmp_m2;
	wtk_robin_t *input_feature_robin;	//cache robin for do work;
	wtk_feat_t *last_feature;
	struct wtk_fnn *dnn;
    unsigned int frame_index;
};

wtk_blas_t* wtk_blas_new(wtk_blas_cfg_t *cfg,struct wtk_fnn *dnn);
void wtk_blas_delete(wtk_blas_t *b);
void wtk_blas_reset(wtk_blas_t *b);
void wtk_blas_process_layer(wtk_blas_t *d,wtk_feat_t **pv,int npv,wtk_feat_t *f);
void wtk_blas_flush_layer(wtk_blas_t *d);
void wtk_blas_flush_end(wtk_blas_t *d);
#ifdef __cplusplus
};
#endif
#endif
#endif
