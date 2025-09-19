#include "qtk_stitch_run_cfg.h"

int qtk_stitch_run_cfg_init(qtk_stitch_run_cfg_t *cfg)
{
    memset(cfg,0,sizeof(qtk_stitch_run_cfg_t));
    cfg->camera_num = 0;
    cfg->cameras_cfg = NULL;
    cfg->seammasks_cfg = NULL;
    
    cfg->estimator = QTK_STITCH_CAMERA_ESTIMATOR_HOMOGRAPHY;
    cfg->warper_type = QTK_STITCH_WARPER_SPHERICAL;
    cfg->crop = 1;
    cfg->compensator = QTK_STITCH_EXPOSURE_GAIN_BLOCK;
    cfg->nr_feeds = 1;
    cfg->block_size = 32;
    cfg->blender_type = QTK_STITCH_BLENDER_MULTIBAND;
    cfg->blend_strength = 5;
    cfg->use_gpu = 0;

    cfg->w = 1280;
    cfg->h = 720;
    cfg->use_nthread = 0;
    cfg->medium_megapix = 0.6;
    cfg->low_megapix = 0.5;
    cfg->remap_type = 0;
    cfg->use_alpha = 0;
    cfg->camera_alpha = NULL;
    cfg->camera_alpha_x = NULL;
    cfg->use_compensator = 0;
    cfg->smooth = 0.0f;
    cfg->use_all_img = 0;
    cfg->src_type = 0;
    cfg->use_reseammask = 0;
    cfg->seammask_threshold = 0.02;
    cfg->reseammask_split_num = 2;
    cfg->use_shift_seam_position = 0;
    cfg->shift_seam_offsets = NULL;
    cfg->left_full = 30;
    cfg->right_full = 30;
    cfg->use_compensator_on_single = 1;
    cfg->use_humanseg = 0;
    qtk_stitch_humanseg_cfg_init(&cfg->humanseg_cfg);
    return 0;
}

int qtk_stitch_run_cfg_clean(qtk_stitch_run_cfg_t *cfg)
{
    int i = 0;
    if(cfg->cameras_cfg){
        for(i = 0; i < cfg->camera_num; i++){
            qtk_stitch_run_camera_cfg_clean(cfg->cameras_cfg+i);
        }
        wtk_free(cfg->cameras_cfg);
    }
    if(cfg->seammasks_cfg){
        for(i = 0; i < cfg->camera_num; i++){
            qtk_stitch_run_seammask_cfg_clean(cfg->seammasks_cfg+i);
        }
        wtk_free(cfg->seammasks_cfg);
    }
    if(cfg->intersection){
        for(i = 0; i < cfg->camera_num; i++){
            qtk_stitch_run_rectangle_cfg_clean(cfg->intersection+i);
        }
        wtk_free(cfg->intersection);
    }
    if(cfg->overlapping){
        for(i = 0; i < cfg->camera_num; i++){
            qtk_stitch_run_rectangle_cfg_clean(cfg->overlapping+i);
        }
        wtk_free(cfg->overlapping);
    }
    qtk_stitch_humanseg_cfg_clean(&cfg->humanseg_cfg);
    cfg->camera_num = 0;
    return 0;
}

