#include "qtk_stitch_def.h"
#include "qtk_stitch_cropper.h"
#include "wtk/core/wtk_alloc.h"
#include "qtk_stitch_blender.h"
#include <iostream>
#include "opencv2/core/mat.hpp"
#include "opencv2/stitching/detail/motion_estimators.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/core/hal/interface.h"
#include "image/qtk_stitch_image.h"

std::vector<cv::Mat> adjacencies_all_directions(cv::Mat &grid);
cv::Mat horizontal_adjacency_left2right(cv::Mat &grid);
cv::Mat horizontal_adjacency_right2left(cv::Mat &grid);
cv::Mat vertical_adjacency_top2bottom(cv::Mat &grid);
cv::Mat vertical_adjacency_bottom2top(cv::Mat &grid);
int predict_vector_size(cv::Mat &array);
cv::Mat h_vector(cv::Mat &h_adjacency, int x, int y);
cv::Mat v_vector(cv::Mat &v_adjacency, int x, int y);
cv::Mat h_vector_bottom2top(cv::Mat &h_adjacency, int x, int y);
cv::Mat v_vector_right2left(cv::Mat &v_adjacency, int x, int y);
std::vector<cv::Mat> v_vectors_all_directions(cv::Mat &v_top2bottom, cv::Mat &v_bottom2top, int x, int y);
std::vector<cv::Mat> h_vectors_all_directions(cv::Mat &h_left2right, cv::Mat &h_right2left, int x, int y);
std::vector<cv::Mat> spans_all_directions(std::vector<cv::Mat> h_vectors, std::vector<cv::Mat> v_vectors);
int get_n_directions(std::vector<cv::Mat> &spans_all_directions);
std::vector<cv::Point> _get_zero_center_corners(std::vector<cv::Point> &corners);
std::vector<cv::Rect> get_rectangles(std::vector<cv::Point> &corners,std::vector<cv::Size> &sizes);
std::vector<cv::Rect> _get_overlaps(std::vector<cv::Rect> &corners, cv::Rect &lir);
std::vector<cv::Rect> _get_intersections(std::vector<cv::Rect> &rectangles, std::vector<cv::Rect> &overlapping_rectangles);
cv::Mat qtk_stitch_lir(cv::Mat &mask, std::vector<cv::Point> &contour);
cv::Mat largest_interior_rectangle(cv::Mat &grid, std::vector<cv::Point> &contour);
std::vector<cv::Mat> create_maps(std::vector<cv::Mat> &adjacencies, std::vector<cv::Point> &contour);
cv::Mat unique(cv::Mat &m);
std::vector<cv::Mat> get_xy_arrays(int x, int y, std::vector<cv::Mat> &spans_all_directions);
int cell_on_contour(int x, int y, std::vector<cv::Point> &contour);
std::vector<int> biggest_span_in_span_map(cv::Mat span_map);
cv::Mat the_span_map(cv::Mat &grid, cv::Mat &h_adjacency, cv::Mat &v_adjacency);
cv::Mat biggest_span(cv::Mat &spans);
cv::Mat biggest_rectangle(std::vector<int> rect1, std::vector<int> rect2);
// void _stitch_remap_linear(cv::Mat &img,cv::Mat &dst,cv::UMat &uxmap,cv::UMat &uymap);
void _stitch_remap_linear(cv::Mat &img,cv::Mat &dst,cv::Mat &offmap,cv::UMat &uymap);
void _stitch_remap_linear_1channel(cv::Mat &img,cv::Mat &dst,cv::Mat &offmap,cv::UMat &uymap);
void _initInterTab2D(void);
static void _initInterTab2D2(void);

#define INTER_BITS      (5)
#define INTER_BITS2     (INTER_BITS * 2)
#define INTER_TAB_SIZE  (1 << INTER_BITS)
#define INTER_TAB_SIZE2   (INTER_TAB_SIZE * INTER_TAB_SIZE)
#define INTER_MAX           (7)

// static float BilinearTab_f[INTER_TAB_SIZE2][2][2] = {0.f};
static uchar BilinearTab_i[INTER_TAB_SIZE2][2][2] = {0,};
// static uchar NNDeltaTab_i[INTER_TAB_SIZE2][2] = {0,};
const int INTER_REMAP_COEF_BITS=7;
const int INTER_REMAP_COEF_SCALE=(1 << INTER_REMAP_COEF_BITS);

qtk_stitch_cropper_t* qtk_stitch_cropper_new(int do_crop,int ncamera,int remap_type, int img_type)
{
    qtk_stitch_cropper_t* cropper = (qtk_stitch_cropper_t*)wtk_malloc(sizeof(qtk_stitch_cropper_t));
    wtk_debug("do_crop %d ncarmer %d img type %d\n",do_crop,ncamera,img_type);
    cropper->do_crop = do_crop;
    cropper->ncamera = ncamera;
    cropper->remap_type = remap_type;
    cropper->img_type = img_type;

    cropper->overlapping_rectangles = NULL;
    cropper->intersection_rectangles = NULL;
    cropper->masks = NULL;
    cropper->imgs = NULL;
    cropper->corners = NULL;
    cropper->size = NULL;
    cropper->imgs = new cv::Mat[ncamera];
    cropper->masks = new cv::Mat[ncamera];
    cropper->crop_offset_maps = new cv::Mat[ncamera];
    cropper->crop_uv_offset_maps = new cv::Mat[ncamera];
    cropper->concert_maps1 = new cv::Mat[ncamera];
    cropper->concert_maps2 = new cv::Mat[ncamera];
    cropper->crop_offset_table = wtk_strbufs_new(ncamera);
    cropper->crop_offset_uv_table = wtk_strbufs_new(ncamera);
    
    if(remap_type == cv::INTER_LINEAR){
        _initInterTab2D2();
    }
    return cropper;
}

void qtk_stitch_cropper_set_src_rect(qtk_stitch_cropper_t* cropper,int width, int hight)
{
    cropper->width = width;
    cropper->hight = hight;
    return;
}

void* _estimate_panorama_mask(cv::Mat* imgs,cv::Mat* mask,
                            std::vector<cv::Point> &corners,std::vector<cv::Size> &sizes)
{
    void *ret_mask = qtk_stitch_blender_create_panorama((void*)imgs,(void*)mask,(void*)&corners,(void*)&sizes);
    return ret_mask;
}

cv::Rect _estimate_largest_interior_rectangle(cv::Mat &mask)
{
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::Mat lir;
    cv::findContours(mask,contours,hierarchy,cv::RETR_TREE,cv::CHAIN_APPROX_NONE);
    // wtk_debug("%d %d %d\n",mask.rows,mask.cols,mask.channels());
    // std::vector<std::vector<cv::Point>>::iterator it1;
    // for(it1 = contours.begin();it1 != contours.end();it1++){
    //     for(std::vector<cv::Point>::iterator it2 = it1->begin();it2 != it1->end();it2++){
    //         printf("%d %d\n",it2->x,it2->y);
    //     }
    // }
    //有个判断 验证的时候再说
    if(hierarchy.size() != 1){
        wtk_debug("hierarchy.size() != 1 error\n");
        exit(1);
    }
    //计算内部最大矩形
    cv::Mat mm = mask > 0;
    lir = qtk_stitch_lir(mm,contours[0]);
    cv::Rect r = cv::Rect(lir.at<int>(0,0),lir.at<int>(1,0),lir.at<int>(2,0),lir.at<int>(3,0));
    return r;
}



void qtk_stitch_cropper_prepare(qtk_stitch_cropper_t* cropper,void *imgs, void *masks, void *corners,void *sizes)
{
    cv::Mat *warper_imgs = (cv::Mat*)imgs;
    cv::Mat *warper_mask = (cv::Mat*)masks;
    std::vector<cv::Point> *roi_corners = (std::vector<cv::Point>*) corners;
    std::vector<cv::Size> *roi_sizes = (std::vector<cv::Size>*) sizes;
    cv::Mat *result_mask = NULL;
    std::vector<cv::Rect> rectangles;
    std::vector<cv::Point> the_corners;
    std::vector<cv::Rect> overlapping_rectangles;
    std::vector<cv::Rect> intersection_rectangles;

    if(cropper->do_crop){
        result_mask = (cv::Mat*)_estimate_panorama_mask(warper_imgs,warper_mask,*roi_corners,*roi_sizes);
        cv::Rect lir = _estimate_largest_interior_rectangle(*result_mask);
        the_corners = _get_zero_center_corners(*roi_corners);
        rectangles = get_rectangles(the_corners,*roi_sizes);
        overlapping_rectangles = _get_overlaps(rectangles,lir);
        intersection_rectangles = _get_intersections(rectangles,overlapping_rectangles);
        if(cropper->overlapping_rectangles){
            std::vector<cv::Rect> *tmp = (std::vector<cv::Rect>*)cropper->overlapping_rectangles;
            delete tmp;
            cropper->overlapping_rectangles = NULL;
        }
        cropper->overlapping_rectangles = (void*)new std::vector<cv::Rect>(overlapping_rectangles);
        if(cropper->intersection_rectangles){
            std::vector<cv::Rect> *tmp = (std::vector<cv::Rect>*)cropper->intersection_rectangles;
            delete tmp;
            cropper->intersection_rectangles = NULL;
        }
        cropper->intersection_rectangles = (void*)new std::vector<cv::Rect>(intersection_rectangles);
        // for(std::vector<cv::Mat>::iterator it=warper_imgs->begin();it!=warper_imgs->end();it++){
        //     // std::cout << *it << std::endl;
        //     wtk_debug("%d %d %d %d\n",it->rows,it->cols,it->channels(),it->type());
        // }
        // exit(1);
        delete result_mask;
    }
    return;
}

void qtk_stitch_cropper_delete(qtk_stitch_cropper_t* cropper)
{
    if(cropper){
        if(cropper->overlapping_rectangles){
            std::vector<cv::Rect> *tmp = (std::vector<cv::Rect>*)cropper->overlapping_rectangles;
            delete tmp;
            cropper->overlapping_rectangles = NULL;
        }
        if(cropper->intersection_rectangles){
            std::vector<cv::Rect> *tmp = (std::vector<cv::Rect>*)cropper->intersection_rectangles;
            delete tmp;
            cropper->intersection_rectangles = NULL;
        }
        if(cropper->masks){
            cv::Mat *tmp = (cv::Mat*)cropper->masks;
            delete[] tmp;
            cropper->masks = NULL;
        }
        if(cropper->imgs){
            cv::Mat *tmp = (cv::Mat*)cropper->imgs;
            delete[] tmp;
            cropper->imgs = NULL;
        }
        if(cropper->corners){
            std::vector<cv::Point> *tmp = (std::vector<cv::Point>*)cropper->corners;
            delete tmp;
            cropper->corners = NULL;
        }
        if(cropper->size){
            std::vector<cv::Size> *tmp = (std::vector<cv::Size>*)cropper->size;
            delete tmp;
            cropper->size = NULL;
        }
        if(cropper->crop_uxmaps){
            std::vector<cv::UMat> *tmp = (std::vector<cv::UMat>*)cropper->crop_uxmaps;
            delete tmp;
            cropper->crop_uxmaps = NULL;
        }
        if(cropper->crop_uymaps){
            std::vector<cv::UMat> *tmp = (std::vector<cv::UMat>*)cropper->crop_uymaps;
            delete tmp;
            cropper->crop_uymaps = NULL;
        }
        if(cropper->crop_offset_maps){
            cv::Mat *tmp = (cv::Mat*)cropper->crop_offset_maps;
            delete[] tmp;
            cropper->crop_offset_maps = NULL;
        }
        if(cropper->crop_uv_offset_maps){
            cv::Mat *tmp = (cv::Mat*)cropper->crop_uv_offset_maps;
            delete[] tmp;
            cropper->crop_uv_offset_maps = NULL;
        }
        if(cropper->concert_maps1){
            cv::Mat *tmp = (cv::Mat*)cropper->concert_maps1;
            delete[] tmp;
            cropper->concert_maps1 = NULL;
        }
        if(cropper->concert_maps2){
            cv::Mat *tmp = (cv::Mat*)cropper->concert_maps2;
            delete[] tmp;
            cropper->concert_maps2 = NULL;
        }
        if(cropper->crop_offset_table){
            wtk_strbufs_delete(cropper->crop_offset_table,cropper->ncamera);
        }
        if(cropper->crop_offset_uv_table){
            wtk_strbufs_delete(cropper->crop_offset_uv_table,cropper->ncamera);
        }
        wtk_free(cropper);
    }
    return;
}

std::vector<cv::Point> _get_zero_center_corners(std::vector<cv::Point> &corners)
{
    std::vector<cv::Point> ret;
    int min_corner_x = INT_MAX;
    int min_corner_y = INT_MAX;
    int n = corners.size();
    int i = 0;
    for(i = 0; i < n; ++i){
        min_corner_x = MIN(corners[i].x,min_corner_x);
        min_corner_y = MIN(corners[i].y,min_corner_y);
    }

    for(i = 0; i < n; ++i){
        ret.push_back(cv::Point(corners[i].x-min_corner_x,corners[i].y-min_corner_y));
    }
    return ret;
}

std::vector<cv::Rect> get_rectangles(std::vector<cv::Point> &corners,std::vector<cv::Size> &sizes)
{
    std::vector<cv::Rect> rectangles;
    int n = corners.size();
    for(int i = 0; i < n; i+=1){
        rectangles.push_back(cv::Rect(corners[i],sizes[i]));
    }
    return rectangles;
}

cv::Rect _get_overlap(cv::Rect &r1, cv::Rect &r2)
{
    cv::Rect ret;
    int x1 = MAX(r1.x,r2.x);
    int y1 = MAX(r1.y,r2.y);
    int x2 = MIN(r1.x+r1.width,r2.x+r2.width);
    int y2 = MIN(r1.y+r1.height,r2.y+r2.height);
    if(x2 < x1 || y2 < y1){
        wtk_debug("Rectangles do not overlap!\n");
        exit(1);
    }
    // printf("%d %d %d %d\n",x1,x2,y1,y2);
    return cv::Rect(x1,y1,x2-x1,y2-y1);
}

std::vector<cv::Rect> _get_overlaps(std::vector<cv::Rect> &corners, cv::Rect &lir)
{
   std::vector<cv::Rect> overlapping_rectangles;
   for(std::vector<cv::Rect>::iterator it = corners.begin(); it != corners.end(); ++it){
        cv::Rect overlap = _get_overlap(*it,lir);
        overlapping_rectangles.push_back(overlap);
   }
   return overlapping_rectangles;
}

cv::Rect _get_intersection(cv::Rect &rectangle, cv::Rect &overlapping_rectangle)
{
    int x = abs(overlapping_rectangle.x - rectangle.x);
    int y = abs(overlapping_rectangle.y - rectangle.y);
    int width = overlapping_rectangle.width;
    int height = overlapping_rectangle.height;
    return cv::Rect(x, y, width, height);
}

