#include <istream>
#include <opencv2/core.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <stdint.h>
#include "qtk/cv/detection/qtk_cv_detection_stand.h"
#include "wtk/asr/fextra/onnxruntime/qtk_onnxruntime.h"
#include "wtk/asr/fextra/onnxruntime/qtk_onnxruntime_cfg.h"


using namespace cv;
using namespace std;

void data_process(const Mat &src, uint8_t *data, int width, int height,
                  int channel) {
    if (src.isContinuous()) {
        memcpy(data, src.ptr(), src.total() * src.elemSize());
    } else {
        for (int i = 0; i < height; i++)
            memcpy(data + i * width * channel, src.ptr() + i * src.step,
                   src.elemSize() * src.cols);
    }
}

static void print(){
    cout<<"error! please enter:"<<endl;
    cout<<"                     -m  head model"<<endl;
    cout<<"                     -v  input video"<<endl;
    cout<<"selectable :"<<endl;
    cout<<"                     -o  output video mp4"<<endl;
}

int main(int argc, char *argv[]) {

    wtk_arg_t *arg=nullptr;
    char *head_model=nullptr;
    char *input=nullptr;
    char *out=nullptr;
    arg=wtk_arg_new(argc,argv);
    wtk_arg_get_str_s(arg,"m",&head_model);
    wtk_arg_get_str_s(arg,"v",&input);
    wtk_arg_get_str_s(arg,"o",&out);
    if (!head_model && !input){
        wtk_arg_delete(arg);
        print();
        return 0;
    }

    cv::VideoCapture cap;
    cap.open(input);
    if (!cap.isOpened()) {
        cout << "cap error" << endl;
        return 0;
    }
    const int width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
    const int height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
    const int totalFrames = cap.get(cv::CAP_PROP_FRAME_COUNT);
    const int frameRate = cap.get(cv::CAP_PROP_FPS);
    std::cout << "视频宽度： " << width << std::endl;
    if (width <= 0) {
        std::cout << "视频宽度错误 " << std::endl;
        return 0;
    }
    std::cout << "视频高度： " << height << std::endl;
    if (height <= 0) {
        std::cout << "视频高度错误 " << std::endl;
        return 0;
    }
    std::cout << "视频总帧数： " << totalFrames << std::endl;
    if (totalFrames <= 0) {
        std::cout << "视频总帧数错误 " << std::endl;
        return 0;
    }
    std::cout << "帧率： " << frameRate << std::endl;
    if (frameRate <= 0) {
        std::cout << "视频帧率错误 " << std::endl;
        return 0;
    }
    cv::VideoWriter wir;
    if (out){
        bool sm = wir.open(out, 0x7634706d, frameRate,
                       cv::Size(width, height));
        if (!sm) {
            cout << "wir error" << endl;
            cap.release();
            wir.release();
            return 0;
        }
    }
    
    cv::Mat frame;
    qtk_cv_detection_stand_t *hk = qtk_cv_detection_stand_new();
    hk->score_dio = 1.2;
    hk->w_dio = 1.2;
    hk->h_dio = 1.2;
    hk->desc.width = width;
    hk->desc.height = height;
    hk->head->iou = 0.45;
    hk->head->conf = 0.3;
    hk->head->img_desc.width = 960;
    hk->head->img_desc.height = 544;
    hk->front_x = 0.15;
    hk->mim_x = 0.2;
    hk->heig_x = 0.4;
    hk->head->invo =
        qtk_cv_detection_onnxruntime_new(head_model);
    uint8_t *data1 = (uint8_t *)malloc(960 * 544 * 3);

    
    while (1) {
        cap >> frame;
        if (frame.empty())
            break;
        static int num = 0;
        if (++num % 3)
            continue;
        num = 0;
        Mat img;
        resize(frame, img, Size(960, 544), 0, 0, INTER_AREA);
        data_process(img, data1, 960, 544, 3);
        qtk_cv_detection_stand_process(
            hk, data1, qtk_cv_detection_onnxruntime_invoke_single);
        double w_dio = (double)width / (double)960;
        double h_dio = (double)height / (double)544;
        for (int i = 0; i < hk->head_res.cnt; i++)
            rectangle(frame,
                      Point((int)(hk->head_res.box[i].roi.x1 * w_dio),
                            (int)(hk->head_res.box[i].roi.y1 * h_dio)),
                      Point((int)(hk->head_res.box[i].roi.x2 * w_dio),
                            (int)(hk->head_res.box[i].roi.y2 * h_dio)),
                      Scalar(100, 0, 0), 1, 0, 0);
        for (int i = 0; i < hk->body_res.cnt; i++)
            rectangle(frame,
                      Point((int)(hk->body_res.box[i].roi.x1 * w_dio),
                            (int)(hk->body_res.box[i].roi.y1 * h_dio)),
                      Point((int)(hk->body_res.box[i].roi.x2 * w_dio),
                            (int)(hk->body_res.box[i].roi.y2 * h_dio)),
                      Scalar(127, 255, 127), 2, 0, 0);
        if (out)
            wir << frame;
        imshow("stand", frame);
        int key=waitKey(5);
        if (key ==  int('q'))  break;
    }
    cap.release();
    wir.release();
    qtk_cv_detection_onnxruntime_delete(hk->head->invo);
    qtk_cv_detection_stand_delete(hk);
    free(data1);
    wtk_arg_delete(arg);
    return 0;
}