
#include "qtk/cv/face_rec/qtk_cv_face_rec_alignment.h"
#include "qtk/cv/geometric/qtk_geometric_transform.h"

// for 112 x 112
static float default_ref_pts[10] = {
    38.29459953, 51.69630051, 73.53179932, 51.50139999, 56.02519989,
    71.73660278, 41.54930115, 92.3655014,  70.72990036, 92.20410156};


// C API

qtk_cv_face_rec_alignment_t *
qtk_cv_face_rec_alignment_new(qtk_cv_face_rec_alignment_cfg_t *cfg) {
    qtk_cv_face_rec_alignment_t *align;

    align = (qtk_cv_face_rec_alignment_t *)wtk_malloc(
        sizeof(qtk_cv_face_rec_alignment_t));
    align->cfg = cfg;
    align->result = (uint8_t *)wtk_malloc(sizeof(uint8_t) * cfg->dst_width *
                                          cfg->dst_height * 3);
    return align;
}

void qtk_cv_face_rec_alignment_delete(qtk_cv_face_rec_alignment_t *align) {
    wtk_free(align->result);
    wtk_free(align);
}

uint8_t *qtk_cv_face_rec_alignment_feed(qtk_cv_face_rec_alignment_t *align,
                                        uint8_t *frame, int width, int height,
                                        float *key_pts) {
    int i;
    float scale = align->cfg->ref_ptr_scale;
    float ref_pts_data[10];
    float result[3 * 3] = {0.0};
    for (i = 0; i < 10; i++) {
        ref_pts_data[i] = default_ref_pts[i] * scale;
    }

    qtk_geometric_similarity_transform(key_pts, ref_pts_data, 5, 2, result);
    qtk_geometric_apply_affine_transform(frame, align->result, height,
                                    width, align->cfg->dst_height, align->cfg->dst_width,
                                    3, result);
    for (i = 0; i < 5; i++) {
        float x = key_pts[i * 2 + 0];
        float y = key_pts[i * 2 + 1];
        key_pts[i * 2 + 0] = result[0] * x + result[1] * y + result[2];

        key_pts[i * 2 + 1] = result[3/*3 * 1 + 0*/] * x + result[4/*3 * 1 + 1*/] * y + result[5/*3 * 1 + 2*/];
    }

    /* MAT(R, C, M) => MAT(i, j, k) = ptr[i *(C *M)+ j * M + k] a[2][4][5]*/
    return align->result;
}

