#include "qtk_blender_no.h"
#include <iostream>
#include <math.h>
#include "qtk/stitch/qtk_stitch_def.h"
#include "opencv2/stitching/detail/util.hpp"
#include "opencv2/stitching/detail/blenders.hpp"
#include "opencv2/imgproc.hpp"

struct qtk_blender_no{
    cv::UMat dst_, dst_mask_;
    cv::Rect dst_roi_;
    std::vector<cv::UMat> weight_maps_; //原始的权重
};

#ifdef USE_9391
#define BLEND_DST_CHANNEL 4
#else
#define BLEND_DST_CHANNEL 3
#endif

qtk_blender_no_t* qtk_blender_no_new(void)
{
    qtk_blender_no_t *blender = new qtk_blender_no_t;
    return blender;
}

int qtk_blender_no_delete(qtk_blender_no_t *blender)
{
    delete blender;
    return 0;
}

void qtk_blender_no_prepare(qtk_blender_no_t *blender,cv::Rect &dst_roi)
{
    blender->dst_.create(dst_roi.size(), CV_8UC3);
    blender->dst_.setTo(cv::Scalar::all(0));
    blender->dst_roi_ = dst_roi;
    return;
}

void qtk_blender_no_prepare2(qtk_blender_no_t *blender, cv::Rect dst_roi, void *ptr)
{
#ifdef USE_9391
    blender->dst_ = cv::Mat(dst_roi.size(), CV_8UC4, ptr).getUMat(cv::ACCESS_RW);
#else
    blender->dst_ = cv::Mat(dst_roi.size(), CV_8UC3, ptr).getUMat(cv::ACCESS_RW);
#endif
    blender->dst_roi_ = dst_roi;
    return;
}

void qtk_blender_no_nv12_prepare2(qtk_blender_no_t *blender, cv::Rect dst_roi, void *ptr)
{
    int cols = dst_roi.width;
    int rows = dst_roi.height*1.5;
    blender->dst_ = cv::Mat(cv::Size(cols,rows), CV_8UC1, ptr).getUMat(cv::ACCESS_RW);
    blender->dst_roi_ = dst_roi;
    return;
}

void qtk_blender_no_feed(qtk_blender_no_t *blender, cv::InputArray &_img, cv::InputArray &_mask, cv::Point &tl)
{
    cv::Mat img = _img.getMat();
    cv::Mat mask = _mask.getMat();
    cv::Mat dst = blender->dst_.getMat(cv::ACCESS_RW);

    int dx = tl.x - blender->dst_roi_.x;
    int dy = tl.y - blender->dst_roi_.y;

    for (int y = 0; y < img.rows; ++y)
    {
        const cv::Point3_<uchar> *src_row = img.ptr<cv::Point3_<uchar> >(y);
        cv::Point3_<uchar> *dst_row = dst.ptr<cv::Point3_<uchar> >(dy + y);
        const uchar *mask_row = mask.ptr<uchar>(y);

        for (int x = 0; x < img.cols; ++x)
        {
            if (mask_row[x])
                dst_row[dx + x] = src_row[x];
        }
    }
}

void qtk_blender_no_feed2(qtk_blender_no_t *blender, cv::InputArray &_img, cv::InputArray &_mask, cv::Point &tl)
{
    cv::Mat img = _img.getMat();
    cv::Mat mask = _mask.getMat();
    cv::Mat dst = blender->dst_.getMat(cv::ACCESS_RW);

    int dx = tl.x - blender->dst_roi_.x;
    int dy = tl.y - blender->dst_roi_.y;

    const uchar *src_row = img.ptr<uchar>();
    for (int y = 0; y < img.rows; ++y)
    {
        uchar *dst_row = dst.ptr<uchar>(dy+y,dx);
        const uchar *mask_row = mask.ptr<uchar>(y);
        
        for (int x = 0; x < img.cols; ++x)
        {
            if (mask_row[x]){
                memcpy(dst_row, src_row, 3);
#ifdef USE_9391
                dst_row[3] = 255;
#endif
            }
            src_row += 3;
            dst_row+=BLEND_DST_CHANNEL;
        }
    }
}

