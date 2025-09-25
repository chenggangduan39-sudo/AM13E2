#ifndef WTK_BFIO_MASKNET_WTK_LSTMNET_LAYER
#define WTK_BFIO_MASKNET_WTK_LSTMNET_LAYER
#include "wtk/core/cfg/wtk_local_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_lstmnet_layer wtk_lstmnet_layer_t;

typedef struct
{
    float **weight_ih_l;
    float **weight_hh_l;
    float *bias_ih_l;
    float *bias_hh_l;

#ifdef USE_NEON
    float *weight_h_l;
    float *weight_i_l;
    float *lstm_tmp1;
    float *lstm_tmp2;
#endif
    int input_idim;
    int output_idim;

    int input_hdim;
    int output_hdim;

}wtk_lstmnet_wb_t;

struct wtk_lstmnet_layer
{
    int input_dim;
    int lstm_hidden;
    int lstm_depth;

    wtk_lstmnet_wb_t *lstm_wb;
    float prelu_w;

    unsigned use_sigmoid:1;
    unsigned use_relu:1;
    unsigned use_prelu:1;
    unsigned use_tanh:1;
    unsigned use_no_nonlinearity:1;
};

wtk_lstmnet_layer_t * wtk_lstmnet_layer_new(wtk_source_t *src, wtk_strbuf_t *buf);
void wtk_lstmnet_layer_delete(wtk_lstmnet_layer_t *layer);

#ifdef __cplusplus
};
#endif
#endif