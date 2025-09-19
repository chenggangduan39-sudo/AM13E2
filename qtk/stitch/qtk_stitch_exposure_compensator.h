#ifndef __QTK_STITCH_EXPOSURE_COMPENSATOR_H__
#define __QTK_STITCH_EXPOSURE_COMPENSATOR_H__

#ifdef __cplusplus
extern "C" {
#endif
//曝光补偿
typedef struct qtk_stitch_exposure_compensator_t qtk_stitch_exposure_compensator_t;

qtk_stitch_exposure_compensator_t* qtk_stitch_exposure_compensator_new(int type, 
                                                                int nr_feeds, 
                                                                int block_size,int num_cameras,int img_type);
void qtk_stitch_exposure_compensator_delete(qtk_stitch_exposure_compensator_t *comp);
void qtk_stitch_exposure_compensator_feed(qtk_stitch_exposure_compensator_t *comp,void *corners,void *imgs,void *masks);
void qtk_stitch_exposure_compensator_apply(qtk_stitch_exposure_compensator_t *comp,void *corners,void *img,void *mask);
void qtk_stitch_exposure_compensator_apply_index(qtk_stitch_exposure_compensator_t *comp,
                                                    void *corners,void *img,void *mask, int index);
void *qtk_stitch_exposure_compensator_gain_mat_get(qtk_stitch_exposure_compensator_t *comp);
void qtk_stitch_exposure_compensator_resize(qtk_stitch_exposure_compensator_t *comp,void *imgs);
void qtk_stitch_exposure_compensator_resize2point(qtk_stitch_exposure_compensator_t *comp,void *imgs);
#ifdef __cplusplus
}
#endif

#endif