#ifndef __QBL_IMAGE_QBL_IMAGE_FEATURE_H__
#define __QBL_IMAGE_QBL_IMAGE_FEATURE_H__
#pragma once
#include "qtk/core/qtk_type.h"
#include "qtk/image/qtk_image.h"
#ifdef __cplusplus
extern "C" {
#endif

int qtk_image_feature_cn(uint8_t *img, qtk_image_desc_t *desc, float *dst);

#ifdef __cplusplus
};
#endif
#endif
