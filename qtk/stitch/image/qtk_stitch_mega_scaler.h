#ifndef __QTK_STITCH_Mega_Scaler_H__
#define __QTK_STITCH_Mega_Scaler_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_stitch_mega_pix_scaler{
    float megapix;
    float scale;
    int is_scale_set;
}qtk_stitch_mega_pix_scaler_t;

float qtk_stitch_get_scale_by_resolution(qtk_stitch_mega_pix_scaler_t *dscale,float resolution);
float qtk_stitch_mega_scaler_force_downscale(float scale);
void qtk_stitch_mega_scaler_set_scale(qtk_stitch_mega_pix_scaler_t *dscale,float scale);
void qtk_stitch_set_scale_by_img_size(qtk_stitch_mega_pix_scaler_t *dscale,int image_w,int image_h);
void qtk_stitch_get_scaled_img_size(qtk_stitch_mega_pix_scaler_t *dscale,int image_w,int image_h,int *scaled_w,int *scaled_h);
void qtk_stitch_mega_pix_scaler_init(qtk_stitch_mega_pix_scaler_t *dscale,float megapix);

#ifdef __cplusplus
}
#endif


#endif