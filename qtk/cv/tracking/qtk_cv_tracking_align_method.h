#ifndef E813481B_E3AF_4900_AE21_015232B0EC7E
#define E813481B_E3AF_4900_AE21_015232B0EC7E
#ifdef __cplusplus
extern "C" {
#endif
/*说明：此版本适用于opencv4.7.0,如果编译报错或者存在未定义标识符 “Point2d”
       请在相应的.cpp文件注释掉   “typedef cv::Point_<double>Point2d”
       或者更新opencv*/
#include "qtk/cv/detection/qtk_cv_detection_onnx.h"

//人脸矫正 src:原图 dst:输出图像 tform:变换矩阵  dst_desc:输出图像的信息
//src_desc:原图的信息
void qtk_cv_tracking_align_method_warpaffine(uint8_t *src, uint8_t *dst,
                                             float *tform,
                                             qtk_image_desc_t *dst_desc,
                                             qtk_image_desc_t *src_desc);

//图像翻转  imagedata:原图 desc:原图信息 data:输出图像
void qtk_cv_tracking_align_method_flip(uint8_t *imagedata,
                                       qtk_image_desc_t *desc, uint8_t *data);

//人脸角度判定 imagedata:原图 desc:原图信息
//drop:人脸关键点坐标(前五个存x，后五个存y) box:人脸框坐标 res:输出结果(角度)
void qtk_cv_tracking_align_method_facial_angle(uint8_t *imagedata,
                                               qtk_image_desc_t *desc,
                                               float *drop, qtk_cv_bbox_t *box,
                                               double *res);

double qtk_cv_tracking_align_method_brightness(uint8_t *imagedata,
                                               qtk_image_desc_t *img_desc);

#ifdef __cplusplus
};
#endif
#endif /* E813481B_E3AF_4900_AE21_015232B0EC7E */
