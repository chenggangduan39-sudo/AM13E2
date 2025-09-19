#ifndef WTK_VITE_PARM_POST_DNN_FLAT_WTK_FLAT_CFG_H_
#define WTK_VITE_PARM_POST_DNN_FLAT_WTK_FLAT_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "../wtk_fnn_type.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/math/wtk_mat.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_flat_cfg wtk_flat_cfg_t;


typedef enum
{
	WTK_DNN_FLOAT,
	WTK_DNN_FIX_FLOAT,
	WTK_DNN_FIX0,
}wtk_dnn_float_type_t;

typedef struct
{
	wtk_matuc_t *uc;
	int min;
}wtk_matxuc_t;

typedef struct
{
	union{
		wtk_matc_t *c;
		wtk_mati_t *i;
	}w;
	wtk_matxuc_t *uc;
	wtk_mati_t *b;
	float scale;
//	int min_0;
//	int max_0;
}wtk_dnn_fix_layer_t;

typedef struct
{
	wtk_queue_node_t q_n;
	wtk_matrix_t *w;   //wx+b
	wtk_matrix_t *b;
	wtk_matrix_t *r_alpha;
	wtk_matrix_t *r_beta;
	wtk_dnn_fix_layer_t *fix_wb;
	wtk_fnn_post_type_t type;
	wtk_dnn_float_type_t float_type;
	int index;
}wtk_dnn_layer_t;


typedef struct
{
	wtk_vector_t *b;
	wtk_vector_t *m;
}wtk_dnn_trans_t;

struct wtk_flat_cfg
{
	char *net_fn;
	char *trans_fn;
	wtk_dnn_trans_t *trans;
	wtk_queue_t layer_q;		//wtk_dnn_layer_t
	int cache_size;
	int nx;
	float max_0_w;
	float max_w;	//127.0
	float max_b;	//255.0
	float min_avg_scale;
	int min_avg_v;
	float min_trans_avg_scale;
	int min_trans_avg_v;
	unsigned char use_c:1;
	unsigned char is_bin:1;
	unsigned char use_fix_float:1;
	unsigned char use_fix_res:1;
	unsigned char use_lazy_out:1;
	unsigned char use_transpose:1;
	unsigned char use_fix_trans_matrix:1;
	unsigned char use_fix_0_layer:1;
	unsigned char use_fix_0_c:1;
	unsigned char use_mt:1;
	unsigned char use_fix_0_cx:1;
};
void wtk_dnn_layer_process(wtk_dnn_layer_t *l,wtk_matrix_t *input,wtk_matrix_t *output,int reverse);
void wtk_dnn_trans_process(wtk_dnn_trans_t *l,wtk_vector_t *v);

int wtk_flat_cfg_init(wtk_flat_cfg_t *cfg);
int wtk_flat_cfg_clean(wtk_flat_cfg_t *cfg);
int wtk_flat_cfg_update_local(wtk_flat_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_flat_cfg_update2(wtk_flat_cfg_t *cfg,wtk_source_loader_t *sl);
int wtk_flat_cfg_update(wtk_flat_cfg_t *cfg);

//=================== value section =================
int wtk_flat_cfg_in_cols(wtk_flat_cfg_t *cfg);
int wtk_flat_cfg_out_cols(wtk_flat_cfg_t *cfg);
void  wtk_dnn_fix_layer_delete(wtk_dnn_fix_layer_t *l,int use_c);
wtk_dnn_fix_layer_t* wtk_dnn_fix_layer_new2();
#ifdef __cplusplus
};
#endif
#endif