std::vector<cv::Rect> _get_intersections(std::vector<cv::Rect> &rectangles, std::vector<cv::Rect> &overlapping_rectangles)
{
    std::vector<cv::Rect> intersection_rectangles;
    int n = rectangles.size();
    if(rectangles.size() != overlapping_rectangles.size()){
        wtk_debug("intersection error\n");
        exit(1);
    }
    for(int i = 0; i < n; ++i){
        cv::Rect intersection_rectangle = _get_intersection(rectangles[i],overlapping_rectangles[i]);
        intersection_rectangles.push_back(intersection_rectangle);
    }
    return intersection_rectangles;
}

cv::Rect _rect_times(cv::Rect &r,float scale)
{
    int x = roundf(r.x * scale);
    int y = roundf(r.y * scale);
    int width = roundf(r.width * scale);
    int height = roundf(r.height * scale);
    return cv::Rect(x,y,width,height);
}

void _crop_images(qtk_stitch_cropper_t* cropper,cv::Mat* imgs, float aspect,cv::Mat* output)
{
    std::vector<cv::Rect> *intersection_rectangles = (std::vector<cv::Rect>*)cropper->intersection_rectangles;
    int n = cropper->ncamera;
    if(cropper->do_crop){
        for(int i = 0,idx=0; i < n; ++i,++idx){
            cv::Rect intersection_rect = (*intersection_rectangles)[idx];
            cv::Rect rr = _rect_times(intersection_rect,aspect);
            int min_y = MIN(rr.y+rr.height,imgs[i].rows);
            int min_x = MIN(rr.x+rr.width,imgs[i].cols);
            output[i] = imgs[i](cv::Range(rr.y,min_y),cv::Range(rr.x,min_x)).clone();
        }
        // return new std::vector<cv::Mat>(cropped_img);
        return;
    }else{
        for(int i = 0,idx=0; i < n; ++i,++idx){
            output[i] = imgs[i];
        }
    }
    // return new std::vector<cv::Mat>(imgs);
    return;
}

void _crop_image(qtk_stitch_cropper_t* cropper,cv::Mat *img, float aspect,cv::Mat *output,int idx)
{
    std::vector<cv::Rect> *intersection_rectangles = (std::vector<cv::Rect>*)cropper->intersection_rectangles;
    if(cropper->do_crop){
        cv::Rect intersection_rect = (*intersection_rectangles)[idx];
        cv::Rect rr = _rect_times(intersection_rect,aspect);
        int min_y = MIN(rr.y+rr.height,img->rows);
        int min_x = MIN(rr.x+rr.width,img->cols);
        *output = img[0](cv::Range(rr.y,min_y),cv::Range(rr.x,min_x)).clone();
        return;
    }else{
        *output = *img;
    }
    // return new std::vector<cv::Mat>(imgs);
    return;
}

std::vector<cv::Point> get_zero_center_corners(std::vector<cv::Point> &cropped_corners)
{
    std::vector<cv::Point>::iterator it;
    it = cropped_corners.begin();
    int min_corner_x = INT_MAX;
    int min_corner_y = INT_MAX;
    std::vector<cv::Point> cropped_corners_ret;
    
    for(;it < cropped_corners.end(); ++it){
        min_corner_x = MIN(min_corner_x,it->x);
        min_corner_y = MIN(min_corner_y,it->y);
    }
    for(it = cropped_corners.begin();it < cropped_corners.end(); ++it){
        cropped_corners_ret.push_back(cv::Point(it->x-min_corner_x,it->y-min_corner_y));
    }
    return cropped_corners_ret;
}

void _crop_rois(qtk_stitch_cropper_t* cropper, std::vector<cv::Point> &corners, std::vector<cv::Size> &sizes, float aspect)
{
    std::vector<cv::Rect> *overlapping_rectangles = (std::vector<cv::Rect>*)cropper->overlapping_rectangles;
    std::vector<cv::Point> cropped_corners;
    std::vector<cv::Point> cropped_corners_ret;
    std::vector<cv::Size> cropped_sizes;

    if(cropper->corners){
        std::vector<cv::Point> *tmp = (std::vector<cv::Point>*)cropper->corners;
        delete tmp;
        cropper->corners = NULL;
    }
    if(cropper->size){
        std::vector<cv::Size> *tmp = (std::vector<cv::Size>*)cropper->size;
        delete tmp;
        cropper->size = NULL;
    }
    if(cropper->do_crop){
        for(std::vector<cv::Rect>::iterator it = overlapping_rectangles->begin(); it < overlapping_rectangles->end(); ++it){
            cv::Rect r = _rect_times(*it,aspect);
            cropped_corners.push_back(cv::Point(r.x,r.y));
            cropped_sizes.push_back(cv::Size(r.width,r.height));
        }
        cropped_corners_ret = get_zero_center_corners(cropped_corners);
        //改x为偶数倍
        for(std::vector<cv::Point>::iterator it = cropped_corners_ret.begin();it!=cropped_corners_ret.end();++it){
            // std::cout << *it << std::endl;
            // it->x = it->x>>1<<1;
            it->x = it->x+(it->x%2); //可能会引起黒缝 先跑跑看
        }
        cropper->corners = (void*)new std::vector<cv::Point>(cropped_corners_ret);
        cropper->size = (void*)new std::vector<cv::Size>(cropped_sizes);
        return;
    }
    cropper->corners = (void*)new std::vector<cv::Point>(corners);
    cropper->size = (void*)new std::vector<cv::Size>(sizes);
    return;
}

void qtk_stitch_corpper_crop(qtk_stitch_cropper_t* cropper,void *imgs, void *masks, 
                void *corners,void *sizes, float aspect)
{
    cv::Mat *warper_imgs = (cv::Mat*)imgs;
    cv::Mat *warper_mask = (cv::Mat*)masks;
    std::vector<cv::Point> *roi_corners = (std::vector<cv::Point>*) corners;
    std::vector<cv::Size> *roi_sizes = (std::vector<cv::Size>*) sizes;

    _crop_images(cropper,warper_mask,aspect,(cv::Mat*)cropper->masks);
    _crop_images(cropper,warper_imgs,aspect,(cv::Mat*)cropper->imgs);
    _crop_rois(cropper,*roi_corners,*roi_sizes,aspect);

    // std::vector<cv::Mat> *tmp = (std::vector<cv::Mat>*)cropper->masks;
    // for(std::vector<cv::Mat>::iterator it = tmp->begin();it!=tmp->end();it++){
    //     for(int i = 0; i < it->rows; i++){
    //         for(int j = 0; j < it->cols; j++){
    //             printf("%d\n",it->ptr<uchar>(i,j)[0]);
    //             // printf("%d\n",it->ptr<uchar>(i,j)[1]);
    //             // printf("%d\n",it->ptr<uchar>(i,j)[2]);
    //         }
    //     }
    //     // wtk_debug("%d %d %d \n",it->rows,it->cols,it->channels());
    // }
    // exit(1);
    return;
}

void qtk_stitch_corpper_crop_parameter(qtk_stitch_cropper_t* cropper,void *masks, 
                void *corners,void *sizes, float aspect)
{
    cv::Mat *warper_mask = (cv::Mat*)masks;
    std::vector<cv::Point> *roi_corners = (std::vector<cv::Point>*) corners;
    std::vector<cv::Size> *roi_sizes = (std::vector<cv::Size>*) sizes;

    // if(cropper->masks){
    //     std::vector<cv::Mat> *tmp = (std::vector<cv::Mat>*)cropper->masks;
    //     delete tmp;
    //     cropper->masks = NULL;
    // }

    _crop_images(cropper,warper_mask,aspect,(cv::Mat*)cropper->masks);
    _crop_rois(cropper,*roi_corners,*roi_sizes,aspect);

    // std::vector<cv::Mat> *tmp = (std::vector<cv::Mat>*)cropper->masks;
    // for(std::vector<cv::Mat>::iterator it = tmp->begin();it!=tmp->end();it++){
    //     for(int i = 0; i < it->rows; i++){
    //         for(int j = 0; j < it->cols; j++){
    //             printf("%d\n",it->ptr<uchar>(i,j)[0]);
    //             // printf("%d\n",it->ptr<uchar>(i,j)[1]);
    //             // printf("%d\n",it->ptr<uchar>(i,j)[2]);
    //         }
    //     }
    //     // wtk_debug("%d %d %d \n",it->rows,it->cols,it->channels());
    // }
    // std::vector<cv::Size> *tmp = (std::vector<cv::Size>*)cropper->size;
    // for(std::vector<cv::Size>::iterator it = tmp->begin();it!=tmp->end();it++){
    //     std::cout << *it << std::endl;
    // }
    // exit(1);
    return;
}

void qtk_stitch_corpper_crop_imgs(qtk_stitch_cropper_t* cropper,void *imgs, float aspect)
{
    cv::Mat *warper_imgs = (cv::Mat*)imgs;

    // if(cropper->imgs){
    //     std::vector<cv::Mat> *tmp = (std::vector<cv::Mat>*)cropper->imgs;
    //     delete tmp;
    //     cropper->imgs = NULL;
    // }
    _crop_images(cropper,warper_imgs,aspect,(cv::Mat*)cropper->imgs);

    // int n = cropper->ncamera;
    // cv::Mat *tmp = (cv::Mat*)cropper->imgs;
    // for(int k= 0; k < n; ++k){
    //     for(int i = 0; i < tmp[k].rows; i++){
    //         for(int j = 0; j < tmp[k].cols; j++){
    //             printf("%d\n",tmp[k].ptr<uchar>(i,j)[0]);
    //             printf("%d\n",tmp[k].ptr<uchar>(i,j)[1]);
    //             printf("%d\n",tmp[k].ptr<uchar>(i,j)[2]);
    //         }
    //     }
    //     // wtk_debug("%d %d %d \n",it->rows,it->cols,it->channels());
    // }
    // exit(1);
    return;
}
//初始化一回
void qtk_stitch_corpper_crop_maps(qtk_stitch_cropper_t* cropper,void *imgs1,void *imgs2, float aspect)
{
    std::vector<cv::UMat>* uxmaps = (std::vector<cv::UMat>*)imgs1;
    std::vector<cv::UMat>* uymaps = (std::vector<cv::UMat>*)imgs2;
    std::vector<cv::Rect> *intersection_rectangles = (std::vector<cv::Rect>*)cropper->intersection_rectangles;
    int n = cropper->ncamera;
    std::vector<cv::UMat> tmp1;
    std::vector<cv::UMat> tmp2;
    cv::Mat *dst_imgs = (cv::Mat*)cropper->imgs;
    for(int k = 0; k < n; k++){
        cv::Rect intersection_rect = (*intersection_rectangles)[k];
        cv::Rect rr = _rect_times(intersection_rect,aspect);
        int min_y = MIN(rr.y+rr.height,(*uxmaps)[k].rows);
        int min_x = MIN(rr.x+rr.width,(*uxmaps)[k].cols);
        tmp1.push_back((*uxmaps)[k](cv::Range(rr.y,min_y),cv::Range(rr.x,min_x)).clone());
        tmp2.push_back((*uymaps)[k](cv::Range(rr.y,min_y),cv::Range(rr.x,min_x)).clone());
        if(cropper->img_type == QTK_STITCH_USE_IMG_RGB){
            if(cropper->remap_type == cv::INTER_NEAREST || cropper->remap_type == cv::INTER_LINEAR){
                dst_imgs[k].create(tmp1[k].rows,tmp1[k].cols,CV_8UC3);
            }else{
                dst_imgs[k].create(tmp1[k].rows+1,tmp1[k].cols+1,CV_8UC3);
            }
        }else if(cropper->img_type == QTK_STITCH_USE_IMG_NV12){
            int h = tmp1[k].rows*1.5f;
            dst_imgs[k].create(h,tmp1[k].cols,CV_8UC1);
        }
    }
    cropper->crop_uxmaps = new std::vector<cv::UMat>(tmp1);
    cropper->crop_uymaps = new std::vector<cv::UMat>(tmp2);

    return;
}

void qtk_stitch_corpper_clear_crop_maps(qtk_stitch_cropper_t* cropper)
{
    if(cropper->crop_uxmaps){
        std::vector<cv::UMat> *crop_uxmaps = (std::vector<cv::UMat>*)cropper->crop_uxmaps;
        delete crop_uxmaps;
        cropper->crop_uxmaps = NULL;
    }
    if(cropper->crop_uymaps){
        std::vector<cv::UMat> *crop_uymaps = (std::vector<cv::UMat>*)cropper->crop_uymaps;
        delete crop_uymaps;
        cropper->crop_uymaps = NULL;
    }

    return;
}

void _create_offset_map(qtk_stitch_cropper_t* cropper,
                        cv::UMat &uxmap,cv::UMat &uymap,cv::Mat &offset_map,
                        int x_offset,int step)
{
    float *pux = NULL,*puy = NULL;
    short *puxs = NULL;
    int *dmp = offset_map.ptr<int>(0,0);
    int rows = uxmap.rows;
    int cols = uxmap.cols;
    int w = 0;
    int h = 0;
    if(cropper->remap_type == cv::INTER_LINEAR){
        w = cropper->width+x_offset-1;
        h = cropper->hight-1;
        puxs = uxmap.getMat(cv::ACCESS_RW).ptr<short>(0,0);
    }else{
        w = cropper->width+x_offset;
        h = cropper->hight;
        pux = uxmap.getMat(cv::ACCESS_RW).ptr<float>(0,0);
        puy = uymap.getMat(cv::ACCESS_RW).ptr<float>(0,0);
    }

    if(uxmap.channels() == 1){
        for(int i = 0; i < rows; i++){
            for(int j = 0; j < cols; j++){
                int puxi = pux[i*cols+j]+x_offset; //是x轴的偏移
                int puyi = puy[i*cols+j]; //是y轴的偏移
                if(puxi >= w || puyi >= h || puxi < 0 || puyi < 0){
                    dmp[i*cols+j] = 0;
                }else{
    #ifdef USE_9391
                    dmp[i*cols+j] = puxi*4+puyi*4*step;
    #else
                    dmp[i*cols+j] = puxi*3+puyi*3*step;
    #endif
                    // printf("%d\n",puxi*3+puyi*3*w);
                }
            }
        }
    }else if(uxmap.channels() == 2){
        for(int i = 0; i < rows; i++){
            for(int j = 0; j < cols; j++){
                int k = (i*cols+j)*2;
                int puxi = puxs[k]+x_offset; //是x轴的偏移
                int puyi = puxs[k+1]; //是y轴的偏移
                if(puxi >= w || puyi >= h || puxi < 0 || puyi < 0){
                    dmp[i*cols+j] = 0;
                }else{
    #ifdef USE_9391
                    dmp[i*cols+j] = puxi*4+puyi*4*step;
    #else
                    dmp[i*cols+j] = puxi*3+puyi*3*step;
    #endif
                    // printf("%d\n",puxi*3+puyi*3*w);
                }
            }
        }
    }
    return;
}

