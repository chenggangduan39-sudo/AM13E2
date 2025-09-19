#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <iostream>
#include "qtk/cv/detection/qtk_cv_detection_face.h"
#include "qtk/cv/tracking/qtk_cv_tracking_align.h"

using namespace cv;
using namespace std;

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

static void print(){
    cout<<"error! please enter:"<<endl;
    cout<<"                     -f  face model"<<endl;
    cout<<"                     -a  align model"<<endl;
    cout<<"                     -i  image"<<endl;
    cout<<"selectable :"<<endl;
    cout<<"                     -o  output image"<<endl;
}
int main(int argc,char *argv[]){

    wtk_arg_t *arg=nullptr;
    char *face_model=nullptr;
    char *align_model=nullptr;
    char *image=nullptr;
    char *out=nullptr;
    arg=wtk_arg_new(argc,argv);
    wtk_arg_get_str_s(arg,"f",&face_model);
    wtk_arg_get_str_s(arg,"a",&align_model);
    wtk_arg_get_str_s(arg,"i",&image);
    wtk_arg_get_str_s(arg,"o",&out);
    if (!face_model && !align_model && !image){
        wtk_arg_delete(arg);
        print();
        return 0;
    }

    qtk_cv_detection_face_t *fk=qtk_cv_detection_face_new();
    qtk_cv_tracking_align_t *lign=qtk_cv_tracking_align_new();
    fk->invo=qtk_cv_detection_onnxruntime_new(face_model);
    lign->invo=qtk_cv_detection_onnxruntime_new(align_model);
    
    fk->iou=0.5;
    fk->conf=0.3;

    lign->rec_thresh=1.24;

    Mat frame=imread(image);
    if (frame.empty()) return 0;
    qtk_image_desc_t desc;
    desc.width=frame.cols;
    desc.height=frame.rows;
    desc.channel=frame.channels();
    uint8_t *imagedata=new uint8_t[desc.width*desc.height*desc.channel];
    data_process(frame,imagedata,desc.width,desc.height,desc.channel);
    qtk_cv_detection_face_process_data(fk,imagedata,&desc,resize_process,qtk_cv_detection_onnxruntime_invoke_single);
    for (int i=0;i<fk->res.cnt;i++){
        lign->align.box[i].roi.x1=fk->res.box[i].roi.x1;
        lign->align.box[i].roi.y1=fk->res.box[i].roi.y1;
        lign->align.box[i].roi.x2=fk->res.box[i].roi.x2;
        lign->align.box[i].roi.y2=fk->res.box[i].roi.y2;
        memcpy(lign->align.key_points[i],fk->res.box[i].data,10*sizeof(float));
    }
    lign->align.cnt=fk->res.cnt;
    lign->align.img_desc.width=desc.width;
    lign->align.img_desc.height=desc.height;
    qtk_cv_tracking_align_process(lign,imagedata,qtk_cv_tracking_align_method_warpaffine,qtk_cv_tracking_align_method_flip
        ,qtk_cv_detection_onnxruntime_invoke_single);
    for(int i=0;i<fk->res.cnt;i++)
        rectangle(frame,Point((int)(fk->res.box[i].roi.x1),(int)(fk->res.box[i].roi.y1)),
                Point((int)(fk->res.box[i].roi.x2),(int)(fk->res.box[i].roi.y2)),Scalar(0,255,0),2,0,0);

    if (out)
        imwrite(out,frame);
    qtk_cv_detection_onnxruntime_delete(fk->invo);
    qtk_cv_detection_onnxruntime_delete(lign->invo);
    qtk_cv_detection_face_delete(fk);
    qtk_cv_tracking_align_delete(lign);
    delete []imagedata;
    wtk_arg_delete(arg);
}
