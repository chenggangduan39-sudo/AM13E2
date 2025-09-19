#include "qtk_blender_feather.h"
#include <iostream>
#include <math.h>
#include "qtk/stitch/qtk_stitch_def.h"
#include "opencv2/stitching/detail/util.hpp"
#include "opencv2/stitching/detail/blenders.hpp"
#ifdef USE_OPENMP
#include <omp.h>
#endif

struct qtk_blender_feather{
    cv::UMat dst_, dst_mask_;
    cv::Rect dst_roi_;
    float sharpness_;
    float smooth;
    cv::UMat weight_map_; //原函数的
    cv::UMat dst_weight_map_;
    std::vector<cv::UMat> weight_maps_; //原始的
};

#ifdef USE_9391
#define BLEND_DST_CHANNEL 4
#else
#define BLEND_DST_CHANNEL 3
#endif

static inline u_char _alpah_sx_to(ushort t);

qtk_blender_feather_t *qtk_blender_feather_new(float sharpness, float smooth)
{
    qtk_blender_feather_t *feather = new qtk_blender_feather_t;
    feather->sharpness_ = sharpness;
    feather->smooth = smooth;
    return feather;
}

int qtk_blender_feather_delete(qtk_blender_feather_t *feather)
{
    delete feather;
    return 0;
}

void qtk_blender_feather_prepare(qtk_blender_feather_t *feather, cv::Rect dst_roi)
{
    // feather->dst_.create(dst_roi.size(), CV_16SC3);
    feather->dst_.setTo(cv::Scalar::all(0));
    // feather->dst_mask_.create(dst_roi.size(), CV_8U);
    // feather->dst_mask_.setTo(cv::Scalar::all(0));
    feather->dst_roi_ = dst_roi;
    // feather->dst_weight_map_.create(dst_roi.size(), CV_32F);
    // feather->dst_weight_map_.setTo(0);
    return;
}

void qtk_blender_feather_prepare2(qtk_blender_feather_t *feather, cv::Rect dst_roi, void *ptr)
{
#ifdef USE_9391
    feather->dst_ = cv::Mat(dst_roi.size(), CV_8UC4, ptr).getUMat(cv::ACCESS_RW);
#else
    feather->dst_ = cv::Mat(dst_roi.size(), CV_8UC3, ptr).getUMat(cv::ACCESS_RW);
#endif
    feather->dst_roi_ = dst_roi;
    return;
}

void qtk_blender_feather_nv12_prepare2(qtk_blender_feather_t *feather, cv::Rect dst_roi, void *ptr)
{
    int cols = dst_roi.width;
    int rows = dst_roi.height*1.5;
    feather->dst_ = cv::Mat(cv::Size(cols,rows), CV_8UC1, ptr).getUMat(cv::ACCESS_RW);
    feather->dst_roi_ = dst_roi;
    return;
}