void _create_nv12_offset_map(qtk_stitch_cropper_t* cropper,
                        cv::UMat &uxmap,cv::UMat &uymap,cv::Mat &offset_map,
                        int x_offset,int step)
{
    float *pux = NULL,*puy = NULL;
    short *puxs = NULL;
    int *dmp = offset_map.ptr<int>(0,0);
    int rows = uxmap.rows;
    int cols = uxmap.cols;
    int w = 0;
    int h = 0;
    if(cropper->remap_type == cv::INTER_LINEAR){
        w = cropper->width+x_offset-1;
        h = cropper->hight-1;
        puxs = uxmap.getMat(cv::ACCESS_RW).ptr<short>(0,0);
    }else{
        w = cropper->width+x_offset;
        h = cropper->hight;
        pux = uxmap.getMat(cv::ACCESS_RW).ptr<float>(0,0);
        puy = uymap.getMat(cv::ACCESS_RW).ptr<float>(0,0);
    }

    if(uxmap.channels() == 1){
        for(int i = 0; i < rows; i++){
            for(int j = 0; j < cols; j++){
                int puxi = pux[i*cols+j]+x_offset; //是x轴的偏移
                int puyi = puy[i*cols+j]; //是y轴的偏移
                if(puxi >= w || puyi >= h || puxi < 0 || puyi < 0){
                    if((i*cols+j) == 0){
                        dmp[i*cols+j] = 0;
                    }else{
                        dmp[i*cols+j] = dmp[i*cols+j-1];
                    }
                }else{
                    dmp[i*cols+j] = puxi+puyi*step;
                    // printf("%d\n",puxi*3+puyi*3*w);
                }
            }
        }
    }else if(uxmap.channels() == 2){
        int max = 0;
        for(int i = 0; i < rows; i++){
            for(int j = 0; j < cols; j++){
                int k = (i*cols+j)*2;
                int puxi = puxs[k]+x_offset; //是x轴的偏移
                int puyi = puxs[k+1]; //是y轴的偏移
                if(puxi >= w || puyi >= h || puxi < 0 || puyi < 0){
                    if((i*cols+j) == 0){
                        dmp[i*cols+j] = 0;
                    }else{
                        dmp[i*cols+j] = dmp[i*cols+j-1];
                    }
                }else{
                    dmp[i*cols+j] = puxi+puyi*step;
                }
                if(dmp[i*cols+j] > max){
                    max = dmp[i*cols+j];
                }
            }
        }
    }
    return;
}

void _create_nv12_uv_offset_map(qtk_stitch_cropper_t* cropper,cv::UMat &uxmap,cv::UMat &uymap,
                        cv::Mat &nv_offset_map,int x_offset,int step)
{
    float *pux = NULL,*puy = NULL;
    short *puxs = NULL;
    int *dmp = nv_offset_map.ptr<int>(0,0);
    int rows = nv_offset_map.rows;
    int cols = nv_offset_map.cols;
    int xcols = uxmap.cols;
    int w = 0;
    int h = 0;
    int base = 0;

    if(cropper->remap_type == cv::INTER_LINEAR){
        w = (cropper->width+x_offset)/2-1;
        h = cropper->hight/2-1;
        puxs = uxmap.getMat(cv::ACCESS_RW).ptr<short>(0,0);
    }else{
        w = (cropper->width+x_offset)/2;
        h = cropper->hight/2;
        pux = uxmap.getMat(cv::ACCESS_RW).ptr<float>(0,0);
        puy = uymap.getMat(cv::ACCESS_RW).ptr<float>(0,0);
    }
    
    base = cropper->hight*step;
    // printf("%d \n",base);

    if(uxmap.channels() == 1){
        for(int i = 0; i < rows; ++i){
            for(int j = 0; j < cols; j++){
                int puxi = (pux[(i*xcols+j)*2]+x_offset)/2; //是x轴的偏移
                int puyi = (puy[(i*xcols+j)*2])/2; //是y轴的偏移
                if(puxi >= w || puyi >= h || puxi < 0 || puyi < 0){
                    dmp[i*cols+j] = base;
                }else{
                    dmp[i*cols+j] = base+puxi*2+puyi*step;//puyi*step/2*2;
                    // printf("%d\n",puxi*3+puyi*3*w);
                }
            }
        }
    }else if(uxmap.channels() == 2){
        for(int i = 0; i < rows; ++i){
            for(int j = 0; j < cols; ++j){
                int k = (i*xcols+j)*2*2;
                int puxi = (puxs[k]+x_offset)/2; //是x轴的偏移
                int puyi = (puxs[k+1])/2; //是y轴的偏移
                if(puxi >= w || puyi >= h || puxi < 0 || puyi < 0){
                    dmp[i*cols+j] = base;
                }else{
                    dmp[i*cols+j] = base+puxi*2+puyi*step;//puyi*step/2*2;
                    // printf("%d\n",puxi*3+puyi*3*w);
                }
            }
        }
    }
    return;
}

//这个是双插值采样ymap的优化 ymap在fastmap之后是查表的量化查表
void _create_offset_map_ymap(qtk_stitch_cropper_t* cropper,cv::UMat &uymap)
{
    cv::Mat uymap_ = uymap.getMat(cv::ACCESS_READ);
    ushort *dmp = uymap_.ptr<ushort>(0,0);
    int rows = uymap_.rows;
    int cols = uymap_.cols;
    for(int i = 0; i < rows; ++i){
        for(int j = 0; j < cols; ++j){
            *dmp = *dmp*4;
            ++dmp;
        }
    }
    return;
}

void qtk_stitch_corpper_create_offset_maps(qtk_stitch_cropper_t* cropper)
{
    std::vector<cv::UMat>* uxmaps = (std::vector<cv::UMat>*)cropper->crop_uxmaps;
    std::vector<cv::UMat>* uymaps = (std::vector<cv::UMat>*)cropper->crop_uymaps;
    cv::Mat *concert_maps1 = (cv::Mat*)cropper->concert_maps1; //cv::Mat[]
    cv::Mat *concert_maps2 = (cv::Mat*)cropper->concert_maps2; //cv::Mat[]
    int size = uxmaps->size();
    cv::Mat *offset_maps = (cv::Mat*)cropper->crop_offset_maps;
    cv::Mat *uv_offset_maps = (cv::Mat*)cropper->crop_uv_offset_maps;
    cv::UMat uxmap,uymap;
    for(int idx = 0; idx < size; idx++){
        if(cropper->remap_type == cv::INTER_NEAREST){
            uxmap = (*uxmaps)[idx];
            uymap = (*uymaps)[idx];
        }else if(cropper->remap_type == cv::INTER_LINEAR){
            uxmap = concert_maps1[idx].getUMat(cv::ACCESS_RW);
            uymap = concert_maps2[idx].getUMat(cv::ACCESS_RW);
            _create_offset_map_ymap(cropper,uymap);
        }
        offset_maps[idx].create(uxmap.rows,uxmap.cols,CV_32S);
        //如果是nv12格式的话是要创建一个新的nv12 nv部分的偏移表
        if(cropper->img_type == QTK_STITCH_USE_IMG_NV12){
            _create_nv12_offset_map(cropper,uxmap,uymap,offset_maps[idx],0,cropper->width);
            uv_offset_maps[idx].create(uxmap.rows/2,uxmap.cols/2,CV_32S);
            _create_nv12_uv_offset_map(cropper,uxmap,uymap,uv_offset_maps[idx],0,cropper->width);
        }else if(cropper->img_type == QTK_STITCH_USE_IMG_RGB){
            _create_offset_map(cropper,uxmap,uymap,offset_maps[idx],0,cropper->width);
        }
    }
    cropper->crop_offset_maps = offset_maps;
    return;
}

//一大张图片的
void qtk_stitch_corpper_create_offset_maps_all(qtk_stitch_cropper_t* cropper)
{
    std::vector<cv::UMat>* uxmaps = (std::vector<cv::UMat>*)cropper->crop_uxmaps;
    std::vector<cv::UMat>* uymaps = (std::vector<cv::UMat>*)cropper->crop_uymaps;
    cv::Mat *concert_maps1 = (cv::Mat*)cropper->concert_maps1; //cv::Mat[]
    cv::Mat *concert_maps2 = (cv::Mat*)cropper->concert_maps2; //cv::Mat[]
    int size = uxmaps->size();
    cv::Mat *offset_maps = (cv::Mat*)cropper->crop_offset_maps;
    cv::Mat *uv_offset_maps = (cv::Mat*)cropper->crop_uv_offset_maps;
    cv::UMat uxmap,uymap;
    int step = cropper->width*cropper->ncamera;

    for(int idx = 0; idx < size; idx++){
        if(cropper->remap_type == cv::INTER_NEAREST){
            uxmap = (*uxmaps)[idx];
            uymap = (*uymaps)[idx];
        }else if(cropper->remap_type == cv::INTER_LINEAR){
            uxmap = concert_maps1[idx].getUMat(cv::ACCESS_RW);
            uymap = concert_maps2[idx].getUMat(cv::ACCESS_RW);
            _create_offset_map_ymap(cropper,uymap);
        }
        offset_maps[idx].create(uxmap.rows,uxmap.cols,CV_32S);
        if(cropper->img_type == QTK_STITCH_USE_IMG_NV12){
            _create_nv12_offset_map(cropper,uxmap,uymap,offset_maps[idx],idx*cropper->width,step);
            uv_offset_maps[idx].create(uxmap.rows/2,uxmap.cols/2,CV_32S);
            _create_nv12_uv_offset_map(cropper,uxmap,uymap,uv_offset_maps[idx],idx*cropper->width,step);
        }else if(cropper->img_type == QTK_STITCH_USE_IMG_RGB){
            _create_offset_map(cropper,uxmap,uymap,offset_maps[idx],idx*cropper->width,step);
        }
    }
    cropper->crop_offset_maps = offset_maps;
    return;
}

void _stitch_corpper_nearest_2table(cv::Mat *offset, wtk_strbuf_t *table, int step)
{
    int rows = offset->rows;
    int cols = offset->cols;
    int n = rows * cols;
    int *dmp = offset->ptr<int>(0,0);

    int j = 0;
    int k[3] = {0,};
    for(int i = 1; i < n; ++i){
        if(dmp[i-1]+step == dmp[i]){
            continue;
        }
        k[0] = dmp[j];
        k[1] = (i - j)*step;
        k[2] = j*step;
        wtk_strbuf_push(table,(char*)k,sizeof(int)*3);
        j = i;
    }
    k[0] = dmp[j];
    k[1] = (n - j)*step;
    k[2] = j*step;
    wtk_strbuf_push(table,(char*)k,sizeof(int)*3);
    return;
}

//把邻近插值算法的offset转换为批量处理的做法
void qtk_stitch_corpper_nearest_2tables(qtk_stitch_cropper_t* cropper)
{
    int n = cropper->ncamera;
    cv::Mat *crop_offset_maps = (cv::Mat*)cropper->crop_offset_maps;
    cv::Mat *crop_offset_uv_maps = (cv::Mat*)cropper->crop_uv_offset_maps;
    if(cropper->img_type == QTK_STITCH_USE_IMG_NV12){
        for(int i = 0 ; i < n; ++i){
            _stitch_corpper_nearest_2table(crop_offset_maps+i,cropper->crop_offset_table[i],1);
            // printf("tab1 %d\n",cropper->crop_offset_table[i]->pos);
            _stitch_corpper_nearest_2table(crop_offset_uv_maps+i,cropper->crop_offset_uv_table[i],2);
            // printf("tab2 %d\n",cropper->crop_offset_uv_table[i]->pos);
        }
    }
    return;
}
//使用convertMaps转成快速方式
void qtk_stitch_corpper_create_fast_maps(qtk_stitch_cropper_t* cropper)
{
    std::vector<cv::UMat>* uxmaps = (std::vector<cv::UMat>*)cropper->crop_uxmaps;
    std::vector<cv::UMat>* uymaps = (std::vector<cv::UMat>*)cropper->crop_uymaps;
    cv::Mat *convertmaps1 = (cv::Mat*)cropper->concert_maps1;
    cv::Mat *convertmaps2 = (cv::Mat*)cropper->concert_maps2;
    int size = uxmaps->size();
    for(int idx = 0; idx < size; idx++){
        cv::UMat uxmap = (*uxmaps)[idx];
        cv::UMat uymap = (*uymaps)[idx];
        cv::Mat dst;
        cv::convertMaps(uxmap,uymap,convertmaps1[idx],convertmaps2[idx],CV_16SC2);
        // printf("%d %d\n",convertmaps1[idx].type(),convertmaps2[idx].type());
    }

    return;
}

//邻近插值法
#if 0
void _stitch_remap_nearest(cv::Mat &img,cv::Mat &dst,cv::UMat &uxmap,cv::UMat &uymap)
{
    uchar *p = img.ptr<uchar>(0,0);
    float *pux = uxmap.getMat(cv::ACCESS_RW).ptr<float>(0,0);
    float *puy = uymap.getMat(cv::ACCESS_RW).ptr<float>(0,0);
    uchar *dstp = dst.ptr<uchar>(0,0);
    int rows = dst.rows;
    int cols = dst.cols;
    int img_rows = img.rows;
    int img_cols = img.cols;
    for(int i = 0; i < rows; i++){
        for(int j = 0; j < cols; j++){
            int puxi = pux[i*cols+j];
            int puyi = puy[i*cols+j];
            if(puxi >= img_cols || puyi >= img_rows || puxi < 0 || puyi < 0){
                dstp[i*cols*3+j*3] = 0;
                dstp[i*cols*3+j*3+1] = 0;
                dstp[i*cols*3+j*3+2] = 0;
            }else{
                dstp[i*cols*3+j*3] = p[puxi*3+puyi*3*img_cols];
                dstp[i*cols*3+j*3+1] = p[puxi*3+puyi*3*img_cols+1];
                dstp[i*cols*3+j*3+2] = p[puxi*3+puyi*3*img_cols+2];
            }
        }
    }
    return;
}
#else
void _stitch_remap_nearest(cv::Mat &img,cv::Mat &dst,cv::Mat &offset_map)
{
    uchar *p = img.ptr<uchar>(0,0);
    int *ofmp = offset_map.ptr<int>(0,0);
    uchar *dstp = dst.ptr<uchar>(0,0);
    int rows = dst.rows;
    int cols = dst.cols;
    int n = rows * cols;
    int i = 0;
    uchar *pp = NULL;
    // for(pp = p+*ofmp; i < n-1;++i,dstp+=3,++ofmp,pp = p+*ofmp){
    for(pp = p+*ofmp; i < n-1;++i,dstp+=3,pp = p+*(++ofmp)){
        // *((int*)dstp)=*((int*)pp);
        memcpy(dstp,pp,3);
    }
    memcpy(dstp,pp,3);
    // for(; i < n; ++i){
    //     memcpy(dstp,p+ofmp[i],3);
    //     dstp+=3;
    // }
    return;
}

void _stitch_remap_nearest_1channel(cv::Mat &img,cv::Mat &dst,cv::Mat &offset_map)
{
    uchar *p = img.ptr<uchar>(0,0);
    int *ofmp = offset_map.ptr<int>(0,0);
    uchar *dstp = dst.ptr<uchar>(0,0);
    int rows = offset_map.rows;
    int cols = offset_map.cols;
    int n = rows * cols;
    int i = 0;
    uchar *pp = NULL;
    for(pp = p+*ofmp; i < n;++i,++dstp,pp = p+*(++ofmp)){
        // *((int*)dstp)=*((int*)pp);
        *dstp = *pp;
    }
    return;
}

