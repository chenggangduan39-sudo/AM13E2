#include "qtk_cv_detection_onnxruntime.h"
#include "qtk/image/qtk_image.h"
#include "wtk/core/wtk_os.h"

#if defined ONNX_DEC
qtk_cv_detection_onnxruntime_t *qtk_cv_detection_onnxruntime_new(char *model) {
    qtk_cv_detection_onnxruntime_t *invo =
        (qtk_cv_detection_onnxruntime_t *)wtk_malloc(
            sizeof(qtk_cv_detection_onnxruntime_t));
    qtk_onnxruntime_cfg_init(&invo->onnx_cfg);
    invo->onnx_cfg.onnx_fn = model;
    invo->onnx = qtk_onnxruntime_new(&invo->onnx_cfg);
    return invo;
}

qtk_cv_detection_onnxruntime_t *qtk_cv_detection_onnxruntime_new2(qtk_onnxruntime_cfg_t *cfg) {
    qtk_cv_detection_onnxruntime_t *invo =
        (qtk_cv_detection_onnxruntime_t *)wtk_malloc(
            sizeof(qtk_cv_detection_onnxruntime_t));
    qtk_onnxruntime_cfg_init(&invo->onnx_cfg);
    invo->onnx = qtk_onnxruntime_new(cfg);
    return invo;
}

void qtk_cv_detection_onnxruntime_delete(qtk_cv_detection_onnxruntime_t *invo) {
    qtk_onnxruntime_cfg_clean(&invo->onnx_cfg);
    qtk_onnxruntime_delete(invo->onnx);
    wtk_free(invo);
    invo = NULL;
}

void qtk_cv_detection_onnxruntime_invoke_single(
    float *input, float **output, uint8_t *data, float *svd, float *std,
    int64_t *shape, qtk_cv_detection_onnxruntime_t *invo, const int size) {

    qtk_onnxruntime_reset(invo->onnx);
    const int len = shape[2] * shape[3];
    qtk_image_pretreatment(data, input, svd, std, len);
    qtk_onnxruntime_feed(invo->onnx, input, len * 3, shape, 4, 0, 0);
    qtk_onnxruntime_run(invo->onnx);
    float *out = (float *)qtk_onnxruntime_getout(invo->onnx, 0);
    *output = out;
}

void qtk_cv_detection_onnxruntime_invoke_multiple(
    float **input, float **output, uint8_t *data, float *svd, float *std,
    int64_t **shape, qtk_cv_detection_onnxruntime_t *invo, const int size,
    const int len) {

    const int con = shape[0][2] * shape[0][3];
    qtk_image_pretreatment(data, input[0], svd, std, con);
    for (int i = 0; i < len; i++) {
        const int num = shape[i][1] * shape[i][2] * shape[i][3];
        qtk_onnxruntime_feed(invo->onnx, input[i], num, shape[i], 4, 0, i);
    }

    qtk_onnxruntime_run(invo->onnx);
    float *out = (float *)qtk_onnxruntime_getout(invo->onnx, 0);
    memcpy(output[0], out, size * sizeof(float));
    for (int i = 1; i < len; i++) {
        float *out = (float *)qtk_onnxruntime_getout(invo->onnx, i);
        memcpy(input[i], out,
               shape[i][1] * shape[i][2] * shape[i][3] * sizeof(float));
        memcpy(output[i], out,
               shape[i][1] * shape[i][2] * shape[i][3] * sizeof(float));
    }
    qtk_onnxruntime_reset(invo->onnx);
}

#elif defined IPU_DEC
qtk_cv_detection_onnxruntime_t *qtk_cv_detection_onnxruntime_ipu_new(char *model){
    qtk_cv_detection_onnxruntime_t *invo=(qtk_cv_detection_onnxruntime_t*)wtk_malloc(sizeof(qtk_cv_detection_onnxruntime_t));
    invo->ipu=qtk_ipu_channel_new(model);
    return invo;
}

void qtk_cv_detection_onnxruntime_ipu_delete(qtk_cv_detection_onnxruntime_t *invo){
    qtk_ipu_channel_delete(invo->ipu);
    wtk_free(invo);
    invo=NULL;
}

void qtk_cv_detection_onnxruntime_invoke_single_ipu(
    float *input, float **output, uint8_t *data, float *svd, float *std,
    int64_t *shape, qtk_cv_detection_onnxruntime_t *invo, const int size) {

    *output = qtk_ipu_single_invoke(invo->ipu, data);
}

void qtk_cv_detection_onnxruntime_invoke_multiple_ipu(
    float **input, float **output, uint8_t *data, float *svd, float *std,
    int64_t **shape, qtk_cv_detection_onnxruntime_t *invo,
    const int size, const int len){

        qtk_ipu_multiple_invoke(invo->ipu,data);
        for (int i=1;i<len;i++){
            memcpy(output[i],invo->ipu->output[0],shape[i][1] * shape[i][2] * shape[i][3] * sizeof(float));
        }
        memcpy(output[0],invo->ipu->output[0],size*sizeof(float));
}

#endif 
