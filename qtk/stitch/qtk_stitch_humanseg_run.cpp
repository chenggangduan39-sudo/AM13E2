#include "qtk_stitch_humanseg_run.h"
#include <iostream>
#include "opencv2/core/mat.hpp"
#include "opencv2/imgproc.hpp"
#include "image/qtk_stitch_image.h"

static cv::Mat _nv12yuv(cv::Mat &crop_img,int rows,int cols)
{
    int uv_rows = crop_img.rows/3;
    int uv_cols = crop_img.cols/2;

    cv::Mat y = cv::Mat(uv_rows*2,uv_cols*2,CV_8UC1,crop_img.ptr());
    cv::Mat u;
    u.create(uv_rows,uv_cols,CV_8UC1);
    cv::Mat v;
    v.create(uv_rows,uv_cols,CV_8UC1);

    cv::Mat uv = cv::Mat(uv_rows,uv_cols*2,CV_8UC1,crop_img.ptr(uv_rows*2));
    uchar *p = uv.ptr(0,0);
    for(int i = 0;i<uv_rows;++i){
        for(int j = 0; j < uv_cols;++j){
            v.at<uchar>(i,j) = p[0];
            u.at<uchar>(i,j) = p[1];
            p+=2;
        }
    }

    cv::Mat dd;
    cv::Mat du;
    cv::Mat dv;
    std::vector<cv::Mat> channels;
    cv::resize(y,dd,cv::Size(cols,rows));
    channels.push_back(dd);
    cv::resize(u,du,cv::Size(cols,rows));
    channels.push_back(du);
    cv::resize(v,dv,cv::Size(cols,rows));
    channels.push_back(dv);

    cv::Mat yuv;
    cv::merge(channels, yuv);

    return yuv;
}

void qtk_stitch_humanseg_get_humanseg(qtk_stitch_humanseg_t *seg, void *_corners, void *_low_mask, void *_imgs)
{
    cv::Mat *cropper_imgs = (cv::Mat*)_imgs;
    cv::Mat *the_low_masks = (cv::Mat*)_low_mask;
    std::vector<cv::Point> *roi_corners = (std::vector<cv::Point>*)_corners;
    int n = roi_corners->size();
    std::vector<cv::Mat> low_humanseg;
    int rows = 0;
    int cols = 0;
    int iw = seg->cfg->in_w;
    int ih = seg->cfg->in_h;
    
    for(int i = 1; i < n; ++i){
        rows = the_low_masks[i].rows;
        cols = the_low_masks[i].cols;
        cv::Mat low_yuv = _nv12yuv(cropper_imgs[i],rows,cols);
        cv::Mat low_rgb;
        cv::cvtColor(low_yuv,low_rgb,cv::COLOR_YUV2RGB);
        int pw = the_low_masks[i-1].cols;
        int set = roi_corners->at(i).x - roi_corners->at(i-1).x;
        int w = pw - set;
        // printf("%d %d %d\n",pw,set,w);
        cv::Mat low_crop = low_rgb(cv::Rect(0,0,w,rows)).clone();
        // char path[128] = {0};
        // sprintf(path,"%s_%d.png","rgbimg",i);
        // qtk_stitch_image_save_data(path,low_crop.data,low_crop.cols,low_crop.rows,3);
        cv::Mat in_rgb;
        cv::resize(low_crop,in_rgb,cv::Size(iw,ih));

        cv::Mat out = cv::Mat(ih,iw,CV_8UC1);
        qtk_stitch_humanseg_feed(seg,in_rgb.data,out.data);
        cv::resize(out,low_crop,cv::Size(w,rows));
        // sprintf(path,"%s_%d.png","humanseg",i);
        // qtk_stitch_image_save_data(path,low_crop.data,low_crop.cols,low_crop.rows,1);
        low_humanseg.push_back(low_crop);
    }
    seg->humansegs = new std::vector<cv::Mat>(low_humanseg);
    return;
}

void qtk_stitch_humanseg_clear_humanseg(qtk_stitch_humanseg_t *seg)
{
    std::vector<cv::Mat> *low_humanseg = (std::vector<cv::Mat>*)seg->humansegs;
    delete low_humanseg;
    seg->humansegs = NULL;
    return;
}

void _humanseg_seammask_cross_point(cv::Mat humanseg_mask,int *point,int point_num, 
                                            int cross_size,int split_num, int *is_coress)
{
    int rows = humanseg_mask.rows;
    int cols = humanseg_mask.cols;
    int cross_size_n = cross_size/2;
    int rows_split = (rows+1)/split_num;

    for(int i = 0; i < rows; ++i){
        if(point[i] > cols){
            continue;
        }
        int s = point[i]-cross_size_n;
        s = s < 0 ? 0 : s;
        int e = point[i]+cross_size_n;
        e = e > cols  ? cols : e;
        for(int j = s; j < e; ++j){
            if(humanseg_mask.at<uchar>(i,j) == 0){
                // printf("%d %d\n",i,j);
                int k = i/rows_split;
                is_coress[k] = 1;
            }
        }
    }
    // for(int i = 0; i < split_num; ++i){
    //     printf("%d \n",is_coress[i]);
    // }
    // printf("qqqq\n");
    return;
}

void qtk_stitch_humanseg_seammask_cross(qtk_stitch_humanseg_t *seg,int *point,int point_num,
                                    int cross_size,int split_num,int *is_coress)
{
    std::vector<cv::Mat> *low_humanseg = (std::vector<cv::Mat>*)seg->humansegs;
    int *pointu = point+(point_num*2); //第一张图跳过
    int n = low_humanseg->size();

    for(int i = 0; i < n; ++i){
        cv::Mat low_crop_mask = low_humanseg->at(i);
        _humanseg_seammask_cross_point(low_crop_mask,pointu+(i*point_num*2),point_num,cross_size,split_num,is_coress+split_num*i);
    }
    
    return;
}

int qtk_stitch_humanseg_mask_connect(qtk_stitch_humanseg_t *seg,int split_num,int idx, int the_split)
{
    std::vector<cv::Mat> *low_humanseg = (std::vector<cv::Mat>*)seg->humansegs;
    int n = low_humanseg->size();
    int rows = 0;
    int cols = 0;
    int rows_n = 0;

    if(idx > n){
        return 0;
    }
    cv::Mat low_crop_mask = low_humanseg->at(idx);
    
    rows = low_crop_mask.rows;
    cols = low_crop_mask.cols;

    rows_n = rows/split_num;
    int s = rows_n*the_split;
    int e = s+rows_n;
    int connect = 1;
    for(int i = s; i < e; ++i) {
        connect = 0;
        for(int j = 0; j < cols; ++j){
            if(low_crop_mask.at<uchar>(i,j) > 0) {
                int mm = 0;
                int mm_re = (i+1) > rows?rows:(i+1);
                int mm_cs = (j-1)<0?0:(j-1);
                int mm_ce = (j+1)>cols?cols:(j+1);
                for(int mc = mm_cs; mc < mm_ce; ++mc){
                    mm += low_crop_mask.at<uchar>(mm_re,mc);
                }
                if(mm > 0){
                    connect = 1; //确实连通了
                }
            }
            if(connect == 1){
                break;
            }
        }
        if(connect == 0){ //没连通
            break;
        }
    }
    return connect;
}