
#ifndef DFCA76AE_1697_CC3D_035C_F4D06C50F497
#define DFCA76AE_1697_CC3D_035C_F4D06C50F497

#include "qtk/nnrt/qtk_nnrt_cfg.h"
#include "wtk/core/cfg/wtk_local_cfg.h"

typedef struct qtk_cv_detection_cfg qtk_cv_detection_cfg_t;

struct qtk_cv_detection_cfg {
    qtk_nnrt_cfg_t nnrt;
    float *mean;
    float *std;
    float conf_threshold;
    int nstride;
    int *stride;
    int nanchor;
    int width;
    int height;
    float thresh;
    float nms_thresh;
    unsigned stand_sit_phone;
};

int qtk_cv_detection_cfg_init(qtk_cv_detection_cfg_t *cfg);
int qtk_cv_detection_cfg_update(qtk_cv_detection_cfg_t *cfg);
int qtk_cv_detection_cfg_clean(qtk_cv_detection_cfg_t *cfg);
int qtk_cv_detection_cfg_update_local(qtk_cv_detection_cfg_t *cfg,
                                      wtk_local_cfg_t *lc);
int qtk_cv_detection_cfg_update2(qtk_cv_detection_cfg_t *cfg,
                                 wtk_source_loader_t *sl);

#endif /* DFCA76AE_1697_CC3D_035C_F4D06C50F497 */
