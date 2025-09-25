#include <dirent.h>
#include <filesystem>
#include <iostream>
#include <opencv2/calib3d.hpp>
#include <opencv2/core.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/core/utility.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/mcc/checker_detector.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/core/matx.hpp>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include "qtk_cv_image.h"

extern "C" {

static double qtk_cv_image_definition_method_laplacian(uint8_t *data,
                                                       qtk_image_desc_t *desc) {
    cv::Mat image = cv::Mat(desc->height, desc->width, CV_8UC3, data);
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    cv::Mat dst, abs;
    cv::Laplacian(gray, dst, CV_64FC2);
    cv::Scalar mean_s;
    cv::Mat std;
    cv::meanStdDev(dst, mean_s, std);
    double x = std.ptr<double>(0)[0];
    return pow(x, 2);
}

static double qtk_cv_image_definition_method_brenner(uint8_t *data,
                                                     qtk_image_desc_t *desc) {
    cv::Mat image = cv::Mat(desc->height, desc->width, CV_8UC3, data);
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    double d = 0;
    uint8_t *p = (uint8_t *)gray.data;
    for (int x = 0; x < gray.rows - 2; x++) {
        int m0 = (x + 2) * gray.cols;
        int m1 = x * gray.cols;
        for (int y = 0; y < gray.cols; y++) {
            int k0 = m0 + y;
            int k1 = m1 + y;
            d += pow((p[k0] - p[k1]), 2);
        }
    }
    return d;
}

static double qtk_cv_image_definition_method_smd(uint8_t *data,
                                                 qtk_image_desc_t *desc) {
    cv::Mat image = cv::Mat(desc->height, desc->width, CV_8UC3, data);
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    double d = 0;
    uint8_t *p = (uint8_t *)gray.data;
    for (int x = 0; x < gray.rows - 1; x++) {
        int m0 = x * gray.cols;
        int m1 = (x + 1) * gray.cols;
        for (int y = 0; y < gray.cols; y++) {
            int k0 = m1 + y;
            int k1 = m0 + y;
            int k2 = m0 + y - 1;
            if (y - 1 < 0)
                k2 = m0 + gray.cols - 1;
            d += fabs(p[k1] - p[k2]);
            d += fabs(p[k1] - p[k0]);
        }
    }
    return d;
}

static double qtk_cv_image_definition_method_smd2(uint8_t *data,
                                                  qtk_image_desc_t *desc) {
    cv::Mat image = cv::Mat(desc->height, desc->width, CV_8UC3, data);
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    double d = 0;
    uint8_t *p = (uint8_t *)gray.data;
    for (int x = 0; x < gray.rows - 1; x++) {
        int m0 = x * gray.cols;
        int m1 = (x + 1) * gray.cols;
        for (int y = 0; y < gray.cols - 1; y++) {
            int k0 = m0 + y;
            int k1 = m1 + y;
            int k2 = m0 + y + 1;
            d += fabs(p[k0] - p[k1]) * fabs(p[k0] - p[k2]);
        }
    }
    return d;
}

static double qtk_cv_image_definition_method_variance(uint8_t *data,
                                                      qtk_image_desc_t *desc) {
    cv::Mat image = cv::Mat(desc->height, desc->width, CV_8UC3, data);
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    double d = 0;
    cv::Scalar mean1 = cv::mean(gray);
    uint8_t *p = (uint8_t *)gray.data;
    const double k = mean1[0];
    for (int x = 0; x < gray.rows; x++) {
        int m = x * gray.cols;
        for (int y = 0; y < gray.cols; y++) {
            int km = m + y;
            d += pow((p[km] - k), 2);
        }
    }
    return d;
}

static double qtk_cv_image_definition_method_energy(uint8_t *data,
                                                    qtk_image_desc_t *desc) {
    cv::Mat image = cv::Mat(desc->height, desc->width, CV_8UC3, data);
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    double d = 0;
    uint8_t *p = (uint8_t *)gray.data;
    for (int x = 0; x < gray.rows - 1; x++) {
        int m0 = x * gray.cols;
        int m1 = (x + 1) * gray.cols;
        for (int y = 0; y < gray.cols - 1; y++) {
            int k0 = m0 + y;
            int k1 = m1 + y;
            int k2 = m0 + y + 1;
            d += pow((p[k1] - p[k0]), 2) * pow((p[k2] - p[k0]), 2);
        }
    }
    return d;
}

static double qtk_cv_image_definition_method_vollath(uint8_t *data,
                                                     qtk_image_desc_t *desc) {
    cv::Mat image = cv::Mat(desc->height, desc->width, CV_8UC3, data);
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    cv::Scalar mean1 = cv::mean(gray);
    double d = -gray.cols * gray.rows * pow(mean1[0], 2);
    uint8_t *p = (uint8_t *)gray.data;
    for (int x = 0; x < gray.rows - 1; x++) {
        int m0 = x * gray.cols;
        int m1 = (x + 1) * gray.cols;
        for (int y = 0; y < gray.cols; y++) {
            d += p[m0 + y] * p[m1 + y];
        }
    }
    return d;
}

static double qtk_cv_image_definition_method_tenengrad(uint8_t *data,
                                                       qtk_image_desc_t *desc) {
    cv::Mat image = cv::Mat(desc->height, desc->width, CV_8UC3, data);
    cv::Mat img;
    cv::cvtColor(image, img, cv::COLOR_BGR2GRAY);
    double Grad_value = 0;
    double Sx, Sy;
    for (int i = 1; i < img.rows - 1; i++) {
        uint8_t *current_ptr = (uint8_t *)img.data + i * img.cols;
        uint8_t *pre_ptr = (uint8_t *)img.data + (i - 1) * img.cols;
        uint8_t *next_ptr = (uint8_t *)img.data + (i + 1) * img.cols;
        for (int j = 1; j < img.cols - 1; j++) {
            Sx = pre_ptr[j - 1] * (-1) + pre_ptr[j + 1] +
                 current_ptr[j - 1] * (-2) + current_ptr[j + 1] * 2 +
                 next_ptr[j - 1] * (-1) + next_ptr[j + 1];
            Sy = pre_ptr[j - 1] + 2 * pre_ptr[j] + pre_ptr[j + 1] -
                 next_ptr[j - 1] - 2 * next_ptr[j] - next_ptr[j + 1];
            Grad_value += Sx * Sx + Sy * Sy;
        }
    }
    return Grad_value / (img.cols - 2) / (img.rows - 2);
}

double qtk_cv_image_definition_method(uint8_t *data, qtk_image_desc_t *desc,
                                      qtk_im_evaluation tool) {
    double b = 0;
    switch (tool) {
    case TENENGRAD:
        b = qtk_cv_image_definition_method_tenengrad(data, desc);
        break;
    case LAPLACIAN:
        b = qtk_cv_image_definition_method_laplacian(data, desc);
        break;
    case SMD:
        b = qtk_cv_image_definition_method_smd(data, desc);
        break;
    case SMD2:
        b = qtk_cv_image_definition_method_smd2(data, desc);
        break;
    case BRENNER:
        b = qtk_cv_image_definition_method_brenner(data, desc);
        break;
    case VARIANCE:
        b = qtk_cv_image_definition_method_variance(data, desc);
        break;
    case ENERGY:
        b = qtk_cv_image_definition_method_energy(data, desc);
        break;
    case VOLLATH:
        b = qtk_cv_image_definition_method_vollath(data, desc);
        break;
    default:
        break;
    }
    return b;
}

double qtk_cv_image_ccdiff(const char *fn1, const char *fn2) {
    cv::Mat img1 = cv::imread(fn1);
    if (img1.empty()) {
        std::cout << "fn1 error!" << std::endl;
        return 0;
    }
    cv::Mat img2 = cv::imread(fn2);
    if (img2.empty()) {
        std::cout << "fn2 error!" << std::endl;
        return 0;
    }
    cv::Ptr<cv::mcc::CCheckerDetector> detector =
        cv::mcc::CCheckerDetector::create();
    
    if (!detector->process(img1, cv::mcc::MCC24, 1)) {
        std::cout << "ChartColor not detected" << std::endl;
        return 0;
    }
    cv::Ptr<cv::mcc::CChecker> res1 = detector->getBestColorChecker();
    cv::Mat cv1 = res1->getChartsRGB();
    cv::Mat i1 = cv1(cv::Range::all(), cv::Range(1, 2)).clone();
    if (!detector->process(img2, cv::mcc::MCC24, 1)) {
        std::cout << "ChartColor not detected" << std::endl;
        return 0;
    }
    cv::Ptr<cv::mcc::CChecker> res2 = detector->getBestColorChecker();
    cv::Mat cv2 = res2->getChartsRGB();
    cv::Mat i2 = cv2(cv::Range::all(), cv::Range(1, 2)).clone();
    cv::Mat dst;
    cv::absdiff(i1, i2, dst);
    cv::Mat powed;
    cv::multiply(dst, dst, powed);
    cv::Scalar t = cv::sum(powed);
    double k = t.val[0];
    return sqrt(k / dst.rows);
}

double qtk_cv_image_fov(const char *path) {
    if (path == NULL) {
        std::cout << "path error" << std::endl;
        return 0;
    }
    struct stat s = {};
    lstat(path, &s);
    if (!S_ISDIR(s.st_mode)) {
        std::cout << "dir_name is not a valid directory !" << std::endl;
        return 0;
    }
    cv::TermCriteria criteria = cv::TermCriteria(
        cv::TermCriteria::EPS + cv::TermCriteria::MAX_ITER, 30, 0.001);
    std::vector<cv::Point3f> objp(48);
    for (int i = 0; i < 48; i++) {
        objp[i].x = i % 8;
        objp[i].y = i / 8;
        objp[i].z = 0;
    }
    struct dirent *filename = NULL;
    DIR *dir = opendir(path);
    if (NULL == dir) {
        std::cout << "Can not open dir " << path << std::endl;
        return 0;
    }
    const char lll = '/';
    std::string y(&lll, 1);
    std::vector<std::vector<cv::Point3f>> objpoints;
    std::vector<std::vector<cv::Point2f>> imgpoints;
    cv::Mat gray, img;
    while ((filename = readdir(dir)) != NULL) {
        if (strcmp(filename->d_name, ".") == 0 ||
            strcmp(filename->d_name, "..") == 0)
            continue;
        std::string fn = path;
        char z = fn.at(fn.size() - 1);
        std::string x(&z, 1);
        if (x.compare(y)) {
            fn += y;
        }
        std::string fn_name = filename->d_name;
        fn_name = fn + fn_name;
        img = cv::imread(fn_name);
        std::vector<cv::Point2f> corners;
        cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
        if (cv::findChessboardCorners(gray, cv::Size(8, 6), corners)) {
            objpoints.push_back(objp);
            cv::cornerSubPix(gray, corners, cv::Size(11, 11), cv::Size(-1, -1),
                             criteria);
            imgpoints.push_back(corners);
        }
    }
    // cv::destroyAllWindows();
    closedir(dir);
    cv::Mat mtx;
    cv::Mat dist;
    std::vector<cv::Mat> rvecs, tvecs;
    cv::calibrateCamera(objpoints, imgpoints, cv::Size(img.cols, img.rows), mtx,
                        dist, rvecs, tvecs, 0, criteria);
    double *m = (double *)mtx.data;
    double k = m[0];
    return std::atan(3840 / (2 * k)) * 2 * 180 / std::acos(-1);
}
}
