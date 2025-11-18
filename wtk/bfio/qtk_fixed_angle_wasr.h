#ifndef __WTK_BFIO_QTK_FIXED_ANGLE_WASR_H__
#define __WTK_BFIO_QTK_FIXED_ANGLE_WASR_H__
#include "wtk/bfio/aec/wtk_aec.h"
#include "wtk/bfio/qform/wtk_qform2.h"
#include "wtk/bfio/qtk_fixed_angle_wasr_cfg.h"
#include "wtk/bfio/qtk_wasr.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_fixed_angle_wasr qtk_fixed_angle_wasr_t;

struct qtk_fixed_angle_wasr {
    qtk_fixed_angle_wasr_cfg_t *cfg;
    qtk_wasr_t *wasr;
    wtk_qform2_t *qform2;
    wtk_aec_t *aec;
};

qtk_fixed_angle_wasr_t *
qtk_fixed_angle_wasr_new(qtk_fixed_angle_wasr_cfg_t *cfg);
void qtk_fixed_angle_wasr_delete(qtk_fixed_angle_wasr_t *w);
int qtk_fixed_angle_wasr_feed(qtk_fixed_angle_wasr_t *w, short **d, int len);
int qtk_fixed_angle_wasr_feed_end(qtk_fixed_angle_wasr_t *w);
void qtk_fixed_angle_wasr_set_wasr_notify(qtk_fixed_angle_wasr_t *w,
                                          qtk_wasr_notify_t notify,
                                          void *upval);

#ifdef __cplusplus
};
#endif
#endif
