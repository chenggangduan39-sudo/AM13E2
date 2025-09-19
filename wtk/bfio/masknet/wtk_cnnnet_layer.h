#ifndef WTK_BFIO_MASKNET_WTK_CNNNET_LAYER
#define WTK_BFIO_MASKNET_WTK_CNNNET_LAYER
#include "wtk/core/cfg/wtk_local_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_cnnnet_layer wtk_cnnnet_layer_t;
struct wtk_cnnnet_layer
{
    int in_channel;
    int out_channel;
    int zeropad2d[4];
    int kernel_size[2];
    int dilation[2];
    int stride[2];
    float **weight;
    float *bias;
    float prelu_w;

    unsigned use_bias:1;
    unsigned use_sigmoid:1;
    unsigned use_tanh:1;
    unsigned use_relu:1;
    unsigned use_prelu:1;
    unsigned use_no_nonlinearity:1;
};

wtk_cnnnet_layer_t * wtk_cnnnet_layer_new(wtk_source_t *src, wtk_strbuf_t *buf);
void wtk_cnnnet_layer_delete(wtk_cnnnet_layer_t *layer);

#ifdef __cplusplus
};
#endif
#endif