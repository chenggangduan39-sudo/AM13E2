
#ifndef __QTK_CV_STAND_SIT_CFG_H__
#define __QTK_CV_STAND_SIT_CFG_H__

#include "wtk/core/cfg/wtk_local_cfg.h"
#include "qtk/cv/detection/qtk_cv_detection.h"
#include "qtk/cv/classify/qtk_cv_classify.h"
typedef struct qtk_cv_stand_sit_cfg qtk_cv_stand_sit_cfg_t;
struct qtk_cv_stand_sit_cfg{
    qtk_cv_detection_cfg_t person_detection;
    qtk_cv_classify_cfg_t classify;
    unsigned use_phone_cls : 1;
    
};

int qtk_cv_stand_sit_cfg_init(qtk_cv_stand_sit_cfg_t *cfg);
int qtk_cv_stand_sit_cfg_clean(qtk_cv_stand_sit_cfg_t *cfg);
int qtk_cv_stand_sit_cfg_update(qtk_cv_stand_sit_cfg_t *cfg);
int qtk_cv_stand_sit_cfg_update2(qtk_cv_stand_sit_cfg_t *cfg,
                                          wtk_source_loader_t *sl);
int qtk_cv_stand_sit_cfg_update_local(
    qtk_cv_stand_sit_cfg_t *cfg, wtk_local_cfg_t *lc);

#endif