void qtk_blender_no_feed2_1channel(qtk_blender_no_t *blender, cv::InputArray &_img, cv::InputArray &_mask, 
                            cv::Point &tl,cv::InputArray &_yoffset)
{
    cv::Mat img = _img.getMat();
    cv::Mat mask = _mask.getMat();
    cv::Mat dst = blender->dst_.getMat(cv::ACCESS_RW);
    cv::Mat yoffset = _yoffset.getMat();
    int cols = mask.cols;
    int rows = mask.rows;
    int *yoffsetp = yoffset.ptr<int>();
    uchar *dst_row = dst.ptr<uchar>();
    uchar *dst_row_p = NULL;
    const uchar *mask_row = mask.ptr<uchar>();
    const uchar *src_row = img.ptr<uchar>();

    for (int y = 0; y < rows; ++y)
    {
        dst_row_p = dst_row+yoffsetp[y];
        for (int x = 0; x < cols; ++x)
        {
            if (*mask_row){
                *dst_row_p = *src_row;
            }
            ++src_row;
            ++dst_row_p;
            ++mask_row;
        }
    }
    return;
}

void qtk_blender_no_feed2_sp_2channel(qtk_blender_no_t *feather, cv::InputArray &_img, cv::InputArray &mask, 
                    cv::Point tl,cv::InputArray &_yoffset)
{
    cv::Mat img = _img.getMat();
    cv::Mat dst = feather->dst_.getMat(cv::ACCESS_RW);
    cv::Mat yoffset = _yoffset.getMat();
    cv::Mat mask_map = mask.getMat();
    int rows = feather->dst_roi_.height;

    const u_char *src_row = img.ptr(0,0);
    u_char *dst_row_uv = dst.ptr<u_char>(rows);
    u_char *dst_row = NULL;
    const u_char* mask_row = NULL;
    int *yoffsetp = yoffset.ptr<int>();
    int n1 = img.rows;
    int n2 = img.cols;
    // printf("yoffsetp %d\n",*yoffsetp);
    for(int y = 0; y < n1; ++y,++yoffsetp){
        dst_row = dst_row_uv + *yoffsetp;
        mask_row = mask_map.ptr(y*2,1);
        for(int x = 0; x < n2; ++x){
            if(*mask_row){
                dst_row[0] = src_row[0];
                dst_row[1] = src_row[1];
            }
            src_row += 2;
            dst_row += 2;
            mask_row += 2;
        }
    }
    return;
}

void qtk_blender_no_feed2_1channel_rect(qtk_blender_no_t *blender, cv::InputArray &_img, cv::InputArray &_mask, 
                            cv::Point &tl,cv::InputArray &_yoffset,int s,int e)
{
    cv::Mat img = _img.getMat();
    cv::Mat mask = _mask.getMat();
    cv::Mat dst = blender->dst_.getMat(cv::ACCESS_RW);
    cv::Mat yoffset = _yoffset.getMat();
    int cols = mask.cols;
    int *yoffsetp = yoffset.ptr<int>(0,s);
    uchar *dst_row = dst.ptr<uchar>();
    uchar *dst_row_p = NULL;
    const uchar *mask_row = mask.ptr<uchar>(s);
    const uchar *src_row = img.ptr<uchar>(s);

    for (int y = s; y < e; ++y,++yoffsetp)
    {
        dst_row_p = dst_row+(*yoffsetp);
        for (int x = 0; x < cols; ++x)
        {
            if (*mask_row){
                *dst_row_p = *src_row;
            }
            ++src_row;
            ++dst_row_p;
            ++mask_row;
        }
    }
    return;
}

