#ifndef __QTK_STITCH_CROPPER_H__
#define __QTK_STITCH_CROPPER_H__

#include "wtk/core/wtk_strbuf.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct qtk_stitch_cropper{
    int do_crop;
    int ncamera;
    int remap_type; //crop remap 的 remap函数使用
    int img_type; //需要处理的img的数据格式 默认rgb
    int width;
    int hight;
    void *overlapping_rectangles; //std::vector<cv::Rect> 
    void *intersection_rectangles; //std::vector<cv::Rect>

    void *masks; //cv::Mat[]
    void *imgs; //cv::Mat[]
    void *corners; //std::vector<cv::Point>
    void *size; //std::vector<cv::Size>
    void *crop_uxmaps; //std::vector<cv::UMat>
    void *crop_uymaps; // std::vector<cv::UMat>
    void *crop_offset_maps; // cv::Mat[]
    void *crop_uv_offset_maps; // cv::Mat[]
    void *concert_maps1; //cv::Mat[]
    void *concert_maps2; //cv::Mat[]
    wtk_strbuf_t **crop_offset_table;
    wtk_strbuf_t **crop_offset_uv_table;
}qtk_stitch_cropper_t;

qtk_stitch_cropper_t* qtk_stitch_cropper_new(int do_crop, int ncamera,int remap_type,int img_type);
void qtk_stitch_cropper_set_src_rect(qtk_stitch_cropper_t* cropper,int width, int hight);
void qtk_stitch_cropper_delete(qtk_stitch_cropper_t* cropper);
void qtk_stitch_cropper_prepare(qtk_stitch_cropper_t* cropper,void *imgs, void *masks, void *corners,void *sizes);
void qtk_stitch_corpper_crop(qtk_stitch_cropper_t* cropper,void *imgs, void *masks, 
                                            void *corners,void *sizes, float aspect);
void qtk_stitch_corpper_crop_parameter(qtk_stitch_cropper_t* cropper,void *masks, 
                void *corners,void *sizes, float aspect);
void qtk_stitch_corpper_crop_imgs(qtk_stitch_cropper_t* cropper,void *imgs, float aspect);
void qtk_stitch_corpper_crop_img(qtk_stitch_cropper_t* cropper,void *imgs, float aspect,int index);
void qtk_stitch_cropper_rectangles(qtk_stitch_cropper_t* cropper);
void qtk_stitch_cropper_rectangles_overlapping_push(qtk_stitch_cropper_t* cropper,int x, int y, int width, int height);
void qtk_stitch_cropper_rectangles_intersection_push(qtk_stitch_cropper_t* cropper,int x, int y, int width, int height);
void qtk_stitch_corpper_crop_maps(qtk_stitch_cropper_t* cropper,void *imgs1,void *imgs2, float aspect);
void qtk_stitch_corpper_clear_crop_maps(qtk_stitch_cropper_t* cropper);
void qtk_stitch_corpper_crop_remaps(qtk_stitch_cropper_t* cropper,wtk_queue_t* queue);
void qtk_stitch_corpper_crop_remaps2(qtk_stitch_cropper_t* cropper,wtk_queue_t* queue);
void qtk_stitch_corpper_crop_remap(qtk_stitch_cropper_t* cropper,wtk_queue_t* queue, int idx);
void qtk_stitch_corpper_create_offset_maps(qtk_stitch_cropper_t* cropper);
void qtk_stitch_corpper_create_offset_maps_all(qtk_stitch_cropper_t* cropper);
void qtk_stitch_corpper_create_fast_maps(qtk_stitch_cropper_t* cropper);
void qtk_stitch_corpper_crop_remaps2_all(qtk_stitch_cropper_t* cropper,wtk_queue_t* queue);
void* qtk_stitch_cropper_get_image(qtk_stitch_cropper_t* cropper,int *w, int *h, int idx);
void qtk_stitch_cropper_rectangle_masks(qtk_stitch_cropper_t* cropper,int *offsets);
void qtk_stitch_corpper_nearest_2tables(qtk_stitch_cropper_t* cropper);
void qtk_stitch_cropper_overlap_save(qtk_stitch_cropper_t *cropper, char *path);
#ifdef __cplusplus
}
#endif

#endif
