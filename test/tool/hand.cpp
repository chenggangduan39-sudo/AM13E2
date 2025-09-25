
#include <istream>
#include <opencv2/core.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <stdint.h>
#include "qtk/cv/detection/qtk_cv_detection_body.h"
#include "qtk/cv/detection/qtk_cv_detection_hand.h"
using namespace cv;
using namespace std;
#include <string>
#include <iostream>
#include "qtk/qtk_ipu.h"

std::string qtk_cv_detection_hand_show3(int t) {
    switch (t) {
    case 0:
        return "bg";
    case 1:
        return "fist";
    case 2:
        return "like";
    case 3:
        return "neg_hand";
    case 4:
        return "ok";
    case 5:
        return "palm";
    case 6:
        return "peace";
    default:
        return "";
    }
}
void data_process(const Mat &src,uint8_t *data,int width,int height,int channel){
    if (src.isContinuous()){
        memcpy(data,src.ptr(),src.total()*src.elemSize());
    }
    else {
        for (int i=0;i<height;i++)
            memcpy(data+i*width*channel,src.ptr()+i*src.step,src.elemSize()*src.cols);
    }
}

void resize_process(uint8_t *src,qtk_image_desc_t *src_desc,uint8_t *dst,qtk_image_desc_t *dst_desc){
    src_desc->fmt=QBL_IMAGE_RGB24;
    qtk_image_resize(src_desc,src,dst_desc->height,dst_desc->width,dst);
}

void nms(qtk_cv_detection_onnx_box_t *shape){
    float varea[1000] = {0};
    for (int i = 0; i < shape->cnt; i++) {
        varea[i] = (shape->box[i].roi.x2 - shape->box[i].roi.x1 + 1) *
                   (shape->box[i].roi.y2 - shape->box[i].roi.y1 + 1);
    }
    uint8_t isSuppressed[1000] = {0};
    for (int i = 0; i < shape->cnt; i++) {
        if (isSuppressed[i])
            continue;
        for (int j = i + 1; j < shape->cnt; j++) {
            if (isSuppressed[j])
                continue;
            float x1 = max(shape->box[i].roi.x1, shape->box[j].roi.x1);
            float y1 = max(shape->box[i].roi.y1, shape->box[j].roi.y1);
            float x2 = min(shape->box[i].roi.x2, shape->box[j].roi.x2);
            float y2 = min(shape->box[i].roi.y2, shape->box[j].roi.y2);
            float w = max((float)0, x2 - x1 + 1);
            float h = max((float)0, y2 - y1 + 1);
            float inter = w * h;
            float ovr = inter / (varea[i] + varea[j] - inter);
            if (ovr >= 0.3)
                isSuppressed[j] = 1;
        }
    }
    qtk_cv_detection_onnx_box_t res;
    int num=0;
    for (int i = 0; i < shape->cnt; i++) {
        if (isSuppressed[i])
            continue;
        res.box[num].roi.x1 = shape->box[i].roi.x1;
        res.box[num].roi.y1 = shape->box[i].roi.y1;
        res.box[num].roi.x2 = shape->box[i].roi.x2;
        res.box[num].roi.y2 = shape->box[i].roi.y2;
        res.box[num].score =  shape->box[i].score;
        num++;
    }
    for (int i=0;i<num;i++){
        shape->box[i].roi.x1=res.box[i].roi.x1;
        shape->box[i].roi.x2=res.box[i].roi.x2;
        shape->box[i].roi.y1=res.box[i].roi.y1;
        shape->box[i].roi.y2=res.box[i].roi.y2;
    }
    shape->cnt=num;
}

static void print(){
    cout<<"error! please enter:"<<endl;
    cout<<"                     -b  body model"<<endl;
    cout<<"                     -h  hand model"<<endl;
    cout<<"                     -g  gesture model"<<endl;
    cout<<"                     -i  image"<<endl;
    cout<<"selectable :"<<endl;
    cout<<"                     -o  output image"<<endl;
}

