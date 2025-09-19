#ifndef WTK_BFIO_MASKNET_WTK_LSTMNET
#define WTK_BFIO_MASKNET_WTK_LSTMNET
#include "wtk_lstmnet_layer.h"
#ifdef USE_NEON
#include <arm_neon.h>
#endif
#ifdef __cplusplus
extern "C" {
#endif
typedef void (*wtk_lstmnet_notify_f)(void *ths, int depth_idx, float *out, int len, int is_end);

typedef struct wtk_lstmnet wtk_lstmnet_t;

typedef struct
{
    wtk_queue_node_t q_n;

    float *ft;
    float *it;
    float *gt;
    float *ot;

    float *h_out;
    float *c;

    int lstm_hidden;

    wtk_lstmnet_wb_t *wb;
}wtk_lstmnet_node_t;

struct wtk_lstmnet
{
    wtk_lstmnet_layer_t *layer;

     int depth_idx;

    wtk_queue_t lstm_q;

    float *out;

    void *ths;
    wtk_lstmnet_notify_f notify;
};

wtk_lstmnet_t *wtk_lstmnet_new(wtk_lstmnet_layer_t *layer, int depth_idx);
void wtk_lstmnet_delete(wtk_lstmnet_t *lstm);
void wtk_lstmnet_reset(wtk_lstmnet_t *lstm);

void wtk_lstmnet_feed(wtk_lstmnet_t *lstm, float *data, int len, int is_end);
void wtk_lstmnet_set_notify(wtk_lstmnet_t *lstm, void *ths, wtk_lstmnet_notify_f notify);


#ifdef __cplusplus
};
#endif
#endif