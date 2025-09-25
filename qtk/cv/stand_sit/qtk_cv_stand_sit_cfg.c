#include "qtk/cv/stand_sit/qtk_cv_stand_sit_cfg.h"

int qtk_cv_stand_sit_cfg_init(qtk_cv_stand_sit_cfg_t *cfg){
    qtk_cv_detection_cfg_init(&cfg->person_detection);
    qtk_cv_classify_cfg_init(&cfg->classify);
    cfg->use_phone_cls = 0;
    return 0;
}

int qtk_cv_stand_sit_cfg_clean(qtk_cv_stand_sit_cfg_t *cfg){
    qtk_cv_detection_cfg_clean(&cfg->person_detection);
    qtk_cv_classify_cfg_clean(&cfg->classify);
    return 0;
}

int qtk_cv_stand_sit_cfg_update(qtk_cv_stand_sit_cfg_t *cfg){
    qtk_cv_detection_cfg_update(&cfg->person_detection);
    qtk_cv_classify_cfg_update(&cfg->classify);
    return 0;
}

int qtk_cv_stand_sit_cfg_update2(qtk_cv_stand_sit_cfg_t *cfg, wtk_source_loader_t *sl){
    qtk_cv_detection_cfg_update2(&cfg->person_detection, sl);
    qtk_cv_classify_cfg_update2(&cfg->classify, sl);
    return 0;

}

int qtk_cv_stand_sit_cfg_update_local(qtk_cv_stand_sit_cfg_t *cfg, wtk_local_cfg_t *lc){
    wtk_local_cfg_t *sub_lc; 
    wtk_string_t *v;
    sub_lc = wtk_local_cfg_find_lc_s(lc, "person_detection");
    if (sub_lc) {
        qtk_cv_detection_cfg_update_local(&cfg->person_detection, sub_lc);
    }
    sub_lc = wtk_local_cfg_find_lc_s(lc, "classify");
    if (sub_lc) {
        qtk_cv_classify_cfg_update_local(&cfg->classify, sub_lc);
    }
    wtk_local_cfg_update_cfg_i(lc, cfg, use_phone_cls, v);
    return 0;
}