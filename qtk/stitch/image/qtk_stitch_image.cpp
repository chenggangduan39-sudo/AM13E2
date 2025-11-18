#include "qtk_stitch_image.h"
#include "wtk/core/wtk_alloc.h"
#include "wtk/core/wtk_str.h"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/core/mat.hpp"

void* qtk_stitch_image_read_image(const char *name);
void qtk_stitch_image_get_image_size(qtk_stitch_image_t *stitch_image);
void* qtk_stitch_image_data_image(void *data, int w, int h);
qtk_stitch_image_t* qtk_stitch_image_file_new(const char *name, float medium_megapix, 
                                            float low_megapix, float final_megapix)
{
    qtk_stitch_image_t *stitch_image = (qtk_stitch_image_t*)malloc(sizeof(*stitch_image));
    memset(stitch_image, 0, sizeof(*stitch_image));

    stitch_image->name = wtk_str_dup(name);
    qtk_stitch_mega_pix_scaler_init(&stitch_image->medium_megapix,medium_megapix);
    qtk_stitch_mega_pix_scaler_init(&stitch_image->low_megapix,low_megapix);
    qtk_stitch_mega_pix_scaler_init(&stitch_image->final_megapix,final_megapix);

    stitch_image->image_data = qtk_stitch_image_read_image(stitch_image->name);
    qtk_stitch_image_get_image_size(stitch_image);
    // wtk_debug("%s size(%d %d)\n",stitch_image->name,stitch_image->w,stitch_image->h);

    qtk_stitch_set_scale_by_img_size(&stitch_image->medium_megapix,stitch_image->w,stitch_image->h);
    qtk_stitch_set_scale_by_img_size(&stitch_image->low_megapix,stitch_image->w,stitch_image->h);
    qtk_stitch_set_scale_by_img_size(&stitch_image->final_megapix,stitch_image->w,stitch_image->h);

    return stitch_image;
}

qtk_stitch_image_t* qtk_stitch_image_new(int row, int col, float medium_megapix, 
                                            float low_megapix, float final_megapix)
{
    qtk_stitch_image_t *stitch_image = (qtk_stitch_image_t*)wtk_malloc(sizeof(*stitch_image));
    memset(stitch_image, 0, sizeof(*stitch_image));

    qtk_stitch_mega_pix_scaler_init(&stitch_image->medium_megapix,medium_megapix);
    qtk_stitch_mega_pix_scaler_init(&stitch_image->low_megapix,low_megapix);
    qtk_stitch_mega_pix_scaler_init(&stitch_image->final_megapix,final_megapix);

    stitch_image->w = col;
    stitch_image->h = row;
    // wtk_debug("%s size(%d %d)\n",stitch_image->name,stitch_image->w,stitch_image->h);

    qtk_stitch_set_scale_by_img_size(&stitch_image->medium_megapix,stitch_image->w,stitch_image->h);
    qtk_stitch_set_scale_by_img_size(&stitch_image->low_megapix,stitch_image->w,stitch_image->h);
    qtk_stitch_set_scale_by_img_size(&stitch_image->final_megapix,stitch_image->w,stitch_image->h);

    return stitch_image;
}

int qtk_stitch_image_todata(qtk_stitch_image_t *stitch_image,void *data)
{
    stitch_image->image_data = qtk_stitch_image_data_image(data,stitch_image->w,stitch_image->h);
    return 0;
}

int qtk_stitch_image_todata2(qtk_stitch_image_t *stitch_image,float channel, void *data)
{
    cv::Mat *img_to = NULL;
    if(channel == 3){
        img_to = new cv::Mat(stitch_image->h,stitch_image->w,CV_8UC3,data);
    }else if(channel == 4){
        img_to = new cv::Mat(stitch_image->h,stitch_image->w,CV_8UC4,data);
    }else if(channel == 1.5f){ //nv12等格式
        int h = stitch_image->h * 1.5f;
        img_to = new cv::Mat(h,stitch_image->w,CV_8UC1,data);
    }
    stitch_image->image_data = (void*)img_to;
    return 0;
}

void qtk_stitch_image_get_image_size(qtk_stitch_image_t *stitch_image)
{
    cv::Mat *M = (cv::Mat*)stitch_image->image_data;

    stitch_image->w = M->cols;
    stitch_image->h = M->rows;

    return;
}