void _stitch_remap_nearest_2channel(cv::Mat &img,cv::Mat &dst,cv::Mat &offset_map)
{
    uchar *p = img.ptr<uchar>(0,0);
    int *ofmp = offset_map.ptr<int>(0,0);
    uchar *dstp = (uchar *)dst.ptr();
    int rows = offset_map.rows;
    int cols = offset_map.cols;
    int n = rows * cols;
    int i = 0;
    uchar *pp = NULL;
    for(pp = p+*ofmp; i < n;++i,dstp+=2,pp = p+*(ofmp)){
        // *((int*)dstp)=*((int*)pp);
        dstp[0] = pp[0];
        dstp[1] = pp[1];
        ++ofmp;
    }
    return;
}

void _stitch_remap_nearest_table_1channel(cv::Mat &img,cv::Mat &dst,cv::Mat &table)
{
    uchar *p = img.ptr<uchar>(0,0);
    int *ofmp = table.ptr<int>(0,0);
    uchar *dstp = dst.ptr<uchar>(0,0);
    int rows = table.rows;
    int n = rows * 1;
    int i = 0;
    for(; i < n; ++i){
        int sof = ofmp[0];
        int len = ofmp[1];
        int dof = ofmp[2];
        memcpy(dstp+dof,p+sof,len);
        ofmp+=3;
    }
    return;
}

void _stitch_remap_nearest_table_2channel(cv::Mat &img,cv::Mat &dst,cv::Mat &table)
{
    uchar *p = img.ptr<uchar>(0,0);
    int *ofmp = table.ptr<int>(0,0);
    uchar *dstp = dst.ptr<uchar>(0,0);
    int rows = table.rows;
    int n = rows * 1;
    int i = 0;
    for(; i < n; ++i){
        int sof = ofmp[0];
        int len = ofmp[1];
        int dof = ofmp[2];
        memcpy(dstp+dof,p+sof,len);
        ofmp+=3;
    }
    return;
}

#ifndef QTK_THREAD
class cromper_remape_parallel: public cv::ParallelLoopBody
{
public:
    cromper_remape_parallel(int _type, cv::Mat *_img,cv::Mat *_dst,cv::Mat *_offset_map)
    {
        img = _img;
        dst = _dst;
        offset_map = _offset_map;
        type = _type;
    }
    cromper_remape_parallel(int _type, cv::Mat *_img,cv::Mat *_dst,cv::Mat *_offset_map,cv::Mat *_uymap)
    {
        img = _img;
        dst = _dst;
        offset_map = _offset_map;
        type = _type;
        uymap = _uymap;
    }
    ~cromper_remape_parallel(){};
    void operator()(const cv::Range& range) const;
private:
    cv::Mat *img;
    cv::Mat *dst;
    cv::Mat *offset_map;
    cv::Mat *uymap;
    int type;
};

void _stitch_remap_linear_rect(cv::Mat &img,cv::Mat &dst,cv::Mat &offset_map,cv::Mat &uymap, int s, int e)
{
    cv::Size dsize = dst.size();
    uchar* wtab = (uchar*)BilinearTab_i[0][0];
    uchar* S0 = img.ptr<uchar>();
    size_t sstep = img.step/sizeof(S0[0]);
    int width = dsize.width;
    int* XY = offset_map.ptr<int>(s,0);
    uchar* D = dst.ptr<uchar>(s,0);
    ushort* FXY = uymap.ptr<ushort>(s,0);
    uchar* w = NULL;
    uchar* S = NULL;
    int cross_n = width * (e-s);
    size_t sstep1 = sstep+1;
    size_t sstep2 = sstep1+1;

    int n = 0;
#if USE_NEON
    int cross_n2 = cross_n-2;
    uchar* w2 = NULL;
    uchar *S2 = NULL;
    uchar SS1[8] = {0,};
    uchar SS2[8] = {0,};
    uchar ww1[8] = {0,};
    uchar ww2[8] = {0,};
    uchar DD[8] = {0,};
    for(;n < cross_n2; n+=2){
        w = wtab + *FXY;
        S = S0 + *XY;
        w2 = wtab + *(FXY+1);
        S2 = S0 + *(XY+1);
        memcpy(SS1,S,4);
        memcpy(SS1+4,S2,4);
        memcpy(SS2,S+sstep,4);
        memcpy(SS2+4,S2+sstep,4);
        memset(ww1,w[0],4);
        memset(ww1+4,w2[0],4);
        memset(ww2,w[1],4);
        memset(ww2+4,w2[1],4);

        uint8x8_t S1v = vld1_u8(SS1);
        uint8x8_t S2v = vld1_u8(SS2);
        uint8x8_t w1v = vld1_u8(ww1);
        uint8x8_t w2v = vld1_u8(ww2);

        uint16x8_t S1wv = vmull_u8(S1v, w1v);
        uint16x8_t S2wv = vmlal_u8(S1wv, S2v, w2v);
        uint8x8_t Dv = vqrshrun_n_s16((int16x8_t)S2wv,7);

        vst1_u8(DD, Dv);
        memcpy(D,DD,4);
        memcpy(D+3,DD+4,4);
        XY+=2;
        FXY+=2;
        D+=6;
    }
#endif
    for(; n < cross_n; ++n,++XY,++FXY){
        w = wtab + *FXY;
        S = S0 + *XY;

        D[0] = (S[0]*w[0] + S[sstep]*w[1])>>INTER_REMAP_COEF_BITS;
        D[1] = (S[1]*w[0] + S[sstep1]*w[1])>>INTER_REMAP_COEF_BITS;
        D[2] = (S[2]*w[0] + S[sstep2]*w[1])>>INTER_REMAP_COEF_BITS;
        D += 3;
    }
    return;
}

void _stitch_remap_linear_1channel_rect(cv::Mat &img,cv::Mat &dst,cv::Mat &offset_map,cv::Mat &uymap, int s, int e)
{
    uchar* wtab = (uchar*)BilinearTab_i[0][0];
    uchar* S0 = img.ptr<uchar>();
    size_t sstep = img.step/sizeof(S0[0]);
    int width = offset_map.cols;
    int* XY = offset_map.ptr<int>(s,0);
    uchar* D = dst.ptr<uchar>(s,0);
    ushort* FXY = uymap.ptr<ushort>(s,0);
    uchar* w = NULL;
    uchar* S = NULL;

    int cross_n = width * (e-s);
    int cross_n_p = cross_n - 4;
    uchar w11 = 0;
    uchar w12 = 0;
    uchar w21 = 0;
    uchar w22 = 0;
    uchar w31 = 0;
    uchar w32 = 0;
    uchar w41 = 0;
    uchar w42 = 0;
    uchar s11 = 0;
    uchar s12 = 0;
    uchar s21 = 0;
    uchar s22 = 0;
    uchar s31 = 0;
    uchar s32 = 0;
    uchar s41 = 0;
    uchar s42 = 0;
    int n = 0;
    for(;n < cross_n_p;n+=4){
        // w = wtab + *FXY;
        // S = S0 + *XY;

        w = wtab + FXY[0];
        w11 = w[0];
        w12 = w[1];
        w = wtab + FXY[1];
        w21 = w[0];
        w22 = w[1];
        w = wtab + FXY[2];
        w31 = w[0];
        w32 = w[1];
        w = wtab + FXY[3];
        w41 = w[0];
        w42 = w[1];
        
        S = S0 + XY[0];
        s11 = S[0];
        s12 = S[sstep];
        S = S0 + XY[1];
        s21 = S[0];
        s22 = S[sstep];
        S = S0 + XY[2];
        s31 = S[0];
        s32 = S[sstep];
        S = S0 + XY[3];
        s41 = S[0];
        s42 = S[sstep];
        
        D[0] = (s11*w11 + s12*w12)>>INTER_REMAP_COEF_BITS;
        D[1] = (s21*w21 + s22*w22)>>INTER_REMAP_COEF_BITS;
        D[2] = (s31*w31 + s32*w32)>>INTER_REMAP_COEF_BITS;
        D[3] = (s41*w41 + s42*w42)>>INTER_REMAP_COEF_BITS;
        
        D+=4;
        XY+=4;
        FXY+=4;
    }
    for(; n < cross_n; ++n,++XY){
        // w = wtab + *FXY;
        S = S0 + *XY;

        // D[0] = (S[0]*w[0] + S[sstep]*w[1])>>INTER_REMAP_COEF_BITS;
        D[0] = S[0];
        ++D;
    }
    return;
}

void _stitch_remap_nearest_rect(cv::Mat &img,cv::Mat &dst,cv::Mat &offset_map, int s, int e)
{
    uchar *p = img.ptr<uchar>(0,0);
    int *ofmp = offset_map.ptr<int>(s,0);
    uchar *dstp = dst.ptr<uchar>(s,0);
    int rows = e-s;
    int cols = dst.cols;
    int n = rows * cols;
    int i = 0;
    uchar *pp = NULL;
    
    for(pp = p+*ofmp; i < n-1;++i,dstp+=3,pp = p+*(++ofmp)){
        memcpy(dstp,pp,3);
    }
    memcpy(dstp,pp,3);
    return;
}

void _stitch_remap_nearest_1channel_rect(cv::Mat &img,cv::Mat &dst,cv::Mat &offset_map,int s, int e)
{
    uchar *p = img.ptr<uchar>(0,0);
    int *ofmp = offset_map.ptr<int>(s,0);
    uchar *dstp = dst.ptr<uchar>(s,0);
    int rows = e-s;
    int cols = offset_map.cols;
    int n = rows * cols;
    uchar *pp = NULL;
    uchar *pp2 = NULL;
    uchar *pp3 = NULL;
    uchar *pp4 = NULL;
#if 0
    int i = 0;
    int cross_n = n>>2<<2;
    for(; i < cross_n;i+=4){
        // *((int*)dstp)=*((int*)pp);
        pp = p+ofmp[0];
        pp2 = p+ofmp[1];
        pp3 = p+ofmp[2];
        pp4 = p+ofmp[3];

        dstp[0] = pp[0];
        dstp[1] = pp2[0];
        dstp[2] = pp3[0];
        dstp[3] = pp4[0];
        // dstp[0] = *(p+ofmp[0]);
        // dstp[1] = *(p+ofmp[1]);
        // dstp[2] = *(p+ofmp[2]);
        // dstp[3] = *(p+ofmp[3]);
 
        dstp+=4;
        ofmp+=4;
    }
    for(; i < n;++i){
        // pp = p+*ofmp;
        // *dstp = *pp;
        dstp[0] = *(p+ofmp[0]);
        ++dstp;
        ++ofmp;
    }
#else 
    uchar *dstp_e = dstp+n;
    uchar *dstp_en = dstp_e-4;
    while(dstp < dstp_en){
        pp = p+ofmp[0];
        pp2 = p+ofmp[1];
        pp3 = p+ofmp[2];
        pp4 = p+ofmp[3];

        dstp[0] = pp[0];
        dstp[1] = pp2[0];
        dstp[2] = pp3[0];
        dstp[3] = pp4[0];

        dstp+=4;
        ofmp+=4;
    }
    while(dstp < dstp_e){
        dstp[0] = *(p+ofmp[0]);
        ++dstp;
        ++ofmp;
    }
#endif
    return;
}

void _stitch_remap_nearest_2channel_rect(cv::Mat &img,cv::Mat &dst,cv::Mat &offset_map,int s, int e)
{
    uchar *p = img.ptr<uchar>(0,0);
    int *ofmp = offset_map.ptr<int>(s,0);
    ushort *dstp = (ushort *)dst.ptr(s,0);
    int rows = e-s;
    int cols = offset_map.cols;
    int n = rows * cols;
    uchar *pp = NULL;
    uchar *pp2 = NULL;
    uchar *pp3 = NULL;
    uchar *pp4 = NULL;
#if 0
    int i = 0;
    int cross_n = n>>2<<2;
    for(;i < cross_n;i+=4){
        // *((int*)dstp)=*((int*)pp);
        // dstp[0] = pp[0];
        // dstp[1] = pp[1];
        pp = p+ofmp[0];
        pp2 = p+ofmp[1];
        pp3 = p+ofmp[2];
        pp4 = p+ofmp[3];

        dstp[0] = ((ushort*)pp)[0];
        dstp[1] = ((ushort*)pp2)[0];
        dstp[2] = ((ushort*)pp3)[0];
        dstp[3] = ((ushort*)pp4)[0];
        ofmp+=4;
        dstp+=4;
    }
    for(;i < n;++i){
        pp = p+ofmp[0];
        dstp[0] = ((ushort*)pp)[0];
        ++ofmp;
        ++dstp;
    }
#else
    ushort *dstp_e = dstp+n;
    ushort *dstp_en = dstp+(n-4);
    while(dstp < dstp_en){
        pp = p+ofmp[0];
        pp2 = p+ofmp[1];
        pp3 = p+ofmp[2];
        pp4 = p+ofmp[3];

        dstp[0] = ((ushort*)pp)[0];
        dstp[1] = ((ushort*)pp2)[0];
        dstp[2] = ((ushort*)pp3)[0];
        dstp[3] = ((ushort*)pp4)[0];

        dstp+=4;
        ofmp+=4;
    }
    while(dstp < dstp_e){
        dstp[0] = *((ushort*)(p+ofmp[0]));
        ++dstp;
        ++ofmp;
    }
#endif
    return;
}

void cromper_remape_parallel::operator()(const cv::Range& range) const
{
    if(type==cv::INTER_NEAREST){
        if(dst->channels() == 1){
            _stitch_remap_nearest_1channel_rect(*img,*dst,*offset_map,range.start,range.end);
        }else if(dst->channels() == 2){
            _stitch_remap_nearest_2channel_rect(*img,*dst,*offset_map,range.start,range.end);
        }else if(dst->channels() == 3){
            _stitch_remap_nearest_rect(*img,*dst,*offset_map,range.start,range.end);
        }
    }else if(type==cv::INTER_LINEAR){
        if(dst->channels() == 1){
            _stitch_remap_linear_1channel_rect(*img,*dst,*offset_map,*uymap,range.start,range.end);
        }else if(dst->channels() == 3){
            _stitch_remap_linear_rect(*img,*dst,*offset_map,*uymap,range.start,range.end);
        }
    }
    return;
}

class cromper_table_remape_parallel: public cv::ParallelLoopBody
{
public:
    cromper_table_remape_parallel(int _type, cv::Mat *_img,cv::Mat *_dst,cv::Mat *_table)
    {
        img = _img;
        dst = _dst;
        table = _table;
        type = _type;
    }
    ~cromper_table_remape_parallel(){};
    void operator()(const cv::Range& range) const;
private:
    cv::Mat *img;
    cv::Mat *dst;
    cv::Mat *table;
    int type;
};