void qtk_blender_no_feed2_sp_2channel_rect(qtk_blender_no_t *blender, cv::InputArray &_img, cv::InputArray &mask, 
                    cv::Point tl,cv::InputArray &_yoffset,int s, int e)
{
    cv::Mat img = _img.getMat();
    cv::Mat dst = blender->dst_.getMat(cv::ACCESS_RW);
    cv::Mat yoffset = _yoffset.getMat();
    cv::Mat mask_map = mask.getMat();
    int rows = blender->dst_roi_.height;

    const u_char *src_row = img.ptr(s,0);
    u_char *dst_row_uv = dst.ptr<u_char>(rows);
    u_char *dst_row = NULL;
    const u_char* mask_row = NULL;
    int *yoffsetp = yoffset.ptr<int>(0,s);
    int n2 = img.cols;
    // printf("yoffsetp %d\n",*yoffsetp);
    for(int y = s; y < e; ++y,++yoffsetp){
        dst_row = dst_row_uv + *yoffsetp;
        mask_row = mask_map.ptr(y*2,1);
        for(int x = 0; x < n2; ++x){
            if(*mask_row){
                dst_row[0] = src_row[0];
                dst_row[1] = src_row[1];
            }
            src_row += 2;
            dst_row += 2;
            mask_row += 2;
        }
    }
    return;
}

//计算个权重
void qtk_blender_no_weightmaps(qtk_blender_no_t *blender,const std::vector<cv::UMat> &masks, 
                    const std::vector<cv::Point> &corners,std::vector<cv::UMat> &weight_maps)
{
    weight_maps.clear();
    blender->weight_maps_.clear();
    for (size_t i = 0; i < masks.size(); i++)
    {
        cv::UMat weight_map;
        cv::threshold(masks[i], weight_map, 1, 255, cv::THRESH_TRUNC);
        weight_maps.push_back(weight_map);
        blender->weight_maps_.push_back(weight_map);
    }
    return;
}

//把曝光补偿加进来
void qtk_blender_no_weight_maps_update(qtk_blender_no_t *blender,std::vector<cv::Mat> inupmaps,
                                                std::vector<cv::UMat> &weight_maps)
{
    if(blender->weight_maps_.size() != inupmaps.size()){
        return;
    }
    weight_maps.clear();
    weight_maps.resize(inupmaps.size());
    for(size_t i = 0; i < blender->weight_maps_.size(); ++i){
        if(inupmaps[i].channels() == 3){
            std::vector<cv::UMat> gains_channels;
            cv::UMat u_gain_map;
            gains_channels.push_back(blender->weight_maps_[i]);
            gains_channels.push_back(blender->weight_maps_[i]);
            gains_channels.push_back(blender->weight_maps_[i]);
            merge(gains_channels, u_gain_map);
            cv::multiply(u_gain_map, inupmaps[i], weight_maps[i], 1, CV_32F);
        }else{
            cv::multiply(blender->weight_maps_[i], inupmaps[i], weight_maps[i], 1, CV_32F);
        }
    }
    //定点化
    double dd = pow(2,QTK_STITCH_FIX_POINT);
    for(size_t i = 0; i < weight_maps.size(); ++i){
        weight_maps[i] = weight_maps[i].mul(dd);
        // cv::subtract(weight_maps[i], cv::Scalar::all(0.5), weight_maps[i]);
        cv::UMat out;
        if(weight_maps[i].channels() == 3){
            weight_maps[i].convertTo(out, CV_16UC3);
        }else{
            weight_maps[i].convertTo(out, CV_16U);
        }
        weight_maps[i] = out;
    }
    return;
}

