#include "qtk_nv12.h"
#include "qtk/math/qtk_math.h"

#include <math.h>

#define NV12_GET_YP(d, x, y, width) ((d) + (y) * (width) + (x))
#define NV12_GET_UP(d, x, y, width) ((d) + (y) * (width) + (x << 1))
#define NV12_GET_VP(d, x, y, width) ((d) + (y) * (width) + (x << 1) + 1)

int qtk_nv12_crop(const uint8_t *src, int src_w, int src_h, uint8_t *dst,
                  qtk_image_roi_t *roi) {
    const uint8_t *src_y, *src_uv;
    int dst_w = roi->width;
    int dst_h = roi->height;
    int x = roi->x;
    int y = roi->y;
    uint8_t *dst_y, *dst_uv;
    int src_uv_offset = src_w * src_h;
    int dst_uv_offset = dst_w * dst_h;
    int src_y_stride = src_w;
    int dst_y_stride = dst_w;
    int src_uv_stride = (src_w >> 1) << 1;
    int dst_uv_stride = (dst_w >> 1) << 1;
    int r, l;

    l = dst_h;
    for (r = 0, src_y = src + y * src_y_stride, dst_y = dst; r < l;
         r++, src_y += src_y_stride, dst_y += dst_y_stride) {
        memcpy(dst_y, src_y + x, sizeof(uint8_t) * dst_w);
    }

    l = dst_h >> 1;
    for (r = 0, src_uv = src + src_uv_offset + (y >> 1) * src_uv_stride,
        dst_uv = dst + dst_uv_offset;
         r < l; r++, src_uv += src_uv_stride, dst_uv += dst_uv_stride) {
        memcpy(dst_uv, src_uv + x, sizeof(uint8_t) * dst_w);
    }

    return 0;
}

int qtk_nv12_draw_rect(uint8_t *src, int src_w, int src_h, qtk_image_roi_t *roi,
                       int x_width, int y_width, uint8_t Y, uint8_t U,
                       uint8_t V) {
    int i, j;
    uint8_t *cur_y;
    uint8_t *cur_uv;
    int tot_row, tot_col;
    int y_stride, uv_stride, uv_offset;
    uint8_t uv[2];
    uv[0] = U;
    uv[1] = V;
    int rect_h, rect_w, x, y;

    if (roi) {
        rect_h = roi->height;
        rect_w = roi->width;
        x = roi->x;
        y = roi->y;
    } else {
        rect_h = src_h;
        rect_w = src_w;
        x = 0;
        y = 0;
    }
    y_stride = src_w;
    cur_y = src + y * y_stride;
    uv_offset = src_w * src_h;
    uv_stride = src_w;

    tot_row = rect_h;
    for (i = 0, cur_y = src + y * y_stride; i < tot_row;
         i++, cur_y += y_stride) {
        if (i < y_width) {
            memset(cur_y + x, Y, rect_w);
        } else if (i >= rect_h - y_width) {
            memset(cur_y + x, Y, rect_w);
        } else {
            memset(cur_y + x, Y, x_width);
            memset(cur_y + x + rect_w - x_width, Y, x_width);
        }
    }

    y_width = min(1, y_width >> 1);
    x_width = min(1, x_width >> 1);
    tot_row = rect_h >> 1;
    tot_col = rect_w >> 1;
    x = x >> 1;
    y = y >> 1;
    for (i = 0, cur_uv = src + uv_offset + y * uv_stride; i < tot_row;
         i++, cur_uv += uv_stride) {
        if (i < y_width) {
            for (j = 0; j < tot_col; j++) {
                cast(uint16_t *, cur_uv)[j + x] = *cast(uint16_t *, uv);
            }
        } else if (i >= tot_row - y_width) {
            for (j = 0; j < tot_col; j++) {
                cast(uint16_t *, cur_uv)[j + x] = *cast(uint16_t *, uv);
            }
        } else {
            for (j = 0; j < x_width; j++) {
                cast(uint16_t *, cur_uv)[j + x] = *cast(uint16_t *, uv);
            }
            for (j = 0; j < x_width; j++) {
                cast(uint16_t *, cur_uv)[j + x + tot_col - x_width] =
                    *cast(uint16_t *, uv);
            }
        }
    }

    return 0;
}

