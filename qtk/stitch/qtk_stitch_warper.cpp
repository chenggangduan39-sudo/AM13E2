#include "qtk_stitch_warper.h"
#include "wtk/core/wtk_alloc.h"
#include "opencv2/core/mat.hpp"
#include "opencv2/stitching/detail/motion_estimators.hpp"
#include "opencv2/stitching/warpers.hpp"
#include <vector>
#include "image/qtk_stitch_image.h"
#ifdef USE_OPENMP
#include "omp.h"
#endif

extern "C"{
    double time_get_ms(void);
};

const char *WARP_TYPE_CHOICES[]={
        "spherical",
        "plane",
        "affine",
        "cylindrical",
        "fisheye",
        "stereographic",
        "compressedPlaneA2B1",
        "compressedPlaneA1.5B1",
        "compressedPlanePortraitA2B1",
        "compressedPlanePortraitA1.5B1",
        "paniniA2B1",
        "paniniA1.5B1",
        "paniniPortraitA2B1",
        "paniniPortraitA1.5B1",
        "mercator",
        "transverseMercator",
};

qtk_stitch_warper_t* qtk_stitch_warper_new(int type,int num_camera)
{
    qtk_stitch_warper_t* warpe = (qtk_stitch_warper_t*)wtk_malloc(sizeof(qtk_stitch_warper_t));
    wtk_debug("warper type %d\n",type);
    warpe->type = type;
    warpe->scale = 0.0f;
    warpe->warper_imgs = NULL;
    warpe->warper_mask = NULL;
    warpe->roi_corners = NULL;
    warpe->roi_sizes = NULL;
    warpe->num_camera = num_camera;
    warpe->warper_imgs = new cv::Mat[num_camera];
    warpe->warper_mask = new cv::Mat[num_camera];
    warpe->uxmap = NULL;
    warpe->uymap = NULL;
    warpe->dst_roi = NULL;
    return warpe;
}

void qtk_stitch_warper_set_scale(qtk_stitch_warper_t* warpe, void *estimated_cameras)
{
    std::vector<cv::detail::CameraParams> *cameras = (std::vector<cv::detail::CameraParams>*)estimated_cameras;
    std::vector<float> focals;

    for(std::vector<cv::detail::CameraParams>::iterator it = cameras->begin(); it != cameras->end(); ++it){
        focals.push_back(it->focal);
    }
    sort(focals.begin(), focals.end());
    int n = focals.size()/2;
    int u = focals.size()%2;
    float k = 0.0f;
    if(u == 0){ //取中位数 偶数 中间两位的平均值 奇数 中间一位
        k = (focals[n-1] + focals[n])/2.0f;
    }else{
        k = focals[n];
    }
    warpe->scale = k;
    return;
}

cv::Mat _warper_get_K(const cv::Mat K, float aspect)
{
    cv::Mat K1;
    K.assignTo(K1,CV_32F);
    // wtk_debug("%d %d %d\n",K1.step[0],K1.step[1],K1.type());
    // wtk_debug("%d %d %d\n",K.step[0],K.step[1],K.type());
    K1.at<float>(0,0) *= aspect;
    K1.at<float>(0,2) *= aspect;
    K1.at<float>(1,1) *= aspect;
    K1.at<float>(1,2) *= aspect;
    // float *p = (float*)K1.ptr();
    // for(int i = 0; i < K1.cols;++i){
    //     for(int j = 0; j < K1.rows;++j){
    //         printf("%f\n",p[i*K1.rows+j]);
    //     }
    // }
    // exit(1);
    return K1; 
}

cv::Mat _warper_warp_image(qtk_stitch_warper_t *warpe,qtk_stitch_image_t *image,
                        cv::detail::CameraParams &camera, float aspect, int type)
{
    cv::PyRotationWarper warper(WARP_TYPE_CHOICES[warpe->type],warpe->scale*aspect);
    // wtk_debug("%s %f\n",WARP_TYPE_CHOICES[warpe->type],warpe->scale*aspect);
    cv::Mat *img = NULL;
    if(type == QTK_STITCH_IMAGE_RESOLUTION_LOW){
        img = (cv::Mat*)image->low_image_data;
    }else if(type == QTK_STITCH_IMAGE_RESOLUTION_FINAL){
        img = (cv::Mat*)image->final_image_data;
    }
    cv::Mat dst;
    warper.warp(*img,_warper_get_K(camera.K(),aspect),camera.R,cv::INTER_LINEAR,cv::BORDER_REFLECT,dst);
    // wtk_debug("dst %d %d %d %d\n",dst.rows,dst.cols,dst.channels(),dst.type());
    // uchar *p = dst.ptr();
    // for(int i = 0; i < dst.rows; i++){
    //     for(int j = 0; j < dst.cols; j++){
    //         for(int k = 0; k < dst.channels(); k++){
    //             printf("%d\n",p[i*dst.cols*dst.channels()+j*dst.channels()+k]);
    //         }
    //     }
    // }
    // exit(1);
    return dst;
}