void qtk_blender_no_feed2_exposure(qtk_blender_no_t *blender, cv::InputArray &_img, cv::InputArray &_weight_map, cv::Point &tl,
                            cv::InputArray &_yoffset)
{
    cv::Mat img = _img.getMat();
    cv::Mat dst = blender->dst_.getMat(cv::ACCESS_RW);
    cv::Mat yoffset = _yoffset.getMat();

    cv::Mat weight_map = _weight_map.getMat();

    const u_char *src_row = img.data;
    u_char *dst_row = dst.ptr<u_char>();
    const ushort* weight_row = weight_map.ptr<ushort>();
    int *yoffsetp = yoffset.ptr<int>();
    int n1 = img.rows;
    int n2 = img.cols;
    if(weight_map.channels() != 3){
        for(int y = 0; y < n1; ++y,++yoffsetp){
            // dst_row = dst.data + (y * dst_cols + dx) * BLEND_DST_CHANNEL;
            dst_row = (u_char*)dst.data + *yoffsetp;
            for(int x = 0; x < n2; ++x){
                ushort weight = *weight_row;
                if(weight){
                    dst_row[0] += static_cast<u_char>((src_row[0] * weight)>>QTK_STITCH_FIX_POINT);
                    dst_row[1] += static_cast<u_char>((src_row[1] * weight)>>QTK_STITCH_FIX_POINT);
                    dst_row[2] += static_cast<u_char>((src_row[2] * weight)>>QTK_STITCH_FIX_POINT);
                #ifdef USE_9391
                    dst_row[3] = 255;
                #endif
                }
                src_row += 3;
                dst_row += BLEND_DST_CHANNEL;
                ++weight_row;
            }
        }
    }else{
        for(int y = 0; y < n1; ++y,++yoffsetp){
            // dst_row = (u_short*)dst.data + (y * dst_cols + dx) * BLEND_DST_CHANNEL;
            dst_row = (u_char*)dst.data + *yoffsetp;
            for(int x = 0; x < n2; ++x){
                ushort weight1 = *weight_row;
                if(weight1){
                    ushort weight2 = *(weight_row+1);
                    ushort weight3 = *(weight_row+2);
                    dst_row[0] += static_cast<u_char>((src_row[0] * weight1)>>QTK_STITCH_FIX_POINT);
                    dst_row[1] += static_cast<u_char>((src_row[1] * weight2)>>QTK_STITCH_FIX_POINT);
                    dst_row[2] += static_cast<u_char>((src_row[2] * weight3)>>QTK_STITCH_FIX_POINT);
                #ifdef USE_9391
                    dst_row[3] = 255;
                #endif
                }
                src_row += 3;
                dst_row += BLEND_DST_CHANNEL;
                weight_row += 3;
            }
        }
    }
    return;
}

void qtk_blender_no_feed2_1channel_exposure(qtk_blender_no_t *feather, cv::InputArray &_img, cv::Point tl, 
                    cv::InputArray &_weight_map, cv::InputArray &_yoffset)
{
    cv::Mat img = _img.getMat();
    cv::Mat dst = feather->dst_.getMat(cv::ACCESS_RW);
    cv::Mat yoffset = _yoffset.getMat();
    cv::Mat weight_map = _weight_map.getMat();

    const u_char *src_row = img.data;
    u_char *dst_row = dst.ptr<u_char>();
    const ushort* weight_row = weight_map.ptr<ushort>();
    int *yoffsetp = yoffset.ptr<int>();
    int n1 = weight_map.rows;
    int n2 = weight_map.cols;
    // printf("yoffsetp %d\n",*yoffsetp);
    for(int y = 0; y < n1; ++y,++yoffsetp){
        dst_row = (u_char*)dst.data + *yoffsetp;
        for(int x = 0; x < n2; ++x){
            ushort weight = *weight_row;
            if(weight){
                dst_row[0] += static_cast<u_char>((src_row[0] * weight)>>QTK_STITCH_FIX_POINT);
            }
            src_row += 1;
            dst_row += 1;
            ++weight_row;
        }
    }
    return;
}


void qtk_blender_no_blend(qtk_blender_no_t *blender,cv::InputOutputArray dst, cv::InputOutputArray dst_mask)
{
    cv::UMat mask;
    dst.assign(blender->dst_);
    // dst_mask.assign(blender->dst_mask_);
    blender->dst_.release();
    // blender->dst_mask_.release();
}


