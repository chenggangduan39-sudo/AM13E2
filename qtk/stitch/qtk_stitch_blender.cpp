#include "qtk_stitch_blender.h"
#include "qtk/stitch/image/qtk_stitch_image.h"
#include "wtk/core/wtk_alloc.h"
#include "qtk_stitch_def.h"
#include "wtk/os/wtk_thread.h"
#include "wtk/os/wtk_blockqueue.h"
#include "wtk/os/wtk_sem.h"
#include "wtk/core/wtk_strbuf.h"
#include "blenders/qtk_blender_feather.h"
#include "blenders/qtk_blender_no.h"
#include "opencv2/core/mat.hpp"
#include "opencv2/stitching/detail/motion_estimators.hpp"
#include "opencv2/stitching/detail/blenders.hpp"
#include "opencv2/core/utility.hpp"
#include <math.h>
#include <iostream>
#include "opencv2/imgproc.hpp"
#ifdef USE_OPENMP
#include "omp.h"
#endif

extern "C"{
    double time_get_ms(void);
}
#ifdef QTK_THREAD
typedef struct _stitch_blender_thread_data{
    cv::Mat *img;
    cv::Mat *mask;
    cv::Point *corner;
    cv::Mat *weight_maps;
    int idx;
} qtk_stitch_blender_thread_data_t;
#else
class blender_parallel: public cv::ParallelLoopBody
{
public:
    blender_parallel(qtk_stitch_blender_t *_blender,cv::Mat *_img,cv::Mat *_mask,cv::Point *_corner,
                        cv::UMat *_weight_map,cv::Mat *_y_offset_map)
    {
        img = _img;
        mask = _mask;
        corner = _corner;
        blender = _blender;
        weight_map = _weight_map;
        y_offset_map = _y_offset_map;
    }
    blender_parallel(qtk_stitch_blender_t *_blender,cv::Mat *_img,cv::Mat *_mask,cv::Point *_corner)
    {
        img = _img;
        mask = _mask;
        corner = _corner;
        blender = _blender;
    }
    ~blender_parallel(){};
    void operator()(const cv::Range& range) const;
private:
    cv::Mat *img;
    cv::Mat *mask;
    cv::Point *corner;
    qtk_stitch_blender_t *blender;
    cv::UMat *weight_map;
    cv::Mat *y_offset_map;
};

class blender_table_parallel: public cv::ParallelLoopBody
{
public:
    blender_table_parallel(qtk_stitch_blender_t *_blender,cv::Mat *_img,cv::UMat *_weight_map, cv::Mat *_table)
    {
        img = _img;
        blender = _blender;
        weight_map = _weight_map;
        table = _table;
    }
    ~blender_table_parallel(){};
    void operator()(const cv::Range& range) const;
private:
    cv::Mat *img;
    cv::Mat *table;
    qtk_stitch_blender_t *blender;
    cv::UMat *weight_map;
};


#endif
typedef struct{
    int num;
    void *userdata;
}_blender_thread_use_t;

struct qtk_stitch_blender{
    int type;
    float blend_strength;
    int use_gpu;
    cv::Ptr<cv::detail::Blender> blender;
    qtk_blender_feather_t *feather;
    qtk_blender_no_t *blender_no;

    cv::Mat panorama; //cv::Mat
    std::vector<cv::UMat> weight_maps;
    std::vector<cv::Mat> y_offset_maps;
    //yuvsp的uv部分
    std::vector<cv::UMat> uv_weight_maps;
    // std::vector<cv::UMat>  uv_masks;
    std::vector<cv::Mat> uv_y_offset_maps;
    float blend_width;
    char *blend_alpha;
    float *blend_x;
    int use_alpha;
    cv::Rect dst_sz;
    float smooth;
    int use_nthread;
    int img_type;
    int use_compensator; //曝光补偿和融合一起作
    wtk_strbuf_t **weight_tabs;
    wtk_strbuf_t **uv_weight_tabs;
#ifdef QTK_THREAD
    int thread_num;
    int thread_run;
    wtk_thread_t *threads;
    wtk_blockqueue_t the_bqs;
    qtk_stitch_blender_thread_data_t thread_data;
    wtk_sem_t the_threads_sem;
#endif
};

int qtk_stitch_blender_run_rect_process(void *data,wtk_thread_t *thread);
#ifdef QTK_THREAD
static qtk_stitch_queue_data_t *_run_queue_data_new(void *data);
static int _run_queue_data_delete(void *data);
void qtk_stitch_blender_run_rect_process_wait(qtk_stitch_blender_t *blender);
void qtk_stitch_blender_run_rect_process_wake(qtk_stitch_blender_t *blender);
static void _pthread_setaffinity(int n);
#endif
void qtk_stitch_blender_create_weigth_table(qtk_stitch_blender_t* blender,std::vector<cv::Mat> &y_offset_maps,
                                    std::vector<cv::UMat> &weight_maps,wtk_strbuf_t **weight_tabs,int step);

