#ifndef WTK_BFIO_MASKNET_WTK_MASKNET
#define WTK_BFIO_MASKNET_WTK_MASKNET
#include "wtk_masknet_cfg.h"
#include "wtk/core/wtk_complex.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef void (*wtk_masknet_notify_f)(void *ths, float *mask, int len, int is_end);
typedef void (*wtk_masknet_notify_f2)(void *ths, float *mask, int len, int idx, int is_end);

typedef struct wtk_masknet wtk_masknet_t;

typedef enum
{
    WTK_CNNNET_NET,
    WTK_LSTMNET_NET,
    WTK_TDNNNET_NET,
    WTK_DNNNET_NET,
    WTK_GRUNET_NET,
}wtk_masknet_net_type_t;

typedef struct
{
    wtk_queue_node_t q_n;

    wtk_cnnnet_t *cnn;
    wtk_lstmnet_t *lstm;
    wtk_grunet_t *gru;
    wtk_tdnnnet_t *tdnn;
    wtk_dnnnet_t *dnn;

    wtk_masknet_net_type_t type;
}wtk_masknet_net_t;

struct wtk_masknet
{
    wtk_masknet_cfg_t *cfg;
    wtk_queue_t net_q;

    int ifeat_size[2];
    float **ifeat;
    
    wtk_masknet_notify_f notify;
    void *ths;
    wtk_masknet_notify_f2 notify2;

    int idx;
};

wtk_masknet_t *wtk_masknet_new(wtk_masknet_cfg_t *cfg);
void wtk_masknet_delete(wtk_masknet_t *masknet);
void wtk_masknet_reset(wtk_masknet_t *masknet);
void wtk_masknet_feed(wtk_masknet_t *masknet, float *data, int len, int is_end);
void wtk_masknet_set_notify(wtk_masknet_t *masknet, void *ths, wtk_masknet_notify_f notify);
void wtk_masknet_set_notify2(wtk_masknet_t *masknet, void *ths, wtk_masknet_notify_f2 notify2);
#ifdef __cplusplus
};
#endif
#endif