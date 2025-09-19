#define _GNU_SOURCE
#include "qtk_stitch_run_phase.h"
#include <sys/types.h>
#include <stdio.h>
#include "qtk/stitch/image/qtk_stitch_image.h"
#include <sched.h>
#include <pthread.h>
#include "qtk_stitch_humanseg_run.h"

typedef struct _warper_thread_use{
    int num;
    void *userdata;
}_warper_thread_use_t;


void _crop_final_resolution_parameter(qtk_stitch_run_phase_t *phase,qtk_stitch_image_t *image);
void _warp_final_resolution_parameter(qtk_stitch_run_phase_t *phase,qtk_stitch_image_t *image);
void _warp_low_resolution_parameter(qtk_stitch_run_phase_t *phase,qtk_stitch_image_t *image);
void _crop_low_resolution_parameter(qtk_stitch_run_phase_t *phase,qtk_stitch_image_t *image);
void _warp_final_resolution_imgs(qtk_stitch_run_phase_t* phase);
void _warp_final_resolution_imgs2(qtk_stitch_run_phase_t* phase);
void _warp_final_resolution_img(qtk_stitch_run_phase_t* phase,int index);
void _crop_final_resolution_imgs(qtk_stitch_run_phase_t* phase);
void _crop_final_resolution_img(qtk_stitch_run_phase_t* phase,int idx);
void _run_phase_compensate_exposure_errors(qtk_stitch_run_phase_t* phase);
void _run_phase_estimate_exposure_errors(qtk_stitch_run_phase_t* phase);
void _run_phase_compensate_exposure_error(qtk_stitch_run_phase_t* phase, int index);
void _run_phase_initialize_composition(qtk_stitch_run_phase_t *phase);
void _run_phase_initialize_composition2(qtk_stitch_run_phase_t *phase);
void _run_phase_blend_images(qtk_stitch_run_phase_t* phase,void *reseam_masks);
void _run_phase_blend_image(qtk_stitch_run_phase_t* phase,void *reseam_masks, int index);
void __run_phase_create_final_panorama(qtk_stitch_run_phase_t* phase);
int _run_paper(void *data,wtk_thread_t *t);
int _run_paper_wake(qtk_stitch_run_phase_t *phase);
int _run_all_warper(void *t_data,wtk_thread_t *t);
static void _pthread_setaffinity(int n);
void _crop_remaps_imgs(qtk_stitch_run_phase_t* phase);
void _crop_remaps_img(qtk_stitch_run_phase_t* phase, int idx);
void _crop_remaps_low_imgs(qtk_stitch_run_phase_t* phase);
int _run_phase_call_notify(qtk_stitch_run_phase_t *phase);
int _run_phase_re_charge_seammask(qtk_stitch_run_phase_t *phase);
void _re_charge_seammask_section(qtk_stitch_run_phase_t *phase,int idx,u_char *update_list);

qtk_stitch_queue_data_t *_stitch_run_queue_data_new(void *data)
{
    qtk_stitch_queue_data_t *qdata = (qtk_stitch_queue_data_t *)wtk_malloc(sizeof(qtk_stitch_queue_data_t));
    memset(qdata,0,sizeof(qtk_stitch_queue_data_t));
    return qdata;
}

int _stitch_run_queue_data_delete(void *data)
{
    qtk_stitch_queue_data_t *qdata = (qtk_stitch_queue_data_t*)data;
    wtk_free(qdata);
    return 0;
}

float *_get_cameras_data(char *path,int row, int col)
{
    float *Rt = wtk_malloc(sizeof(float)*row*col);
    memset(Rt,0,sizeof(float)*row*col);
    FILE *fp = fopen(path,"rb");
    int r = sizeof(double);
    float *p = Rt;
    double pu = 0.0;
    int ret = 0;
    // wtk_debug("read %s %d %d\n",path,row,col);
    do{
        ret = fread(&pu,1,r,fp);
        if(ret != r){
            break;
        }
        *p = pu;
        // wtk_debug("%f\n",*p);
        p++;
    }while(1);
    fclose(fp);
    return Rt;
}

unsigned char *_get_seam_data(char *path,int row, int col)
{
    unsigned char *seam = wtk_malloc(sizeof(unsigned char)*row*col);
    FILE *fp = fopen(path,"rb");
    int r = sizeof(unsigned char);
    unsigned char *p = seam;
    unsigned char pt = 0;
    int ret = 0;
    do{
        ret = fread(&pt,1,r,fp);
        if(ret != r){
            break;
        }
        *p = pt;
        p++;
    }while(1);
    fclose(fp);
    return seam;
}

