#include "qtk/cv/sr/qtk_cv_sr.h"

qtk_cv_sr_t *qtk_cv_sr_new(qtk_cv_sr_cfg_t *cfg) {
    qtk_cv_sr_t *sr = wtk_malloc(sizeof(qtk_cv_sr_t));
    sr->cfg = cfg;
    sr->nnrt = qtk_nnrt_new(&cfg->nnrt);
    sr->input_size = 0;
    sr->input = NULL;
    sr->output = NULL;
    return sr;
}

void qtk_cv_sr_delete(qtk_cv_sr_t *sr) {
    if (sr->input) {
        wtk_free(sr->input);
    }
    if (sr->output) {
        wtk_free(sr->output);
    }
    qtk_nnrt_delete(sr->nnrt);
    wtk_free(sr);
}

uint8_t *qtk_cv_sr_feed(qtk_cv_sr_t *sr, uint8_t *img, int width, int height) {
    int i;
    int npixel = width * height;
    int need_sz = npixel * 3;
    int output_sz;
    float *output_val;
    qtk_nnrt_value_t input, output;
    int64_t shape[4] = {1, 3, height, width};
    if (sr->input_size < need_sz) {
        if (sr->input) {
            wtk_free(sr->input);
        }
        sr->input = wtk_malloc(sizeof(float) * need_sz);
        sr->input_size = need_sz;
    }
    for (i = 0; i < npixel; i++) {
        sr->input[i] = img[i * 3 + 0] / 255.0f;
        sr->input[i + npixel] = img[i * 3 + 1] / 255.0f;
        sr->input[i + npixel * 2] = img[i * 3 + 2] / 255.0f;
    }
    if (sr->output) {
        qtk_nnrt_value_release(sr->nnrt, sr->output);
    }
    input = qtk_nnrt_value_create_external(sr->nnrt, QTK_NNRT_VALUE_ELEM_F32,
                                           shape, 4, sr->input);
    qtk_nnrt_feed(sr->nnrt, input, 0);
    qtk_nnrt_run(sr->nnrt);
    qtk_nnrt_get_output(sr->nnrt, &output, 0);
    qtk_nnrt_value_release(sr->nnrt, input);
    output_val = qtk_nnrt_value_get_data(sr->nnrt, output);
    qtk_nnrt_value_get_shape(sr->nnrt, output, shape, 4);
    npixel = shape[2] * shape[3];
    output_sz = npixel * 3;
    if (sr->output_size < output_sz) {
        if (sr->output) {
            wtk_free(sr->output);
        }
        sr->output = wtk_malloc(sizeof(uint8_t) * output_sz);
        sr->output_size = output_sz;
    }
    for (i = 0; i < npixel; i++) {
        sr->output[i * 3 + 0] = output_val[i] * 255;
        sr->output[i * 3 + 1] = output_val[i + npixel] * 255;
        sr->output[i * 3 + 2] = output_val[i + npixel * 2] * 255;
    }
    return sr->output;
}
