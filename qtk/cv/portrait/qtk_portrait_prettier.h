#ifndef QBL_CV_PORTRAIT_QBL_PORTRAIT_PRETTIER_H
#define QBL_CV_PORTRAIT_QBL_PORTRAIT_PRETTIER_H
#pragma once
#include "qtk/image/qtk_image.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_portrait_prettier qtk_portrait_prettier_t;
typedef struct qtk_portrait_prettier_cfg qtk_portrait_prettier_cfg_t;

struct qtk_portrait_prettier {
    unsigned char midtones_lut[256];
    qtk_portrait_prettier_cfg_t *cfg;
};

struct qtk_portrait_prettier_cfg {
    float dermabrasion_accuracy;
    float dermabrasion_degree;
    float skin_whitening_degree;
    unsigned dermabrasion : 1;
    unsigned skin_whitening : 1;
    unsigned skin_detect_with_morph : 1;
};

int qtk_portrait_prettier_init(qtk_portrait_prettier_t *p,
                               qtk_portrait_prettier_cfg_t *cfg);
int qtk_portrait_prettier_process(qtk_portrait_prettier_t *p, void *img,
                                  qtk_image_desc_t *desc);
void qtk_portrait_prettier_clean(qtk_portrait_prettier_t *p);
void qtk_portrait_prettier_cfg_init(qtk_portrait_prettier_cfg_t *cfg);

#ifdef __cplusplus
};
#endif
#endif
