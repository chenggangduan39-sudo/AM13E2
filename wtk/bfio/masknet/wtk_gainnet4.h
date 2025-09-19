#ifndef WTK_BFIO_MASKNET_WTK_GAINNET4
#define WTK_BFIO_MASKNET_WTK_GAINNET4
#include "wtk_gainnet4_cfg.h"
#include "wtk/core/wtk_complex.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef void (*wtk_gainnet4_notify_f)(void *ths, float *gain, int len, int is_end);
typedef void (*wtk_gainnet4_notify_f2)(void *ths, float *gain, int len, int is_end);
typedef struct wtk_gainnet4 wtk_gainnet4_t;

struct wtk_gainnet4
{
    wtk_gainnet4_cfg_t *cfg;

    wtk_strbuf_t *mici;
    wtk_strbuf_t *x;
    wtk_strbuf_t *x2;
    wtk_strbuf_t *neti;
    wtk_strbuf_t *neto;

    wtk_dnnnet_t *ns_dnn;
    wtk_grunet_t *ns_gru;
    wtk_grunet_t *ns_gru2;
    wtk_grunet_t *ns_gru3;
    wtk_dnnnet_t *ns_odnn;

    wtk_dnnnet_t *xv_dnn;
    wtk_cnnnet_t *der_cnn;
    wtk_dnnnet_t *der_dnn;
    wtk_grunet_t *der_gru;
    wtk_grunet_t *der_gru2;
    wtk_grunet_t *der_gru3;
    wtk_dnnnet_t *der_odnn;

    wtk_gainnet4_notify_f notify;
    void *ths;

    wtk_gainnet4_notify_f2 notify2;
    void *ths2;
};

wtk_gainnet4_t *wtk_gainnet4_new(wtk_gainnet4_cfg_t *cfg);
void wtk_gainnet4_delete(wtk_gainnet4_t *masknet);
void wtk_gainnet4_reset(wtk_gainnet4_t *masknet);
void wtk_gainnet4_feed(wtk_gainnet4_t *masknet, float *data, int len, float *xv, int xv_len, int is_end);

void wtk_gainnet4_set_notify(wtk_gainnet4_t *masknet, void *ths, wtk_gainnet4_notify_f notify);
void wtk_gainnet4_set_notify2(wtk_gainnet4_t *masknet, void *ths, wtk_gainnet4_notify_f2 notify2);
#ifdef __cplusplus
};
#endif
#endif