//只用final来用
void qtk_stitch_warper_warp_buildmaps(qtk_stitch_warper_t *warpe,int w,int h,void *estimated_cameras,float aspect)
{
    std::vector<cv::UMat> uxmaps;
    std::vector<cv::UMat> uymaps;
    std::vector<cv::Rect> dst_rois;
    cv::UMat uxmap, uymap;
    std::vector<cv::detail::CameraParams> *cameras = (std::vector<cv::detail::CameraParams>*)estimated_cameras;
    int n = cameras->size();
    cv::PyRotationWarper warper(WARP_TYPE_CHOICES[warpe->type],warpe->scale*aspect);
    for(int i = 0; i < n; ++i){
        cv::Rect dst_roi = warper.buildMaps(cv::Size(w,h), _warper_get_K((*cameras)[i].K(),aspect), (*cameras)[i].R, uxmap, uymap);
        dst_rois.push_back(dst_roi);
        uxmaps.push_back(uxmap);
        uymaps.push_back(uymap);
    }
    warpe->dst_roi = new std::vector<cv::Rect>(dst_rois);
    warpe->uxmap = new std::vector<cv::UMat>(uxmaps);
    warpe->uymap = new std::vector<cv::UMat>(uymaps);

    return;
}

void qtk_stitch_warper_warp_clearmaps(qtk_stitch_warper_t *warpe)
{
    if(warpe->dst_roi){
        std::vector<cv::Rect> *dst_roi = (std::vector<cv::Rect>*)warpe->dst_roi;
        delete dst_roi;
        warpe->dst_roi = NULL;
    }
    if(warpe->uxmap){
        std::vector<cv::UMat> *uxmap = (std::vector<cv::UMat>*)warpe->uxmap;
        delete uxmap;
        warpe->uxmap = NULL;
    }
    if(warpe->uymap){
        std::vector<cv::UMat> *uymap = (std::vector<cv::UMat>*)warpe->uymap;
        delete uymap;
        warpe->uymap = NULL;
    }
    return;
}

cv::Mat _warper_warp_image_remap(qtk_stitch_warper_t *warpe,qtk_stitch_image_t *image,
                                cv::UMat &uxmap,cv::UMat &uymap, cv::Rect &dst_roi)
{
    cv::Mat dst;
    cv::Mat *img = NULL;

    img = (cv::Mat*)image->final_image_data;

    dst.create(dst_roi.height + 1, dst_roi.width + 1, img->type());
    // wtk_debug("%d %d %d %d\n",uxmap.rows,uxmap.cols,dst_roi.height,dst_roi.width);
    // wtk_debug("----> %lf\n",time_get_ms());
    cv::remap(*img, dst, uxmap, uymap, cv::INTER_LINEAR,cv::BORDER_REFLECT);
    // for(int i = 0; i < uymap.rows; ++i){
    //     for(int j = 0; j < uymap.cols; ++j){
    //         printf("%f\n",uymap.getMat(cv::ACCESS_READ).at<float>(i,j));
    //     }
    // }
    return dst;
}

void qtk_stitch_warper_warp_images(qtk_stitch_warper_t* warpe,wtk_queue_t* queue,
                                    void *estimated_cameras,float aspect, int type)
{
    int n = queue->length;
    std::vector<cv::detail::CameraParams> *cameras = (std::vector<cv::detail::CameraParams>*)estimated_cameras;
    wtk_queue_node_t *node = NULL;
    qtk_stitch_queue_data_t *image = NULL;
    cv::Mat *warp_image = (cv::Mat*)warpe->warper_imgs;
    
    // std::vector<cv::Mat> images;
    // std::vector<cv::Mat> *images_ptr = NULL;
#ifdef USE_OPENMP
    #pragma omp parallel for num_threads(n)
#endif
    for(int i = 0; i < n; ++i){
        node = wtk_queue_peek(queue,i);
        image = data_offset2(node,qtk_stitch_queue_data_t,node);
        warp_image[i] = _warper_warp_image(warpe,(qtk_stitch_image_t*)image->img_data,(*cameras)[i],aspect,type);
        // // wtk_debug("%d %d %d %d\n",warp_image.rows,warp_image.cols,warp_image.channels(),warp_image.type());
    }
    // for(int i = 0; i < n; ++i){
    //     images.push_back(warp_image[i]);
    // }

    // if(warpe->warper_imgs){
    //     images_ptr = (std::vector<cv::Mat>*)warpe->warper_imgs;
    //     delete images_ptr;
    //     warpe->warper_imgs = NULL;
    // }
    // warpe->warper_imgs = (void*)new std::vector<cv::Mat>(images);
    return;
}

