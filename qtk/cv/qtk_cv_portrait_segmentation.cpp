#include <istream>
#include <opencv2/core.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <stdint.h>
#include <string>
#include <vector>
#include <math.h>
#include <chrono>
#include "qtk_cv_portrait_segmentation.h"

using namespace std;
using namespace cv;

void qtk_cv_portrait_segmentation_post(Mat &img,uint8_t *dst,int rows,int cols){
    Mat dst_0=img.clone();
    dst_0*=255;
    Mat dst_t;
    threshold(dst_0,dst_t,240,1,THRESH_BINARY);
    Mat kernel_erode=getStructuringElement(MORPH_ELLIPSE, Size(5,5));
    Mat kernel_dilate=getStructuringElement(MORPH_ELLIPSE, Size(25,25));
    Mat mask_erode;
    erode(dst_t,mask_erode,kernel_erode);
    Mat mask_dilate;
    dilate(mask_erode,mask_dilate,kernel_dilate);
    img.dot(mask_dilate);
}

qtk_cv_portrait_segmentation_t *qtk_cv_portrait_segmentation_new(){
    qtk_cv_portrait_segmentation_t *pa= new qtk_cv_portrait_segmentation_t;
    /*参数修改*/
    for (int i=0;i<3;i++){
        pa->std[i]=127.5;
        pa->svd[i]=127.5;
    }
    pa->sv_shape[0]=1;
    pa->sv_shape[1]=3;
    pa->sv_shape[2]=144;//
    pa->sv_shape[3]=256;//
    pa->out_step=2*256*144;
    pa->body_desc.width=256;//修改
    pa->body_desc.height=144;//修改
    pa->body_desc.channel=3;
    pa->data_bk=0;
    pa->dk=new uint8_t[pa->sv_shape[2]*pa->sv_shape[3]];
    const int step=pa->sv_shape[0]*pa->sv_shape[1]*pa->sv_shape[2]*pa->sv_shape[3];
    pa->input=new float[step];
    pa->dst=nullptr;
    pa->fk_data = new uint8_t[step];
    pa->data=new uint8_t[3840*2160*3];
    return pa;
}

void qtk_cv_portrait_segmentation_delete(qtk_cv_portrait_segmentation_t *pa) {
    if (pa->input!=nullptr){
        delete []pa->input;
        pa->input=nullptr;
    }
    if (pa->fk_data!=nullptr){
        delete []pa->fk_data;
    }
    if (pa->dst!=nullptr){
        delete []pa->dst;
    }
    if (pa->dk!=nullptr){
        delete []pa->dk;
    }
    if (pa->data!=nullptr){
        delete []pa->data;
    }
    delete pa;
    pa=nullptr;
}

static void data_process(const Mat &src, uint8_t *data, int width, int height,int channel){
    if (src.isContinuous()) {
        memcpy(data, src.ptr(), src.total() * src.elemSize());
    } else {
        for (int i = 0; i < height; i++)
            memcpy(data + i * width * channel, src.ptr() + i * src.step,
                   src.elemSize() * src.cols);
    }
}

static void data_processf(const Mat &src, float *data, int width, int height,int channel){
    if (src.isContinuous()) {
        memcpy(data, src.ptr(), src.total() * src.elemSize());
    } else {
        for (int i = 0; i < height; i++)
            memcpy(data + i * width * channel, src.ptr() + i * src.step,
                   src.elemSize() * src.cols );
    }
}

static void portrait_segmentation_sxxcz2(const Mat &dst_0,qtk_image_desc_t *img_desc,int h,int w,float *dk){
    Mat dk_0;
    resize(dst_0,dk_0,Size(w,h));
    data_processf(dk_0,dk,w,h,1);
}

void qtk_cv_portrait_segmentation_process(qtk_cv_portrait_segmentation_t *pa,uint8_t *img,invoke_t invoke ,uint8_t *dt_img){
    float *output;
    if (pa->data_bk==0){
        pa->dst= new float[pa->img_desc.width*pa->img_desc.height];
        pa->data_bk=1;
    }


    //auto beforeTime = std::chrono::steady_clock::now();



    Mat dst_ph=Mat(pa->img_desc.height,pa->img_desc.width,CV_8UC3,dt_img);
    Mat dst_t=dst_ph.clone();      
    Mat mat_img=Mat(pa->img_desc.height, pa->img_desc.width, CV_8UC3, img);
    Mat mat_dst;
    resize(mat_img,mat_dst,Size(pa->body_desc.width,pa->body_desc.height));
    data_process(mat_dst,pa->fk_data,pa->body_desc.width,pa->body_desc.height,3);

    invoke(pa->input, &output, pa->fk_data, pa->svd, pa->std, pa->sv_shape,
           pa->invo, pa->out_step);
    const int k=pa->out_step>>1;
    Mat dst_0 =
        Mat(pa->body_desc.height, pa->body_desc.width, CV_32FC1, output + k);
    qtk_cv_portrait_segmentation_post(dst_0,pa->dk,pa->body_desc.height,pa->body_desc.width);
    Mat dst_1;
    portrait_segmentation_sxxcz2(dst_0,&pa->body_desc,pa->img_desc.height,pa->img_desc.width,pa->dst);
    const int step=pa->img_desc.width*pa->img_desc.height;
    if(dst_t.isContinuous()){
        for (int i=0;i<step;i++){
            if(pa->dst[i]<0.2) continue;
            else{
                int dst_num=i*3;
                dst_t.data[dst_num]=img[dst_num]*pa->dst[i];
                dst_t.data[dst_num+1]=img[dst_num+1]*pa->dst[i];
                dst_t.data[dst_num+2]=img[dst_num+2]*pa->dst[i];
            }
        }
    }
    else{
        for (int i=0;i<dst_t.rows;i++){
            uint8_t *p=(uint8_t*)dst_t.ptr()+i*dst_t.step;
            const int step=i*dst_t.cols;
            for (int j=0;j<dst_t.cols;j++){
                if(pa->dst[step+j]<0.2) continue;
                else{
                    int dst_num=j*3;
                    p[dst_num]=pa->dst[step+j]*img[step+dst_num];
                    p[dst_num+1]=pa->dst[step+j]*img[step+dst_num+1];
                    p[dst_num+2]=pa->dst[step+j]*img[step+dst_num+2];
                }
            }
        }
    }
    
    
    data_process(dst_t,pa->data,pa->img_desc.width,pa->img_desc.height,3);
    /*auto afterTime = std::chrono::steady_clock::now();
    double duration_millsecond = std::chrono::duration<double, std::milli>(afterTime - beforeTime).count();
	std::cout << duration_millsecond << "毫秒" << std::endl;*/
}
