#ifndef WTK_BFIO_MASKNET_WTK_GAINNET
#define WTK_BFIO_MASKNET_WTK_GAINNET
#include "wtk_gainnet_cfg.h"
#include "wtk/core/wtk_complex.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef void (*wtk_gainnet_notify_f)(void *ths, float *gain, int len, int is_end);

typedef struct wtk_gainnet wtk_gainnet_t;

struct wtk_gainnet
{
    wtk_gainnet_cfg_t *cfg;

    wtk_strbuf_t *input;
    wtk_strbuf_t *neti;
    wtk_strbuf_t *neto;

    wtk_dnnnet_t *in_dnn;
    wtk_grunet_t *vad_gru;
    wtk_lstmnet_t *vad_lstm;
    // wtk_dnnnet_t *vad_odnn;

    wtk_grunet_t *noise_gru;
    wtk_lstmnet_t *noise_lstm;
    wtk_grunet_t *denoise_gru;
    wtk_lstmnet_t *denoise_lstm;
    wtk_dnnnet_t *denoise_odnn;
    
    wtk_gainnet_notify_f notify;
    void *ths;

    float vad;
};

wtk_gainnet_t *wtk_gainnet_new(wtk_gainnet_cfg_t *cfg);
void wtk_gainnet_delete(wtk_gainnet_t *masknet);
void wtk_gainnet_reset(wtk_gainnet_t *masknet);
void wtk_gainnet_feed(wtk_gainnet_t *masknet, float *data, int len, int is_end);
void wtk_gainnet_feed2(wtk_gainnet_t *masknet, float *data, int len, float *data2, int len2, int is_end);
void wtk_gainnet_set_notify(wtk_gainnet_t *masknet, void *ths, wtk_gainnet_notify_f notify);
#ifdef __cplusplus
};
#endif
#endif