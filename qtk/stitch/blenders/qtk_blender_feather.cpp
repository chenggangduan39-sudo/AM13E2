#include "qtk_blender_feather.h"
#include <iostream>
#include <math.h>
#include "qtk/stitch/qtk_stitch_def.h"
#include "opencv2/stitching/detail/util.hpp"
#include "opencv2/stitching/detail/blenders.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
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
    cv::Mat weights_sum;
    std::vector<cv::UMat> weight_maps_; //原始的
    std::vector<cv::UMat> weight_temps_; //作为中间内存
};

#ifdef USE_9391
#define BLEND_DST_CHANNEL 4
#else
#define BLEND_DST_CHANNEL 3
#endif
void qtk_blender_create_weight_map(qtk_blender_feather_t *feather,
                            cv::InputArray mask,float sharpness, cv::InputOutputArray weight, cv::InputOutputArray temp);
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
    feather->dst_.create(dst_roi.size(), CV_16SC3);
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
            // for(int j = 0;j < tl; ++j){
            //     ds[0] += is[0];
            //     ++ds;
            //     ++is;
            // }
            memcpy(ds,is,tl);
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
            // for(int j = 0;j < tl; ++j){
            //     short k1 = is[0]-128;
            //     short k2 = is[1]-128;
            //     ds[0] = ds[0] + k1;
            //     ds[1] = ds[1] + k2;
            //     ds+=2;
            //     is+=2;
            // }
            memcpy(ds,is,tl*sizeof(short));
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

static inline u_char _alpah_x_to(float t)
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

void _weight2point(cv::UMat &weight_map,cv::UMat &weight_map_in)
{
    // float dd = powf(2.0,QTK_STITCH_FIX_POINT);
    int rows = weight_map_in.rows;
    int cols = weight_map_in.cols;
    if(weight_map.empty()){
        weight_map.create(weight_map_in.rows,weight_map_in.cols,CV_16U);
    }
    ushort *p = weight_map.getMat(cv::ACCESS_RW).ptr<ushort>();
    float *p_in = weight_map_in.getMat(cv::ACCESS_RW).ptr<float>();
    for(int i = 0; i < rows; ++i){
        for(int j = 0; j < cols; ++j){
            p[0] = p_in[0]*(1<<QTK_STITCH_FIX_POINT);
            ++p;
            ++p_in;
        }
    }
    // weight_map = weight_map_in.mul(dd);
    // cv::UMat out;
    // weight_map.convertTo(out, CV_16U);
    // weight_map = out;
    return;
}

cv::Rect qtk_blender_feather_weightmaps(qtk_blender_feather_t *feather,const std::vector<cv::UMat> &masks, 
                    const std::vector<cv::Point> &corners,std::vector<cv::UMat> &weight_maps)
{
    weight_maps.resize(masks.size());
    feather->weight_maps_.resize(masks.size());
    feather->weight_temps_.resize(masks.size());
    for (size_t i = 0; i < masks.size(); ++i){
        // cv::detail::createWeightMap(masks[i], feather->sharpness_, feather->weight_maps_[i]);
        qtk_blender_create_weight_map(feather, masks[i], feather->sharpness_, feather->weight_maps_[i],feather->weight_temps_[i]);
        // cv::Mat weight = feather->weight_maps_[i].getMat(cv::ACCESS_READ);
        // int rows = feather->weight_maps_[i].rows;
        // int cols = feather->weight_maps_[i].cols;
        // for(int k = 0; k < rows; ++k){
        //     for(int j = 0; j < cols; ++j){
        //         printf("%f\n",weight.at<float>(k,j));
        //     }
        // }
    }
    cv::Rect dst_roi = cv::detail::resultRoi(corners, masks);
    if(feather->weights_sum.empty()){
        feather->weights_sum.create(dst_roi.size(), CV_32F);
    }
    float *ws = (float*)feather->weights_sum.ptr<float>();
    // feather->weights_sum.setTo(0);
    memset(ws,0,dst_roi.width*dst_roi.height*sizeof(float));
    for (size_t i = 0; i < feather->weight_maps_.size(); ++i)
    {
        cv::Rect roi(corners[i].x - dst_roi.x, corners[i].y - dst_roi.y,
                 feather->weight_maps_[i].cols, feather->weight_maps_[i].rows);
        add(feather->weights_sum(roi), feather->weight_maps_[i], feather->weights_sum(roi));
    }
    feather->weights_sum+=1e-5;
    for (size_t i = 0; i < feather->weight_maps_.size(); ++i)
    {
        cv::Rect roi(corners[i].x - dst_roi.x, corners[i].y - dst_roi.y,
                 feather->weight_maps_[i].cols, feather->weight_maps_[i].rows);
        cv::Mat tmp = feather->weights_sum(roi);
        // tmp+=1e-5;
        divide(feather->weight_maps_[i], tmp, feather->weight_maps_[i]);
        // printf("%d %d %d %d\n",feather->weight_maps_[i].cols,feather->weight_maps_[i].rows,
        //                 feather->weight_maps_[i].channels(),feather->weight_maps_[i].type());
    }
    //定点化
    for(size_t i = 0; i < feather->weight_maps_.size(); ++i){
        _weight2point(weight_maps[i], feather->weight_maps_[i]);
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
        ushort *pp = p.ptr<ushort>(i,0);
        ushort *ww = w.ptr<ushort>(i*2,0);
        for(int j = 0; j < colse; ++j){
            pp[0] = ww[0];
            ++pp;
            ww+=2;
        }
    }
    return;
}