// void save_write(cv::UMat weight_map)
// {
//     static int indix = 0;
//     char path[125] = {0};
//     sprintf(path,"%d.xml",indix);
//     FileStorage fs(path,FileStorage::WRITE);
//     fs.write("weight_map",weight_map.getMat(ACCESS_RW));
//     fs.release();
//     indix++;
//     return;
// }
//原始函数
void qtk_blender_feather_feed(qtk_blender_feather_t *feather, cv::InputArray _img, 
                                cv::InputArray mask, cv::Point tl)
{
    cv::Mat img = _img.getMat();
    cv::Mat dst = feather->dst_.getMat(cv::ACCESS_RW);

    CV_Assert(img.type() == CV_16SC3);
    CV_Assert(mask.type() == CV_8U);

    cv::detail::createWeightMap(mask, feather->sharpness_, feather->weight_map_);
    // save_write(weight_map_);
    cv::Mat weight_map = feather->weight_map_.getMat(cv::ACCESS_READ);
    cv::Mat dst_weight_map = feather->dst_weight_map_.getMat(cv::ACCESS_RW);

    int dx = tl.x - feather->dst_roi_.x;
    int dy = tl.y - feather->dst_roi_.y;

    for (int y = 0; y < img.rows; ++y)
    {
        const cv::Point3_<short>* src_row = img.ptr<cv::Point3_<short> >(y);
        cv::Point3_<short>* dst_row = dst.ptr<cv::Point3_<short> >(dy + y);
        const float* weight_row = weight_map.ptr<float>(y);
        float* dst_weight_row = dst_weight_map.ptr<float>(dy + y);

        for (int x = 0; x < weight_map.cols; ++x)
        {
            dst_row[dx + x].x += static_cast<short>(src_row[x].x * weight_row[x]);
            dst_row[dx + x].y += static_cast<short>(src_row[x].y * weight_row[x]);
            dst_row[dx + x].z += static_cast<short>(src_row[x].z * weight_row[x]);
            dst_weight_row[dx + x] += weight_row[x];
        }
    }
}
//提取出了权重的
void qtk_blender_feather_feed2(qtk_blender_feather_t *feather, cv::InputArray _img, cv::InputArray mask, 
                    cv::Point tl, cv::InputArray _weight_map, cv::InputArray _yoffset)
{
    cv::Mat img = _img.getMat();
    cv::Mat dst = feather->dst_.getMat(cv::ACCESS_RW);
    cv::Mat yoffset = _yoffset.getMat();

    // CV_Assert(img.type() == CV_16SC3);
    // CV_Assert(mask.type() == CV_8U);

    // createWeightMap(mask, sharpness_, weight_map_);
    cv::Mat weight_map = _weight_map.getMat();
    // cv::Mat dst_weight_map = feather->dst_weight_map_.getMat(cv::ACCESS_RW);

    // printf("%d %d %d\n",dst.isContinuous(),img.isContinuous(),weight_map.isContinuous());
#if 1
    // int dst_cols = dst.cols;
    // int dst_rows = dst.rows;
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
                dst_row[0] += static_cast<u_char>((src_row[0] * weight)>>QTK_STITCH_FIX_POINT);
                dst_row[1] += static_cast<u_char>((src_row[1] * weight)>>QTK_STITCH_FIX_POINT);
                dst_row[2] += static_cast<u_char>((src_row[2] * weight)>>QTK_STITCH_FIX_POINT);
    #ifdef USE_9391
                dst_row[3] = 255;
    #endif
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
                ushort weight2 = *(weight_row+1);
                ushort weight3 = *(weight_row+2);
                dst_row[0] += static_cast<u_char>((src_row[0] * weight1)>>QTK_STITCH_FIX_POINT);
                dst_row[1] += static_cast<u_char>((src_row[1] * weight2)>>QTK_STITCH_FIX_POINT);
                dst_row[2] += static_cast<u_char>((src_row[2] * weight3)>>QTK_STITCH_FIX_POINT);
    #ifdef USE_9391
                dst_row[3] = 255;
    #endif
                src_row += 3;
                dst_row += BLEND_DST_CHANNEL;
                weight_row += 3;
            }
        }
    }
#else
    int dx = tl.x - feather->dst_roi_.x;
    int dy = tl.y - feather->dst_roi_.y;
    for (int y = 0; y < img.rows; ++y)
    {
        const cv::Point3_<u_char>* src_row = img.ptr<cv::Point3_<u_char> >(y);
        cv::Point3_<u_char>* dst_row = dst.ptr<cv::Point3_<u_char> >(dy + y);
        const ushort* weight_row = weight_map.ptr<ushort>(y);
        // float* dst_weight_row = dst_weight_map.ptr<float>(dy + y);

        for (int x = 0; x < img.cols; ++x)
        {
            dst_row[dx + x].x += static_cast<u_char>((src_row[x].x * weight_row[x])>>QTK_STITCH_FIX_POINT);
            dst_row[dx + x].y += static_cast<u_char>((src_row[x].y * weight_row[x])>>QTK_STITCH_FIX_POINT);
            dst_row[dx + x].z += static_cast<u_char>((src_row[x].z * weight_row[x])>>QTK_STITCH_FIX_POINT);
            // dst_weight_row[dx + x] += weight_row[x];
        }
    }
#endif
}

//sp 的一个面 比如420sp的y面
void qtk_blender_feather_feed2_1channal(qtk_blender_feather_t *feather, cv::InputArray &_img, cv::InputArray &mask, 
                    cv::Point tl, cv::InputArray &_weight_map, cv::InputArray &_yoffset)
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
            dst_row[0] += _alpah_sx_to((src_row[0] * weight)>>QTK_STITCH_FIX_POINT);
            src_row += 1;
            dst_row += 1;
            ++weight_row;
        }
    }
    return;
}

