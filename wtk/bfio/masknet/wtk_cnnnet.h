#ifndef WTK_BFIO_MASKNET_WTK_CNNNET
#define WTK_BFIO_MASKNET_WTK_CNNNET
#include "wtk_cnnnet_layer.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef void (*wtk_cnnnet_notify_f)(void *ths,int depth_idx, float **out,int out_channel,int len,int is_end);

typedef struct wtk_cnnnet wtk_cnnnet_t;

typedef struct 
{
    wtk_queue_node_t q_n;
    float **ifeat;
    int in_channel;
    int feat_len;
}wtk_cnnnet_in_t;

struct wtk_cnnnet
{
    wtk_cnnnet_layer_t *layer;

    wtk_queue_t in_q;

    int out_len;
    float **out;

    int depth_idx;

    wtk_cnnnet_notify_f notify;
    void *ths;

    unsigned start:1;
};

wtk_cnnnet_t *wtk_cnnnet_new(wtk_cnnnet_layer_t *layer, int ifeat_len, int depth_idx);
void wtk_cnnnet_reset(wtk_cnnnet_t *cnn);
void wtk_cnnnet_delete(wtk_cnnnet_t *cnn);
void wtk_cnnnet_feed(wtk_cnnnet_t *cnn, float **feat,int len,int is_end);
void wtk_cnnnet_set_notify(wtk_cnnnet_t *cnn,void *ths,wtk_cnnnet_notify_f notify);

#ifdef __cplusplus
};
#endif
#endif