int main(int argc , char *argv[]){

    wtk_arg_t *arg=nullptr; 
    char *body_model=nullptr;
    char *hand_model=nullptr;
    char *ges_model=nullptr;
    char *image=nullptr;
    char *out=nullptr;
    char *ipu=nullptr;
    // char *ii=nullptr;

    arg=wtk_arg_new(argc,argv);
    wtk_arg_get_str_s(arg,"b",&body_model);
    wtk_arg_get_str_s(arg,"o",&out);
    wtk_arg_get_str_s(arg,"h",&hand_model);
    wtk_arg_get_str_s(arg,"g",&ges_model);
    wtk_arg_get_str_s(arg,"i",&image);
    wtk_arg_get_str_s(arg,"p",&ipu);

    if (!body_model && !hand_model && !ges_model && !image){
        wtk_arg_delete(arg);
        print();
        return 0;
    }

    qtk_cv_detection_body_t *body=qtk_cv_detection_body_new();
    qtk_cv_detection_hand_t *hand=qtk_cv_detection_hand_new();
    qtk_cv_detection_hand_gesture_t *ges=qtk_cv_detection_hand_gesture_new();
    body->iou=0.45;
    body->conf=0.2;
    hand->iou=0.4;
    hand->conf=0.3;
    
#if 1  //pc测试

    body->invo=qtk_cv_detection_onnxruntime_new(body_model);
    hand->invo=qtk_cv_detection_onnxruntime_new(hand_model);
    ges->invo=qtk_cv_detection_onnxruntime_new(ges_model);

    VideoCapture cap(image);  // 打开视频文件
    if (!cap.isOpened()) {
        cerr << "Error opening video file." << endl;
        return -1;
    }

    int width = 1920;
    int height = 1080;
    int fps = 30;

    cv::VideoWriter writer(out, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), fps, cv::Size(width, height));
    Mat frame;

    while(cap.read(frame)){
        if (frame.empty()) return 0;
        static int num = 0;
        if (++num % 3)
            continue;
        num = 0;
        qtk_image_desc_t desc;
        desc.width=frame.cols;
        desc.height=frame.rows;
        desc.channel=frame.channels();
        uint8_t *imagedata= new uint8_t[desc.width*desc.height*desc.channel*2];
        data_process(frame,imagedata,desc.width,desc.height,desc.channel);
        qtk_cv_detection_body_process_data(imagedata,body,qtk_cv_detection_onnxruntime_invoke_single,&desc,resize_process);
        qtk_cv_detection_onnx_box_square(&body->res,desc);
        for (int i=0;i<body->res.cnt;i++){
            rectangle(frame,
                    Point((int)body->res.box[i].roi.x1,
                            (int)body->res.box[i].roi.y1),
                    Point((int)body->res.box[i].roi.x2,
                            (int)body->res.box[i].roi.y2),
                    Scalar(255, 0, 0), 2, 0, 0);
        }
        qtk_cv_detection_body_hand_process(body,hand,imagedata,&desc,resize_process,qtk_cv_detection_onnxruntime_invoke_single);
        nms(&hand->res);
        for (int i=0;i<hand->res.cnt;i++){
            rectangle(frame,
                    Point((int)hand->res.box[i].roi.x1,
                            (int)hand->res.box[i].roi.y1),
                    Point((int)hand->res.box[i].roi.x2,
                            (int)hand->res.box[i].roi.y2),
                    Scalar(0, 255, 0), 2, 0, 0);
        }
        qtk_cv_detection_hand_gesture_process(hand,ges,imagedata,&desc,resize_process,qtk_cv_detection_onnxruntime_invoke_single);
        string aa[10];
        for (int i=0;i<hand->res.cnt;i++)
            aa[i] =  qtk_cv_detection_hand_show3(hand->res.box[i].classify);
        for (int i=0;i<hand->res.cnt;i++){
            putText(frame,
                    aa[i], 
                    Point((int)hand->res.box[i].roi.x1,
                            (int)hand->res.box[i].roi.y1 - 10),
                    FONT_HERSHEY_SIMPLEX,
                    0.9,
                    Scalar(0, 255, 0), 2);
        }        
        for (int i=0;i<hand->res.cnt;i++)
            qtk_cv_detection_hand_show(hand->res.box[i].classify);
        
        if (out){
            writer.write(frame);
        }

        imshow("test", frame);
        waitKey(5);

        delete []imagedata;
        imagedata=nullptr;
    }
    qtk_cv_detection_onnxruntime_delete(body->invo);
    qtk_cv_detection_onnxruntime_delete(hand->invo);
    qtk_cv_detection_onnxruntime_delete(ges->invo);
    qtk_cv_detection_body_detele(body);
    qtk_cv_detection_hand_delete(hand);
    qtk_cv_detection_hand_gesture_delete(ges);
    wtk_arg_delete(arg);
	return 0;


#else  //8838 9193测试
    char *acc[]={body_model,hand_model,ges_model};
    body->invo=qtk_cv_detection_onnxruntime_ipu_new(body_model);
    hand->invo=qtk_cv_detection_onnxruntime_ipu_new(hand_model);
    ges->invo=qtk_cv_detection_onnxruntime_ipu_new(ges_model);

    qtk_image_desc_t desc;
    desc.width=1920;
    desc.height=1080;
    desc.channel=3;  
    uint8_t *imagedata= new uint8_t[desc.width*desc.height*desc.channel];  
    imagedata = qtk_image_load(&desc,image);
    qtk_cv_detection_body_process_data(imagedata,body,qtk_cv_detection_onnxruntime_invoke_single_ipu,&desc,resize_process);
    qtk_cv_detection_onnx_box_square(&body->res,desc);
    for (int i=0;i<body->res.cnt;i++){
        qtk_cv_detection_body_res_hand_process(body->res.box[i],hand,imagedata,&desc,resize_process,qtk_cv_detection_onnxruntime_invoke_single_ipu);
        nms(&hand->res);
        qtk_cv_detection_hand_gesture_process(hand,ges,imagedata,&desc,resize_process,qtk_cv_detection_onnxruntime_invoke_single_ipu);
        for (int i=0;i<hand->res.cnt;i++){
            qtk_cv_detection_hand_show(hand->res.box[i].classify);
        }
    }

    delete []imagedata;
    imagedata=nullptr;
    qtk_cv_detection_onnxruntime_ipu_delete(body->invo);
    qtk_cv_detection_onnxruntime_ipu_delete(hand->invo);
    qtk_cv_detection_onnxruntime_ipu_delete(ges->invo);
    qtk_cv_detection_body_detele(body);
    qtk_cv_detection_hand_delete(hand);
    qtk_cv_detection_hand_gesture_delete(ges);
    wtk_arg_delete(arg);
	return 0;
#endif

}
