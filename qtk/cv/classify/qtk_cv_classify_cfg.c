
#include "qtk/cv/classify/qtk_cv_classify_cfg.h"

int qtk_cv_classify_cfg_init(qtk_cv_classify_cfg_t *cfg) {
    cfg->height = 0;
    cfg->width = 0;
    cfg->mean = NULL;
    cfg->std = NULL;
    cfg->labels = NULL;
    cfg->nlabels = 0;

    
    qtk_nnrt_cfg_init(&cfg->nnrt);
    return 0;
}

int qtk_cv_classify_cfg_clean(qtk_cv_classify_cfg_t *cfg) {
    qtk_nnrt_cfg_clean(&cfg->nnrt);
    return 0;
}

int qtk_cv_classify_cfg_update(qtk_cv_classify_cfg_t *cfg) {
    qtk_nnrt_cfg_update(&cfg->nnrt);
    return 0;
}

int qtk_cv_classify_cfg_update_local(qtk_cv_classify_cfg_t *cfg, wtk_local_cfg_t *lc) {
    wtk_array_t *a;
    wtk_local_cfg_t *sub;
    wtk_string_t *v;
    if ((a = wtk_local_cfg_find_float_array_s(lc, "mean"))) {
        cfg->mean = a->slot;
    }
    if ((a = wtk_local_cfg_find_float_array_s(lc, "std"))) {
        cfg->std = a->slot;
    } 
    if ((a = wtk_local_cfg_find_array_s(lc, "labels"))) {
        cfg->labels = (wtk_string_t **)a->slot;
        cfg->nlabels = a->nslot;
    } 
    sub = wtk_local_cfg_find_lc_s(lc, "nnrt");
    if (sub) {
        qtk_nnrt_cfg_update_local(&cfg->nnrt, sub);
    }
    wtk_local_cfg_update_cfg_i(lc, cfg, width, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, height, v);

    return 0;
}

int qtk_cv_classify_cfg_update2(qtk_cv_classify_cfg_t *cfg, wtk_source_loader_t *sl) {
    qtk_nnrt_cfg_update2(&cfg->nnrt, sl);
    return 0;
}