void _stitch_remap_nearest_table_1channel_rect(cv::Mat &img,cv::Mat &dst,cv::Mat &table,int s, int e)
{
    uchar *p = img.ptr<uchar>(0,0);
    int *ofmp = table.ptr<int>(s,0);
    uchar *dstp = dst.ptr<uchar>(0,0);
    int rows = e-s;
    int n = rows * 1;
    int i = 0;

    for(; i < n; ++i){
        int sof = ofmp[0];
        int len = ofmp[1];
        int dof = ofmp[2];
        switch(len){
        case 1:
            memcpy(dstp+dof,p+sof,1);
            break;
        case 2:
            memcpy(dstp+dof,p+sof,2);
            break;
        case 3:
            memcpy(dstp+dof,p+sof,3);
            break;
        case 4:
            memcpy(dstp+dof,p+sof,4);
            break;
        case 5:
            memcpy(dstp+dof,p+sof,5);
            break;
        case 6:
            memcpy(dstp+dof,p+sof,6);
            break;
        case 7:
            memcpy(dstp+dof,p+sof,7);
            break;
        case 8:
            memcpy(dstp+dof,p+sof,8);
            break;
        default:
            memcpy(dstp+dof,p+sof,len);
            break;
        }
        ofmp+=3;
    }
    return;
}

void _stitch_remap_nearest_table_2channel_rect(cv::Mat &img,cv::Mat &dst,cv::Mat &table,int s, int e)
{
    uchar *p = img.ptr<uchar>(0,0);
    int *ofmp = table.ptr<int>(s,0);
    uchar *dstp = dst.ptr<uchar>(0,0);
    int rows = e-s;
    int n = rows * 1;
    int i = 0;
    for(; i < n; ++i){
        int sof = ofmp[0];
        int len = ofmp[1];
        int dof = ofmp[2];
        switch(len){
        case 2:
            memcpy(dstp+dof,p+sof,2);
            break;
        case 4:
            memcpy(dstp+dof,p+sof,4);
            break;
        case 6:
            memcpy(dstp+dof,p+sof,6);
            break;
        case 8:
            memcpy(dstp+dof,p+sof,8);
            break;
        case 10:
            memcpy(dstp+dof,p+sof,10);
            break;
        case 12:
            memcpy(dstp+dof,p+sof,12);
            break;
        case 14:
            memcpy(dstp+dof,p+sof,14);
            break;
        case 16:
            memcpy(dstp+dof,p+sof,16);
            break;
        case 18:
            memcpy(dstp+dof,p+sof,18);
            break;
        default:
            memcpy(dstp+dof,p+sof,len);
            break;
        }
        ofmp+=3;
    }
    return;
}

void cromper_table_remape_parallel::operator()(const cv::Range& range) const
{
    if(dst->channels() == 1){
        _stitch_remap_nearest_table_1channel_rect(*img,*dst,*table,range.start,range.end);
    }else if(dst->channels() == 2){
        _stitch_remap_nearest_table_2channel_rect(*img,*dst,*table,range.start,range.end);
    }
    return;
}

#endif
#endif
cv::Mat _corpper_crop_image_remap(qtk_stitch_cropper_t *warpe,qtk_stitch_image_t *image,
                                cv::UMat &uxmap,cv::UMat &uymap,cv::Mat &dst)
{
    // cv::Mat dst;
    cv::Mat *img = NULL;

    img = (cv::Mat*)image->final_image_data;

    // wtk_debug("%d %d %d %d\n",uxmap.rows,uxmap.cols,dst_roi.height,dst_roi.width);
    // wtk_debug("----> %lf\n",time_get_ms());
    // wtk_debug("uxmap.rows %d uxmap.col %d uymap.col %d uymap.row %d\n",uxmap.rows,uxmap.cols,uymap.rows,uymap.cols);
    // wtk_debug("%d %d %d\n",img->isContinuous(),uxmap.isContinuous(),uymap.isContinuous());
    // wtk_debug("%d %d %d\n",img->type(),uxmap.type(),uymap.type());

    cv::remap(*img, dst, uxmap, uymap, warpe->remap_type,cv::BORDER_REFLECT);

    // for(int i = 0; i < dst.rows; i++){
    //     for(int j = 0; j < dst.cols; j++){
    //         printf("%d\n",dst.ptr<uchar>(i,j)[0]);
    //         printf("%d\n",dst.ptr<uchar>(i,j)[1]);
    //         printf("%d\n",dst.ptr<uchar>(i,j)[2]);
    //     }
    // }
    // wtk_debug("%d %d %d \n",it->rows,it->cols,it->channels());
    return dst;
}

cv::Mat _corpper_crop_image_remap2(qtk_stitch_cropper_t *warpe,qtk_stitch_image_t *image,
                                cv::UMat *uxmap,cv::UMat *uymap, cv::Mat *offset_map,cv::Mat *dst)
{
    // cv::Mat dst;
    cv::Mat *img = NULL;

    img = (cv::Mat*)image->final_image_data;

    // wtk_debug("%d %d %d %d\n",uxmap.rows,uxmap.cols,dst_roi.height,dst_roi.width);
    // wtk_debug("----> %lf\n",time_get_ms());
    // wtk_debug("uxmap.rows %d uxmap.col %d uymap.col %d uymap.row %d\n",uxmap.rows,uxmap.cols,uymap.rows,uymap.cols);
    // wtk_debug("%d %d %d\n",img->isContinuous(),uxmap.isContinuous(),uymap.isContinuous());
    // wtk_debug("%d %d %d\n",img->type(),uxmap.type(),uymap.type());
    if(warpe->remap_type == cv::INTER_NEAREST){
#ifdef QTK_THREAD
        _stitch_remap_nearest(*img, *dst, *offset_map);
#else
        class cromper_remape_parallel cp = cromper_remape_parallel(cv::INTER_NEAREST,img,dst,offset_map);
        cv::parallel_for_(cv::Range(0,dst->rows),cp);
#endif
    }
    else if(warpe->remap_type == cv::INTER_LINEAR){
#ifdef QTK_THREAD
        // cv::remap(*img, dst, uxmap, uymap, warpe->remap_type,cv::BORDER_REFLECT);
        _stitch_remap_linear(*img, dst, offset_map,uymap);
#else
        cv::Mat uymap_(uymap->getMat(cv::ACCESS_RW));
        class cromper_remape_parallel cp = cromper_remape_parallel(cv::INTER_LINEAR,img,dst,offset_map,&uymap_);
        cv::parallel_for_(cv::Range(0,dst->rows),cp);
#endif
    }
    else{
        cv::remap(*img, *dst, *uxmap, *uymap, warpe->remap_type,cv::BORDER_REFLECT);
    } 
    // for(int i = 0; i < dst.rows; i++){
    //     for(int j = 0; j < dst.cols; j++){
    //         printf("%d\n",dst.ptr<uchar>(i,j)[0]);
    //         printf("%d\n",dst.ptr<uchar>(i,j)[1]);
    //         printf("%d\n",dst.ptr<uchar>(i,j)[2]);
    //     }
    // }
    // wtk_debug("%d %d %d \n",it->rows,it->cols,it->channels());
    return (*dst);
}

//y分量 1channel
cv::Mat _corpper_crop_image_nv12_remap2(qtk_stitch_cropper_t *warpe,qtk_stitch_image_t *image,
                            cv::UMat *uymap,cv::Mat *offset_map,cv::Mat *uv_offset_map,cv::Mat *dst)
{
    // cv::Mat dst;
    cv::Mat *img = NULL;

    img = (cv::Mat*)image->final_image_data;

    if(warpe->remap_type == cv::INTER_NEAREST){
#ifdef QTK_THREAD
        _stitch_remap_nearest_1channel(*img, dst, offset_map);
#else
        class cromper_remape_parallel cp = cromper_remape_parallel(cv::INTER_NEAREST,img,dst,offset_map);
        cv::parallel_for_(cv::Range(0,offset_map->rows),cp);
#endif
    }else if(warpe->remap_type == cv::INTER_LINEAR){
#ifdef QTK_THREAD
        _stitch_remap_linear_1channel(*img, dst, offset_map,uymap);
#else
        cv::Mat uymap_(uymap->getMat(cv::ACCESS_RW));
        class cromper_remape_parallel cp = cromper_remape_parallel(cv::INTER_LINEAR,img,dst,offset_map,&uymap_);
        cv::parallel_for_(cv::Range(0,offset_map->rows),cp);
#endif
    }else{
        printf("error remape type in yuv\n");
        exit(1);
    }
    //重新组装一个uv
    cv::Mat dp(dst->rows/2,dst->cols/2,CV_8UC2,dst->ptr(offset_map->rows,0));
#ifdef QTK_THREAD
    _stitch_remap_nearest_2channel(*img, dp, *uv_offset_map);
#else
    class cromper_remape_parallel cp = cromper_remape_parallel(cv::INTER_NEAREST,img,&dp,uv_offset_map);
    cv::parallel_for_(cv::Range(0,uv_offset_map->rows),cp);
#endif

    return (*dst);
}

//使用tab的方法 只有邻近插值能用
void _corpper_crop_image_nv12_table_remap2(qtk_stitch_cropper_t *warpe,qtk_stitch_image_t *image,
                wtk_strbuf_t *_table, wtk_strbuf_t *_uv_table, cv::Mat *dst)
{
    int row = dst->rows/3;
    cv::Mat *img = NULL;
    img = (cv::Mat*)image->final_image_data;
    cv::Mat table = cv::Mat(cv::Size(1,_table->pos/(3*sizeof(int))), CV_32SC3, _table->data);
    cv::Mat uv_table = cv::Mat(cv::Size(1,_uv_table->pos/(3*sizeof(int))), CV_32SC3, _uv_table->data);
#ifdef QTK_THREAD
    _stitch_remap_nearest_table_1channel(*img,*dst,table);
#else
    class cromper_table_remape_parallel cp = cromper_table_remape_parallel(cv::INTER_NEAREST,img,dst,&table);
    cv::parallel_for_(cv::Range(0,table.rows),cp);
#endif
    //重新组装一个uv
    cv::Mat dp(dst->rows/2,dst->cols/2,CV_8UC2,dst->ptr(row*2,0));
#ifdef QTK_THREAD
    _stitch_remap_nearest_table_2channel(*img,dp,uv_table);
#else
    cp = cromper_table_remape_parallel(cv::INTER_NEAREST,img,&dp,&uv_table);
    cv::parallel_for_(cv::Range(0,uv_table.rows),cp);
#endif
    return;
}


//只用作注册使用
void qtk_stitch_corpper_crop_remaps(qtk_stitch_cropper_t* cropper,wtk_queue_t* queue)
{
    int n = queue->length;
    wtk_queue_node_t *node = NULL;
    qtk_stitch_queue_data_t *image = NULL;
    // cv::Mat warp_image[10];
    cv::Mat *images = (cv::Mat*)cropper->imgs;
    // std::vector<cv::Mat> *images_ptr = NULL;

    std::vector<cv::UMat> *uxmap = (std::vector<cv::UMat>*)cropper->crop_uxmaps;
    std::vector<cv::UMat> *uymap = (std::vector<cv::UMat>*)cropper->crop_uymaps;
#ifdef USE_OPENMP
    #pragma omp parallel for num_threads(n)
#endif
    for(int i = 0; i < n; ++i){
        node = wtk_queue_peek(queue,i);
        image = data_offset2(node,qtk_stitch_queue_data_t,node);
        // double tt = time_get_ms();
        _corpper_crop_image_remap(cropper,(qtk_stitch_image_t*)image->img_data,
                                uxmap->at(i),uymap->at(i),images[i]);
        // wtk_debug("%lf\n",time_get_ms()-tt);
    }
    return;
}

void qtk_stitch_corpper_crop_remaps2(qtk_stitch_cropper_t* cropper,wtk_queue_t* queue)
{
    int n = queue->length;
    wtk_queue_node_t *node = NULL;
    qtk_stitch_queue_data_t *image = NULL;
    // cv::Mat warp_image[10];
    cv::Mat *images = (cv::Mat*)cropper->imgs;
    // std::vector<cv::Mat> *images_ptr = NULL;

    std::vector<cv::UMat> *uxmap = (std::vector<cv::UMat>*)cropper->crop_uxmaps;
    std::vector<cv::UMat> *uymap = (std::vector<cv::UMat>*)cropper->crop_uymaps;
    cv::Mat *offset_maps = (cv::Mat*)cropper->crop_offset_maps;
    cv::Mat *uv_offset_maps = (cv::Mat*)cropper->crop_uv_offset_maps;
    // cv::Mat *concert_maps1 = (cv::Mat*)cropper->concert_maps1;
    cv::Mat *concert_maps2 = (cv::Mat*)cropper->concert_maps2;
#ifdef USE_OPENMP
    #pragma omp parallel for num_threads(n)
#endif
    for(int i = 0; i < n; ++i){
        node = wtk_queue_peek(queue,i);
        image = data_offset2(node,qtk_stitch_queue_data_t,node);
        if(cropper->img_type == QTK_STITCH_USE_IMG_RGB){
            if(cropper->remap_type == cv::INTER_NEAREST){
                _corpper_crop_image_remap2(cropper,(qtk_stitch_image_t*)image->img_data,
                                        NULL,NULL,offset_maps+i,images+i);
            }else if(cropper->remap_type == cv::INTER_LINEAR){
                cv::UMat map2 = concert_maps2[i].getUMat(cv::ACCESS_RW);
                _corpper_crop_image_remap2(cropper,(qtk_stitch_image_t*)image->img_data,
                                        NULL,&map2,offset_maps+i,images+i);
            }else{
                _corpper_crop_image_remap2(cropper,(qtk_stitch_image_t*)image->img_data,
                            &uxmap->at(i),&uymap->at(i),offset_maps+i,images+i);
            }
            // char path[128] = {0};
            // sprintf(path,"crop_%d*%d_%d.png",images[i].cols,images[i].rows,i);
            // qtk_stitch_image_save_data(path,images[i].ptr(),images[i].cols,images[i].rows,3);
        }else if(cropper->img_type == QTK_STITCH_USE_IMG_NV12){
            if(cropper->remap_type == cv::INTER_NEAREST){
                _corpper_crop_image_nv12_remap2(cropper,(qtk_stitch_image_t*)image->img_data,
                                                NULL,offset_maps+i,uv_offset_maps+i,images+i);
            }else if(cropper->remap_type == cv::INTER_LINEAR){
                cv::UMat map2 = concert_maps2[i].getUMat(cv::ACCESS_RW);
                _corpper_crop_image_nv12_remap2(cropper,(qtk_stitch_image_t*)image->img_data,
                                        &map2,offset_maps+i,uv_offset_maps+i,images+i);
            }
            // char path[128] = {0};
            // sprintf(path,"crop_%d*%d_%d.png",images[i].cols,images[i].rows,i);
            // qtk_stitch_image_save_data(path,images[i].ptr(),images[i].cols,images[i].rows,1);
        }
    }
    return;
}
//一大张图片的
void qtk_stitch_corpper_crop_remaps2_all(qtk_stitch_cropper_t* cropper,wtk_queue_t* queue)
{
    int n = cropper->ncamera;
    wtk_queue_node_t *node = NULL;
    qtk_stitch_queue_data_t *image = NULL;
    // cv::Mat warp_image[10];
    cv::Mat *images = (cv::Mat*)cropper->imgs;
    // std::vector<cv::Mat> *images_ptr = NULL;

    std::vector<cv::UMat> *uxmap = (std::vector<cv::UMat>*)cropper->crop_uxmaps;
    std::vector<cv::UMat> *uymap = (std::vector<cv::UMat>*)cropper->crop_uymaps;
    cv::Mat *offset_maps = (cv::Mat*)cropper->crop_offset_maps;
    cv::Mat *uv_offset_maps = (cv::Mat*)cropper->crop_uv_offset_maps;
    // cv::Mat *concert_maps1 = (cv::Mat*)cropper->concert_maps1;
    cv::Mat *concert_maps2 = (cv::Mat*)cropper->concert_maps2;

    node = wtk_queue_peek(queue,0);
    image = data_offset2(node,qtk_stitch_queue_data_t,node);
    for(int i = 0; i < n; ++i){
        // double tt = time_get_ms();
        if(cropper->img_type == QTK_STITCH_USE_IMG_RGB){
            if(cropper->remap_type == cv::INTER_NEAREST){
                _corpper_crop_image_remap2(cropper,(qtk_stitch_image_t*)image->img_data,
                                        NULL,NULL,offset_maps+i,images+i);
            }else if(cropper->remap_type == cv::INTER_LINEAR){
                cv::UMat map2 = concert_maps2[i].getUMat(cv::ACCESS_RW);
                _corpper_crop_image_remap2(cropper,(qtk_stitch_image_t*)image->img_data,
                                        NULL,&map2,offset_maps+i,images+i);
            }else{
                _corpper_crop_image_remap2(cropper,(qtk_stitch_image_t*)image->img_data,
                            &uxmap->at(i),&uymap->at(i),offset_maps+i,images+i);
            }
            // char path[128] = {0};
            // sprintf(path,"crop_%dx%d_%d.png",images[i].cols,images[i].rows,i);
            // qtk_stitch_image_save_data(path,images[i].data,images[i].cols,images[i].rows,3);
        }else if(cropper->img_type == QTK_STITCH_USE_IMG_NV12){
            if(cropper->remap_type == cv::INTER_NEAREST){
                // _corpper_crop_image_nv12_remap2(cropper,(qtk_stitch_image_t*)image->img_data,
                //                         NULL,offset_maps+i,uv_offset_maps+i,images+i);
                _corpper_crop_image_nv12_table_remap2(cropper,(qtk_stitch_image_t*)image->img_data,
                                    cropper->crop_offset_table[i],cropper->crop_offset_uv_table[i],images+i);
            }else if(cropper->remap_type == cv::INTER_LINEAR){
                cv::UMat map2 = concert_maps2[i].getUMat(cv::ACCESS_RW);
                _corpper_crop_image_nv12_remap2(cropper,(qtk_stitch_image_t*)image->img_data,
                                        &map2,offset_maps+i,uv_offset_maps+i,images+i);
            }
            // char path[128] = {0};
            // sprintf(path,"crop_%d*%d_%d.png",images[i].cols,images[i].rows,i);
            // qtk_stitch_image_save_data(path,images[i].data,images[i].cols,images[i].rows,1);
        }
        // wtk_debug("%lf\n",time_get_ms()-tt);
    }
    // exit(1);
    return;
}

