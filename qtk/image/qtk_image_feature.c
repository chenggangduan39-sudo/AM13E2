#include "qtk/image/qtk_image_feature.h"
#include "qtk/math/qtk_math.h"
#include "qtk/core/qtk_type.h"

static const float color_names[][10]={
#include "qtk/image/qtk_image_feature_colornames.h"
};

// Note: Convert BGR to ColorNames
int qtk_image_feature_cn(uint8_t *img, qtk_image_desc_t *desc, float *dst) {
    uint8_t *pixel = img;
    unsigned int index;
    qtk_assert(desc->channel == 3);

    for (int i = 0; i < desc->height; i++) {
        for (int j = 0; j <desc->width; j++, pixel+=3) {
            index = cast(unsigned int, qtk_floorf(cast(float, pixel[2])/8) +
                                       32 * qtk_floorf(cast(float, pixel[1])/8) +
                                       32*32*qtk_floorf(cast(float, pixel[0])/8));
            memcpy(dst, color_names[index], sizeof(color_names[index]));
            dst += 10;
        }
    }
    return 0;
}