void* qtk_stitch_image_read_image(const char *name)
{
    cv::Mat img;
    img = cv::imread(name);
    cv::Mat *img_to = new cv::Mat(img);
    // uchar *d = img_to->data;
    // wtk_debug("dim %d rows %d cols %d step %d %d type %d\n",img_to->dims,img_to->rows,img_to->cols,img_to->step[0],img_to->step[1],img_to->type());
    // for(int i = 0; i < img.rows; i++){
    //     for(int j = 0; j < img.cols; j++){
    //         for(int k = 0; k < img.step[1]; ++k){
    //             printf("%d\n",d[i*img.step[0]+j*img.step[1]+k]);
    //         }
    //     }
    // }
    return img_to;
}

void* qtk_stitch_image_data_image(void *data, int w, int h)
{
    cv::Mat img;
    cv::Mat *img_to = new cv::Mat(h,w,CV_8UC3,data);
    // uchar *d = img_to->data;
    // wtk_debug("dim %d rows %d cols %d step %d %d type %d\n",img_to->dims,img_to->rows,img_to->cols,img_to->step[0],img_to->step[1],img_to->type());
    // for(int i = 0; i < img.rows; i++){
    //     for(int j = 0; j < img.cols; j++){
    //         for(int k = 0; k < img.step[1]; ++k){
    //             printf("%d\n",d[i*img.step[0]+j*img.step[1]+k]);
    //         }
    //     }
    // }
    return img_to;
}


cv::Mat* _image_resize(cv::Mat& input_img,int dst_w,int dst_h)
{
    cv::Mat output_img;
    if(input_img.rows != dst_h || input_img.cols != dst_w){
        cv::resize(input_img,output_img,cv::Size(dst_w,dst_h),0,0,cv::InterpolationFlags::INTER_LINEAR_EXACT);
    }else{
        output_img = input_img;
    }
    return new cv::Mat(output_img);
}

void qtk_stitch_image_resize(qtk_stitch_image_t* img,int type)
{
    qtk_stitch_mega_pix_scaler_t *scale = NULL;
    int dst_w,dst_h;
    cv::Mat *input_img = NULL;
    cv::Mat output_img;

    switch(type){
        case QTK_STITCH_IMAGE_RESOLUTION_MEDIUM:
            input_img = (cv::Mat *)img->image_data;
            scale = &img->medium_megapix;
            break;
        case QTK_STITCH_IMAGE_RESOLUTION_LOW:
            input_img = (cv::Mat *)img->medium_image_data;
            scale = &img->low_megapix;
            break;
        case QTK_STITCH_IMAGE_RESOLUTION_FINAL:
            input_img = (cv::Mat *)img->image_data;
            scale = &img->final_megapix;
            break;
        default:
            break;
    }
    
    qtk_stitch_get_scaled_img_size(scale,img->w,img->h,&dst_w,&dst_h);
    wtk_debug("resize %d %d -> %d %d\n",input_img->rows,input_img->cols,dst_w,dst_h);
    switch(type){
        case QTK_STITCH_IMAGE_RESOLUTION_MEDIUM:
            img->medium_image_data = (void*)_image_resize(*input_img,dst_w,dst_h);
            break;
        case QTK_STITCH_IMAGE_RESOLUTION_LOW:
            img->low_image_data = (void*)_image_resize(*input_img,dst_w,dst_h);
            break;
        case QTK_STITCH_IMAGE_RESOLUTION_FINAL:
            img->final_image_data = (void*)_image_resize(*input_img,dst_w,dst_h);
            break;
        default:
            wtk_debug("resize case error!\n");
            exit(1);
            break;
    }
    return;
}

void qtk_stitch_image_set_final(qtk_stitch_image_t* img)
{
    cv::Mat *input_img = NULL;

    input_img = (cv::Mat *)img->image_data;
    img->final_image_data = new cv::Mat(*input_img);

    return;
}

void qtk_stitch_image_delete(qtk_stitch_image_t *stitch_image)
{
    if(stitch_image){
        if(stitch_image->final_image_data){
            cv::Mat *M = (cv::Mat*)stitch_image->final_image_data;
            delete M;
            stitch_image->final_image_data = NULL;
        }
        if(stitch_image->low_image_data){
            cv::Mat *M = (cv::Mat*)stitch_image->low_image_data;
            delete M;
            stitch_image->low_image_data = NULL;
        }
        if(stitch_image->medium_image_data){
            cv::Mat *M = (cv::Mat*)stitch_image->medium_image_data;
            delete M;
            stitch_image->medium_image_data = NULL;
        }
        if(stitch_image->image_data){
            cv::Mat *M = (cv::Mat*)stitch_image->image_data;
            delete M;
            stitch_image->image_data = NULL;
        }
        if(stitch_image->name){
            wtk_free(stitch_image->name);
        }
        wtk_free(stitch_image);
    }
    return;
}