void qtk_stitch_corpper_crop_remap(qtk_stitch_cropper_t* cropper,wtk_queue_t* queue, int idx)
{
    wtk_queue_node_t *node = NULL;
    qtk_stitch_queue_data_t *image = NULL;
    // cv::Mat warp_image[10];
    cv::Mat *images = (cv::Mat*)cropper->imgs;
    // std::vector<cv::Mat> *images_ptr = NULL;

    std::vector<cv::UMat> *uxmap = (std::vector<cv::UMat>*)cropper->crop_uxmaps;
    std::vector<cv::UMat> *uymap = (std::vector<cv::UMat>*)cropper->crop_uymaps;

    node = wtk_queue_peek(queue,idx);
    image = data_offset2(node,qtk_stitch_queue_data_t,node);
    _corpper_crop_image_remap(cropper,(qtk_stitch_image_t*)image->img_data,
                            uxmap->at(idx),uymap->at(idx),images[idx]);

    return;
}

void qtk_stitch_corpper_crop_img(qtk_stitch_cropper_t* cropper,void *imgs, float aspect,int index)
{
    cv::Mat *warper_imgs = (cv::Mat*)imgs;

    _crop_image(cropper,warper_imgs+index,aspect,((cv::Mat*)cropper->imgs)+index,index);

    return;
}

////lir
cv::Mat qtk_stitch_lir(cv::Mat &mask, std::vector<cv::Point> &contour)
{
    cv::Mat lir = largest_interior_rectangle(mask,contour);
    return lir;
}

cv::Mat largest_interior_rectangle(cv::Mat &grid, std::vector<cv::Point> &contour)
{
    std::vector<cv::Mat> adjacencies = adjacencies_all_directions(grid);
    std::vector<cv::Mat> maps = create_maps(adjacencies,contour);
    cv::Mat s_map = maps[0];
    cv::Mat saddle_candidates_map = maps[2];
    std::vector<int> lir1 = biggest_span_in_span_map(s_map);
    cv::Mat s_map2 = the_span_map(saddle_candidates_map,adjacencies[0],adjacencies[2]);
    std::vector<int> lir2 = biggest_span_in_span_map(s_map2);
    cv::Mat lir = biggest_rectangle(lir1,lir2);
    // std::cout << "lir1: " << lir << std::endl;
    // exit(1);
    return lir;
}

std::vector<cv::Mat> adjacencies_all_directions(cv::Mat &grid)
{
    cv::Mat h_left2right = horizontal_adjacency_left2right(grid);
    cv::Mat h_right2left = horizontal_adjacency_right2left(grid);
    cv::Mat v_top2bottom = vertical_adjacency_top2bottom(grid);
    cv::Mat v_bottom2top = vertical_adjacency_bottom2top(grid);
    // int *p = (int*)v_bottom2top.ptr();
    // for(int i = 0; i < v_bottom2top.rows; i++){
    //     for(int j = 0; j < v_bottom2top.cols; j++){
    //         printf("%d\n",p[i*v_bottom2top.cols+j]);
    //     }
    // }
    // exit(1);
    std::vector<cv::Mat> adjacencies;
    adjacencies.push_back(h_left2right);
    adjacencies.push_back(h_right2left);
    adjacencies.push_back(v_top2bottom);
    adjacencies.push_back(v_bottom2top);
    return adjacencies;
}

cv::Mat horizontal_adjacency_left2right(cv::Mat &grid)
{
    cv::Mat result = cv::Mat::zeros(grid.rows,grid.cols,CV_32S);
    int y = 0;
    int n = grid.rows;
    int x = 0;
    // printf("%d\n",grid.at<u_char>(0,439));
    // exit(1);
    for(y = 0; y < n; ++y){
        int span = 0;
        for(x = grid.cols - 1; x > -1; --x){
            if(grid.at<u_char>(y,x) > 0){
                span += 1;
            }else{
                span = 0;
            }
            result.at<int>(y,x) = span;
        }
    }
    return result;
}

cv::Mat horizontal_adjacency_right2left(cv::Mat &grid)
{
    cv::Mat result = cv::Mat::zeros(grid.rows,grid.cols,CV_32S);
    int y = 0;
    int n = grid.rows;
    int x = 0;
    for(y = 0; y < n; ++y){
        int span = 0;
        for(x = 0; x < grid.cols; ++x){
            if(grid.at<u_char>(y,x)){
                span += 1;
            }else{
                span = 0;
            }
            result.at<int>(y,x) = span;
        }
    }
    return result;
}

cv::Mat vertical_adjacency_top2bottom(cv::Mat &grid)
{
    cv::Mat result = cv::Mat::zeros(grid.rows,grid.cols,CV_32S);
    
    int y = 0;
    int n = grid.cols;
    int x = 0;
    for(x = 0; x < n; ++x){
        int span = 0;
        for(y = grid.rows - 1; y > -1; --y){
            if(grid.at<u_char>(y,x)){
                span += 1;
            }else{
                span = 0;
            }
            result.at<int>(y,x) = span;
        }
    }
    return result;
}

cv::Mat vertical_adjacency_bottom2top(cv::Mat &grid)
{
    cv::Mat result = cv::Mat::zeros(grid.rows,grid.cols,CV_32S);
    int y = 0;
    int n = grid.cols;
    int x = 0;
    for(x = 0; x < n; ++x){
        int span = 0;
        for(y = 0; y < grid.rows; ++y){
            if(grid.at<u_char>(y,x)){
                span += 1;
            }else{
                span = 0;
            }
            result.at<int>(y,x) = span;
        }
    }
    return result;
}


std::vector<cv::Mat> create_maps(std::vector<cv::Mat> &adjacencies, std::vector<cv::Point> &contour)
{
    cv::Mat h_left2right(adjacencies[0]);
    cv::Mat h_right2left(adjacencies[1]);
    cv::Mat v_top2bottom(adjacencies[2]);
    cv::Mat v_bottom2top(adjacencies[3]);

    cv::Mat span_map = cv::Mat::zeros(h_left2right.rows,h_left2right.cols,CV_32SC2);
    cv::Mat direction_map = cv::Mat::zeros(h_left2right.rows,h_left2right.cols,CV_8U);
    cv::Mat saddle_candidates_map = cv::Mat::zeros(h_left2right.rows,h_left2right.cols,CV_8U);
    std::vector<cv::Mat> h_vectors;
    std::vector<cv::Mat> v_vectors;
    std::vector<cv::Mat> span_arrays;

    int contour_len = contour.size();
    for(int idx = 0; idx < contour_len; ++idx){
        int x = contour[idx].x;
        int y = contour[idx].y;
        h_vectors = h_vectors_all_directions(h_left2right,h_right2left, x, y);
        v_vectors = v_vectors_all_directions(v_top2bottom,v_bottom2top, x, y);
        // wtk_debug("%d %d\n",x,y);
        span_arrays = spans_all_directions(h_vectors,v_vectors);
        int n = get_n_directions(span_arrays);
        direction_map.at<uchar>(y,x) = n;
        std::vector<cv::Mat> xy_arrays = get_xy_arrays(x,y,span_arrays);
        // for(std::vector<cv::Mat>::iterator it = xy_arrays.begin(); it != xy_arrays.end(); ++it){
        //     for(int i = 0; i < (*it).rows; ++i){
        //         for(int j = 0; j < (*it).cols; ++j){
        //             printf("%d %d %d\n",it->at<int>(i,j),x,y);
        //         }
        //     }
        // }
        for(int direction_idx = 0; direction_idx < 4; ++direction_idx){
            cv::Mat xy_array = xy_arrays[direction_idx];
            cv::Mat span_array = span_arrays[direction_idx];
            for(int span_idx = 0; span_idx < span_array.rows; ++span_idx){
                int x2 = xy_array.at<int>(span_idx,0);
                int y2 = xy_array.at<int>(span_idx,1);
                int w = span_array.at<int>(span_idx,0);
                int h = span_array.at<int>(span_idx,1);

                if((w * h) > (span_map.ptr<int>(y2,x2)[0] * span_map.ptr<int>(y2,x2)[1])){
                    span_map.ptr<int>(y2,x2)[0] = w;
                    span_map.ptr<int>(y2,x2)[1] = h;
                }
                int both_true = cell_on_contour(x2,y2,contour);
                if(n == 3 && !both_true){
                    saddle_candidates_map.at<uchar>(y2,x2) = 1;
                }
            }
        }
    }
    std::vector<cv::Mat> result;
    result.push_back(span_map);
    result.push_back(direction_map);
    result.push_back(saddle_candidates_map);
    return result;
}

cv::Mat h_vector(cv::Mat &h_adjacency, int x, int y)
{
    cv::Mat h_adjacency_sub = h_adjacency(cv::Range(y,h_adjacency.rows),cv::Range(x,x+1));
    // wtk_debug("%d %d %d %d\n",h_adjacency.step[0],h_adjacency.step[1],h_adjacency_sub.step[0],h_adjacency_sub.step[1]);
    // std::cout << h_adjacency_sub << std::endl;
    // exit(1);
    int vector_size = predict_vector_size(h_adjacency_sub);
    cv::Mat h_vector = cv::Mat::zeros(vector_size,1,CV_32S);

    int h = INT_MAX;
    for(int i = 0; i < vector_size; ++i){
        h = MIN(h_adjacency.at<int>(y+i,x),h);
        h_vector.at<int>(i,0) = h;
    }
    h_vector = unique(h_vector);
    return h_vector;
}

cv::Mat v_vector(cv::Mat &v_adjacency, int x, int y)
{
    cv::Mat v_adjacency_sub = v_adjacency(cv::Range(y,y+1),cv::Range(x,v_adjacency.cols));
    // wtk_debug("%d %d %d %d\n",h_adjacency.step[0],h_adjacency.step[1],h_adjacency_sub.step[0],h_adjacency_sub.step[1]);
    // std::cout << v_adjacency_sub << std::endl;
    // exit(1);
    int vector_size = predict_vector_size(v_adjacency_sub);
    cv::Mat v_vector = cv::Mat::zeros(vector_size,1,CV_32S);

    int v = INT_MAX;
    for(int i = 0; i < vector_size; ++i){
        v = MIN(v_adjacency.at<int>(y,x+i),v);
        v_vector.at<int>(i,0) = v;
    }
    v_vector = unique(v_vector);
    return v_vector;
}

std::vector<cv::Mat> h_vectors_all_directions(cv::Mat &h_left2right, cv::Mat &h_right2left, int x, int y)
{
    std::vector<cv::Mat> ret;

    cv::Mat h_l2r_t2b = h_vector(h_left2right,x,y);
    cv::Mat h_r2l_t2b = h_vector(h_right2left,x,y);
    cv::Mat h_l2r_b2t = h_vector_bottom2top(h_left2right,x,y);
    cv::Mat h_r2l_b2t = h_vector_bottom2top(h_right2left,x,y);

    ret.push_back(h_l2r_t2b);
    ret.push_back(h_r2l_t2b);
    ret.push_back(h_l2r_b2t);
    ret.push_back(h_r2l_b2t);
    
    return ret;
}

cv::Mat unique(cv::Mat &m)
{
    // wtk_debug("%d %d \n",m.rows,m.cols);
    cv::Mat dst(m);
    cv::sort(m,dst,cv::SORT_ASCENDING);
    std::vector<int> ret;
    int push = 0;
    for(int i = 0; i < dst.rows; i++){
        for(int j = 0; j < dst.cols; j++){
            int c = dst.at<int>(i,j);
            push = 1;
            for(std::vector<int>::iterator it = ret.begin(); it != ret.end(); it++){
                if(*it == c){
                    push = 0;
                    break;
                }
            }
            if(push){
                ret.push_back(c);
            }
        }
    }
    return cv::Mat(ret,true);
}

int predict_vector_size(cv::Mat &array)
{
    if(array.empty()){
        return 0;
    }
    std::vector<int> zero_indices;
    zero_indices.clear();
    // wtk_debug("array size %d %d\n",array.rows,array.cols);
    for(int i=0;i<array.rows;i++){
        for(int j = 0; j < array.cols; j++){
            if(array.at<int>(i,j) == 0){
                zero_indices.push_back(i*array.cols+j);
            }
        }
    }

    if(zero_indices.size()==0){
        return array.rows * array.cols;
    }
    // wtk_debug("--- %d %d %d\n",zero_indices.size(),array.at<int>(0,0),zero_indices[0]);
    return zero_indices[0];
}

