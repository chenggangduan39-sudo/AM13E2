#ifndef F13F190A_51ED_4DE2_832B_06FA67CA5E12
#define F13F190A_51ED_4DE2_832B_06FA67CA5E12
#include "qtk/nnrt/qtk_nnrt_cfg.h"
#include "qtk/nnrt/qtk_nnrt_value.h"
#ifdef QTK_NNRT_NCNN
#ifdef __cplusplus
extern "C++" {
#endif
#include "ncnn/c_api.h"
#ifdef __cplusplus
}
#endif
#endif
#ifdef QTK_NNRT_ONNXRUNTIME
#include "onnxruntime_c_api.h"
#endif
#ifdef QTK_NNRT_SS_IPU
#include "mi_ipu.h"
#include "mi_sys.h"
#endif
#ifdef QTK_NNRT_RKNPU
#include "rknn_api.h"

#define RKNN_MAX_INPUT 32
#define RKNN_MAX_OUTPUT 32
#endif

#ifdef QTK_NNRT_LIBTORCH
#include "qtk/nnrt/qtk_nnrt_libtorch.h"
#endif

typedef struct qtk_nnrt qtk_nnrt_t;

struct qtk_nnrt {
    qtk_nnrt_cfg_t *cfg;
    int num_in;
    int num_out;
#ifdef QTK_NNRT_NCNN
    ncnn_net_t net;
    ncnn_option_t ncnn_option;
    ncnn_extractor_t extractor;
    ncnn_allocator_t allocator;
#endif
#ifdef QTK_NNRT_ONNXRUNTIME
    const OrtApi *ort_api;
    OrtEnv *ort_env;
    OrtSession *ort_session;
    OrtSessionOptions *ort_options;
    OrtAllocator *ort_allocator;
    OrtMemoryInfo *ort_meminfo;
    char **ort_input_name;
    char **ort_output_name;
    OrtValue **ort_in;
    OrtValue **ort_out;
#endif
#ifdef QTK_NNRT_RKNPU
    rknn_context ctx;
    rknn_input input[RKNN_MAX_INPUT];
    rknn_tensor_attr input_attrs[RKNN_MAX_INPUT];
    rknn_tensor_attr output_attrs[RKNN_MAX_OUTPUT];
    rknn_output outputs[RKNN_MAX_OUTPUT];
#endif
#ifdef QTK_NNRT_SS_IPU
    MI_U32 u32ChannelID;
    MI_IPU_SubNet_InputOutputDesc_t desc;
    MI_IPU_TensorVector_t InputTensorVector;
    MI_IPU_TensorVector_t OutputTensorVector;
#endif
#ifdef QTK_NNRT_LIBTORCH
    qtk_nnrt_libtorch_t *libtorch;
#endif
};

qtk_nnrt_t *qtk_nnrt_new(qtk_nnrt_cfg_t *cfg);
void qtk_nnrt_delete(qtk_nnrt_t *rt);
int qtk_nnrt_run(qtk_nnrt_t *rt);
int qtk_nnrt_reset(qtk_nnrt_t *rt);
int qtk_nnrt_feed(qtk_nnrt_t *rt, qtk_nnrt_value_t value, int idx);
int qtk_nnrt_feed_s(qtk_nnrt_t *rt, qtk_nnrt_value_t value, const char *name);
int qtk_nnrt_get_output(qtk_nnrt_t *rt, qtk_nnrt_value_t *out, int idx);
int qtk_nnrt_get_output_s(qtk_nnrt_t *rt, qtk_nnrt_value_t *out,
                          const char *name);
int qtk_nnrt_get_num_in(qtk_nnrt_t *rt);
int qtk_nnrt_get_num_out(qtk_nnrt_t *rt);
int qtk_nnrt_metadata_lookup(qtk_nnrt_t *rt, char *key, char *data, int len);
int qtk_nnrt_get_input_shape(qtk_nnrt_t *rt, int idx, int64_t *shape,
                             int shape_cap);
#ifdef QTK_NNRT_ONNXRUNTIME
int qtk_nnrt_set_global_thread_pool(int num_threads, char *affinity);
#endif

#ifdef QTK_NNRT_RKNPU
int qtk_nnrt_feed_image(qtk_nnrt_t *rt, qtk_nnrt_value_t value, int idx);
#endif

qtk_nnrt_value_t qtk_nnrt_value_create_external(qtk_nnrt_t *rt,
                                                qtk_nnrt_value_elem_type_t t,
                                                int64_t *shape, int shape_len,
                                                void *data);
qtk_nnrt_value_t qtk_nnrt_value_create(qtk_nnrt_t *rt,
                                       qtk_nnrt_value_elem_type_t t,
                                       int64_t *shape, int shape_len);
qtk_nnrt_value_t qtk_nnrt_create_input(qtk_nnrt_t *rt, int idx);
int qtk_nnrt_get_output_shape(qtk_nnrt_t *rt, int idx, int64_t *shape,
                              int shape_cap);
size_t qtk_nnrt_get_output_nbytes(qtk_nnrt_t *rt, int idx);
size_t qtk_nnrt_get_output_nelems(qtk_nnrt_t *rt, int idx);
size_t qtk_nnrt_get_input_nbytes(qtk_nnrt_t *rt, int idx);
size_t qtk_nnrt_get_input_nelems(qtk_nnrt_t *rt, int idx);
qtk_nnrt_value_elem_type_t qtk_nnrt_get_input_elem_type(qtk_nnrt_t *rt,
                                                        int idx);
qtk_nnrt_value_elem_type_t qtk_nnrt_get_output_elem_type(qtk_nnrt_t *rt,
                                                         int idx);

void qtk_nnrt_value_release(qtk_nnrt_t *rt, qtk_nnrt_value_t value);
int qtk_nnrt_value_get_shape(qtk_nnrt_t *rt, qtk_nnrt_value_t value,
                             int64_t *shape, int shape_cap);
void *qtk_nnrt_value_get_data(qtk_nnrt_t *rt, qtk_nnrt_value_t value);
void *qtk_nnrt_value_get_channel_data(qtk_nnrt_t *rt, qtk_nnrt_value_t value, int idx);
char *qtk_nnrt_net_get_input_name(qtk_nnrt_t *rt, int idx);
char *qtk_nnrt_net_get_output_name(qtk_nnrt_t *rt, int idx);
qtk_nnrt_value_t qtk_nnrt_value_create_external2(qtk_nnrt_t *rt,
                                                 qtk_nnrt_value_elem_type_t t,
                                                 int64_t *shape, int shape_len,
                                                 void *data, int64_t len);
int qtk_nnrt_disable_output_dequant(qtk_nnrt_t *rt, int idx);

#endif /* F13F190A_51ED_4DE2_832B_06FA67CA5E12 */
