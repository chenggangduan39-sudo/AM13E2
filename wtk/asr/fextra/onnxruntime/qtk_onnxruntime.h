#ifdef ONNX_DEC
#ifndef QTK_ONNXRUNTIME_H_
#define QTK_ONNXRUNTIME_H_
#pragma message(                                                               \
    "qtk_onnxruntime.h is deprecated, use qtk/nnrt/qtk_nnrt.h instead.")
#include "qtk_onnxruntime_cfg.h"
#include "onnxruntime_c_api.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_onnxruntime qtk_onnxruntime_t;
typedef struct qtk_onnx_item qtk_onnx_item_t;

struct qtk_onnx_item
{
	void *val;
	int64_t *shape;
	char *name;
	ONNXTensorElementDataType type;
	int in_dim;
	int64_t shape_len;
	int bytes;
};

struct qtk_onnxruntime
{
	qtk_onnxruntime_cfg_t *cfg;
	const OrtApiBase *base;
	const OrtApi *api;
	OrtEnv *env;
	OrtSessionOptions *sop;
	OrtSession *session;
	OrtAllocator *allocator;
	OrtMemoryInfo *meminfo;
	OrtValue **in;
	OrtValue **out;
	qtk_onnx_item_t *in_items;
	long unsigned int num_in;
	long unsigned int num_out;
	long unsigned int stream_len;
	char **name_in;
	char **name_out;
	char *stream_val;
};

qtk_onnxruntime_t* qtk_onnxruntime_new(qtk_onnxruntime_cfg_t *cfg);
void qtk_onnxruntime_delete(qtk_onnxruntime_t *onnx);
void qtk_onnxruntime_reset(qtk_onnxruntime_t *onnx);
void qtk_onnxruntime_reset2(qtk_onnxruntime_t *onnx);
void qtk_onnxruntime_item_reset(qtk_onnxruntime_t *onnx);
void qtk_onnxruntime_feed(qtk_onnxruntime_t *onnx, void *input, int in_dim, int64_t *in_shape, int shape_len,int type,int index);
void qtk_onnxruntime_run(qtk_onnxruntime_t *onnx);
int64_t* qtk_onnxruntime_get_outshape(qtk_onnxruntime_t *onnx, int index, int64_t *size_len);
int64_t *qtk_onnxruntime_get_inshape(qtk_onnxruntime_t *onnx, int index,
                                     int64_t *size_len);
void* qtk_onnxruntime_getout(qtk_onnxruntime_t *onnx, int index);
void qtk_onnxruntime_print_type_info(qtk_onnxruntime_t *onnx);

#ifdef __cplusplus
};
#endif
#endif
#endif
