#ifndef __QBL_IMAGE_QBL_IMAGE_H__
#define __QBL_IMAGE_QBL_IMAGE_H__
#include "qtk/core/qtk_type.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    QBL_IMAGE_MJPEG,
    QBL_IMAGE_NV12,
    QBL_IMAGE_RGB24,
    QBL_IMAGE_BGR24,
    QBL_IMAGE_GRAY8,
} qtk_image_fmt_t;

typedef struct qtk_image_roi qtk_image_roi_t;
struct qtk_image_roi {
    int x;
    int y;
    int width;
    int height;
};

typedef struct qtk_image_roi_f qtk_image_roi_f_t;
struct qtk_image_roi_f {
    float x;
    float y;
    float width;
    float height;
};

typedef struct qtk_image_desc qtk_image_desc_t;
struct qtk_image_desc {
    qtk_image_fmt_t fmt;
    int height;
    int width;
    int channel;
};

typedef enum {
    QBL_IMAGE_BORDER_ZERO,
} qtk_image_border_t;

#define qtk_image_sub_u8(desc, roi, result, src)                               \
    qtk_image_sub(desc, roi, result, src, sizeof(uint8_t))

#define qtk_image_sub_u8_with_border(desc, roi, result, src, top, bottom,      \
                                     left, right, border)                      \
    qtk_image_sub_with_border(desc, roi, result, src, sizeof(uint8_t), top,    \
                              bottom, left, right, border)

int qtk_image_sub(qtk_image_desc_t *desc, qtk_image_roi_t *roi, void *result,
                  void *src, int sample_byte);
int qtk_image_patch(qtk_image_desc_t *desc, qtk_image_roi_t *roi, void *result,
                  void *src, int sample_byte);
uint8_t *qtk_image_load(qtk_image_desc_t *desc, const char *filename);
uint8_t *qtk_image_load_from_memory(qtk_image_desc_t *desc, const uint8_t *data,
                                    uint32_t len);
uint8_t *qtk_image_load_bgr(qtk_image_desc_t *desc, const char *filename);
int qtk_image_save_bmp(const char *filename, uint8_t *data,
                       qtk_image_desc_t *desc);
int qtk_image_save_png(const char *filename, const uint8_t *data,
                       qtk_image_desc_t *desc);
int qtk_image_sub_with_border(qtk_image_desc_t *desc, qtk_image_roi_t *roi,
                              void *result, void *src, int sample_byte, int top,
                              int bottom, int left, int right,
                              qtk_image_border_t border);
int qtk_image_sub_with_border_zero(qtk_image_desc_t *desc, qtk_image_roi_t *roi,
                                   void *result, void *src, int sample_byte,
                                   int top, int bottom, int left, int right);

void qtk_image_rot90(qtk_image_desc_t *desc, void *src, void *dst,
                     int sample_byte);
void qtk_image_rot180(qtk_image_desc_t *desc, void *src, void *dst,
                      int sample_byte);
void qtk_image_rot270(qtk_image_desc_t *desc, void *src, void *dst,
                      int sample_byte);

int qtk_image_resize(qtk_image_desc_t *desc, void *src, int out_h, int out_w,
                     void *dst);

//图像数据前处理
/*  imagedata:图像数据
    data:输出数据
    svd:均值
    std:方差
    len:图片大小 （width*height）
*/
void qtk_image_pretreatment(const uint8_t *imagedata, float *data, float *svd,
                            float *std, const int len);
#ifdef __cplusplus
};
#endif
#endif