int qtk_stitch_run_cfg_update_local(qtk_stitch_run_cfg_t *cfg,wtk_local_cfg_t *main_lc)
{
    char *key = NULL;
    wtk_string_t *v = NULL;
    int i = 0;
    wtk_local_cfg_t *lc = NULL;

    key = wtk_malloc(125);
    
    wtk_local_cfg_update_cfg_i(main_lc,cfg,camera_num,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,w,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,h,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,blender_type,v);
    wtk_local_cfg_update_cfg_f(main_lc,cfg,blend_strength,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,compensator,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,nr_feeds,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,block_size,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,use_gpu,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,use_nthread,v);
    wtk_local_cfg_update_cfg_f(main_lc,cfg,medium_megapix,v);
    wtk_local_cfg_update_cfg_f(main_lc,cfg,low_megapix,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,warper_type,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,remap_type,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,use_alpha,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,use_compensator,v);
    wtk_local_cfg_update_cfg_f(main_lc,cfg,smooth,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,use_all_img,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,src_type,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,use_reseammask,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,reseammask_point_num,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,finder,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,reseammask_split_num,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,use_shift_seam_position,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,left_full,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,right_full,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,use_compensator_on_single,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,use_humanseg,v);

    if(cfg->camera_num){
        cfg->cameras_cfg = wtk_malloc(sizeof(qtk_stitch_run_camera_cfg_t)*cfg->camera_num);
        for(i = 0; i < cfg->camera_num; i++){
            sprintf(key,"camera%d",i);
            lc = wtk_local_cfg_find_lc(main_lc,key,strlen(key));
            if(lc){
                qtk_stitch_run_camera_cfg_update_local(cfg->cameras_cfg+i,lc);
            }
        }
        cfg->seammasks_cfg = wtk_malloc(sizeof(qtk_stitch_run_seammask_cfg_t)*cfg->camera_num);
        for(i = 0; i < cfg->camera_num; i++){
            sprintf(key,"seam_mask%d",i);
            lc = wtk_local_cfg_find_lc(main_lc,key,strlen(key));
            if(lc){
                qtk_stitch_run_seammask_cfg_update_local(cfg->seammasks_cfg+i,lc);
            }
        }
        cfg->intersection = wtk_malloc(sizeof(qtk_stitch_run_rectangle_cfg_t)*cfg->camera_num);
        for(i = 0; i < cfg->camera_num; i++){
            sprintf(key,"intersection%d",i);
            lc = wtk_local_cfg_find_lc(main_lc,key,strlen(key));
            if(lc){
                qtk_stitch_run_rectangle_cfg_update_local(cfg->intersection+i,lc);
            }
        }
        cfg->overlapping = wtk_malloc(sizeof(qtk_stitch_run_rectangle_cfg_t)*cfg->camera_num);
        for(i = 0; i < cfg->camera_num; i++){
            sprintf(key,"overlapping%d",i);
            lc = wtk_local_cfg_find_lc(main_lc,key,strlen(key));
            if(lc){
                qtk_stitch_run_rectangle_cfg_update_local(cfg->overlapping+i,lc);
            }
        }
    }
    if(cfg->use_alpha){
        wtk_array_t *arr = wtk_local_cfg_find_int_array_s(main_lc,"camera_alpha");
        if(arr){
            if(arr->nslot != cfg->camera_num * 3){
                wtk_debug("camera alpha num error\n");
            }else{
                cfg->camera_alpha = (int*)arr->slot;
            }
        }
        arr = wtk_local_cfg_find_float_array_s(main_lc,"camera_alpha_x");
        if(arr){
            if(arr->nslot != cfg->camera_num * 3){
                wtk_debug("camera alpha num error\n");
            }else{
                cfg->camera_alpha_x = (float*)arr->slot;
            }
        }
    }
    if(cfg->use_shift_seam_position){
        wtk_array_t *arr = wtk_local_cfg_find_int_array_s(main_lc,"shift_seam_offsets");
        if(arr){
            if(arr->nslot != cfg->camera_num * 2){
                wtk_debug("shift seam offset error\n");
            }else{
                cfg->shift_seam_offsets = (int*)arr->slot;
            }
        }
    }
    wtk_free(key);

    lc = wtk_local_cfg_find_lc_s(main_lc,"humanseg");
    if(lc){
        qtk_stitch_humanseg_cfg_update_local(&cfg->humanseg_cfg,lc);
    }
    return 0;
}

int qtk_stitch_run_cfg_update(qtk_stitch_run_cfg_t *cfg)
{
    qtk_stitch_humanseg_cfg_update(&cfg->humanseg_cfg);
    return 0;
}