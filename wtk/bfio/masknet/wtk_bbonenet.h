#ifndef WTK_BFIO_MASKNET_WTK_BBONENET
#define WTK_BFIO_MASKNET_WTK_BBONENET
#include "wtk_bbonenet_cfg.h"
#include "wtk/core/wtk_complex.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef void (*wtk_bbonenet_notify_f)(void *ths, float *gain, int len, int is_end);
typedef void (*wtk_bbonenet_notify_f2)(void *ths, float *gain, int len, int is_end);
typedef struct wtk_bbonenet wtk_bbonenet_t;

struct wtk_bbonenet
{
    wtk_bbonenet_cfg_t *cfg;

    wtk_strbuf_t *input1;
    wtk_strbuf_t *input2;
    wtk_strbuf_t *neti;
    wtk_strbuf_t *neto;

    wtk_dnnnet_t *idnn;
    wtk_cnn1dnet_t *cnn1;
    wtk_cnn1dnet_t *cnn2;
    wtk_grunet_t *gru1;
    wtk_grunet_t *gru2;
    wtk_grunet_t *gru3;
    wtk_dnnnet_t *odnn;

    wtk_bbonenet_notify_f notify;
    void *ths;
};

wtk_bbonenet_t *wtk_bbonenet_new(wtk_bbonenet_cfg_t *cfg);
void wtk_bbonenet_delete(wtk_bbonenet_t *masknet);
void wtk_bbonenet_reset(wtk_bbonenet_t *masknet);
void wtk_bbonenet_feed(wtk_bbonenet_t *masknet, float *data, int len, int len2, int is_end);

void wtk_bbonenet_set_notify(wtk_bbonenet_t *masknet, void *ths, wtk_bbonenet_notify_f notify);

#ifdef __cplusplus
};
#endif
#endif