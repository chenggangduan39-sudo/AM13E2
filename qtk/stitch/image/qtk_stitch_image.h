#ifndef __QTK_STITCH_IMAGE_HPP__
#define __QTK_STITCH_IMAGE_HPP__

#include "qtk_stitch_mega_scaler.h"

#define QTK_STITCH_IMAGE_RESOLUTION_MEDIUM  0
#define QTK_STITCH_IMAGE_RESOLUTION_LOW     1
#define QTK_STITCH_IMAGE_RESOLUTION_FINAL   2

#ifdef __cplusplus
extern "C"{
#endif

typedef struct qtk_stitch_image{
    qtk_stitch_mega_pix_scaler_t medium_megapix;
    qtk_stitch_mega_pix_scaler_t low_megapix;
    qtk_stitch_mega_pix_scaler_t final_megapix;

    char *name; //file name 
    void *image_data; // 原始 cv::Mat
    void *medium_image_data; // 缩放后的 cv::Mat
    void *low_image_data;
    void *final_image_data;
    int w;
    int h;
}qtk_stitch_image_t;

qtk_stitch_image_t* qtk_stitch_image_file_new(const char *name, float medium_megapix, 
                                            float low_megapix, float final_megapix);
qtk_stitch_image_t* qtk_stitch_image_new(int row, int col, float medium_megapix, 
                                            float low_megapix, float final_megapix);
int qtk_stitch_image_todata(qtk_stitch_image_t *stitch_image,void *data);
int qtk_stitch_image_todata2(qtk_stitch_image_t *stitch_image,float channel, void *data);
void qtk_stitch_image_resize(qtk_stitch_image_t* img,int type);
void qtk_stitch_image_set_final(qtk_stitch_image_t* img);
void qtk_stitch_image_delete(qtk_stitch_image_t *stitch_image);
void qtk_stitch_image_get_scaled_img_sizes(qtk_stitch_image_t *image, int type, int *w, int *h);
float qtk_stitch_image_get_ratio(qtk_stitch_image_t *image, int form_type, int to_type);
void* qtk_stitch_image_read_data(const char *name);
void* qtk_stitch_image_read_data_raw(const char *name);
void qtk_stitch_image_save_data(const char *name, void *img, int w, int h, int c);
void* qtk_stitch_image_read_image(const char *name);
void* qtk_stitch_image_cvtColor_nv122rgb(void *img,int w, int h);

#ifdef __cplusplus
}
#endif

#endif