void _get_scaled_img_size(qtk_stitch_mega_pix_scaler_t *scale,int w, int h, int *tw, int *th)
{
    *tw = round(w*scale->scale);
    *th = round(h*scale->scale);
    return;
}
void qtk_stitch_image_get_scaled_img_sizes(qtk_stitch_image_t *image, int type, int *w, int *h)
{
    qtk_stitch_mega_pix_scaler_t *scale = NULL;
    switch(type){
        case QTK_STITCH_IMAGE_RESOLUTION_MEDIUM:
            scale = &image->medium_megapix;
            break;
        case QTK_STITCH_IMAGE_RESOLUTION_LOW:
            scale = &image->low_megapix;
            break;
        case QTK_STITCH_IMAGE_RESOLUTION_FINAL:
            scale = &image->final_megapix;
            break;
        default:
            wtk_debug("resize case error!\n");
            exit(1);
            break;
    }
    _get_scaled_img_size(scale, image->w, image->h, w, h);
    return;
}

float qtk_stitch_image_get_ratio(qtk_stitch_image_t *image, int form_type, int to_type)
{
    qtk_stitch_mega_pix_scaler_t *from_scale = NULL;
    qtk_stitch_mega_pix_scaler_t *to_scale = NULL;
    switch(form_type){
        case QTK_STITCH_IMAGE_RESOLUTION_MEDIUM:
            from_scale = &image->medium_megapix;
            break;
        case QTK_STITCH_IMAGE_RESOLUTION_LOW:
            from_scale = &image->low_megapix;
            break;
        case QTK_STITCH_IMAGE_RESOLUTION_FINAL:
            from_scale = &image->final_megapix;
            break;
        default:
            wtk_debug("resize case error!\n");
            exit(1);
            break;
    }
    switch(to_type){
        case QTK_STITCH_IMAGE_RESOLUTION_MEDIUM:
            to_scale = &image->medium_megapix;
            break;
        case QTK_STITCH_IMAGE_RESOLUTION_LOW:
            to_scale = &image->low_megapix;
            break;
        case QTK_STITCH_IMAGE_RESOLUTION_FINAL:
            to_scale = &image->final_megapix;
            break;
        default:
            wtk_debug("resize case error!\n");
            exit(1);
            break;
    }
    return (to_scale->scale/from_scale->scale);
}

void* qtk_stitch_image_read_data(const char *name)
{
    cv::Mat img;
    img = cv::imread(name,cv::IMREAD_UNCHANGED);
    // printf("img rows: %d, cols: %d channel: %d\n",img.rows,img.cols,img.channels());
#ifdef USE_9391
    void *img_to = wtk_malloc(img.rows*img.cols*4);
    memcpy(img_to,img.ptr(),img.rows*img.cols*4);
#else
    void *img_to = wtk_malloc(img.rows*img.cols*3);
    memcpy(img_to,img.ptr(),img.rows*img.cols*3);
#endif
    return img_to;
}

void qtk_stitch_image_save_data(const char *name, void *img, int w, int h, int c)
{
    cv::Mat img_to;
    if(c == 1){
        img_to = cv::Mat(h,w,CV_8UC1,img);
    }else if(c == 3){
        img_to = cv::Mat(h,w,CV_8UC3,img);
    }else if(c == 4){
        img_to = cv::Mat(h,w,CV_8UC4,img);
    }
    cv::imwrite(name,img_to);
    return;
}

void* qtk_stitch_image_read_data_raw(const char *name)
{
    FILE *f = fopen(name,"rb");
    if(f == NULL){
        printf("path %s error\n",name);
        return NULL;
    }
    int len = 0;
    void *img_to = NULL;
    fseek(f,0,SEEK_END);
    len = ftell(f);
    fseek(f,0,SEEK_SET);
    img_to = wtk_malloc(len);
    int ret = fread(img_to,1,len,f);
    if(ret != len){
        printf("read_data_raw error\n");
        exit(1);
    }
    printf("raw data len %d\n",len);
    fclose(f);
    return img_to;
}
void* qtk_stitch_image_cvtColor_nv122rgb(void *img,int w, int h)
{
    int h_to = h/1.5f;
    cv::Mat src = cv::Mat(h,w,CV_8UC1,img);
    cv::Mat dst = cv::Mat(h_to,w,CV_8UC3);
    cv::cvtColor(src,dst,cv::COLOR_YUV2RGB_NV12);
    void *img_to = wtk_malloc(dst.rows*dst.cols*3);
    memcpy(img_to,dst.ptr(),dst.rows*dst.cols*3);
    return img_to;
}