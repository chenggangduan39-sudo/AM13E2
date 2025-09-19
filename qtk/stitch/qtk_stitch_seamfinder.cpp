#include <math.h>
#include <iostream>
#include "qtk_stitch_seamfinder.h"
#include "opencv2/stitching/detail/seam_finders.hpp"
#include "wtk/core/wtk_alloc.h"
#include "qtk_stitch_def.h"
#include "opencv2/core/mat.hpp"
#include "opencv2/imgproc.hpp"
#include "qtk/stitch/image/qtk_stitch_image.h"

struct qtk_stitch_seamfinder{
    int type;
    cv::Ptr<cv::detail::SeamFinder> finder;
    int img_type;
    int left_full;
    int right_full;
};

void _seammasks_update(cv::UMat &new_seamask,cv::UMat &old_seammask,int *point,int is_right);
void _seam_mask_find_rect(cv::UMat &seam_mask,int *w,int *n);
void _seammasks_update_block(cv::UMat &new_seamask_block,cv::UMat &old_seammask,int *point,int is_right,int split_num,int start);

qtk_stitch_seamfinder_t* qtk_stitch_seamfinder_new(int type, int img_type, int left_full, int right_full)
{
    qtk_stitch_seamfinder_t* finder = new qtk_stitch_seamfinder_t;
    finder->type = type;
    finder->img_type = img_type;
    wtk_debug("seamfinder type %d\n",type);
    switch (type){
        case QTK_STITCH_FINDER_DP_COLOR:
            finder->finder = new cv::detail::DpSeamFinder(cv::detail::DpSeamFinder::COLOR);
            break;
        case QTK_STITCH_FINDER_DP_COLORGRAD:
            finder->finder = new cv::detail::DpSeamFinder(cv::detail::DpSeamFinder::COLOR_GRAD);
            break;
        case QTK_STITCH_FINDER_GC_COLOR:
            finder->finder = new cv::detail::GraphCutSeamFinder(cv::detail::GraphCutSeamFinder::COST_COLOR);
            break;
        case QTK_STITCH_FINDER_GC_COLORGRAD:
            finder->finder = new cv::detail::GraphCutSeamFinder(cv::detail::GraphCutSeamFinder::COST_COLOR_GRAD);
            break;
        case QTK_STITCH_FINDER_VORONOI:
            finder->finder = cv::detail::SeamFinder::createDefault(cv::detail::SeamFinder::VORONOI_SEAM);
            break;
        case QTK_STITCH_FINDER_NO:
            finder->finder = cv::detail::SeamFinder::createDefault(cv::detail::SeamFinder::NO);
            break;
    }
    finder->left_full = left_full;
    finder->right_full = right_full;
    return finder;
}

void qtk_stitch_seamfinder_delete(qtk_stitch_seamfinder_t *finder)
{
    if(finder){
        if(finder->finder != nullptr){
            finder->finder.release();
        }
        delete finder;
    }
}

void* qtk_stitch_seamfinder_find(qtk_stitch_seamfinder_t *finder,void* corners,void* imgs, void* masks)
{
    cv::Mat *crop_imgs = (cv::Mat*)imgs;
    std::vector<cv::Point> *crop_corners = (std::vector<cv::Point>*)corners;
    cv::Mat *crop_masks = (cv::Mat*)masks;
    std::vector<cv::UMat> crop_images_umat;
    std::vector<cv::UMat> crop_masks_umat;
    
    int n = crop_corners->size();
    for(int i = 0; i < n; i++){
        cv::Mat um;
        crop_imgs[i].assignTo(um,CV_32F);
        crop_images_umat.push_back(um.getUMat(cv::ACCESS_RW));
    }
    for(int i = 0; i < n; i++){
        crop_masks_umat.push_back(crop_masks[i].getUMat(cv::ACCESS_RW).clone());
    }

    finder->finder->find(crop_images_umat,*crop_corners,crop_masks_umat);
    // for(std::vector<cv::Mat>::iterator it = crop_masks->begin();it!=crop_masks->end();it++){
    //     for(int i = 0;i<it->rows;i++){
    //         for(int j = 0; j < it->cols;j++){
    //             printf("%d\n",it->at<uchar>(i,j));
    //         }
    //     }
    // }
    // exit(1);
    return new std::vector<cv::UMat>(crop_masks_umat);
}

//nv12转yuv并且resize
cv::Mat qtk_stitch_seamfinder_nv12yuv(cv::Mat &crop_img,int rows,int cols)
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
    // cv::Mat rgb;
    // cv::cvtColor(yuv, rgb, cv::COLOR_YUV2RGB);
    // qtk_stitch_image_save_data("nv12yuv_rgb.png",rgb.data,rgb.cols,rgb.rows,3);

    return yuv;
}

//创建根据现在的拼接缝使用full作限制的拼接缝mask
//site 0 左 site 1 中 site 2 右
cv::Mat _stitch_seamfinder_create_fullmask(qtk_stitch_seamfinder_t *finder,cv::Mat *mask,int *seammask_points,int point_num, int site)
{
    int left_full = finder->left_full;
    int right_full = finder->right_full;
    int rows = mask->rows;
    int cols = mask->cols;

    int *left_p = seammask_points;
    int *right_p = seammask_points + point_num;
    int left_max;
    int left_min;
    int right_max;
    int right_min;
    left_max = left_min = left_p[0];
    right_max = right_min = right_p[0];
    for(int i = 0; i < rows; ++i){
        if(left_p[i] > left_max){
            left_max = left_p[i];
        }else if(left_p[i] < left_min){
            left_min = left_p[i];
        }
        if(right_p[i] > right_max){
            right_max = right_p[i];
        }else if(right_p[i] < right_min){
            right_min = right_p[i];
        }
    }

    //mask的边界
    int left_line = cols;
    int right_line = 0;
    for(int i = 0; i < rows; i++){
        int j1 = 0;
        int j2 = cols;
        for(int j = 1; j < cols; j++){
            if(mask->at<uchar>(i,j) == 255 && mask->at<uchar>(i,j1) != 255){
                j1 = j;
            }
            if(mask->at<uchar>(i,j1) == 255 && mask->at<uchar>(i,j) != 255){
                j2 = j;
                break;
            }
        }
        left_line = left_line > j1 ? j1 : left_line;
        right_line = right_line < j2 ? j2 : right_line;
    }

    //按照这个重新画一个mask
    cv::Mat mask_new = cv::Mat::zeros(mask->rows, mask->cols, CV_8UC1); 
    if(site == 0){ //左边要改右边缝
        int right_center = (right_max + right_min)/2;
        int right_ufull = right_center+right_full;
        int right = MIN(right_line,right_ufull);
        cv::rectangle(mask_new,cv::Point(0,0),cv::Point(right,rows-1),255,-1);
    }else if(site == 1){ //中间
        int left_center = (left_max + left_min)/2;
        int right_center = (right_max + right_min)/2;
        int left_ufull = left_center-left_full;
        int right_ufull = right_center+right_full;
        int left = MAX(left_line,left_ufull);
        int right = MIN(right_line,right_ufull);
        cv::rectangle(mask_new,cv::Point(left,0),cv::Point(right,rows-1),255,-1);
    }else if(site == 2){ //右边改左边缝
        int left_center = (left_max + left_min)/2;
        int left_ufull = left_center-left_full;
        int left = MAX(left_line,left_ufull);
        cv::rectangle(mask_new,cv::Point(left,0),cv::Point(cols-1,rows-1),255,-1);
    }

    return mask_new;
}

