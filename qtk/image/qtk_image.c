#include "qtk/image/qtk_image.h"
#include "wtk/core/wtk_alloc.h"

#define STBI_MALLOC wtk_malloc
#define STBI_REALLOC realloc
#define STBI_FREE wtk_free
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

#ifdef QTK_USE_RGA
#include "RgaUtils.h"
#include "im2d.h"
#include "rga.h"
#endif

int qtk_image_sub(qtk_image_desc_t *desc, qtk_image_roi_t *roi, void *result,
                  void *src, int sample_byte) {
    if (!desc || !roi || !result || !src) {
        wtk_debug("input pointer is null, desc:%p, roi:%p, result:%p, src:%p\n", desc, roi, result, src);
        return -1; // 检查指针是否为 NULL
    }

    // if (roi->x + roi->width > desc->width ||
    //     roi->y + roi->height > desc->height) {
    //     return -1;
    // }
    if (roi->x < 0 || roi->y < 0 || 
        roi->x + roi->width > desc->width || 
        roi->y + roi->height > desc->height) {
        wtk_debug("roi out of range\n x:%d y:%d w:%d h:%d\n", roi->x, roi->y, roi->width, roi->height);
        return -1; 
    }
    int bytes_per_pixel = sample_byte * desc->channel;
    src = cast(char *, src) + bytes_per_pixel * (roi->y * desc->width + roi->x);
    for (int i = 0; i < roi->height; i++) {
        memcpy(result, src, bytes_per_pixel * roi->width);
        src = cast(char *, src) + bytes_per_pixel * desc->width;
        result = cast(char *, result) + bytes_per_pixel * roi->width;
    }
    return 0;
}

int qtk_image_patch(qtk_image_desc_t *desc, qtk_image_roi_t *roi, void *result,
                  void *src, int sample_byte) {
    if (!desc || !roi || !result || !src) {
        wtk_debug("input pointer is null, desc:%p, roi:%p, result:%p, src:%p\n", desc, roi, result, src);
        return -1; // 检查指针是否为 NULL
    }

    // if (roi->x + roi->width > desc->width ||
    //     roi->y + roi->height > desc->height) {
    //     return -1;
    // }
    if (roi->x < 0 || roi->y < 0 || 
        roi->x + roi->width > desc->width || 
        roi->y + roi->height > desc->height) {
        wtk_debug("roi out of range\n x:%d y:%d w:%d h:%d\n", roi->x, roi->y, roi->width, roi->height);
        return -1; 
    }
    int bytes_per_pixel = sample_byte * desc->channel;
    result = cast(char *, result) + bytes_per_pixel * (roi->y * desc->width + roi->x);
    for (int i = 0; i < roi->height; i++) {
        memcpy(result, src, bytes_per_pixel * roi->width);
        src = cast(char *, src) + bytes_per_pixel * roi->width;
        result = cast(char *, result) + bytes_per_pixel * desc->width;
    }
    return 0;
}

void qtk_image_rot270(qtk_image_desc_t *desc, void *src_v, void *dst_v,
                      int sample_byte) {
    char *dst = dst_v;
    char *src = src_v;
    int channel_byte = desc->channel * sample_byte;
    for (int i = 0; i < desc->width; i++) {
        for (int j = 0; j < desc->height; j++, dst += channel_byte) {
            memcpy(dst,
                   src +
                       (j * desc->width + (desc->width - i - 1)) * channel_byte,
                   channel_byte);
        }
    }
}

void qtk_image_rot180(qtk_image_desc_t *desc, void *src_v, void *dst_v,
                      int sample_byte) {
    int channel_byte = desc->channel * sample_byte;
    char *dst = dst_v;
    char *src = src_v;
    for (int i = 0; i < desc->height; i++) {
        for (int j = 0; j < desc->width; j++, dst += channel_byte) {
            memcpy(dst,
                   src + ((desc->height - i - 1) * desc->width +
                          (desc->width - j - 1)) *
                             channel_byte,
                   channel_byte);
        }
    }
}

void qtk_image_rot90(qtk_image_desc_t *desc, void *src_v, void *dst_v,
                     int sample_byte) {
    int channel_byte = desc->channel * sample_byte;
    char *dst = dst_v;
    char *src = src_v;
    for (int i = 0; i < desc->width; i++) {
        for (int j = 0; j < desc->height; j++, dst += channel_byte) {
            memcpy(dst,
                   src + ((desc->height - j - 1) * desc->width + i) *
                             channel_byte,
                   channel_byte);
        }
    }
}

