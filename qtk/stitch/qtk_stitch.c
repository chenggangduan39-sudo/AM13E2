#include "qtk_stitch.h"
#include "wtk/core/wtk_alloc.h"
#include "image/qtk_stitch_image.h"

void qtk_stitch_queue_clean(qtk_stitch_t* stitch);
void _stitch_camera_choose(qtk_stitch_t *stitch);
//data process
qtk_stitch_queue_data_t *qtk_stitch_queue_data_new(void *data)
{
    qtk_stitch_queue_data_t *qdata = (qtk_stitch_queue_data_t *)wtk_malloc(sizeof(qtk_stitch_queue_data_t));
    memset(qdata,0,sizeof(qtk_stitch_queue_data_t));
    return qdata;
}

int qtk_stitch_queue_data_delete(void *data)
{
    qtk_stitch_queue_data_t *qdata = (qtk_stitch_queue_data_t*)data;
    wtk_free(qdata);
    return 0;
}

qtk_stitch_t* qtk_stitch_new(qtk_stitch_cfg_t *cfg)
{
	qtk_stitch_t* stitch = (qtk_stitch_t*)wtk_malloc(sizeof(qtk_stitch_t));
    memset(stitch, 0, sizeof(qtk_stitch_t));
    stitch->cfg = cfg;
    stitch->try_use_gpu = cfg->try_use_gpu;

    stitch->detector = qtk_stitch_feature_detector_new(cfg->detector,cfg->nfeatures);
    stitch->match_conf = qtk_stitch_feature_get_match_conf(cfg->match_conf,cfg->detector);
    stitch->matcher = qtk_stitch_feature_matcher_new(cfg->matcher_type,cfg->range_width,
                                                    stitch->match_conf);
    // stitch->subsetter = qtk_stitch_subsetter_new(cfg->confidence_threshold,cfg->matches_graph_dot_file.data);
    stitch->camera_estimator = qtk_stitch_camera_estimator_new(cfg->estimator);
    stitch->camera_estimator2 = qtk_stitch_camera_estimator_new(cfg->estimator);
    stitch->camera_estimator_tmp = qtk_stitch_camera_estimator_new(cfg->estimator);
    stitch->camera_adjuster = qtk_stitch_camera_adjuster_new(cfg->adjuster,cfg->refinement_mask.data,
                                                              cfg->confidence_threshold);
    stitch->wave_corrector = qtk_stitch_wave_corrector_new(cfg->wave_correct_kind);
    stitch->warper = qtk_stitch_warper_new(cfg->warper_type,cfg->num_camera);
    stitch->cropper = qtk_stitch_cropper_new(cfg->crop,cfg->num_camera,1,0);
    stitch->compensator = qtk_stitch_exposure_compensator_new(cfg->compensator,
                                                        cfg->nr_feeds,cfg->block_size,
                                                        cfg->num_camera,QTK_STITCH_USE_IMG_RGB);
    stitch->seam_finder = qtk_stitch_seamfinder_new(cfg->finder,QTK_STITCH_USE_IMG_RGB,0,0);
    stitch->blender = qtk_stitch_blender_new(cfg->blender_type,cfg->blend_strength,cfg->try_use_gpu,0,0,0);
    stitch->timelapser = qtk_stitch_timelapser_new(cfg->timelapse,cfg->timelapse_prefix.data);

    wtk_queue_init(&stitch->img_queue);
    wtk_lockhoard_init(&stitch->img_hoard,offsetof(qtk_stitch_queue_data_t,node),
                    10,(wtk_new_handler_t)qtk_stitch_queue_data_new,
                    (wtk_delete_handler_t)qtk_stitch_queue_data_delete,stitch);

    return stitch;
}

void qtk_stitch_push_image_file(qtk_stitch_t* stitch, const char* filename)
{
    qtk_stitch_queue_data_t* data = wtk_lockhoard_pop(&stitch->img_hoard);
    data->img_data = qtk_stitch_image_file_new(filename,stitch->cfg->medium_megapix,stitch->cfg->low_megapix,-1);
    qtk_stitch_image_resize(data->img_data,QTK_STITCH_IMAGE_RESOLUTION_MEDIUM);
    qtk_stitch_image_resize(data->img_data,QTK_STITCH_IMAGE_RESOLUTION_LOW);
    qtk_stitch_image_resize(data->img_data,QTK_STITCH_IMAGE_RESOLUTION_FINAL);
    double t = time_get_ms();
    data->feature_data = qtk_stitch_feature_detector_detect(stitch->detector,data->img_data);
    printf("feature detect cost %lf ms\n",time_get_ms()-t);

    wtk_queue_push(&stitch->img_queue,&data->node);

    return;
}