void _stitch_seamfinder_block_mask(cv::Mat mask,int row_start, int mode, int is_right,int *seam_point,int num)
{
    int rows = mask.rows;
    int cols = mask.cols;
    int point[2] = {0,0};
    int *the_seam_point = seam_point;
    if(is_right == 1){
        the_seam_point = seam_point+num;
    }
    if(mode == 0){ //头
        point[0] = -1;
        point[1] = the_seam_point[row_start+rows];
    }else if(mode == 1){ //中间
        point[0] = the_seam_point[row_start];
        point[1] = the_seam_point[row_start+cols];
    }else if(mode == 2){ //尾部
        point[0] = the_seam_point[row_start];
        point[1] = -1;
    }
    // printf("%d %d %d %d %d\n",mode,point[0],point[1],row_start,rows);
    for(int i = 0; i < rows; ++i){
        for(int j = 0; j < cols; ++j){
            int x_t = 0;
            int x_b = 0;
            int is_zero = 0;
            if(is_right){
                if(point[0] > 0 && j > point[0]){
                    x_t = j - point[0];
                    if(i < x_t * 0.5){
                        is_zero = 1;
                    }
                }
                if(point[1] > 0 && j > point[1]){
                    x_b = j - point[1];
                    if(i > rows - (x_b * 0.5)){
                        is_zero = 1;
                    }
                }
            }else{
                if(point[0] > 0 && j < point[0]){
                    x_t = point[0] - j;
                    if(i < x_t * 0.5){
                        is_zero = 1;
                    }
                }
                if(point[1] > 0 && j < point[1]){
                    x_b = point[1] - j;
                    if(i > rows - (x_b * 0.5)){
                        is_zero = 1;
                    }
                }
            }
            if(is_zero){
                mask.at<uchar>(i,j) = 0;
            }
        }
    }

    return;
}

//更新全部拼接缝
void* qtk_stitch_seamfinder_find2(qtk_stitch_seamfinder_t *finder,void* corners,void* imgs, void* masks,
                                int *seammask_points,int point_num)
{
    cv::Mat *crop_imgs = (cv::Mat*)imgs;
    std::vector<cv::Point> *crop_corners = (std::vector<cv::Point>*)corners;
    cv::Mat *crop_masks = (cv::Mat*)masks;
    std::vector<cv::UMat> crop_images_umat;
    std::vector<cv::UMat> crop_masks_umat;
    int rows = 0;
    int cols = 0;
    std::vector<cv::Mat> low_imgs;
    
    int n = crop_corners->size();
    if(finder->img_type == QTK_STITCH_USE_IMG_RGB){
        for(int i = 0; i < n; ++i){ //没测过
            cv::Mat low_img;
            rows = crop_masks[i].rows;
            cols = crop_masks[i].cols;
            cv::resize(crop_imgs[i],low_img,cv::Size(cols,rows), 0, 0, cv::INTER_LINEAR);
            low_imgs.push_back(low_img);
        }
    }else if(finder->img_type == QTK_STITCH_USE_IMG_NV12){
        for(int i = 0; i < n; ++i){
            rows = crop_masks[i].rows;
            cols = crop_masks[i].cols;
            cv::Mat low_yuv = qtk_stitch_seamfinder_nv12yuv(crop_imgs[i],rows,cols);
            low_imgs.push_back(low_yuv);
        }
    }
    for(int i = 0; i < n; i++){
        cv::Mat um;
        low_imgs[i].assignTo(um,CV_32F);
        crop_images_umat.push_back(um.getUMat(cv::ACCESS_RW));
    }
    for(int i = 0; i < n; i++){
        // printf("crop_masks %d %d\n",crop_masks[i].rows,crop_masks[i].cols);
        cv::Mat new_crop_mask;
        if(i == 0){
            new_crop_mask = _stitch_seamfinder_create_fullmask(finder,crop_masks+i,seammask_points+(i*point_num*2),point_num,0);
        }else if(i == (n-1)){
            new_crop_mask = _stitch_seamfinder_create_fullmask(finder,crop_masks+i,seammask_points+(i*point_num*2),point_num,2);
        }else{
            new_crop_mask = _stitch_seamfinder_create_fullmask(finder,crop_masks+i,seammask_points+(i*point_num*2),point_num,1);
        }
        // crop_masks_umat.push_back(crop_masks[i].getUMat(cv::ACCESS_RW).clone());
        crop_masks_umat.push_back(new_crop_mask.getUMat(cv::ACCESS_RW).clone());
    }
    finder->finder->find(crop_images_umat,*crop_corners,crop_masks_umat);
 
    // char path[256] = {0,};
    // for(int i = 0; i < n; i++){
    //     sprintf(path,"new_seammask_%d.png",i);
    //     cv::Mat pp = crop_masks_umat[i].getMat(cv::ACCESS_READ);
    //     qtk_stitch_image_save_data(path,pp.data,pp.cols,pp.rows,1);
    // }
    return new std::vector<cv::UMat>(crop_masks_umat);
}

//更新idx的拼接缝
//从0开始
void qtk_stitch_seamfinder_find3(qtk_stitch_seamfinder_t *finder,void *low_seammasks,
                        void* corners,void* imgs, void* masks,int *point,int point_num,int idx)
{
    cv::Mat *crop_imgs = (cv::Mat*)imgs;
    std::vector<cv::Point> *crop_corners = (std::vector<cv::Point>*)corners;
    cv::Mat *crop_masks = (cv::Mat*)masks;
    std::vector<cv::UMat> crop_images_umat;
    std::vector<cv::UMat> crop_masks_umat;
    std::vector<cv::UMat> *the_low_seammasks = (std::vector<cv::UMat>*)low_seammasks;
    int rows = 0;
    int cols = 0;
    std::vector<cv::Mat> low_imgs;
    
    if(idx < 0){
        printf("idx error\n");
        return;
    }
    int n = crop_corners->size();
    if(finder->img_type == QTK_STITCH_USE_IMG_RGB){
        for(int i = idx; i <= (idx+1); ++i){ //没测过
            cv::Mat low_img;
            rows = crop_masks[i].rows;
            cols = crop_masks[i].cols;
            cv::resize(crop_imgs[i],low_img,cv::Size(cols,rows), 0, 0, cv::INTER_LINEAR);
            low_imgs.push_back(low_img);
        }
    }else if(finder->img_type == QTK_STITCH_USE_IMG_NV12){
        for(int i = idx; i <= (idx+1); ++i){
            rows = crop_masks[i].rows;
            cols = crop_masks[i].cols;
            cv::Mat low_yuv = qtk_stitch_seamfinder_nv12yuv(crop_imgs[i],rows,cols);
            low_imgs.push_back(low_yuv);
        }
    }

    for(size_t i = 0; i < low_imgs.size(); i++){
        cv::Mat um;
        low_imgs[i].assignTo(um,CV_32F);
        crop_images_umat.push_back(um.getUMat(cv::ACCESS_RW));
    }
    for(int i = idx; i <= (idx+1); i++){
        // printf("crop_masks %d %d\n",crop_masks[i].rows,crop_masks[i].cols);
        cv::Mat new_crop_mask;
        if(i == 0){
            new_crop_mask = _stitch_seamfinder_create_fullmask(finder,crop_masks+i,point+(i*point_num*2),point_num,0);
        }else if(i == (n-1)){
            new_crop_mask = _stitch_seamfinder_create_fullmask(finder,crop_masks+i,point+(i*point_num*2),point_num,2);
        }else{
            new_crop_mask = _stitch_seamfinder_create_fullmask(finder,crop_masks+i,point+(i*point_num*2),point_num,1);
        }
        crop_masks_umat.push_back(new_crop_mask.getUMat(cv::ACCESS_RW).clone());
        // char path[256] = {0};
        // sprintf(path,"low_mask%d.png",i);
        // qtk_stitch_image_save_data(path,crop_masks[i].data,crop_masks[i].cols,crop_masks[i].rows,1);
    }
    std::vector<cv::Point> crop_corners_new;
    crop_corners_new.push_back(cv::Point(0,0));
    crop_corners_new.push_back(crop_corners->at(idx+1)-crop_corners->at(idx));

    finder->finder->find(crop_images_umat,crop_corners_new,crop_masks_umat);
    //update
    cv::UMat new_seammask = crop_masks_umat[0];
    cv::UMat old_seammask = the_low_seammasks->at(idx);
    // char path[256] = {0};
    // sprintf(path,"new_seammask_%d.png",0);
    // qtk_stitch_image_save_data(path,new_seammask.getMat(cv::ACCESS_READ).data,new_seammask.cols,new_seammask.rows,1);
    // sprintf(path,"old_seammask_%d.png",idx);
    // qtk_stitch_image_save_data(path,old_seammask.getMat(cv::ACCESS_READ).data,old_seammask.cols,old_seammask.rows,1);
    _seammasks_update(new_seammask,old_seammask,point+((idx*2+1)*point_num),1);
    // sprintf(path,"update_old_seammask_%d.png",idx);
    // qtk_stitch_image_save_data(path,old_seammask.getMat(cv::ACCESS_READ).data,old_seammask.cols,old_seammask.rows,1);
    new_seammask = crop_masks_umat[1];
    old_seammask = the_low_seammasks->at(idx+1);
    // sprintf(path,"new_seammask_%d.png",1);
    // qtk_stitch_image_save_data(path,new_seammask.getMat(cv::ACCESS_READ).data,new_seammask.cols,new_seammask.rows,1);
    // sprintf(path,"old_seammask_%d.png",idx+1);
    // qtk_stitch_image_save_data(path,old_seammask.getMat(cv::ACCESS_READ).data,old_seammask.cols,old_seammask.rows,1);
    _seammasks_update(new_seammask,old_seammask,point+(((idx+1)*2+1)*point_num),0);
    // sprintf(path,"update_old_seammask_%d.png",idx+1);
    // qtk_stitch_image_save_data(path,old_seammask.getMat(cv::ACCESS_READ).data,old_seammask.cols,old_seammask.rows,1);

    // exit(1);
    return;
}

