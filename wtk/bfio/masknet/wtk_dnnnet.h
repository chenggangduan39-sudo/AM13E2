#ifndef WTK_BFIO_MASKNET_WTK_DNNNET
#define WTK_BFIO_MASKNET_WTK_DNNNET
#ifdef USE_NEON
#include <arm_neon.h>
#endif
#include "wtk_dnnnet_layer.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef void (*wtk_dnnnet_notify_f)(void *ths,int depth_idx, float *out,int len,int is_end);

typedef struct wtk_dnnnet wtk_dnnnet_t;

struct wtk_dnnnet
{
    wtk_dnnnet_layer_t *layer;

    float *out;
    
    int depth_idx;

    wtk_dnnnet_notify_f notify;
    void *ths;
};

wtk_dnnnet_t *wtk_dnnnet_new(wtk_dnnnet_layer_t *layer, int depth_idx);
void wtk_dnnnet_reset(wtk_dnnnet_t *dnn);
void wtk_dnnnet_delete(wtk_dnnnet_t *dnn);
void wtk_dnnnet_feed(wtk_dnnnet_t *dnn, float *feat,int len,int is_end);
void wtk_dnnnet_set_notify(wtk_dnnnet_t *dnn,void *ths,wtk_dnnnet_notify_f notify);



#ifdef __cplusplus
};
#endif
#endif