#include "qtk/cv/app/qtk_face_classify_cfg.h"

int qtk_face_classify_cfg_init(qtk_face_classify_cfg_t *cfg) {
    qtk_cv_detection_cfg_init(&cfg->face_detection);
    qtk_cv_classify_cfg_init(&cfg->classify);
    return 0;
}

int qtk_face_classify_cfg_clean(qtk_face_classify_cfg_t *cfg) {
    qtk_cv_detection_cfg_clean(&cfg->face_detection);
    qtk_cv_classify_cfg_clean(&cfg->classify);
    return 0;
}

int qtk_face_classify_cfg_update(qtk_face_classify_cfg_t *cfg) {
    qtk_cv_detection_cfg_update(&cfg->face_detection);
    qtk_cv_classify_cfg_update(&cfg->classify);
    return 0;
}

int qtk_face_classify_cfg_update_local(qtk_face_classify_cfg_t *cfg, wtk_local_cfg_t *lc) {
    wtk_local_cfg_t *sub;
    sub = wtk_local_cfg_find_lc_s(lc, "face_detection");
    if (sub) {
        qtk_cv_detection_cfg_update_local(&cfg->face_detection, sub);
    }
    sub = wtk_local_cfg_find_lc_s(lc, "classify");
    if (sub) {
        qtk_cv_classify_cfg_update_local(&cfg->classify, sub);
    }
    return 0;
}

int qtk_face_classify_cfg_update2(qtk_face_classify_cfg_t *cfg, wtk_source_loader_t *sl) {
    qtk_cv_detection_cfg_update2(&cfg->face_detection, sl);
    qtk_cv_classify_cfg_update2(&cfg->classify, sl);
    return 0;
}