qtk_stitch_blender_t* qtk_stitch_blender_new(int type,float blend_stength,int use_gpu, 
                                        int use_alpha, float smooth, int img_type)
{
    // qtk_stitch_blender_t* blender = (qtk_stitch_blender_t*)wtk_malloc(sizeof(*blender));
    qtk_stitch_blender_t *blender = new qtk_stitch_blender_t;
    wtk_debug("blender type %d %f img type %d\n",type,blend_stength, img_type);
    blender->type = type;
    blender->blend_strength = blend_stength;
    blender->use_gpu = 0;
    blender->feather = NULL;
    blender->blend_alpha = NULL;
    blender->use_alpha = use_alpha;
    blender->smooth = smooth;
    blender->blender_no = NULL;
    blender->img_type = img_type;
    blender->use_compensator = 0;
    blender->weight_tabs = wtk_strbufs_new(8); //先设8个
    blender->uv_weight_tabs = wtk_strbufs_new(8);
    return blender;
}

void qtk_stitch_blender_delete(qtk_stitch_blender_t* blender)
{
    if(blender->blender != nullptr){
        cv::Ptr<cv::detail::Blender> pp = (cv::Ptr<cv::detail::Blender>)blender->blender;
        pp.release();
    }
    if(blender->feather != NULL){
        qtk_blender_feather_delete(blender->feather);
        blender->feather = NULL;
        free(blender->blend_alpha);
        free(blender->blend_x);
    }
    if(blender->blender_no){
        qtk_blender_no_delete(blender->blender_no);
        blender->blender_no = NULL;
    }
    if(blender->weight_tabs){
        wtk_strbufs_delete(blender->weight_tabs,8);
    }
    if(blender->uv_weight_tabs){
        wtk_strbufs_delete(blender->uv_weight_tabs,8);
    }
    delete(blender);
    return;
}

void _prepare(qtk_stitch_blender_t* blender,std::vector<cv::Point> &corners, std::vector<cv::Size> &sizes)
{
    cv::Rect dst_sz;
    float blend_width;
    dst_sz = cv::detail::resultRoi(corners,sizes);
    // wtk_debug("%d %d %d %d\n",dst_sz.x,dst_sz.y,dst_sz.width,dst_sz.height);

    blend_width = sqrt(dst_sz.width*dst_sz.height) * blender->blend_strength/100.0f;
    // wtk_debug("blend_width %f\n",blend_width);

    if(blender->blender != nullptr){
        blender->blender.release();
    }
    if(blender->type == QTK_STITCH_BLENDER_NO || blend_width < 1){
        blender->blender = cv::detail::Blender::createDefault(cv::detail::Blender::NO);
    }else if(blender->type == QTK_STITCH_BLENDER_MULTIBAND){
        cv::Ptr<cv::detail::MultiBandBlender> pp = new cv::detail::MultiBandBlender(blender->use_gpu);
        int num = log(blend_width)/log(2.0f) - 1;
        pp->setNumBands(num);
        blender->blender = pp;
    }else if(blender->type == QTK_STITCH_BLENDER_FEATHER){
        cv::Ptr<cv::detail::FeatherBlender> pp = new cv::detail::FeatherBlender();
        float sharpness = 1.0f/blend_width;
        pp->setSharpness(sharpness);
        blender->blender = pp;
    }
    // std::cout << dst_sz << std::endl;
    blender->blender->prepare(dst_sz);
    return;
}

// void save_bin(void *img,int row,int col,int c, int p)
// {
//     char path[128] = {0};
//     static int index = 0;
//     sprintf(path,"%d.bin",index);
//     FILE *fp = fopen(path,"wb");
//     fwrite(img,p*row*col*c,1,fp);
//     fclose(fp);
//     index++;
//     return; 
// }

void _feed(qtk_stitch_blender_t* blender, cv::Mat &img, cv::Mat &mask, cv::Point &corner,int idx)
{
    // save_bin(img.ptr(),img.rows,img.cols,img.channels(),1);
    cv::Mat uM;
    // wtk_debug("%d -> %d\n",img.type(),uM.type());
    // double t = time_get_ms();
    if(blender->feather == NULL && blender->blender_no == NULL){
        img.assignTo(uM,CV_16S);
        blender->blender->feed(uM,mask,corner);
    }else if(blender->blender_no != NULL){
#ifdef QTK_THREAD
        qtk_blender_no_feed2(blender->blender_no,img,mask,corner);
#else
        class blender_parallel bp = blender_parallel(blender,&img,&mask,&corner,&(blender->weight_maps[idx]),&blender->y_offset_maps[idx]);
        cv::parallel_for_(cv::Range(0,img.rows),bp,-1.0);
#endif 
    }else{
        if(blender->use_alpha == 0){
            if(blender->use_nthread == 0){
                qtk_blender_feather_feed2(blender->feather,img,mask,corner,blender->weight_maps[idx],
                                            blender->y_offset_maps[idx]);
            }else{
#ifdef QTK_THREAD
                blender->thread_data.img = &img;
                blender->thread_data.mask = &mask;
                blender->thread_data.corner = &corner;
                blender->thread_data.idx = idx;
                qtk_stitch_blender_run_rect_process_wake(blender);
                int n = img.rows/(blender->thread_num+1);
                qtk_blender_feather_feed2_rect(blender->feather,img,mask,corner,blender->weight_maps[idx],
                                                blender->y_offset_maps[idx],2*n,img.rows);
                qtk_stitch_blender_run_rect_process_wait(blender);
#else
                class blender_parallel bp = blender_parallel(blender,&img,&mask,&corner,
                                                &(blender->weight_maps[idx]),&(blender->y_offset_maps[idx]));
                cv::parallel_for_(cv::Range(0,img.rows),bp,-1.0);
#endif
            }
        }
        // else{
        //     qtk_blender_feather_feed3(blender->feather,img,mask,corner,blender->weight_maps[idx],
        //                                 blender->y_offset_maps[idx],blender->blend_alpha[idx*3],
        //                                 blender->blend_alpha[idx*3+1],blender->blend_alpha[idx*3+2],
        //                             blender->blend_x[idx*3],blender->blend_x[idx*3+1],
        //                             blender->blend_x[idx*3+2]);
        // }
    }
    // wtk_debug("%f\n",time_get_ms()-t);
    return;
}

