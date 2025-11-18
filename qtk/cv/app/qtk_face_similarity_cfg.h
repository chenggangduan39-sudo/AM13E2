#ifndef G_0A0DDAB3C81F47A0B3E3AAECA2244DA0
#define G_0A0DDAB3C81F47A0B3E3AAECA2244DA0

#include "wtk/core/cfg/wtk_local_cfg.h"
#include "qtk/cv/face_rec/qtk_cv_face_rec_embedding_cfg.h"

typedef struct qtk_face_similarity_cfg qtk_face_similarity_cfg_t;

struct qtk_face_similarity_cfg {
    qtk_cv_face_rec_embedding_cfg_t emb;
};

int qtk_face_similarity_cfg_init(qtk_face_similarity_cfg_t *cfg);
int qtk_face_similarity_cfg_clean(qtk_face_similarity_cfg_t *cfg);
int qtk_face_similarity_cfg_update(qtk_face_similarity_cfg_t *cfg);
int qtk_face_similarity_cfg_update_local(qtk_face_similarity_cfg_t *cfg, wtk_local_cfg_t *lc);
int qtk_face_similarity_cfg_update2(qtk_face_similarity_cfg_t *cfg, wtk_source_loader_t *sl);

#endif
