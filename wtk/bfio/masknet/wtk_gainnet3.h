#ifndef WTK_BFIO_MASKNET_WTK_GAINNET3
#define WTK_BFIO_MASKNET_WTK_GAINNET3
#include "wtk_gainnet3_cfg.h"
#include "wtk/core/wtk_complex.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef void (*wtk_gainnet3_notify_f)(void *ths, float *gain, int len, int is_end);
typedef void (*wtk_gainnet3_notify_f2)(void *ths, float *gain, int len, int is_end);
typedef struct wtk_gainnet3 wtk_gainnet3_t;

struct wtk_gainnet3
{
    wtk_gainnet3_cfg_t *cfg;

    wtk_strbuf_t *mici;
    wtk_strbuf_t *x;
    wtk_strbuf_t *neti;
    wtk_strbuf_t *neto;

    wtk_dnnnet_t *ns_dnn;
    wtk_grunet_t *ns_gru;
    wtk_grunet_t *ns_gru2;
    wtk_grunet_t *ns_gru3;

    wtk_cnnnet_t *der_cnn;
    wtk_dnnnet_t *der_dnn;
    wtk_grunet_t *der_gru;
    wtk_grunet_t *der_gru2;
    wtk_grunet_t *der_gru3;
    wtk_dnnnet_t *der_odnn;

    wtk_dnnnet_t *agc_dnn;
    wtk_grunet_t *agc_gru;
    wtk_grunet_t *agc_gru2;
    wtk_grunet_t *agc_gru3;
    wtk_dnnnet_t *agc_odnn;

    wtk_gainnet3_notify_f notify;
    void *ths;

    wtk_gainnet3_notify_f2 notify2;
    void *ths2;
    // float vad;

    unsigned feed_agc:1;
};

wtk_gainnet3_t *wtk_gainnet3_new(wtk_gainnet3_cfg_t *cfg);
void wtk_gainnet3_delete(wtk_gainnet3_t *masknet);
void wtk_gainnet3_reset(wtk_gainnet3_t *masknet);
void wtk_gainnet3_feed(wtk_gainnet3_t *masknet, float *data, int len, int is_end);

void wtk_gainnet3_set_notify(wtk_gainnet3_t *masknet, void *ths, wtk_gainnet3_notify_f notify);
void wtk_gainnet3_set_notify2(wtk_gainnet3_t *masknet, void *ths, wtk_gainnet3_notify_f2 notify2);
#ifdef __cplusplus
};
#endif
#endif