//默认 img 和 weight_map 大小一致
void qtk_blender_feather_feed2_table_1channal(qtk_blender_feather_t *feather, cv::InputArray &_img, cv::InputArray &_weight_map, cv::InputArray &_table)
{
    cv::Mat img = _img.getMat();
    cv::Mat dst = feather->dst_.getMat(cv::ACCESS_RW);
    cv::Mat weight_map = _weight_map.getMat();
    cv::Mat table = _table.getMat();

    const u_char *src_row = img.data;
    u_char *dst_row = dst.ptr<u_char>();
    const ushort* weight_row = weight_map.ptr<ushort>();
    const int *table_p = table.ptr<int>();
    int n = table.rows;
    for(int i = 0; i < n; ++i){
        int type = table_p[0];
        int tl = table_p[1];
        const ushort* ws = weight_row+table_p[2];
        const u_char* is = src_row+table_p[2];
        u_char *ds = dst_row+table_p[3];
        if(type == 1){
            for(int j = 0;j < tl; ++j){
                ds[0] += is[0];
                ++ds;
                ++is;
            }
        }else if(type == 2){
            for(int j = 0;j < tl; ++j){
                ds[0] = _alpah_sx_to((is[0] * ws[0])>>QTK_STITCH_FIX_POINT)+ds[0];
                ++ds;
                ++is;
                ++ws;
            }
        }
        table_p+=4;
    }

    return;
}

void qtk_blender_feather_feed2_sp_2channal(qtk_blender_feather_t *feather, cv::InputArray &_img, cv::InputArray &mask, 
                    cv::Point tl, cv::InputArray &_weight_map, cv::InputArray &_yoffset)
{
    cv::Mat img = _img.getMat();
    cv::Mat dst = feather->dst_.getMat(cv::ACCESS_RW);
    cv::Mat yoffset = _yoffset.getMat();
    cv::Mat weight_map = _weight_map.getMat();
    int rows = feather->dst_roi_.height;

    const u_char *src_row = img.ptr(0,0);
    u_char *dst_row_uv = dst.ptr<u_char>(rows);
    u_char *dst_row = NULL;
    const ushort* weight_row = weight_map.ptr<ushort>();
    int *yoffsetp = yoffset.ptr<int>();
    int n1 = weight_map.rows;
    int n2 = weight_map.cols;
    // printf("yoffsetp %d\n",*yoffsetp);
    for(int y = 0; y < n1; ++y,++yoffsetp){
        dst_row = dst_row_uv + *yoffsetp;
        for(int x = 0; x < n2; ++x){
            // u_char weight = *weight_row;
            // if(weight){
            //     // dst_row[0] += static_cast<u_char>((src_row[0] * weight+128*((1<<7)-weight))>>7);
            //     // dst_row[1] += static_cast<u_char>((src_row[1] * weight+128*((1<<7)-weight))>>7);
            //     dst_row[0] = src_row[0];
            //     dst_row[1] = src_row[1];
            // }
            ushort weight = *weight_row;
            short k1 = src_row[0]-128; // 这个效果好点
            short k2 = src_row[1]-128;
            dst_row[0] = dst_row[0] + static_cast<u_char>((k1 * weight)>>QTK_STITCH_FIX_POINT);
            dst_row[1] = dst_row[1] + static_cast<u_char>((k2 * weight)>>QTK_STITCH_FIX_POINT);
            src_row += 2;
            dst_row += 2;
            ++weight_row;
        }
    }
    return;
}

void qtk_blender_feather_feed2_table_2channal(qtk_blender_feather_t *feather, cv::InputArray &_img, cv::InputArray &_weight_map, cv::InputArray &_table)
{
    cv::Mat img = _img.getMat();
    cv::Mat dst = feather->dst_.getMat(cv::ACCESS_RW);
    cv::Mat weight_map = _weight_map.getMat();
    cv::Mat table = _table.getMat();
    int rows = feather->dst_roi_.height;

    const u_char *src_row = img.data;
    u_char *dst_row_uv = dst.ptr<u_char>(rows);
    const ushort* weight_row = weight_map.ptr<ushort>();
    const int *table_p = table.ptr<int>();
    int n = table.rows;
    for(int i = 0; i < n; ++i){
        int type = table_p[0];
        int tl = table_p[1];
        const ushort* ws = weight_row+table_p[2];
        const u_char* is = src_row+table_p[2]*2;
        u_char *ds = dst_row_uv+table_p[3];
        if(type == 1){
            for(int j = 0;j < tl; ++j){
                short k1 = is[0]-128;
                short k2 = is[1]-128;
                ds[0] = ds[0] + k1;
                ds[1] = ds[1] + k2;
                ds+=2;
                is+=2;
            }
        }else if(type == 2){
            for(int j = 0;j < tl; ++j){
                ushort weight = ws[0];
                short k1 = is[0]-128;
                short k2 = is[1]-128;
                ds[0] = ((k1 * weight)>>QTK_STITCH_FIX_POINT)+ds[0];
                ds[1] = ((k2 * weight)>>QTK_STITCH_FIX_POINT)+ds[1];
                ds+=2;
                is+=2;
                ++ws;
            }
        }
        table_p+=4;
    }

    return;
}