void qtk_stitch_run_init(qtk_stitch_run_phase_t *phase)
{
    int num_camera = phase->cfg->camera_num;
    qtk_stitch_run_camera_cfg_t *cameras_cfg = phase->cfg->cameras_cfg;
    qtk_stitch_run_seammask_cfg_t *seam_cfg = phase->cfg->seammasks_cfg;
    qtk_stitch_image_t *image = NULL;

    int i = 0;
    for (i = 0; i < num_camera; i++){
        qtk_stitch_run_camera_cfg_t *cfg = cameras_cfg+i;
        int *shape = (int*)cfg->Rt_shape->slot;
        int row = shape[0];
        int col = shape[1];
        // wtk_debug("%d %d %s\n",row,col,cfg->Rt_path);
        float *Rt = _get_cameras_data(cfg->Rt_path,row,col);
        qtk_stitch_camera_estimator_camera_push(phase->camera_estimator,cfg->aspect,cfg->focal,
                                            cfg->ppx,cfg->ppy,Rt,Rt+(3*3));
        wtk_free(Rt);
    }

    phase->reseam_masks = qtk_stitch_seamfinder_seam_masks_vector_new();
    if(phase->cfg->use_reseammask){
        phase->reseam_masks_low = qtk_stitch_seamfinder_seam_masks_vector_new();
    }
    for(i = 0; i < num_camera; i++){
        qtk_stitch_run_seammask_cfg_t *cfg = seam_cfg+i;
        int *shape = (int*)cfg->shape->slot;
        int row = shape[0];
        int col = shape[1];
        // wtk_debug("%d %d %s\n",row,col,cfg->seam_mask_path);
        unsigned char *seam = _get_seam_data(cfg->seam_mask_path,row,col);
        // char pp[25] = {0};
        // sprintf(pp,"./%d_seam.png",i);
        // printf("%s\n",pp);
        // qtk_stitch_image_save_data(pp,seam,col,row,1);
        qtk_stitch_seamfinder_seam_masks_vector_push(phase->reseam_masks,seam,row,col);
        // if(phase->cfg->blender_type == QTK_STITCH_BLENDER_NO){
        //    qtk_stitch_seamfinder_seam_masks_vector_push(phase->reseam_masks_no,seam,row,col); 
        // }
        wtk_free(seam);
    }
    qtk_stitch_cropper_rectangles(phase->cropper);
    for(i = 0; i < num_camera; i++){
        qtk_stitch_run_rectangle_cfg_t *intersection = phase->cfg->intersection;
        qtk_stitch_run_rectangle_cfg_t *overlapping = phase->cfg->overlapping;

        qtk_stitch_cropper_rectangles_intersection_push(phase->cropper,intersection[i].x,intersection[i].y,
                                        intersection[i].width,intersection[i].height);
        qtk_stitch_cropper_rectangles_overlapping_push(phase->cropper,overlapping[i].x,overlapping[i].y,
                                        overlapping[i].width,overlapping[i].height);
    }
    qtk_stitch_warper_set_scale(phase->warper,phase->camera_estimator->cameras);
    image = qtk_stitch_image_new(phase->cfg->h,phase->cfg->w,phase->cfg->medium_megapix,phase->cfg->low_megapix,-1);
    if(phase->cfg->use_compensator || phase->cfg->use_reseammask){
        qtk_stitch_warper_set_scale(phase->low_warper,phase->camera_estimator->cameras);
        qtk_stitch_cropper_rectangles(phase->low_cropper);
        for(i = 0; i < num_camera; i++){
            qtk_stitch_run_rectangle_cfg_t *intersection = phase->cfg->intersection;
            qtk_stitch_run_rectangle_cfg_t *overlapping = phase->cfg->overlapping;

            qtk_stitch_cropper_rectangles_intersection_push(phase->low_cropper,intersection[i].x,intersection[i].y,
                                            intersection[i].width,intersection[i].height);
            qtk_stitch_cropper_rectangles_overlapping_push(phase->low_cropper,overlapping[i].x,overlapping[i].y,
                                            overlapping[i].width,overlapping[i].height);
        }
        _warp_low_resolution_parameter(phase,image);
        qtk_stitch_warper_warp_buildmaps(phase->low_warper,phase->size_w[0],phase->size_h[0],
                                    phase->camera_estimator->cameras,phase->camera_aspect);
        _crop_low_resolution_parameter(phase,image);
        if(phase->cfg->use_shift_seam_position){
            qtk_stitch_cropper_rectangle_masks(phase->low_cropper,phase->cfg->shift_seam_offsets);
        }
        // qtk_stitch_cropper_set_src_rect(phase->low_cropper,phase->cfg->w,phase->cfg->h);
        qtk_stitch_cropper_set_src_rect(phase->low_cropper,phase->size_w[0],phase->size_h[0]);
        qtk_stitch_corpper_crop_maps(phase->low_cropper,phase->low_warper->uxmap,phase->low_warper->uymap,phase->lir_aspect);
        qtk_stitch_warper_warp_clearmaps(phase->low_warper);
        if(phase->low_cropper->remap_type == 0){
            qtk_stitch_corpper_create_offset_maps(phase->low_cropper);
            qtk_stitch_corpper_clear_crop_maps(phase->low_cropper);
        }else if(phase->low_cropper->remap_type == 1){
            qtk_stitch_corpper_create_fast_maps(phase->low_cropper);
            qtk_stitch_corpper_create_offset_maps(phase->low_cropper);
            qtk_stitch_corpper_clear_crop_maps(phase->low_cropper);
        }
    }
    if(phase->cfg->use_reseammask){
        phase->seammask_points = wtk_malloc(2*sizeof(int)*phase->cfg->camera_num*phase->cfg->reseammask_point_num);
        phase->low_seammask_h = wtk_malloc(sizeof(int)*phase->cfg->camera_num);
        phase->low_seammask_w = wtk_malloc(sizeof(int)*phase->cfg->camera_num);
        qtk_stitch_seamfinder_get_low_seam_size(phase->low_cropper->masks,phase->cfg->camera_num,phase->low_seammask_w,phase->low_seammask_h);
        qtk_stitch_seamfinder_seammasks_final2low(phase->reseam_masks,phase->reseam_masks_low,phase->low_seammask_w,phase->low_seammask_h);
        qtk_stitch_seamfinder_seammasks_find_point(phase->reseam_masks_low,phase->cfg->reseammask_point_num,phase->seammask_points,1);
        phase->seammask_score = wtk_malloc((phase->cfg->camera_num-1) * sizeof(float)*phase->cfg->reseammask_split_num);
        memset(phase->seammask_score,0,(phase->cfg->camera_num-1) * sizeof(float)*phase->cfg->reseammask_split_num);

        phase->reseam_masks_src_low = qtk_stitch_seamfinder_seammasks_copy(phase->reseam_masks_low);
        phase->seammask_points_src = wtk_malloc(2*sizeof(int)*phase->cfg->camera_num*phase->cfg->reseammask_point_num);
        // phase->seammask_score_src = wtk_malloc(phase->cfg->camera_num * sizeof(float)*phase->cfg->reseammask_split_num);
        qtk_stitch_seamfinder_seammasks_find_point(phase->reseam_masks_src_low,phase->cfg->reseammask_point_num,phase->seammask_points_src,1);
        // memset(phase->seammask_score_src,0,phase->cfg->camera_num * sizeof(float)*phase->cfg->reseammask_split_num);
    }
    _warp_final_resolution_parameter(phase,image);
    qtk_stitch_warper_warp_buildmaps(phase->warper,phase->cfg->w,phase->cfg->h,
                                    phase->camera_estimator->cameras,phase->camera_aspect);
    _crop_final_resolution_parameter(phase,image);
    qtk_stitch_cropper_set_src_rect(phase->cropper,phase->cfg->w,phase->cfg->h);
    qtk_stitch_corpper_crop_maps(phase->cropper,phase->warper->uxmap,phase->warper->uymap,phase->lir_aspect);
    qtk_stitch_warper_warp_clearmaps(phase->warper); //释放掉warpbuildmaps的内存
    if(phase->cfg->remap_type == 0){
        if(phase->cfg->use_all_img){
            qtk_stitch_corpper_create_offset_maps_all(phase->cropper);
        }else{
            qtk_stitch_corpper_create_offset_maps(phase->cropper);
        }
        qtk_stitch_corpper_clear_crop_maps(phase->cropper);
        qtk_stitch_corpper_nearest_2tables(phase->cropper);
    }else if(phase->cfg->remap_type == 1){
        qtk_stitch_corpper_create_fast_maps(phase->cropper);
        if(phase->cfg->use_all_img){
            qtk_stitch_corpper_create_offset_maps_all(phase->cropper);
        }else{
            qtk_stitch_corpper_create_offset_maps(phase->cropper);
        }
        qtk_stitch_corpper_clear_crop_maps(phase->cropper);
    }
    qtk_stitch_image_delete(image);
    return;
}

void qtk_stitch_run_uninit(qtk_stitch_run_phase_t *phase)
{
    qtk_stitch_seamfinder_seam_masks_delete(phase->reseam_masks);
    phase->reseam_masks = NULL;
    // if(phase->reseam_masks_no != NULL){
    //     qtk_stitch_seamfinder_seam_masks_delete(phase->reseam_masks_no);
    //     phase->reseam_masks_no = NULL;   
    // }
    if(phase->reseam_masks_low){
        qtk_stitch_seamfinder_seam_masks_delete(phase->reseam_masks_low);
        phase->reseam_masks_low = NULL;
    }
    if(phase->reseam_masks_src_low){
        qtk_stitch_seamfinder_seam_masks_delete(phase->reseam_masks_src_low);
        phase->reseam_masks_src_low = NULL;
    }
    if(phase->seammask_points) wtk_free(phase->seammask_points);
    if(phase->seammask_points_src) wtk_free(phase->seammask_points_src);
    if(phase->low_seammask_h) wtk_free(phase->low_seammask_h);
    if(phase->low_seammask_w) wtk_free(phase->low_seammask_w);
    if(phase->seammask_score) wtk_free(phase->seammask_score);
    // if(phase->seammask_score_src) wtk_free(phase->seammask_score_src);
    return;
}

qtk_stitch_run_phase_t* qtk_stitch_run_phase_new(qtk_stitch_run_cfg_t *cfg)
{
    qtk_stitch_run_phase_t *phase = (qtk_stitch_run_phase_t*)wtk_malloc(sizeof(qtk_stitch_run_phase_t));
    memset(phase,0,sizeof(qtk_stitch_run_phase_t));
    phase->cfg = cfg;
    phase->camera_estimator = qtk_stitch_camera_estimator_new(cfg->estimator);

    phase->warper = qtk_stitch_warper_new(cfg->warper_type,cfg->camera_num);
    phase->cropper = qtk_stitch_cropper_new(cfg->crop,cfg->camera_num,cfg->remap_type,cfg->src_type);
    if(cfg->use_compensator || cfg->use_reseammask){
        phase->low_warper = qtk_stitch_warper_new(cfg->warper_type,cfg->camera_num);
        phase->low_cropper = qtk_stitch_cropper_new(cfg->crop,cfg->camera_num,1,0);
    }
    phase->compensator = qtk_stitch_exposure_compensator_new(cfg->compensator,
                                                        cfg->nr_feeds,cfg->block_size,cfg->camera_num,cfg->src_type);
    phase->blender = qtk_stitch_blender_new(cfg->blender_type,cfg->blend_strength,
                        cfg->use_gpu,cfg->use_alpha,cfg->smooth,cfg->src_type);

    if(cfg->use_reseammask){
        phase->seamfinder = qtk_stitch_seamfinder_new(cfg->finder,cfg->src_type,cfg->left_full,cfg->right_full);
    }

    if(cfg->use_humanseg){
        phase->humanseg = qtk_stitch_humanseg_new(&cfg->humanseg_cfg);
    }

    qtk_stitch_run_init(phase);

    wtk_queue_init(&phase->img_queue);
    wtk_queue_init(&phase->low_img_queue);
    // wtk_lockhoard_init(&phase->img_hoard,offsetof(qtk_stitch_queue_data_t,node),
    //                 10,(wtk_new_handler_t)_stitch_run_queue_data_new,
    //                 (wtk_delete_handler_t)_stitch_run_queue_data_delete,phase);
    wtk_blockqueue_init(&phase->paper_bq);
    wtk_thread_init(&phase->paper_thread,_run_paper,phase);
    wtk_sem_init(&phase->the_paper_sem,0);
    phase->paper_thread_run = 0;
    if(cfg->use_nthread){
        phase->the_threads = (wtk_thread_t*)wtk_malloc(sizeof(wtk_thread_t)*(cfg->camera_num-1));
        for(int i = 0; i < cfg->camera_num-1; i++){
            _warper_thread_use_t *user_data = wtk_malloc(sizeof(_warper_thread_use_t));
            user_data->userdata = phase;
            user_data->num = i+1;
            wtk_thread_init(&phase->the_threads[i],_run_all_warper,user_data);
        }
        wtk_blockqueue_init(&phase->the_bqs);
        wtk_sem_init(&phase->the_threads_sem,0);
    }
    phase->blend_mem = NULL;
    phase->first = 1;
    return phase;
}

