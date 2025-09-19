#ifndef CD00878C_2BB4_0579_6F96_FA47CA42DCB9
#define CD00878C_2BB4_0579_6F96_FA47CA42DCB9

#include "qtk/cv/detection/qtk_cv_detection.h"
#include "qtk/cv/embedding/qtk_cv_embedding.h"
#include "qtk/cv/face_rec/qtk_cv_face_rec_alignment.h"
#include "qtk/cv/face_rec/qtk_cv_face_rec_embedding_cfg.h"
#include "qtk/cv/classify/qtk_cv_classify.h"
#include "qtk/cv/qtk_cv_bbox.h"

typedef struct qtk_cv_face_rec_embedding qtk_cv_face_rec_embedding_t;

typedef struct {
    float conf;
    qtk_cv_bbox_t box;
    float *embedding;
    int embedding_dim;
    int cls_id;
} qtk_cv_face_rec_embedding_result_t;

struct qtk_cv_face_rec_embedding {
    qtk_cv_face_rec_embedding_cfg_t *cfg;
    qtk_cv_detection_t *face_detection;
    qtk_cv_embedding_t *embedding;
    qtk_cv_face_rec_alignment_t *alignment;
    qtk_cv_classify_t *classify;
    wtk_heap_t *heap;
};

qtk_cv_face_rec_embedding_t *
qtk_cv_face_rec_embedding_new(qtk_cv_face_rec_embedding_cfg_t *cfg);
void qtk_cv_face_rec_embedding_delete(qtk_cv_face_rec_embedding_t *em);
int qtk_cv_face_rec_embedding_process(
    qtk_cv_face_rec_embedding_t *em, uint8_t *image,
    qtk_cv_face_rec_embedding_result_t **result);

#endif /* CD00878C_2BB4_0579_6F96_FA47CA42DCB9 */
