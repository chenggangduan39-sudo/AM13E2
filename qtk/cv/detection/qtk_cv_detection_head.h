#ifndef A7349D55_F3E0_426A_B571_557A68F799D4
#define A7349D55_F3E0_426A_B571_557A68F799D4
#ifdef __cplusplus
extern "C" {
#endif
#include "qtk_cv_detection_onnx.h"

/*人头结构体指针*/
typedef struct qtk_cv_detection_head {
    float *input;  //onnx输入结果
    float svd[3];  //图像的均值 BGR   127.5
    float std[3];  //图像的方差 BGR   128
    int64_t sv_shape[4]; //图像的形状 或为onnx的输入形状  （1，3，544，960）
    qtk_image_desc_t head_desc;  //onnx的输入形状 （width：960 height：544）
    qtk_image_desc_t img_desc;   //图像的形状
    float iou;    //筛选概率
    float conf;   //重叠概率
    int len;      //检测步长  5
    int anchors_num; //重复检测次数  2
    int steps_nums;  //检测数量  3
    qtk_cv_detection_onnx_box_t shape; //初次筛选结果
    qtk_cv_detection_onnx_box_t res;   //最终结果
    qtk_cv_detection_onnxruntime_t *invo;
} qtk_cv_detection_head_t;

//人头结构体指针内存申请
qtk_cv_detection_head_t *qtk_cv_detection_head_new();

//人头结构体指针内存释放  head:人头结构体指针
void qtk_cv_detection_head_delete(qtk_cv_detection_head_t *head);

//人头检测    head：人头结构体指针   imagedata：图像数据（960*544*3） BGR
void qtk_cv_detection_head_process(qtk_cv_detection_head_t *head,
                                   uint8_t *imagedata, invoke_t invoke);
#ifdef __cplusplus
};
#endif
#endif /* A7349D55_F3E0_426A_B571_557A68F799D4 */
