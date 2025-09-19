#include "qtk/cv/embedding/qtk_cv_embedding_cfg.h"

int qtk_cv_embedding_cfg_init(qtk_cv_embedding_cfg_t *cfg) {
    qtk_nnrt_cfg_init(&cfg->nnrt);
    cfg->width = 112;
    cfg->height = 112;
    return 0;
}

int qtk_cv_embedding_cfg_clean(qtk_cv_embedding_cfg_t *cfg) {
    qtk_nnrt_cfg_clean(&cfg->nnrt);
    return 0;
}

int qtk_cv_embedding_cfg_update(qtk_cv_embedding_cfg_t *cfg) {
    qtk_nnrt_cfg_update(&cfg->nnrt);
    return 0;
}

int qtk_cv_embedding_cfg_update_local(qtk_cv_embedding_cfg_t *cfg,
                                      wtk_local_cfg_t *main) {
    wtk_local_cfg_t *lc;
    wtk_string_t *v;
    lc = wtk_local_cfg_find_lc_s(main, "nnrt");
    if (lc) {
        qtk_nnrt_cfg_update_local(&cfg->nnrt, lc);
    }
    wtk_local_cfg_update_cfg_i(main, cfg, width, v);
    wtk_local_cfg_update_cfg_i(main, cfg, height, v);
    return 0;
}

int qtk_cv_embedding_cfg_update2(qtk_cv_embedding_cfg_t *cfg,
                                 wtk_source_loader_t *sl) {
    return qtk_nnrt_cfg_update2(&cfg->nnrt, sl);
}
