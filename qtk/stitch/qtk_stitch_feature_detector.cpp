#include "qtk_stitch_feature_detector.h"
#include "opencv2/features2d.hpp"
#include "opencv2/stitching/detail/matchers.hpp"
#include "opencv2/core/mat.hpp"
#include "wtk/core/wtk_alloc.h"
#include "image/qtk_stitch_image.h"


struct qtk_stitch_feature_detector{
    cv::Ptr<cv::FeatureDetector> detector;
    int type;
    int nfeatures;
};

qtk_stitch_feature_detector_t* qtk_stitch_feature_detector_new(int type, int nfeatures)
{
    qtk_stitch_feature_detector_t* detector = new qtk_stitch_feature_detector_t;
    detector->nfeatures = nfeatures;
    detector->type = type;
    cv::Ptr<cv::FeatureDetector> feature2d;
    wtk_debug("detector nfeatures: %d\n",nfeatures);
    switch (type){
    case QTK_STITCH_FEATURE_DETECTOR_ORB:
        wtk_debug("detector type: ord\n");
        feature2d = cv::ORB::create(detector->nfeatures);
        break;
    case QTK_STITCH_FEATURE_DETECTOR_SIFT:
        wtk_debug("detector type: sift\n");
        feature2d = cv::SIFT::create(detector->nfeatures);
        break;
    case QTK_STITCH_FEATURE_DETECTOR_BRISK:
        wtk_debug("detector type: brisk\n");
        feature2d = cv::BRISK::create();
        break;
    case QTK_STITCH_FEATURE_DETECTOR_AKAZE:
        wtk_debug("detector type: akaze\n");
        feature2d = cv::AKAZE::create();
        break;
    default:
        wtk_debug("detector type: unknown\n");
        exit(1);
        break;
    }
    detector->detector = feature2d;
	return detector;
}

void* qtk_stitch_feature_detector_detect(qtk_stitch_feature_detector_t *detector,void *image)
{
    cv::Ptr<cv::FeatureDetector> feature2d = detector->detector;
    qtk_stitch_image_t *img = (qtk_stitch_image_t*)image;
    cv::detail::ImageFeatures features;
    cv::Mat *img_data = (cv::Mat*)img->medium_image_data;

    cv::detail::computeImageFeatures(detector->detector,cv::InputArray(*img_data),features);
    // wtk_debug("index %d %d \n",features.img_size.width,features.img_size.height);
    // std::vector< cv::KeyPoint > k = features.getKeypoints();
    // for(std::vector<cv::KeyPoint>::iterator it = k.begin();it!=k.end();it++){
    //     printf("%f %d %f %f %f\n",it->angle,it->octave,it->response,it->pt.x,it->pt.y);
    // }
    // exit(1);
    return (void*)new cv::detail::ImageFeatures(features);
}

void qtk_stitch_feature_features_delete(void *features)
{
    cv::detail::ImageFeatures *p = (cv::detail::ImageFeatures *)features;
    delete p;
    return;
}

void qtk_stitch_feature_detector_delete(qtk_stitch_feature_detector_t* detector)
{
    detector->detector.release();
	delete(detector);
    return;
}