#ifndef WTK_ASR_FEXTRA_FNN_QLAS_WTK_QLAS_CFG
#define WTK_ASR_FEXTRA_FNN_QLAS_WTK_QLAS_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/math/wtk_mat.h"
#include "wtk/core/math/wtk_math.h"
#include "../wtk_fnn_type.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_qlas_cfg wtk_qlas_cfg_t;

/*
 * FBNK_D
 * NUM_CHANS=18
 * win=11
 *	input feature:  18*2*11=396 => |1*396|   |v[1][1] v[2][1] ...  v[n][1] v[1][2] v[2][2] ...  v[n][2]...|
 *	+ process transfs (|1*396|+|1*396|(bias))*(|1*396|(win))=|1*396|
 *	+ dnn layer:
 *	          * input*win   |1*396|*|396*256|=|1*256|
 *	          * add bias  |1*256|+|1*256|=|1*256|
 *	          * sigmoid|softmax
 */

typedef struct
{
	wtk_vecf_t *bias;
	wtk_vecf_t *win;
}wtk_qlas_trans_t;

typedef struct
{
	wtk_queue_node_t q_n;
	wtk_matf_t *win;
	wtk_vecf_t *bias;
    wtk_vecf_t *tmp;
	union
	{
		wtk_mats_t *swin;
		wtk_matb_t *bwin;
	}fixwin;
	wtk_fnn_post_type_t type;
	float max_bias;
}wtk_qlas_layer_t;

struct wtk_qlas_cfg
{
	char *net_fn;
	char *trans_fn;
	wtk_qlas_trans_t *transf;
	wtk_queue_t layer_q;
	float max_w;//127.0
	unsigned use_char:1;
	unsigned use_fix:1;
	unsigned use_bin:1;
	unsigned use_fix_res:1;
	int cache_size;
};

int wtk_qlas_cfg_init(wtk_qlas_cfg_t *cfg);
int wtk_qlas_cfg_clean(wtk_qlas_cfg_t *cfg);
int wtk_qlas_cfg_update_local(wtk_qlas_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_qlas_cfg_update(wtk_qlas_cfg_t *cfg);
int wtk_qlas_cfg_update2(wtk_qlas_cfg_t *cfg,wtk_source_loader_t *sl);
int wtk_qlas_cfg_out_cols(wtk_qlas_cfg_t *cfg);

void wtk_qlas_cfg_write_fix_bin(wtk_qlas_cfg_t *cfg,char *fn);
#ifdef __cplusplus
};
#endif
#endif
