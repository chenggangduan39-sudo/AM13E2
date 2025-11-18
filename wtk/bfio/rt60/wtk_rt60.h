#ifndef WTK_BFIO_RT60_WTK_RT60
#define WTK_BFIO_RT60_WTK_RT60
#include "wtk/bfio/maskdenoise/wtk_drft.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/core/wtk_strbuf.h"
#include "wtk_rt60_cfg.h"
#include <float.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_rt60 wtk_rt60_t;
typedef void (*wtk_rt60_notify_f)(void *ths, float rt60_val);

struct wtk_rt60 {
    wtk_rt60_cfg_t *cfg;

    wtk_strbuf_t **mic;

    void *ths;
    wtk_rt60_notify_f notify;
};

wtk_rt60_t *wtk_rt60_new(wtk_rt60_cfg_t *cfg);
void wtk_rt60_delete(wtk_rt60_t *rt60);
void wtk_rt60_start(wtk_rt60_t *rt60);
void wtk_rt60_reset(wtk_rt60_t *rt60);
void wtk_rt60_set_notify(wtk_rt60_t *rt60, void *ths,
                            wtk_rt60_notify_f notify);
/**
 * len=mic array samples
 */
void wtk_rt60_feed(wtk_rt60_t *rt60, short *data, int len, int is_end);

short *wtk_rt60_get_play_signal(wtk_rt60_t *rt60, int *len);
#ifdef __cplusplus
};
#endif
#endif