void _feed_nv12(qtk_stitch_blender_t* blender, cv::Mat &img, cv::Mat &mask, cv::Point &corner,int idx)
{
    cv::Mat uM;
    int w = 0;
    int h = 0;
    // wtk_debug("%d -> %d\n",img.type(),uM.type());
    // double t = time_get_ms();
    if(blender->feather == NULL && blender->blender_no == NULL){
        printf("not use error blend %d\n",blender->type);
    }else if(blender->blender_no != NULL){  //NO
        w = mask.cols;
        h = mask.rows;
        uM = cv::Mat(h/2,w/2,CV_8UC2,img.ptr(h));
        // printf("img %d %d mask %d %d corner %d %d\n",img.rows,img.cols,mask.rows,mask.cols,corner.x,corner.y);
#ifdef QTK_THREAD
        qtk_blender_no_feed2_1channel(blender->blender_no,img,mask,corner,blender->y_offset_maps[idx]);
        qtk_blender_no_feed2_sp_2channel(blender->blender_no,uM,mask,corner,blender->uv_y_offset_maps[idx]);
#else
        class blender_parallel bp = blender_parallel(blender,&img,&mask,&corner,&(blender->weight_maps[idx]),
                                                &(blender->y_offset_maps[idx]));
        cv::parallel_for_(cv::Range(0,h),bp,-1.0);
        bp = blender_parallel(blender,&uM,&mask,&corner,NULL,
                        &(blender->uv_y_offset_maps[idx]));
        cv::parallel_for_(cv::Range(0,h/2),bp,-1.0);
#endif 
    }else{ //feather
        w = blender->weight_maps[idx].cols;
        h = blender->weight_maps[idx].rows;
        uM = cv::Mat(h/2,w/2,CV_8UC2,img.ptr(h));
#ifdef QTK_THREAD
        qtk_blender_feather_feed2_1channal(blender->feather,img,mask,corner,blender->weight_maps[idx],
                                        blender->y_offset_maps[idx]);
        qtk_blender_feather_feed2_sp_2channal(blender->feather,uM,mask,corner,blender->uv_weight_maps[idx],
                                        blender->uv_y_offset_maps[idx]);
#else
        // class blender_parallel bp = blender_parallel(blender,&img,&mask,&corner,&(blender->weight_maps[idx]),
        //                                 &(blender->y_offset_maps[idx]));
        // cv::parallel_for_(cv::Range(0,blender->weight_maps[idx].rows),bp,-1.0);
        // class blender_parallel bp1 = blender_parallel(blender,&uM,&mask,&corner,&(blender->uv_weight_maps[idx]),
        //                 &(blender->uv_y_offset_maps[idx]));
        // cv::parallel_for_(cv::Range(0,blender->uv_weight_maps[idx].rows),bp1,-1.0);
        cv::Mat table = cv::Mat(cv::Size(1,blender->weight_tabs[idx]->pos/(4*sizeof(int))), CV_32SC4, blender->weight_tabs[idx]->data);
        class blender_table_parallel bp = blender_table_parallel(blender,&img,&(blender->weight_maps[idx]),&table);
        cv::parallel_for_(cv::Range(0,table.rows),bp,-1.0);
        cv::Mat uv_table = cv::Mat(cv::Size(1,blender->uv_weight_tabs[idx]->pos/(4*sizeof(int))), CV_32SC4, blender->uv_weight_tabs[idx]->data);
        class blender_table_parallel bp1 = blender_table_parallel(blender,&uM,&(blender->uv_weight_maps[idx]),&uv_table);
        cv::parallel_for_(cv::Range(0,uv_table.rows),bp1,-1.0);
#endif
    }   
    // wtk_debug("%f\n",time_get_ms()-t);
    return;
}

cv::Mat _blend(qtk_stitch_blender_t* blender)
{
    cv::Mat dst;
    cv::Mat dst_mask;
    cv::Mat result;

    cv::detail::Blender* blender_ptr = (cv::detail::Blender*)blender->blender;
    blender_ptr->blend(dst,dst_mask);
    cv::convertScaleAbs(dst,result);
    // wtk_debug("%d %d %d %d %d %d %d\n",dst.rows,dst.cols,dst.channels(),dst.type(),dst_mask.rows,dst_mask.cols,dst_mask.type());
    // for(int i = 0; i < dst.rows;++i){
    //     for(int j = 0; j < dst.cols;++j){
    //         printf("%d\n",dst.ptr<short>(i,j)[0]);
    //         printf("%d\n",dst.ptr<short>(i,j)[1]);
    //         printf("%d\n",dst.ptr<short>(i,j)[2]);
    //     }
    // }
    // exit(1);
    return dst_mask;
}

