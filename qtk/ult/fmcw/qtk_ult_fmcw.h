#ifndef G_862DFEDF77864F998FCDA2EF05D3D821
#define G_862DFEDF77864F998FCDA2EF05D3D821

#include "qtk/ult/fmcw/qtk_ult_fmcw_cfg.h"
#include "wtk/signal/wtk_conv2.h"

typedef struct qtk_ult_fmcw qtk_ult_fmcw_t;
typedef int (*qtk_ult_fmcw_notifier_t)(void *upval, wtk_complex_t *samp);

typedef enum {
    QTK_ULT_FMCW_INIT,
    QTK_ULT_FMCW_ALIGNED,
} qtk_ult_fmcw_state_t;

struct qtk_ult_fmcw {
    qtk_ult_fmcw_cfg_t *cfg;
    wtk_complex_t *cir;
    wtk_conv2_cpx_t **conv;
    wtk_complex_t *samples;
    wtk_complex_t *frames;
    int pos;
    int sample_pos;
    uint32_t pre_sync_pos;
    uint32_t nframe;
    float cur_peak;

    void *upval;
    qtk_ult_fmcw_notifier_t notifier;
    qtk_ult_fmcw_state_t state;
};

qtk_ult_fmcw_t *qtk_ult_fmcw_new(qtk_ult_fmcw_cfg_t *cfg, void *upval,
                                 qtk_ult_fmcw_notifier_t notifier);
void qtk_ult_fmcw_delete(qtk_ult_fmcw_t *m);
int qtk_ult_fmcw_feed(qtk_ult_fmcw_t *fmcw, short **wav, int len);

#endif