inline u_char _alpah_x_to(float t)
{
    u_char tt = 0;
    if(t > 255){
        tt = 255;
    }else if(t < 0){
        tt = 0;
    }else{
        tt = t;
    }
    return tt;
}

static inline u_char _alpah_sx_to(ushort t)
{
    u_char tt = t;
    if(t > 255){
        tt = 255;
    }
    return tt;
}

void qtk_blender_feather_feed3(qtk_blender_feather_t *feather, cv::InputArray _img, cv::InputArray mask, 
                    cv::Point tl, cv::InputArray _weight_map, cv::InputArray _yoffset, 
                    char alpha,char beta, char gamma,float alpha_x,float beta_x,float gamma_x)
{
    cv::Mat img = _img.getMat();
    cv::Mat dst = feather->dst_.getMat(cv::ACCESS_RW);
    cv::Mat yoffset = _yoffset.getMat();

    // CV_Assert(img.type() == CV_16SC3);
    CV_Assert(mask.type() == CV_8U);

    // createWeightMap(mask, sharpness_, weight_map_);
    cv::Mat weight_map = _weight_map.getMat();
    // cv::Mat dst_weight_map = feather->dst_weight_map_.getMat(cv::ACCESS_RW);

    // printf("%d %d %d\n",dst.isContinuous(),img.isContinuous(),weight_map.isContinuous());
#if 1
    // int dst_cols = dst.cols;
    // int dst_rows = dst.rows;
    const u_char *src_row = img.data;
    u_char *dst_row = dst.ptr<u_char>();
    const ushort* weight_row = weight_map.ptr<ushort>();
    int *yoffsetp = yoffset.ptr<int>();
    int n1 = img.rows;
    int n2 = img.cols;
    float d1 = 0, d2 = 0, d3 = 0;
    // printf("%d %d %d %f %f %f\n",alpha,beta,gamma,alpha_x,beta_x,gamma_x);
    for(int y = 0; y < n1; ++y,++yoffsetp){
        // dst_row = dst.data + (y * dst_cols + dx) * BLEND_DST_CHANNEL;
        dst_row = dst.data + *yoffsetp;
        for(int x = 0; x < n2; ++x){
            ushort weight = *weight_row;
            // dst_row[0] += static_cast<u_char>(((src_row[0] * weight)>>7)*alpha_x+alpha);
            // dst_row[1] += static_cast<u_char>(((src_row[1] * weight)>>7)*beta_x+beta);
            // dst_row[2] += static_cast<u_char>(((src_row[2] * weight)>>7)*gamma_x+gamma);
            // dst_row[0] += static_cast<u_char>(((src_row[0] * weight)>>7));
            // dst_row[1] += static_cast<u_char>(((src_row[1] * weight)>>7));
            // dst_row[2] += static_cast<u_char>(((src_row[2] * weight)>>7));
            d1 = _alpah_sx_to(dst_row[0]+((src_row[0] * weight)>>QTK_STITCH_FIX_POINT));
            d2 = _alpah_sx_to(dst_row[1]+((src_row[1] * weight)>>QTK_STITCH_FIX_POINT));
            d3 = _alpah_sx_to(dst_row[2]+((src_row[2] * weight)>>QTK_STITCH_FIX_POINT));
            if(dst_row[0] == 0 || dst_row[1] == 0 || dst_row[2] == 0){
                d1 = d1*alpha_x+alpha;
                d2 = d2*beta_x+beta;
                d3 = d3*gamma_x+gamma;
            }
            dst_row[0] = _alpah_x_to(d1);
            dst_row[1] = _alpah_x_to(d2);
            dst_row[2] = _alpah_x_to(d3);
#ifdef USE_9391
            dst_row[3] = 255;
#endif
            src_row += 3;
            dst_row += BLEND_DST_CHANNEL;
            ++weight_row;
        }
    }
#else
    int dx = tl.x - feather->dst_roi_.x;
    int dy = tl.y - feather->dst_roi_.y;
    for (int y = 0; y < img.rows; ++y)
    {
        const cv::Point3_<u_char>* src_row = img.ptr<cv::Point3_<u_char> >(y);
        cv::Point3_<u_char>* dst_row = dst.ptr<cv::Point3_<u_char> >(dy + y);
        const ushort* weight_row = weight_map.ptr<ushort>(y);
        // float* dst_weight_row = dst_weight_map.ptr<float>(dy + y);

        for (int x = 0; x < img.cols; ++x)
        {
            dst_row[dx + x].x += static_cast<u_char>((src_row[x].x * weight_row[x])>>QTK_STITCH_FIX_POINT);
            dst_row[dx + x].y += static_cast<u_char>((src_row[x].y * weight_row[x])>>QTK_STITCH_FIX_POINT);
            dst_row[dx + x].z += static_cast<u_char>((src_row[x].z * weight_row[x])>>QTK_STITCH_FIX_POINT);
            // dst_weight_row[dx + x] += weight_row[x];
        }
    }
#endif
}