cv::Mat _blend2(qtk_stitch_blender_t* blender)
{
    cv::Mat dst;
    cv::Mat dst_mask;
    cv::Mat result;

    if(blender->feather == NULL && blender->blender_no == NULL){
        cv::detail::Blender* blender_ptr = (cv::detail::Blender*)blender->blender;
        // double t = time_get_ms();
        blender_ptr->blend(dst,dst_mask);
        cv::convertScaleAbs(dst,result); //这个把数据格式 short -> uchar
    }else if(blender->feather != NULL){
        qtk_blender_feather_blend(blender->feather,result,dst_mask);
    }else{
        qtk_blender_no_blend(blender->blender_no,result,dst_mask);
    }
    // double t = time_get_ms();
    // wtk_debug("ttt %lf\n",time_get_ms()-t);
    // wtk_debug("%d %d %d %d\n",result.rows,result.cols,result.channels(),result.type());
    // for(int i = 0; i < result.rows;++i){
    //     for(int j = 0; j < result.cols;++j){
    //         printf("%d\n",result.ptr<uchar>(i,j)[0]);
    //         printf("%d\n",result.ptr<uchar>(i,j)[1]);
    //         printf("%d\n",result.ptr<uchar>(i,j)[2]);
    //     }
    // }
    // exit(1);
    // wtk_debug("%lf\n",time_get_ms()-t);
    return result;
}

//返回cv::Mat
void* qtk_stitch_blender_create_panorama(void *imgs, void *masks, void *corners, void *sizes)
{
    cv::Mat *warper_imgs = (cv::Mat*)imgs;
    cv::Mat *warper_mask = (cv::Mat*)masks;
    std::vector<cv::Point> *roi_corners = (std::vector<cv::Point>*) corners;
    std::vector<cv::Size> *roi_sizes = (std::vector<cv::Size>*) sizes;
    int i = 0;
    cv::Mat result_mask;

    qtk_stitch_blender_t* blender = qtk_stitch_blender_new(QTK_STITCH_BLENDER_NO,5,0,0,0,0);
    _prepare(blender,*roi_corners,*roi_sizes);
    int n = roi_corners->size();
    for( i = 0; i < n; i++){
        _feed(blender,warper_imgs[i],warper_mask[i],roi_corners->at(i),i);
    }

    result_mask = _blend(blender);
    qtk_stitch_blender_delete(blender);
    return (void*) new cv::Mat(result_mask);
}

void qtk_stitch_blender_prepare(qtk_stitch_blender_t* blend,void *corners, void *sizes)
{
    std::vector<cv::Point> *roi_corners = (std::vector<cv::Point>*) corners;
    std::vector<cv::Size> *roi_sizes = (std::vector<cv::Size>*) sizes;
    _prepare(blend,*roi_corners,*roi_sizes);
    return;
}

//配合start使用
void qtk_stitch_blender_prepare2(qtk_stitch_blender_t* blender,void *corners, void *sizes, void *ptr)
{
    std::vector<cv::Point> *roi_corners = (std::vector<cv::Point>*) corners;
    std::vector<cv::Size> *roi_sizes = (std::vector<cv::Size>*) sizes;
    cv::Rect dst_sz;
    dst_sz = cv::detail::resultRoi(*roi_corners,*roi_sizes);
    if(blender->feather == NULL && blender->blender_no == NULL){
        blender->blender->prepare(dst_sz);
    }else if(blender->feather != NULL){
        if(blender->img_type == QTK_STITCH_USE_IMG_RGB){
            // qtk_blender_feather_prepare(blender->feather,dst_sz);
            qtk_blender_feather_prepare2(blender->feather, dst_sz, ptr);
        }else if(blender->img_type == QTK_STITCH_USE_IMG_NV12){
            qtk_blender_feather_nv12_prepare2(blender->feather, dst_sz, ptr);
        }
    }else{
        if(blender->img_type == QTK_STITCH_USE_IMG_RGB){
            qtk_blender_no_prepare2(blender->blender_no, dst_sz, ptr);
        }else if(blender->img_type == QTK_STITCH_USE_IMG_NV12){
            qtk_blender_no_nv12_prepare2(blender->blender_no, dst_sz, ptr);
        }
    }
    return;
}