//选择一部分作为更新拼接缝
void qtk_stitch_seamfinder_findblock_process(qtk_stitch_seamfinder_t *finder,void *low_seammasks,
                            void* corners,void* imgs, void* masks,int *point,int point_num,int idx,int split_num,int start,int end) //开始块和结束块
{
    cv::Mat *crop_imgs = (cv::Mat*)imgs;
    std::vector<cv::Point> *crop_corners = (std::vector<cv::Point>*)corners;
    cv::Mat *crop_masks = (cv::Mat*)masks;
    std::vector<cv::UMat> crop_images_umat;
    std::vector<cv::UMat> crop_masks_umat;
    std::vector<cv::UMat> *the_low_seammasks = (std::vector<cv::UMat>*)low_seammasks;
    int rows = 0;
    int cols = 0;
    std::vector<cv::Mat> low_imgs;
    int nblock = end-start;
    
    if(idx < 0){
        printf("idx error\n");
        return;
    }
    int n = crop_corners->size();
    if(finder->img_type == QTK_STITCH_USE_IMG_RGB){
        for(int i = idx; i <= (idx+1); ++i){ //没测过
            cv::Mat low_img;
            rows = crop_masks[i].rows;
            cols = crop_masks[i].cols;
            cv::resize(crop_imgs[i],low_img,cv::Size(cols,rows), 0, 0, cv::INTER_LINEAR);
            low_imgs.push_back(low_img);
        }
    }else if(finder->img_type == QTK_STITCH_USE_IMG_NV12){
        for(int i = idx; i <= (idx+1); ++i){
            rows = crop_masks[i].rows;
            cols = crop_masks[i].cols;
            cv::Mat low_yuv = qtk_stitch_seamfinder_nv12yuv(crop_imgs[i],rows,cols);
            int u_rows = (rows/split_num)*nblock;
            int u_y = (rows/split_num)*start;
            if(start == (split_num-1)){
                u_rows += rows%split_num;
            }
            // printf("%d %d\n",low_yuv.rows,low_yuv.cols);
            // std::cout << cv::Rect(0,u_y,cols,u_rows) << std::endl;
            low_imgs.push_back(low_yuv(cv::Rect(0,u_y,cols,u_rows)));
            // qtk_stitch_image_save_data("corp_png.jpg",low_imgs[i-idx].data,low_imgs[i-idx].cols,low_imgs[i-idx].rows,low_imgs[i-idx].channels());
            // printf("rows %d cols %d\n",low_imgs[i-idx].rows,low_imgs[i-idx].cols);
        }
    }

    for(size_t i = 0; i < low_imgs.size(); i++){ //转换数据
        cv::Mat um;
        low_imgs[i].assignTo(um,CV_32F);
        crop_images_umat.push_back(um.getUMat(cv::ACCESS_RW));
    }
    for(int i = idx; i <= (idx+1); i++){
        cv::Mat new_crop_mask;
        if(i == 0){
            new_crop_mask = _stitch_seamfinder_create_fullmask(finder,crop_masks+i,point+(i*point_num*2),point_num,0);
        }else if(i == (n-1)){
            new_crop_mask = _stitch_seamfinder_create_fullmask(finder,crop_masks+i,point+(i*point_num*2),point_num,2);
        }else{
            new_crop_mask = _stitch_seamfinder_create_fullmask(finder,crop_masks+i,point+(i*point_num*2),point_num,1);
        }
        rows = new_crop_mask.rows;
        cols = new_crop_mask.cols;
        int u_rows = (rows/split_num)*nblock;
        int u_y = (rows/split_num)*start;
        if(start == (split_num-1)){
            u_rows += rows%split_num;
        }
        // std::cout << cv::Rect(0,u_y,cols,u_rows) << std::endl;
        // printf("%d %d \n",new_crop_mask.rows,new_crop_mask.cols);
        cv::Mat new_crop_mask_block = new_crop_mask(cv::Rect(0,u_y,cols,u_rows));
        int mode = 0;
        if(start == 0){
            mode = 0;
        }else if(end == split_num){
            mode = 2;
        }else{
            mode = 1;
        }
        if(i == idx){
            _stitch_seamfinder_block_mask(new_crop_mask_block,u_y,mode,1,point+(i*point_num*2),point_num);
        }else{
            _stitch_seamfinder_block_mask(new_crop_mask_block,u_y,mode,0,point+(i*point_num*2),point_num);
        }
        crop_masks_umat.push_back(new_crop_mask_block.getUMat(cv::ACCESS_RW));
        // char path[128] = {0};
        // sprintf(path,"crop_mask_%d.png",i);
        // qtk_stitch_image_save_data(path,crop_masks_umat[i-idx].getMat(cv::ACCESS_RW).data,crop_masks_umat[i-idx].cols,crop_masks_umat[i-idx].rows,1);
    }
    std::vector<cv::Point> crop_corners_new;
    crop_corners_new.push_back(cv::Point(0,0));
    crop_corners_new.push_back(crop_corners->at(idx+1)-crop_corners->at(idx));

    finder->finder->find(crop_images_umat,crop_corners_new,crop_masks_umat);
    // for(int i = 0; i < crop_masks_umat.size(); i++){
    //     char path[128] = {0};
    //     sprintf(path,"crop_mask_f_%d.png",i);
    //     qtk_stitch_image_save_data(path,crop_masks_umat[i].getMat(cv::ACCESS_RW).data,crop_masks_umat[i].cols,crop_masks_umat[i].rows,1);
    // }

    //update
    cv::UMat new_seammask = crop_masks_umat[0];
    cv::UMat old_seammask = the_low_seammasks->at(idx);
    _seammasks_update_block(new_seammask,old_seammask,point+((idx*2+1)*point_num),1,split_num,start);
    new_seammask = crop_masks_umat[1];
    old_seammask = the_low_seammasks->at(idx+1);
    _seammasks_update_block(new_seammask,old_seammask,point+(((idx+1)*2+1)*point_num),0,split_num,start);
    return;
}

