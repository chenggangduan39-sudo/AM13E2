#ifndef QTK_STITCH_HUMANSEG_RUN_H
#define QTK_STITCH_HUMANSEG_RUN_H 

#include "qtk_stitch_humanseg.h"

#ifdef __cplusplus
extern "C" {
#endif 

void qtk_stitch_humanseg_get_humanseg(qtk_stitch_humanseg_t *seg, void *_corners, void *_low_mask, void *_imgs);
void qtk_stitch_humanseg_clear_humanseg(qtk_stitch_humanseg_t *seg);
void qtk_stitch_humanseg_seammask_cross(qtk_stitch_humanseg_t *seg,int *point,int point_num,
                            int cross_size,int split_num,int *is_coress);
int qtk_stitch_humanseg_mask_connect(qtk_stitch_humanseg_t *seg,int split_num,int idx, int the_split);
#ifdef __cplusplus
}
#endif

#endif