int qtk_nv12_fill_rect(uint8_t *src, int src_w, int src_h, qtk_image_roi_t *roi,
                       uint8_t *roi_imag) {
    int x, y, rect_h, rect_w;
    int src_uv_offset, roi_uv_offset;
    int src_y_stride, roi_y_stride, src_uv_stride, roi_uv_stride;
    int i, cnt;
    uint8_t *src_y, *roi_y;
    uint8_t *src_uv, *roi_uv;

    if (roi) {
        rect_h = roi->height;
        rect_w = roi->width;
        x = roi->x;
        y = roi->y;
    } else {
        rect_h = src_h;
        rect_w = src_w;
        x = 0;
        y = 0;
    }

    src_uv_offset = src_w * src_h;
    roi_uv_offset = rect_w * rect_h;
    src_y_stride = src_w;
    roi_y_stride = rect_w;
    src_uv_stride = src_w;
    roi_uv_stride = rect_w;

    for (i = 0, cnt = rect_h, src_y = src + y * src_y_stride, roi_y = roi_imag;
         i < cnt; i++, roi_y += roi_y_stride, src_y += src_y_stride) {
        memcpy(src_y + x, roi_y, sizeof(uint8_t) * rect_w);
    }

    for (i = 0, cnt = rect_h >> 1,
        src_uv = src + src_uv_offset + (y >> 1) * src_uv_stride,
        roi_uv = roi_imag + roi_uv_offset;
         i < cnt; i++, src_uv += src_uv_stride, roi_uv += roi_uv_stride) {
        memcpy(src_uv + x, roi_uv, sizeof(uint8_t) * rect_w);
    }

    return 0;
}

int qtk_nv12_resize_bilinear(uint8_t *src, int src_w, int src_h, int dst_w,
                             int dst_h, uint8_t *dst) {
    float x_ratio, y_ratio;
    float x_weight, y_weight;
    float m, n, p, q, m1, n1, p1, q1;
    int x_l, y_l, x_h, y_h;
    uint8_t *dst_cur_y, *dst_cur_u, *dst_cur_v, *dst_uv, *src_uv;
    int i, j;
    int row, col, src_row, src_col;

    x_ratio = dst_w > 1 ? cast(float, src_w) / cast(float, dst_w) : 0;
    y_ratio = dst_h > 1 ? cast(float, src_h) / cast(float, dst_h) : 0;

    dst_cur_y = dst;
    row = dst_h;
    col = dst_w;
    src_row = src_h;
    src_col = src_w;
    for (i = 0; i < row; i++) {
        for (j = 0; j < col; j++, dst_cur_y++) {
            x_l = cast(int, floorf(x_ratio * j));
            y_l = cast(int, floorf(y_ratio * i));
            x_h = min(cast(int, ceilf(x_ratio * j)), src_col - 1);
            y_h = min(cast(int, ceilf(y_ratio * i)), src_row - 1);

            x_weight = (x_ratio * j) - x_l;
            y_weight = (y_ratio * i) - y_l;

            m = *NV12_GET_YP(src, x_l, y_l, src_col);
            n = *NV12_GET_YP(src, x_h, y_l, src_col);
            p = *NV12_GET_YP(src, x_l, y_h, src_col);
            q = *NV12_GET_YP(src, x_h, y_h, src_col);

            *dst_cur_y = m * (1 - x_weight) * (1 - y_weight) +
                         n * x_weight * (1 - y_weight) +
                         p * y_weight * (1 - x_weight) +
                         q * x_weight * y_weight;
        }
    }

    row = dst_h >> 1;
    col = dst_w >> 1;
    src_row = src_h >> 1;
    src_col = src_w >> 1;
    dst_uv = dst + dst_w * dst_h;
    src_uv = src + src_w * src_h;
    for (i = 0; i < row; i++) {
        for (j = 0; j < col; j++, dst_cur_y++) {
            x_l = cast(int, floorf(x_ratio * j));
            y_l = cast(int, floorf(y_ratio * i));
            x_h = min(cast(int, ceilf(x_ratio * j)), src_col - 1);
            y_h = min(cast(int, ceilf(y_ratio * i)), src_row - 1);

            x_weight = (x_ratio * j) - x_l;
            y_weight = (y_ratio * i) - y_l;

            m = *NV12_GET_UP(src_uv, x_l, y_l, src_w);
            n = *NV12_GET_UP(src_uv, x_h, y_l, src_w);
            p = *NV12_GET_UP(src_uv, x_l, y_h, src_w);
            q = *NV12_GET_UP(src_uv, x_h, y_h, src_w);

            m1 = *NV12_GET_VP(src_uv, x_l, y_l, src_w);
            n1 = *NV12_GET_VP(src_uv, x_h, y_l, src_w);
            p1 = *NV12_GET_VP(src_uv, x_l, y_h, src_w);
            q1 = *NV12_GET_VP(src_uv, x_h, y_h, src_w);

            dst_cur_u = NV12_GET_UP(dst_uv, j, i, dst_w);
            dst_cur_v = NV12_GET_VP(dst_uv, j, i, dst_w);

            *dst_cur_u = m * (1 - x_weight) * (1 - y_weight) +
                         n * x_weight * (1 - y_weight) +
                         p * y_weight * (1 - x_weight) +
                         q * x_weight * y_weight;

            *dst_cur_v = m1 * (1 - x_weight) * (1 - y_weight) +
                         n1 * x_weight * (1 - y_weight) +
                         p1 * y_weight * (1 - x_weight) +
                         q1 * x_weight * y_weight;
        }
    }

    return 0;
}

