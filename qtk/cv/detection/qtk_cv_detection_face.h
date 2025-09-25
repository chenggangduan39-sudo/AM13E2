#ifndef F5D91FF4_446B_4232_8562_A274F64B8170
#define F5D91FF4_446B_4232_8562_A274F64B8170
#pragma message(                                                               \
    __FILE__ " deprecated: use qtk/cv/detection/qtk_cv_detection.h instead")
#ifdef __cplusplus
extern "C" {
#endif

#include "qtk_cv_detection_onnx.h"

/*人脸检测结构体*/
typedef struct qtk_cv_detection_face {
    float *input;
    float svd[3];               //图像的均值 127.5
    float std[3];               //图像的方差 128
    int64_t sv_shape[4];        //模型输入形状 (1,3,544,960) BGR
    qtk_image_desc_t face_desc; //模型的输入形状 width:960  height:544
    qtk_image_desc_t img_desc;  //图片的输入形状
    float iou;                  //筛选得分
    float conf;                 //重叠得分
    int anchors_num;            //重复检测次数 2
    int len;                    //单次检测长度 15
    int steps_num;              //检测数量 3
    qtk_cv_detection_onnx_box_t shape;
    qtk_cv_detection_onnx_box_t res; //结果
    qtk_cv_detection_onnxruntime_t *invo;
    uint8_t *data0;
    uint8_t *data1;
} qtk_cv_detection_face_t;

//人脸检测结构体指针内存申请
qtk_cv_detection_face_t *qtk_cv_detection_face_new();

//人脸检测结构体指针内存释放 face：人脸检测结构体指针
void qtk_cv_detection_face_delete(qtk_cv_detection_face_t *face);

//人脸检测 face：人脸检测结构体指针  imagedata:图像数据
void qtk_cv_detection_face_process(qtk_cv_detection_face_t *face,
                                   uint8_t *imagedata, invoke_t invoke);

//人脸检测通用接口 imagedata:图像数据(不超过4K) resize:缩放接口  invoke:推理接口
void qtk_cv_detection_face_process_data(qtk_cv_detection_face_t *face,
                                        uint8_t *imagedata,
                                        qtk_image_desc_t *desc, resize_t resize,
                                        invoke_t invoke);

#ifdef __cplusplus
};
#endif
#endif /* F5D91FF4_446B_4232_8562_A274F64B8170 */
