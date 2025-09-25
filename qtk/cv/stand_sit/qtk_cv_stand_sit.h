
#ifndef __QTK_CV_STAND_SIT_H__
#define __QTK_CV_STAND_SIT_H__

extern "C"{
#include "qtk_cv_stand_sit_cfg.h"
}

typedef struct{
    qtk_cv_stand_sit_cfg_t *cfg;
    qtk_cv_detection_t *person_detection;
    qtk_cv_classify_t * classify;
    wtk_string_t *result;
    qtk_cv_detection_result_t *person;

}qtk_cv_stand_sit_t;

qtk_cv_stand_sit_t *qtk_cv_stand_sit_new(qtk_cv_stand_sit_cfg_t *cfg);
void qtk_cv_stand_sit_delete(qtk_cv_stand_sit_t *stand_sit);
int qtk_cv_stand_sit_process(qtk_cv_stand_sit_t *stand_sit, uint8_t * image, int height, int width);

#endif