//作为拆分 _prepare 的函数
void qtk_stitch_blender_start(qtk_stitch_blender_t* blender,void *seam_masks, void *corners, void *sizes, int use_nthread)
{
    std::vector<cv::Point> *roi_corners = (std::vector<cv::Point>*) corners;
    std::vector<cv::Size> *roi_sizes = (std::vector<cv::Size>*) sizes;
    std::vector<cv::UMat> *masks = (std::vector<cv::UMat>*)seam_masks;
    
    // for(int i = 0; i < roi_sizes->size(); i++){
    //     std::cout << roi_sizes->at(i).width << " " << roi_sizes->at(i).height << std::endl;
    //     std::cout << roi_corners->at(i).x << " " << roi_corners->at(i).y << std::endl;
    // }
    cv::Rect dst_sz;
    dst_sz = cv::detail::resultRoi(*roi_corners,*roi_sizes);
    // wtk_debug("%d %d %d %d\n",dst_sz.x,dst_sz.y,dst_sz.width,dst_sz.height);

    blender->blend_width = sqrt(dst_sz.width*dst_sz.height) * blender->blend_strength/100.0f;
    // wtk_debug("blend_width %f\n",blend_width);
    blender->use_nthread = use_nthread;
    // if(blender->blender != nullptr){
    //     blender->blender.release();
    // }
    if(blender->type == QTK_STITCH_BLENDER_NO || blender->blend_width < 1){
        // blender->blender = cv::detail::Blender::createDefault(cv::detail::Blender::NO);
        blender->blender_no = qtk_blender_no_new();
        // blender->uv_masks.resize(roi_sizes->size());
        blender->weight_maps.resize(roi_sizes->size());
        qtk_blender_no_weightmaps(blender->blender_no,*masks,*roi_corners,blender->weight_maps);
        if(blender->img_type == QTK_STITCH_USE_IMG_RGB){
        #ifdef USE_9391
            qtk_blender_no_yoffsetmaps(blender->blender_no,*masks,dst_sz,*roi_corners,blender->y_offset_maps,4);
        #else
            qtk_blender_no_yoffsetmaps(blender->blender_no,*masks,dst_sz,*roi_corners,blender->y_offset_maps,3);
        #endif
        }else if(blender->img_type == QTK_STITCH_USE_IMG_NV12){
            qtk_blender_no_yoffsetmaps(blender->blender_no,*masks,dst_sz,*roi_corners,blender->y_offset_maps,1);
            qtk_blender_no_uv_yoffsetmaps(blender->blender_no,*masks,dst_sz,*roi_corners,blender->uv_y_offset_maps);
        }
    }else if(blender->type == QTK_STITCH_BLENDER_MULTIBAND){
        cv::Ptr<cv::detail::MultiBandBlender> pp = new cv::detail::MultiBandBlender(blender->use_gpu);
        int num = log(blender->blend_width)/log(2.0f) - 1;
        pp->setNumBands(num);
        blender->blender = pp;
    }else if(blender->type == QTK_STITCH_BLENDER_FEATHER){
        float sharpness = 1.0f/blender->blend_width;
        blender->feather = qtk_blender_feather_new(sharpness,blender->smooth);
        blender->weight_maps.resize(roi_sizes->size());
        qtk_blender_feather_weightmaps(blender->feather,*masks,*roi_corners,blender->weight_maps);
        if(blender->img_type == QTK_STITCH_USE_IMG_RGB){
        #ifdef USE_9391
            qtk_blender_feather_yoffsetmaps(blender->feather,*masks,dst_sz,*roi_corners,blender->y_offset_maps,4);
        #else
            qtk_blender_feather_yoffsetmaps(blender->feather,*masks,dst_sz,*roi_corners,blender->y_offset_maps,3);
        #endif
        }else if(blender->img_type == QTK_STITCH_USE_IMG_NV12){
            qtk_blender_feather_uv_weightmaps(blender->feather,blender->weight_maps,blender->uv_weight_maps);
            qtk_blender_feather_yoffsetmaps(blender->feather,*masks,dst_sz,*roi_corners,blender->y_offset_maps,1);
            qtk_stitch_blender_create_weigth_table(blender,blender->y_offset_maps,blender->weight_maps,blender->weight_tabs,1);
        }
        blender->blend_alpha = (char*)malloc(sizeof(char)*roi_corners->size()*3);
        blender->blend_x = (float*)malloc(sizeof(float)*roi_corners->size()*3);
        if(blender->img_type == QTK_STITCH_USE_IMG_NV12){
            qtk_blender_feather_uv_yoffsetmaps(blender->feather,*masks,dst_sz,*roi_corners,blender->uv_y_offset_maps);
            qtk_stitch_blender_create_weigth_table(blender,blender->uv_y_offset_maps,blender->uv_weight_maps,blender->uv_weight_tabs,2);
        }
    }
    blender->dst_sz = dst_sz;
#ifdef QTK_THREAD
    if(use_nthread != 0){
        blender->thread_num = masks->size()-1;
        blender->threads = (wtk_thread_t*)wtk_malloc((blender->thread_num)*sizeof(wtk_thread_t));
        blender->thread_run = 1;
        wtk_blockqueue_init(&blender->the_bqs);
        wtk_sem_init(&blender->the_threads_sem,0);
        for(int i = 0; i < blender->thread_num; ++i){
            _blender_thread_use_t *userdata = (_blender_thread_use_t*)malloc(sizeof(*userdata));
            wtk_thread_init(blender->threads+i,qtk_stitch_blender_run_rect_process,userdata);
            userdata->userdata = blender;
            userdata->num = i+1;
            wtk_thread_start(blender->threads+i);
        }
    }
#endif
    return;
}

void qtk_stitch_blender_stop(qtk_stitch_blender_t* blender)
{
    if(blender->blender != NULL){
        blender->blender.release();
        blender->blender = NULL;
    }
    if(blender->feather){
        if(blender->blend_alpha){free(blender->blend_alpha);blender->blend_alpha = NULL;}
        if(blender->blend_x){free(blender->blend_x);blender->blend_x = NULL;}
        blender->uv_y_offset_maps.clear();
        blender->y_offset_maps.clear();
        blender->uv_weight_maps.clear();
        blender->weight_maps.clear();
        qtk_blender_feather_delete(blender->feather);
        blender->feather = NULL;
    }
    if(blender->blender_no){
        blender->y_offset_maps.clear();
        blender->uv_y_offset_maps.clear();
        // blender->uv_masks.clear();
        qtk_blender_no_delete(blender->blender_no);
        blender->blender_no = NULL;
    }
#ifdef QTK_THREAD
    if(blender->use_nthread != 0){
        blender->thread_run = 0;
        wtk_sem_init(&blender->the_threads_sem,0);
        for(int i = 0; i < blender->thread_num; ++i){
            wtk_blockqueue_wake(&blender->the_bqs);
        }
        for(int i = 0; i < blender->thread_num; ++i){
            wtk_thread_join(blender->threads+i);
        }
        
        blender->thread_num = 0;
        wtk_free(blender->threads);
    }
#endif
    return;
}

