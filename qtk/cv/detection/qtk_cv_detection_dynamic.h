#ifndef D1805221_474D_45AE_AE14_9C7F41EEEBF5
#define D1805221_474D_45AE_AE14_9C7F41EEEBF5
#ifdef __cplusplus
extern "C" {
#endif
#include "qtk/cv/recognize/qtk_recognize_tsm.h"
#include "qtk_cv_detection_onnx.h"

#define DYNAMIC_NUM 17
/*动态手势指针*/
typedef struct qtk_cv_detection_dynamic {
    float *output[DYNAMIC_NUM]; //输出结果
    float *input[DYNAMIC_NUM];  //输入
    float svd[3];      //均值 RGB  [123.675,116.25,103.53] RGB
    float std[3];      //方差 RGB  [58.395,57.12,57.375] RGB
    int64_t *sv_shape[DYNAMIC_NUM]; //输入形状
    qtk_image_desc_t dynamic_desc;  //输入形状 width:224 height:224
    qtk_image_desc_t img_desc;    //图片的形状
    int num;          //结果
    qtk_recognize_tcm_post_t post; //平滑处理
    float score;      //得分阈值
    qtk_cv_detection_onnxruntime_t *invo;
    uint8_t *data0;
    uint8_t *data1;
} qtk_cv_detection_dynamic_t;

//申请动态手势内存空间
qtk_cv_detection_dynamic_t *qtk_cv_detection_dynamic_new();

//销毁动态手势内存空间  dy：动态手势指针
void qtk_cv_detection_dynamic_delete(qtk_cv_detection_dynamic_t *dy);

//动态手势检测  dy：动态手势指针 imagedata：图像数据
void qtk_cv_detection_dynamic_process(qtk_cv_detection_dynamic_t *dy,
                                      uint8_t *imagedata, invoke_mum invoke);

//显示手势类别  t:由动态手势得到的结果
void qtk_cv_detection_dynamic_show(int t);

//动态手势通用接口 imagedata:图像数据(不超过4K) desc:图像信息 resize:缩放接口
//invoke:推理接口
void qtk_cv_detection_dynamic_process_data(qtk_cv_detection_dynamic_t *dy,
                                           uint8_t *imagedata,
                                           qtk_image_desc_t *desc,
                                           resize_t resize, invoke_mum invoke);

#ifdef __cplusplus
};
#endif
#endif /* D1805221_474D_45AE_AE14_9C7F41EEEBF5 */
