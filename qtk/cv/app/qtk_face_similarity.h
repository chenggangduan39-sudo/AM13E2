#ifndef G_381EDC797B1F4C518FD1B6158F03639C
#define G_381EDC797B1F4C518FD1B6158F03639C

#include "qtk/cv/app/qtk_face_similarity_cfg.h"
#include "qtk/cv/face_rec/qtk_cv_face_rec_embedding.h"

typedef struct qtk_face_similarity qtk_face_similarity_t;

struct qtk_face_similarity {
    qtk_face_similarity_cfg_t *cfg;
    qtk_cv_face_rec_embedding_t *emb;
    float *embeding_tmp;
};

qtk_face_similarity_t *qtk_face_similarity_new(qtk_face_similarity_cfg_t *cfg);
void qtk_face_similarity_delete(qtk_face_similarity_t *sim);
int qtk_face_similarity_get(qtk_face_similarity_t *sim, uint8_t *img1, uint8_t *img2, float *similarity);

#endif
