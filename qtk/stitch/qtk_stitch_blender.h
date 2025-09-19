#ifndef __QTK_STITCH_BLENDER_H__
#define __QTK_STITCH_BLENDER_H__

#include "wtk/core/wtk_queue.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_stitch_blender qtk_stitch_blender_t;

qtk_stitch_blender_t* qtk_stitch_blender_new(int type,float blend_stength,int use_gpu,int use_alpha, float smooth, int img_type);
void qtk_stitch_blender_delete(qtk_stitch_blender_t* blender);
void* qtk_stitch_blender_create_panorama(void *imgs, void *masks, void *corners, void *sizes);
void qtk_stitch_blender_prepare(qtk_stitch_blender_t* blend,void *corners, void *sizes);
void qtk_stitch_blender_feed(qtk_stitch_blender_t* blend,void *imgs,void *masks,void *corner);
void qtk_stitch_blender_feed_index(qtk_stitch_blender_t* blend,
                                void *imgs,void *seam_masks,void *corners, int index);
void qtk_stitch_blender_blend(qtk_stitch_blender_t* blender);
unsigned char* qtk_stitch_blender_get_panorama(qtk_stitch_blender_t* blender,int *row,int *col);
void qtk_stitch_blender_prepare2(qtk_stitch_blender_t* blender,void *corners, void *sizes, void *ptr);
void qtk_stitch_blender_start(qtk_stitch_blender_t* blender,void *masks, void *corners, void *sizes, int use_nthread);
void qtk_stitch_blender_stop(qtk_stitch_blender_t* blender);
void qtk_stitch_blender_get_rect(qtk_stitch_blender_t* blender,void *corners, void *sizes,int *rows, int *cols);
void qtk_stitch_blender_set_rgb(qtk_stitch_blender_t* blender,int idx, char alpha,char beta,char gamma,
                                float alpha_x,float beta_x,float gamma_x);
void qtk_stitch_blender_update_weight_maps(qtk_stitch_blender_t* blender,void *gain_mat);
void qtk_stitch_blender_set_smooth(qtk_stitch_blender_t* blender,float smooth);
// void qtk_stitch_blender_remap_feather(qtk_stitch_blender_t* blender, wtk_queue_t* queue,
//                                 void *seam_masks,void *corners,void *crop_offset_maps);
#ifdef __cplusplus
}
#endif

#endif