extern "C" {
#include "qtk/cv/face_rec/qtk_cv_face_rec_embedding.h"
}

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>

static void face_process_single_(qtk_cv_face_rec_embedding_t *em,
                                 qtk_cv_detection_result_t *face_info,
                                 qtk_cv_face_rec_embedding_result_t *result,
                                 uint8_t *image) {
    float *embedding;
    uint8_t *aligned_image;
    int embedding_dim, i;
    result->conf = face_info->conf;
    result->box = face_info->box;
    aligned_image = qtk_cv_face_rec_alignment_feed(
        em->alignment, image, em->cfg->face_detection.width,
        em->cfg->face_detection.height, face_info->keypoints);
    result->cls_id = qtk_cv_classify_feed(em->classify, aligned_image);
    embedding_dim =
        qtk_cv_embedding_process(em->embedding, aligned_image, &embedding);
    result->embedding =
        (float *)wtk_heap_malloc(em->heap, sizeof(float) * embedding_dim);
    memcpy(result->embedding, embedding, sizeof(float) * embedding_dim);
    result->embedding_dim = embedding_dim;
    if (em->cfg->use_flip) {
        float sum = 0;
        cv::Mat flipped_mat;
        cv::Mat aligned_mat(em->cfg->alignment.dst_height,
                            em->cfg->alignment.dst_width, CV_8UC3,
                            aligned_image);
        cv::flip(aligned_mat, flipped_mat, 1);
        if (flipped_mat.isContinuous()) {
            embedding_dim = qtk_cv_embedding_process(
                em->embedding, flipped_mat.ptr(), &embedding);
        } else {
            cv::Mat tmp = flipped_mat.clone();
            embedding_dim =
                qtk_cv_embedding_process(em->embedding, tmp.ptr(), &embedding);
        }
        for (i = 0; i < embedding_dim; i++) {
            result->embedding[i] += embedding[i];
            sum += result->embedding[i] * result->embedding[i];
        }
        sum = sqrt(sum);
        for (i = 0; i < embedding_dim; i++) {
            result->embedding[i] /= sum;
        }
    }
}

extern "C" {
qtk_cv_face_rec_embedding_t *
qtk_cv_face_rec_embedding_new(qtk_cv_face_rec_embedding_cfg_t *cfg) {
    qtk_cv_face_rec_embedding_t *em = (qtk_cv_face_rec_embedding_t *)wtk_malloc(
        sizeof(qtk_cv_face_rec_embedding_t));
    em->cfg = cfg;
    em->embedding = qtk_cv_embedding_new(&cfg->embedding);
    em->face_detection = qtk_cv_detection_new(&cfg->face_detection);
    em->alignment = qtk_cv_face_rec_alignment_new(&cfg->alignment);
    em->classify = qtk_cv_classify_new(&cfg->classify);
    em->heap = wtk_heap_new(4096);
    if (em->classify->H != cfg->alignment.dst_height || em->classify->W != cfg->alignment.dst_width) {
        qtk_cv_face_rec_embedding_delete(em);
        return NULL;
    }
    return em;
}

void qtk_cv_face_rec_embedding_delete(qtk_cv_face_rec_embedding_t *em) {
    qtk_cv_embedding_delete(em->embedding);
    qtk_cv_detection_delete(em->face_detection);
    qtk_cv_face_rec_alignment_delete(em->alignment);
    qtk_cv_classify_delete(em->classify);
    wtk_heap_delete(em->heap);
    wtk_free(em);
}

int qtk_cv_face_rec_embedding_process(
    qtk_cv_face_rec_embedding_t *em, uint8_t *image,
    qtk_cv_face_rec_embedding_result_t **result) {
    qtk_cv_detection_result_t *faces;
    int nfaces, i;
    nfaces = qtk_cv_detection_detect(em->face_detection, image, &faces);
    wtk_heap_reset(em->heap);
    if (nfaces > 0) {
        *result = (qtk_cv_face_rec_embedding_result_t *)wtk_heap_malloc(
            em->heap, sizeof(qtk_cv_face_rec_embedding_result_t) * nfaces);
        for (i = 0; i < nfaces; i++) {
            face_process_single_(em, faces + i, *result + i, image);
        }
    }
    return nfaces;
}
}
