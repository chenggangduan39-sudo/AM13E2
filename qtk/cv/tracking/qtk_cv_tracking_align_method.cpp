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
#include <opencv2/opencv.hpp>
#include "qtk_cv_tracking_align_method.h"

typedef cv::Point_<double> Point2d;

extern "C" {
void qtk_cv_tracking_align_method_warpaffine(uint8_t *src, uint8_t *dst,
                                             float *tform,
                                             qtk_image_desc_t *dst_desc,
                                             qtk_image_desc_t *src_desc) {
    const cv::Mat src_mat =
        cv::Mat(src_desc->height, src_desc->width, CV_8UC3, src);
    cv::Mat dst_mat;
    const cv::Mat rot = cv::Mat(2, 3, CV_32FC1, tform);
    cv::Size dst_sz(dst_desc->height, dst_desc->width);
    cv::warpAffine(src_mat, dst_mat, rot, dst_sz);
    if (dst_mat.isContinuous())
        memcpy(dst, dst_mat.ptr(), dst_mat.total() * dst_mat.elemSize());
    else {
        for (int i = 0; i < dst_desc->height; i++)
            memcpy(dst + i * dst_desc->width * 3,
                   dst_mat.ptr() + i * dst_mat.step,
                   dst_mat.elemSize() * dst_mat.cols);
    }
}

void qtk_cv_tracking_align_method_flip(uint8_t *imagedata,
                                       qtk_image_desc_t *desc, uint8_t *data) {
    if (imagedata == NULL) {
        std::cout << "imagedata NULL" << std::endl;
        return;
    }
    cv::Mat src = cv::Mat(desc->height, desc->width, CV_8UC3, imagedata);
    cv::Mat dst;
    cv::flip(src, dst, 1);
    if (dst.isContinuous())
        memcpy(data, dst.ptr(), dst.total() * dst.elemSize());
    else {
        for (int i = 0; i < desc->height; i++)
            memcpy(data + i * desc->width * 3, dst.ptr() + i * dst.step,
                   dst.elemSize() * dst.cols);
    }
}
};

static void rot2Euler(cv::Mat faceImg, const cv::Mat &rotation3_3,
                      double *res) {
    double q0 =
        std::sqrt(1 + rotation3_3.at<double>(0, 0) +
                  rotation3_3.at<double>(1, 1) + rotation3_3.at<double>(2, 2)) /
        2;
    double q1 = (rotation3_3.at<double>(2, 1) - rotation3_3.at<double>(1, 2)) /
                (4 * q0);
    double q2 = (rotation3_3.at<double>(0, 2) - rotation3_3.at<double>(2, 0)) /
                (4 * q0);
    double q3 = (rotation3_3.at<double>(1, 0) - rotation3_3.at<double>(0, 1)) /
                (4 * q0);
    double yaw = std::asin(2 * (q0 * q2 + q1 * q3));
    double pitch = std::atan2(2 * (q0 * q1 - q2 * q3),
                              q0 * q0 - q1 * q1 - q2 * q2 + q3 * q3);
    double roll = std::atan2(2 * (q0 * q3 - q1 * q2),
                             q0 * q0 + q1 * q1 - q2 * q2 - q3 * q3);
    pitch = (std::asin(std::sin(pitch))) * 180 / std::acos(-1);
    roll = (std::asin(std::sin(roll))) * 180 / std::acos(-1);
    yaw = (std::asin(std::sin(yaw))) * 180 / std::acos(-1);
    res[0] = yaw;
    res[1] = pitch;
    res[2] = roll;
}

static void headPosEstimate(const cv::Mat &faceImg,
                            const std::vector<cv::Point2d> &facial5Pts,
                            double *res) {
    std::vector<cv::Point3d> model_points;
    model_points.push_back(cv::Point3d(-165.0f, 170.0f, -115.0f)); // Left eye
    model_points.push_back(cv::Point3d(165.0f, 170.0f, -115.0f));  // Right eye
    model_points.push_back(cv::Point3d(0.0f, 0.0f, 0.0f));         // Nose tip
    model_points.push_back(
        cv::Point3d(-150.0f, -150.0f, -125.0f)); // Left Mouth corner
    model_points.push_back(
        cv::Point3d(150.0f, -150.0f, -125.0f)); // Right Mouth corner
    double focal_length = faceImg.cols;
    cv::Point2d center = cv::Point2d(faceImg.cols / 2, faceImg.rows / 2);
    cv::Mat camera_matrix = (cv::Mat_<double>(3, 3) << focal_length, 0,
                             center.x, 0, focal_length, center.y, 0, 0, 1);
    cv::Mat dist_coeffs = cv::Mat::zeros(4, 1, cv::DataType<double>::type);
    cv::Mat rotation_vector;
    cv::Mat translation_vector;
    cv::solvePnP(model_points, facial5Pts, camera_matrix, dist_coeffs,
                 rotation_vector, translation_vector, false, cv::SOLVEPNP_EPNP);

    cv::Mat rotation3_3;
    cv::Rodrigues(rotation_vector, rotation3_3);
    rot2Euler(faceImg.clone(), rotation3_3, res);
}

extern "C" {
void qtk_cv_tracking_align_method_facial_angle(uint8_t *imagedata,
                                               qtk_image_desc_t *desc,
                                               float *drop, qtk_cv_bbox_t *box,
                                               double *res) {
    const cv::Mat img = cv::Mat(desc->height, desc->width, CV_8UC3, imagedata);
    const cv::Mat face_img =
        img(cv::Rect(box->x1, box->y1, box->x2 - box->x1, box->y2 - box->y1));
    std::vector<Point2d> face5Pts;
    for (int i = 0; i < 5; i++) {
        face5Pts.push_back(Point2d(std::max(drop[i] - box->x1, (float)0),
                                   std::max(drop[i + 5] - box->y1, (float)0)));
    }
    headPosEstimate(face_img, face5Pts, res);
}

static void BGR2YUV(const cv::Mat &bgrImg, cv::Mat &y) {
    cv::Mat out;
    cv::cvtColor(bgrImg, out, cv::COLOR_BGR2YUV);
    cv::Mat channel[3];
    cv::split(out, channel);
    y = channel[0];
}

double qtk_cv_tracking_align_method_brightness(uint8_t *imagedata,
                                               qtk_image_desc_t *img_desc) {
    cv::Mat img =
        cv::Mat(img_desc->height, img_desc->width, CV_8UC3, imagedata);
    cv::Mat y;
    BGR2YUV(img, y);
    cv::Scalar tempVal = cv::mean(y);
    return tempVal.val[0];
}
}
