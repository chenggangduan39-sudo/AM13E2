#include <istream>
#include <opencv2/core.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <stdint.h>
#include "qtk/cv/api/qtk_cv_detection_api.h"

using namespace cv;
using namespace std;

extern "C" {
extern void* wtk_arg_new(int argc,char** argv);
extern int wtk_arg_get_str(void *arg,const char *key,int bytes,char** pv);
extern int wtk_arg_get_int(void *arg,const char *key,int bytes,int* number);
#define wtk_arg_get_str_s(arg,k,pv) wtk_arg_get_str(arg,(char*)k,sizeof(k)-1,pv)
#define wtk_arg_get_int_s(arg,k,n) wtk_arg_get_int(arg,k,sizeof(k)-1,n)
};

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

static void Usage(int argc, char *argv[])
{
	cout << "Usage:" << endl;
	cout << "\tcvdetection_api -i input.mp4 -c cfg_fn [-o out.mp4] -t type\n" << endl;
}

int main(int argc, char *argv[])
{
    void *arg=nullptr;
    char *cfn=nullptr;
    char *mp4=nullptr;
    char *out=nullptr;    
    int type=-1;
    arg= wtk_arg_new(argc,argv);
    wtk_arg_get_str_s(arg,"o",&out);
    wtk_arg_get_str_s(arg,"c",&cfn);
    wtk_arg_get_str_s(arg,"i",&mp4);
    wtk_arg_get_int_s(arg,"t",&type);


    if (type < 0 || type > 5 || nullptr == cfn){
    	cout << "don't point cfn or type" << endl;
    	Usage(argc, argv);
    	return 0;
    }

    if (nullptr==mp4){
    	cout << "don't find input file, See Usage" << endl;
    	Usage(argc, argv);
    	return 0;
    }

    cv::VideoCapture cap;
    cap.open(mp4);
    if (!cap.isOpened()) {
        cout << "video file error" << endl;
        return 0;
    }
 
    const int width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
    const int height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
    const int totalFrames = cap.get(cv::CAP_PROP_FRAME_COUNT);
    const int frameRate = cap.get(cv::CAP_PROP_FPS);

    std::cout << "=========================="<< std::endl;
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
    std::cout << "=========================="<< std::endl;

    cv::VideoWriter wir;
    if (nullptr == out)
    {
    	out = "./out.mp4";
    }
    bool sm = wir.open(out, 0x7634706d, frameRate,
                       cv::Size(width, height));
    if (!sm) {
        cout << "wir error" << endl;
        cap.release();
        wir.release();
        return 0;
    }
    cv::Mat frame;
    void *detect;

    qtk_cv_detection_api_box_t *box;

    std::cout << "===>1. build detect engine..."<< std::endl;
    detect = qtk_cv_detection_api_new(cfn, (qtk_cv_detection_api_type_t)type);
    std::cout << "===>2. set param..."<< std::endl;
    switch(type){
    case QTK_CV_DETECTION_HEAD:
    case QTK_CV_DETECTION_FACE:
    	qtk_cv_detection_api_setProb(detect, 0.45, 0.3);
    	qtk_cv_detection_api_setAttrs(detect, 960, 540, 0);
    	break;
    }

    uint8_t *data1 = (uint8_t *)malloc(960 * 544 * 3);
    const int diff = 2 * 960 * 3;
    std::cout << "===>feed data..."<< std::endl;
//    char path[20];
//    int no=0;
    while (1) {
        cap >> frame;
        if (frame.empty())
            break;
        static int num = 0;
        if (++num % 3)
            continue;
        num = 0;

        //input and output prepare
        memset(data1, 127, 960 * 544 * 3);
        Mat img;
        resize(frame, img, Size(960, 540), 0, 0, INTER_AREA);
        //save img file
        //Mat
//        sprintf(path, "./tmpimg/%d.jpg", no++);
//        printf("======path:[%s]\n", path);
//        imwrite(path, img);
        //lplImage
        //sprintf(path, "./tmpimg/%d.jpg", no++);
        //cvSaveImage(path, IplImage(img));

        data_process(img, data1 + diff, 960, 540, 3);

        //3. feed data
        qtk_cv_detection_api_process(detect, data1);

        //4. obtain result data, next to do (point or other action)
        double w_dio = (double)width / (double)960;
        double h_dio = (double)height / (double)540;
        box=qtk_cv_detection_api_rstValue(detect);
        for (int i = 0; i < qtk_cv_detection_api_rstCount(detect); i++){
        	printf("=========(%d, %d) (%d, %d)[%d]\n",
        			(int)(box[i].x1 ),
                    (int)(box[i].y1 ),
					(int)(box[i].x2 ),
					(int)(box[i].y2 ),
					box[i].no);
            rectangle(frame,
                      Point((int)(box[i].x1 * w_dio),
                            (int)(box[i].y1 * h_dio)),
                      Point((int)(box[i].x2 * w_dio),
                            (int)(box[i].y2 * h_dio)),
                      Scalar(100, 0, 0), 1, 0, 0);
            cv::Point p(box[i].x1 * w_dio - 10, box[i].y1* h_dio - 10);
            std::string lab=std::to_string(box[i].no);
            cv::putText(frame, lab, p, cv::FONT_HERSHEY_SIMPLEX, 1.0,
                            cv::Scalar(255, 0, 0), 2);
        }

        wir << frame;
        imshow("Show", frame);
        waitKey(5);
        img.release();
    }
    cap.release();
    wir.release();
    //5. destroy engine
    std::cout << "===>end and destroy engine..."<< std::endl;
    qtk_cv_detection_api_delete(detect);

    free(data1);
    return 0;
}