void _match_features(qtk_stitch_t* stitch)
{
    qtk_stitch_feature_matcher_match_feature(stitch->matcher,&stitch->img_queue);
    return;
}

//一个排序 顺序输入可以不作
void _subset(qtk_stitch_t* stitch)
{
    // qtk_stitch_subsetter_subset(stitch->subsetter,&stitch->img_queue,stitch->matcher->pairwise_matches);
    return;
}
//估计相机参数
void _estimate_camera_parameters(qtk_stitch_t* stitch)
{
    qtk_stitch_camera_estimator_estimate(stitch->camera_estimator,&stitch->img_queue,stitch->matcher->pairwise_matches);
    return;
}

//估计备用相机参数
void _estimate_camera_parameters2(qtk_stitch_t* stitch)
{
    qtk_stitch_camera_estimator_estimate(stitch->camera_estimator2,&stitch->img_queue,stitch->matcher->pairwise_matches);
    return;
}

//优化相机参数
void _refine_camera_parameters(qtk_stitch_t* stitch)
{
    qtk_stitch_camera_adjuster_adjust(stitch->camera_adjuster,&stitch->img_queue,
                    stitch->matcher->pairwise_matches,stitch->camera_estimator->cameras);
    return;
}

//波浪校正
void _perform_wave_correction(qtk_stitch_t* stitch)
{
    qtk_stitch_wave_corrector_correct(stitch->wave_corrector,stitch->camera_estimator->cameras);
    return;
}
//设置变量
void _estimate_scale(qtk_stitch_t* stitch)
{
    qtk_stitch_warper_set_scale(stitch->warper,stitch->camera_estimator->cameras);
    return;
}

void _warp(qtk_stitch_t* stitch,float camera_aspect,int type)
{
    qtk_stitch_warper_warp_images(stitch->warper,&stitch->img_queue,stitch->camera_estimator->cameras,camera_aspect,type);
    qtk_stitch_warper_create_and_warp_mask(stitch->warper,stitch->size_w,stitch->size_h,stitch->camera_estimator->cameras,camera_aspect);
    qtk_stitch_warper_warp_rois(stitch->warper,stitch->size_w,stitch->size_h,stitch->camera_estimator->cameras,camera_aspect);
    return;
}

void _warp_low_resolution(qtk_stitch_t* stitch)
{
    int i = 0;
    int n = stitch->img_queue.length;
    wtk_queue_node_t *node = NULL;
    qtk_stitch_queue_data_t *image = NULL;
    int *size_w = NULL, *size_h = NULL;
    float camera_aspect = 0.0f;
    
    size_w = (int*)wtk_malloc(sizeof(int)*n); //不需要频繁申请  到时可以优化
    size_h = (int*)wtk_malloc(sizeof(int)*n);
    for ( i = 0; i < n; i++)
    {
        node = wtk_queue_peek(&stitch->img_queue,i);
        image = data_offset2(node,qtk_stitch_queue_data_t,node);
        qtk_stitch_image_get_scaled_img_sizes(image->img_data, QTK_STITCH_IMAGE_RESOLUTION_LOW, 
                                            size_w+i, size_h+i);
        // wtk_debug("%d %d\n",size_w[i],size_h[i]);
    }
    if(stitch->size_w){
        wtk_free(stitch->size_w);
    }
    if(stitch->size_h){
        wtk_free(stitch->size_h);
    }
    stitch->size_w = size_w;
    stitch->size_h = size_h;
    camera_aspect = qtk_stitch_image_get_ratio(image->img_data,QTK_STITCH_IMAGE_RESOLUTION_MEDIUM,QTK_STITCH_IMAGE_RESOLUTION_LOW);
    _warp(stitch,camera_aspect,QTK_STITCH_IMAGE_RESOLUTION_LOW);
    return;
}

