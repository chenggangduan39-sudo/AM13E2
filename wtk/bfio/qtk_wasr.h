#ifndef __WTK_BFIO_QTK_WASR_H__
#define __WTK_BFIO_QTK_WASR_H__
#include "wtk/asr/vad/wtk_vad2.h"
#include "wtk/asr/wakeup/wtk_kwake.h"
#include "wtk/asr/wfst/kaldifst/qtk_decoder_wrapper.h"
#include "wtk/bfio/qform/wtk_qmmse.h"
#include "wtk/bfio/qtk_wasr_cfg.h"
#include "wtk/core/fft/wtk_stft2.h"
#include "wtk/core/wtk_buf.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_wasr qtk_wasr_t;
typedef struct qtk_wasr_res qtk_wasr_res_t;
typedef enum {
    QTK_WASR_REC,
    QTK_WASR_WAKEUP,
} qtk_wasr_res_type_t;

struct qtk_wasr_res {
    qtk_wasr_res_type_t t;
    float fs;
    float fe;
    union {
        struct {
            char *res;
            int len;
        } rec;
    } v;
};

typedef int (*qtk_wasr_notify_t)(void *upval, qtk_wasr_res_t *res);

struct qtk_wasr {
    qtk_wasr_cfg_t *cfg;

    qtk_decoder_wrapper_t *decoder;
    wtk_vad2_t *vad;
    wtk_kwake_t *wakeup;
    wtk_qmmse_t *qmmse;
    wtk_stft2_t *stft2;
    float *pad;
    float *denoise_output;
    wtk_complex_t *freq;
    unsigned int vad_output;
    unsigned int wake_start;
    qtk_wasr_notify_t notify;
    void *upval;
    wtk_buf_t *cache;

    float rec_fs;
    float rec_fe;

    unsigned speeching;
    unsigned waked;
};

qtk_wasr_t *qtk_wasr_new(qtk_wasr_cfg_t *cfg);
void qtk_wasr_delete(qtk_wasr_t *w);
int qtk_wasr_feed(qtk_wasr_t *w, short *d, int len);
int qtk_wasr_feed_end(qtk_wasr_t *w);
void qtk_wasr_set_notify(qtk_wasr_t *w, qtk_wasr_notify_t notify, void *upval);

#ifdef __cplusplus
};
#endif
#endif
