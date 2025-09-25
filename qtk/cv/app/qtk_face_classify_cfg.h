#ifndef G_D49738B80E1F49BE936A4007F20F10FA
#define G_D49738B80E1F49BE936A4007F20F10FA
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "qtk/cv/classify/qtk_cv_classify_cfg.h"
#include "qtk/cv/detection/qtk_cv_detection_cfg.h"

typedef struct qtk_face_classify_cfg qtk_face_classify_cfg_t;

struct qtk_face_classify_cfg {
    qtk_cv_classify_cfg_t classify;
    qtk_cv_detection_cfg_t face_detection;
};

int qtk_face_classify_cfg_init(qtk_face_classify_cfg_t *cfg);
int qtk_face_classify_cfg_clean(qtk_face_classify_cfg_t *cfg);
int qtk_face_classify_cfg_update(qtk_face_classify_cfg_t *cfg);
int qtk_face_classify_cfg_update_local(qtk_face_classify_cfg_t *cfg, wtk_local_cfg_t *lc);
int qtk_face_classify_cfg_update2(qtk_face_classify_cfg_t *cfg, wtk_source_loader_t *sl);

#endif
