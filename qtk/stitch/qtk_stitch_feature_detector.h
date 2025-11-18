#ifndef __QTK_STITCH_FEATURE_DETECTOR_H__
#define __QTK_STITCH_FEATURE_DETECTOR_H__

#include "qtk_stitch_def.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_stitch_feature_detector qtk_stitch_feature_detector_t;

qtk_stitch_feature_detector_t* qtk_stitch_feature_detector_new(int type, int nfeatures);
void* qtk_stitch_feature_detector_detect(qtk_stitch_feature_detector_t *detector,void *image);
void qtk_stitch_feature_features_delete(void *features);
void qtk_stitch_feature_detector_delete(qtk_stitch_feature_detector_t* detector);

#ifdef __cplusplus
}
#endif

#endif