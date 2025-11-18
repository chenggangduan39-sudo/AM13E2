#include "qtk/stitch/qtk_stitch.h"
#include "qtk/image/qtk_image.h"
#include "wtk/core/rbin/wtk_flist.h"
#include "qtk/stitch/image/qtk_stitch_image.h"
#include "wtk/core/cfg/wtk_main_cfg.h"

int test1(int argc, char *argv[])
{
    int ret = 0;
    qtk_stitch_t *stitch = NULL;
    qtk_stitch_cfg_t cfg;

    qtk_stitch_cfg_init(&cfg);
    cfg.medium_megapix = 0.6;
    cfg.detector = QTK_STITCH_FEATURE_DETECTOR_ORB;
    cfg.nfeatures = 3000;
    cfg.matcher_type = QTK_STITCH_FEATURE_MATCHER_HOMOGRAPHY;
    cfg.range_width = -1;
    cfg.try_use_gpu = 0;
    cfg.match_conf = 0;
    cfg.confidence_threshold = 1.2f;
    wtk_string_set(&cfg.matches_graph_dot_file,"",0);
    cfg.estimator = QTK_STITCH_CAMERA_ESTIMATOR_HOMOGRAPHY;
    cfg.adjuster = QTK_STITCH_CAMERA_ADJUSTER_RAY;
    wtk_string_set_s(&cfg.refinement_mask,"xxxxx");
    cfg.wave_correct_kind = QTK_STITCH_WAVE_CORRECT_HORIZ;
    cfg.warper_type = QTK_STITCH_WARPER_SPHERICAL;
    cfg.low_megapix = 0.1;
    cfg.crop = 1;
    cfg.compensator = QTK_STITCH_EXPOSURE_GAIN_BLOCK;
    cfg.nr_feeds = 1;
    cfg.block_size = 32;
    cfg.finder = QTK_STITCH_FINDER_DP_COLOR;
    cfg.final_megapix = -1;
    cfg.blender_type = QTK_STITCH_BLENDER_MULTIBAND;
    cfg.blend_strength = 5;
    cfg.timelapse = QTK_STITCH_TIMELAPSE_NO;
    wtk_string_set_s(&cfg.timelapse_prefix,"fixed_");


    stitch = qtk_stitch_new(&cfg);
    qtk_stitch_push_image_file(stitch, "/home/shensy/work/stitching/capture1/0207_042155/camera1.jpg");
    qtk_stitch_push_image_file(stitch, "/home/shensy/work/stitching/capture1/0207_042155/camera2.jpg");
    qtk_stitch_push_image_file(stitch, "/home/shensy/work/stitching/capture1/0207_042155/camera3.jpg");
    double t = time_get_ms();
    qtk_stitch_stitch(stitch);
    wtk_debug("%lf\n",time_get_ms() - t);
    
    int row = 0;
    int col = 0;
    void *data = qtk_stitch_get_end(stitch, &row, &col);
    qtk_image_desc_t desc = {
        .fmt = QBL_IMAGE_BGR24,
        .channel = 3,
        .width = col,
        .height = row,
    };
    qtk_image_save_bmp("out.bmp",data,&desc);
    qtk_stitch_delete(stitch);
    return ret;
}

