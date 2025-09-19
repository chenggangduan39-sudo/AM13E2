#ifndef __WTK_BFIO_QTK_FIXED_ANGLE_WASR_CFG_H__
#define __WTK_BFIO_QTK_FIXED_ANGLE_WASR_CFG_H__
#include "wtk/bfio/aec/wtk_aec_cfg.h"
#include "wtk/bfio/qform/wtk_qform2_cfg.h"
#include "wtk/bfio/qtk_wasr_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_fixed_angle_wasr_cfg qtk_fixed_angle_wasr_cfg_t;

struct qtk_fixed_angle_wasr_cfg {
    qtk_wasr_cfg_t wasr;
    wtk_qform2_cfg_t qform2;
    wtk_aec_cfg_t aec;
    float theta;
    float phi;

    unsigned use_aec : 1;
};

int qtk_fixed_angle_wasr_cfg_init(qtk_fixed_angle_wasr_cfg_t *cfg);
int qtk_fixed_angle_wasr_cfg_clean(qtk_fixed_angle_wasr_cfg_t *cfg);
int qtk_fixed_angle_wasr_cfg_update(qtk_fixed_angle_wasr_cfg_t *cfg);
int qtk_fixed_angle_wasr_cfg_update_local(qtk_fixed_angle_wasr_cfg_t *cfg,
                                          wtk_local_cfg_t *lc);

#ifdef __cplusplus
};
#endif
#endif