void qtk_blender_no_feed2_rect(qtk_blender_no_t *blender, cv::InputArray &_img, cv::InputArray &_mask, cv::Point &tl,
                                        int s, int e)
{
    cv::Mat img = _img.getMat();
    cv::Mat mask = _mask.getMat();
    cv::Mat dst = blender->dst_.getMat(cv::ACCESS_RW);

    int dx = tl.x - blender->dst_roi_.x;
    int dy = tl.y - blender->dst_roi_.y;

    const uchar *src_row = img.ptr<uchar>(s);
    for (int y = s; y < e; ++y)
    {
        uchar *dst_row = dst.ptr<uchar>(dy+y,dx);
        const uchar *mask_row = mask.ptr<uchar>(y);
        
        for (int x = 0; x < img.cols; ++x)
        {
            if (mask_row[x]){
                memcpy(dst_row, src_row, 3);
#ifdef USE_9391
                dst_row[3] = 255;
#endif
            }
            src_row += 3;
            dst_row+=BLEND_DST_CHANNEL;
        }
    }
}

void qtk_blender_no_yoffsetmaps(qtk_blender_no_t *blender_no,const std::vector<cv::UMat> &uxs, 
                    cv::Rect dst_roi,std::vector<cv::Point> &corners,std::vector<cv::Mat> &yoffset_maps, int channel)
{
    int n = corners.size();
    for(int i = 0; i < n; ++i){
        int cols = uxs[i].rows;
        cv::Mat yoffset_map(1,cols,CV_32S);
        for(int j = 0; j < cols; ++j){
            yoffset_map.at<int>(j) = (j*dst_roi.size().width+(corners[i].x - dst_roi.x))*channel;
        }
        yoffset_maps.push_back(yoffset_map);
    }
}

void qtk_blender_no_uv_yoffsetmaps(qtk_blender_no_t *blender_no,const std::vector<cv::UMat> &uxs, 
                    cv::Rect dst_roi,std::vector<cv::Point> &corners,std::vector<cv::Mat> &yoffset_maps)
{
    int n = corners.size();
    for(int i = 0; i < n; ++i){
        int cols = uxs[i].rows/2;
        cv::Mat yoffset_map(1,cols,CV_32S);
        // printf("%d %d %d %d\n",cols,dst_roi.size().width,corners[i].x,dst_roi.x);
        for(int j = 0; j < cols; ++j){
            int k = j*dst_roi.size().width+(corners[i].x - dst_roi.x);
            yoffset_map.at<int>(j) = k+(k%2);
        }
        yoffset_maps.push_back(yoffset_map);
    }
}

