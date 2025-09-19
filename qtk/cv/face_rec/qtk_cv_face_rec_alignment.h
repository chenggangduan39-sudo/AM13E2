#ifndef CCFEEDFA_06DB_8C3E_4312_1405D0D2B666
#define CCFEEDFA_06DB_8C3E_4312_1405D0D2B666

#include "qtk/cv/face_rec/qtk_cv_face_rec_alignment_cfg.h"

typedef struct qtk_cv_face_rec_alignment qtk_cv_face_rec_alignment_t;

struct qtk_cv_face_rec_alignment {
    qtk_cv_face_rec_alignment_cfg_t *cfg;
    uint8_t *result;
};

qtk_cv_face_rec_alignment_t *
qtk_cv_face_rec_alignment_new(qtk_cv_face_rec_alignment_cfg_t *cfg);
void qtk_cv_face_rec_alignment_delete(qtk_cv_face_rec_alignment_t *align);
uint8_t *qtk_cv_face_rec_alignment_feed(qtk_cv_face_rec_alignment_t *align,
                                        uint8_t *frame, int width, int height,
                                        float *key_pts);

#endif /* CCFEEDFA_06DB_8C3E_4312_1405D0D2B666 */