int qtk_image_sub_with_border_zero(qtk_image_desc_t *desc, qtk_image_roi_t *roi,
                                   void *result, void *src, int sample_byte,
                                   int top, int bottom, int left, int right) {
    int bytes_per_pixel;
    bytes_per_pixel = sample_byte * desc->channel;

    memset(result, 0,
           (roi->width + left + right) * (roi->height + top + bottom) *
               bytes_per_pixel);
    result =
        cast(char *, result) + bytes_per_pixel * (desc->width * top + left);
    src = cast(char *, src) + bytes_per_pixel * (roi->y * desc->width + roi->x);

    for (int i = 0; i < roi->height; i++) {
        memcpy(result, src, bytes_per_pixel * roi->width);
        src = cast(char *, src) + bytes_per_pixel * desc->width;
        result = cast(char *, result) +
                 bytes_per_pixel * (roi->width + left + right);
    }

    return 0;
}

int qtk_image_sub_with_border(qtk_image_desc_t *desc, qtk_image_roi_t *roi,
                              void *result, void *src, int sample_byte, int top,
                              int bottom, int left, int right,
                              qtk_image_border_t border) {
    switch (border) {
    case QBL_IMAGE_BORDER_ZERO:
        return qtk_image_sub_with_border_zero(
            desc, roi, result, src, sample_byte, top, bottom, left, right);
    }
    return -1;
}

uint8_t *qtk_image_load(qtk_image_desc_t *desc, const char *filename) {
    uint8_t *data =
        stbi_load(filename, &desc->width, &desc->height, &desc->channel, 0);
    desc->fmt = QBL_IMAGE_RGB24;
    return data;
}

uint8_t *qtk_image_load_from_memory(qtk_image_desc_t *desc, const uint8_t *data,
                                    uint32_t len) {
    uint8_t *img = stbi_load_from_memory(data, len, &desc->width, &desc->height,
                                         &desc->channel, 0);
    return img;
}

uint8_t *qtk_image_load_bgr(qtk_image_desc_t *desc, const char *filename) {
    int npixel;
    uint8_t *data_ptr, *cur_pixel;
    uint8_t *data = qtk_image_load(desc, filename);
    if (desc->channel < 3) {
        wtk_free(data);
        return NULL;
    }
    data_ptr = data;
    cur_pixel = data;
    npixel = desc->width * desc->height;
    for (int i = 0; i < npixel; i++) {
        uint8_t tmp;
        tmp = data_ptr[0];
        cur_pixel[0] = data_ptr[2];
        cur_pixel[1] = data_ptr[1];
        cur_pixel[2] = tmp;
        cur_pixel += 3;
        data_ptr += desc->channel;
    }
    desc->fmt = QBL_IMAGE_BGR24;
    desc->channel = 3;
    return data;
}

int qtk_image_save_bmp(const char *filename, uint8_t *data,
                       qtk_image_desc_t *desc) {
    return stbi_write_bmp(filename, desc->width, desc->height, desc->channel,
                          data);
}

int qtk_image_save_png(const char *filename, const uint8_t *data,
                       qtk_image_desc_t *desc) {
    return stbi_write_png(filename, desc->width, desc->height, desc->channel,
                          data, 0);
}

int qtk_image_resize(qtk_image_desc_t *desc, void *src, int out_h, int out_w,
                     void *dst) {
    if (desc->fmt == QBL_IMAGE_RGB24 || desc->fmt == QBL_IMAGE_BGR24) {
#ifdef QTK_USE_RGA
        if (out_w % 16 == 0 && desc->width % 16 == 0) {
            rga_buffer_t src_buf, dst_buf;
            im_rect src_rect, dst_rect;
            memset(&src_buf, 0, sizeof(src_buf));
            memset(&dst_buf, 0, sizeof(dst_buf));
            memset(&src_rect, 0, sizeof(src_rect));
            memset(&dst_rect, 0, sizeof(dst_rect));
            src_buf = wrapbuffer_virtualaddr(src, desc->width, desc->height,
                                             RK_FORMAT_RGB_888);
            dst_buf =
                wrapbuffer_virtualaddr(dst, out_w, out_h, RK_FORMAT_RGB_888);
            int ret = imcheck(src_buf, dst_buf, src_rect, dst_rect);
            if (ret != IM_STATUS_NOERROR) {
                wtk_debug("error %d : %s\n", ret, imStrError((IM_STATUS)ret));
            }
            imresize(src_buf, dst_buf);
	    return 0;
        } else {
            return stbir_resize_uint8(src, desc->width, desc->height, 0, dst,
                                      out_w, out_h, 0, desc->channel);
        }
#else
        return stbir_resize_uint8(src, desc->width, desc->height, 0, dst, out_w,
                                  out_h, 0, desc->channel);
#endif
    }
    return stbir_resize_uint8(src, desc->width, desc->height, 0, dst, out_w,
                              out_h, 0, desc->channel);
}

void qtk_image_pretreatment(const uint8_t *imagedata, float *data, float *svd,
                            float *std, const int len) {
    for (int i = 0; i < len; i++) {
        data[i] = (imagedata[i * 3] - svd[0]) / std[0];
        data[i + len] = (imagedata[i * 3 + 1] - svd[1]) / std[1];
        data[i + len * 2] = (imagedata[i * 3 + 2] - svd[2]) / std[2];
    }
}
