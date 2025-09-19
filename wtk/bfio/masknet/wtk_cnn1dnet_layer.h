#ifndef WTK_BFIO_MASKNET_WTK_CNN1DNET_LAYER
#define WTK_BFIO_MASKNET_WTK_CNN1DNET_LAYER
#include "wtk/core/cfg/wtk_local_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_cnn1dnet_layer wtk_cnn1dnet_layer_t;
struct wtk_cnn1dnet_layer
{
    int in_channel;
    int out_channel;
    int zeropad2d[2];
    int kernel_size;
    int dilation;
    int stride;
    float prelu_w;
    float **weight;
    float *bias;

    unsigned use_bias:1;
    unsigned use_sigmoid:1;
    unsigned use_tanh:1;
    unsigned use_relu:1;
    unsigned use_prelu:1;
    unsigned use_no_nonlinearity:1;
};

wtk_cnn1dnet_layer_t * wtk_cnn1dnet_layer_new(wtk_source_t *src, wtk_strbuf_t *buf);
void wtk_cnn1dnet_layer_delete(wtk_cnn1dnet_layer_t *layer);

#ifdef __cplusplus
};
#endif
#endif
