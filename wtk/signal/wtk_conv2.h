#ifndef WTK_SIGNAL_WTK_CONV2
#define WTK_SIGNAL_WTK_CONV2
#include "wtk/signal/qtk_fft.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_conv2 wtk_conv2_t;
typedef struct wtk_conv2_cpx wtk_conv2_cpx_t;

typedef void(*wtk_conv2_notify_f)(void *usr_ths,float *data,int len);
typedef void (*wtk_conv2_cpx_notify_f)(void *usr_ths, wtk_complex_t *data,
                                       int len);

struct wtk_conv2
{
    qtk_fft_t *fft;
    int klen;
    int len;
    int shift;
    float *kf;
    float *xf;
    float *xx;
    float *cf;
    float *cx;
    wtk_strbuf_t *buf;
    int nframe;
    float *dst;
    void *ths;
    wtk_conv2_notify_f notify;
    int fft_len;
    char ref_fft;
};

struct wtk_conv2_cpx {
    qtk_cfft_t *fft;
    int klen;
    int len;
    int shift;
    wtk_complex_t *kf;
    wtk_complex_t *cf;
    wtk_complex_t *dst;
    wtk_complex_t *zero_tmp;
    wtk_complex_t *tmp;
    int nframe;
    int fft_len;
};

wtk_conv2_t *wtk_conv2_new(float *kernel, int klen, qtk_fft_t *fft, int ilen);
void wtk_conv2_delete(wtk_conv2_t *conv);
void wtk_conv2_reset(wtk_conv2_t *conv);
void wtk_conv2_set_notify(wtk_conv2_t *conv,wtk_conv2_notify_f notify,void *ths);
void wtk_conv2_feed(wtk_conv2_t *conv,float *data,int len,int is_end);
void wtk_conv2_feed_frame(wtk_conv2_t *conv, float *pv, int len);

wtk_conv2_cpx_t *wtk_conv2_cpx_new(wtk_complex_t *kernel, int klen, int ilen);
void wtk_conv2_cpx_delete(wtk_conv2_cpx_t *conv);
void wtk_conv2_cpx_reset(wtk_conv2_cpx_t *conv);
int wtk_conv2_cpx_feed_frame(wtk_conv2_cpx_t *conv, wtk_complex_t *data,
                             wtk_complex_t **result);

#ifdef __cplusplus
};
#endif
#endif
