#include "qtk/image/qtk_image_color.h"

int qtk_image_color_bgr2gray_u8(uint8_t *bgr, uint8_t *gray,
                                qtk_image_desc_t *desc) {
    for (int i = 0; i < desc->height; i++) {
        for (int j = 0; j < desc->width; j++, gray++, bgr += 3) {
            *gray =
                cast(uint8_t,
                     (bgr[2] * 6969 + bgr[1] * 23434 + bgr[0] * 2365) >> 15);
        }
    }
    return 0;
}

int qtk_image_color_rgb2gray_u8(uint8_t *rgb, uint8_t *gray,
                                qtk_image_desc_t *desc) {
    for (int i = 0; i < desc->height; i++) {
        for (int j = 0; j < desc->width; j++, gray++, rgb += 3) {
            *gray =
                cast(uint8_t,
                     (rgb[0] * 6969 + rgb[1] * 23434 + rgb[2] * 2365) >> 15);
        }
    }
    return 0;
}
