#include <math.h>
#include "qtk_stitch_mega_scaler.h"
#include "wtk/core/wtk_type.h"

#ifndef MIN
#define MIN(x,y) ((x) < (y) ? (x) : (y))
#endif
float qtk_stitch_mega_scaler_force_downscale(float scale)
{
    return MIN(1.0,scale);
}

void qtk_stitch_mega_scaler_set_scale(qtk_stitch_mega_pix_scaler_t *dscale,float scale)
{
    scale = qtk_stitch_mega_scaler_force_downscale(scale);
    dscale->scale = scale;
    dscale->is_scale_set = 1;
    return;
}

void qtk_stitch_set_scale_by_img_size(qtk_stitch_mega_pix_scaler_t *dscale,int image_w,int image_h)
{
    float scale = qtk_stitch_get_scale_by_resolution(dscale,image_w*image_h);
    qtk_stitch_mega_scaler_set_scale(dscale,scale);
    return;
}

void qtk_stitch_get_scaled_img_size(qtk_stitch_mega_pix_scaler_t *dscale,int image_w,int image_h,int *scaled_w,int *scaled_h)
{
    *scaled_w = roundf(image_w * dscale->scale); //这里不知道对不对
    *scaled_h = roundf(image_h * dscale->scale);
    return;
}

float qtk_stitch_get_scale_by_resolution(qtk_stitch_mega_pix_scaler_t *dscale,float resolution)
{
    float megapix = 1.0f;
    if(dscale->megapix > 0){
        megapix = sqrtf(dscale->megapix*1e6/resolution);
    }
    return megapix;
}

void qtk_stitch_mega_pix_scaler_init(qtk_stitch_mega_pix_scaler_t *dscale,float megapix)
{
    dscale->megapix = megapix;
    dscale->is_scale_set = 0;
    dscale->scale = 0.0f;
    return;
}
