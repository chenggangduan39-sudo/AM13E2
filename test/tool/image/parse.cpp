#include <iostream>
#include <fstream>
#include <opencv2/core.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <opencv4/opencv2/core/types.hpp>
#include <stdint.h>
#include <string>
#include "qtk/cv/qtk_cv_portrait_segmentation.h"

using namespace std;
using namespace cv;
void data_process(const Mat &src, uint8_t *data, int width, int height,int channel){
    if (src.isContinuous()) {
        memcpy(data, src.ptr(), src.total() * src.elemSize());
    } else {
        for (int i = 0; i < height; i++)
            memcpy(data + i * width * channel, src.ptr() + i * src.step,
                   src.elemSize() * src.cols);
    }
}

static void print(){
      cout<<"error!  ";
      cout<<"please enter:"<<endl;
      cout<<"                -b  Background picture;"<<endl;
      cout<<"                -m model_fn"<<endl;
      cout<<"selectable :"<<endl;
      cout<<"                -o  Export video mp4"<<endl;
      
}

int main(int argc ,char*argv[]){
    
    cv::VideoCapture cap(0);
    const int width=1280;
    const int height=720;
    if (!cap.isOpened()) //判断相机是否打开
    {
        std::cerr << "ERROR!!Unable to open camera\n";
        return 0;
    }

    wtk_arg_t *arg=nullptr; 
    char *model=nullptr;
    char *bg=nullptr;
    char *out=nullptr;    
    arg=wtk_arg_new(argc,argv);
    wtk_arg_get_str_s(arg,"b",&bg);
    wtk_arg_get_str_s(arg,"o",&out);
    wtk_arg_get_str_s(arg,"m",&model);
    cv::VideoWriter wir;
    if(!bg && !model){
         print();
         wtk_arg_delete(arg);
         return 0;
    }
    if (out){
       bool sm = wir.open(out, 0x7634706d, 25,
                       cv::Size(width, height));
       if (!sm) {
        cout << "wir error" << endl;
        wir.release();
        return 0;
       }
    }
    Mat bg_img=imread(bg);
    Mat bg_t;
    resize(bg_img,bg_t,Size(width,height));
    uint8_t *bg_data=new uint8_t[width*height*3];
    data_process(bg_t,bg_data,width,height,3);
    uint8_t *data=new uint8_t[width*height*3];
    qtk_cv_portrait_segmentation_t *pa=qtk_cv_portrait_segmentation_new();
    pa->img_desc.width=width;
    pa->img_desc.height=height;
    pa->invo=qtk_cv_detection_onnxruntime_new(model);
    Mat frame;
    while(1){
       cap >> frame;
       if (frame.empty()) break;
       Mat img;
       resize(frame,img,Size(width,height));
       data_process(img,data,pa->img_desc.width,pa->img_desc.height,3);
       qtk_cv_portrait_segmentation_process(pa,data,qtk_cv_detection_onnxruntime_invoke_single,bg_data);
       Mat cp=Mat(height,width,CV_8UC3,pa->data);
       if (out) wir << cp;
       imshow("res",cp);
       int key = waitKey(30);
       if (key ==  int('q'))  break;
    }

    qtk_cv_detection_onnxruntime_delete(pa->invo);
    qtk_cv_portrait_segmentation_delete(pa);
    delete []data;
    delete []bg_data;
    cap.release();
    wir.release();
    wtk_arg_delete(arg);
    return 0;
}