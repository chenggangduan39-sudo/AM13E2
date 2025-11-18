#ifndef WTK_BFIO_MASKNET_WTK_TDNNNET_LAYER
#define WTK_BFIO_MASKNET_WTK_TDNNNET_LAYER
#include "wtk/core/cfg/wtk_local_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_tdnnnet_layer wtk_tdnnnet_layer_t;
struct wtk_tdnnnet_layer
{
    int left_context;
    int right_context;
    int input_dim;
    int output_dim;
    int dilation;
    float **weight;
    float *bias;
    float prelu_w;
    unsigned use_bias:1;
    unsigned use_sigmoid:1;
    unsigned use_relu:1;
    unsigned use_prelu:1;
    unsigned use_tanh:1;
    unsigned use_no_nonlinearity:1;
};

wtk_tdnnnet_layer_t * wtk_tdnnnet_layer_new(wtk_source_t *src, wtk_strbuf_t *buf);
void wtk_tdnnnet_layer_delete(wtk_tdnnnet_layer_t *layer);

#ifdef __cplusplus
};
#endif
#endif