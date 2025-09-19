#include "qtk/cv/app/qtk_face_similarity.h"
#include "qtk/sci/qtk_distance.h"

qtk_face_similarity_t *qtk_face_similarity_new(qtk_face_similarity_cfg_t *cfg) {
    qtk_face_similarity_t *sim = wtk_malloc(sizeof(qtk_face_similarity_t));
    sim->cfg = cfg;
    sim->emb = qtk_cv_face_rec_embedding_new(&cfg->emb);
    sim->embeding_tmp = NULL;
    return sim;
}

void qtk_face_similarity_delete(qtk_face_similarity_t *sim) {
    qtk_cv_face_rec_embedding_delete(sim->emb);
    if (sim->embeding_tmp) {
        wtk_free(sim->embeding_tmp);
    }
    wtk_free(sim);
}

int qtk_face_similarity_get(qtk_face_similarity_t *sim, uint8_t *img1, uint8_t *img2, float *similarity) {
    int nemb;
    qtk_cv_face_rec_embedding_result_t *emb_res;
    nemb = qtk_cv_face_rec_embedding_process(sim->emb, img1, &emb_res);
    if (nemb != 1) {
        return -1;
    }
    if (!sim->embeding_tmp) {
        sim->embeding_tmp = wtk_malloc(sizeof(float) * emb_res[0].embedding_dim);
    }
    memcpy(sim->embeding_tmp, emb_res[0].embedding, sizeof(float) * emb_res[0].embedding_dim);
    nemb = qtk_cv_face_rec_embedding_process(sim->emb, img2, &emb_res);
    if (nemb != 1) {
        return -1;
    }
    *similarity = qtk_distance_euclidean(sim->embeding_tmp, emb_res[0].embedding, emb_res[0].embedding_dim);
    return 0;
}
