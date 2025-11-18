

#include <istream>
#include <opencv2/core.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <stdint.h>

#include "qtk/cv/detection/qtk_cv_detection_face.h"
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

int main(int argc, char *argv[]) {
    
    wtk_arg_t *arg=nullptr; 
    char *face_model=nullptr;
    char *mp4=nullptr;
    char *out=nullptr;    
    arg=wtk_arg_new(argc,argv);
    wtk_arg_get_str_s(arg,"o",&out);
    wtk_arg_get_str_s(arg,"h",&face_model);
    wtk_arg_get_str_s(arg,"i",&mp4);
    
    
    cv::VideoCapture cap;
    cap.open(mp4);
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
    bool sm = wir.open(out, 0x7634706d, frameRate,
                       cv::Size(width, height));
    if (!sm) {
        cout << "wir error" << endl;
        cap.release();
        wir.release();
        return 0;
    }
    cv::Mat frame;
    // qtk_cv_detection_head_t *face = qtk_cv_detection_face_new();
    qtk_cv_detection_face_t *face = qtk_cv_detection_face_new();
    face->iou = 0.45;
    face->conf = 0.3;
    face->img_desc.width = 960;
    face->img_desc.height = 540;

    const char *model = face_model;
    face->invo =
        qtk_cv_detection_onnxruntime_new(const_cast<char *>(model));
    
    uint8_t *data1 = (uint8_t *)malloc(960 * 544 * 3);
    const int diff = 2 * 960 * 3;
    int b=0;
    while (1) {
        cap >> frame;
        if (frame.empty())
            break;
        static int num = 0;
        if (++num % 3)
            continue;
        num = 0;
        memset(data1, 127, 960 * 544 * 3);
        Mat img;
        resize(frame, img, Size(960, 540), 0, 0, INTER_AREA);
        data_process(img, data1 + diff, 960, 540, 3);
        qtk_cv_detection_face_process(face, data1, qtk_cv_detection_onnxruntime_invoke_single);
        double w_dio = (double)width / (double)960;
        double h_dio = (double)height / (double)540;
        for (int i = 0; i < face->res.cnt; i++){                               
            rectangle(frame,
                      Point((int)(face->res.box[i].roi.x1 * w_dio),
                            (int)(face->res.box[i].roi.y1 * h_dio)),
                      Point((int)(face->res.box[i].roi.x2 * w_dio),
                            (int)(face->res.box[i].roi.y2 * h_dio)),
                      Scalar(100, 0, 0), 1, 0, 0);
        }
        wir << frame;
        imshow("stand", frame);
        waitKey(5);
        img.release();
    }
    cap.release();
    wir.release();
    qtk_cv_detection_onnxruntime_delete(face->invo);
    qtk_cv_detection_face_delete(face);

    free(data1);
    return 0;
}
