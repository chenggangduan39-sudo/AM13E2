#ifndef F8BFF5B6_D380_430B_A485_CF4BA2CC345C
#define F8BFF5B6_D380_430B_A485_CF4BA2CC345C
#ifdef __cplusplus
extern "C" {
#endif
#include "qtk_cv_detection_onnx.h"

/*人形检测结构体*/
typedef struct qtk_cv_detection_body {
    int64_t sv_shape[4];           // onnx输入形状  (1,3,640,1024)
    float svd[3];                  //图片的均值 127.5
    float std[3];                  //图片的方差 128
    float *input;                  // onnx的输入
    qtk_image_desc_t body_desc; // onnx的输入形状  （width：1024 height：640）
    qtk_image_desc_t img_desc; //图像的形状
    float iou;                 //检测概率
    float conf;                //重叠概率
    int reg_max;               //检测步长  5
    int num_stages;            // stride的个数  （检测数据长度） 5
    qtk_cv_detection_onnx_box_t shape; //初步检测结果
    qtk_cv_detection_onnx_box_t res;   //最终检测结果
    uint8_t *data0;
    uint8_t *data1;
    qtk_cv_detection_onnxruntime_t *invo;
} qtk_cv_detection_body_t;

//人形检测结构体内存申请
qtk_cv_detection_body_t *qtk_cv_detection_body_new();

//人形检测结构体内存释放  body为人形结构体指针
void qtk_cv_detection_body_detele(qtk_cv_detection_body_t *body);

//人形检测  body为人形结构体指针 imagedata为图像数据（1024*640*3）BGR
void qtk_cv_detection_body_process(qtk_cv_detection_body_t *body,
                                   uint8_t *imagedata, invoke_t invoke);

/*人形检测通用接口
    imagedata:图像数据(大小不能超过4K)；
    body:人形结构体指针
    invoke:onnx推理指针
    desc:输入图像参数
    resize:缩放函数指针
*/
void qtk_cv_detection_body_process_data(uint8_t *imagedata,
                                        qtk_cv_detection_body_t *body,
                                        invoke_t invoke, qtk_image_desc_t *desc,
                                        resize_t resize);

#ifdef __cplusplus
};
#endif
#endif /* F8BFF5B6_D380_430B_A485_CF4BA2CC345C */
