#ifndef WTK_VITE_PARM_POST_DNN_FLAT_WTK_FLAT_H_
#define WTK_VITE_PARM_POST_DNN_FLAT_WTK_FLAT_H_
#include "wtk/core/wtk_type.h"
#include "wtk_flat_cfg.h"
#include "../../wtk_feat.h"
#include "wtk/core/wtk_robin.h"
#include "wtk_mat_heap.h"
#ifdef __mips_x1000
#include "wtk_mat_mips.h"
#endif
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_flat wtk_flat_t;
struct wtk_fnn;

typedef struct
{
	wtk_matuc_t *fix_0_output2;
	wtk_mati_t *fix_0_output2_i;
	wtk_matuc_t **fix_output;
}wtk_flat_mc_t;

typedef struct
{
	wtk_mati_t *fix_0_output2;
	wtk_mati_t **fix_output;
}wtk_flat_mi_t;

struct wtk_flat
{
	wtk_flat_cfg_t *cfg;
	struct wtk_fnn *dnn;
	wtk_matrix_t *feat_matrix;

	wtk_matrix_t **matrixs;

	wtk_matrix_t *fix_0_output1;

	wtk_matc_t *fix_0_char_input;
	wtk_mati_t *fix_0_int_input;
	wtk_mati_t *fix_0_mul;
	wtk_matuc_t *fix_0_char_output;
	wtk_mati_t *fix_0_int_output;

	union{
		wtk_flat_mc_t c;
		wtk_flat_mi_t i;
	}fix_output;

	wtk_mati_t **fix_tmp;

	wtk_feat_t *last_feature;
	wtk_robin_t *robin;
	int index;
	wtk_dnn_layer_t *last_l;
	float last_scale;

	wtk_matc_t *output_c;
};

wtk_flat_t* wtk_flat_new(wtk_flat_cfg_t *cfg,struct wtk_fnn *dnn);
void wtk_flat_delete(wtk_flat_t *f);
void wtk_flat_process_layer(wtk_flat_t *d,wtk_feat_t **pv,int npv,wtk_feat_t *f);
void wtk_flat_flush(wtk_flat_t *f);
void wtk_flat_reset(wtk_flat_t *f);
void wtk_flat_flush_end(wtk_flat_t *f);
float wtk_flat_get_dnn_value(wtk_flat_t *d,wtk_feat_t *f,int index);
#ifdef __cplusplus
};
#endif
#endif
