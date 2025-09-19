#include "qtk_stitch_camera_adjuster.h"
#include "wtk/core/wtk_alloc.h"
#include "qtk_stitch_def.h"
#include "opencv2/stitching/detail/motion_estimators.hpp"
#include "opencv2/core/mat.hpp"
#include "wtk/core/wtk_type.h"

void _set_refinement_mask(qtk_stitch_camera_adjuster_t* adjuster, const char* mask);

void _print_Mat_d2_u(cv::Mat &m)
{
    int dims = m.dims;
    int rows = m.rows;
    int cols = m.cols;
    int rows_step = m.step[0];
    int i = 0;
    int j = 0;
    uchar *p = m.data;
    printf("dims: %d %d %d\n", dims,rows,cols);
    for(i = 0; i < rows; ++i){
        for(j = 0; j < cols; ++j){
            printf("%d ", p[i*rows_step + j]);
        }
        printf("\n");
    }
    return;
}

qtk_stitch_camera_adjuster_t* qtk_stitch_camera_adjuster_new(int type,
                                                        char* refinement_mask, 
                                                        float confidence_threshold)
{
    qtk_stitch_camera_adjuster_t *adjuster = (qtk_stitch_camera_adjuster_t*)wtk_malloc(sizeof(qtk_stitch_camera_adjuster_t));
    adjuster->type = type;
    adjuster->refinement_mask = refinement_mask;
    adjuster->confidence_threshold = confidence_threshold;
    adjuster->adjuster = NULL;
    wtk_debug("camera adjuster %d %f\n",type,confidence_threshold);
    switch(adjuster->type){
        case QTK_STITCH_CAMERA_ADJUSTER_RAY:
            adjuster->adjuster = new cv::detail::BundleAdjusterRay();
            break;
        case QTK_STITCH_CAMERA_ADJUSTER_REPROJ:
            adjuster->adjuster = new cv::detail::BundleAdjusterReproj();
            break;
        case QTK_STITCH_CAMERA_ADJUSTER_AFFINE:
            adjuster->adjuster = new cv::detail::BundleAdjusterAffine();
            break;
        case QTK_STITCH_CAMERA_ADJUSTER_NO:
            adjuster->adjuster = new cv::detail::NoBundleAdjuster();
            break;
    }
    _set_refinement_mask(adjuster,refinement_mask);
    ((cv::detail::BundleAdjusterBase*)adjuster->adjuster)->setConfThresh(confidence_threshold);
    return adjuster;
}

void qtk_stitch_camera_adjuster_delete(qtk_stitch_camera_adjuster_t* adjuster)
{
    cv::detail::BundleAdjusterBase *adjusterp = NULL;
    if(adjuster){
        if(adjuster->adjuster){
            adjusterp = (cv::detail::BundleAdjusterBase*)adjuster->adjuster;
            delete adjusterp;
            adjuster->adjuster = NULL;
        }
        wtk_free(adjuster);
    }
}

void _set_refinement_mask(qtk_stitch_camera_adjuster_t* adjuster, const char* mask)
{
    cv::Mat mat;
    mat = cv::Mat::zeros(3,3,CV_8U);
    cv::detail::BundleAdjusterBase *ad = (cv::detail::BundleAdjusterBase*)adjuster->adjuster;
    if(mask[0] =='x'){
        mat.at<uchar>(0,0) = 1;
    }
    if(mask[1] == 'x'){
        mat.at<uchar>(0,1) = 1;
    }
    if(mask[2] == 'x'){
        mat.at<uchar>(0,2) = 1;
    }
    if(mask[3] == 'x'){
        mat.at<uchar>(1,1) = 1;
    }
    if(mask[4] == 'x'){
        mat.at<uchar>(1,2) = 1;
    }
    // _print_Mat_d2_u(mat);
    ad->setRefinementMask(mat);
    return;
}

void qtk_stitch_camera_adjuster_adjust(qtk_stitch_camera_adjuster_t* adjuster,
                                        wtk_queue_t *queue,void *pairwise_matchesp,
                                        void *estimated_cameras)
{
    std::vector<cv::detail::MatchesInfo> *pairwise_matches = (std::vector<cv::detail::MatchesInfo>*)pairwise_matchesp;
    std::vector<cv::detail::ImageFeatures> features;
    std::vector<cv::detail::CameraParams> *cameras = (std::vector<cv::detail::CameraParams>*)estimated_cameras;
    cv::detail::BundleAdjusterBase *bundle_adjuster = (cv::detail::BundleAdjusterBase*)adjuster->adjuster;

    int i = 0;
    int n = queue->length;
    int ret = 0;
    qtk_stitch_queue_data_t *data;
    wtk_queue_node_t *qn;

    for(i = 0;i < n;i++){
        qn = wtk_queue_peek(queue,i);
        data = data_offset2(qn,qtk_stitch_queue_data_t,node);
        cv::detail::ImageFeatures *feature = (cv::detail::ImageFeatures*)data->feature_data;
        features.push_back(*feature);
    }

    ret = (*bundle_adjuster)(features, *pairwise_matches, *cameras);
    if(!ret){
        wtk_debug("Camera parameters adjusting failed.\n");
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