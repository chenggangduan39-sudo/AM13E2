#include "qtk_stitch_exposure_compensator.h"
#include "qtk_stitch_def.h"
#include "wtk/core/wtk_alloc.h"
#include <vector>
#include "opencv2/stitching/detail/exposure_compensate.hpp"
#include "opencv2/imgproc.hpp"
#include "qtk/stitch/image/qtk_stitch_image.h"
#ifdef USE_OPENMP
#include "omp.h"
#endif

struct qtk_stitch_exposure_compensator_t{
    int type;
    int nr_feeds;
    int block_size;
    int num_cameras;
    int img_type;
    cv::Ptr<cv::detail::ExposureCompensator> comp;
    std::vector<cv::Mat> gain_mat;
    std::vector<cv::Mat> point_mat;
};

class exposure_compensator_parallel: public cv::ParallelLoopBody
{
public:
    exposure_compensator_parallel(qtk_stitch_exposure_compensator_t *_comp,cv::Mat *_img,cv::Mat *_gain)
    {
        img = _img;
        gain = _gain;
        comp = _comp;
    }
    ~exposure_compensator_parallel(){};
    void operator()(const cv::Range& range) const;
private:
    qtk_stitch_exposure_compensator_t *comp;
    cv::Mat *img;
    cv::Mat *gain;
};

void _run_apply(cv::Mat &img,cv::Mat &gain);

qtk_stitch_exposure_compensator_t* qtk_stitch_exposure_compensator_new(int type, 
                                                                int nr_feeds, 
                                                                int block_size, int num_cameras, int img_type)
{
    qtk_stitch_exposure_compensator_t* comp = new qtk_stitch_exposure_compensator_t;
    comp->type = type;
    comp->nr_feeds = nr_feeds;
    comp->block_size = block_size;
    comp->num_cameras = num_cameras;
    comp->img_type = img_type;
    wtk_debug("exposure compensator %d\n",type);
    switch(type){
        case QTK_STITCH_EXPOSURE_CHANNEL:
            comp->comp = new cv::detail::ChannelsCompensator(nr_feeds);
            break;
        case QTK_STITCH_EXPOSURE_CHANNEL_BLOCKS:
            comp->comp = new cv::detail::BlocksChannelsCompensator(block_size, block_size, nr_feeds);
            break;
        default:
            if(type == QTK_STITCH_EXPOSURE_GAIN){
                comp->comp = cv::detail::ExposureCompensator::createDefault(cv::detail::ExposureCompensator::GAIN);
            }else if(type == QTK_STITCH_EXPOSURE_GAIN_BLOCK){
                comp->comp = cv::detail::ExposureCompensator::createDefault(cv::detail::ExposureCompensator::GAIN_BLOCKS);
            }else if(type == QTK_STITCH_EXPOSURE_NO){
                comp->comp = cv::detail::ExposureCompensator::createDefault(cv::detail::ExposureCompensator::NO);
            }
            break;
    }
    return comp; 
}

void qtk_stitch_exposure_compensator_delete(qtk_stitch_exposure_compensator_t *comp)
{
    if(comp){
        if(comp->comp != nullptr){
            cv::Ptr<cv::detail::ExposureCompensator> pp = (cv::Ptr<cv::detail::ExposureCompensator>)comp->comp;
            pp.release();
        }
        delete comp;
    }
}

void qtk_stitch_exposure_compensator_feed(qtk_stitch_exposure_compensator_t *comp,void *corners,void *imgs,void *masks)
{
    std::vector<cv::Point> *crop_corners = (std::vector<cv::Point> *)corners;
    cv::Mat *crop_images = (cv::Mat *)imgs;
    cv::Mat *crop_masks = (cv::Mat *)masks;
    std::vector<cv::UMat> crop_images_umat;
    std::vector<cv::UMat> crop_masks_umat;
    int n = crop_corners->size();
    for(int i = 0;i<n;++i){
        crop_images_umat.push_back(crop_images[i].getUMat(cv::ACCESS_RW));
    }
    for(int i = 0;i<n;++i){
        crop_masks_umat.push_back(crop_masks[i].getUMat(cv::ACCESS_RW));
    }
    comp->comp->feed(*crop_corners,crop_images_umat,crop_masks_umat);
    comp->comp->getMatGains(comp->gain_mat);
    return;
}