void _prepare_cropper(qtk_stitch_t* stitch)
{
    qtk_stitch_cropper_prepare(stitch->cropper,stitch->warper->warper_imgs,
                stitch->warper->warper_mask,stitch->warper->roi_corners,stitch->warper->roi_sizes);
    return;
}


void _crop_low_resolution(qtk_stitch_t* stitch)
{
    qtk_stitch_corpper_crop(stitch->cropper,stitch->warper->warper_imgs,
                stitch->warper->warper_mask,stitch->warper->roi_corners,stitch->warper->roi_sizes,1.0f);
    return;
}

void _estimate_exposure_errors(qtk_stitch_t* stitch)
{
    qtk_stitch_exposure_compensator_feed(stitch->compensator,stitch->cropper->corners,
                        stitch->cropper->imgs,stitch->cropper->masks);
    return;
}

void* _find_seam_masks(qtk_stitch_t* stitch)
{
    void *ret = qtk_stitch_seamfinder_find(stitch->seam_finder,stitch->cropper->corners,
                            stitch->cropper->imgs,stitch->cropper->masks);
    return ret;
}

void _warp_final_resolution(qtk_stitch_t* stitch)
{
    int i = 0;
    int n = stitch->img_queue.length;
    wtk_queue_node_t *node = NULL;
    qtk_stitch_queue_data_t *image = NULL;
    int *size_w = NULL, *size_h = NULL;
    float camera_aspect = 0.0f;
    
    size_w = (int*)wtk_malloc(sizeof(int)*n);
    size_h = (int*)wtk_malloc(sizeof(int)*n);
    for ( i = 0; i < n; i++)
    {
        node = wtk_queue_peek(&stitch->img_queue,i);
        image = data_offset2(node,qtk_stitch_queue_data_t,node);
        qtk_stitch_image_get_scaled_img_sizes(image->img_data, QTK_STITCH_IMAGE_RESOLUTION_FINAL,
                                            size_w+i, size_h+i);
        // wtk_debug("%d %d\n",size_w[i],size_h[i]);
    }
    if(stitch->size_w){
        wtk_free(stitch->size_w);
    }
    if(stitch->size_h){
        wtk_free(stitch->size_h);
    }
    stitch->size_w = size_w;
    stitch->size_h = size_h;

    camera_aspect = qtk_stitch_image_get_ratio(image->img_data,QTK_STITCH_IMAGE_RESOLUTION_MEDIUM,QTK_STITCH_IMAGE_RESOLUTION_FINAL);
    _warp(stitch,camera_aspect,QTK_STITCH_IMAGE_RESOLUTION_FINAL);
    return;
}

void _crop_final_resolution(qtk_stitch_t* stitch)
{
    float lir_aspect = 0.0f;
    qtk_stitch_queue_data_t *image = NULL;
    wtk_queue_node_t *node = NULL;

    node = wtk_queue_peek(&stitch->img_queue,0);
    image = data_offset2(node,qtk_stitch_queue_data_t,node);

    lir_aspect = qtk_stitch_image_get_ratio(image->img_data,QTK_STITCH_IMAGE_RESOLUTION_LOW,QTK_STITCH_IMAGE_RESOLUTION_FINAL);
    qtk_stitch_corpper_crop(stitch->cropper,stitch->warper->warper_imgs,
                stitch->warper->warper_mask,stitch->warper->roi_corners,stitch->warper->roi_sizes, lir_aspect);
    return;
}

void _compensate_exposure_errors(qtk_stitch_t *stitch)
{
    qtk_stitch_exposure_compensator_apply(stitch->compensator,stitch->cropper->corners,
                                        stitch->cropper->imgs,stitch->cropper->masks);
    return;
}

void* _resize_seam_masks(qtk_stitch_t* stitch,void *seam_masks)
{
    void *ret = qtk_stitch_seamfinder_resize(stitch->seam_finder,stitch->cropper->masks,seam_masks);
    return ret;
}

void _initialize_composition(qtk_stitch_t *stitch)
{
    qtk_stitch_blender_prepare(stitch->blender,stitch->cropper->corners,stitch->cropper->size);
    return;
}

void _blend_images(qtk_stitch_t* stitch,void *reseam_masks)
{
    qtk_stitch_blender_feed(stitch->blender,stitch->cropper->imgs,reseam_masks,stitch->cropper->corners);
    return;
}