int test2(int argc, char *argv[])
{
    int ret = 0;
    qtk_stitch_t *stitch = NULL;
    qtk_stitch_cfg_t *cfg = NULL;
    wtk_main_cfg_t *main_cfg = NULL;
    wtk_flist_it_t* it = NULL;
    char pp[128] = {0,};

    // qtk_stitch_cfg_init(&cfg);
    // cfg.medium_megapix = 0.6;
    // cfg.detector = QTK_STITCH_FEATURE_DETECTOR_SIFT;
    // cfg.nfeatures = 5000;
    // cfg.matcher_type = QTK_STITCH_FEATURE_MATCHER_HOMOGRAPHY;
    // cfg.range_width = -1;
    // cfg.try_use_gpu = 0;
    // cfg.match_conf = 0;
    // cfg.confidence_threshold = 1.f;
    // wtk_string_set(&cfg.matches_graph_dot_file,"",0);
    // cfg.estimator = QTK_STITCH_CAMERA_ESTIMATOR_HOMOGRAPHY;
    // cfg.adjuster = QTK_STITCH_CAMERA_ADJUSTER_RAY;
    // wtk_string_set_s(&cfg.refinement_mask,"xxxxx");
    // cfg.wave_correct_kind = QTK_STITCH_WAVE_CORRECT_HORIZ;
    // cfg.warper_type = QTK_STITCH_WARPER_SPHERICAL;
    // cfg.low_megapix = 0.5;
    // cfg.crop = 1;
    // cfg.compensator = QTK_STITCH_EXPOSURE_CHANNEL_BLOCKS;
    // cfg.nr_feeds = 3;
    // cfg.block_size = 32;
    // cfg.finder = QTK_STITCH_FINDER_GC_COLOR;
    // cfg.final_megapix = -1;
    // cfg.blender_type = QTK_STITCH_BLENDER_FEATHER;
    // cfg.blend_strength = 1;
    // cfg.timelapse = QTK_STITCH_TIMELAPSE_NO;
    // wtk_string_set_s(&cfg.timelapse_prefix,"fixed_");

    char *list_fn = NULL;
    char *out_dir = NULL;
    char *cfn = NULL;
    int use_list = 0;
    wtk_arg_t *arg = wtk_arg_new(argc,argv);
    if(arg == NULL){
        goto end;
    }

    wtk_arg_get_str_s(arg,"c",&cfn);
    if(cfn == NULL){
        printf("no config file use -c config_fn\n");
        goto end;
    }

    wtk_arg_get_str_s(arg,"l",&list_fn);
    if(list_fn){
        use_list = 1;
    }

    wtk_arg_get_str_s(arg,"o",&out_dir);
    if(out_dir == NULL){
        wtk_debug("no out dir\n");
        goto end;
    }

    main_cfg = wtk_main_cfg_new_type(qtk_stitch_cfg, cfn);
    if(main_cfg == NULL){
        printf("cfg error\n");
        goto end;
    }

    cfg = main_cfg->cfg;
    stitch = qtk_stitch_new(cfg);
    
    if(use_list){
        it = wtk_flist_it_new(list_fn);
        do{
            char *path = wtk_flist_it_next(it);
            if(path){
                char *num = path+(strlen(path)-4);
                printf("<<< %s\n",path);
                double t = time_get_ms();
                sprintf(pp,"%s/%s",path,"camare_1.jpg");
                qtk_stitch_push_image_file(stitch, pp);
                sprintf(pp,"%s/%s",path,"camare_0.jpg");
                qtk_stitch_push_image_file(stitch, pp);
                sprintf(pp,"%s/%s",path,"camare_2.jpg");
                qtk_stitch_push_image_file(stitch, pp);
                qtk_stitch_stitch2(stitch);
                wtk_debug("%lf\n",time_get_ms() - t);
                int row = 0;
                int col = 0;
                void *data = qtk_stitch_get_end(stitch, &row, &col);
                sprintf(pp,"%s/%s%s",out_dir,num,".jpg");
                printf(">>>>> %s\n",pp);
                qtk_stitch_image_save_data(pp,data, col, row, 3);
                qtk_stitch_queue_clean(stitch);
            }else{
                break;
            }
        }while(1);
        wtk_flist_it_delete(it);
    }
end:
    if(stitch) qtk_stitch_delete(stitch);
    if(main_cfg) wtk_main_cfg_delete(main_cfg);
    if(arg) wtk_arg_delete(arg);

    return ret;
}

int main(int argc, char *argv[])
{
    test2(argc,argv);
    return 0;
}
