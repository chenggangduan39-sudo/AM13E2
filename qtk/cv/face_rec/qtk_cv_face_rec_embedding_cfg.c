#include "qtk/cv/face_rec/qtk_cv_face_rec_embedding_cfg.h"

int qtk_cv_face_rec_embedding_cfg_init(qtk_cv_face_rec_embedding_cfg_t *cfg) {
    qtk_cv_detection_cfg_init(&cfg->face_detection);
    qtk_cv_embedding_cfg_init(&cfg->embedding);
    qtk_cv_face_rec_alignment_cfg_init(&cfg->alignment);
    qtk_cv_classify_cfg_init(&cfg->classify);
    cfg->use_flip = 1;
    return 0;
}

int qtk_cv_face_rec_embedding_cfg_clean(qtk_cv_face_rec_embedding_cfg_t *cfg) {
    qtk_cv_detection_cfg_clean(&cfg->face_detection);
    qtk_cv_embedding_cfg_clean(&cfg->embedding);
    qtk_cv_face_rec_alignment_cfg_clean(&cfg->alignment);
    qtk_cv_classify_cfg_clean(&cfg->classify);
    return 0;
}

static int local_update_(qtk_cv_face_rec_embedding_cfg_t *cfg) {
    if (cfg->embedding.width != cfg->alignment.dst_width ||
        cfg->embedding.height != cfg->alignment.dst_height) {
        goto err;
    }
    return 0;
err:
    return -1;
}

int qtk_cv_face_rec_embedding_cfg_update(qtk_cv_face_rec_embedding_cfg_t *cfg) {
    qtk_cv_detection_cfg_update(&cfg->face_detection);
    qtk_cv_embedding_cfg_update(&cfg->embedding);
    qtk_cv_face_rec_alignment_cfg_update(&cfg->alignment);
    qtk_cv_classify_cfg_update(&cfg->classify);
    return local_update_(cfg);
}

int qtk_cv_face_rec_embedding_cfg_update_local(
    qtk_cv_face_rec_embedding_cfg_t *cfg, wtk_local_cfg_t *lc) {
    wtk_string_t *v;
    wtk_local_cfg_t *sub_lc;
    wtk_local_cfg_update_cfg_b(lc, cfg, use_flip, v);
    sub_lc = wtk_local_cfg_find_lc_s(lc, "face_detection");
    if (sub_lc) {
        qtk_cv_detection_cfg_update_local(&cfg->face_detection, sub_lc);
    }
    sub_lc = wtk_local_cfg_find_lc_s(lc, "embedding");
    if (sub_lc) {
        qtk_cv_embedding_cfg_update_local(&cfg->embedding, sub_lc);
    }
    sub_lc = wtk_local_cfg_find_lc_s(lc, "alignment");
    if (sub_lc) {
        qtk_cv_face_rec_alignment_cfg_update_local(&cfg->alignment, sub_lc);
    }
    sub_lc = wtk_local_cfg_find_lc_s(lc, "classify");
    if (sub_lc) {
        qtk_cv_classify_cfg_update_local(&cfg->classify, sub_lc);
    }
    return 0;
}

int qtk_cv_face_rec_embedding_cfg_update2(qtk_cv_face_rec_embedding_cfg_t *cfg,
                                          wtk_source_loader_t *sl) {
    qtk_cv_detection_cfg_update2(&cfg->face_detection, sl);
    qtk_cv_embedding_cfg_update2(&cfg->embedding, sl);
    qtk_cv_face_rec_alignment_cfg_update2(&cfg->alignment, sl);
    qtk_cv_classify_cfg_update2(&cfg->classify, sl);
    return local_update_(cfg);
}