//只用作注册使用
void qtk_stitch_warper_warp_images2(qtk_stitch_warper_t* warpe,wtk_queue_t* queue)
{
    int n = queue->length;
    wtk_queue_node_t *node = NULL;
    qtk_stitch_queue_data_t *image = NULL;
    // cv::Mat warp_image[10];
    cv::Mat *images = (cv::Mat*)warpe->warper_imgs;
    // std::vector<cv::Mat> *images_ptr = NULL;

    std::vector<cv::Rect> *dst_roi = (std::vector<cv::Rect>*)warpe->dst_roi;
    std::vector<cv::UMat> *uxmap = (std::vector<cv::UMat>*)warpe->uxmap;
    std::vector<cv::UMat> *uymap = (std::vector<cv::UMat>*)warpe->uymap;
#ifdef USE_OPENMP
    #pragma omp parallel for num_threads(n)
#endif
    for(int i = 0; i < n; ++i){
        node = wtk_queue_peek(queue,i);
        image = data_offset2(node,qtk_stitch_queue_data_t,node);
        // double tt = time_get_ms();
        images[i] = _warper_warp_image_remap(warpe,(qtk_stitch_image_t*)image->img_data,
                                                    uxmap->at(i),uymap->at(i),dst_roi->at(i));
        // // wtk_debug("%d %d %d %d\n",warp_image.rows,warp_image.cols,warp_image.channels(),warp_image.type());
        // wtk_debug("%lf\n",time_get_ms()-tt);
    }
    // for(int i = 0; i < n; ++i){
    //     images.push_back(warp_image[i]);
    // }

    // if(warpe->warper_imgs){
    //     images_ptr = (std::vector<cv::Mat>*)warpe->warper_imgs;
    //     delete images_ptr;
    //     warpe->warper_imgs = NULL;
    // }
    // warpe->warper_imgs = (void*)new std::vector<cv::Mat>(images);
    return;
}
//单张图片处理
void qtk_stitch_warper_warp_image(qtk_stitch_warper_t* warpe,wtk_queue_t* queue,int index)
{
    wtk_queue_node_t *node = NULL;
    qtk_stitch_queue_data_t *image = NULL;
    // cv::Mat warp_image[10];
    cv::Mat *images = (cv::Mat*)warpe->warper_imgs;
    // std::vector<cv::Mat> *images_ptr = NULL;

    std::vector<cv::Rect> *dst_roi = (std::vector<cv::Rect>*)warpe->dst_roi;
    std::vector<cv::UMat> *uxmap = (std::vector<cv::UMat>*)warpe->uxmap;
    std::vector<cv::UMat> *uymap = (std::vector<cv::UMat>*)warpe->uymap;

    node = wtk_queue_peek(queue,index);
    image = data_offset2(node,qtk_stitch_queue_data_t,node);
    // double tt = time_get_ms();
    images[index] = _warper_warp_image_remap(warpe,(qtk_stitch_image_t*)image->img_data,
                                                uxmap->at(index),uymap->at(index),dst_roi->at(index));

    return;
}

cv::Mat _create_and_warp_mask(qtk_stitch_warper_t *warpe, 
                                int size_w, int size_h,
                                cv::detail::CameraParams &camera, float aspect)
{
    cv::PyRotationWarper warper(WARP_TYPE_CHOICES[warpe->type],warpe->scale*aspect);
    cv::Mat mask = cv::Mat::ones(size_h, size_w, CV_8UC1);
    mask*=255.0;
    cv::Mat dst;
    warper.warp(mask,_warper_get_K(camera.K(),aspect),camera.R,cv::INTER_NEAREST,cv::BORDER_CONSTANT,dst);
    // wtk_debug("%d %d\n",dst.rows,dst.cols);
    // uchar *p = dst.ptr();
    // for(int i = 0; i < dst.rows; ++i){
    //     for(int j = 0; j < dst.cols; ++j){
    //         printf("%d\n",p[i*dst.cols+j]);
    //     }
    // }
    // exit(1);
    return dst;
}

