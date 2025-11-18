#ifndef QTK_IVECTOR_CFG_H_
#define QTK_IVECTOR_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/math/wtk_vector.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/math/wtk_matrix.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_ivector_cfg qtk_ivector_cfg_t;
typedef struct qtk_ivector_extractor qtk_ivector_extractor_t;
typedef struct qtk_diaggmm qtk_diaggmm_t;

struct qtk_ivector_extractor
{
	qtk_blas_double_matrix_t *w;
	wtk_double_vector_t *w_vec;
	qtk_blas_double_matrix_t **m;
	qtk_sp_matrix_t **sigma_inv;
	double prior_offset;
	double* gconst;
	qtk_blas_double_matrix_t *u;
	qtk_blas_double_matrix_t **sigma_inv_m;
	int msize;
	int ssize;
};

struct qtk_diaggmm
{
	int valid_gconst;
	wtk_vector_t *gconsts;
	wtk_vector_t *weights;
	qtk_blas_matrix_t *inv_vars;
	qtk_blas_matrix_t *means_invvars;
};

struct qtk_ivector_cfg
{
	char *lda_fn;			//lda-matirx file
	char *ubm_fn;			//diag-ubm file
	char *extractor_fn; //ivector-extractor file
	qtk_ivector_extractor_t* extractor;
	qtk_diaggmm_t* diag_gmm;
	qtk_blas_matrix_t *lda_m;
	int max_count;			//!< min frame to update mean vector.
	int max_remembered_frames;
	int num_gselect;
	int num_cg_iters;
	int ivector_period;
	int left_context;//splice.conf
	int right_context;//splice.conf
	int window;
	int order;
	float min_post;
	float posterior_scale;
	unsigned use_whole:1;
	unsigned smooth:1;
	unsigned save_cmn:1;
	unsigned use_hist:1;
	unsigned greedy_ivector_extractor:1;
	unsigned use_most_recent_ivector:1;
};

int qtk_ivector_cfg_init(qtk_ivector_cfg_t *cfg);
int qtk_ivector_cfg_clean(qtk_ivector_cfg_t *cfg);
int qtk_ivector_cfg_update_local(qtk_ivector_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_ivector_cfg_update(qtk_ivector_cfg_t *cfg,wtk_source_loader_t *sl);
#ifdef __cplusplus
};
#endif
#endif