void qtk_stitch_exposure_compensator_resize(qtk_stitch_exposure_compensator_t *comp,void *imgs)
{
    cv::Mat *crop_images = (cv::Mat *)imgs;
    cv::Mat u_gain_map;
    int i = 0;
    for(std::vector<cv::Mat>::iterator it = comp->gain_mat.begin();it != comp->gain_mat.end();++it,++i){
        // std::cout << "resize gain_mat" << std::endl;
        cv::resize(*it, u_gain_map, crop_images[i].size(), 0, 0, cv::INTER_LINEAR);
        *it = u_gain_map;
    }
    return;
}
//定点化的
void qtk_stitch_exposure_compensator_resize2point(qtk_stitch_exposure_compensator_t *comp,void *imgs)
{
    cv::Mat *crop_images = (cv::Mat *)imgs;
    cv::Mat u_gain_map;
    int i = 0;
    float d = powf(2,QTK_STITCH_FIX_POINT);
    comp->point_mat.clear();
    for(std::vector<cv::Mat>::iterator it = comp->gain_mat.begin();it != comp->gain_mat.end();++it,++i){
        // std::cout << "resize gain_mat" << std::endl;
        cv::resize(*it, u_gain_map, crop_images[i].size(), 0, 0, cv::INTER_LINEAR);
        //resize 定点化
        u_gain_map = u_gain_map.mul(d);
        cv::Mat out;
        u_gain_map.convertTo(out, CV_16U);
        comp->point_mat.push_back(out);
    }
    return;
}

// static cv::Mat _get_mask(std::vector<cv::Mat> &masks,int idx)
// {
//     if(idx < masks.size()){
//         return masks[idx].clone();
//     }else{
//         wtk_debug("Invalid Mask Index!\n");
//         exit(1);
//     }
//     return NULL;
// }
#include "wtk/core/wtk_os.h"
void qtk_stitch_exposure_compensator_apply(qtk_stitch_exposure_compensator_t *comp,void *corners,void *img,void *mask)
{
    std::vector<cv::Point> *crop_corners = (std::vector<cv::Point> *)corners;
    cv::Mat *crop_img = (cv::Mat *)img;
    cv::Mat *crop_mask = (cv::Mat *)mask;
    int n = comp->num_cameras;
#ifdef USE_OPENMP
    #pragma omp parallel for num_threads(n)
#endif
    for(int idx = 0; idx < n; idx++){
        // wtk_debug("qtk_stitch_exposure_compensator_apply\n");
        if(comp->img_type == QTK_STITCH_USE_IMG_NV12 && comp->type == QTK_STITCH_EXPOSURE_GAIN_BLOCK){
            class exposure_compensator_parallel ecp = exposure_compensator_parallel(comp,crop_img+idx,&comp->point_mat[idx]);
            cv::parallel_for_(cv::Range(0,comp->point_mat[idx].rows),ecp,-1.0);
            // _run_apply(crop_img[idx],comp->gain_mat[idx]);
        }else{
            comp->comp->apply(idx,(*crop_corners)[idx],crop_img[idx],crop_mask[idx]);
        }
        // for(int i = 0;i < it->rows;++i){
        //     for(int j = 0;j < it->cols;++j){
        //         printf("%d\n",it->ptr<uchar>(i,j)[0]);     
        //         printf("%d\n",it->ptr<uchar>(i,j)[1]); 
        //         printf("%d\n",it->ptr<uchar>(i,j)[2]);           
        //     }
        // }
        // wtk_debug("%d %d %d\n",it->rows,it->cols,it->channels());
        // char path[125] = {0,};
        // sprintf(path,"exposure_%d.png",idx);
        // qtk_stitch_image_save_data(path,crop_img[idx].data,crop_img[idx].cols,crop_img[idx].rows,1);
    }

    return;
}

inline uchar _mage(float q)
{
    if(q > 255){
        return 255;
    }else{
        return q;
    }
}

inline uchar _mage2(ushort q)
{
    if(q > 255){
        return 255;
    }else{
        return q;
    }
}