// void qtk_blender_no_uv_seammasks(qtk_blender_no_t *blender_no,const std::vector<cv::UMat> &uxs, std::vector<cv::UMat> &uv_seammamsk)
// {
//     int n = uxs.size();
//     for(int i = 0; i < n; ++i){
//         int cols = uxs[i].rows/2;
//         cv::Mat yoffset_map(1,cols,CV_32S);
//         // printf("%d %d %d %d\n",cols,dst_roi.size().width,corners[i].x,dst_roi.x);
//         for(int j = 0; j < cols; ++j){
//             int k = j*dst_roi.size().width+(corners[i].x - dst_roi.x);
//             yoffset_map.at<int>(j) = k+(k%2);
//         }
//         yoffset_maps.push_back(yoffset_map);
//     }
// }
static inline u_char _sx_to(ushort t)
{
    u_char tt = t;
    if(t > 255){
        tt = 255;
    }
    return tt;
}
void qtk_blender_no_feed2_exposure_rect(qtk_blender_no_t *blender, cv::InputArray _img, cv::Point tl, 
                    cv::InputArray _weight_map, cv::InputArray _yoffset,int s, int e)
{
    cv::Mat img = _img.getMat();
    cv::Mat dst = blender->dst_.getMat(cv::ACCESS_RW);
    cv::Mat yoffset = _yoffset.getMat();
    cv::Mat weight_map = _weight_map.getMat();

    const u_char *src_row = img.ptr<u_char>(s);
    u_char *dst_row = dst.ptr<u_char>();
    const ushort* weight_row = weight_map.ptr<ushort>(s,0);
    int *yoffsetp = yoffset.ptr<int>(0,s);
    int n2 = img.cols;
    if(weight_map.channels() != 3){
        for(int y = s; y < e; ++y,++yoffsetp){
            dst_row = (u_char*)dst.data + *yoffsetp;
            for(int x = 0; x < n2; ++x){
                ushort weight = *weight_row;
                if(weight){
                    dst_row[0] = _sx_to((src_row[0] * weight)>>QTK_STITCH_FIX_POINT);
                    dst_row[1] = _sx_to((src_row[1] * weight)>>QTK_STITCH_FIX_POINT);
                    dst_row[2] = _sx_to((src_row[2] * weight)>>QTK_STITCH_FIX_POINT);
                #ifdef USE_9391
                    dst_row[3] = 255;
                #endif
                }
                src_row += 3;
                dst_row += BLEND_DST_CHANNEL;
                ++weight_row;
            }
        }
    }else{
        for(int y = s; y < e; ++y,++yoffsetp){
            // dst_row = (u_short*)dst.data + (y * dst_cols + dx) * BLEND_DST_CHANNEL;
            dst_row = (u_char*)dst.data + *(yoffsetp);
            for(int x = 0; x < n2; ++x){
                ushort weight1 = *weight_row;
                if(weight1){
                    ushort weight2 = *(weight_row+1);
                    ushort weight3 = *(weight_row+2);
                    dst_row[0] = _sx_to((src_row[0] * weight1)>>QTK_STITCH_FIX_POINT);
                    dst_row[1] = _sx_to((src_row[1] * weight2)>>QTK_STITCH_FIX_POINT);
                    dst_row[2] = _sx_to((src_row[2] * weight3)>>QTK_STITCH_FIX_POINT);
                #ifdef USE_9391
                    dst_row[3] = 255;
                #endif
                }
                src_row += 3;
                dst_row += BLEND_DST_CHANNEL;
                weight_row += 3;
            }
        }
    }
    return;
}

void qtk_blender_no_feed2_1channel_exposure_rect(qtk_blender_no_t *feather, cv::InputArray &_img, cv::Point tl, 
                    cv::InputArray &_weight_map, cv::InputArray &_yoffset, int s, int e)
{
    cv::Mat img = _img.getMat();
    cv::Mat dst = feather->dst_.getMat(cv::ACCESS_RW);
    cv::Mat yoffset = _yoffset.getMat();
    cv::Mat weight_map = _weight_map.getMat();

    const u_char *src_row = img.ptr<u_char>(s);
    u_char *dst_row = dst.ptr<u_char>();
    const ushort* weight_row = weight_map.ptr<ushort>(s,0);
    int *yoffsetp = yoffset.ptr<int>(0,s);
    int n2 = weight_map.cols;
    // printf("yoffsetp %d\n",*yoffsetp);
    for(int y = s; y < e; ++y,++yoffsetp){
        dst_row = (u_char*)dst.data + *yoffsetp;
        int x = 0;
#ifdef USE_NEON
        // int cross_n2 = n2>>3<<3;
        // for(;x < cross_n2;x+=8){
        //     uint8x8_t Sv = vld1_u8(src_row);
        //     uint8x8_t Wv = vld1_u8(weight_row);
        //     uint16x8_t Swv = vmull_u8(Sv, Wv);
        //     uint8x8_t Sdv = vqrshrn_n_u16(Swv,QTK_STITCH_FIX_POINT);
        //     vst1_u8(dst_row, Sdv);
            
        //     src_row+=8;
        //     weight_row+=8;
        //     dst_row+=8;
        // }
#endif
        for(;x < n2; ++x){
            ushort weight = *weight_row;
            if(weight){
                dst_row[0] = _sx_to((src_row[0] * weight)>>QTK_STITCH_FIX_POINT);
            }
            ++src_row;
            ++dst_row;
            ++weight_row;
        }
    }
    return;
}