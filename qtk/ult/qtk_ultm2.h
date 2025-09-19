#ifndef WTK_ULT_WTK_ULTM2
#define WTK_ULT_WTK_ULTM2
#include "qtk/ult/qtk_ultm2_cfg.h"
#include "wtk/core/wtk_fring.h"
#include "wtk/signal/qtk_fft.h"
#include "wtk/signal/wtk_conv2.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_ultm2 qtk_ultm2_t;
typedef int (*qtk_ultm2_notify_f)(void *upval, float **data, int len);

typedef enum {
    QTK_ULTM2_SEEK_START,
    QTK_ULTM2_INIT,
    QTK_ULTM2_ALIGND,
} qtk_ultm2_state_t;

struct qtk_ultm2 {
    qtk_ultm2_cfg_t *cfg;

    wtk_strbuf_t *echo_raw;
    wtk_strbuf_t *echo_conved;
    wtk_conv2_t *echo_conv;
    wtk_conv2_t *echo_align_conv[2];
    wtk_strbuf_t *echo_align_raw[2];
    wtk_strbuf_t *echo_align_conved[2];

    wtk_conv2_t *mic_h_conv;
    wtk_strbuf_t *mic_h;

    qtk_fft_t *fft;
    wtk_strbuf_t *mic_raw;
    wtk_strbuf_t **mic_conved;
    wtk_conv2_t **conv2;
    float *pv_tmp;
    float **pv;
    int L;
    int skip;
    int nx;
    int nframe;
    int skipped;
    int echo_hint;
    int cut_hint;
    int harm_hint;
    int mic_err;
    float last_hamonic_e;
    float last_echo_e;  // 回声超声能量
    float last_echo_e2; // 回声能量
    float last_mic_e;   // 录音超声能量
    qtk_ultm2_state_t state;
    wtk_fring_t *align_idx;
    void *upval;
    qtk_ultm2_notify_f notify;
    unsigned echoed : 1;
    unsigned cuted : 1;
    unsigned harmed : 1;
};

qtk_ultm2_t *qtk_ultm2_new(qtk_ultm2_cfg_t *cfg, int L, int skip);
void qtk_ultm2_delete(qtk_ultm2_t *m);
void qtk_ultm2_start(qtk_ultm2_t *m);
void qtk_ultm2_reset(qtk_ultm2_t *m);
float qtk_ultm2_get_e(qtk_ultm2_t *m);
void qtk_ultm2_feed(qtk_ultm2_t *m, char **data, int len, int is_end,
                    int nchannel);
void qtk_ultm2_set_notify(qtk_ultm2_t *m, qtk_ultm2_notify_f notify,
                          void *upval);

#ifdef __cplusplus
};
#endif
#endif
