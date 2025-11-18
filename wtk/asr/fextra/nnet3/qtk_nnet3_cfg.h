#ifndef WTK_QTK_NNET3_CFG_H_
#define WTK_QTK_NNET3_CFG_H_
#include <math.h>
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/cfg/wtk_source.h"
#include "qtk_nnet3_component.h"
#include "wtk/asr/fextra/nnet3/qtk_nnet3_compution.h"
#include "wtk/asr/wfst/kaldifst/qtk_trans_model_cfg.h"
#include "wtk/asr/wfst/kwdec/qtk_wakeup_trans_model_cfg.h"
#include "qtk_nnet3_fix.h"
#include "wtk/core/math/wtk_mat.h"
#include "wtk/asr/fextra/kparm/knn/wtk_knn_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_nnet3_cfg qtk_nnet3_cfg_t;

enum qtk_custom_accplat{    //accelerate platform
	WTK_CUSTOM_COMM=1,
	WTK_CUSTOM_AVX,
	WTK_CUSTOM_AVX2,
	WTK_CUSTOM_NEON
};


struct qtk_nnet3_cfg
{
	nnet3_component_t* comp_q[120];
	qtk_trans_model_cfg_t *t_mdl;
	qtk_wakeup_trans_model_cfg_t *wt_mdl;
	qtk_nnet3_compution_t *compution;
	qtk_nnet3_compution_t *compution2;
	wtk_mats_t* ms[1000];
	qtk_blas_matrix_t* prior;
	wtk_vecf_t *xvector;
	char *tdnn_fn;
	char *feat_fn;
	char *compution_fn;
	char *compution_fn2;
	char *xvector_fn;
	int left_context;
	int right_context;
	int frame_plus;
	int frame_per_chunk;
	int frame_subsample_factor;
	int fixed_nbytes;
	int extra_left;
	unsigned int use_custom_acc;    //here not use bit control, because value maybe auto adjust with environment for more fitting, it had affected when use_op=1;
	unsigned int use_kwdec:1;
	unsigned int use_xvector:1;
	unsigned int is_fixed :1;
	unsigned int use_fix_res :1;
	unsigned int normal :1;
        unsigned int use_fix_wake : 1;
        int porder;      //must >=2, valid when op_level=1
	char* bin_fn;
	int comp_cnt;
	float max_w;

};

int qtk_nnet3_cfg_init(qtk_nnet3_cfg_t *cfg);
int qtk_nnet3_cfg_clean(qtk_nnet3_cfg_t *cfg);
int qtk_nnet3_cfg_update_local(qtk_nnet3_cfg_t *cfg, wtk_local_cfg_t *lc);
int qtk_nnet3_cfg_update(qtk_nnet3_cfg_t *cfg);
int qtk_nnet3_cfg_update2(qtk_nnet3_cfg_t *cfg, wtk_source_loader_t *sl);
int qtk_nnet3_cfg_bytes(qtk_nnet3_cfg_t *cfg);
void qtk_nnet3_cfg_write_fix_bin(qtk_nnet3_cfg_t *cfg, char *fn);
#ifdef __cplusplus
};
#endif
#endif
