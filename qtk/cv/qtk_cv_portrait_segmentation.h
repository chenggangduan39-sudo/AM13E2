#ifndef A33FCE47_6C88_4941_83C9_A1D29019E721
#define A33FCE47_6C88_4941_83C9_A1D29019E721
#ifdef __cplusplus
extern "C" {
#endif

#include "qtk/cv/detection/qtk_cv_detection_onnx.h"
#include "qtk/cv/detection/qtk_cv_detection_onnxruntime.h"

typedef struct qtk_cv_portrait_segmentation
{
    int out_step;
    float *dst;
    uint8_t *dk;
    int64_t sv_shape[4];
    float *input;
    uint8_t *fk_data;
    unsigned int data_bk:1;
    uint8_t *data;
    float svd[3];
    float std[3];
    qtk_image_desc_t body_desc;
    qtk_image_desc_t img_desc;
    qtk_cv_detection_onnxruntime_t *invo;
}qtk_cv_portrait_segmentation_t;


qtk_cv_portrait_segmentation_t *qtk_cv_portrait_segmentation_new();
void qtk_cv_portrait_segmentation_delete(qtk_cv_portrait_segmentation_t *pa);
void qtk_cv_portrait_segmentation_process(qtk_cv_portrait_segmentation_t *pa,uint8_t *img,invoke_t invoke ,uint8_t *dt_img);

#ifdef __cplusplus
};
#endif
#endif /* A33FCE47_6C88_4941_83C9_A1D29019E721 */
