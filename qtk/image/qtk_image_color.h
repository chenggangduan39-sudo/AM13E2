#ifndef __QBL_IMAGE_QBL_IMAGE_COLOR_H__
#define __QBL_IMAGE_QBL_IMAGE_COLOR_H__
#pragma once
#include "qtk/core/qtk_type.h"
#include "qtk/image/qtk_image.h"
#ifdef __cplusplus
extern "C" {
#endif

int qtk_image_color_bgr2gray_u8(uint8_t *bgr, uint8_t *gray,
                                qtk_image_desc_t *desc);
int qtk_image_color_rgb2gray_u8(uint8_t *rgb, uint8_t *gray,
                                qtk_image_desc_t *desc);

#ifdef __cplusplus
};
#endif
#endif