void qtk_stitch_blender_feed(qtk_stitch_blender_t* blend,void *imgs,void *seam_masks,void *corners)
{
    cv::Mat *cropper_imgs = (cv::Mat*)imgs;
    std::vector<cv::UMat> *masks = (std::vector<cv::UMat>*)seam_masks;
    std::vector<cv::Point> *roi_corners = (std::vector<cv::Point>*)corners;
    int n = masks->size();

    // double t = time_get_ms();
    for(int idx = 0;idx < n; ++idx){
        // cv::Mat cm = cropper_imgs->at(idx);
        cv::Mat mask(masks->at(idx).getMat(cv::ACCESS_READ));
        if(blend->img_type == QTK_STITCH_USE_IMG_RGB){
            _feed(blend,cropper_imgs[idx],mask,roi_corners->at(idx),idx);
        }else if(blend->img_type == QTK_STITCH_USE_IMG_NV12){
            _feed_nv12(blend,cropper_imgs[idx],mask,roi_corners->at(idx),idx);
        }
        // printf("corner %d %d mask %d\n",roi_corners->at(idx).x,roi_corners->at(idx).y,mask.cols);
    }
    // wtk_debug("%lf\n",time_get_ms()-t);
    return;
}

void qtk_stitch_blender_feed_index(qtk_stitch_blender_t* blend,
                                void *imgs,void *seam_masks,void *corners, int index)
{
    cv::Mat *cropper_imgs = (cv::Mat*)imgs;
    std::vector<cv::UMat> *masks = (std::vector<cv::UMat>*)seam_masks;
    std::vector<cv::Point> *roi_corners = (std::vector<cv::Point>*)corners;

    cv::Mat mask(masks->at(index).getMat(cv::ACCESS_READ));
    _feed(blend,cropper_imgs[index],mask,roi_corners->at(index),index);
    return;
}

void qtk_stitch_blender_blend(qtk_stitch_blender_t* blender)
{
    // cv::Mat panorama = _blend2(blender);
    // blender->panorama = panorama.clone();
    // cv::cvtColor(panorama,blender->panorama,cv::COLOR_BGR2RGB);
    blender->panorama = _blend2(blender);
    return;
}

//channle 默认是 3
uchar* qtk_stitch_blender_get_panorama(qtk_stitch_blender_t* blender,int *row,int *col)
{
    uchar* panorama = blender->panorama.data;
    *row = blender->panorama.rows;
    *col = blender->panorama.cols;
    return panorama;
}

void qtk_stitch_blender_get_rect(qtk_stitch_blender_t* blender,void *corners, void *sizes,int *rows, int *cols)
{
    std::vector<cv::Point> *roi_corners = (std::vector<cv::Point>*) corners;
    std::vector<cv::Size> *roi_sizes = (std::vector<cv::Size>*) sizes;
    cv::Rect dst_sz;
    // for(int i = 0; i < roi_corners->size(); ++i){
    //     std::cout << roi_corners->at(i) << " " << roi_sizes->at(i) << std::endl;
    // }
    dst_sz = cv::detail::resultRoi(*roi_corners,*roi_sizes);
    *rows = dst_sz.height;
    *cols = dst_sz.width;
    return;
}

void qtk_stitch_blender_set_rgb(qtk_stitch_blender_t* blender,int idx, char alpha,char beta,char gamma,
                                float alpha_x,float beta_x,float gamma_x)
{
    if(blender->type == QTK_STITCH_BLENDER_FEATHER){
        blender->blend_alpha[idx*3] = alpha;
        blender->blend_alpha[idx*3+1] = beta;
        blender->blend_alpha[idx*3+2] = gamma;
        blender->blend_x[idx*3] = alpha_x;
        blender->blend_x[idx*3+1] = beta_x;
        blender->blend_x[idx*3+2] = gamma_x;
    }
    return;
}

void qtk_stitch_blender_update_weight_maps(qtk_stitch_blender_t* blender,void *gain_mat)
{
    std::vector<cv::Mat> *up_mats = (std::vector<cv::Mat>*)gain_mat;
    if(blender->type == QTK_STITCH_BLENDER_FEATHER){
        blender->use_compensator = 1;
        qtk_blender_feather_weight_maps_update(blender->feather,*up_mats,blender->weight_maps);
        qtk_stitch_blender_create_weigth_table(blender,blender->y_offset_maps,blender->weight_maps,blender->weight_tabs,1);
    }else if(blender->type == QTK_STITCH_BLENDER_NO){
        blender->use_compensator = 1;
        qtk_blender_no_weight_maps_update(blender->blender_no,*up_mats,blender->weight_maps);
    }
    return;
}

void qtk_stitch_blender_set_smooth(qtk_stitch_blender_t* blender,float smooth)
{
    if(blender->type == QTK_STITCH_BLENDER_FEATHER){
        qtk_feather_set_smooth(blender->feather,smooth);
    }
    return;
}

