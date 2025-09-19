
#include "qtk/cv/classify/qtk_cv_classify.h"
#include "wtk/core/math/wtk_math.h"

qtk_cv_classify_t *qtk_cv_classify_new(qtk_cv_classify_cfg_t *cfg) {
    qtk_cv_classify_t *cls = wtk_malloc(sizeof(qtk_cv_classify_t));
    cls->cfg = cfg;
    cls->nnrt = qtk_nnrt_new(&cfg->nnrt);
    // (cls->nnrt, 0, shape, 4);
    cls->H = cfg->height;
    cls->W = cfg->width;
    cls->input = wtk_malloc(sizeof(float) * cls->H * cls->W * 3);
    return cls;
}

void qtk_cv_classify_delete(qtk_cv_classify_t *cls) {
    qtk_nnrt_delete(cls->nnrt);
    wtk_free(cls->input);
    wtk_free(cls);
}

int qtk_cv_classify_feed(qtk_cv_classify_t *cls, uint8_t *image) {
    int n = cls->H * cls->W;
    // wtk_debug("cls->H = %d, cls->W = %d, n = %d\n", cls->H, cls->W, n);
    qtk_nnrt_value_t input, output;
    float *output_val;
    int i;
    int result;
    int64_t shape[4] = {1, 3, cls->H, cls->W};
    if (cls->nnrt->cfg->use_ss_ipu) {
        input = qtk_nnrt_value_create_external(cls->nnrt, QTK_NNRT_VALUE_ELEM_U8, shape, 4, image) ;
    }else{
        for (i = 0; i < n; i++) {
            cls->input[i] = (image[i * 3 + 0] - cls->cfg->mean[0]) / cls->cfg->std[0];
            cls->input[i + n] =
                (image[i * 3 + 1] - cls->cfg->mean[1]) / cls->cfg->std[1];
            cls->input[i + n * 2] =
                (image[i * 3 + 2] - cls->cfg->mean[2]) / cls->cfg->std[2];
        }
        input = qtk_nnrt_value_create_external(cls->nnrt, QTK_NNRT_VALUE_ELEM_F32,
                                            shape, 4, cls->input);
    }
    qtk_nnrt_feed(cls->nnrt, input, 0);
    qtk_nnrt_run(cls->nnrt);
    qtk_nnrt_get_output(cls->nnrt, &output, 0);
    output_val = qtk_nnrt_value_get_data(cls->nnrt, output);
    // qtk_nnrt_value_get_shape(cls->nnrt, output, outshape, 2);

    result = wtk_float_argmax(output_val, cls->cfg->nlabels);
    qtk_nnrt_value_release(cls->nnrt, input);
    qtk_nnrt_value_release(cls->nnrt, output); 
    return result;
}

wtk_string_t *qtk_cv_classify_get_label(qtk_cv_classify_t *cls, int id) {
    if (id >= cls->cfg->nlabels || id < 0) {
        return NULL;
    }
    return cls->cfg->labels[id];
}
