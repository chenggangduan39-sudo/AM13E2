#ifndef ECEA19DD_888E_4CBA_BAE0_376A6FDF095A
#define ECEA19DD_888E_4CBA_BAE0_376A6FDF095A
#ifdef __cplusplus
extern "C" {
#endif

#include "wtk/asr/fextra/onnxruntime/qtk_onnxruntime.h"
#include "wtk/asr/fextra/onnxruntime/qtk_onnxruntime_cfg.h"
#include "wtk/core/wtk_alloc.h"

#if defined  IPU_DEC
#include "qtk/qtk_ipu.h"
#endif

#include <stdint.h>
#include <stdio.h>

typedef struct qtk_cv_detection_onnxruntime {
#if defined  ONNX_DEC
    qtk_onnxruntime_cfg_t onnx_cfg;
    qtk_onnxruntime_t *onnx;
#elif defined IPU_DEC
    qtk_ipu_t *ipu;
#endif 
} qtk_cv_detection_onnxruntime_t;

#if defined  ONNX_DEC
//创建
qtk_cv_detection_onnxruntime_t *qtk_cv_detection_onnxruntime_new(char *model);
qtk_cv_detection_onnxruntime_t *qtk_cv_detection_onnxruntime_new2(qtk_onnxruntime_cfg_t *cfg);

//销毁
void qtk_cv_detection_onnxruntime_delete(qtk_cv_detection_onnxruntime_t *invo);

/*单输入输出 input:输入所需的内存
           output:输出结果
           data:图像数据
           svd:均值
           std:方差
           shape:送入数据的形状信息
           size:输出数据的长度
*/
void qtk_cv_detection_onnxruntime_invoke_single(
    float *input, float **output, uint8_t *data, float *svd, float *std,
    int64_t *shape, qtk_cv_detection_onnxruntime_t *invo, const int size);

/*多输入输出 input:输入所需的内存
           output:输出结果
           data:图像数据
           svd:均值
           std:方差
           shape:输入的形状
           size:输出0通道的长度
           len:输入\输出的个数
*/
void qtk_cv_detection_onnxruntime_invoke_multiple(
    float **input, float **output, uint8_t *data, float *svd, float *std,

    int64_t **shape, qtk_cv_detection_onnxruntime_t *invo,
    const int size, const int len);

#elif defined IPU_DEC
//创建IPU通道 注意:请先创建IPU设备
qtk_cv_detection_onnxruntime_t *qtk_cv_detection_onnxruntime_ipu_new(char *model);

//销毁IPU通道 注意:别忘了在这之后销毁IPU设备
void qtk_cv_detection_onnxruntime_ipu_delete(qtk_cv_detection_onnxruntime_t *invo);


/*单输入输出推理 input:NULL 
            data:图像数据
            output:输出结果(size*sizeof(float))
            svd:NULL
            std:NULL 
            shape:NULL 
            size:输出长度*/
void qtk_cv_detection_onnxruntime_invoke_single_ipu(
    float *input, float **output, uint8_t *data, float *svd, float *std,
    int64_t *shape, qtk_cv_detection_onnxruntime_t *invo, const int size);

/*多输入输出推理 input:NULL
                data:图像数据
                output:输出结果
                std:NULL
                svd:NULL
                shape:输入通道的大小
                size:0通道输出的大小
                len:输出通道的个数
                */
void qtk_cv_detection_onnxruntime_invoke_multiple_ipu(
    float **input, float **output, uint8_t *data, float *svd, float *std,
    int64_t **shape, qtk_cv_detection_onnxruntime_t *invo,
    const int size, const int len);


#endif

#ifdef __cplusplus
};
#endif
#endif /* ECEA19DD_888E_4CBA_BAE0_376A6FDF095A */

