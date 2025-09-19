#ifndef WTK_BFIO_MASKNET_WTK_CNN1DNET
#define WTK_BFIO_MASKNET_WTK_CNN1DNET
#include "wtk_cnn1dnet_layer.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef void (*wtk_cnn1dnet_notify_f)(void *ths,int depth_idx, float *out,int out_channel,int len,int is_end);

typedef struct wtk_cnn1dnet wtk_cnn1dnet_t;

struct wtk_cnn1dnet
{
    wtk_cnn1dnet_layer_t *layer;

    wtk_strbuf_t* in_q;

    int out_len;
    float *out;

    int depth_idx;

    wtk_cnn1dnet_notify_f notify;
    void *ths;

    unsigned start:1;
};

wtk_cnn1dnet_t *wtk_cnn1dnet_new(wtk_cnn1dnet_layer_t *layer, int ifeat_len, int depth_idx);
void wtk_cnn1dnet_reset(wtk_cnn1dnet_t *cnn);
void wtk_cnn1dnet_delete(wtk_cnn1dnet_t *cnn);
void wtk_cnn1dnet_feed(wtk_cnn1dnet_t *cnn, float *feat,int len,int is_end);
void wtk_cnn1dnet_set_notify(wtk_cnn1dnet_t *cnn,void *ths,wtk_cnn1dnet_notify_f notify);

#ifdef __cplusplus
};
#endif
#endif