void _run_apply(cv::Mat &img,cv::Mat &gain)
{
    int rows = gain.rows;
    int cols = gain.cols;
    int n = rows*cols;
    ushort *g = gain.ptr<ushort>();
    uchar *p = img.ptr<uchar>();
    if(img.channels() == 1){
        for(int i = 0;i < n; ++i){
            p[i] = _mage2((p[i]*g[i])>>QTK_STITCH_FIX_POINT);
        }
    }else if(img.channels() == 3){
        for(int i = 0;i < n; ++i){
            // printf("%d\n",g[i]);
            p[0] = _mage2((p[0]*g[i])>>QTK_STITCH_FIX_POINT);
            p[1] = _mage2((p[1]*g[i])>>QTK_STITCH_FIX_POINT);
            p[2] = _mage2((p[2]*g[i])>>QTK_STITCH_FIX_POINT);
            p+=3;
        }
    }
    return;
}

void _run_apply_float(cv::Mat &img,cv::Mat &gain)
{
    int rows = gain.rows;
    int cols = gain.cols;
    int n = rows*cols;
    float *g = gain.ptr<float>();
    uchar *p = img.ptr<uchar>();
    if(img.channels() == 1){
        for(int i = 0;i < n; ++i){
            p[i] = _mage(p[i]*g[i]);
        }
    }else if(img.channels() == 3){
        for(int i = 0;i < n; ++i){
            // printf("%f\n",g[i]*(1<<7));
            p[0] = _mage(p[0]*g[i]);
            p[1] = _mage(p[1]*g[i]);
            p[2] = _mage(p[2]*g[i]);
            p+=3;
        }
    }
    return;
}

void _run_apply_rect(cv::Mat &img,cv::Mat &gain,int s, int e)
{
    int cols = gain.cols;
    int n = (e-s)*cols;
    ushort *g = gain.ptr<ushort>(s,0);
    uchar *p = img.ptr<uchar>(s,0);
    int i = 0;
#ifdef USE_NEON
    // int cross_n = n>>3<<3;
    // ushort v[8] = {0,};
    // for(; i < cross_n; i+=8){
    //     v[0] = p[0];
    //     v[1] = p[1];
    //     v[2] = p[2];
    //     v[3] = p[3];
    //     v[4] = p[4];
    //     v[5] = p[5];
    //     v[6] = p[6];
    //     v[7] = p[7];
    //     uint16x8_t Sv = vld1q_u16(v);
    //     uint16x8_t Wv = vld1q_u16(g+i);
    //     uint16x8_t Swv = vmulq_u16(Sv, Wv);
    //     uint8x8_t Sdv = vqrshrn_n_u16(Swv,QTK_STITCH_FIX_POINT);
    //     vst1_u8(p, Sdv);
    //     p+=8;
    // }
#endif
    for(; i < n; ++i,++p){
        *p = _mage2(((*p)*g[i])>>QTK_STITCH_FIX_POINT);
    }
    return;
}

void qtk_stitch_exposure_compensator_apply_index(qtk_stitch_exposure_compensator_t *comp,
                                                    void *corners,void *img,void *mask, int index)
{
    // std::vector<cv::Point> *crop_corners = (std::vector<cv::Point> *)corners;
    cv::Mat *crop_img = (cv::Mat *)img;
    // cv::Mat *crop_mask = (cv::Mat *)mask;
    if(comp->img_type == QTK_STITCH_USE_IMG_RGB){
        // comp->comp->apply(index,(*crop_corners)[index],crop_img[index],crop_mask[index]);
        _run_apply_float(crop_img[index],comp->gain_mat[index]);
        // _run_apply(crop_img[index],comp->point_mat[index]);
    }else if(comp->img_type == QTK_STITCH_USE_IMG_NV12 && comp->type == QTK_STITCH_EXPOSURE_GAIN_BLOCK){
        _run_apply(crop_img[index],comp->point_mat[index]);
    }

    return;
}

void *qtk_stitch_exposure_compensator_gain_mat_get(qtk_stitch_exposure_compensator_t *comp)
{
    return (void*)&comp->gain_mat;
}

void exposure_compensator_parallel::operator()(const cv::Range& range) const
{
    if(comp->type == QTK_STITCH_EXPOSURE_GAIN_BLOCK && comp->img_type == QTK_STITCH_USE_IMG_NV12){
        _run_apply_rect(*img,*gain,range.start,range.end);
    }
    return;
}