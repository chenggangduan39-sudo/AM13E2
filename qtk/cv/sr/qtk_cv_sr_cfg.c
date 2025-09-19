#include "qtk/cv/sr/qtk_cv_sr_cfg.h"

int qtk_cv_sr_cfg_init(qtk_cv_sr_cfg_t *cfg) {
    qtk_nnrt_cfg_init(&cfg->nnrt);
    return 0;
}

int qtk_cv_sr_cfg_clean(qtk_cv_sr_cfg_t *cfg) {
    qtk_nnrt_cfg_clean(&cfg->nnrt);
    return 0;
}

int qtk_cv_sr_cfg_update(qtk_cv_sr_cfg_t *cfg) {
    qtk_nnrt_cfg_update(&cfg->nnrt);
    return 0;
}

int qtk_cv_sr_cfg_update_local(qtk_cv_sr_cfg_t *cfg, wtk_local_cfg_t *lc) {
    wtk_local_cfg_t *sub;
    sub = wtk_local_cfg_find_lc_s(lc, "nnrt");
    if (sub) {
        qtk_nnrt_cfg_update_local(&cfg->nnrt, sub);
    }
    return 0;
}
int qtk_cv_sr_cfg_update2(qtk_cv_sr_cfg_t *cfg, wtk_source_loader_t *sl) {
    return qtk_nnrt_cfg_update2(&cfg->nnrt, sl);
}
