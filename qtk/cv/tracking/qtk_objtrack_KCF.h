#ifndef __QBL_OBJECT_TRACKING_QBL_OBJTRACK_KCF_H__
#define __QBL_OBJECT_TRACKING_QBL_OBJTRACK_KCF_H__
#pragma once
#include "qtk/cv/tracking/qtk_objtrack.h"
#include "qtk/image/qtk_image.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_objtrack_KCF_cfg qtk_objtrack_KCF_cfg_t;
typedef void *qtk_objtrack_KCF_t;

struct qtk_objtrack_KCF_cfg {
    float detect_thresh;
    float sigma;
    float lambda;
    float interp_fact;
    float output_sigma_fact;
    float expand_fact;
    float retracking_moving_fact;
    float pca_learning_rate;
    int max_patch_size;
    int compress_size;
    int post_hist;
    int motion_vector_hist;

    qtk_objtrack_mode_t desc_pca;
    qtk_objtrack_mode_t desc_npca;

    unsigned resize : 1;
    unsigned split_coeff : 1;
    unsigned wrap_kernel : 1;
    unsigned compress_feature : 1;
};

qtk_objtrack_KCF_t qtk_objtrack_KCF_new(qtk_objtrack_KCF_cfg_t *cfg,
                                        uint8_t *image,
                                        qtk_image_desc_t *img_desc,
                                        qtk_image_roi_t *roi);

void qtk_objtrack_KCF_delete(qtk_objtrack_KCF_t kcf);
int qtk_objtrack_KCF_update(qtk_objtrack_KCF_t kcf, uint8_t *image,
                            qtk_image_desc_t *desc, qtk_image_roi_t *result,
                            float *conf);
void qtk_objtrack_KCF_set_roi(qtk_objtrack_KCF_t kcf, qtk_image_roi_t *roi);
int qtk_objtrack_KCF_retracking(qtk_objtrack_KCF_t kcf, uint8_t *image,
                                qtk_image_desc_t *desc, qtk_image_roi_t *result,
                                float *conf);

void qtk_objtrack_KCF_cfg_init(qtk_objtrack_KCF_cfg_t *cfg);

#ifdef __cplusplus
};
#endif
#endif
