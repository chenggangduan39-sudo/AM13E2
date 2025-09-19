#ifndef E6C62F1A_9A6C_4A15_B233_80904EE048B0
#define E6C62F1A_9A6C_4A15_B233_80904EE048B0
#ifdef __cplusplus
extern "C" {
#endif

#include "qtk/cv/tracking/qtk_mot_sort.h"
#include "qtk_cv_detection_head.h"
#include "wtk/core/wtk_str_hash.h"

//检测序列长度
#define STAND_NUM 8

typedef struct qtk_cv_detection_stand {
    qtk_cv_detection_head_t *head;        //人头指针
    qtk_cv_detection_onnx_box_t head_res; //人头结果
    qtk_cv_detection_onnx_box_t body_res; //站立结果
    qtk_mot_sort_cfg_t cfg;
    qtk_mot_sort_t s;      //追踪指针
    float score_dio;       //检测阈值  >=1.0
    float h_score_dio;
    int stand_res;         //站立消息
    int down_res;          //坐下消息
    float w_dio;           //站立框显示放大倍数 x
    float h_dio;           //站立框显示放大倍数 y
    float front_x;         //画面中前面消除误检的阈值 0.1-0.2
    float mim_x;           //画面中间消除误检的阈值 0.2-0.3
    float heig_x;          //画面中间消除误检的阈值 0.3-0.35
    qtk_image_desc_t desc; //图像信息 960*544*3
    wtk_str_hash_t *tracklets;
    qtk_cv_bbox_t *sort_dets;
} qtk_cv_detection_stand_t;

//站立结构体内存申请
qtk_cv_detection_stand_t *qtk_cv_detection_stand_new();

//站立结构体内存释放
void qtk_cv_detection_stand_delete(qtk_cv_detection_stand_t *hk);

//站立检测接口 imagedata:图像数据(544*960*3)  invoke:推理接口
void qtk_cv_detection_stand_process(qtk_cv_detection_stand_t *hk,
                                    uint8_t *imagedata, invoke_t invoke);

#ifdef __cplusplus
};
#endif
#endif /* E6C62F1A_9A6C_4A15_B233_80904EE048B0 */