void qtk_stitch_warper_create_and_warp_mask(qtk_stitch_warper_t* warpe,
                                int *size_w, int *size_h, 
                                void *estimated_cameras,float aspect)
{
    std::vector<cv::detail::CameraParams> *cameras = (std::vector<cv::detail::CameraParams>*)estimated_cameras;
    cv::Mat *warped_mask = (cv::Mat*)warpe->warper_mask;
    // std::vector<cv::Mat> masks;
    // std::vector<cv::Mat> *masks_ptr = NULL;
    int i = 0;

    std::vector<cv::detail::CameraParams>::iterator it;
    for(it = cameras->begin(), i=0; it < cameras->end(); ++it, ++i){
        warped_mask[i] = _create_and_warp_mask(warpe,size_w[i],size_h[i],*it,aspect);
        // masks.push_back(warped_mask.clone());
    }
    // if(warpe->warper_mask){
    //     masks_ptr = (std::vector<cv::Mat> *)warpe->warper_mask;
    //     delete masks_ptr;
    //     warpe->warper_mask = NULL;
    // }
    // warpe->warper_mask = (void*)new std::vector<cv::Mat>(masks);
    return;
}

cv::Rect _warp_roi(qtk_stitch_warper_t* warpe, int w, int h, 
                cv::detail::CameraParams &camera, float aspect)
{
    cv::PyRotationWarper warper(WARP_TYPE_CHOICES[warpe->type],warpe->scale*aspect);
    cv::Size size(w,h);
    cv::Mat K = _warper_get_K(camera.K(),aspect);
    cv::Rect roi = warper.warpRoi(size,K,camera.R);
    return roi;
}

void qtk_stitch_warper_warp_rois(qtk_stitch_warper_t* warpe,
                                int *size_w, int *size_h, 
                                void *estimated_cameras,float aspect)
{
    std::vector<cv::detail::CameraParams> *cameras = (std::vector<cv::detail::CameraParams>*)estimated_cameras;
    int i = 0;
    cv::Rect roi;
    std::vector<cv::Point> roi_corners;
    std::vector<cv::Size> roi_sizes;
    std::vector<cv::Point> *tmp;
    std::vector<cv::Size> *tmp2;
    std::vector<cv::detail::CameraParams>::iterator it;
    for(it = cameras->begin(), i=0; it < cameras->end(); ++it, ++i){
        roi = _warp_roi(warpe,size_w[i],size_h[i],*it,aspect);
        roi_corners.push_back(cv::Point(roi.x,roi.y));
        roi_sizes.push_back(cv::Size(roi.width,roi.height));
    }
    if(warpe->roi_corners){
        tmp = (std::vector<cv::Point>*)warpe->roi_corners;
        delete tmp;
        warpe->roi_corners = NULL;
    }
    if(warpe->roi_sizes){
        tmp2 = (std::vector<cv::Size>*)warpe->roi_sizes;
        delete tmp2;
        warpe->roi_sizes = NULL;
    }
    warpe->roi_corners = (void*)new std::vector<cv::Point>(roi_corners);
    warpe->roi_sizes = (void*)new std::vector<cv::Size>(roi_sizes);
    return;
}

void qtk_stitch_warper_delete(qtk_stitch_warper_t* warpe)
{
    std::vector<cv::Point> *tmp;
    std::vector<cv::Size> *tmp2;
    std::vector<cv::UMat> *tmp3 = NULL;
    std::vector<cv::Rect> *tmp4;
    if(warpe->warper_imgs){
        cv::Mat* images = (cv::Mat*)warpe->warper_imgs;
        delete[] images;
        warpe->warper_imgs = NULL;
    }
    if(warpe->warper_mask){
        cv::Mat *images = (cv::Mat*)warpe->warper_mask;
        delete[] images;
        warpe->warper_mask = NULL;
    }
    if(warpe->roi_corners){
        tmp = (std::vector<cv::Point>*)warpe->roi_corners;
        delete tmp;
        warpe->roi_corners = NULL;
    }
    if(warpe->roi_sizes){
        tmp2 = (std::vector<cv::Size>*)warpe->roi_sizes;
        delete tmp2;
        warpe->roi_sizes = NULL;
    }
    if(warpe->dst_roi){
        tmp4 = (std::vector<cv::Rect>*)warpe->dst_roi;
        delete tmp4;
        warpe->dst_roi = NULL;
    }
    if(warpe->uxmap){
        tmp3 = (std::vector<cv::UMat>*)warpe->uxmap;
        delete tmp3;
        warpe->uxmap = NULL;
    }
    if(warpe->uymap){
        tmp3 = (std::vector<cv::UMat>*)warpe->uymap;
        delete tmp3;
        warpe->uymap = NULL;
    }
    wtk_free(warpe);
    return;
}