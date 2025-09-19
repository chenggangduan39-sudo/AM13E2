extern "C" {
#include "qtk/cv/face_rec/qtk_cv_face_rec_embedding.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/core/wtk_arg.h"
#include <stdio.h>
}

#include <map>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

static std::map<std::string, float *> hub;

static void register_(qtk_cv_face_rec_embedding_t *em, cv::Mat &frame) {
    qtk_cv_bbox_t *bbox;
    char buf[1024];
    qtk_cv_face_rec_embedding_result_t *result;
    int nresult;
    nresult = qtk_cv_face_rec_embedding_process(em, frame.ptr(), &result);
    if (nresult != 1) {
        wtk_debug("register face > 1\n");
        exit(-1);
    }
    bbox = &result[0].box;
    cv::rectangle(frame, cv::Point(bbox->x1, bbox->y1),
                  cv::Point(bbox->x2, bbox->y2), cv::Scalar(100, 0, 0), 1, 0,
                  0);
    imshow("test", frame);
    float *_tmp = new float[result[0].embedding_dim];
    memcpy(_tmp, result[0].embedding, sizeof(float) * result[0].embedding_dim);
    FILE *out = popen(
        "zenity  --title  \"register!\" --entry --text \"Enter your name\"",
        "r");
    int ret = fread(buf, 1, sizeof(buf), out);
    buf[ret - 1] = '\0';
    if (auto search = hub.find(buf); search != hub.end()) {
        float *embedding = search->second;
        for (int i = 0; i < result[0].embedding_dim; i++) {
            embedding[i] = (embedding[i] + result[0].embedding[i]) / 2;
        }
    } else {
        hub.emplace(buf, _tmp);
    }
}

static double calc_dist_(float *a, float *b, int len) {
    double res = 0;
    for (int i = 0; i < len; i++) {
        res += (a[i] - b[i]) * (a[i] - b[i]);
    }
    return res;
}

static void rec_(qtk_cv_face_rec_embedding_t *em, cv::Mat &frame,
                 float thresh) {
    qtk_cv_face_rec_embedding_result_t *result;
    int nresult;
    std::string lab;
    nresult = qtk_cv_face_rec_embedding_process(em, frame.ptr(), &result);
    for (int i = 0; i < nresult; i++) {
        float min_dist = FLT_MAX;
        qtk_cv_bbox_t *bbox = &result[i].box;
        for (auto const &[key, val] : hub) {
            double dist =
                calc_dist_(result[i].embedding, val, result[i].embedding_dim);
            if (dist < min_dist) {
                min_dist = dist;
                lab = key;
            }
        }
        cv::rectangle(frame, cv::Point(bbox->x1, bbox->y1),
                      cv::Point(bbox->x2, bbox->y2), cv::Scalar(100, 0, 0), 2);
        if (min_dist < thresh) {
            cv::Point p(result[i].box.x1 - 10, result[i].box.y1 - 10);
            cv::putText(frame, lab, p, cv::FONT_HERSHEY_SIMPLEX, 1.0,
                        cv::Scalar(255, 0, 0), 2);
        }
    }
}

int main(int argc, char *argv[]) {
    wtk_arg_t *arg;
    char *cfn;
    int use_bin = 0;
    float thresh = 1;
    wtk_main_cfg_t *main_cfg = NULL;
    wtk_mbin_cfg_t *mbin_cfg = NULL;
    arg = wtk_arg_new(argc, argv);
    wtk_arg_get_str_s(arg, "c", &cfn);
    wtk_arg_get_float_s(arg, "t", &thresh);
    wtk_arg_get_int_s(arg, "b", &use_bin);

    cv::VideoCapture cap;
    qtk_cv_face_rec_embedding_t *em;

    if (use_bin) {
        mbin_cfg = wtk_mbin_cfg_new_type(qtk_cv_face_rec_embedding_cfg, cfn,
                                         (char *)"./main.cfg");
        em = qtk_cv_face_rec_embedding_new(
            (qtk_cv_face_rec_embedding_cfg_t *)mbin_cfg->cfg);
    } else {
        main_cfg = wtk_main_cfg_new_type(qtk_cv_face_rec_embedding_cfg, cfn);
        em = qtk_cv_face_rec_embedding_new(
            (qtk_cv_face_rec_embedding_cfg_t *)main_cfg->cfg);
    }
    cap.open(0);
    cv::Mat frame;
    cv::Mat resize_frame;
    int rec_flag = 0;

    while (1) {
        cap >> frame;
        cv::resize(frame, resize_frame,
                   cv::Size(em->cfg->face_detection.width,
                            em->cfg->face_detection.height));
        if (resize_frame.isContinuous()) {
            frame = resize_frame;
        } else {
            frame = resize_frame.clone();
        }
        int key = cv::waitKey(5);
        switch (key) {
        case ' ':
            rec_flag = 0;
            register_(em, frame);
            break;
        case 'p':
            rec_flag = 1;
            break;
        case 'q':
            goto end;
        }
        if (rec_flag) {
            rec_(em, frame, thresh);
        }
        imshow("test", frame);
    }

    for (auto const &[_, val] : hub) {
        delete[](val);
    }

end:
    qtk_cv_face_rec_embedding_delete(em);
    if (main_cfg) {
        wtk_main_cfg_delete(main_cfg);
    }
    if (mbin_cfg) {
        wtk_mbin_cfg_delete(mbin_cfg);
    }
    wtk_arg_delete(arg);
}