void qtk_stitch_run_phase_queue_clean(qtk_stitch_run_phase_t* phase)
{
    wtk_queue_node_t *node = NULL;
    qtk_stitch_queue_data_t *data = NULL;
    while((node = wtk_queue_pop(&phase->img_queue))){
        data = data_offset2(node,qtk_stitch_queue_data_t,node);
        qtk_stitch_image_delete(data->img_data);
        data->img_data = NULL;
        // wtk_lockhoard_push(&phase->img_hoard,data);
        _stitch_run_queue_data_delete(data);
    }
    while((node = wtk_queue_pop(&phase->low_img_queue))){
        data = data_offset2(node,qtk_stitch_queue_data_t,node);
        qtk_stitch_image_delete(data->img_data);
        data->img_data = NULL;
        // wtk_lockhoard_push(&phase->img_hoard,data);
        _stitch_run_queue_data_delete(data);
    }
    return;
}

int qtk_stitch_run_phase_delete(qtk_stitch_run_phase_t *phase)
{
    if(phase->cfg->use_nthread){
        wtk_free(phase->the_threads);
    }
    if(phase->size_w){
        wtk_free(phase->size_w);
    }
    if(phase->size_h){
        wtk_free(phase->size_h);
    }
    if(phase->blender){
        qtk_stitch_blender_delete(phase->blender);
        phase->blender = NULL;
    }
    if(phase->compensator){
        qtk_stitch_exposure_compensator_delete(phase->compensator);
        phase->compensator = NULL;
    }
    if(phase->cropper){
        qtk_stitch_cropper_delete(phase->cropper);
        phase->cropper = NULL;
    }
    if(phase->warper){
        qtk_stitch_warper_delete(phase->warper);
        phase->warper = NULL;
    }
    if(phase->seamfinder){
        qtk_stitch_seamfinder_delete(phase->seamfinder);
        phase->seamfinder = NULL;
    }
    if(phase->low_cropper){
        qtk_stitch_cropper_delete(phase->low_cropper);
        phase->low_cropper = NULL;
    }
    if(phase->low_warper){
        qtk_stitch_warper_delete(phase->low_warper);
        phase->low_warper = NULL;
    }
    if(phase->humanseg){
        qtk_stitch_humanseg_delete(phase->humanseg);
        phase->humanseg = NULL;
    }
    qtk_stitch_run_phase_queue_clean(phase);
    // wtk_lockhoard_clean(&phase->img_hoard);
    qtk_stitch_run_uninit(phase);
    qtk_stitch_camera_estimator_delete(phase->camera_estimator);
    wtk_free(phase);
    return 0;
}

void qtk_stitch_run_phase_estimate_esposure(qtk_stitch_run_phase_t *phase)
{
    if(phase->cfg->use_compensator){
        _crop_remaps_low_imgs(phase);
        _run_phase_estimate_exposure_errors(phase);
        qtk_stitch_exposure_compensator_resize(phase->compensator,phase->cropper->masks);
        if(phase->cfg->use_compensator_on_single){
            void *gain = qtk_stitch_exposure_compensator_gain_mat_get(phase->compensator);
            qtk_stitch_blender_update_weight_maps(phase->blender,gain);
        }
        qtk_stitch_exposure_compensator_resize2point(phase->compensator,phase->cropper->masks);
    }
    return;
}

int qtk_stitch_run_phase_start(qtk_stitch_run_phase_t *phase)
{
    phase->paper_thread_run = 1;
    wtk_thread_start(&phase->paper_thread);
    if(phase->cfg->use_nthread){
        int i = 0;
        phase->threads_run = 1;
        _pthread_setaffinity(0);
        for(i = 0; i < phase->cfg->camera_num-1; i++){
            wtk_thread_start(phase->the_threads+i);
        }
    }
    qtk_stitch_blender_start(phase->blender,phase->reseam_masks,
                            phase->cropper->corners,phase->cropper->size,phase->cfg->use_nthread);
    if(phase->cfg->use_alpha){
        for(int i = 0; i < phase->cfg->camera_num; i++){
            qtk_stitch_blender_set_rgb(phase->blender,i,phase->cfg->camera_alpha[i*3],
                                phase->cfg->camera_alpha[i*3+1],phase->cfg->camera_alpha[i*3+2],
                                phase->cfg->camera_alpha_x[i*3],phase->cfg->camera_alpha_x[i*3+1],
                                phase->cfg->camera_alpha_x[i*3+2]);
        }
    }
    return 0;
}

int qtk_stitch_run_phase_stop(qtk_stitch_run_phase_t *phase)
{
    phase->paper_thread_run = 0;
    wtk_blockqueue_wake(&phase->paper_bq);
    wtk_thread_join(&phase->paper_thread);
    if(phase->cfg->use_nthread){
        phase->threads_run = 0;
        for(int i = 0; i < phase->cfg->camera_num-1; i++){
            wtk_blockqueue_wake(&phase->the_bqs);
        }
        for(int i = 0; i < phase->cfg->camera_num-1; i++){
            wtk_thread_join(phase->the_threads+i);
        }
    }
    qtk_stitch_blender_stop(phase->blender);
    return 0;
}

void _warpper_thread_wake(qtk_stitch_run_phase_t *phase)
{
    int i = 0;
    qtk_stitch_queue_data_t *data = NULL;
    for(i = 0; i < phase->cfg->camera_num-1; i++){
        data = _stitch_run_queue_data_new(phase);
        data->num = i+1;
        wtk_blockqueue_push(&phase->the_bqs,&data->node);
    }
    return;
}

void _warpper_thread_wait(qtk_stitch_run_phase_t *phase)
{
    int i = 0;
    for(i = 0; i < phase->cfg->camera_num-1; i++){
        wtk_sem_acquire(&phase->the_threads_sem,-1);
    }
    return;
}

void _paper_thread_wait(qtk_stitch_run_phase_t *phase)
{
    wtk_sem_acquire(&phase->the_paper_sem,-1);
    return;
}