void _seammasks_update(cv::UMat &new_seamask,cv::UMat &old_seammask,int *point,int is_right)
{
    int rows = old_seammask.rows;
    int cols = old_seammask.cols;
    uchar *data = old_seammask.getMat(cv::ACCESS_RW).data;
    for(int i = 0; i < rows; ++i){
        if(is_right){ //修补右缝
            // memset(data+point[i],255,cols-point[i]);
            int k = MIN(point[i],cols/2);
            memset(data+k,255,cols-k);
        }else{
            // memset(data,255,(point[i]+1));
            int k = MAX(point[i],cols/2);
            memset(data,255,(k+1));
        }
        data+=cols;
    }

    //修补new_seamask非必要部位
    cv::Mat new_seamask_mat = new_seamask.getMat(cv::ACCESS_RW);
    for(int i = 0; i < rows; ++i){
        int j1 = 0;
        int j2 = cols;
        for(int j = 0; j < cols; ++j){
            if(new_seamask_mat.at<uchar>(i,j) == 255 && new_seamask_mat.at<uchar>(i,j1) != 255){
                j1 = j;
            }
            if(new_seamask_mat.at<uchar>(i,j1) == 255 && new_seamask_mat.at<uchar>(i,j) != 255){
                j2 = j;
                break;
            }
        }
        if(is_right){
            // memset(new_seamask_mat.ptr(i),255,j1);
            int k = MAX(j1,cols/2);
            memset(new_seamask_mat.ptr(i),255,k);
        }else{
            // memset(new_seamask_mat.ptr(i)+j2,255,cols-j2);
            int k = MIN(j2,cols/2);
            memset(new_seamask_mat.ptr(i,k),255,cols-k);
        }
    }

    cv::bitwise_and(new_seamask,old_seammask,old_seammask);

    return;
}

void _seammasks_update_block(cv::UMat &new_seamask_block,cv::UMat &old_seammask,int *point,int is_right,int split_num,int start)
{
    int rows = new_seamask_block.rows;
    int cols = new_seamask_block.cols;
    int start_rows = (old_seammask.rows/split_num)*start;
    uchar *data = old_seammask.getMat(cv::ACCESS_RW).data;
    for(int i = start_rows; i < (start_rows+rows); ++i){
        if(is_right){ //修补右缝
            // memset(data+point[i],255,cols-point[i]);
            int k = MIN(point[i],cols/2);
            memset(data+(i*cols)+k,255,cols-k);
        }else{
            // memset(data,255,(point[i]+1));
            int k = MAX(point[i],cols/2);
            memset(data+(i*cols),255,(k+1));
        }
    }

    //修补new_seamask非必要部位
    cv::Mat new_seamask_mat = new_seamask_block.getMat(cv::ACCESS_RW);
    for(int i = 0; i < rows; ++i){
        int j1 = 0;
        int j2 = cols;
        for(int j = 0; j < cols; ++j){
            if(new_seamask_mat.at<uchar>(i,j) == 255 && new_seamask_mat.at<uchar>(i,j1) != 255){
                j1 = j;
            }
            if(new_seamask_mat.at<uchar>(i,j1) == 255 && new_seamask_mat.at<uchar>(i,j) != 255){
                j2 = j;
                break;
            }
        }
        if(is_right){
            // memset(new_seamask_mat.ptr(i),255,j1);
            int k = MAX(j1,cols/2);
            memset(new_seamask_mat.ptr(i),255,k);
        }else{
            // memset(new_seamask_mat.ptr(i)+j2,255,cols-j2);
            int k = MIN(j2,cols/2);
            memset(new_seamask_mat.ptr(i,k),255,cols-k);
        }
    }

    cv::Mat new_seammask(old_seammask.size(),CV_8UC1);
    new_seammask.setTo(255);
    for(int i = start_rows; i < (start_rows+rows);++i){
        for(int j = 0; j < cols; ++j){
            new_seammask.at<uchar>(i,j) = new_seamask_mat.at<uchar>(i-start_rows,j);
        }
    }
    cv::bitwise_and(new_seammask,old_seammask,old_seammask);
    return;
}

// static cv::Mat* _get_mask(std::vector<cv::Mat> &masks,int idx)
// {
//     if(idx < masks.size()){
//         return new cv::Mat(masks[idx]);
//     }else{
//         wtk_debug("Invalid Mask Index!\n");
//         exit(1);
//     }
//     return NULL;
// }
void* qtk_stitch_seamfinder_resize(qtk_stitch_seamfinder_t *finder, void *masks, void *seam_masks_s)
{
    std::vector<cv::UMat> *seam_masks = (std::vector<cv::UMat>*)seam_masks_s;
    cv::Mat *masksp = (cv::Mat*)masks;
    int idx =  0;
    std::vector<cv::UMat>::iterator it;
    cv::Mat mask;
    std::vector<cv::UMat> seam_out;

    for(it = seam_masks->begin();it < seam_masks->end();it++,idx++){
        mask = masksp[idx];
        cv::Mat dilated_mask;
        cv::dilate(cv::InputArray(*it),dilated_mask,cv::noArray());
        cv::Mat resized_seam_mask;
        cv::resize(dilated_mask,resized_seam_mask,cv::Size(mask.cols,mask.rows),0,0,cv::INTER_LINEAR_EXACT);
        cv::UMat bitwise_dst;
        // wtk_debug("%d %d %d %d\n",resized_seam_mask.rows,resized_seam_mask.cols,mask.rows,mask.cols);
        cv::bitwise_and(resized_seam_mask,mask,bitwise_dst);
        // wtk_debug("%d %d\n",bitwise_dst.rows,bitwise_dst.cols);
        // for(int i = 0;i < bitwise_dst.rows;i++){
        //     for(int j = 0;j < bitwise_dst.cols;j++){
        //         printf("%d\n",bitwise_dst.at<uchar>(i,j));
        //     }
        // }
        // exit(1);
        seam_out.push_back(bitwise_dst);
    }
    // finder->resize_seam_masks = (void*)new std::vector<cv::OutputArray>(seam_out);

    return new std::vector<cv::UMat>(seam_out);
}

void qtk_stitch_seamfinder_seam_masks_delete(void *seam_masks)
{
    std::vector<cv::UMat> *d = (std::vector<cv::UMat>*)seam_masks;
    delete d;
    return;
}

void* qtk_stitch_seamfinder_seam_masks_vector_new(void)
{
    return new std::vector<cv::UMat>();
}

void qtk_stitch_seamfinder_seam_masks_vector_push(void *seam_masks,unsigned char *data,int rows,int cols)
{
    std::vector<cv::UMat> *d = (std::vector<cv::UMat>*)seam_masks;
    int nrows = rows;
    int ncols = cols;
    unsigned char *new_data = data;
    // printf("seam masks %d %d\n",rows,cols);
    //把行和列转为2的倍数
    if((rows%2)!=0){
        nrows = rows-1;
    }
    if((cols%2)!=0){
        ncols = cols-1;
    }
    new_data = (unsigned char*)wtk_malloc(nrows*ncols);
    if(nrows == rows && ncols == cols){
        memcpy(new_data,data,nrows*ncols);
    }else{
        for(int i = 0; i < nrows; ++i){
            for(int j = 0; j < ncols; ++j){
                new_data[i*ncols+j] = data[i*cols+j];
            }
        }
    }
    cv::Mat m = cv::Mat(nrows,ncols,CV_8UC1,new_data);
    cv::UMat u = cv::UMat(m.getUMat(cv::ACCESS_RW).clone());
    // for(int i = 0;i < u.rows;i++){
    //     for(int j = 0;j < u.cols;j++){
    //         printf("%d\n",u.getMat(cv::ACCESS_RW).at<unsigned char>(i,j));
    //     }
    // }
    // char pp[25] = {0};
    // sprintf(pp,"./%d_%d_seam.png",u.rows,u.cols);
    // printf("%s\n",pp);
    // qtk_stitch_image_save_data(pp,u.getMat(cv::ACCESS_RW).ptr(),u.cols,u.rows,1);
    d->push_back(u);
    wtk_free(new_data);
    return;
}
//////////////////////////树直拼接缝移动
//生成竖直接缝
//s -> e 是有数据的
void qtk_stitch_seamfinder_seam_masks_line_set(cv::UMat &seammask,int s, int e)
{
    seammask.setTo(0);
    uchar *p = seammask.getMat(cv::ACCESS_RW).ptr<uchar>();
    int row = seammask.rows;
    int col = seammask.cols;
    for(int i = 0; i < row; i++){
        memset(p+i*col+s,255,e-s);
    }
    return;
}

