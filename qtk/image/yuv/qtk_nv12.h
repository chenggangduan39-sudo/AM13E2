#ifndef __QBL_IMAGE_YUV_QBL_NV12_H__
#define __QBL_IMAGE_YUV_QBL_NV12_H__
#include "qtk/core/qtk_type.h"
#include "qtk/image/qtk_image.h"
#ifdef __cplusplus
extern "C" {
#endif

int qtk_nv12_crop(const uint8_t *src, int src_w, int src_h, uint8_t *dst,
                  qtk_image_roi_t *roi);
int qtk_nv12_draw_rect(uint8_t *src, int src_w, int src_h, qtk_image_roi_t *roi,
                       int x_width, int y_width, uint8_t Y, uint8_t U,
                       uint8_t V);
int qtk_nv12_fill_rect(uint8_t *src, int src_w, int src_h, qtk_image_roi_t *roi,
                       uint8_t *roi_imag);
int qtk_nv12_resize_bilinear(uint8_t *src, int src_w, int src_h, int dst_w,
                             int dst_h, uint8_t *dst);
void qtk_nv12_to_bgr24(const uint8_t *nv12, uint8_t *bgr, int w, int h);
void qtk_nv12_to_rgb24(const uint8_t *nv12, uint8_t *rgb, int w, int h);

#ifdef __cplusplus
};
#endif
#endif