void qtk_stitch_run_phase_run(qtk_stitch_run_phase_t *phase)
{
    if(phase->cfg->use_compensator_on_single && phase->cfg->use_compensator && phase->cfg->use_reseammask){
        wtk_debug("error cfg use_compensator_on_single \n");
        return;
    }
    double t = time_get_ms();
    if(phase->cfg->blender_type != QTK_STITCH_BLENDER_FEATHER && phase->cfg->blender_type != QTK_STITCH_BLENDER_NO){
        _run_paper_wake(phase);
    }
    if(phase->cfg->use_nthread == 0){
        _crop_remaps_imgs(phase);   //这个是新的crop+remape的函数
        wtk_debug("%f\n",time_get_ms()-t);
        if(phase->cfg->use_compensator && phase->cfg->use_reseammask == 1){
            _run_phase_compensate_exposure_errors(phase);
            wtk_debug("%f\n",time_get_ms()-t);
        }else if(phase->cfg->use_compensator && phase->cfg->blender_type != QTK_STITCH_BLENDER_FEATHER){
            _run_phase_compensate_exposure_errors(phase);
            wtk_debug("%f\n",time_get_ms()-t);
        }
    }else{
#ifdef QTK_THREAD
        _warpper_thread_wake(phase);
        _crop_remaps_img(phase,0);
#else
        _crop_remaps_imgs(phase);
        wtk_debug("%f\n",time_get_ms()-t);
#endif
        if(phase->cfg->use_compensator && phase->cfg->use_compensator_on_single == 0){
            if(phase->cfg->src_type == QTK_STITCH_USE_IMG_NV12){
                    _run_phase_compensate_exposure_errors(phase);
                    wtk_debug("%f\n",time_get_ms()-t);
            }else{
                _warpper_thread_wake(phase);
                wtk_debug("%f\n",time_get_ms()-t);
                _run_phase_compensate_exposure_error(phase,0);
                wtk_debug("%f\n",time_get_ms()-t);
                _warpper_thread_wait(phase);
                wtk_debug("%f\n",time_get_ms()-t);
            }
        }
    }
    // if(phase->cfg->use_reseammask){
    //     _run_phase_re_charge_seammask(phase);
    // }
    if(phase->cfg->blender_type != QTK_STITCH_BLENDER_FEATHER && phase->cfg->blender_type != QTK_STITCH_BLENDER_NO){
        _paper_thread_wait(phase);
    }else{
        _run_phase_initialize_composition2(phase);
    }
    _run_phase_blend_images(phase,phase->reseam_masks); //parallel_for
    wtk_debug("%f\n",time_get_ms()-t);
    if(phase->cfg->blender_type != QTK_STITCH_BLENDER_FEATHER && phase->cfg->blender_type != QTK_STITCH_BLENDER_NO){
        __run_phase_create_final_panorama(phase); //parallel_for
        wtk_debug("%f\n",time_get_ms()-t);
    }
    if(phase->notify){
        _run_phase_call_notify(phase);
    }
    return;
}

// void qtk_stitch_run_phase_run2(qtk_stitch_run_phase_t *phase)
// {
//     _run_phase_initialize_composition2(phase);
//     qtk_stitch_blender_remap_feather(phase->blender,&phase->img_queue,
//                             phase->reseam_masks,phase->cropper->corners,phase->cropper->crop_offset_maps);
//     return;
// }

int qtk_stitch_run_phase_run_reseammask(qtk_stitch_run_phase_t *phase)
{
    if(phase->cfg->use_reseammask == 0){
        printf("error: no use_reseamask\n");
        return -1;
    }
    int ret = 0;
    if(phase->cfg->use_nthread == 0){
        _crop_remaps_imgs(phase);   //这个是新的crop+remape的函数
        if(phase->cfg->use_compensator){
            _run_phase_compensate_exposure_errors(phase); //parallel_for 暂时不作了
        }
    }else{
#ifdef QTK_THREAD
        _warpper_thread_wake(phase);
        _crop_remaps_img(phase,0);
#else
        _crop_remaps_imgs(phase);
#endif
        if(phase->cfg->src_type == QTK_STITCH_USE_IMG_NV12){
            if(phase->cfg->use_compensator){
                _run_phase_compensate_exposure_errors(phase);
            }
        }else{
            _warpper_thread_wake(phase);
            if(phase->cfg->use_compensator){
                _run_phase_compensate_exposure_error(phase,0);
            }
            _warpper_thread_wait(phase);
        }
    }
    // phase->first = 0;
    if(phase->cfg->use_reseammask){
        ret = _run_phase_re_charge_seammask(phase);
    }
    return ret;
}
//和 reseammask配套使用的
void qtk_stitch_run_phase_run_no_reseammask(qtk_stitch_run_phase_t *phase)
{
    if(phase->cfg->use_reseammask == 0){
        printf("error:reseamask is no\n");
        return;
    }
    double t = time_get_ms();
    if(phase->cfg->blender_type != QTK_STITCH_BLENDER_FEATHER && phase->cfg->blender_type != QTK_STITCH_BLENDER_NO){
        _run_paper_wake(phase);
    }
    if(phase->cfg->use_nthread == 0){
        _crop_remaps_imgs(phase);   //这个是新的crop+remape的函数
        wtk_debug("%f\n",time_get_ms()-t);
        if(phase->cfg->use_compensator){
            _run_phase_compensate_exposure_errors(phase); //parallel_for 暂时不作了
            wtk_debug("%f\n",time_get_ms()-t);
        }
    }else{
#ifdef QTK_THREAD
        _warpper_thread_wake(phase);
        _crop_remaps_img(phase,0);
#else
        _crop_remaps_imgs(phase);
        wtk_debug("%f\n",time_get_ms()-t);
#endif
        if(phase->cfg->src_type == QTK_STITCH_USE_IMG_NV12){
            if(phase->cfg->use_compensator && phase->cfg->use_compensator_on_single == 0){
                _run_phase_compensate_exposure_errors(phase);
                wtk_debug("%f\n",time_get_ms()-t);
            }
        }else{
            _warpper_thread_wake(phase);
            wtk_debug("%f\n",time_get_ms()-t);
            if(phase->cfg->use_compensator && phase->cfg->use_compensator_on_single == 0){
                _run_phase_compensate_exposure_error(phase,0);
                wtk_debug("%f\n",time_get_ms()-t);
            }
            _warpper_thread_wait(phase);
            wtk_debug("%f\n",time_get_ms()-t);
        }
    }
    if(phase->cfg->blender_type != QTK_STITCH_BLENDER_FEATHER && phase->cfg->blender_type != QTK_STITCH_BLENDER_NO){
        _paper_thread_wait(phase);
    }else{
        _run_phase_initialize_composition2(phase);
    }
    _run_phase_blend_images(phase,phase->reseam_masks); //parallel_for
    wtk_debug("%f\n",time_get_ms()-t);
    if(phase->cfg->blender_type != QTK_STITCH_BLENDER_FEATHER && phase->cfg->blender_type != QTK_STITCH_BLENDER_NO){
        __run_phase_create_final_panorama(phase); //parallel_for
        wtk_debug("%f\n",time_get_ms()-t);
    }
    // if(phase->notify){
    //     _run_phase_call_notify(phase);
    // }
    return;
}

void qtk_stitch_run_phase_run_save_overlap(qtk_stitch_run_phase_t *phase,char *outpath)
{
    if(phase->cfg->use_nthread == 0){
        _crop_remaps_imgs(phase);   //这个是新的crop+remape的函数
        if(phase->cfg->use_compensator){
            _run_phase_compensate_exposure_errors(phase); //parallel_for 暂时不作了
        }
    }else{
#ifdef QTK_THREAD
        _warpper_thread_wake(phase);
        _crop_remaps_img(phase,0);
#else
        _crop_remaps_imgs(phase);
#endif
        if(phase->cfg->src_type == QTK_STITCH_USE_IMG_NV12){
            if(phase->cfg->use_compensator){
                _run_phase_compensate_exposure_errors(phase);
            }
        }else{
            _warpper_thread_wake(phase);
            if(phase->cfg->use_compensator){
                _run_phase_compensate_exposure_error(phase,0);
            }
            _warpper_thread_wait(phase);
        }
    }
    qtk_stitch_cropper_overlap_save(phase->cropper,outpath);
}

////////////

void qtk_stitch_run_phase_push_image_file(qtk_stitch_run_phase_t *phase, const char* filename)
{
    // qtk_stitch_queue_data_t* data = wtk_lockhoard_pop(&phase->img_hoard);
    qtk_stitch_queue_data_t* data = _stitch_run_queue_data_new(phase);
    data->img_data = qtk_stitch_image_file_new(filename,phase->cfg->medium_megapix,phase->cfg->low_megapix,-1);
    qtk_stitch_image_resize(data->img_data,QTK_STITCH_IMAGE_RESOLUTION_FINAL);

    wtk_queue_push(&phase->img_queue,&data->node);

    return;
}

