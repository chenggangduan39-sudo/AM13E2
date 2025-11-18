#ifndef WTK_BFIO_MASKNET_WTK_GAINNET7
#define WTK_BFIO_MASKNET_WTK_GAINNET7
#include "wtk_gainnet7_cfg.h"
#include "wtk/core/wtk_complex.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef void (*wtk_gainnet7_notify_f)(void *ths, float *gain, int len, int is_end);
typedef void (*wtk_gainnet7_notify_f2)(void *ths, float *gain, int len, int is_end);
typedef struct wtk_gainnet7 wtk_gainnet7_t;

struct wtk_gainnet7
{
    wtk_gainnet7_cfg_t *cfg;

    wtk_strbuf_t *mici;
    wtk_strbuf_t *x;
    wtk_strbuf_t *neti;
    wtk_strbuf_t *neto;

    wtk_dnnnet_t *ns_dnn;
    wtk_grunet_t *ns_gru;
    wtk_grunet_t *ns_gru2;
    wtk_grunet_t *ns_gru3;
    wtk_dnnnet_t *ns_odnn;

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

    wtk_gainnet7_notify_f notify;
    void *ths;

    wtk_gainnet7_notify_f2 notify2;
    void *ths2;
    // float vad;

    unsigned feed_agc:1;
};

wtk_gainnet7_t *wtk_gainnet7_new(wtk_gainnet7_cfg_t *cfg);
void wtk_gainnet7_delete(wtk_gainnet7_t *masknet);
void wtk_gainnet7_reset(wtk_gainnet7_t *masknet);
void wtk_gainnet7_feed(wtk_gainnet7_t *masknet, float *data, int len, int is_end);
void wtk_gainnet7_feed2(wtk_gainnet7_t *masknet, float *data, int len, int is_end);

void wtk_gainnet7_feed3(wtk_gainnet7_t *masknet, float *data, int len,int is_end);
void wtk_gainnet7_feed_agc(wtk_gainnet7_t *masknet, float *g, int len,int is_end);

void wtk_gainnet7_feed4(wtk_gainnet7_t *masknet, float *data, int len,float *g, int len2,int is_end);
void wtk_gainnet7_set_notify(wtk_gainnet7_t *masknet, void *ths, wtk_gainnet7_notify_f notify);
void wtk_gainnet7_set_notify2(wtk_gainnet7_t *masknet, void *ths, wtk_gainnet7_notify_f2 notify2);
#ifdef __cplusplus
};
#endif
#endif