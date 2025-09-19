#ifndef __QTK_STITCH_SEAMFINDER_H__
#define __QTK_STITCH_SEAMFINDER_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_stitch_seamfinder qtk_stitch_seamfinder_t;

qtk_stitch_seamfinder_t* qtk_stitch_seamfinder_new(int type, int img_type, int left_full, int right_full);
void qtk_stitch_seamfinder_delete(qtk_stitch_seamfinder_t *finder);
void* qtk_stitch_seamfinder_find(qtk_stitch_seamfinder_t *finder,void* imgs,void* corners, void* masks);
void* qtk_stitch_seamfinder_resize(qtk_stitch_seamfinder_t *finder, void*masks,void *seam_masks);
void qtk_stitch_seamfinder_seam_masks_delete(void *seam_masks);
void* qtk_stitch_seamfinder_seam_masks_vector_new(void);
void qtk_stitch_seamfinder_seam_masks_vector_push(void *seam_masks,unsigned char *data,int rows,int cols);
void qtk_stitch_seamfinder_seam_masks_range(void *seam_masks,void *corners, int x, int y, int w, int h, int idx);
void qtk_stitch_seamfinder_get_low_seam_size(void *corp_mask,int caram_num, int *w,int *h);
void qtk_stitch_seamfinder_seammasks_final2low(void* seam_masks,void *seam_masks_low,int *w, int *h);
void qtk_stitch_seamfinder_seammasks_low2final(void *masks,void* seam_masks,void *seam_masks_low);
void qtk_stitch_seamfinder_seammasks_scores_nsp(qtk_stitch_seamfinder_t *seamfinder,
                    void *imgs,void *seam_masks_low,int *points,float *scores,int points_num,int nsp);
void qtk_stitch_seamfinder_seammasks_find_point(void *seam_masks_low,int point_num,int *points, int channels);
void qtk_stitch_seamfinder_seammasks_scores(qtk_stitch_seamfinder_t *seamfinder, void *imgs,void *seam_masks_low,
                                        int *points,float *scores,int points_num);
void* qtk_stitch_seamfinder_find2(qtk_stitch_seamfinder_t *finder,void* corners,void* imgs, void* masks, int *seammask_points,int point_num);
void qtk_stitch_seamfinder_find3(qtk_stitch_seamfinder_t *finder,void *low_seammasks,
                    void* corners,void* imgs, void* masks, int *point, int point_num, int idx);
void* qtk_stitch_seamfinder_seammasks_copy(void* seam_masks);
void qtk_stitch_seamfinder_findblock_process(qtk_stitch_seamfinder_t *finder,void *low_seammasks,
            void* corners,void* imgs, void* masks,int *point,int point_num,int idx,int split_num,int start,int end);
void qtk_stitch_seamfinder_humanseg_findblock_process(qtk_stitch_seamfinder_t *finder,void *low_seammasks,
            void* corners,void* imgs, void* masks,void *humanseg_mask,int *point,int point_num,int idx,int split_num,int start,int end);
void qtk_stitch_seamfinder_humanseg_find3(qtk_stitch_seamfinder_t *finder,void *low_seammasks,
            void* corners,void* imgs,void* masks,void *humanseg_masks,int *point,int point_num,int idx);
#ifdef __cplusplus
}
#endif

#endif