extern "C"{
extern double time_get_ms();
};

void qtk_blender_feather_blend(qtk_blender_feather_t *feather, cv::InputOutputArray dst, 
                                    cv::InputOutputArray dst_mask)
{
    // cv::UMat mask;
    // cv::detail::normalizeUsingWeightMap(feather->dst_weight_map_, feather->dst_);
    // cv::compare(feather->dst_weight_map_, 1e-5f, feather->dst_mask_, cv::CMP_GT);
    // Blender::blend(dst, dst_mask);
    // cv::compare(feather->dst_mask_, 0, mask, cv::CMP_EQ);
    // feather->dst_.setTo(cv::Scalar::all(0), mask);
    // double tt = time_get_ms();
    dst.assign(feather->dst_);
    // printf("%lf\n",time_get_ms()-tt);
    // dst_mask.assign(feather->dst_mask_);
    // feather->dst_.release();
    // feather->dst_mask_.release();
    // printf("%lf\n",time_get_ms()-tt);
    return;
}


cv::Rect qtk_blender_feather_weightmaps(qtk_blender_feather_t *feather,const std::vector<cv::UMat> &masks, 
                    const std::vector<cv::Point> &corners,std::vector<cv::UMat> &weight_maps)
{
    weight_maps.resize(masks.size());
    feather->weight_maps_.resize(masks.size());
    for (size_t i = 0; i < masks.size(); ++i)
        cv::detail::createWeightMap(masks[i], feather->sharpness_, feather->weight_maps_[i]);

    cv::Rect dst_roi = cv::detail::resultRoi(corners, masks);
    cv::Mat weights_sum(dst_roi.size(), CV_32F);
    weights_sum.setTo(0);

    for (size_t i = 0; i < feather->weight_maps_.size(); ++i)
    {
        cv::Rect roi(corners[i].x - dst_roi.x, corners[i].y - dst_roi.y,
                 feather->weight_maps_[i].cols, feather->weight_maps_[i].rows);
        add(weights_sum(roi), feather->weight_maps_[i], weights_sum(roi));
    }
    for (size_t i = 0; i < feather->weight_maps_.size(); ++i)
    {
        cv::Rect roi(corners[i].x - dst_roi.x, corners[i].y - dst_roi.y,
                 feather->weight_maps_[i].cols, feather->weight_maps_[i].rows);
        cv::Mat tmp = weights_sum(roi);
        tmp+=1e-5;
        divide(feather->weight_maps_[i], tmp, feather->weight_maps_[i]);
        // printf("%d %d %d %d\n",feather->weight_maps_[i].cols,feather->weight_maps_[i].rows,
        //                 feather->weight_maps_[i].channels(),feather->weight_maps_[i].type());
    }
    //定点化
    double dd = pow(2,QTK_STITCH_FIX_POINT);
    for(size_t i = 0; i < feather->weight_maps_.size(); ++i){
        weight_maps[i] = feather->weight_maps_[i].mul(dd);
        cv::UMat out;
        weight_maps[i].convertTo(out, CV_16U);
        weight_maps[i] = out;
    }

    return dst_roi;
}

void _blender_feather_weight_ymap2uvmap(cv::UMat &weight_map, cv::UMat &out)
{
    int rows = out.rows;
    int colse = out.cols;
    cv::Mat w = weight_map.getMat(cv::ACCESS_READ);
    cv::Mat p = out.getMat(cv::ACCESS_RW);
    for(int i = 0; i < rows; ++i){
        for(int j = 0; j < colse; ++j){
            p.at<ushort>(i,j) = w.at<ushort>((i*2)+1,(j*2)+1);
        }
    }
    return;
}