void _create_final_panorama(qtk_stitch_t* stitch)
{
    qtk_stitch_blender_blend(stitch->blender);
    return;
}

void* qtk_stitch_stitch(qtk_stitch_t* stitch)
{
    //match feature
    _match_features(stitch); //匹配特征点
    // _subset(stitch);
    _estimate_camera_parameters(stitch); //估计相机参数
    _refine_camera_parameters(stitch);  //精炼相机参数
    _perform_wave_correction(stitch);
    _estimate_scale(stitch);
    _warp_low_resolution(stitch);
    _prepare_cropper(stitch);
    _crop_low_resolution(stitch);
    _estimate_exposure_errors(stitch);
    void *seam_masks = _find_seam_masks(stitch); //返回值是std::vector<cv::UMat>
    _warp_final_resolution(stitch);
    _crop_final_resolution(stitch);
    _compensate_exposure_errors(stitch);
    void *reseam_masks = _resize_seam_masks(stitch,seam_masks); //std::vector<cv::UMat>
    _initialize_composition(stitch);
    _blend_images(stitch,reseam_masks);
    _create_final_panorama(stitch);

    if(reseam_masks){
        qtk_stitch_seamfinder_seam_masks_delete(reseam_masks);
    }
    if(seam_masks){
        qtk_stitch_seamfinder_seam_masks_delete(seam_masks);
    }
    return NULL;
}

//可能是要做成camera charge
void* qtk_stitch_stitch2(qtk_stitch_t* stitch)
{
    void *seam_masks = NULL;
    double t = time_get_ms();
    //match feature
    _match_features(stitch); //匹配特征点
    printf("match feature cost %lf ms\n",time_get_ms()-t);
    // _subset(stitch);
    t = time_get_ms();
    _estimate_camera_parameters2(stitch); //估计相机参数
    printf("estimate camera parameters cost %lf ms\n",time_get_ms()-t);
    //切换camera
    _stitch_camera_choose(stitch);
    if(stitch->camera_update){
        t = time_get_ms();
        _refine_camera_parameters(stitch);  //精炼相机参数
        printf("refine camera parameters cost %lf ms\n",time_get_ms()-t);
        t = time_get_ms();
        _perform_wave_correction(stitch);
        printf("perform wave correction cost %lf ms\n",time_get_ms()-t);
        _estimate_scale(stitch);
        t = time_get_ms();
        _warp_low_resolution(stitch);
        _prepare_cropper(stitch);
        _crop_low_resolution(stitch);
        _estimate_exposure_errors(stitch);
        seam_masks = _find_seam_masks(stitch); //返回值是std::vector<cv::UMat>
        printf("update seam mask cost %lf ms\n",time_get_ms()-t);
    }
    t = time_get_ms();
    _warp_final_resolution(stitch);
    printf("warp img cost %lf ms\n",time_get_ms()-t);
    t = time_get_ms();
    _crop_final_resolution(stitch);
    printf("crop img cost %lf ms\n",time_get_ms()-t);
    t = time_get_ms();
    _compensate_exposure_errors(stitch);
    printf("compensate exposure cost %f ms\n",time_get_ms()-t);
    t = time_get_ms();
    if(stitch->camera_update){
        if(stitch->reseam_masks){
            qtk_stitch_seamfinder_seam_masks_delete(stitch->reseam_masks);
        }
        stitch->reseam_masks = _resize_seam_masks(stitch,seam_masks); //std::vector<cv::UMat>
    }
    printf("seam masks resize cost %lf ms\n",time_get_ms()-t);
    t = time_get_ms();
    _initialize_composition(stitch);
    _blend_images(stitch,stitch->reseam_masks);
    _create_final_panorama(stitch);
    printf("create panorama cost %lf ms\n",time_get_ms()-t);

    if(seam_masks){
        qtk_stitch_seamfinder_seam_masks_delete(seam_masks);
    }
    return NULL;
}

void qtk_stitch_queue_clean(qtk_stitch_t* stitch)
{
    wtk_queue_node_t *node = NULL;
    qtk_stitch_queue_data_t *data = NULL;
    while((node = wtk_queue_pop(&stitch->img_queue))){
        data = data_offset2(node,qtk_stitch_queue_data_t,node);
        qtk_stitch_image_delete(data->img_data);
        data->img_data = NULL;
        qtk_stitch_feature_features_delete(data->feature_data);
        data->feature_data = NULL;
        wtk_lockhoard_push(&stitch->img_hoard,data);
    }
    return;
}

