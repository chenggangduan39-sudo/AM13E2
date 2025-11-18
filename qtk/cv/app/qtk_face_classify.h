#ifndef G_3C0819AF3500467D80894BEB9E9E04E3
#define G_3C0819AF3500467D80894BEB9E9E04E3

#include "qtk/cv/app/qtk_face_classify_cfg.h"
#include "qtk/cv/classify/qtk_cv_classify.h"
#include "qtk/cv/detection/qtk_cv_detection.h"

typedef struct qtk_face_classify qtk_face_classify_t;
typedef struct {
    int labels_id;
    qtk_cv_detection_result_t *det;
} qtk_face_classify_result_t;

struct qtk_face_classify {
    qtk_face_classify_cfg_t *cfg;
    qtk_cv_detection_t *face_detection;
    qtk_cv_classify_t *classify;
    wtk_heap_t *heap;
    uint8_t *cls_img;
};

qtk_face_classify_t *qtk_face_classify_new(qtk_face_classify_cfg_t *cfg);
void qtk_face_classify_delete(qtk_face_classify_t *cls);
int qtk_face_classify_feed(qtk_face_classify_t *cls, uint8_t *img, qtk_face_classify_result_t **res);
wtk_string_t *qtk_face_classify_get_label(qtk_face_classify_t *cls, int id);

#endif
