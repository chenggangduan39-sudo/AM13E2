#ifndef WTK_BFIO_MASKNET_WTK_TDNNNET
#define WTK_BFIO_MASKNET_WTK_TDNNNET
#include "wtk_tdnnnet_layer.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef void (*wtk_tdnnnet_notify_f)(void *ths,int depth_idx,float *out,int len,int is_end);

typedef struct wtk_tdnnnet wtk_tdnnnet_t;

typedef struct 
{
    wtk_queue_node_t q_n;
    float *ifeat;
    int feat_len;
}wtk_tdnnnet_in_t;

struct wtk_tdnnnet
{
    wtk_tdnnnet_layer_t *layer;

    wtk_queue_t in_q;
    float *out;

    int depth_idx;
    
    wtk_tdnnnet_notify_f notify;
    void *ths;
};

wtk_tdnnnet_t *wtk_tdnnnet_new(wtk_tdnnnet_layer_t *layer,int depth_idx);
void wtk_tdnnnet_reset(wtk_tdnnnet_t *tdnn);
void wtk_tdnnnet_delete(wtk_tdnnnet_t *tdnn);
void wtk_tdnnnet_feed(wtk_tdnnnet_t *tdnn, float *feat,int len,int is_end);
void wtk_tdnnnet_set_notify(wtk_tdnnnet_t *tdnn,void *ths,wtk_tdnnnet_notify_f notify);



#ifdef __cplusplus
};
#endif
#endif