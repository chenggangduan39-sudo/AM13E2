/*
 * qtk_vits_onnx_cfg.h
 *
 *  Created on: Sep 7, 2022
 *      Author: dm
 */

#ifndef QTK_VITS_QTK_VITS_ONNX_CFG_H_
#define QTK_VITS_QTK_VITS_ONNX_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/wtk_heap.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_vits_onnx_cfg qtk_vits_onnx_cfg_t;
struct qtk_vits_onnx_cfg{
	wtk_heap_t* heap;
	wtk_array_t *embedding_dim_num;
	char *onnx_fn;
	char *onnx_data;
	int   onnx_data_len;
	unsigned int nthread;
	unsigned int use_bin:1;
};
int qtk_vits_onnx_cfg_init(qtk_vits_onnx_cfg_t *cfg);
int qtk_vits_onnx_cfg_clean(qtk_vits_onnx_cfg_t *cfg);
int qtk_vits_onnx_cfg_update_local(qtk_vits_onnx_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_vits_onnx_cfg_update(qtk_vits_onnx_cfg_t *cfg);
int qtk_vits_onnx_cfg_update2(qtk_vits_onnx_cfg_t *cfg,wtk_source_loader_t *sl);
#ifdef __cplusplus
};
#endif
#endif /* QTK_VITS_QTK_VITS_ONNX_CFG_H_ */