void qtk_stitch_run_phase_push_data(qtk_stitch_run_phase_t *phase, float channel, void *img)
{
    // qtk_stitch_queue_data_t* data = wtk_lockhoard_pop(&phase->img_hoard);
    qtk_stitch_queue_data_t* data = _stitch_run_queue_data_new(phase);
    memset(data,0,sizeof(qtk_stitch_queue_data_t));
    if(phase->cfg->use_all_img == 0){
        data->img_data = qtk_stitch_image_new(phase->cfg->h,phase->cfg->w,phase->cfg->medium_megapix,phase->cfg->low_megapix,-1);
    }else{
        data->img_data = qtk_stitch_image_new(phase->cfg->h,phase->cfg->w*phase->cfg->camera_num,phase->cfg->medium_megapix,phase->cfg->low_megapix,-1);
    }
    qtk_stitch_image_todata2(data->img_data,channel,img);
    qtk_stitch_image_set_final(data->img_data);

    wtk_queue_push(&phase->img_queue,&data->node);
    return;
}
//rgb
void qtk_stitch_run_phase_push_low_data(qtk_stitch_run_phase_t *phase, void *img)
{
    // qtk_stitch_queue_data_t* data = wtk_lockhoard_pop(&phase->img_hoard);
    qtk_stitch_queue_data_t* data = _stitch_run_queue_data_new(phase);
    memset(data,0,sizeof(qtk_stitch_queue_data_t));
    int h = 0;
    int w = 0;
    qtk_stitch_mega_pix_scaler_t dscale = {0.0f,};
    qtk_stitch_mega_pix_scaler_init(&dscale,phase->cfg->low_megapix);
    qtk_stitch_set_scale_by_img_size(&dscale,phase->cfg->w,phase->cfg->h);
    qtk_stitch_get_scaled_img_size(&dscale,phase->cfg->w,phase->cfg->h,&w,&h);
    data->img_data = qtk_stitch_image_new(h,w,phase->cfg->medium_megapix,phase->cfg->low_megapix,-1);
    qtk_stitch_image_todata(data->img_data,img);
    qtk_stitch_image_resize(data->img_data,QTK_STITCH_IMAGE_RESOLUTION_FINAL);

    wtk_queue_push(&phase->low_img_queue,&data->node);
    return;
}

//rgba
void qtk_stitch_run_phase_push_low_data_rgba(qtk_stitch_run_phase_t *phase, void *img)
{
    // qtk_stitch_queue_data_t* data = wtk_lockhoard_pop(&phase->img_hoard);
    qtk_stitch_queue_data_t* data = _stitch_run_queue_data_new(phase);
    memset(data,0,sizeof(qtk_stitch_queue_data_t));
    int h = 0;
    int w = 0;
    qtk_stitch_mega_pix_scaler_t dscale = {0.0f,};
    qtk_stitch_mega_pix_scaler_init(&dscale,phase->cfg->low_megapix);
    qtk_stitch_set_scale_by_img_size(&dscale,phase->cfg->w,phase->cfg->h);
    qtk_stitch_get_scaled_img_size(&dscale,phase->cfg->w,phase->cfg->h,&w,&h);
    data->img_data = qtk_stitch_image_new(h,w,phase->cfg->medium_megapix,phase->cfg->low_megapix,-1);
    qtk_stitch_image_todata2(data->img_data,4,img);
    qtk_stitch_image_resize(data->img_data,QTK_STITCH_IMAGE_RESOLUTION_FINAL);

    wtk_queue_push(&phase->low_img_queue,&data->node);
    return;
}

void _warp_parameter(qtk_stitch_run_phase_t* phase,float camera_aspect,int type)
{
    qtk_stitch_warper_create_and_warp_mask(phase->warper,phase->size_w,phase->size_h,phase->camera_estimator->cameras,camera_aspect);
    qtk_stitch_warper_warp_rois(phase->warper,phase->size_w,phase->size_h,phase->camera_estimator->cameras,camera_aspect);
    return;
}

void _warp_low_parameter(qtk_stitch_run_phase_t* phase,float camera_aspect,int type)
{
    qtk_stitch_warper_create_and_warp_mask(phase->low_warper,phase->size_w,phase->size_h,phase->camera_estimator->cameras,camera_aspect);
    qtk_stitch_warper_warp_rois(phase->low_warper,phase->size_w,phase->size_h,phase->camera_estimator->cameras,camera_aspect);
    return;
}


void _warp_final_resolution_parameter(qtk_stitch_run_phase_t *phase,qtk_stitch_image_t *image)
{
    int i = 0;
    int n = phase->cfg->camera_num;
    int *size_w = NULL, *size_h = NULL;
    
    size_w = (int*)wtk_malloc(sizeof(int)*n);
    size_h = (int*)wtk_malloc(sizeof(int)*n);
    for ( i = 0; i < n; i++)
    {
        qtk_stitch_image_get_scaled_img_sizes(image, QTK_STITCH_IMAGE_RESOLUTION_FINAL,
                                            size_w+i, size_h+i);
        // wtk_debug("%d %d\n",size_w[i],size_h[i]);
    }
    if(phase->size_w){
        wtk_free(phase->size_w);
    }
    if(phase->size_h){
        wtk_free(phase->size_h);
    }
    phase->size_w = size_w;
    phase->size_h = size_h;

    phase->camera_aspect = qtk_stitch_image_get_ratio(image,QTK_STITCH_IMAGE_RESOLUTION_MEDIUM,QTK_STITCH_IMAGE_RESOLUTION_FINAL);
    _warp_parameter(phase,phase->camera_aspect,QTK_STITCH_IMAGE_RESOLUTION_FINAL);
    return;
}

void _warp_low_resolution_parameter(qtk_stitch_run_phase_t *phase,qtk_stitch_image_t *image)
{
    int i = 0;
    int n = phase->cfg->camera_num;
    int *size_w = NULL, *size_h = NULL;
    
    size_w = (int*)wtk_malloc(sizeof(int)*n);
    size_h = (int*)wtk_malloc(sizeof(int)*n);
    for ( i = 0; i < n; i++)
    {
        qtk_stitch_image_get_scaled_img_sizes(image, QTK_STITCH_IMAGE_RESOLUTION_LOW,
                                            size_w+i, size_h+i);
    }
    if(phase->size_w){
        wtk_free(phase->size_w);
    }
    if(phase->size_h){
        wtk_free(phase->size_h);
    }
    phase->size_w = size_w;
    phase->size_h = size_h;

    phase->camera_aspect = qtk_stitch_image_get_ratio(image,QTK_STITCH_IMAGE_RESOLUTION_MEDIUM,QTK_STITCH_IMAGE_RESOLUTION_LOW);
    _warp_low_parameter(phase,phase->camera_aspect,QTK_STITCH_IMAGE_RESOLUTION_LOW);
    return;
}

void _crop_final_resolution_parameter(qtk_stitch_run_phase_t *phase,qtk_stitch_image_t *image)
{
    phase->lir_aspect = qtk_stitch_image_get_ratio(image,QTK_STITCH_IMAGE_RESOLUTION_FINAL,QTK_STITCH_IMAGE_RESOLUTION_FINAL);
    // phase->lir_aspect = qtk_stitch_image_get_ratio(image,QTK_STITCH_IMAGE_RESOLUTION_LOW,QTK_STITCH_IMAGE_RESOLUTION_FINAL);
    qtk_stitch_corpper_crop_parameter(phase->cropper,phase->warper->warper_mask,
                phase->warper->roi_corners,phase->warper->roi_sizes, phase->lir_aspect);
    return;
}

void _crop_low_resolution_parameter(qtk_stitch_run_phase_t *phase,qtk_stitch_image_t *image)
{
    phase->lir_aspect = qtk_stitch_image_get_ratio(image,QTK_STITCH_IMAGE_RESOLUTION_FINAL,QTK_STITCH_IMAGE_RESOLUTION_LOW);
    // phase->lir_aspect = qtk_stitch_image_get_ratio(image,QTK_STITCH_IMAGE_RESOLUTION_LOW,QTK_STITCH_IMAGE_RESOLUTION_LOW);
    qtk_stitch_corpper_crop_parameter(phase->low_cropper,phase->low_warper->warper_mask,
                phase->low_warper->roi_corners,phase->low_warper->roi_sizes, phase->lir_aspect);
    return;
}