//相对tl的坐标点 作出判断并且移动拼接缝
//idx 从0开始
void qtk_stitch_seamfinder_seam_masks_range(void *seam_masks,void *corners, int x, int y, int w, int h, int idx)
{
    std::vector<cv::Point> *roi_corners = (std::vector<cv::Point>*)corners;
    std::vector<cv::UMat> *d = (std::vector<cv::UMat>*)seam_masks;

    cv::UMat seam_mask = d->at(idx);
    cv::Point tl = roi_corners->at(idx);
    int size = d->size();
    int mask_p[2] = {0,};
    int n_p = 0;
    int s = 0;
    int e = 0;
    int end_x = x+w;
    int use_new_line = 0;

    if(idx > 0 && idx < (size-1)){ //现在只移动中间的图片的拼接缝
        _seam_mask_find_rect(seam_mask,mask_p,&n_p);
        if(n_p != 2){
            printf("_seam_mask_find_rect error\n");
        }
        s = mask_p[0];
        e = mask_p[1];
        if(mask_p[0] > x && mask_p[0] < end_x){ //移动左拼接缝
            cv::UMat seam_mask_pre = d->at(idx-1);
            cv::Point tl_pre = roi_corners->at(idx-1);
            int overlap_x = seam_mask_pre.cols - (tl.x-tl_pre.x);
            int mask_p_pre[2] = {0,};
            int n_p_pre = 0;
            //尽量在这个图片里面
            if(x > 0){
                s = x - 1;
                _seam_mask_find_rect(seam_mask_pre,mask_p_pre,&n_p_pre);
                int s_pre = 0;
                if(n_p_pre != 1){
                    s_pre = mask_p_pre[0];
                }
                int e_pre = (tl.x-tl_pre.x)+s;
                // printf("%d %d %d %d\n",s,e,s_pre,e_pre);
                qtk_stitch_seamfinder_seam_masks_line_set(seam_mask_pre,s_pre,e_pre); //移动前一张照片的拼接缝s
                use_new_line = 1;
            }else if(x == 0 && w < overlap_x - 1){
                s = w+1;
                use_new_line = 1;
                _seam_mask_find_rect(seam_mask_pre,mask_p_pre,&n_p_pre);
                int s_pre = 0;
                if(n_p_pre != 1){
                    s_pre = mask_p_pre[0];
                }
                int e_pre = tl.x+w+1;
                // printf("%d %d\n",s_pre,e_pre);
                qtk_stitch_seamfinder_seam_masks_line_set(seam_mask_pre,s_pre,e_pre); //移动前一张照片的拼接缝s
                use_new_line = 1;
            }
        }
        if(mask_p[1] > x && mask_p[1] < end_x){ //移动右拼接缝
            cv::UMat seam_mask_next = d->at(idx+1);
            cv::Point tl_next = roi_corners->at(idx+1);
            int overlap_x_start = tl_next.x-tl.x;
            int mask_p_next[2] = {0,};
            int n_p_next = 0;
            if(end_x < (seam_mask.cols - 1)){
                e = end_x + 1;
                _seam_mask_find_rect(seam_mask_next,mask_p_next,&n_p_next);
                int s_next = e-(tl_next.x-tl.x);
                int e_next = seam_mask_next.cols;
                if(n_p_next != 1){
                    e_next = mask_p_next[1];
                }
                // printf("%d %d %d %d\n",s,e,s_next,e_next);
                qtk_stitch_seamfinder_seam_masks_line_set(seam_mask_next,s_next,e_next);
                use_new_line = 1;
            }else if(end_x == seam_mask.cols && x > (overlap_x_start+1)){
                e = x -1;
                _seam_mask_find_rect(seam_mask_next,mask_p_next,&n_p_next);
                int s_next = e-overlap_x_start;
                int e_next = seam_mask_next.cols;
                if(n_p_next != 1){
                    e_next = mask_p_next[1];
                }
                // printf("%d %d %d %d\n",s,e,s_next,e_next);
                qtk_stitch_seamfinder_seam_masks_line_set(seam_mask_next,s_next,e_next);
                use_new_line = 1;
            }
        }
        if(use_new_line){
            qtk_stitch_seamfinder_seam_masks_line_set(seam_mask,s,e);
        }
    }

    return;
}

void _seam_mask_find_rect(cv::UMat &seam_mask,int *w,int *n)
{
    int cols = seam_mask.cols;
    uchar *p = seam_mask.getMat(cv::ACCESS_RW).ptr<uchar>();
    uchar a = (*p!=0);
   
    *n = 0;
    for(int i = 0; i < cols; ++i){
        if(a != (p[i]!=0)){
            *w = i;
            w+=1;
            *n+=1;
            a = (p[i]!=0);
        }
    }
    //没有拼接缝的时候 最后就是拼接缝
    if(*n == 0){
        *n+=1;
        *w=cols;
    }
    return;
}

/////////////////////////
//得到low seammaks的size
void qtk_stitch_seamfinder_get_low_seam_size(void *corp_mask,int caram_num, int *w,int *h)
{
    cv::Mat* mask = (cv::Mat*)corp_mask;
    for(int i = 0; i < caram_num; ++i){
        w[i] = mask[i].cols;
        h[i] = mask[i].rows;
    }
    return;
}

void qtk_stitch_seamfinder_seammasks_final2low(void* seam_masks,void *seam_masks_low,int *w, int *h)
{
    std::vector<cv::UMat> *the_seam_masks = (std::vector<cv::UMat>*)seam_masks;
    std::vector<cv::UMat> *the_low_seam_masks = (std::vector<cv::UMat>*)seam_masks_low;

    int n = the_seam_masks->size();
    the_low_seam_masks->clear();
    for(int i = 0; i < n; i++){
        // cv::Mat dilated_mask;
        // cv::dilate(the_seam_masks->at(i),dilated_mask,cv::noArray());
        cv::UMat low_seam_mask;
        cv::resize(the_seam_masks->at(i), low_seam_mask, cv::Size(w[i],h[i]), 0, 0, cv::INTER_LINEAR_EXACT);
        low_seam_mask.setTo(0, low_seam_mask.getMat(cv::ACCESS_RW) < 255); //先试着加上
        the_low_seam_masks->push_back(low_seam_mask);
    }
    // for(int i = 0; i < n; i++){
    //     char pp[25] = {0};
    //     sprintf(pp,"./%d_%d_seam_final.png",the_seam_masks->at(i).rows,the_seam_masks->at(i).cols);
    //     printf("%s\n",pp);
    //     qtk_stitch_image_save_data(pp,the_seam_masks->at(i).getMat(cv::ACCESS_RW).ptr(),the_seam_masks->at(i).cols,the_seam_masks->at(i).rows,1);
    // }
    return;
}

void qtk_stitch_seamfinder_seammasks_low2final(void *masks,void* seam_masks,void *seam_masks_low)
{
    std::vector<cv::UMat> *the_seam_masks = (std::vector<cv::UMat>*)seam_masks;
    std::vector<cv::UMat> *the_low_seam_masks = (std::vector<cv::UMat>*)seam_masks_low;
    cv::Mat *masksp = (cv::Mat*)masks;

    int n = the_seam_masks->size();
    for(int i = 0; i < n; i++){
        int w = the_seam_masks->at(i).cols;
        int h = the_seam_masks->at(i).rows;
        // cv::UMat low_seam_mask;
        // cv::resize(the_low_seam_masks->at(i), the_seam_masks->at(i), cv::Size(w,h), 0, 0, cv::INTER_LINEAR);
        cv::Mat dilated_mask;
        cv::dilate(the_low_seam_masks->at(i),dilated_mask,cv::noArray());
        cv::Mat resized_seam_mask;
        cv::resize(dilated_mask,resized_seam_mask,cv::Size(w,h),0,0,cv::INTER_LINEAR_EXACT);
        cv::UMat bitwise_dst;
        // wtk_debug("%d %d %d %d\n",resized_seam_mask.rows,resized_seam_mask.cols,mask.rows,mask.cols);
        cv::bitwise_and(resized_seam_mask,masksp[i],the_seam_masks->at(i));
    }
    // for(int i = 0; i < n; i++){
    //     char pp[128] = {0};
    //     sprintf(pp,"./%d_%d_seamlow2final.png",the_seam_masks->at(i).rows,the_seam_masks->at(i).cols);
    //     printf("%s\n",pp);
    //     qtk_stitch_image_save_data(pp,the_seam_masks->at(i).getMat(cv::ACCESS_RW).ptr(),the_seam_masks->at(i).cols,the_seam_masks->at(i).rows,1);
    // }
    return;
}

