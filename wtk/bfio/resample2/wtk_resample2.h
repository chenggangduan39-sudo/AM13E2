#ifndef WTK_BFIO_RESAMPLE_WTK_RESAMPLE2
#define WTK_BFIO_RESAMPLE_WTK_RESAMPLE2

#include "wtk/core/math/wtk_math.h"
#include "wtk/bfio/maskdenoise/wtk_drft.h"
#include "wtk/core/fft/wtk_rfft.h"
#include "wtk/core/wtk_strbuf.h"
#include "wtk/bfio/resample2/wtk_resample2_cfg.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef int(*wtk_resample2_notify_f)(void *ths,char *data,int len);

typedef struct wtk_resample2 wtk_resample2_t;

struct wtk_resample2
{
    wtk_resample2_cfg_t *cfg;
    wtk_rfft_t *fft;
    wtk_rfft_t *ifft;
    float *F;
    // float *iF;
    wtk_complex_t *freq;
    wtk_complex_t *weight;
    wtk_strbuf_t *buf;
    float *cache;
    float *input;
    float *output;
    float *frame;
    void *ths;
    wtk_resample2_notify_f notify;
};


wtk_resample2_t* wtk_resample2_new(wtk_resample2_cfg_t *cfg);
int wtk_resample2_start(wtk_resample2_t *r);
int wtk_resample2_delete(wtk_resample2_t *r);
void wtk_resample2_set_notify(wtk_resample2_t *r,void *ths,wtk_resample2_notify_f notify);
int wtk_resample2_feed(wtk_resample2_t *f,short *data,int len,int is_end);

#ifdef __cplusplus
}
#endif

#endif // WTK_BFIO_RESAMPLE_WTK_RESAMPLE2
