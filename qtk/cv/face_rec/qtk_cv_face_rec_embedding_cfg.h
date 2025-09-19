#ifndef BB5B4678_E568_8E36_8D46_B36085966ADA
#define BB5B4678_E568_8E36_8D46_B36085966ADA

#include "qtk/cv/detection/qtk_cv_detection_cfg.h"
#include "qtk/cv/embedding/qtk_cv_embedding_cfg.h"
#include "qtk/cv/face_rec/qtk_cv_face_rec_alignment_cfg.h"
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "qtk/cv/classify/qtk_cv_classify_cfg.h"

typedef struct qtk_cv_face_rec_embedding_cfg qtk_cv_face_rec_embedding_cfg_t;

struct qtk_cv_face_rec_embedding_cfg {
    qtk_cv_detection_cfg_t face_detection;
    qtk_cv_embedding_cfg_t embedding;
    qtk_cv_face_rec_alignment_cfg_t alignment;
    qtk_cv_classify_cfg_t classify;
    unsigned use_flip : 1;
};

int qtk_cv_face_rec_embedding_cfg_init(qtk_cv_face_rec_embedding_cfg_t *cfg);
int qtk_cv_face_rec_embedding_cfg_clean(qtk_cv_face_rec_embedding_cfg_t *cfg);
int qtk_cv_face_rec_embedding_cfg_update(qtk_cv_face_rec_embedding_cfg_t *cfg);
int qtk_cv_face_rec_embedding_cfg_update2(qtk_cv_face_rec_embedding_cfg_t *cfg,
                                          wtk_source_loader_t *sl);
int qtk_cv_face_rec_embedding_cfg_update_local(
    qtk_cv_face_rec_embedding_cfg_t *cfg, wtk_local_cfg_t *lc);

#endif /* BB5B4678_E568_8E36_8D46_B36085966ADA */
