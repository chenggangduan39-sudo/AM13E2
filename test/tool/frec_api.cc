extern "C" {
#include "qtk/cv/api/qtk_cv_frec_api.h"
#include <stdio.h>

extern void* wtk_riff_new(void);
extern int wtk_riff_open(void *f, char *fn);
void wtk_riff_delete(void *f);
extern void* wtk_arg_new(int argc,char** argv);
extern int wtk_arg_delete(void *arg);
extern int wtk_arg_get_str(void *arg,const char *key,int bytes,char** pv);
extern int wtk_arg_get_float(void *arg,const char *key,int bytes,float* number);
#define wtk_arg_get_str_s(arg,k,pv) wtk_arg_get_str(arg,(char*)k,sizeof(k)-1,pv)
#define wtk_arg_get_float_s(arg,k,n) wtk_arg_get_float(arg,k,sizeof(k)-1,n)
extern char* file_read_buf(char* fn, int *n);

}

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

static void register_(void *frec, cv::Mat &frame) {
	qtk_cv_frec_api_box_t *bbox;
    char buf[1024];
    int nresult;

    //input username
    FILE *out = popen(
        "zenity  --title  \"register!\" --entry --text \"Enter your name\"",
        "r");
    int ret = fread(buf, 1, sizeof(buf), out);
    buf[ret - 1] = '\0';

    nresult = qtk_cv_frec_api_register(frec, frame.ptr(), buf, ret);
    if (nresult != 1) {
        printf("register face > 1\n");
        exit(-1);
    }

    //register rec
    nresult=qtk_cv_frec_api_rstValue(frec, &bbox, 1);
    printf("trec x1=%f y1=%f x2=%f y2=%f name=[%s]\n", bbox[0].x1, bbox[0].y1, bbox[0].x2, bbox[0].y2, bbox[0].name);
    cv::rectangle(frame, cv::Point(bbox[0].x1, bbox[0].y1),
                  cv::Point(bbox[0].x2, bbox[0].y2), cv::Scalar(100, 0, 0), 1, 0,
                  0);
    imshow("test", frame);
}


static void rec_(void *frec, cv::Mat &frame) {
    int nresult, i;
    qtk_cv_frec_api_box_t *box;
    nresult = qtk_cv_frec_api_rec(frec, frame.ptr());

    nresult = qtk_cv_frec_api_rstValue(frec, &box, 0);
    for (i = 0; i < nresult; i++) {
    	printf("rec[%d] x1=%f y1=%f x2=%f y2=%f\n", i, box[i].x1, box[i].y1, box[i].x2, box[i].y2);
        cv::rectangle(frame, cv::Point(box[i].x1, box[i].y1),
                      cv::Point(box[i].x2, box[i].y2), cv::Scalar(100, 0, 0), 2);

    }

    nresult = qtk_cv_frec_api_rstValue(frec, &box, 1);
    for (i = 0; i < nresult; i++) {
    	printf("trec x1=%f y1=%f x2=%f y2=%f name=[%s]\n", box[i].x1, box[i].y1, box[i].x2, box[i].y2, box[i].name);
        cv::Point p(box[i].x1 - 10, box[i].y1 - 10);
        std::string lab(box[i].name);
        cv::putText(frame, lab, p, cv::FONT_HERSHEY_SIMPLEX, 1.0,
                        cv::Scalar(255, 0, 0), 2);
    }
}

int main(int argc, char *argv[]) {
    void *arg;
    char *cfn;
    float thresh = 1;
    arg = wtk_arg_new(argc, argv);
    wtk_arg_get_str_s(arg, "c", &cfn);
    wtk_arg_get_float_s(arg, "t", &thresh);

    cv::VideoCapture cap;
    void *frec;


    frec = qtk_cv_frec_api_new(cfn, 0);
    qtk_cv_frec_api_setThresh(frec, thresh);
    cap.open(0);
    cv::Mat frame;
    cv::Mat resize_frame;
    int rec_flag = 0;
    char path[20];
    int no=0;

    while (1) {
        cap >> frame;
        cv::resize(frame, resize_frame,
                   cv::Size(960,
                            544));
        if (resize_frame.isContinuous()) {
            frame = resize_frame;
        } else {
            frame = resize_frame.clone();
        }

        //save img file
//        Mat img;
//        resize(frame, img, Size(960, 540), 0, 0, INTER_AREA);
        //Mat(method1)
//        sprintf(path, "./tmp_facerec/%d.jpg", no++);
//        printf("======path:[%s]\n", path);
//        imwrite(path, resize_frame);
        //lplImage (method2)
        //sprintf(path, "./tmpimg/%d.jpg", no++);
        //cvSaveImage(path, IplImage(img));

        int key = cv::waitKey(5);
        switch (key) {
        case ' ':
            rec_flag = 0;
            register_(frec, frame);
            break;
        case 'p':
            rec_flag = 1;
            break;
        case 'q':
            goto end;
        }
        if (rec_flag) {
            rec_(frec, frame);
        }
        imshow("test", frame);
    }

end:
	qtk_cv_frec_api_delete(frec);
    wtk_arg_delete(arg);
}