//两行  第一行是左边的拼接缝 第二行是右边的拼接缝 然后计算在在内存的offset值
void _seammask_find_point(cv::UMat &seam_mask,int point_num,int *point, int channels)
{
    cv::Mat seam_mask_mat = seam_mask.getMat(cv::ACCESS_RW);
    int rows = seam_mask_mat.rows;
    int j1 = 0, j2 = 0, jk = 0;
    if(rows > point_num){
        printf("point size error num %d size %d\n",point_num,rows);
        exit(1);
    }
    int cols = seam_mask_mat.cols;
    // printf("rows %d cols %d\n",rows,cols);
    for(int i = 0; i < rows; i++){
        j1 = 0;
        j2 = cols;
        jk = 0;
        for(int j = 1; j < cols; j++){
            if(seam_mask_mat.at<uchar>(i,j) == 255 && seam_mask_mat.at<uchar>(i,j1) != 255){
                j1 = j;
                jk = j;
            }
            if(seam_mask_mat.at<uchar>(i,jk) == 255 && seam_mask_mat.at<uchar>(i,j) != 255){
                j2 = j;
                jk = j;
            }
            if(seam_mask_mat.at<uchar>(i,jk) != 255 && seam_mask_mat.at<uchar>(i,j) == 255){
                jk = j;
            }
        }
        // printf("(%d,%d) (%d,%d)\n",i,j1,i,j2);
        // *point = (i*cols + j1)*channels;
        // point[point_num] = (i*cols + j2)*channels;
        *point = j1;
        point[point_num] = j2;
        point++;
    }
    return;
}

void qtk_stitch_seamfinder_seammasks_find_point(void *seam_masks_low,int point_num,int *points, int channels)
{
    int n = 0;
    std::vector<cv::UMat> *the_low_seam_masks = (std::vector<cv::UMat>*)seam_masks_low;
    n = the_low_seam_masks->size();
    for(int i = 0; i < n; i++){
        _seammask_find_point(the_low_seam_masks->at(i),point_num,points+(i*point_num*2), channels);
    }
    return;
}

float _zncc2(cv::Mat &img1,cv::Mat &img2, int row,int point1, int point2, int half_patch)
{
    #define EPSINON (0.00001)
    float mean1 = 0.0f, mean2 = 0.0f;
    float numerator = 0.0f;
    float denominator1 = 0.0f,denominator2 = 0.0f;
    float denominator = 0.0f;
    float ret = 0.0f;

    int r1 = 0,c1 = 0;
    int r2 = 0, c2 = 0;
    int patch_size = half_patch*2 + 1;
    r1 = row - half_patch;
    r2 = row - half_patch;
    c1 = point1 - half_patch;
    c2 = point2 - half_patch;
    // printf("(%d %d) (%d %d) patch_size %d %d %d %d %d\n",r1,c1,r2,c2,patch_size,img1.rows,img1.cols,img2.rows, img2.cols);
    for(int i = 0; i < patch_size; ++i){
        for(int j = 0; j < patch_size; ++j){
            mean1 += img1.at<uchar>(r1+i,c1+j);
            mean2 += img2.at<uchar>(r2+i,c2+j);
        }
    }
    mean1 = mean1/(patch_size*patch_size);
    mean2 = mean2/(patch_size*patch_size);
    for(int i = 0; i < patch_size; ++i){
        for(int j = 0; j < patch_size; ++j){
            float num1 = 0.0f;
            float num2 = 0.0f;
            num1 = img1.at<uchar>(r1+i,c1+j)-mean1;
            num2 = img2.at<uchar>(r2+i,c2+j)-mean2;
            numerator += num1*num2;
            denominator1 +=  num1*num1;
            denominator2 += num2*num2;
        }
    }
    denominator = sqrtf(denominator1*denominator2);
    
    if((denominator >= -EPSINON) && (denominator <= EPSINON)){
        ret = 0.0f;
    }else{
        ret = numerator/denominator;
    }
    return ret;
}

//在灰度图上作
float _semfinder_seammasks_qseam(cv::Mat &img1,cv::Mat &img2,int *point1,int *point2,int patch_size, int s, int e)
{
    float scores = 0.0f;
    int *left = point1;
    int *right = point2;
    int n = img1.rows;
    int cols = img1.cols;
    int half_patch = patch_size/2;
    float zncc_score = 0.0f;
    int valid_count = 0;
    float total_score = 0.0f;
    int us = MAX(s,half_patch);
    int ue = MIN(e,n-half_patch-1);
    for(int i = us; i < ue; ++i){
        //必须是patch_size*patch_size 大小的方块
        if((cols - left[i]) < half_patch){
            continue;
        }
        if(right[i] < half_patch){
            continue;
        }
        zncc_score = _zncc2(img1,img2,i,left[i],right[i],half_patch);
        total_score += (1.0 - zncc_score + 1) / 2;
        valid_count++;
    }

    if(valid_count == 0){
        scores = 0;
    }else{
        scores = total_score/valid_count;
    }

    return scores;
}