cv::Mat h_vector_bottom2top(cv::Mat &h_adjacency, int x, int y)
{
    cv::Mat h_adjacency_sub = h_adjacency(cv::Range(0,y+1),cv::Range(x,x+1));
    cv::Mat h_adjacency_sub_flip;
    cv::flip(h_adjacency_sub,h_adjacency_sub_flip,-1);

    int vector_size = predict_vector_size(h_adjacency_sub_flip);
    cv::Mat h_vector = cv::Mat::zeros(vector_size,1,CV_32S);

    int h = INT_MAX;
    for(int i = 0; i < vector_size; ++i){
        h = MIN(h_adjacency.at<int>(y-i,x),h);
        h_vector.at<int>(i,0) = h;
    }
    h_vector = unique(h_vector); //还要倒置下没实现

    return h_vector;
}

std::vector<cv::Mat> v_vectors_all_directions(cv::Mat &v_top2bottom, cv::Mat &v_bottom2top, int x, int y)
{
    std::vector<cv::Mat> ret;
    cv::Mat v_l2r_t2b = v_vector(v_top2bottom,x,y);
    cv::Mat v_r2l_t2b = v_vector_right2left(v_top2bottom,x,y);
    cv::Mat v_l2r_b2t = v_vector(v_bottom2top,x,y);
    cv::Mat v_r2l_b2t = v_vector_right2left(v_bottom2top,x,y);
    // wtk_debug("%d %d %d %d\n",v_l2r_t2b.at<int>(0,0),v_r2l_t2b.at<int>(0,0),v_l2r_b2t.at<int>(0,0),v_r2l_b2t.at<int>(0,0));
    // exit(1);
    ret.push_back(v_l2r_t2b);
    ret.push_back(v_r2l_t2b);
    ret.push_back(v_l2r_b2t);
    ret.push_back(v_r2l_b2t);
    
    return ret;
}

cv::Mat v_vector_right2left(cv::Mat &v_adjacency, int x, int y)
{
    cv::Mat v_adjacency_sub = v_adjacency(cv::Range(y,y+1),cv::Range(0,x+1));
    cv::Mat v_adjacency_sub_flip;
    cv::flip(v_adjacency_sub,v_adjacency_sub_flip,-1);
    int vector_size = predict_vector_size(v_adjacency_sub_flip);
    cv::Mat v_vector = cv::Mat::zeros(vector_size,1,CV_32S);

    int v = INT_MAX;
    for(int i = 0; i < vector_size; ++i){
        v = MIN(v_adjacency.at<int>(y,x-i),v);
        v_vector.at<int>(i,0) = v;
    }
    // std::cout << "v_vector: " << v_vector << std::endl;
    v_vector = unique(v_vector);
    // exit(0);
    return v_vector;
}

//行堆叠
cv::Mat _spans(cv::Mat mat1,cv::Mat mat2)
{
    cv::Mat M(mat1.rows,mat1.cols+mat2.cols,CV_32S);
    if(mat1.rows != mat2.rows){
        wtk_debug("error _spans row error %d %d\n",mat1.rows,mat2.rows);
        for(int i = 0; i < mat1.rows; ++i){
            for(int j = 0; j < mat1.cols; ++j){
                printf("%d\n",mat1.at<int>(i,j));
            }
        }
        wtk_debug("mat2:\n");
        for(int i = 0; i < mat2.rows; ++i){
            for(int j = 0; j < mat2.cols; ++j){
                printf("%d\n",mat2.at<int>(i,j));
            }
        }
        exit(1);
    }
    for(int i = 0; i < mat1.rows;++i){
        for(int j = 0;j < mat1.cols;j++){
            M.at<int>(i,j) = mat1.at<int>(i,j);
        }
        for(int j = 0;j < mat2.cols;j++){ //行顺序取反
            M.at<int>(i,j+mat1.cols) = mat2.at<int>(mat1.rows-i-1,j);
        }
    }
    return M;
}

std::vector<cv::Mat> spans_all_directions(std::vector<cv::Mat> h_vectors, std::vector<cv::Mat> v_vectors)
{
    std::vector<cv::Mat> result;
    cv::Mat mat;
    mat = _spans(h_vectors[0],v_vectors[0]);
    result.push_back(mat);    
    mat = _spans(h_vectors[1],v_vectors[1]);
    result.push_back(mat);
    mat = _spans(h_vectors[2],v_vectors[2]);
    result.push_back(mat);
    mat = _spans(h_vectors[3],v_vectors[3]);
    result.push_back(mat);
    return result;
}

int _all(cv::Mat &mat,int num)
{
    for(int i = 0; i < mat.rows; ++i){
        for(int j = 0; j < mat.cols; ++j){
            if(mat.at<int>(i,j) != num){
                return 0;
            }
        }
    }
    return 1;
}

int _any(cv::Mat &mat,int num)
{
    for(int i = 0; i < mat.rows; ++i){
        for(int j = 0; j < mat.cols; ++j){
            if(mat.at<int>(i,j) == num){
                return 1;
            }
        }
    }
    return 0;
}

int get_n_directions(std::vector<cv::Mat> &spans_all_directions)
{
    int n_directions = 1;
    int all_x_1 = 0;
    int all_y_1 = 0;
    for(std::vector<cv::Mat>::iterator it = spans_all_directions.begin(); it < spans_all_directions.end();++it){
        cv::Mat m1 = (*it).col(0);
        all_x_1 = _all(m1,1);
        cv::Mat m2 = (*it).col(1);
        all_y_1 = _all(m2,1);
        if(!all_x_1 && !all_y_1){
            n_directions += 1;
        }
    }
    return n_directions;
}

cv::Mat get_xy_array(int x, int y, cv::Mat &spans, int mode)
{
    cv::Mat result = spans.clone();

    result.col(0) = x;
    result.col(1) = y;
    if(mode == 1){
        result.col(0) = result.col(0) - spans.col(0) + 1;
    }
    if(mode == 2){
        result.col(1) = result.col(1) - spans.col(1) + 1;
    }
    if(mode == 3){
        result.col(0) = result.col(0) - spans.col(0) + 1;
        result.col(1) = result.col(1) - spans.col(1) + 1;
    }
    return result;
}

std::vector<cv::Mat> get_xy_arrays(int x, int y, std::vector<cv::Mat> &spans_all_directions)
{
    std::vector<cv::Mat> result;
    cv::Mat xy_l2r_t2b = get_xy_array(x,y,spans_all_directions[0],0);
    cv::Mat xy_r2l_t2b = get_xy_array(x,y,spans_all_directions[1],1);
    cv::Mat xy_l2r_b2t = get_xy_array(x,y,spans_all_directions[2],2);
    cv::Mat xy_r2l_b2t = get_xy_array(x,y,spans_all_directions[3],3);
    // std::cout << xy_r2l_b2t << std::endl;
    // exit(1);

    result.push_back(xy_l2r_t2b);
    result.push_back(xy_r2l_t2b);
    result.push_back(xy_l2r_b2t);
    result.push_back(xy_r2l_b2t);

    return result;
}

int cell_on_contour(int x, int y, std::vector<cv::Point> &contour)
{
    int ret = 0;
    std::vector<int> both_true;
    for(std::vector<cv::Point>::iterator it = contour.begin(); it < contour.end(); ++it){
        if(x == it->x && y == it->y){
            both_true.push_back(1);
        }else{
            both_true.push_back(0);
        }
    }
    cv::Mat both_true_mat(both_true,true);
    ret = _any(both_true_mat,1);
    return ret;
}

std::vector<int> biggest_span_in_span_map(cv::Mat span_map)
{
    cv::Mat areas = cv::Mat::zeros(span_map.rows,span_map.cols,CV_32SC1);
    int max = -1;
    int aa = 0;
    for(int i = 0; i < span_map.rows; ++i){
        for(int j = 0; j < span_map.cols; ++j){
            aa = span_map.ptr<int>(i,j)[0] * span_map.ptr<int>(i,j)[1];
            areas.at<int>(i,j) = aa;
            max = MAX(max,aa);
        }
    }
    std::vector<cv::Point> max_points;
    for(int i = 0; i < areas.rows; ++i){
        for(int j = 0; j < areas.cols; ++j){
            if(areas.at<int>(i,j) == max){
                max_points.push_back(cv::Point(i,j));
            }
        }
    }
    int x = max_points[0].y;
    int y = max_points[0].x;
    int *span = span_map.ptr<int>(y,x);
    std::vector<int> result;
    result.push_back(x);
    result.push_back(y);
    result.push_back(span[0]);
    result.push_back(span[1]);
    // wtk_debug("%d %d %d %d\n",result[0],result[1],result[2],result[3]);
    return result;
}

cv::Mat the_span_map(cv::Mat &grid, cv::Mat &h_adjacency, cv::Mat &v_adjacency)
{
    std::vector<int> y_values;
    std::vector<int> x_values;
    cv::Mat span_map = cv::Mat::zeros(grid.rows,grid.cols, CV_32SC2);
    //nonzero
    for(int i = 0; i < grid.rows; i++){
        for(int j = 0; j < grid.cols; j++){
            if(grid.at<uchar>(i,j) != 0){
                y_values.push_back(i);
                x_values.push_back(j);
            }
        }
    }
    for (size_t idx = 0; idx < x_values.size(); ++idx){
        int x = x_values[idx];
        int y = y_values[idx];
        cv::Mat h_vec = h_vector(h_adjacency,x,y);
        cv::Mat v_vec = v_vector(v_adjacency,x,y);
        cv::Mat s = _spans(h_vec,v_vec);
        // for(int i = 0; i < s.rows; i++){
        //     for(int j = 0; j < s.cols; j++){
        //         printf("%d\n",s.at<int>(i,j));
        //     }
        // }
        cv::Mat bs = biggest_span(s);
        span_map.ptr<int>(y,x)[0] = bs.at<int>(0,0);
        span_map.ptr<int>(y,x)[1] = bs.at<int>(1,0);
    }
    // wtk_debug("%d %d\n",span_map.rows,span_map.cols);
    // for(int i = 0; i < span_map.rows; i++){
    //     for(int j = 0; j < span_map.cols; j++){
    //         printf("%d\n",span_map.ptr<int>(i,j)[0]);
    //         printf("%d\n",span_map.ptr<int>(i,j)[1]);
    //     }
    // }
    // exit(1);
    return span_map;
}

cv::Mat biggest_span(cv::Mat &spans)
{
    int max = -1;
    if(spans.rows == 0 || spans.cols == 0){
        return cv::Mat(cv::Mat_<int>(2,1) << 0,0);
    }
    // wtk_debug("%d %d\n",spans.rows,spans.cols);
    cv::Mat areas(spans.rows,1,CV_32S);
    for(int i = 0; i < areas.rows; ++i){
        int aa = spans.at<int>(i,0) * spans.at<int>(i,1);
        areas.at<int>(i,0) = aa;
        max = MAX(max,areas.at<int>(i,0));
    }
    std::vector<cv::Point> max_points;
    for(int i = 0; i < areas.rows; ++i){
        for(int j = 0; j < areas.cols; ++j){
            if(areas.at<int>(i,j) == max){
                max_points.push_back(cv::Point(i,j));
            }
        }
    }
    int p_row = max_points[0].x+max_points[0].y;
    cv::Mat result = (cv::Mat_<int>(2,1) << spans.at<int>(p_row,0),spans.at<int>(p_row,1));
    // for(int i = 0; i < result.rows; ++i){
    //     for(int j = 0; j < result.cols; ++j){
    //         printf("%d \n",result.at<int>(i,j));
    //     }
    // }
    return result;
}

cv::Mat biggest_rectangle(std::vector<int> rect1, std::vector<int> rect2)
{
    cv::Mat biggest_rect = cv::Mat::zeros(4,1,CV_32SC1);
    if(rect1[2]*rect1[3] > biggest_rect.at<int>(2,0)*biggest_rect.at<int>(3,0)){
        biggest_rect = (cv::Mat_<int>(4,1) << rect1[0],rect1[1],rect1[2],rect1[3]);
    }
    if(rect2[2]*rect2[3] > biggest_rect.at<int>(2,0)*biggest_rect.at<int>(3,0)){
        biggest_rect = (cv::Mat_<int>(4,1) << rect2[0],rect2[1],rect2[2],rect2[3]);
    }
    return biggest_rect;
}

void qtk_stitch_cropper_rectangles(qtk_stitch_cropper_t* cropper)
{
    if(cropper->overlapping_rectangles){
        std::vector<cv::Rect> *tmp = (std::vector<cv::Rect>*)cropper->overlapping_rectangles;
        delete tmp;
        cropper->overlapping_rectangles = NULL;
    }
    cropper->overlapping_rectangles = (void*)new std::vector<cv::Rect>();
    if(cropper->intersection_rectangles){
        std::vector<cv::Rect> *tmp = (std::vector<cv::Rect>*)cropper->intersection_rectangles;
        delete tmp;
        cropper->intersection_rectangles = NULL;
    }
    cropper->intersection_rectangles = (void*)new std::vector<cv::Rect>();

    return;
}

void qtk_stitch_cropper_rectangles_overlapping_push(qtk_stitch_cropper_t* cropper,int x, int y, int width, int height)
{
    std::vector<cv::Rect> *over = (std::vector<cv::Rect>*)cropper->overlapping_rectangles;
    //宽和高改为2的倍数
    // printf("qtk_stitch_cropper_rectangles_overlapping_push %d %d\n",width,height);
    if((width%2)!=0){
        width = width - 1;
    }
    if((height%2)!=0){
        height = height - 1;
    }
    over->push_back(cv::Rect(x,y,width,height));

    return;
}

void qtk_stitch_cropper_rectangles_intersection_push(qtk_stitch_cropper_t* cropper,int x, int y, int width, int height)
{
    std::vector<cv::Rect> *over = (std::vector<cv::Rect>*)cropper->intersection_rectangles;
    // printf("qtk_stitch_cropper_rectangles_intersection_push %d %d\n",width,height);
    //把切块的宽度作成2的倍数
    if((width%2)!=0){
        width = width - 1;
    }
    if((height%2)!=0){
        height = height - 1;
    }
    over->push_back(cv::Rect(x,y,width,height));

    return;
}

