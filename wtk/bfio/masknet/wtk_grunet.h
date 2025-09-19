#ifndef WTK_BFIO_MASKNET_WTK_GRUNET
#define WTK_BFIO_MASKNET_WTK_GRUNET
#include "wtk_grunet_layer.h"
#ifdef USE_NEON
#include <arm_neon.h>
#endif
#ifdef __cplusplus
extern "C" {
#endif
typedef void (*wtk_grunet_notify_f)(void *ths, int depth_idx, float *out, int len, int is_end);

typedef struct wtk_grunet wtk_grunet_t;

typedef struct
{
    wtk_queue_node_t q_n;

    float *rt;
    float *zt;
    float *nt;
    
    float *h_out;

    int gru_hidden;

    wtk_grunet_wb_t *wb;
}wtk_grunet_node_t;

struct wtk_grunet
{
    wtk_grunet_layer_t *layer;

     int depth_idx;

    wtk_queue_t gru_q;

    float *out;

    void *ths;
    wtk_grunet_notify_f notify;
};

wtk_grunet_t *wtk_grunet_new(wtk_grunet_layer_t *layer, int depth_idx);
void wtk_grunet_delete(wtk_grunet_t *gru);
void wtk_grunet_reset(wtk_grunet_t *gru);

void wtk_grunet_feed(wtk_grunet_t *gru, float *data, int len, int is_end);
void wtk_grunet_set_notify(wtk_grunet_t *gru, void *ths, wtk_grunet_notify_f notify);

#ifdef __cplusplus
};
#endif
#endif
