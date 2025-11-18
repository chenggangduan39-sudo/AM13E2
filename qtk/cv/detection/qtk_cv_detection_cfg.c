
#include "qtk/cv/detection/qtk_cv_detection_cfg.h"

int qtk_cv_detection_cfg_init(qtk_cv_detection_cfg_t *cfg) {
    qtk_nnrt_cfg_init(&cfg->nnrt);
    cfg->mean = NULL;
    cfg->std = NULL;
    cfg->width = 0;
    cfg->height = 0;
    cfg->conf_threshold = 0.0;
    cfg->nstride = 0;
    cfg->stride = NULL;
    cfg->nanchor = 2;
    cfg->thresh = 0.5;
    cfg->nms_thresh = 0.5;
    cfg->stand_sit_phone = 0;
    return 0;
}

int qtk_cv_detection_cfg_clean(qtk_cv_detection_cfg_t *cfg) {
    qtk_nnrt_cfg_clean(&cfg->nnrt);
    return 0;
}

static int local_update_(qtk_cv_detection_cfg_t *cfg) {
    return ((cfg->std && cfg->std) || cfg->nnrt.use_ss_ipu) && cfg->stride ? 0 : -1;
}

int qtk_cv_detection_cfg_update(qtk_cv_detection_cfg_t *cfg) {
    qtk_nnrt_cfg_update(&cfg->nnrt);
    return local_update_(cfg);
}

int qtk_cv_detection_cfg_update_local(qtk_cv_detection_cfg_t *cfg,
                                      wtk_local_cfg_t *main) {
    wtk_local_cfg_t *lc;
    wtk_array_t *a;
    wtk_string_t *v;
    lc = wtk_local_cfg_find_lc_s(main, "nnrt");
    if (lc) {
        qtk_nnrt_cfg_update_local(&cfg->nnrt, lc);
    }
    if ((a = wtk_local_cfg_find_float_array_s(main, "mean"))) {
        cfg->mean = a->slot;
    }
    if ((a = wtk_local_cfg_find_float_array_s(main, "std"))) {
        cfg->std = a->slot;
    }
    if ((a = wtk_local_cfg_find_int_array_s(main, "stride"))) {
        cfg->stride = a->slot;
        cfg->nstride = a->nslot;
    }
    wtk_local_cfg_update_cfg_i(main, cfg, width, v);
    wtk_local_cfg_update_cfg_i(main, cfg, height, v);
    wtk_local_cfg_update_cfg_f(main, cfg, thresh, v);
    wtk_local_cfg_update_cfg_f(main, cfg, nms_thresh, v);
    wtk_local_cfg_update_cfg_f(main, cfg, conf_threshold, v);
    wtk_local_cfg_update_cfg_i(main, cfg, stand_sit_phone, v);
    return 0;
}

int qtk_cv_detection_cfg_update2(qtk_cv_detection_cfg_t *cfg,
                                 wtk_source_loader_t *sl) {
    qtk_nnrt_cfg_update2(&cfg->nnrt, sl);
    return local_update_(cfg);
}