static inline void _interpolateLinear( float x, float* coeffs )
{
    coeffs[0] = 1.f - x;
    coeffs[1] = x;
}
static void _initInterTab1D(int method, float* tab, int tabsz)
{
    float scale = 1.f/tabsz;
    if( method == cv::INTER_LINEAR )
    {
        for( int i = 0; i < tabsz; i++, tab += 2 )
            _interpolateLinear( i*scale, tab );
    }else{
        wtk_debug( "Unknown interpolation method" );
    }
    return;
}
#if 0
void _initInterTab2D(void)
{
    static bool inittab = false;
    // float* tab = 0;
    short* itab = 0;
    int ksize = 0;

    // tab = BilinearTab_f[0][0];
    itab = BilinearTab_i[0][0];
    ksize=2;
    if( !inittab )
    {
        float* _tab = (float*)malloc(8*INTER_TAB_SIZE*sizeof(float));
        int i, j, k1, k2;
        _initInterTab1D(cv::INTER_LINEAR, _tab, INTER_TAB_SIZE);
        for( i = 0; i < INTER_TAB_SIZE; i++ )
            // for( j = 0; j < INTER_TAB_SIZE; j++, tab += ksize*ksize, itab += ksize*ksize )
            for( j = 0; j < INTER_TAB_SIZE; j++, itab += ksize*ksize )
            {
                int isum = 0;
                // NNDeltaTab_i[i*INTER_TAB_SIZE+j][0] = j < INTER_TAB_SIZE/2;
                // NNDeltaTab_i[i*INTER_TAB_SIZE+j][1] = i < INTER_TAB_SIZE/2;

                for( k1 = 0; k1 < ksize; k1++ )
                {
                    float vy = _tab[i*ksize + k1];
                    for( k2 = 0; k2 < ksize; k2++ )
                    {
                        float v = vy*_tab[j*ksize + k2];
                        // float v = _tab[j*ksize + k2];
                        // float v = vy;
                        // tab[k1*ksize + k2] = v;
                        isum += itab[k1*ksize + k2] = cv::saturate_cast<short>(v*INTER_REMAP_COEF_SCALE);
                    }
                }

                if( isum != INTER_REMAP_COEF_SCALE )
                {
                    int diff = isum - INTER_REMAP_COEF_SCALE;
                    int ksize2 = ksize/2, Mk1=ksize2, Mk2=ksize2, mk1=ksize2, mk2=ksize2;
                    for( k1 = ksize2; k1 < ksize2+2; k1++ )
                        for( k2 = ksize2; k2 < ksize2+2; k2++ )
                        {
                            if( itab[k1*ksize+k2] < itab[mk1*ksize+mk2] )
                                mk1 = k1, mk2 = k2;
                            else if( itab[k1*ksize+k2] > itab[Mk1*ksize+Mk2] )
                                Mk1 = k1, Mk2 = k2;
                        }
                    if( diff < 0 )
                        itab[Mk1*ksize + Mk2] = (short)(itab[Mk1*ksize + Mk2] - diff);
                    else
                        itab[mk1*ksize + mk2] = (short)(itab[mk1*ksize + mk2] - diff);
                }
            }
        // tab -= INTER_TAB_SIZE2*ksize*ksize;
        itab -= INTER_TAB_SIZE2*ksize*ksize;
        inittab = true;
        free(_tab);
    }
    return;
}
#endif
static void _initInterTab2D2(void)
{
    static bool inittab = false;
    uchar* itab = 0;
    int ksize = 0;

    itab = BilinearTab_i[0][0];
    ksize=2;
    if( !inittab )
    {
        float* _tab = (float*)malloc(8*INTER_TAB_SIZE*sizeof(float));
        int i, j, k1;
        // k2;
        _initInterTab1D(cv::INTER_LINEAR, _tab, INTER_TAB_SIZE);
        for( i = 0; i < INTER_TAB_SIZE; i++ )
            for( j = 0; j < INTER_TAB_SIZE; j++, itab += ksize*ksize )
            {
                // int isum = 0;
                for( k1 = 0; k1 < ksize; k1++ )
                {
                    float vy = _tab[i*ksize + k1];
                    // for( k2 = 0; k2 < ksize; k2++ )
                    // {
                    //     // float v = vy*_tab[j*ksize + k2];
                    //     // float v = _tab[j*ksize + k2];
                    //     float v = vy;
                    //     // tab[k1*ksize + k2] = v;
                    //     isum += itab[k1*ksize + k2] = cv::saturate_cast<short>(v*INTER_REMAP_COEF_SCALE);
                    // }
                    float v = vy;
                    itab[k1] = cv::saturate_cast<uchar>(v*INTER_REMAP_COEF_SCALE);
                }
                // if( isum != INTER_REMAP_COEF_SCALE )
                // {
                //     int diff = isum - INTER_REMAP_COEF_SCALE;
                //     int ksize2 = ksize/2, Mk1=ksize2, Mk2=ksize2, mk1=ksize2, mk2=ksize2;
                //     for( k1 = ksize2; k1 < ksize2+2; k1++ )
                //         for( k2 = ksize2; k2 < ksize2+2; k2++ )
                //         {
                //             if( itab[k1*ksize+k2] < itab[mk1*ksize+mk2] )
                //                 mk1 = k1, mk2 = k2;
                //             else if( itab[k1*ksize+k2] > itab[Mk1*ksize+Mk2] )
                //                 Mk1 = k1, Mk2 = k2;
                //         }
                //     if( diff < 0 )
                //         itab[Mk1*ksize + Mk2] = (short)(itab[Mk1*ksize + Mk2] - diff);
                //     else
                //         itab[mk1*ksize + mk2] = (short)(itab[mk1*ksize + mk2] - diff);
                // }
            }
        itab -= INTER_TAB_SIZE2*ksize*ksize;
        inittab = true;
        free(_tab);
    }
    return;
}

#if 0
void _stitch_remap_linear(cv::Mat &img,cv::Mat &dst,cv::UMat &uxmap,cv::UMat &uymap)
{
    cv::Size ssize = img.size(), dsize = dst.size();
    const float* wtab = (const float*)_initInterTab2D();
    const uchar* S0 = img.ptr<uchar>();
    size_t sstep = img.step/sizeof(S0[0]);
    cv::Mat uxmap_ = uxmap.getMat(cv::ACCESS_READ);
    cv::Mat uymap_ = uymap.getMat(cv::ACCESS_READ);
    short sy = 0,sx = 0;
    unsigned width1 = ssize.width, height1 = ssize.height;
    CV_Assert( !ssize.empty() );

    for(int dy = 0; dy < dsize.height; dy++ )
    {
        uchar* D = dst.ptr<uchar>(dy);
        const short* XY = uxmap_.ptr<short>(dy);
        const ushort* FXY = uymap_.ptr<ushort>(dy);
        for(int dx = 0; dx < dsize.width; dx++ )
        {
            sx = XY[dx*2];
            sy = XY[dx*2+1];
            if(sx < width1-1 && sy < height1-1){
                const float* w = wtab + FXY[dx]*4;
                const uchar* S = S0 + sy*sstep + sx*3;
                float t0 = S[0]*w[0] + S[3]*w[1] + S[sstep]*w[2] + S[sstep+3]*w[3];
                float t1 = S[1]*w[0] + S[4]*w[1] + S[sstep+1]*w[2] + S[sstep+4]*w[3];
                float t2 = S[2]*w[0] + S[5]*w[1] + S[sstep+2]*w[2] + S[sstep+5]*w[3];
                D[0] = uchar(t0); D[1] = uchar(t1); D[2] = uchar(t2);
            }else{
                memset(D,0,3);
            }
            D += 3;
        }
    }
    return;
}
#endif
void _stitch_remap_linear(cv::Mat &img,cv::Mat &dst,cv::Mat &offmap,cv::UMat &uymap)
{
    cv::Size dsize = dst.size();
    uchar* wtab = (uchar*)BilinearTab_i[0][0];
    uchar* S0 = img.ptr<uchar>();
    size_t sstep = img.step/sizeof(S0[0]);
    cv::Mat uymap_ = uymap.getMat(cv::ACCESS_READ);
    int height = dsize.height;
    int width = dsize.width;
    int* XY = offmap.ptr<int>(0,0);
    uchar* D = dst.ptr<uchar>(0,0);
    ushort* FXY = uymap_.ptr<ushort>(0,0);
    uchar* w = NULL;
    uchar* S = NULL;
    int cross_n = width * height;
    size_t sstep1 = sstep+1;
    size_t sstep2 = sstep1+1;
    // size_t sstep3 = sstep2+1;
    // size_t sstep4 = sstep3+1;
    // size_t sstep5 = sstep4+1;

    for(int n = 0; n < cross_n; ++n,++XY,++FXY){
        w = wtab + *FXY;
        S = S0 + *XY;
        // D[0] = (S[0]*w[0] + S[3]*w[1] + S[sstep]*w[2] + S[sstep+3]*w[3])>>INTER_REMAP_COEF_BITS;
        // D[1] = (S[1]*w[0] + S[4]*w[1] + S[sstep+1]*w[2] + S[sstep+4]*w[3])>>INTER_REMAP_COEF_BITS;
        // D[2] = (S[2]*w[0] + S[5]*w[1] + S[sstep+2]*w[2] + S[sstep+5]*w[3])>>INTER_REMAP_COEF_BITS;
        
        // D[0] = (S[0]*w[0] + S[3]*w[1] + S[sstep]*w[2] + S[sstep3]*w[3])>>INTER_REMAP_COEF_BITS;
        // D[1] = (S[1]*w[0] + S[4]*w[1] + S[sstep1]*w[2] + S[sstep4]*w[3])>>INTER_REMAP_COEF_BITS;
        // D[2] = (S[2]*w[0] + S[5]*w[1] + S[sstep2]*w[2] + S[sstep5]*w[3])>>INTER_REMAP_COEF_BITS;

        // D[0] = (S[0]*w[0] + S[3]*w[1])>>INTER_REMAP_COEF_BITS;
        // D[1] = (S[1]*w[0] + S[4]*w[1])>>INTER_REMAP_COEF_BITS;
        // D[2] = (S[2]*w[0] + S[5]*w[1])>>INTER_REMAP_COEF_BITS;

        D[0] = (S[0]*w[0] + S[sstep]*w[1])>>INTER_REMAP_COEF_BITS; //这里比较惊险
        D[1] = (S[1]*w[0] + S[sstep1]*w[1])>>INTER_REMAP_COEF_BITS;
        D[2] = (S[2]*w[0] + S[sstep2]*w[1])>>INTER_REMAP_COEF_BITS;
        D += 3;
    }
    return;
}

void _stitch_remap_linear_1channel(cv::Mat &img,cv::Mat &dst,cv::Mat &offmap,cv::UMat &uymap)
{
    uchar* wtab = (uchar*)BilinearTab_i[0][0];
    uchar* S0 = img.ptr<uchar>();
    size_t sstep = img.step/sizeof(S0[0]);
    cv::Mat uymap_ = uymap.getMat(cv::ACCESS_READ);
    int height = offmap.rows;
    int width = offmap.cols;
    int* XY = offmap.ptr<int>(0,0);
    uchar* D = dst.ptr<uchar>(0,0);
    ushort* FXY = uymap_.ptr<ushort>(0,0);
    uchar* w = NULL;
    uchar* S = NULL;
    int cross_n = width * height;

    for(int n = 0; n < cross_n; ++n,++XY,++FXY){
        w = wtab + *FXY;
        S = S0 + *XY;

        D[0] = (S[0]*w[0] + S[sstep]*w[1])>>INTER_REMAP_COEF_BITS; //这里比较惊险
        ++D;
    }
    return;
}

void* qtk_stitch_cropper_get_image(qtk_stitch_cropper_t* cropper,int *w, int *h, int idx)
{
    cv::Mat *images = (cv::Mat*)cropper->imgs;
    *w = images[idx].cols;
    *h = images[idx].rows;
    return images[idx].ptr();
}

void qtk_stitch_cropper_rectangle_masks(qtk_stitch_cropper_t* cropper,int *offsets)
{
    cv::Mat *masks = (cv::Mat*)cropper->masks;
    int n = cropper->ncamera;
    for(int i = 0; i < n; ++i){
        // char path[256]  = {0,};
        // sprintf(path,"low_mask%d.png",i);
        // qtk_stitch_image_save_data(path,masks[i].data,masks[i].cols,masks[i].rows,1);
        int left_offset = offsets[i*2];
        if(left_offset > 0){
            int left_len = MAX(0,(masks[i].cols)/2-left_offset);
            cv::rectangle(masks[i],cv::Point(0,0),cv::Point(left_len,masks[i].rows-1),0,-1);
        }
        
        int right_offset = offsets[i*2+1];
        if(right_offset > 0){
            int right_len = MIN((masks[i].cols)/2+right_offset,masks[i].cols-1);
            cv::rectangle(masks[i],cv::Point(right_len,0),cv::Point(masks[i].cols-1,masks[i].rows-1),0,-1);
        }
        // sprintf(path,"low_mask%d_new.png",i);
        // qtk_stitch_image_save_data(path,masks[i].data,masks[i].cols,masks[i].rows,1);
    }
    return;
}

//rgb的
void qtk_stitch_cropper_overlap_save(qtk_stitch_cropper_t *cropper, char *path)
{
    cv::Mat *imgs = (cv::Mat*)cropper->imgs;
    cv::Mat *masks = (cv::Mat*)cropper->masks;
    std::vector<cv::Point> *corners = (std::vector<cv::Point>*)cropper->corners;
    int n = corners->size();
    char the_path[128] = {0};

    for(int i = 1; i < n; ++i){
        cv::Point p = corners->at(i);
        cv::Point p_prev = corners->at(i-1);
        int x = p.x-p_prev.x;

        cv::Mat mask_prev = masks[i-1](cv::Rect(x,0,masks[i-1].cols-x,masks[i-1].rows));
        cv::Mat mask = masks[i](cv::Rect(0,0,masks[i-1].cols-x,masks[i].rows));

        cv::Mat mask_and;
        cv::bitwise_and(mask_prev,mask,mask_and);

        //简单求一个树直的面积
        int j1 = 0;
        int j2 = mask_and.cols;
        int jk = 0;
        int rows = 0;
        for(int j = 1; j < mask_and.cols; j++){
            if(mask_and.at<uchar>(rows,j) == 255 && mask_and.at<uchar>(rows,j1) != 255){
                j1 = j;
                jk = j;
            }
            if(mask_and.at<uchar>(rows,jk) == 255 && mask_and.at<uchar>(rows,j) != 255){
                j2 = j;
                jk = j;
            }
        }
        int j3 = 0;
        int j4 = mask_and.cols;
        jk = 0;
        rows = mask_and.rows-1;
        for(int j = 1; j < mask_and.cols; j++){
            if(mask_and.at<uchar>(rows,j) == 255 && mask_and.at<uchar>(rows,j3) != 255){
                j3 = j;
                jk = j;
            }
            if(mask_and.at<uchar>(rows,jk) == 255 && mask_and.at<uchar>(rows,j) != 255){
                j4 = j;
                jk = j;
            }
        }
        int of_x = MAX(j1,j3);
        int of_x1 = mask_and.cols - MIN(j2,j4);
        // printf("%d %d %d %d %d %d\n",j1,j2,j3,j4,of_x,of_x1);
        cv::Mat img_prev = imgs[i-1](cv::Rect(x+of_x,0,imgs[i-1].cols-x-(of_x+of_x1),imgs[i-1].rows)).clone();
        sprintf(the_path,"%s/%d_1.png",path,i);
        qtk_stitch_image_save_data(the_path,img_prev.data,img_prev.cols,img_prev.rows,3);
        cv::Mat img = imgs[i](cv::Rect(of_x,0,imgs[i-1].cols-x-(of_x+of_x1),imgs[i].rows)).clone();
        sprintf(the_path,"%s/%d_2.png",path,i);
        qtk_stitch_image_save_data(the_path,img.data,img.cols,img.rows,3);
    }
    
    return;
}