void qtk_nv12_to_bgr24(const uint8_t *nv12, uint8_t *bgr, int w, int h) {
    int wi, hi;
    int y_idx, u_idx, v_idx;
    int y, u, v, r, g, b;
    const uint8_t *y_ptr, *uv_ptr;

    y_ptr = nv12;
    uv_ptr = nv12 + w * h;
    for (y_idx = 0, hi = 0; hi < h; hi++) {
        for (wi = 0; wi < w; wi++, y_idx++) {
            u_idx = (hi >> 1) * w + (wi >> 1 << 1);
            v_idx = u_idx + 1;

            y = y_ptr[y_idx];
            u = cast(int, uv_ptr[u_idx]) - 128;
            v = cast(int, uv_ptr[v_idx]) - 128;

            r = y + v + ((v * 103) >> 8);
            g = y - (((u * 88) >> 8) + ((v * 183) >> 8));
            b = y + u + ((u * 198) >> 8);

            r = QBL_USAT8(r);
            g = QBL_USAT8(g);
            b = QBL_USAT8(b);

            bgr[3 * y_idx] = b;
            bgr[3 * y_idx + 1] = g;
            bgr[3 * y_idx + 2] = r;
        }
    }
}

void qtk_nv12_to_rgb24(const uint8_t *nv12, uint8_t *rgb, int w, int h) {
    int wi, hi;
    int y_idx, u_idx, v_idx;
    int y, u, v, r, g, b;
    const uint8_t *y_ptr, *uv_ptr;

    y_ptr = nv12;
    uv_ptr = nv12 + w * h;
    for (y_idx = 0, hi = 0; hi < h; hi++) {
        for (wi = 0; wi < w; wi++, y_idx++) {
            u_idx = (hi >> 1) * w + (wi >> 1 << 1);
            v_idx = u_idx + 1;

            y = y_ptr[y_idx];
            u = cast(int, uv_ptr[u_idx]) - 128;
            v = cast(int, uv_ptr[v_idx]) - 128;

            r = y + v + ((v * 103) >> 8);
            g = y - (((u * 88) >> 8) + ((v * 183) >> 8));
            b = y + u + ((u * 198) >> 8);

            r = QBL_USAT8(r);
            g = QBL_USAT8(g);
            b = QBL_USAT8(b);

            rgb[3 * y_idx] = r;
            rgb[3 * y_idx + 1] = g;
            rgb[3 * y_idx + 2] = b;
        }
    }
}
