#include "qtk/cv/portrait/qtk_portrait_prettier.h"
#include "opencv2/core/mat.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "qtk/image/qtk_image.h"
#include "qtk/math/qtk_math.h"
#ifndef WITH_OPENCV
#error "currently need opencv support"
#endif

using namespace cv;

static void skin_detect(InputArray img, OutputArray mask,
                        bool with_morph = true) {
    Mat hsv;
    Vec3b hsvL(0, 58, 30);
    Vec3b hsvH(33, 174, 255);
    cvtColor(img, hsv, COLOR_BGR2HSV);
    inRange(hsv, hsvL, hsvH, mask);
    if (with_morph) {
        auto disc = getStructuringElement(MORPH_ELLIPSE, Size(6, 6));
        morphologyEx(mask, mask, MORPH_OPEN, disc);
    }
}

static void dermabrasion(InputArray img, OutputArray out, InputArray mask,
                         float accuracy, float degree) {
    Mat tmp, tmp1, blured;
    Mat mask_c;
    if (accuracy == 0 && degree == 0) {
        img.copyTo(out);
        return;
    }
    bilateralFilter(img, tmp, accuracy * 5, accuracy * 12.5, accuracy * 12.5);
    auto img1 = tmp - img.getMat() + 128;
    Size sz(2 * degree - 1, 2 * degree - 1);
    GaussianBlur(img1, tmp1, sz, 0);
    auto img2 = img.getMat() + 2 * tmp1 - 255;
    blured = (img.getMat() * 50) / 100 + (img2 * 50) / 100;
    bitwise_not(mask, mask_c);
    tmp1 = 0;
    bitwise_and(img, img, tmp1, mask_c);
    tmp = 0;
    bitwise_and(blured, blured, tmp, mask);
    out.create(img.size(), CV_8UC(img.channels()));
    out.getMat() = tmp1 + tmp;
}

// capi
extern "C" {

int qtk_portrait_prettier_init(qtk_portrait_prettier_t *p,
                               qtk_portrait_prettier_cfg_t *cfg) {
    p->cfg = cfg;
    for (int i = 0; i < 256; i++) {
        float midtone = i + p->cfg->skin_whitening_degree * 0.667 *
                                (1 - ((i - 127.0) / 127) * ((i - 127.0) / 127));
        p->midtones_lut[i] = QBL_USAT8(qtk_roundf(midtone));
    }
    return 0;
}

void qtk_portrait_prettier_clean(qtk_portrait_prettier_t *p) {}

int qtk_portrait_prettier_process(qtk_portrait_prettier_t *p, void *img,
                                  qtk_image_desc_t *desc) {
    Mat mask, out;
    int processed = 0;
    if (desc->fmt != QBL_IMAGE_BGR24) {
        return -1;
    }
    Mat in(desc->height, desc->width, CV_8UC3, img);
    skin_detect(in, mask, true);

    if (p->cfg->skin_whitening) {
        uint8_t *imgp = cast(uint8_t *, img);
        for (int i = 0; i < desc->height; i++) {
            for (int j = 0; j < desc->width; j++) {
                if (mask.data[i * mask.step + j]) {
                    for (int c = 0; c < desc->channel; c++) {
                        imgp[c] = (p->midtones_lut[imgp[c]]) * 50 / 100 +
                                  (imgp[c] * 50) / 100;
                    }
                }
                imgp += desc->channel;
            }
        }
    }
    if (p->cfg->dermabrasion) {
        dermabrasion(in, out, mask, p->cfg->dermabrasion_accuracy,
                     p->cfg->dermabrasion_degree);
        processed = 1;
    }
    if (processed) {
        if (out.isContinuous()) {
            memcpy(img, out.ptr(), out.total() * out.elemSize());
        } else {
            for (int i = 0; i < desc->height; i++) {
                memcpy(cast(char *, img) + desc->channel * desc->width,
                       out.ptr() + i * out.step, out.elemSize() * out.cols);
            }
        }
    }
    return 0;
}

void qtk_portrait_prettier_cfg_init(qtk_portrait_prettier_cfg_t *cfg) {
    cfg->dermabrasion = 1;
    cfg->skin_whitening = 1;
    cfg->skin_detect_with_morph = 1;
    cfg->dermabrasion_accuracy = 3;
    cfg->dermabrasion_degree = 2;
    cfg->skin_whitening_degree = 30;
}

}; // extern "C"