//计算拼接缝得分
void qtk_stitch_seamfinder_seammasks_scores(qtk_stitch_seamfinder_t *seamfinder, void *imgs,void *seam_masks_low,int *points,float *scores,int points_num)
{
    cv::Mat *cropper_imgs = (cv::Mat*)imgs;
    std::vector<cv::UMat> *the_low_seam_masks = (std::vector<cv::UMat>*)seam_masks_low;
    int n = the_low_seam_masks->size();
    int rows = 0;
    int cols = 0;
    std::vector<cv::Mat> low_imgs;
    if(seamfinder->img_type == QTK_STITCH_USE_IMG_RGB){
        printf("the image type error\n");
        exit(1);
        // for(int i = 0; i < n; ++i){
        //     cv::Mat low_img;
        //     rows = the_low_seam_masks->at(i).rows;
        //     cols = the_low_seam_masks->at(i).cols;
        //     cv::resize(cropper_imgs[i],low_img,cv::Size(cols,rows), 0, 0, cv::INTER_NEAREST);
        //     //还要转灰度图 rgb 的还没做
        //     low_imgs.push_back(low_img);
        // }
    }else if(seamfinder->img_type == QTK_STITCH_USE_IMG_NV12){
        for(int i = 0; i < n; ++i){
            cv::Mat low_img;
            int img_rows = cropper_imgs[i].rows/1.5;
            int img_cols = cropper_imgs[i].cols;
            cv::Mat yimg = cv::Mat(cv::Size(img_cols,img_rows),CV_8UC1,cropper_imgs[i].ptr());
            rows = the_low_seam_masks->at(i).rows;
            cols = the_low_seam_masks->at(i).cols;
            cv::resize(yimg,low_img,cv::Size(cols,rows), 0, 0, cv::INTER_NEAREST);
            // printf("%d %d\n",low_img.rows,low_img.cols);
            low_imgs.push_back(low_img);
        }
    }
    for(size_t i = 1; i < low_imgs.size(); ++i){
        //默认patch_size 为 11
        scores[i-1] = _semfinder_seammasks_qseam(low_imgs[i-1],low_imgs[i],points+(points_num*(2*i-1)),points+(points_num*2*i),11,0,low_imgs[i].rows);
    }

    return;
}
//算多段的分值
void qtk_stitch_seamfinder_seammasks_scores_nsp(qtk_stitch_seamfinder_t *seamfinder, 
                        void *imgs,void *seam_masks_low,int *points,float *scores,int points_num,int nsp)
{
    cv::Mat *cropper_imgs = (cv::Mat*)imgs;
    std::vector<cv::UMat> *the_low_seam_masks = (std::vector<cv::UMat>*)seam_masks_low;
    int n = the_low_seam_masks->size();
    int rows = 0;
    int cols = 0;
    std::vector<cv::Mat> low_imgs;
    if(seamfinder->img_type == QTK_STITCH_USE_IMG_RGB){
        printf("the image type error\n");
        exit(1);
        // for(int i = 0; i < n; ++i){
        //     cv::Mat low_img;
        //     rows = the_low_seam_masks->at(i).rows;
        //     cols = the_low_seam_masks->at(i).cols;
        //     cv::resize(cropper_imgs[i],low_img,cv::Size(cols,rows), 0, 0, cv::INTER_NEAREST);
        //     //还要转灰度图 rgb 的还没做
        //     low_imgs.push_back(low_img);
        // }
    }else if(seamfinder->img_type == QTK_STITCH_USE_IMG_NV12){
        for(int i = 0; i < n; ++i){
            cv::Mat low_img;
            int img_rows = cropper_imgs[i].rows/1.5;
            int img_cols = cropper_imgs[i].cols;
            cv::Mat yimg = cv::Mat(cv::Size(img_cols,img_rows),CV_8UC1,cropper_imgs[i].ptr());
            rows = the_low_seam_masks->at(i).rows;
            cols = the_low_seam_masks->at(i).cols;
            cv::resize(yimg,low_img,cv::Size(cols,rows), 0, 0, cv::INTER_NEAREST);
            // printf("%d %d\n",low_img.rows,low_img.cols);
            low_imgs.push_back(low_img);
        }
    }
    for(size_t i = 1; i < low_imgs.size(); ++i){
        //默认patch_size 为 11
        size_t sp = low_imgs[i-1].rows/nsp;
        for(int j = 0; j < nsp; ++j){
            scores[(i-1)*nsp+j] = _semfinder_seammasks_qseam(low_imgs[i-1],low_imgs[i],
                        points+(points_num*(2*i-1)),points+(points_num*2*i),11,sp*j,sp*(j+1));
        }
    }

    return;
}

void* qtk_stitch_seamfinder_seammasks_copy(void* seam_masks)
{
    std::vector<cv::UMat> *src_seam_masks = (std::vector<cv::UMat>*)seam_masks;

    return new std::vector<cv::UMat>(*src_seam_masks);
}


void qtk_stitch_seamfinder_humanseg_find3(qtk_stitch_seamfinder_t *finder,void *low_seammasks,
                        void* corners,void* imgs,void* masks,void *humanseg_masks,int *point,int point_num,int idx)
{
    cv::Mat *crop_imgs = (cv::Mat*)imgs;
    std::vector<cv::Point> *crop_corners = (std::vector<cv::Point>*)corners;
    cv::Mat *crop_masks = (cv::Mat*)masks;
    std::vector<cv::UMat> crop_images_umat;
    std::vector<cv::UMat> crop_masks_umat;
    std::vector<cv::UMat> *the_low_seammasks = (std::vector<cv::UMat>*)low_seammasks;
    std::vector<cv::Mat> *the_humanseg_masks = (std::vector<cv::Mat>*)humanseg_masks;
    int rows = 0;
    int cols = 0;
    std::vector<cv::Mat> low_imgs;
    
    if(idx < 0){
        printf("idx error\n");
        return;
    }
    int n = crop_corners->size();
    if(finder->img_type == QTK_STITCH_USE_IMG_RGB){
        for(int i = idx; i <= (idx+1); ++i){ //没测过
            cv::Mat low_img;
            rows = crop_masks[i].rows;
            cols = crop_masks[i].cols;
            cv::resize(crop_imgs[i],low_img,cv::Size(cols,rows), 0, 0, cv::INTER_LINEAR);
            low_imgs.push_back(low_img);
        }
    }else if(finder->img_type == QTK_STITCH_USE_IMG_NV12){
        for(int i = idx; i <= (idx+1); ++i){
            rows = crop_masks[i].rows;
            cols = crop_masks[i].cols;
            cv::Mat low_yuv = qtk_stitch_seamfinder_nv12yuv(crop_imgs[i],rows,cols);
            low_imgs.push_back(low_yuv);
        }
    }

    for(size_t i = 0; i < low_imgs.size(); i++){
        cv::Mat um;
        low_imgs[i].assignTo(um,CV_32F);
        crop_images_umat.push_back(um.getUMat(cv::ACCESS_RW));
    }
    for(int i = idx; i <= (idx+1); i++){
        // printf("crop_masks %d %d\n",crop_masks[i].rows,crop_masks[i].cols);
        cv::Mat new_crop_mask;
        if(i == 0){
            new_crop_mask = _stitch_seamfinder_create_fullmask(finder,crop_masks+i,point+(i*point_num*2),point_num,0);
        }else if(i == (n-1)){
            new_crop_mask = _stitch_seamfinder_create_fullmask(finder,crop_masks+i,point+(i*point_num*2),point_num,2);
        }else{
            new_crop_mask = _stitch_seamfinder_create_fullmask(finder,crop_masks+i,point+(i*point_num*2),point_num,1);
        }
        //把humanseg mask 放到 mask 上
        if(i == idx+1){
            cv::Mat humanseg = the_humanseg_masks->at(idx);
            for(int i = 0; i < humanseg.rows; ++i){
                for(int j = 0; j < humanseg.cols; ++j){
                    new_crop_mask.at<uchar>(i,j) = humanseg.at<uchar>(i,j);
                }
            }
        }
        crop_masks_umat.push_back(new_crop_mask.getUMat(cv::ACCESS_RW).clone());
        // char path[256] = {0};
        // sprintf(path,"low_mask%d.png",i);
        // qtk_stitch_image_save_data(path,crop_masks[i].data,crop_masks[i].cols,crop_masks[i].rows,1);
    }
    std::vector<cv::Point> crop_corners_new;
    crop_corners_new.push_back(cv::Point(0,0));
    crop_corners_new.push_back(crop_corners->at(idx+1)-crop_corners->at(idx));

    finder->finder->find(crop_images_umat,crop_corners_new,crop_masks_umat);
    //update
    cv::UMat new_seammask = crop_masks_umat[0];
    cv::UMat old_seammask = the_low_seammasks->at(idx);
    // char path[256] = {0};
    // sprintf(path,"new_seammask_%d.png",0);
    // qtk_stitch_image_save_data(path,new_seammask.getMat(cv::ACCESS_READ).data,new_seammask.cols,new_seammask.rows,1);
    // sprintf(path,"old_seammask_%d.png",idx);
    // qtk_stitch_image_save_data(path,old_seammask.getMat(cv::ACCESS_READ).data,old_seammask.cols,old_seammask.rows,1);
    _seammasks_update(new_seammask,old_seammask,point+((idx*2+1)*point_num),1);
    // sprintf(path,"update_old_seammask_%d.png",idx);
    // qtk_stitch_image_save_data(path,old_seammask.getMat(cv::ACCESS_READ).data,old_seammask.cols,old_seammask.rows,1);
    new_seammask = crop_masks_umat[1];
    old_seammask = the_low_seammasks->at(idx+1);
    // sprintf(path,"new_seammask_%d.png",1);
    // qtk_stitch_image_save_data(path,new_seammask.getMat(cv::ACCESS_READ).data,new_seammask.cols,new_seammask.rows,1);
    // sprintf(path,"old_seammask_%d.png",idx+1);
    // qtk_stitch_image_save_data(path,old_seammask.getMat(cv::ACCESS_READ).data,old_seammask.cols,old_seammask.rows,1);
    _seammasks_update(new_seammask,old_seammask,point+(((idx+1)*2+1)*point_num),0);
    // sprintf(path,"update_old_seammask_%d.png",idx+1);
    // qtk_stitch_image_save_data(path,old_seammask.getMat(cv::ACCESS_READ).data,old_seammask.cols,old_seammask.rows,1);

    // exit(1);
    return;
}