void _warp_images(qtk_stitch_run_phase_t* stitch,float camera_aspect, int type)
{
    qtk_stitch_warper_warp_images(stitch->warper,&stitch->img_queue,stitch->camera_estimator->cameras,camera_aspect,type);
    return;
}

void _warp_final_resolution_imgs(qtk_stitch_run_phase_t* phase)
{   
    _warp_images(phase,phase->camera_aspect,QTK_STITCH_IMAGE_RESOLUTION_FINAL);
    return;
}

void _warp_images2(qtk_stitch_run_phase_t* stitch,float camera_aspect, int type)
{
    qtk_stitch_warper_warp_images2(stitch->warper,&stitch->img_queue);
    return;
}

void _warp_final_resolution_imgs2(qtk_stitch_run_phase_t* phase)
{   
    _warp_images2(phase,phase->camera_aspect,QTK_STITCH_IMAGE_RESOLUTION_FINAL);
    return;
}

void _warp_final_resolution_img(qtk_stitch_run_phase_t* phase,int index)
{   
    qtk_stitch_warper_warp_image(phase->warper,&phase->img_queue,index);
    return;
}

void _crop_final_resolution_imgs(qtk_stitch_run_phase_t* phase)
{
    // float lir_aspect = 0.0f;
    // qtk_stitch_queue_data_t *image = NULL;
    // wtk_queue_node_t *node = NULL;

    // node = wtk_queue_peek(&phase->img_queue,0);
    // image = data_offset2(node,qtk_stitch_queue_data_t,node);

    // lir_aspect = qtk_stitch_image_get_ratio(image->img_data,QTK_STITCH_IMAGE_RESOLUTION_LOW,QTK_STITCH_IMAGE_RESOLUTION_FINAL);
    qtk_stitch_corpper_crop_imgs(phase->cropper,phase->warper->warper_imgs,phase->lir_aspect);
    return;
}

void _crop_final_resolution_img(qtk_stitch_run_phase_t* phase,int idx)
{
    // float lir_aspect = 0.0f;
    // qtk_stitch_queue_data_t *image = NULL;
    // wtk_queue_node_t *node = NULL;

    // node = wtk_queue_peek(&phase->img_queue,0);
    // image = data_offset2(node,qtk_stitch_queue_data_t,node);

    // lir_aspect = qtk_stitch_image_get_ratio(image->img_data,QTK_STITCH_IMAGE_RESOLUTION_LOW,QTK_STITCH_IMAGE_RESOLUTION_FINAL);
    qtk_stitch_corpper_crop_img(phase->cropper,phase->warper->warper_imgs,phase->lir_aspect,idx);
    return;
}

void _crop_remaps_imgs(qtk_stitch_run_phase_t* phase)
{
    if(phase->cfg->use_all_img == 0){
        qtk_stitch_corpper_crop_remaps2(phase->cropper,&phase->img_queue);
    }else{
        qtk_stitch_corpper_crop_remaps2_all(phase->cropper,&phase->img_queue);
    }
    return;
}

void _crop_remaps_low_imgs(qtk_stitch_run_phase_t* phase)
{
    // qtk_stitch_corpper_crop_remaps(phase->low_cropper,&phase->low_img_queue);
    qtk_stitch_corpper_crop_remaps2(phase->low_cropper,&phase->low_img_queue);
    return;
}

void _crop_remaps_img(qtk_stitch_run_phase_t* phase, int idx)
{
    qtk_stitch_corpper_crop_remap(phase->cropper,&phase->img_queue,idx);
    return;
}

void _run_phase_compensate_exposure_errors(qtk_stitch_run_phase_t* phase)
{
    qtk_stitch_exposure_compensator_apply(phase->compensator,phase->cropper->corners,
                                        phase->cropper->imgs,phase->cropper->masks);
    return;
}

void _run_phase_compensate_exposure_error(qtk_stitch_run_phase_t* phase, int index)
{
    qtk_stitch_exposure_compensator_apply_index(phase->compensator,phase->cropper->corners,
                                        phase->cropper->imgs,phase->cropper->masks,index);
    return;
}

void _run_phase_estimate_exposure_errors(qtk_stitch_run_phase_t* phase)
{
    qtk_stitch_exposure_compensator_feed(phase->compensator,phase->low_cropper->corners,
                        phase->low_cropper->imgs,phase->low_cropper->masks);
    return;
}

void _run_phase_initialize_composition(qtk_stitch_run_phase_t *phase)
{
    qtk_stitch_blender_prepare(phase->blender,phase->cropper->corners,phase->cropper->size);
    return;
}

void _run_phase_initialize_composition2(qtk_stitch_run_phase_t *phase)
{
    qtk_stitch_blender_prepare2(phase->blender,phase->cropper->corners,phase->cropper->size,phase->blend_mem);
    return;
}

void _run_phase_blend_images(qtk_stitch_run_phase_t* phase,void *reseam_masks)
{
    // if(phase->cfg->blender_type != QTK_STITCH_BLENDER_NO){
    //     qtk_stitch_blender_feed(phase->blender,phase->cropper->imgs,reseam_masks,phase->cropper->corners);
    // }else{
    //     qtk_stitch_blender_feed(phase->blender,phase->cropper->imgs,phase->reseam_masks_no,phase->cropper->corners);
    // }
    qtk_stitch_blender_feed(phase->blender,phase->cropper->imgs,reseam_masks,phase->cropper->corners);
    return;
}

void _run_phase_blend_image(qtk_stitch_run_phase_t* phase,void *reseam_masks, int index)
{
    qtk_stitch_blender_feed_index(phase->blender,phase->cropper->imgs,reseam_masks,phase->cropper->corners,index);
    return;
}

void __run_phase_create_final_panorama(qtk_stitch_run_phase_t* phase)
{
    qtk_stitch_blender_blend(phase->blender);
    return;
}

void* qtk_stitch_run_phase_get_end(qtk_stitch_run_phase_t *phase,int *row,int *col)
{
    return qtk_stitch_blender_get_panorama(phase->blender,row,col);
}

void qtk_stitch_run_phase_img_clean(qtk_stitch_run_phase_t* phase)
{
    wtk_queue_node_t *node = NULL;
    qtk_stitch_queue_data_t *data = NULL;
    while(1){
        node = wtk_queue_pop(&phase->img_queue);
        if(node == NULL) break;
        data = data_offset2(node,qtk_stitch_queue_data_t,node);
        qtk_stitch_image_delete(data->img_data);
        data->img_data = NULL;
        // wtk_lockhoard_push(&phase->img_hoard,data);
        _stitch_run_queue_data_delete(data);
    }
    return;
}

void qtk_stitch_run_phase_low_img_clean(qtk_stitch_run_phase_t* phase)
{
    wtk_queue_node_t *node = NULL;
    qtk_stitch_queue_data_t *data = NULL;
    while(1){
        node = wtk_queue_pop(&phase->low_img_queue);
        if(node == NULL) break;
        data = data_offset2(node,qtk_stitch_queue_data_t,node);
        qtk_stitch_image_delete(data->img_data);
        data->img_data = NULL;
        // wtk_lockhoard_push(&phase->img_hoard,data);
        _stitch_run_queue_data_delete(data);
    }
    return;
}

int _run_paper(void *t_data,wtk_thread_t *t)
{
    wtk_queue_node_t *node = NULL;
    qtk_stitch_queue_data_t *data = NULL;
    qtk_stitch_run_phase_t *phase = t_data;

    while(phase->paper_thread_run){
        node = wtk_blockqueue_pop(&phase->paper_bq,-1,NULL);
        if(node == NULL){
            continue;
        }
        data = data_offset2(node,qtk_stitch_queue_data_t,node);
        // wtk_lockhoard_push(&phase->img_hoard,data);
        _run_phase_initialize_composition2(phase);
        wtk_sem_inc(&phase->the_paper_sem);
        _stitch_run_queue_data_delete(data);
    }
    return 0;
}

int _run_paper_wake(qtk_stitch_run_phase_t *phase)
{
    // wtk_queue_node_t *node = NULL;
    qtk_stitch_queue_data_t *data = NULL;
    if(phase->paper_thread_run){
        // node = wtk_lockhoard_pop(&phase->img_hoard);
        data = _stitch_run_queue_data_new(phase);
        wtk_blockqueue_push(&phase->paper_bq,&data->node);
    }
    return 0;
}