void qtk_blender_feather_uv_weightmaps(qtk_blender_feather_t *feather, std::vector<cv::UMat> &weight_maps, 
                                            std::vector<cv::UMat> &uv_weight_maps)
{
    uv_weight_maps.resize(weight_maps.size());
    int size = weight_maps.size();
    for(int i = 0; i < size; ++i){
        uv_weight_maps[i] = cv::UMat(weight_maps[i].rows/2,weight_maps[i].cols/2,CV_16U);
        _blender_feather_weight_ymap2uvmap(weight_maps[i],uv_weight_maps[i]);
    }

    return;
}


// y * dst_cols + dx
void qtk_blender_feather_yoffsetmaps(qtk_blender_feather_t *feather,const std::vector<cv::UMat> &uxs, 
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

void qtk_blender_feather_uv_yoffsetmaps(qtk_blender_feather_t *feather,const std::vector<cv::UMat> &uxs, 
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

void qtk_blender_feather_weight_maps_update(qtk_blender_feather_t *feather,std::vector<cv::Mat> inupmaps,
                                                std::vector<cv::UMat> &weight_maps)
{
    if(feather->weight_maps_.size() != inupmaps.size()){
        return;
    }
    weight_maps.clear();
    weight_maps.resize(inupmaps.size());
    for(size_t i = 0; i < feather->weight_maps_.size(); ++i){
        if(inupmaps[i].channels() == 3){
            std::vector<cv::UMat> gains_channels;
            cv::UMat u_gain_map;
            gains_channels.push_back(feather->weight_maps_[i]);
            gains_channels.push_back(feather->weight_maps_[i]);
            gains_channels.push_back(feather->weight_maps_[i]);
            merge(gains_channels, u_gain_map);
            cv::multiply(u_gain_map, inupmaps[i], weight_maps[i], 1);
        }else{
            cv::multiply(feather->weight_maps_[i], inupmaps[i], weight_maps[i], 1);
        }
    }
    if(feather->smooth > 0){
        float max_weight = -1;
        for(size_t i = 0; i < weight_maps.size(); ++i){
            double dmax = 0.0;
            double dmin = 0.0;
            cv::minMaxLoc(inupmaps[i],&dmin,&dmax);
            max_weight = MAX(dmax,max_weight);
        }
        printf("max_weight %f \n",max_weight);
        if(max_weight > 0){
            float fn = feather->smooth/max_weight;
            for(size_t i = 0; i < weight_maps.size(); ++i){
                cv::multiply(weight_maps[i], cv::Scalar::all(fn), weight_maps[i], 1);
            }
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

void qtk_feather_set_smooth(qtk_blender_feather_t *feather,float smooth)
{
    feather->smooth = smooth;
    return;
}

// void qtk_feather_remap_feed(qtk_blender_feather_t *feather, cv::InputArray _img, cv::InputArray mask, 
//                     cv::Point tl, cv::InputArray _weight_map, cv::InputArray _yoffset,cv::Mat &offset_map)
// {
//     cv::Mat img = _img.getMat();
//     cv::Mat dst = feather->dst_.getMat(cv::ACCESS_RW);
//     cv::Mat yoffset = _yoffset.getMat();

//     cv::Mat weight_map = _weight_map.getMat();

//     const u_char *src_row = img.data;
//     const u_char *src = NULL;
//     u_char *dst_row = dst.ptr<u_char>();
//     const u_char* weight_row = weight_map.ptr<u_char>();
//     int *ofmp = offset_map.ptr<int>(0,0);
//     int *yoffsetp = yoffset.ptr<int>();
//     int n1 = offset_map.rows;
//     int n2 = offset_map.cols;

//     if(weight_map.channels() != 3){
//     //     for(int y = 0; y < n1; ++y,++yoffsetp){
//     //         dst_row = (u_char*)dst.data + *yoffsetp;
//     //         for(int x = 0; x < n2; ++x){
//     //             u_char weight = *weight_row;
//     //             dst_row[0] += static_cast<u_char>((src_row[0] * weight)>>7);
//     //             dst_row[1] += static_cast<u_char>((src_row[1] * weight)>>7);
//     //             dst_row[2] += static_cast<u_char>((src_row[2] * weight)>>7);
//     // #ifdef USE_9391
//     //             dst_row[3] = 255;
//     // #endif
//     //             src_row += 3;
//     //             dst_row += BLEND_DST_CHANNEL;
//     //             ++weight_row;
//     //         }
//     //     }
//     }else{
//         for(int y = 0; y < n1; ++y,++yoffsetp){
//             dst_row = (u_char*)dst.data + *yoffsetp;
//             for(int x = 0; x < n2; ++x){
//                 src = src_row+*ofmp;
//                 u_char weight1 = *weight_row;
//                 u_char weight2 = *(weight_row+1);
//                 u_char weight3 = *(weight_row+2);
//                 dst_row[0] += static_cast<u_char>((src[0] * weight1)>>7);
//                 dst_row[1] += static_cast<u_char>((src[1] * weight2)>>7);
//                 dst_row[2] += static_cast<u_char>((src[2] * weight3)>>7);
//     #ifdef USE_9391
//                 dst_row[3] = 255;
//     #endif
//                 dst_row += BLEND_DST_CHANNEL;
//                 weight_row += 3;
//                 ++ofmp;
//             }
//         }
//     }
//     return;
// }

void qtk_blender_feather_feed2_rect(qtk_blender_feather_t *feather, cv::InputArray _img, cv::InputArray mask, 
                    cv::Point tl, cv::InputArray _weight_map, cv::InputArray _yoffset,int s, int e)
{
    cv::Mat img = _img.getMat();
    cv::Mat dst = feather->dst_.getMat(cv::ACCESS_RW);
    cv::Mat yoffset = _yoffset.getMat();
    cv::Mat weight_map = _weight_map.getMat();

    const u_char *src_row = img.ptr<u_char>(s);
    u_char *dst_row = dst.ptr<u_char>();
    const ushort* weight_row = weight_map.ptr<ushort>(s,0);
    int *yoffsetp = yoffset.ptr<int>(0,s);
    int n2 = img.cols;
    if(weight_map.channels() != 3){
        for(int y = s; y < e; ++y,++yoffsetp){
            // dst_row = dst.data + (y * dst_cols + dx) * BLEND_DST_CHANNEL;
            dst_row = (u_char*)dst.data + *yoffsetp;
            for(int x = 0; x < n2; ++x){
                ushort weight = *weight_row;
                dst_row[0] += _alpah_sx_to((src_row[0] * weight)>>QTK_STITCH_FIX_POINT);
                dst_row[1] += _alpah_sx_to((src_row[1] * weight)>>QTK_STITCH_FIX_POINT);
                dst_row[2] += _alpah_sx_to((src_row[2] * weight)>>QTK_STITCH_FIX_POINT);
    #ifdef USE_9391
                dst_row[3] = 255;
    #endif
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
                ushort weight2 = *(weight_row+1);
                ushort weight3 = *(weight_row+2);
                dst_row[0] += _alpah_sx_to((src_row[0] * weight1)>>QTK_STITCH_FIX_POINT);
                dst_row[1] += _alpah_sx_to((src_row[1] * weight2)>>QTK_STITCH_FIX_POINT);
                dst_row[2] += _alpah_sx_to((src_row[2] * weight3)>>QTK_STITCH_FIX_POINT);
    #ifdef USE_9391
                dst_row[3] = 255;
    #endif
                src_row += 3;
                dst_row += BLEND_DST_CHANNEL;
                weight_row += 3;
            }
        }
    }
    return;
}

void qtk_blender_feather_feed2_1channal_rect(qtk_blender_feather_t *feather, cv::InputArray &_img, cv::InputArray &mask, 
                    cv::Point tl, cv::InputArray &_weight_map, cv::InputArray &_yoffset, int s, int e)
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
        //     uint8x8_t Dv = vld1_u8(dst_row);
        //     uint16x8_t Swv = vmull_u8(Sv, Wv);
        //     uint8x8_t Sdv = vqrshrn_n_u16(Swv,QTK_STITCH_FIX_POINT);
        //     uint8x8_t DD  = vadd_u8(Sdv, Dv);
        //     vst1_u8(dst_row, DD);
            
        //     src_row+=8;
        //     weight_row+=8;
        //     dst_row+=8;
        // }
#endif
        for(;x < n2; ++x){
            ushort weight = *weight_row;
            dst_row[0] = _alpah_sx_to((src_row[0] * weight)>>QTK_STITCH_FIX_POINT)+dst_row[0];
            ++src_row;
            ++dst_row;
            ++weight_row;
        }
    }
    return;
}

void qtk_blender_feather_feed2_sp_2channal_rect(qtk_blender_feather_t *feather, cv::InputArray &_img, cv::InputArray &mask, 
                    cv::Point tl, cv::InputArray &_weight_map, cv::InputArray &_yoffset,int s, int e)
{
    cv::Mat img = _img.getMat();
    cv::Mat dst = feather->dst_.getMat(cv::ACCESS_RW);
    cv::Mat yoffset = _yoffset.getMat();
    cv::Mat weight_map = _weight_map.getMat();
    int rows = feather->dst_roi_.height;

    const u_char *src_row = img.ptr(s,0);
    u_char *dst_row_uv = dst.ptr<u_char>(rows);
    u_char *dst_row = NULL;
    const ushort* weight_row = weight_map.ptr<ushort>(s,0);
    int *yoffsetp = yoffset.ptr<int>(0,s);
    int n2 = weight_map.cols;
    // printf("yoffsetp %d\n",*yoffsetp);
    for(int y = s; y < e; ++y,++yoffsetp){
        dst_row = dst_row_uv + *yoffsetp;
        for(int x = 0; x < n2; ++x){
            ushort weight = *weight_row;
            short k1 = src_row[0]-128; // 这个效果好点
            short k2 = src_row[1]-128;
            dst_row[0] += ((k1 * weight)>>QTK_STITCH_FIX_POINT);
            dst_row[1] += ((k2 * weight)>>QTK_STITCH_FIX_POINT);
            // if(weight){
            //     dst_row[0] = src_row[0];
            //     dst_row[1] = src_row[1];
            // }

            src_row += 2;
            dst_row += 2;
            ++weight_row;
        }
    }
    return;
}

void qtk_blender_feather_feed2_table_1channal_rect(qtk_blender_feather_t *feather, cv::InputArray &_img, cv::InputArray &_weight_map, 
                                                cv::InputArray &_table, int s, int e)
{
    cv::Mat img = _img.getMat();
    cv::Mat dst = feather->dst_.getMat(cv::ACCESS_RW);
    cv::Mat weight_map = _weight_map.getMat();
    cv::Mat table = _table.getMat();

    const u_char *src_row = img.data;
    u_char *dst_row = dst.ptr<u_char>();
    const ushort* weight_row = weight_map.ptr<ushort>();
    const int *table_p = table.ptr<int>(0,s);
    int n = e-s;
    for(int i = 0; i < n; ++i){
        int type = table_p[0];
        int tl = table_p[1];
        const ushort* ws = weight_row+table_p[2];
        const u_char* is = src_row+table_p[2];
        u_char *ds = dst_row+table_p[3];
        if(type == 1){
            for(int j = 0;j < tl; ++j){
                ds[0] += is[0];
                ++ds;
                ++is;
            }
        }else if(type == 2){
            for(int j = 0;j < tl; ++j){
                ds[0] = _alpah_sx_to((is[0] * ws[0])>>QTK_STITCH_FIX_POINT)+ds[0];
                ++ds;
                ++is;
                ++ws;
            }
        }
        table_p+=4;
    }

    return;
}

void qtk_blender_feather_feed2_table_2channal_rect(qtk_blender_feather_t *feather, cv::InputArray &_img, cv::InputArray &_weight_map, 
                                            cv::InputArray &_table, int s, int e)
{
    cv::Mat img = _img.getMat();
    cv::Mat dst = feather->dst_.getMat(cv::ACCESS_RW);
    cv::Mat weight_map = _weight_map.getMat();
    cv::Mat table = _table.getMat();
    int rows = feather->dst_roi_.height;

    const u_char *src_row = img.data;
    u_char *dst_row_uv = dst.ptr<u_char>(rows);
    const ushort* weight_row = weight_map.ptr<ushort>();
    const int *table_p = table.ptr<int>(0,s);
    int n = e-s;
    for(int i = 0; i < n; ++i){
        int type = table_p[0];
        int tl = table_p[1];
        const ushort* ws = weight_row+table_p[2];
        const u_char* is = src_row+table_p[2]*2;
        u_char *ds = dst_row_uv+table_p[3];
        if(type == 1){
            for(int j = 0;j < tl; ++j){
                short k1 = is[0]-128;
                short k2 = is[1]-128;
                ds[0] = ds[0] + k1;
                ds[1] = ds[1] + k2;
                ds+=2;
                is+=2;
            }
        }else if(type == 2){
            for(int j = 0;j < tl; ++j){
                ushort weight = ws[0];
                short k1 = is[0]-128;
                short k2 = is[1]-128;
                ds[0] = ((k1 * weight)>>QTK_STITCH_FIX_POINT)+ds[0];
                ds[1] = ((k2 * weight)>>QTK_STITCH_FIX_POINT)+ds[1];
                ds+=2;
                is+=2;
                ++ws;
            }
        }
        table_p+=4;
    }

    return;
}