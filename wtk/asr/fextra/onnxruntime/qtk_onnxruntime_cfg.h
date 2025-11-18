#ifndef QTK_ONNXRUNTIME_CFG_H_
#define QTK_ONNXRUNTIME_CFG_H_
#include "wtk/core/cfg/wtk_cfg_file.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_onnxruntime_cfg qtk_onnxruntime_cfg_t;
struct qtk_onnxruntime_cfg
{
	char *onnx_fn;
	wtk_rbin2_t *rb;
	int outer_in_num;
	int outer_out_num;
	int num_inter_threads;
	int num_intra_threads;
	unsigned int use_mem:1;
	unsigned int use_reduce_mem : 1;
	unsigned int use_inner_item:1;
	unsigned int pre_malloc:1;
};

int qtk_onnxruntime_cfg_init(qtk_onnxruntime_cfg_t *cfg);
int qtk_onnxruntime_cfg_clean(qtk_onnxruntime_cfg_t *cfg);
int qtk_onnxruntime_cfg_update(qtk_onnxruntime_cfg_t *cfg);
int qtk_onnxruntime_cfg_update2(qtk_onnxruntime_cfg_t *cfg,wtk_rbin2_t *rb);
int qtk_onnxruntime_cfg_update_local(qtk_onnxruntime_cfg_t *cfg,wtk_local_cfg_t *lc);

#ifdef __cplusplus
};
#endif
#endif