int _run_all_warper(void *t_data,wtk_thread_t *t)
{
    wtk_queue_node_t *node = NULL;
    qtk_stitch_queue_data_t *data = NULL;
    _warper_thread_use_t *userdata = t_data;
    qtk_stitch_run_phase_t *phase = userdata->userdata;

    _pthread_setaffinity(userdata->num);
    while(phase->threads_run){
        node = wtk_blockqueue_pop(&phase->the_bqs,-1,NULL);
        if(node == NULL){
            continue;
        }
        data = data_offset2(node,qtk_stitch_queue_data_t,node);
#ifdef QTK_THREAD
        _crop_remaps_img(phase,data->num);
#endif
        if(phase->cfg->use_compensator && phase->cfg->use_compensator_on_single == 0){
            _run_phase_compensate_exposure_error(phase,data->num);
        }
        wtk_sem_inc(&phase->the_threads_sem);
        _stitch_run_queue_data_delete(data);
    }
    wtk_free(userdata);
    return 0;
}

void _pthread_setaffinity(int n)
{
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    CPU_SET(n, &cpu_set);
    if(pthread_setaffinity_np(pthread_self(), sizeof(cpu_set), &cpu_set) < 0)
        perror("pthread_setaffinity_np");
    return;
}

void qtk_stitch_run_phase_set_blend_mem(qtk_stitch_run_phase_t *phase,void *ptr)
{
    phase->blend_mem = ptr;
    return;
}

void qtk_stitch_run_phase_get_rect(qtk_stitch_run_phase_t *phase,int *w, int *h)
{
    int blend_w = 0;
    int blend_h = 0;
    qtk_stitch_blender_get_rect(phase->blender,phase->cropper->corners,phase->cropper->size,&blend_h,&blend_w);
    *w = blend_w;
    *h = blend_h;
    return;
}

void qtk_stitch_run_phase_set_rgb(qtk_stitch_run_phase_t *phase,int idx, char alpha,char beta,char gamma,
                    float alpha_x,float beta_x,float gamma_x)
{
    qtk_stitch_blender_set_rgb(phase->blender,idx,alpha,
        beta,gamma,alpha_x,beta_x,gamma_x);
    return;   
}

void qtk_stitch_run_phase_set_blender_smooth(qtk_stitch_run_phase_t *phase,float smooth)
{
    qtk_stitch_blender_set_smooth(phase->blender,smooth);
    return;
}

void qtk_stitch_run_phase_get_low_size(qtk_stitch_run_phase_t *phase,int *w, int *h)
{
    qtk_stitch_image_t *image = NULL;
    image = qtk_stitch_image_new(phase->cfg->h,phase->cfg->w,phase->cfg->medium_megapix,phase->cfg->low_megapix,-1);
    qtk_stitch_image_get_scaled_img_sizes(image, QTK_STITCH_IMAGE_RESOLUTION_LOW,
                                            w, h);
    qtk_stitch_image_delete(image);
}

//在第几张照片上的坐标
void qtk_stitch_run_phase_set_seammask_region(qtk_stitch_run_phase_t *phase, int x,int y,int w,int h, int idx)
{
    // if(phase->cfg->blender_type == QTK_STITCH_BLENDER_NO){
    //     qtk_stitch_seamfinder_seam_masks_range(phase->reseam_masks_no,phase->cropper->corners,x,y,w,h,idx);
    // }
    return;
}

int qtk_stitch_run_phase_set_notify(qtk_stitch_run_phase_t *phase, void *userdata, qtk_stitch_run_phase_notify_f notify)
{
    phase->userdata = userdata;
    phase->notify = notify;
    return 0;
}

int _run_phase_call_notify(qtk_stitch_run_phase_t *phase)
{
    int w;
    int h;
    void *p = NULL;
    p = qtk_stitch_cropper_get_image(phase->cropper,&w,&h,1);
    if(phase->notify){
        phase->notify(phase->userdata,p,w,h,3);
    }
    return 0;
}

//以评分的形式评价seammask
int _run_phase_re_charge_seamask_scores(qtk_stitch_run_phase_t *phase, u_char *update_list)
{
    float threshold = phase->cfg->seammask_threshold;
    int ncamera_num = phase->cfg->camera_num-1;
    int split_num = phase->cfg->reseammask_split_num;
    float *seammask_score_tmp = NULL;
    float *seammask_score_src_tmp = NULL;
    int i = 0,j = 0;
    int update = 0,update_src = 0;
    int update_score_num = 0;

    seammask_score_tmp = wtk_malloc(sizeof(float)*ncamera_num*split_num*2);
    for(i = 0; i < ncamera_num*split_num*2; ++i){
        seammask_score_tmp[i] = 100.0f;
    }
    seammask_score_src_tmp = seammask_score_tmp+(ncamera_num*split_num);
    qtk_stitch_seamfinder_seammasks_scores_nsp(phase->seamfinder,phase->cropper->imgs,phase->reseam_masks_low,
                        phase->seammask_points,seammask_score_tmp,phase->cfg->reseammask_point_num,split_num);
    if(phase->first == 0){
        qtk_stitch_seamfinder_seammasks_scores_nsp(phase->seamfinder,phase->cropper->imgs,phase->reseam_masks_src_low,
                            phase->seammask_points_src,seammask_score_src_tmp,phase->cfg->reseammask_point_num,split_num);
    }
    if(phase->first){
        phase->first = 0;
        for(i = 0; i < phase->cfg->camera_num - 1; ++i){
            for(j = 0; j < split_num; ++j){
                phase->seammask_score[i*split_num+j] = seammask_score_tmp[i*split_num+j];
            }
        }
        goto end;
    }
    //判断是什么情况
    update_score_num = 0;
    for(i = 0; i < phase->cfg->camera_num-1; ++i){
        for(j = 0; j < split_num; ++j){
            int k = i*split_num+j;
            if(fabs(seammask_score_tmp[k]-phase->seammask_score[k]) > threshold){
                update_list[i] = 1;
                ++update_score_num;
                update = 1;
            }
        }
    }
    if(update == 1){
        if(update_score_num == (phase->cfg->camera_num-1)*split_num){ //这个时候全部更新
            update = 1;
        }else{ //局部更新
            update = 2;
        }
    }else{
        goto end;
    }
    // if(update == 1 || update == 2){ //都会更新分数
    //     memcpy(phase->seammask_score,seammask_score_tmp,sizeof(float)*ncamera_num*split_num);
    // }
    //比较标定的缝和现在的缝的分数
    if(update == 1){
        update_src = 1;
        for(i = 0; i < phase->cfg->camera_num-1; ++i){
            for(j = 0; j < split_num; ++j){
                int k = i*split_num+j;
                if(seammask_score_src_tmp[k] <= seammask_score_tmp[k]){ //每一段拼接缝都比现在好才可以转为原来的
                    update_src = 0;
                }
            }
            if(update_src == 0){
                break;
            }
        }
    }
    if(update_src == 1){
        update = 3;
        memcpy(phase->seammask_score,seammask_score_src_tmp,sizeof(float)*ncamera_num*split_num);
    }
end:
    if(seammask_score_tmp) wtk_free(seammask_score_tmp);
    return update;
}

int _run_phase_re_charge_seamask_humanseg(qtk_stitch_run_phase_t *phase, u_char *update_list)
{
    int update = 0;
    int ncamera_num = phase->cfg->camera_num-1;
    int split_num = phase->cfg->reseammask_split_num;
    int *coress = NULL;
    int i = 0, j = 0;
    int update_num = 0;

    qtk_stitch_humanseg_get_humanseg(phase->humanseg,phase->low_cropper->corners,
                                phase->low_cropper->masks,phase->cropper->imgs);
    coress = wtk_malloc(sizeof(int)*ncamera_num*split_num);
    memset(coress,0,sizeof(int)*ncamera_num*split_num);
    qtk_stitch_humanseg_seammask_cross(phase->humanseg,phase->seammask_points,
                                phase->cfg->reseammask_point_num,3,split_num,coress);
    //进行判断是否需要更改
    for(i = 0; i < ncamera_num; ++i) {
        for(j = 0; j < split_num; ++j){
            if(coress[i*split_num+j] == 1 && qtk_stitch_humanseg_mask_connect(phase->humanseg,split_num,i,j) == 1){
                update_list[i*split_num+j] = 1;
                update_num += 1;
            }
        }
    }
    if(update_num == ncamera_num*split_num){
        update = 1;
        phase->update_count = 0;
    }else if(update_num > 0){
        update = 2;
        phase->update_count = 0;
    }else{
        phase->update_count += 1;
        if(phase->update_count == 10){
            update = 3;
        }
    }
    wtk_free(coress);
    return update;
}