// void qtk_stitch_blender_remap_feather(qtk_stitch_blender_t* blender, wtk_queue_t* queue,
//                                 void *seam_masks,void *corners,void *crop_offset_maps)
// {
//     wtk_queue_node_t *node = NULL;
//     qtk_stitch_queue_data_t *image = NULL;
//     std::vector<cv::UMat> *masks = (std::vector<cv::UMat>*)seam_masks;
//     std::vector<cv::Point> *roi_corners = (std::vector<cv::Point>*)corners;
//     int n = masks->size();
//     cv::Mat *offset_maps = (cv::Mat*)crop_offset_maps;


//     for(int idx = 0;idx < n; ++idx){
//         node = wtk_queue_peek(queue,idx);
//         image = data_offset2(node,qtk_stitch_queue_data_t,node);
//         cv::Mat mask(masks->at(idx).getMat(cv::ACCESS_READ));
//         qtk_stitch_image_t *data = (qtk_stitch_image_t *)image->img_data;
//         cv::Mat *src_img = (cv::Mat*)data->final_image_data;
//         qtk_feather_remap_feed(blender->feather,*src_img,masks->at(idx),roi_corners->at(idx),
//                                 blender->weight_maps[idx],blender->y_offset_maps[idx],
//                                 offset_maps[idx]);
//     }
//     // wtk_debug("%f\n",time_get_ms()-t);
//     return;
// }
#ifdef QTK_THREAD
int qtk_stitch_blender_run_rect_process(void *user_data,wtk_thread_t *thread)
{
    wtk_queue_node_t *node = NULL;
    qtk_stitch_queue_data_t *data = NULL;
    _blender_thread_use_t *thread_user_data = (_blender_thread_use_t*)user_data;
    qtk_stitch_blender_t *blender = (qtk_stitch_blender_t*)thread_user_data->userdata;
    int idx = 0;
    int n = 0;
    int spl = 0;

    _pthread_setaffinity(thread_user_data->num);
    while(blender->thread_run){
        node = wtk_blockqueue_pop(&blender->the_bqs,-1,NULL);
        if(node == NULL){
            continue;
        }
        data = data_offset2(node,qtk_stitch_queue_data_t,node);
        idx = blender->thread_data.idx;
        n = blender->thread_data.img->rows;
        n = n/(blender->thread_num+1);
        spl = data->num;
        qtk_blender_feather_feed2_rect(blender->feather,*blender->thread_data.img,*blender->thread_data.mask,*blender->thread_data.corner,
                                blender->weight_maps[idx],blender->y_offset_maps[idx],spl*n,spl*n+n);
        wtk_sem_inc(&blender->the_threads_sem);
        _run_queue_data_delete(data);
    }
    free(thread_user_data);
    return 0;
}

void qtk_stitch_blender_run_rect_process_wait(qtk_stitch_blender_t *blender)
{
    int i = 0;
    for(i = 0; i < blender->thread_num; i++){
        wtk_sem_acquire(&blender->the_threads_sem,-1);
    }
    return;
}

void qtk_stitch_blender_run_rect_process_wake(qtk_stitch_blender_t *blender)
{
    int i = 0;
    qtk_stitch_queue_data_t *data = NULL;
    for(i = 0; i < blender->thread_num; i++){
        data = _run_queue_data_new(blender);
        data->num = i;
        wtk_blockqueue_push(&blender->the_bqs,&data->node);
    }
    return;
}

static qtk_stitch_queue_data_t *_run_queue_data_new(void *data)
{
    qtk_stitch_queue_data_t *qdata = (qtk_stitch_queue_data_t *)wtk_malloc(sizeof(qtk_stitch_queue_data_t));
    memset(qdata,0,sizeof(qtk_stitch_queue_data_t));
    return qdata;
}

static int _run_queue_data_delete(void *data)
{
    qtk_stitch_queue_data_t *qdata = (qtk_stitch_queue_data_t*)data;
    wtk_free(qdata);
    return 0;
}

void qtk_stitch_blender_queue_clean(qtk_stitch_blender_t* blender)
{
    wtk_queue_node_t *node = NULL;
    qtk_stitch_queue_data_t *data = NULL;
    while(1){
        node = wtk_blockqueue_pop(&blender->the_bqs,0,NULL);
        if(node == NULL) break;
        data = data_offset2(node,qtk_stitch_queue_data_t,node);
        _run_queue_data_delete(data);
    }
    return;
}