void qtk_blender_feather_uv_weightmaps(qtk_blender_feather_t *feather, std::vector<cv::UMat> &weight_maps, 
                                            std::vector<cv::UMat> &uv_weight_maps)
{
    if(weight_maps.size() != uv_weight_maps.size()){
        uv_weight_maps.resize(weight_maps.size());
    }
    int size = weight_maps.size();
    for(int i = 0; i < size; ++i){
        if(uv_weight_maps[i].empty()){
            uv_weight_maps[i] = cv::UMat(weight_maps[i].rows/2,weight_maps[i].cols/2,CV_16U);
        }
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

void _blender_feather_weight_maps_update_mul(cv::UMat &weight_map_, cv::Mat &inupmap, cv::UMat &out)
{
    float *weigth = weight_map_.getMat(cv::ACCESS_READ).ptr<float>();
    float *up = inupmap.ptr<float>();
    ushort *outp = out.getMat(cv::ACCESS_RW).ptr<ushort>();
    int rows = weight_map_.rows;
    int cols = weight_map_.cols;
    int n = rows*cols;

    for(int i = 0; i < n; ++i){
        outp[0] = weigth[0] * up[0] * (1<<QTK_STITCH_FIX_POINT);
    }

    return;
}

void qtk_blender_feather_weight_maps_update(qtk_blender_feather_t *feather,std::vector<cv::Mat> inupmaps,
                                                std::vector<cv::UMat> &weight_maps)
{
    if(feather->weight_maps_.size() != inupmaps.size()){
        return;
    }
    if(inupmaps[0].channels() == 3){
        weight_maps.clear();
        weight_maps.resize(inupmaps.size());
        for(size_t i = 0; i < feather->weight_maps_.size(); ++i){
                std::vector<cv::UMat> gains_channels;
                cv::UMat u_gain_map;
                gains_channels.push_back(feather->weight_maps_[i]);
                gains_channels.push_back(feather->weight_maps_[i]);
                gains_channels.push_back(feather->weight_maps_[i]);
                merge(gains_channels, u_gain_map);
                cv::multiply(u_gain_map, inupmaps[i], weight_maps[i], 1);
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
    }else{
        for(size_t i = 0; i < feather->weight_maps_.size(); ++i){
            // cv::multiply(feather->weight_maps_[i], inupmaps[i], weight_maps[i], 1);
            _blender_feather_weight_maps_update_mul(feather->weight_maps_[i], inupmaps[i], weight_maps[i]);
        }
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
            // for(int j = 0;j < tl; ++j){
            //     ds[0] += is[0];
            //     ++ds;
            //     ++is;
            // }
            memcpy(ds,is,tl);
        }else if(type == 2){
            int j = 0;
            int tlp = tl -4;
            for(;j < tlp; j+=4){ 
                ushort w1 = ws[0];
                ushort w2 = ws[1];
                ushort w3 = ws[2];
                ushort w4 = ws[3];
                u_char s1 = is[0];
                u_char s2 = is[1];
                u_char s3 = is[2];
                u_char s4 = is[3];
                ds[0] = _alpah_sx_to((s1 * w1)>>QTK_STITCH_FIX_POINT)+ds[0];
                ds[1] = _alpah_sx_to((s2 * w2)>>QTK_STITCH_FIX_POINT)+ds[1];
                ds[2] = _alpah_sx_to((s3 * w3)>>QTK_STITCH_FIX_POINT)+ds[2];
                ds[3] = _alpah_sx_to((s4 * w4)>>QTK_STITCH_FIX_POINT)+ds[3];
                ds+=4;
                is+=4;
                ws+=4;
            }
            for(;j < tl; ++j){
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
            // for(int j = 0;j < tl; ++j){
            //     short k1 = is[0]-128;
            //     short k2 = is[1]-128;
            //     ds[0] = ds[0] + k1;
            //     ds[1] = ds[1] + k2;
            //     ds+=2;
            //     is+=2;
            // }
            memcpy(ds,is,tl*sizeof(short));
        }else if(type == 2){
            int j = 0;
            int tlp = tl-2;
            for(;j < tlp;j+=2){
                ushort w1 = ws[0];
                ushort w2 = ws[1];
                // ushort w3 = ws[2];
                // ushort w4 = ws[3];
                short k1 = is[0]-128;
                short k2 = is[1]-128;
                short k3 = is[2]-128;
                short k4 = is[3]-128;
                // short k5 = is[4]-128;
                // short k6 = is[5]-128;
                // short k7 = is[6]-128;
                // short k8 = is[7]-128;
                ds[0] = ((k1 * w1)>>QTK_STITCH_FIX_POINT)+ds[0];
                ds[1] = ((k2 * w1)>>QTK_STITCH_FIX_POINT)+ds[1];
                ds[2] = ((k3 * w2)>>QTK_STITCH_FIX_POINT)+ds[2];
                ds[3] = ((k4 * w2)>>QTK_STITCH_FIX_POINT)+ds[3];
                // ds[4] = ((k5 * w3)>>QTK_STITCH_FIX_POINT)+ds[4];
                // ds[5] = ((k6 * w3)>>QTK_STITCH_FIX_POINT)+ds[5];
                // ds[6] = ((k7 * w4)>>QTK_STITCH_FIX_POINT)+ds[6];
                // ds[7] = ((k8 * w4)>>QTK_STITCH_FIX_POINT)+ds[7];
                ds+=4;
                is+=4;
                ws+=2;
            }
            for(;j < tl; ++j){
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

void qtk_blender_feather_draw_seam_lines(qtk_blender_feather_t *feather,cv::Mat &lines)
{
    cv::Mat dst = feather->dst_.getMat(cv::ACCESS_RW);
    cv::Rect roi(0,0,lines.cols,lines.rows);
    cv::Mat tmp = dst(roi);
    tmp.setTo(0,lines>0);
    return;
}

static const int DIST_SHIFT = 16;
#define  CV_FLT_TO_FIX(x,n)  cvRound((x)*(1<<(n)))

static void initTopBottom( cv::Mat& temp, int border, unsigned int value )
{
    cv::Size size = temp.size();
    unsigned int* ttop = (unsigned int*)temp.ptr<int>(0);
    unsigned int* tbottom = (unsigned int*)temp.ptr<int>(size.height - 1);
    for( int i = 0; i < border; i++ )
    {
        for( int j = 0; j < size.width; j++ )
        {
            ttop[j] = value;
            tbottom[j] = value;
        }
        ttop += size.width;
        tbottom -= size.width;
    }
}

static void _distanceTransform_3x3( const cv::Mat& _src, cv::Mat& _temp, cv::Mat& _dist, const float* metrics )
{
    const int BORDER = 1;
    int i, j;
    const unsigned int HV_DIST = CV_FLT_TO_FIX( metrics[0], DIST_SHIFT );
    const unsigned int DIAG_DIST = CV_FLT_TO_FIX( metrics[1], DIST_SHIFT );
    const unsigned int DIST_MAX = UINT_MAX - DIAG_DIST;
    const float scale = 1.f/(1 << DIST_SHIFT);

    const uchar* src = _src.ptr();
    int* temp = _temp.ptr<int>();
    float* dist = _dist.ptr<float>(_dist.rows - 1);
    int srcstep = (int)(_src.step/sizeof(src[0]));
    int step = (int)(_temp.step/sizeof(temp[0]));
    int dststep = (int)(_dist.step/sizeof(dist[0]));
    cv::Size size = _src.size();

    initTopBottom( _temp, BORDER, DIST_MAX );

    // forward pass
    unsigned int* tmp = (unsigned int*)(temp + BORDER*step) + BORDER;
    const uchar* s = src;

    for( i = 0; i < size.height; i++ )
    {
        for( j = 0; j < BORDER; j++ )
            tmp[-j-1] = tmp[size.width + j] = DIST_MAX;

        for( j = 0; j < size.width; j++ )
        {
            unsigned int* tmp1  = tmp-step;
            if( !s[j] ){
                tmp[0] = 0;
            }else{
                unsigned int t0 = *(tmp1-1) + DIAG_DIST;
                unsigned int t = *(tmp1) + HV_DIST;
                if( t0 > t ) t0 = t;
                t = tmp1[1] + DIAG_DIST;
                if( t0 > t ) t0 = t;
                t = *(tmp-1) + HV_DIST;
                if( t0 > t ) t0 = t;
                tmp[0] = (t0 > DIST_MAX) ? DIST_MAX : t0;   
            }
            ++tmp;
        }
        tmp += BORDER*2;
        s += srcstep;
    }

    // backward pass
    float* d = (float*)dist;
    for( i = size.height - 1; i >= 0; i-- )
    {
        tmp -= step;
        for( j = size.width - 1; j >= 0; j-- )
        {
            unsigned int *tmp1 = tmp + j + step;
            unsigned int *tmp2 = tmp + j;
            unsigned int t0 = tmp2[0];
            if( t0 > HV_DIST )
            {
                unsigned int t = tmp1[1] + DIAG_DIST;
                if( t0 > t ) t0 = t;
                t = tmp1[0] + HV_DIST;
                if( t0 > t ) t0 = t;
                t = *(tmp1-1) + DIAG_DIST;
                if( t0 > t ) t0 = t;
                t = tmp2[1] + HV_DIST;
                if( t0 > t ) t0 = t;
                tmp2[0] = t0;
            }
            d[j] = (float)(t0 * scale);
        }
        d -= dststep;
    }
}

static void getDistanceTransformMask( int maskType, float *metrics )
{
    CV_Assert( metrics != 0 );

    switch (maskType)
    {
    case 30:
        metrics[0] = 1.0f;
        metrics[1] = 1.0f;
        break;

    case 31:
        metrics[0] = 1.0f;
        metrics[1] = 2.0f;
        break;

    case 32:
        metrics[0] = 0.955f;
        metrics[1] = 1.3693f;
        break;

    case 50:
        metrics[0] = 1.0f;
        metrics[1] = 1.0f;
        metrics[2] = 2.0f;
        break;

    case 51:
        metrics[0] = 1.0f;
        metrics[1] = 2.0f;
        metrics[2] = 3.0f;
        break;

    case 52:
        metrics[0] = 1.0f;
        metrics[1] = 1.4f;
        metrics[2] = 2.1969f;
        break;
    default:
        CV_Error(cv::Error::StsBadArg, "Unknown metric type");
    }
}

void qtk_distance_transform(cv::InputArray _src,cv::OutputArray _temp, cv::OutputArray _dst, int maskSize)
{
    cv::Mat src = _src.getMat();
    if(_dst.empty()){
        _dst.create( src.size(), CV_32F);
    }
    cv::Mat dst = _dst.getMat();

    float _mask[5] = {0};

    getDistanceTransformMask(1 + maskSize*10, _mask);

    cv::Size size = src.size();

    int border = 1;
    if(_temp.empty()){
        _temp.create(size.height + border*2, size.width + border*2, CV_32SC1);
    }
    cv::Mat temp = _temp.getMat();
    _distanceTransform_3x3(src, temp, dst, _mask);
    return;
}

void qtk_blender_create_weight_map(qtk_blender_feather_t *feather,cv::InputArray mask,float sharpness, cv::InputOutputArray weight, cv::InputOutputArray temp)
{
    qtk_distance_transform(mask, temp, weight, 3);
    // cv::Mat tmp;
    // multiply(weight, sharpness, tmp);
    // threshold(tmp, weight, 1.f, 1.f, cv::THRESH_TRUNC);
    cv::Mat tmp = weight.getMat();
    int rows = tmp.rows;
    int cols = tmp.cols;
    float *p = (float *)tmp.data;
    for(int i = 0; i < rows; ++i){
        for(int j = 0; j < cols; ++j){
            float k = sharpness * p[0];
            p[0] = k > 1.f ? 1.f : k;
            ++p;
        }
    }
    return;
}