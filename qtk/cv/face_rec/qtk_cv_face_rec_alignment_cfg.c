#include "qtk/cv/face_rec/qtk_cv_face_rec_alignment_cfg.h"

int qtk_cv_face_rec_alignment_cfg_init(qtk_cv_face_rec_alignment_cfg_t *cfg) {
    cfg->dst_width = 112;
    cfg->dst_height = 112;
    cfg->ref_ptr_scale = 1;
    return 0;
}

int qtk_cv_face_rec_alignment_cfg_clean(qtk_cv_face_rec_alignment_cfg_t *cfg) {
    return 0;
}

int qtk_cv_face_rec_alignment_cfg_update(qtk_cv_face_rec_alignment_cfg_t *cfg) {
    if (cfg->dst_width != cfg->dst_height) {
        wtk_debug("Not Impl for [%d/%d]\n", cfg->dst_height, cfg->dst_width);
        return -1;
    }
    return 0;
}

int qtk_cv_face_rec_alignment_cfg_update_local(
    qtk_cv_face_rec_alignment_cfg_t *cfg, wtk_local_cfg_t *lc) {
    wtk_string_t *v;
    wtk_local_cfg_update_cfg_i(lc, cfg, dst_width, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, dst_height, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, ref_ptr_scale, v);
    return 0;
}

int qtk_cv_face_rec_alignment_cfg_update2(qtk_cv_face_rec_alignment_cfg_t *cfg,
                                          wtk_source_loader_t *sl) {
    return 0;
}