static void _pthread_setaffinity(int n)
{
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    CPU_SET(n, &cpu_set);
    if(pthread_setaffinity_np(pthread_self(), sizeof(cpu_set), &cpu_set) < 0)
        perror("pthread_setaffinity_np");
    return;
}
#else
void blender_parallel::operator()(const cv::Range& range) const
{
    if(blender->type == QTK_STITCH_BLENDER_FEATHER){
        if(img->channels() == 3){
            qtk_blender_feather_feed2_rect(blender->feather,*img,*mask,*corner,*weight_map,
                                    *y_offset_map,range.start,range.end);
        }else if(img->channels() == 2){
            qtk_blender_feather_feed2_sp_2channal_rect(blender->feather,*img,*mask,*corner,*weight_map,
                                    *y_offset_map,range.start,range.end);
        }else if(img->channels() == 1){
            qtk_blender_feather_feed2_1channal_rect(blender->feather,*img,*mask,*corner,*weight_map,
                                    *y_offset_map,range.start,range.end);
        }
    }else if(blender->type == QTK_STITCH_BLENDER_NO){
        if(img->channels() == 3){
            if(blender->use_compensator == 0){
                qtk_blender_no_feed2_rect(blender->blender_no,*img,*mask,*corner,range.start,range.end);
            }else{
                qtk_blender_no_feed2_exposure_rect(blender->blender_no,*img,*corner,*weight_map,*y_offset_map,range.start,range.end);
            }
        }else if(img->channels() == 2){
            qtk_blender_no_feed2_sp_2channel_rect(blender->blender_no,*img,*mask,*corner,
                                    *y_offset_map,range.start,range.end);
        }else if(img->channels() == 1){
            if(blender->use_compensator == 0){
                qtk_blender_no_feed2_1channel_rect(blender->blender_no,*img,*mask,*corner,*y_offset_map,
                                        range.start,range.end);
            }else{
                qtk_blender_no_feed2_1channel_exposure_rect(blender->blender_no,*img,*corner,*weight_map,*y_offset_map,
                                        range.start,range.end);
            }
        }
    }
    // printf("%d %d %d %d %d %d\n",img->rows,mask->rows,corner->x,weight_map->rows,y_offset_map->cols,range.start);
    
    return;
}

void blender_table_parallel::operator()(const cv::Range& range) const
{
    if(blender->type == QTK_STITCH_BLENDER_FEATHER){
        if(img->channels() == 2){
            qtk_blender_feather_feed2_table_2channal_rect(blender->feather,*img,*weight_map,
                                    *table,range.start,range.end);
        }else if(img->channels() == 1){
            qtk_blender_feather_feed2_table_1channal_rect(blender->feather,*img,*weight_map,
                                    *table,range.start,range.end);
        }
    }
    // printf("%d %d %d %d %d %d\n",img->rows,mask->rows,corner->x,weight_map->rows,y_offset_map->cols,range.start);
    
    return;
}

#endif

void _blender_create_weight_table(cv::Mat &weight,cv::Mat &y_offset, wtk_strbuf_t *buf,int step)
{
    int kkk[4] = {0,};
    int t = -1,tt = 0;
    int tl = 0;
    int ts = 0;
    int td = 0;
    
    wtk_strbuf_reset(buf);
    for(int i = 0; i < weight.rows; ++i){
        if(t == -1){
            if(weight.at<ushort>(i,0) == 1<<QTK_STITCH_FIX_POINT){
                t = 1; //这个权重就是1
            }else if(weight.at<ushort>(i,0) == 0){
                t = 0; //这个权重是0
            }else{
                t = 2;
            }
            tl = 1;
            ts = weight.cols*i;
            td = y_offset.at<int>(0,i);
        }
        for(int j = 1; j < weight.cols; ++j){
            if(weight.at<ushort>(i,j) == 1<<QTK_STITCH_FIX_POINT){
                tt = 1;
            }else if(weight.at<ushort>(i,j) == 0){
                tt = 0;
            }else{
                tt = 2;
            }
            if(t != tt){
                kkk[0] = t; //类型
                kkk[1] = tl; //长度
                kkk[2] = ts; //weight起始位置
                kkk[3] = td; //要放到的位置
                wtk_strbuf_push(buf,(char*)kkk,sizeof(kkk));
                t = tt;
                ts = i*weight.cols+j;
                td = td+tl*step;
                tl = 0;
            }
            ++tl;
        }
        kkk[0] = t; //类型
        kkk[1] = tl; //长度
        kkk[2] = ts; //weight起始位置
        kkk[3] = td; //要放到的位置
        wtk_strbuf_push(buf,(char*)kkk,sizeof(kkk));
        t = -1;
    }
    return;
}

void _table2weight_print(wtk_strbuf_t *buf, cv::Mat &weight)
{
    int *table = (int*)buf->data;
    int n = buf->pos/(sizeof(int)*4);
    for(int i = 0; i < n; ++i){
        int type = table[0];
        int tl = table[1];
        ushort *ws = weight.ptr<ushort>()+table[2];
        if(type == 0){
            for(int j = 0; j < tl; ++j){
                printf("%d\n",0);
            }
        }else if(type == 1){
            for(int j = 0; j < tl; ++j){
                printf("%d\n",1<<QTK_STITCH_FIX_POINT);
            }  
        }else if(type == 2){
            for(int j = 0; j < tl; ++j){
                printf("%d\n",ws[0]);
                ++ws;
            }
        }
        table+=4;
    }
    return;
}

void qtk_stitch_blender_create_weigth_table(qtk_stitch_blender_t* blender,std::vector<cv::Mat> &y_offset_maps,
                                        std::vector<cv::UMat> &weight_maps,wtk_strbuf_t **weight_tabs,int step)
{
    for(unsigned i = 0; i < blender->weight_maps.size(); ++i) {
        cv::Mat weight = weight_maps[i].getMat(cv::ACCESS_READ);
        // for(int j = 0; j < weight.rows; ++j){
        //     for(int k = 0; k < weight.cols; ++k){
        //         printf("%d\n",weight.at<ushort>(j,k));
        //     }
        // }
        cv::Mat y_offset_map = y_offset_maps[i];
        wtk_strbuf_t *buf = weight_tabs[i];
        _blender_create_weight_table(weight,y_offset_map,buf,step);
        // _table2weight_print(buf,weight);
    }
    return;
}