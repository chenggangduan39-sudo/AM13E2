#ifndef F709CFFA_A524_4E57_9AA5_18CFEF5AB0FF
#define F709CFFA_A524_4E57_9AA5_18CFEF5AB0FF
#ifdef __cplusplus
extern "C" {
#endif

#include "qtk/cv/qtk_cv_bbox.h"
#include "qtk/image/qtk_image.h"
#include "qtk_cv_detection_onnxruntime.h"

// onnx推理
typedef void (*invoke_t)(float *input, float **output, uint8_t *data,
                         float *svd, float *std, int64_t *shape,
                         qtk_cv_detection_onnxruntime_t *invo, const int size);

// onnx多输入多数出推理
typedef void (*invoke_mum)(float **input, float **output, uint8_t *data,
                           float *svd, float *std, int64_t **shape,
                           qtk_cv_detection_onnxruntime_t *invo, const int size,
                           const int len);

//缩放
typedef void (*resize_t)(uint8_t *src, qtk_image_desc_t *src_desc, uint8_t *dst,
                         qtk_image_desc_t *dst_desc);

typedef struct qtk_cv_detection_res {
    qtk_cv_bbox_t roi;
    float data[10];
    float score;
    int num;
    int height;
    int width;
    float cen_y;
    float cen_x;
    int classify;
} qtk_cv_detection_res_t;

typedef struct qtk_cv_detection_onnx_box {
    qtk_cv_detection_res_t box[1000];
    int cnt;
} qtk_cv_detection_onnx_box_t;

//筛选框赋值
void qtk_cv_detection_onnx_box_push(qtk_cv_detection_onnx_box_t *shape,
                                    float x1, float y1, float x2, float y2,
                                    float score);

//将结果框坐标变成方形 desc：限制区域
void qtk_cv_detection_onnx_box_square(qtk_cv_detection_onnx_box_t *box,
                                      qtk_image_desc_t desc);

//消除重叠框
void qtk_cv_detection_onnx_nms(qtk_cv_detection_onnx_box_t *shape,
                               qtk_cv_detection_onnx_box_t *res, float iou,
                               qtk_image_desc_t dst_desc);

// softmax
void qtk_cv_detection_onnx_softmax(const float *x, float *y, int length);

//
int qtk_cv_detection_onnx_box_cmp(const void *a, const void *b);

//扣图 imagedata:原图图像数据 desc:原图图像信息 data:扣图输出数据
//box:目标扣图的坐标信息
void qtk_cv_detection_onnx_image_rect(uint8_t *imagedata,
                                      qtk_image_desc_t *desc, uint8_t *data,
                                      qtk_cv_bbox_t *box);

//通道转换BGR转RGB size:rows*cols imagedata:图像数据 data:输出数据
void qtk_cv_detection_onnx_image_BGRtoRGB(uint8_t *imagedata, uint8_t *data,
                                          const int size);

//通道转换BGRA转BGR size:rows*cols  imagedata:图像数据 data:输出数据
void qtk_cv_detection_onnx_image_BGRAtoBGR(uint8_t *imagedata, uint8_t *data,
                                           const int size);

//通道转换BGRA转RGB size:rows*cols imagedata:图像数据 data:输出数据
void qtk_cv_detection_onnx_image_BGRAtoRGB(uint8_t *imagedata, uint8_t *data,
                                           const int size);

//消除检测动态框，保留稳定中的检测框  iou:重叠率(值越大束缚能力越强)
void qtk_cv_detection_onnx_box_stillness(qtk_cv_detection_onnx_box_t *res,
                                         float iou);

#ifdef __cplusplus
};
#endif
#endif /* F709CFFA_A524_4E57_9AA5_18CFEF5AB0FF */
