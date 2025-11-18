#ifndef WTK_BFIO_MASKNET_WTK_DNNNET_LAYER
#define WTK_BFIO_MASKNET_WTK_DNNNET_LAYER
#include "wtk/core/cfg/wtk_local_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_dnnnet_layer wtk_dnnnet_layer_t;
struct wtk_dnnnet_layer
{
    int input_dim;
    int output_dim;
    float **weight;
    float *bias;
    float prelu_w;
    unsigned use_bias:1;
    unsigned use_sigmoid:1;
    unsigned use_relu:1;
    unsigned use_prelu:1;
    unsigned use_tanh:1;
    unsigned use_no_nonlinearity:1;
    unsigned use_log_softmax:1;
};

wtk_dnnnet_layer_t * wtk_dnnnet_layer_new(wtk_source_t *src, wtk_strbuf_t *buf);
void wtk_dnnnet_layer_delete(wtk_dnnnet_layer_t *layer);

#ifdef __cplusplus
};
#endif
#endif