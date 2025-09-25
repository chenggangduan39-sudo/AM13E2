#ifndef WTK_VITE_PARM_POST_DNN_MLAT_WTK_MLAT_CFG
#define WTK_VITE_PARM_POST_DNN_MLAT_WTK_MLAT_CFG
#include "wtk/core/cfg/wtk_local_cfg.h" 
#include "../wtk_fnn_type.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/math/wtk_mat.h"
#include "../flat/wtk_flat_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_mlat_cfg wtk_mlat_cfg_t;

/**
 *	mlat 定点化不转置, python py/wtk/dnn_netfix.py -t 0
 *	flat 定点化转置;
 */

typedef struct
{
	wtk_vecf_t *b;
	wtk_vecf_t *w;//(intput+b)*m;
}wtk_mlat_trans_t;

typedef struct
{
	union{
		wtk_matc_t *c;
		wtk_mati_t *i;
	}w;
	wtk_mati_t *b;
	float scale;
}wtk_mlat_fix_layer_t;

typedef struct
{
	wtk_queue_node_t q_n;
	wtk_dnn_float_type_t float_type;
	wtk_fnn_post_type_t type;
	int index;
	wtk_matf_t *w;
	wtk_vecf_t *b;
	wtk_mlat_fix_layer_t *fix_wb;
}wtk_mlat_layer_t;

struct wtk_mlat_cfg
{
	wtk_mlat_trans_t *trans;
	wtk_queue_t layer_q;	//wtk_mlat_layer_t or wtk_dnn_layer_t
	char *net_fn;
	char *trans_fn;
	int skip_frame;
	int nthread;
	int input_size;
	int output_size;

	float max_0_w;
	float max_w;	//127.0
	float max_b;	//255.0
	unsigned use_int:1;
	unsigned is_bin:1;
	unsigned use_c:1;
	unsigned use_fix_res:1;
	unsigned use_transpose:1;
};

int wtk_mlat_cfg_init(wtk_mlat_cfg_t *cfg);
int wtk_mlat_cfg_clean(wtk_mlat_cfg_t *cfg);
int wtk_mlat_cfg_update_local(wtk_mlat_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_mlat_cfg_update(wtk_mlat_cfg_t *cfg);
int wtk_mlat_cfg_update2(wtk_mlat_cfg_t *cfg,wtk_source_loader_t *sl);

int wtk_mlat_cfg_out_cols(wtk_mlat_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
