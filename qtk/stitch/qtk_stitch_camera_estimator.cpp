#include "qtk_stitch_camera_estimator.h"
#include "wtk/core/wtk_alloc.h"
#include "opencv2/stitching/detail/motion_estimators.hpp"
#include "qtk_stitch_def.h"
#include <vector>
#include "wtk/core/wtk_type.h"
#include <iostream>

qtk_stitch_camera_estimator_t* qtk_stitch_camera_estimator_new(int type)
{
    // std::cout << cv::getBuildInformation() << std::endl;

    qtk_stitch_camera_estimator_t* estimator = (qtk_stitch_camera_estimator_t*)wtk_malloc(sizeof(qtk_stitch_camera_estimator_t));
    memset(estimator, 0, sizeof(qtk_stitch_camera_estimator_t));
    estimator->type = type;
    estimator->estimator = NULL;
    wtk_debug("camera estimator type %d\n",type);
    if(type == QTK_STITCH_CAMERA_ESTIMATOR_AFFINE){
        estimator->estimator = new cv::detail::AffineBasedEstimator();
    }else{
        estimator->estimator = new cv::detail::HomographyBasedEstimator();
    }
    estimator->cameras = new std::vector<cv::detail::CameraParams>;
    return estimator;
}

void qtk_stitch_camera_estimator_estimate(qtk_stitch_camera_estimator_t* estimator,
                                          wtk_queue_t *queue,void *pairwise_matchesp)
{
    std::vector<cv::detail::MatchesInfo> *pairwise_matches = (std::vector<cv::detail::MatchesInfo>*)pairwise_matchesp;
    std::vector<cv::detail::ImageFeatures> features;
    std::vector<cv::detail::CameraParams> *cameras = (std::vector<cv::detail::CameraParams>*)estimator->cameras;

    int i = 0;
    int n = queue->length;
    int ret = 0;
    qtk_stitch_queue_data_t *data;
    wtk_queue_node_t *qn;
    cv::detail::Estimator *cv_estimator = (cv::detail::Estimator *)estimator->estimator;

    for(i = 0;i < n;i++){
        qn = wtk_queue_peek(queue,i);
        data = data_offset2(qn,qtk_stitch_queue_data_t,node);
        cv::detail::ImageFeatures *feature = (cv::detail::ImageFeatures*)data->feature_data;
        features.push_back(*feature);
    }

    cameras->clear();
    ret = (*cv_estimator)(features,*pairwise_matches,*cameras);
    if(!ret){
        wtk_debug("Homography estimation failed.");
        return;
    }
    std::vector<cv::detail::CameraParams>::iterator it = cameras->begin();
    for(;it < cameras->end();++it){
        cv::OutputArray outR(it->R);
        it->R.convertTo(outR,CV_32F);
        // printf("%f %f %f %f %d %d\n",it->aspect,it->focal,it->ppx,it->ppy,it->R.type(),it->R.cols);
        // float *d = (float*)it->R.data;
        // for(int i = 0; i < it->R.cols; ++i){
        //     for(int j = 0; j < it->R.rows; ++j){
        //         printf("%f\n",d[i*it->R.rows+j]);
        //     }
        // }
    }

    return;
}

void qtk_stitch_camera_estimator_delete(qtk_stitch_camera_estimator_t* estimator)
{
    std::vector<cv::detail::CameraParams> *cameras = NULL;
    cv::detail::Estimator *estimatorp = NULL;
    if(estimator){
        if(estimator->cameras){
            cameras =  (std::vector<cv::detail::CameraParams>*)estimator->cameras;
            delete cameras;
            estimator->cameras = NULL;
        }
        if(estimator->estimator){
            estimatorp = (cv::detail::Estimator*)estimator->estimator;
            delete estimatorp;
            estimator->estimator = NULL;
        }
        wtk_free(estimator);
        estimator = NULL;
    }
}

void qtk_stitch_camera_estimator_camera_push(qtk_stitch_camera_estimator_t* estimator,float aspect,
                                float focal,float ppx,float ppy,float *R,float *t)
{
    std::vector<cv::detail::CameraParams> *cameras = (std::vector<cv::detail::CameraParams>*)estimator->cameras;
    cv::detail::CameraParams params = cv::detail::CameraParams();
    params.aspect = aspect;
    params.focal = focal;
    params.ppx = ppx;
    params.ppy = ppy;
    params.R = cv::Mat(3,3,CV_32F,R);
    params.t = cv::Mat(3,1,CV_32F,t);
    cameras->push_back(params);
    return;
}

//得到相机R参数的范数
float _stitch_camera_R_norm(cv::Mat &R1, cv::Mat &R2)
{
    float norm = 0.0f;
    for(int i = 0; i < 3; ++i)
    {
        for(int j = 0; j < 3; ++j)
        {
            float q = R1.at<float>(i,j) - R2.at<float>(i,j);
            norm += q*q;
        }
    }
    norm = sqrtf(norm);
    return norm;
}

float qtk_stitch_camera_estimator_distance(qtk_stitch_camera_estimator_t* estimator1,qtk_stitch_camera_estimator_t* estimator2)
{
    std::vector<cv::detail::CameraParams> *cameras1 = (std::vector<cv::detail::CameraParams>*)estimator1->cameras;
    std::vector<cv::detail::CameraParams> *cameras2 = (std::vector<cv::detail::CameraParams>*)estimator2->cameras;
    if(cameras1->size() == 0){
        return -1.f;
    }
    if(cameras1->size() != cameras2->size()){
        printf("camera size is error %ld != %ld\n",cameras1->size(),cameras2->size());
        exit(1);
    }
    unsigned int i = 0;
    float r = 0.0f;
    for(i = 0; i < cameras1->size(); ++i){
        // std::cout << "camera1 R: " << cameras1->at(i).R << std::endl;
        // std::cout << "camera2 R: " << cameras2->at(i).R << std::endl;
        r += _stitch_camera_R_norm(cameras1->at(i).R,cameras2->at(i).R);
        // printf("r %d %f\n",i,r);
    }
    return r;
}

void qtk_stitch_camera_estimator_2to1(qtk_stitch_camera_estimator_t* estimator2,qtk_stitch_camera_estimator_t* estimator1)
{
    std::vector<cv::detail::CameraParams> *cameras1 = (std::vector<cv::detail::CameraParams>*)estimator1->cameras;
    std::vector<cv::detail::CameraParams> *cameras2 = (std::vector<cv::detail::CameraParams>*)estimator2->cameras;
    cameras1->clear();
    cameras1->assign(cameras2->begin(),cameras2->end());
    cameras2->clear();
    return;
}

void qtk_stitch_camera_estimator_2copy1(qtk_stitch_camera_estimator_t* estimator2,qtk_stitch_camera_estimator_t* estimator1)
{
    std::vector<cv::detail::CameraParams> *cameras1 = (std::vector<cv::detail::CameraParams>*)estimator1->cameras;
    std::vector<cv::detail::CameraParams> *cameras2 = (std::vector<cv::detail::CameraParams>*)estimator2->cameras;
    cameras1->assign(cameras2->begin(),cameras2->end());
    return;
}
