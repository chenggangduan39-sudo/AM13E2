#ifndef B6C39689_8B96_4232_B242_1E3936C04A49
#define B6C39689_8B96_4232_B242_1E3936C04A49

#ifdef __cplusplus
extern "C" {
#endif

#include "qtk_cv_detection_body.h"
// #include "cstring"
// #include <string>
/*手部检测结构体*/
typedef struct qtk_cv_detection_hand {
    float *input;  // onnx输入结果
    float svd[3];  //图像的均值 BGR   0
    float std[3];  //图像的方差 BGR   255
    int64_t sv_shape[4]; //图像的形状 或为onnx的输入形状  （1，3，512，512）
    qtk_image_desc_t hand_desc; // onnx的输入形状 （width：512 height：512）
    qtk_image_desc_t img_desc;         //图像的形状
    float iou;                         //筛选概率
    float conf;                        //重叠概率
    int num_stages;                    // onnx的输出步长   5376
    qtk_cv_detection_onnx_box_t shape; //初次筛选结果
    qtk_cv_detection_onnx_box_t res;   //最终结果
    qtk_cv_detection_onnxruntime_t *invo;
    uint8_t *data0;
    uint8_t *data1;
    uint8_t *data2;
} qtk_cv_detection_hand_t;

/*手势检测结构体*/
typedef struct qtk_cv_detection_hand_gesture {
    qtk_image_desc_t desc; //图像的形状  （width：128 height：128）
    qtk_image_desc_t img_desc;
    float *input;                  // onnx的输入
    float svd[3];                  //图像的均值 BGR   0
    float std[3];                  //图像的方差 BGR   255
    int64_t sv_shape[4]; // onnx的输入形状  （1，3，128，128）
    int max; //手势结果类别   可在qtk_cv_detection_hand_show()中显示类别
    qtk_cv_detection_onnxruntime_t *invo;
    uint8_t *data0;
    uint8_t *data1;
    uint8_t *data2;
} qtk_cv_detection_hand_gesture_t;

//申请手部结构体相关内存
qtk_cv_detection_hand_t *qtk_cv_detection_hand_new();

//释放手部结构体相关内存  hand为手部结构体指针
void qtk_cv_detection_hand_delete(qtk_cv_detection_hand_t *hand);

//手势类别打印 t为手势检测结果
void qtk_cv_detection_hand_show(int t);

// string qtk_cv_detection_hand_show2(int t);

// std::string qtk_cv_detection_hand_show3(int t);

//手部检测 hand为手部检测结构体指针，imagedata为图像数据（512*512*3）RGB数据格式
void qtk_cv_detection_hand_process(qtk_cv_detection_hand_t *hand,
                                   uint8_t *imagedata, invoke_t invoke);

//手势检测结构体内存申请
qtk_cv_detection_hand_gesture_t *qtk_cv_detection_hand_gesture_new();

//手势检测结构体内存释放  ges为手势检测结构体指针
void qtk_cv_detection_hand_gesture_delete(qtk_cv_detection_hand_gesture_t *ges);

//手势检测  ges为手势检测结构体指针 imagedata为图像数据（128*128*3）BGR数据格式
void qtk_cv_detection_gesture_process(qtk_cv_detection_hand_gesture_t *ges,
                                      uint8_t *imagedata, invoke_t invoke);

//依赖人形结果的手部检测 imagedata:原图 desc;原图的信息  resize:缩放接口
//invoke:推理接口
void qtk_cv_detection_body_hand_process(qtk_cv_detection_body_t *body,
                                        qtk_cv_detection_hand_t *hand,
                                        uint8_t *imagedata,
                                        qtk_image_desc_t *desc, resize_t resize,
                                        invoke_t invoke);

//依赖手部结果的手势检测 imagedata:原图 desc:原图的信息  resize:缩放接口
//invoke;推理接口
void qtk_cv_detection_hand_gesture_process(qtk_cv_detection_hand_t *hand,
                                           qtk_cv_detection_hand_gesture_t *ges,
                                           uint8_t *imagedata,
                                           qtk_image_desc_t *desc,
                                           resize_t resize, invoke_t invoke);

//依赖手部结果的手势检测 box:人体检测结果 imagedata:原图 desc:原图的信息  resize:缩放接口
//invoke;推理接口

void qtk_cv_detection_body_res_hand_process(qtk_cv_detection_res_t box,
                                        qtk_cv_detection_hand_t *hand,
                                        uint8_t *imagedata,
                                        qtk_image_desc_t *desc, resize_t resize,
                                        invoke_t invoke);

#ifdef __cplusplus
};
#endif
#endif /* B6C39689_8B96_4232_B242_1E3936C04A49 */
