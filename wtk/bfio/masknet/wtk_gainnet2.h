#ifndef WTK_BFIO_MASKNET_WTK_GAINNET2
#define WTK_BFIO_MASKNET_WTK_GAINNET2
#include "wtk_gainnet2_cfg.h"
#include "wtk/core/wtk_complex.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef void (*wtk_gainnet2_notify_f)(void *ths, float *gain, int len, int is_end);
typedef void (*wtk_gainnet2_notify_f2)(void *ths, float *gain, int len, int is_end);
typedef struct wtk_gainnet2 wtk_gainnet2_t;

struct wtk_gainnet2
{
    wtk_gainnet2_cfg_t *cfg;

    wtk_strbuf_t *input;
    wtk_strbuf_t *neti;
    wtk_strbuf_t *neto;
    wtk_strbuf_t *mici;

    wtk_cnnnet_t *icnn;
    wtk_dnnnet_t *sdnn;
    wtk_dnnnet_t *dnn1;
    wtk_grunet_t *gru1;
    wtk_grunet_t *gru2;
    wtk_grunet_t *gru3;
    wtk_dnnnet_t *odnn;

    wtk_dnnnet_t *agc_dnn;
    wtk_grunet_t *agc_gru;
    wtk_grunet_t *agc_gru2;
    wtk_grunet_t *agc_gru3;
    wtk_dnnnet_t *agc_odnn;

    wtk_gainnet2_notify_f notify;
    void *ths;

    wtk_gainnet2_notify_f2 notify2;
    void *ths2;
};

wtk_gainnet2_t *wtk_gainnet2_new(wtk_gainnet2_cfg_t *cfg);
void wtk_gainnet2_delete(wtk_gainnet2_t *masknet);
void wtk_gainnet2_reset(wtk_gainnet2_t *masknet);
void wtk_gainnet2_feed(wtk_gainnet2_t *masknet, float *data, int len, int len2, int is_end);
void wtk_gainnet2_feed2(wtk_gainnet2_t *masknet, float *data, int len, float *data2, int len2, int is_end);
void wtk_gainnet2_feed3(wtk_gainnet2_t *masknet, float *data, int len, float *data2, int len2, float *data3, int len3, float *data4, int len4, int is_end);

void wtk_gainnet2_set_notify(wtk_gainnet2_t *masknet, void *ths, wtk_gainnet2_notify_f notify);
void wtk_gainnet2_set_notify2(wtk_gainnet2_t *masknet, void *ths, wtk_gainnet2_notify_f2 notify);

#ifdef __cplusplus
};
#endif
#endif