int _run_phase_re_charge_seammask(qtk_stitch_run_phase_t *phase)
{
    int update = 0;
    u_char *update_list = NULL;
    int ret = 0;
    int i = 0;
    float *seammask_score_tmp = NULL;
    int split_num = phase->cfg->reseammask_split_num;
    int ncamera_num = phase->cfg->camera_num-1;

    update_list = wtk_malloc(sizeof(u_char)*(ncamera_num)*split_num);
    memset(update_list,0,sizeof(u_char)*(ncamera_num)*split_num);
    if(phase->cfg->use_humanseg == 0){
        update = _run_phase_re_charge_seamask_scores(phase,update_list);
    }else{ //这边使用人像分割进行计算
        update = _run_phase_re_charge_seamask_humanseg(phase,update_list);
    }

    // update = 2;
    // u_char up[] = {0,0,0,1};
    // memcpy(update_list,up,4);
    wtk_debug("update %d\n",update);
    for(i = 0; i < ncamera_num; ++i){
        int j = 0;
        printf("block ");
        for(j = 0; j < split_num; ++j){
            printf("%d ",update_list[i*split_num+j]);
        }
    }
    printf("\n");
    if(update == 3){
        qtk_stitch_seamfinder_seam_masks_delete(phase->reseam_masks_low);
        //更新拼接缝
        phase->reseam_masks_low = qtk_stitch_seamfinder_seammasks_copy(phase->reseam_masks_src_low);
        //更新拼接缝的点
        memcpy(phase->seammask_points,phase->seammask_points_src,sizeof(int)*phase->cfg->reseammask_point_num*ncamera_num);
        qtk_stitch_seamfinder_seammasks_low2final(phase->cropper->masks,phase->reseam_masks,phase->reseam_masks_low);
        qtk_stitch_blender_stop(phase->blender);
        qtk_stitch_blender_start(phase->blender,phase->reseam_masks,phase->cropper->corners,phase->cropper->size,phase->cfg->use_nthread);
        ret = 1;
    }else if(update == 1){
        qtk_stitch_seamfinder_seam_masks_delete(phase->reseam_masks_low);
        phase->reseam_masks_low = qtk_stitch_seamfinder_find2(phase->seamfinder,phase->low_cropper->corners,phase->cropper->imgs,
                                        phase->low_cropper->masks,phase->seammask_points,phase->cfg->reseammask_point_num);
        //重新找点
        qtk_stitch_seamfinder_seammasks_find_point(phase->reseam_masks_low,phase->cfg->reseammask_point_num,phase->seammask_points,1); //灰度图
        qtk_stitch_seamfinder_seammasks_low2final(phase->cropper->masks,phase->reseam_masks,phase->reseam_masks_low);
        qtk_stitch_blender_stop(phase->blender);
        qtk_stitch_blender_start(phase->blender,phase->reseam_masks,phase->cropper->corners,phase->cropper->size,phase->cfg->use_nthread);
        ret = 1;
    }else if(update == 2){
        u_char* update_list_p = update_list;
        for(i = 0; i < phase->cfg->camera_num-1;++i){
            _re_charge_seammask_section(phase,i,update_list_p);
            update_list_p+=split_num;
        }
        qtk_stitch_seamfinder_seammasks_find_point(phase->reseam_masks_low,phase->cfg->reseammask_point_num,phase->seammask_points,1); //灰度图
        qtk_stitch_seamfinder_seammasks_low2final(phase->cropper->masks,phase->reseam_masks,phase->reseam_masks_low);
        qtk_stitch_blender_stop(phase->blender);
        qtk_stitch_blender_start(phase->blender,phase->reseam_masks,phase->cropper->corners,phase->cropper->size,phase->cfg->use_nthread);
        ret = 1;
    }
    //重新算分
    if(phase->cfg->use_humanseg == 0 && update > 0 && update != 3){
        seammask_score_tmp = wtk_malloc(sizeof(float)*ncamera_num*split_num*2);
        qtk_stitch_seamfinder_seammasks_scores_nsp(phase->seamfinder,phase->cropper->imgs,phase->reseam_masks_low,
            phase->seammask_points,seammask_score_tmp,phase->cfg->reseammask_point_num,split_num);
        memcpy(phase->seammask_score,seammask_score_tmp,sizeof(float)*ncamera_num*split_num);
    }
    
    if(ret == 1 && phase->cfg->use_compensator && phase->cfg->use_compensator_on_single){
        void *gain = qtk_stitch_exposure_compensator_gain_mat_get(phase->compensator);
        qtk_stitch_blender_update_weight_maps(phase->blender,gain);
    }

    if(phase->cfg->use_humanseg){
        qtk_stitch_humanseg_clear_humanseg(phase->humanseg);
    }

    if(update_list) wtk_free(update_list);
    if(seammask_score_tmp) wtk_free(seammask_score_tmp);
    return ret;
}

void _re_charge_seammask_section(qtk_stitch_run_phase_t *phase,int idx, u_char *update_list)
{
    int split_num = phase->cfg->reseammask_split_num;
    int i = 0;
    int start = -1;
    for(i = 0,start = -1; i < split_num; i++){
        if(update_list[i] == 1){
            if(start < 0){
                start = i;
            }
        }else if(start >= 0 && update_list[i] == 0){
            wtk_debug("process seammask block %d %d\n",idx,start);
            // if(phase->cfg->use_humanseg == 0){
            qtk_stitch_seamfinder_findblock_process(phase->seamfinder,phase->reseam_masks_low,phase->low_cropper->corners,
                    phase->cropper->imgs,phase->low_cropper->masks,phase->seammask_points,
                    phase->cfg->reseammask_point_num,idx,split_num,start,i);
            // }else{
            //     qtk_stitch_seamfinder_humanseg_findblock_process(phase->seamfinder,phase->reseam_masks_low,phase->low_cropper->corners,
            //             phase->cropper->imgs,phase->low_cropper->masks,phase->humanseg->humansegs,phase->seammask_points,
            //             phase->cfg->reseammask_point_num,idx,split_num,start,i);
            // }
            start = -1;
        }
        // printf("i:%d start:%d\n",i,start);
    }
    if(start > 0){
        wtk_debug("process seammask block %d %d\n",idx,start);
        // if(phase->cfg->use_humanseg == 0){
        qtk_stitch_seamfinder_findblock_process(phase->seamfinder,phase->reseam_masks_low,phase->low_cropper->corners,
            phase->cropper->imgs,phase->low_cropper->masks,phase->seammask_points,phase->cfg->reseammask_point_num,idx,
            split_num,start,i);
        // }else{
        //     qtk_stitch_seamfinder_humanseg_findblock_process(phase->seamfinder,phase->reseam_masks_low,phase->low_cropper->corners,
        //         phase->cropper->imgs,phase->low_cropper->masks,phase->humanseg->humansegs,phase->seammask_points,
        //         phase->cfg->reseammask_point_num,idx,split_num,start,i);
        // }
    }
    if(start == 0){ //单缝全部更新
        printf("process seammask %d\n",idx);
        // if(phase->cfg->use_humanseg == 0){
        qtk_stitch_seamfinder_find3(phase->seamfinder,phase->reseam_masks_low,phase->low_cropper->corners,
            phase->cropper->imgs,phase->low_cropper->masks,phase->seammask_points,phase->cfg->reseammask_point_num,idx);
        // }else{
        //     qtk_stitch_seamfinder_humanseg_find3(phase->seamfinder,phase->reseam_masks_low,phase->low_cropper->corners,
        //         phase->cropper->imgs,phase->low_cropper->masks,phase->humanseg->humansegs,phase->seammask_points,phase->cfg->reseammask_point_num,idx);
        // }
    }
    return;
}