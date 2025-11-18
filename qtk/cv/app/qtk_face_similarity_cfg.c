#include "qtk/cv/app/qtk_face_similarity_cfg.h"

int qtk_face_similarity_cfg_init(qtk_face_similarity_cfg_t *cfg) {
    qtk_cv_face_rec_embedding_cfg_init(&cfg->emb);
    return 0;
}

int qtk_face_similarity_cfg_clean(qtk_face_similarity_cfg_t *cfg) {
    qtk_cv_face_rec_embedding_cfg_clean(&cfg->emb);
    return 0;
}

int qtk_face_similarity_cfg_update(qtk_face_similarity_cfg_t *cfg) {
    qtk_cv_face_rec_embedding_cfg_update(&cfg->emb);
    return 0;
}

int qtk_face_similarity_cfg_update_local(qtk_face_similarity_cfg_t *cfg, wtk_local_cfg_t *lc) {
    wtk_local_cfg_t *sub;
    sub = wtk_local_cfg_find_lc_s(lc, "emb");
    if (sub) {
        qtk_cv_face_rec_embedding_cfg_update_local(&cfg->emb, sub);
    }
    return 0; 
}
int qtk_face_similarity_cfg_update2(qtk_face_similarity_cfg_t *cfg, wtk_source_loader_t *sl) {
    qtk_cv_face_rec_embedding_cfg_update2(&cfg->emb, sl);
    return 0;
}