void qtk_stitch_delete(qtk_stitch_t* stitch)
{
    if(stitch){
        qtk_stitch_queue_clean(stitch);
        wtk_lockhoard_clean(&stitch->img_hoard);
        if(stitch->size_w){
            wtk_free(stitch->size_w);
            stitch->size_w = NULL;
        }
        if(stitch->size_h){
            wtk_free(stitch->size_h);
            stitch->size_h = NULL;
        }
        if(stitch->timelapser){
            qtk_stitch_timelapser_delete(stitch->timelapser);
            stitch->timelapser = NULL;
        }
        if(stitch->blender){
            qtk_stitch_blender_delete(stitch->blender);
            stitch->blender = NULL;
        }
        if(stitch->seam_finder){
            qtk_stitch_seamfinder_delete(stitch->seam_finder);
            stitch->seam_finder = NULL;
        }
        if(stitch->compensator){
            qtk_stitch_exposure_compensator_delete(stitch->compensator);
            stitch->compensator = NULL;
        }
        if(stitch->cropper){
            qtk_stitch_cropper_delete(stitch->cropper);
            stitch->cropper = NULL;
        }
        if(stitch->warper){
            qtk_stitch_warper_delete(stitch->warper);
            stitch->warper = NULL;
        }
        if(stitch->wave_corrector){
            qtk_stitch_wave_corrector_delete(stitch->wave_corrector);
            stitch->wave_corrector = NULL;
        }
        if(stitch->camera_adjuster){
            qtk_stitch_camera_adjuster_delete(stitch->camera_adjuster);
            stitch->camera_adjuster = NULL;
        }
        if(stitch->camera_estimator){
            qtk_stitch_camera_estimator_delete(stitch->camera_estimator);
            stitch->camera_estimator = NULL;
        }
        if(stitch->camera_estimator2){
            qtk_stitch_camera_estimator_delete(stitch->camera_estimator2);
            stitch->camera_estimator2 = NULL;
        }
        if(stitch->camera_estimator_tmp){
            qtk_stitch_camera_estimator_delete(stitch->camera_estimator_tmp);
            stitch->camera_estimator_tmp = NULL;
        }
        // if(stitch->subsetter){
        //     qtk_stitch_subsetter_delete(stitch->subsetter);
        //     stitch->subsetter = NULL;
        // }
        if(stitch->matcher){
            qtk_stitch_feature_matcher_delete(stitch->matcher);
            stitch->matcher = NULL;
        }
        if(stitch->detector){
            qtk_stitch_feature_detector_delete(stitch->detector);
            stitch->detector = NULL;
        }
        if(stitch->reseam_masks){
            qtk_stitch_seamfinder_seam_masks_delete(stitch->reseam_masks);
        }
        wtk_free(stitch);
    }
    return;
}

void* qtk_stitch_get_end(qtk_stitch_t *stitch,int *row,int *col)
{
    return qtk_stitch_blender_get_panorama(stitch->blender,row,col);
}

void _stitch_camera_choose(qtk_stitch_t *stitch)
{
    float ret = 0;
    stitch->camera_update = 0;
    ret = qtk_stitch_camera_estimator_distance(stitch->camera_estimator_tmp,stitch->camera_estimator2);
    if(ret < 0){
        qtk_stitch_camera_estimator_2copy1(stitch->camera_estimator2,stitch->camera_estimator_tmp);
        qtk_stitch_camera_estimator_2to1(stitch->camera_estimator2,stitch->camera_estimator);
        stitch->touch_camera = 0;
        stitch->camera_update = 1;
        return;
    }
    if(ret > 0.04){
        stitch->touch_camera += 1;
    }else{
        stitch->touch_camera = 0;
    }
    if(stitch->touch_camera > 2 || ret > 0.5){
        qtk_stitch_camera_estimator_2copy1(stitch->camera_estimator2,stitch->camera_estimator_tmp);
        qtk_stitch_camera_estimator_2to1(stitch->camera_estimator2,stitch->camera_estimator);
        stitch->touch_camera = 0;
        stitch->camera_update = 1;
    }
    return;
}
