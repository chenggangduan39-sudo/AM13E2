/*
 * qtk_vits_onnx.h
 *
 *  Created on: Sep 7, 2022
 *      Author: dm
 */

#ifndef QTK_VITS_QTK_VITS_ONNX_H_
#define QTK_VITS_QTK_VITS_ONNX_H_
#include "onnxruntime_c_api.h"
#include "qtk_vits_onnx_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_vits_onnx qtk_vits_onnx_t;
struct qtk_vits_onnx
{
	const OrtApiBase *base;
	const OrtApi *api;
	OrtEnv *env;
	OrtSessionOptions *sop;
	OrtSession *session;
	OrtAllocator *allocator;
	OrtMemoryInfo *meminfo;
	OrtValue **in;
	OrtValue **out;
//	long unsigned int* dim_in;
//	long unsigned int* dim_out;
	long unsigned int num_in;
	long unsigned int num_out;
	char **name_in;
	char **name_out;
};

qtk_vits_onnx_t* qtk_vits_onnx_new(qtk_vits_onnx_cfg_t *cfg);
void qtk_vits_onnx_delete(qtk_vits_onnx_t *onnx);
int qtk_vits_onnx_reset(qtk_vits_onnx_t *onnx);
int qtk_vits_onnx_feed_inparam(qtk_vits_onnx_t *onnx, void *input, int in_dim, int64_t *in_shape, int shape_len,int type,int index);
//int qtk_vits_onnx_feed(qtk_vits_t* vits, char* data, int len);
int qtk_vits_onnx_run(qtk_vits_onnx_t *onnx);
int64_t* qtk_vits_onnx_get_outshape(qtk_vits_onnx_t *onnx, int index, int64_t *size_len);
void* qtk_vits_onnx_getout(qtk_vits_onnx_t *onnx, int index);
#ifdef __cplusplus
};
#endif
#endif /* QTK_VITS_QTK_VITS_ONNX_H_ */