//使用humanseg mask 分割mask
void qtk_stitch_seamfinder_humanseg_findblock_process(qtk_stitch_seamfinder_t *finder,void *low_seammasks,
                            void* corners,void* imgs, void* masks,void *humanseg_mask,int *point,int point_num,
                            int idx,int split_num,int start,int end)
{
    cv::Mat *crop_imgs = (cv::Mat*)imgs;
    std::vector<cv::Point> *crop_corners = (std::vector<cv::Point>*)corners;
    cv::Mat *crop_masks = (cv::Mat*)masks;
    std::vector<cv::UMat> crop_images_umat;
    std::vector<cv::UMat> crop_masks_umat;
    std::vector<cv::UMat> *the_low_seammasks = (std::vector<cv::UMat>*)low_seammasks;
    std::vector<cv::Mat> *the_humanseg_mask = (std::vector<cv::Mat>*)humanseg_mask;

    int rows = 0;
    int cols = 0;
    std::vector<cv::Mat> low_imgs;
    int nblock = end-start;
    
    if(idx < 0){
        printf("idx error\n");
        return;
    }
    int n = crop_corners->size();
    if(finder->img_type == QTK_STITCH_USE_IMG_RGB){
        for(int i = idx; i <= (idx+1); ++i){ //没测过
            cv::Mat low_img;
            rows = crop_masks[i].rows;
            cols = crop_masks[i].cols;
            cv::resize(crop_imgs[i],low_img,cv::Size(cols,rows), 0, 0, cv::INTER_LINEAR);
            low_imgs.push_back(low_img);
        }
    }else if(finder->img_type == QTK_STITCH_USE_IMG_NV12){
        for(int i = idx; i <= (idx+1); ++i){
            rows = crop_masks[i].rows;
            cols = crop_masks[i].cols;
            cv::Mat low_yuv = qtk_stitch_seamfinder_nv12yuv(crop_imgs[i],rows,cols);
            int u_rows = (rows/split_num)*nblock;
            int u_y = (rows/split_num)*start;
            if(start == (split_num-1)){
                u_rows += rows%split_num;
            }
            // printf("%d %d %d\n",u_y,u_rows,start);
            // std::cout << cv::Rect(0,u_y,cols,u_rows) << std::endl;
            low_imgs.push_back(low_yuv(cv::Rect(0,u_y,cols,u_rows)));
            // qtk_stitch_image_save_data("corp_png.jpg",low_imgs[i-idx].data,low_imgs[i-idx].cols,low_imgs[i-idx].rows,low_imgs[i-idx].channels());
            // printf("rows %d cols %d\n",low_imgs[i-idx].rows,low_imgs[i-idx].cols);
        }
    }

    for(size_t i = 0; i < low_imgs.size(); i++){ //转换数据
        cv::Mat um;
        low_imgs[i].assignTo(um,CV_32F);
        crop_images_umat.push_back(um.getUMat(cv::ACCESS_RW));
    }

    for(int i = idx; i <= (idx+1); i++){
        cv::Mat new_crop_mask;
        if(i == 0){
            new_crop_mask = _stitch_seamfinder_create_fullmask(finder,crop_masks+i,point+(i*point_num*2),point_num,0);
        }else if(i == (n-1)){
            new_crop_mask = _stitch_seamfinder_create_fullmask(finder,crop_masks+i,point+(i*point_num*2),point_num,2);
        }else{
            new_crop_mask = _stitch_seamfinder_create_fullmask(finder,crop_masks+i,point+(i*point_num*2),point_num,1);
        }
        if(i == idx+1){
            //把humanseg mask 放到 mask 上
            cv::Mat humanseg = the_humanseg_mask->at(idx);
            for(int i = 0; i < humanseg.rows; ++i){
                for(int j = 0; j < humanseg.cols; ++j){
                    new_crop_mask.at<uchar>(i,j) = humanseg.at<uchar>(i,j);
                }
            }
        }
        rows = new_crop_mask.rows;
        cols = new_crop_mask.cols;
        int u_rows = (rows/split_num)*nblock;
        int u_y = (rows/split_num)*start;
        if(start == (split_num-1)){
            u_rows += rows%split_num;
        }
        // std::cout << cv::Rect(0,u_y,cols,u_rows) << std::endl;
        // printf("%d %d \n",new_crop_mask.rows,new_crop_mask.cols);
        cv::Mat new_crop_mask_block = new_crop_mask(cv::Rect(0,u_y,cols,u_rows));
        // char path[128] = {0};
        // sprintf(path,"crop_mask_%d.png",i);
        // qtk_stitch_image_save_data(path,new_crop_mask.data,new_crop_mask.cols,new_crop_mask.rows,1);
        int mode = 0;
        if(start == 0){
            mode = 0;
        }else if(end == split_num){
            mode = 2;
        }else{
            mode = 1;
        }
        if(i == idx){
            _stitch_seamfinder_block_mask(new_crop_mask_block,u_y,mode,1,point+(i*point_num*2),point_num);
        }else{
            _stitch_seamfinder_block_mask(new_crop_mask_block,u_y,mode,0,point+(i*point_num*2),point_num);
        }
        // int rows_i = new_crop_mask_block.rows/2;
        // int cols_ = new_crop_mask_block.cols;
        // for(int rqq = rows_i; rqq < rows_i+10; rqq++){
        //     for(int qq = 0; qq < cols_; qq++){
        //         new_crop_mask_block.at<uchar>(rqq,qq) = 0;
        //     }
        // }
        crop_masks_umat.push_back(new_crop_mask_block.getUMat(cv::ACCESS_RW));
        // char path[128] = {0};
        // sprintf(path,"crop_mask_%d.png",i);
        // qtk_stitch_image_save_data(path,crop_masks_umat[i-idx].getMat(cv::ACCESS_RW).data,crop_masks_umat[i-idx].cols,crop_masks_umat[i-idx].rows,1);
    }
    std::vector<cv::Point> crop_corners_new;
    crop_corners_new.push_back(cv::Point(0,0));
    crop_corners_new.push_back(crop_corners->at(idx+1)-crop_corners->at(idx));

    finder->finder->find(crop_images_umat,crop_corners_new,crop_masks_umat);
    // for(int i = 0; i < crop_masks_umat.size(); i++){
    //     char path[128] = {0};
    //     sprintf(path,"crop_mask_f_%d.png",i);
    //     qtk_stitch_image_save_data(path,crop_masks_umat[i].getMat(cv::ACCESS_RW).data,crop_masks_umat[i].cols,crop_masks_umat[i].rows,1);
    // }
    
    //update
    cv::UMat new_seammask = crop_masks_umat[0];
    cv::UMat old_seammask = the_low_seammasks->at(idx);
    _seammasks_update_block(new_seammask,old_seammask,point+((idx*2+1)*point_num),1,split_num,start);
    // char path[128] = {0};
    // sprintf(path,"update_old_seammask_%d.png",idx);
    // qtk_stitch_image_save_data(path,old_seammask.getMat(cv::ACCESS_READ).data,old_seammask.cols,old_seammask.rows,1);
    new_seammask = crop_masks_umat[1];
    old_seammask = the_low_seammasks->at(idx+1);
    _seammasks_update_block(new_seammask,old_seammask,point+(((idx+1)*2+1)*point_num),0,split_num,start);
    // sprintf(path,"update_old_seammask_%d.png",idx+1);
    // qtk_stitch_image_save_data(path,old_seammask.getMat(cv::ACCESS_READ).data,old_seammask.cols,old_seammask.rows,1);
    return;
}