#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/wtk_arg.h"
#include "qtk/stitch/qtk_stitch_run_phase.h"
#include "qtk/image/qtk_image.h"
#include "qtk/stitch/image/qtk_stitch_image.h"
#include "wtk/core/rbin/wtk_flist.h"

int test_nv12_scp(qtk_stitch_run_phase_t *stitch, int w, int h ,void *mm, char *infn, char *ofn);

//rgb 模式
int test(int argc,char **argv)
{
    wtk_arg_t *arg = NULL;
    char *cfn = NULL;
    wtk_main_cfg_t *main_cfg = NULL;
    qtk_stitch_run_cfg_t *cfg = NULL;
    qtk_stitch_run_phase_t *stitch = NULL;

    arg = wtk_arg_new(argc, argv);
    wtk_arg_get_str_s(arg, "c", &cfn);

    if (cfn == NULL) {
        wtk_debug("missing  cfn");
        exit(-1);
    }

    main_cfg = wtk_main_cfg_new_type(qtk_stitch_run_cfg, cfn);
    if (main_cfg == NULL){
        wtk_debug("main_cfg_new_type failed");
        goto end;
    }

    cfg = main_cfg->cfg;
    stitch = qtk_stitch_run_phase_new(cfg);
    int w = 0,h = 0;
    qtk_stitch_run_phase_get_rect(stitch,&w,&h);
    wtk_debug("%d %d\n",w,h);
    void *mm = wtk_malloc(w*h*4);
    qtk_stitch_run_phase_set_blend_mem(stitch,mm);
    qtk_stitch_run_phase_start(stitch);
    void *img1 = NULL, *img2 = NULL, *img3 = NULL,*img_all = NULL;
    if(stitch->cfg->use_all_img == 0){
        // img1 = qtk_stitch_image_read_data("0325-2k/camera0325_002/camera/camera1.jpg");
        // img2 = qtk_stitch_image_read_data("0325-2k/camera0325_002/camera/camera2.jpg");
        // img3 = qtk_stitch_image_read_data("0325-2k/camera0325_002/camera/camera3.jpg");
        // img1 = qtk_stitch_image_read_data("nv12_img/camera0325_003/camera/camera1.jpg");
        // img2 = qtk_stitch_image_read_data("nv12_img/camera0325_003/camera/camera2.jpg");
        // img3 = qtk_stitch_image_read_data("nv12_img/camera0325_003/camera/camera3.jpg");
        img1 = qtk_stitch_image_read_data("20250411/0411_4k/camera1.jpg");
        img2 = qtk_stitch_image_read_data("20250411/0411_4k/camera2.jpg");
        img3 = qtk_stitch_image_read_data("20250411/0411_4k/camera3.jpg");
    }else{
#if 0
        img_all = qtk_stitch_image_read_data("0307_121439-2k/yyyy1.png");
#else
        img_all = qtk_stitch_image_read_data("nv12_img/capture_7680_1440.png");
#endif
    }
    if(stitch->cfg->use_compensator){
        int low_w = 0,low_h = 0;
        void *low_img1 = NULL, *low_img2 = NULL, *low_img3 = NULL;
        qtk_stitch_run_phase_get_low_size(stitch,&low_w,&low_h);
        wtk_debug("low %d %d\n",low_w,low_h);
        low_img1 = qtk_stitch_image_read_data("20250411/0411_4k/camera1_low.jpg");
        low_img2 = qtk_stitch_image_read_data("20250411/0411_4k/camera2_low.jpg");
        low_img3 = qtk_stitch_image_read_data("20250411/0411_4k/camera3_low.jpg");
        // low_img1 = qtk_stitch_image_read_data("0325-2k/camera0325_002/camera/camera1_low.jpg");
        // low_img2 = qtk_stitch_image_read_data("0325-2k/camera0325_002/camera/camera2_low.jpg");
        // low_img3 = qtk_stitch_image_read_data("0325-2k/camera0325_002/camera/camera3_low.jpg");
        qtk_stitch_run_phase_push_low_data(stitch,low_img1);
        qtk_stitch_run_phase_push_low_data(stitch,low_img2);
        qtk_stitch_run_phase_push_low_data(stitch,low_img3);
        qtk_stitch_run_phase_estimate_esposure(stitch);
        free(low_img1);
        free(low_img2);
        free(low_img3);
    }
    //测试拼接缝移动
    // qtk_stitch_run_phase_set_seammask_region(stitch,10,0,500,0,1);
    for(int i = 0,n = 1; i < n; ++i){
        if(stitch->cfg->use_all_img == 0){
#if 0
            qtk_stitch_run_phase_push_data(stitch,4,img1);
            qtk_stitch_run_phase_push_data(stitch,4,img2);
            qtk_stitch_run_phase_push_data(stitch,4,img3);  
#else
            qtk_stitch_run_phase_push_data(stitch,3,img1);
            qtk_stitch_run_phase_push_data(stitch,3,img2);
            qtk_stitch_run_phase_push_data(stitch,3,img3);
#endif
        }else{
#if 0
            qtk_stitch_run_phase_push_data(stitch,4,img_all);
#else
            qtk_stitch_run_phase_push_data(stitch,3,img_all);
#endif
        }

        memset(mm,0,w*h*4);
        double t = time_get_ms();
        qtk_stitch_run_phase_run(stitch);
        wtk_debug("%f\n",time_get_ms()-t);

        if(stitch->cfg->blender_type != QTK_STITCH_BLENDER_FEATHER && stitch->cfg->blender_type != QTK_STITCH_BLENDER_NO){
            int row = 0;
            int col = 0;
            void *data = qtk_stitch_run_phase_get_end(stitch, &row, &col);
#if 0
            qtk_stitch_image_save_data("out.png",data,col,row,4);
#else
            qtk_stitch_image_save_data("out.png",data,col,row,3);
#endif
        }else{
#if 0
            qtk_stitch_image_save_data("out.png",mm,w,h,4);
#else
            qtk_stitch_image_save_data("out.png",mm,w,h,3);
#endif
        }
        qtk_stitch_run_phase_img_clean(stitch);
    }
    wtk_free(mm);
    qtk_stitch_run_phase_stop(stitch);
    if(stitch->cfg->use_all_img == 0){
        wtk_free(img1);
        wtk_free(img2);
        wtk_free(img3);
    }else{
        wtk_free(img_all);
    }

end:
    if(stitch){
        qtk_stitch_run_phase_delete(stitch);
    }
    if(main_cfg){
        wtk_main_cfg_delete(main_cfg);
    }
    if(arg){
        wtk_arg_delete(arg);
    }
    return 0;
}

//nv12模式
int test_nv12(int argc,char **argv)
{
    wtk_arg_t *arg = NULL;
    char *cfn = NULL;
    char *scp_fn = NULL;
    char *ofn = NULL;
    wtk_main_cfg_t *main_cfg = NULL;
    qtk_stitch_run_cfg_t *cfg = NULL;
    qtk_stitch_run_phase_t *stitch = NULL;

    arg = wtk_arg_new(argc, argv);
    wtk_arg_get_str_s(arg, "c", &cfn);

    if (cfn == NULL) {
        wtk_debug("missing  cfn\n");
        exit(-1);
    }

    wtk_arg_get_str_s(arg,"scp",&scp_fn);
    wtk_arg_get_str_s(arg,"o",&ofn);

    main_cfg = wtk_main_cfg_new_type(qtk_stitch_run_cfg, cfn);
    if (main_cfg == NULL){
        wtk_debug("main_cfg_new_type failed");
        goto end;
    }

    cfg = main_cfg->cfg;
    stitch = qtk_stitch_run_phase_new(cfg);
    int w = 0,h = 0;
    qtk_stitch_run_phase_get_rect(stitch,&w,&h);
    wtk_debug("%d %d\n",w,h);
    unsigned char* mm = (unsigned char*)wtk_malloc(w*h*1.5);
    qtk_stitch_run_phase_set_blend_mem(stitch,mm);
    qtk_stitch_run_phase_start(stitch);

    if(stitch->cfg->use_compensator){
        int low_w = 0,low_h = 0;
        void *low_img1 = NULL, *low_img2 = NULL, *low_img3 = NULL;
        qtk_stitch_run_phase_get_low_size(stitch,&low_w,&low_h);
        wtk_debug("low %d %d\n",low_w,low_h);
        low_img1 = qtk_stitch_image_read_data("picture2/frame_0005_low1.png");
        low_img2 = qtk_stitch_image_read_data("picture2/frame_0005_low2.png");
        low_img3 = qtk_stitch_image_read_data("picture2/frame_0005_low3.png");
        qtk_stitch_run_phase_push_low_data(stitch,low_img1);
        qtk_stitch_run_phase_push_low_data(stitch,low_img2);
        qtk_stitch_run_phase_push_low_data(stitch,low_img3);
        qtk_stitch_run_phase_estimate_esposure(stitch);
        free(low_img1);
        free(low_img2);
        free(low_img3);
    }
    //测试拼接缝移动
    // qtk_stitch_run_phase_set_seammask_region(stitch,10,0,500,0,1);
    if(scp_fn == NULL){
        void *img1 = NULL, *img2 = NULL, *img3 = NULL,*img_all = NULL;
        if(stitch->cfg->use_all_img == 0){
            img1 = qtk_stitch_image_read_data_raw("04-18 拼接缝/camera1.nv12");
            img2 = qtk_stitch_image_read_data_raw("04-18 拼接缝/camera2.nv12");
            img3 = qtk_stitch_image_read_data_raw("04-18 拼接缝/camera3.nv12");
        }else{
            img_all = qtk_stitch_image_read_data_raw("picture2/frame_0005.nv12");
        }
        for(int i = 0,n = 4; i < n; ++i){
            if(stitch->cfg->use_all_img == 0){
                qtk_stitch_run_phase_push_data(stitch,1.5,img1);
                qtk_stitch_run_phase_push_data(stitch,1.5,img2);
                qtk_stitch_run_phase_push_data(stitch,1.5,img3);
            }else{
                qtk_stitch_run_phase_push_data(stitch,1.5,img_all);
            }

            memset(mm,0,w*h);
            memset(mm+(w*h),128,w*h*0.5);
            double t = time_get_ms();
            qtk_stitch_run_phase_run(stitch);
            // qtk_stitch_run_phase_run_reseammask(stitch);
            // qtk_stitch_run_phase_run_no_reseammask(stitch);
            wtk_debug("%f\n",time_get_ms()-t);

            if(stitch->cfg->blender_type != QTK_STITCH_BLENDER_FEATHER && stitch->cfg->blender_type != QTK_STITCH_BLENDER_NO){
                int row = 0;
                int col = 0;
                void *data = qtk_stitch_run_phase_get_end(stitch, &row, &col);

                qtk_stitch_image_save_data("out.png",data,col,row,1);
            }else{
                qtk_stitch_image_save_data("out.png",mm,w,h*1.5,1);
            }
            qtk_stitch_run_phase_img_clean(stitch);
        }
        if(stitch->cfg->use_all_img == 0){
            wtk_free(img1);
            wtk_free(img2);
            wtk_free(img3);
        }else{
            wtk_free(img_all);
        }
    }else{
        if(ofn == NULL){
            printf("-o arg is null\n");
            goto end;
        }
        wtk_flist_it_t*it = NULL;
        it = wtk_flist_it_new(scp_fn);
        while(1){
            char *istr = wtk_flist_it_next(it);
            if(!istr)  break;
            if(*istr=='#') continue;
            test_nv12_scp(stitch,w,h,mm,istr,ofn);
        }
        wtk_flist_it_delete(it); 
    }
    wtk_free(mm);
    qtk_stitch_run_phase_stop(stitch);

end:
    if(stitch){
        qtk_stitch_run_phase_delete(stitch);
    }
    if(main_cfg){
        wtk_main_cfg_delete(main_cfg);
    }
    if(arg){
        wtk_arg_delete(arg);
    }
    return 0;
}

static int idx = 0;

int test_nv12_scp(qtk_stitch_run_phase_t *stitch, int w, int h ,void* mm, char *infn, char *ofn)
{
    printf("%s %s\n",infn,ofn);

    void *img1 = NULL, *img2 = NULL, *img3 = NULL,*img_all = NULL;
    char path[256] = {0,};
    if(stitch->cfg->use_all_img == 0){
        sprintf(path,"%s/%s",infn,"camera1.nv12");
        img1 = qtk_stitch_image_read_data_raw(path);
        sprintf(path,"%s/%s",infn,"camera2.nv12");
        img2 = qtk_stitch_image_read_data_raw(path);
        sprintf(path,"%s/%s",infn,"camera3.nv12");
        img3 = qtk_stitch_image_read_data_raw(path);
    }else{
        img_all = qtk_stitch_image_read_data_raw(infn);
    }   
    if(stitch->cfg->use_all_img == 0){
        qtk_stitch_run_phase_push_data(stitch,1.5,img1);
        qtk_stitch_run_phase_push_data(stitch,1.5,img2);
        qtk_stitch_run_phase_push_data(stitch,1.5,img3);
    }else{
        qtk_stitch_run_phase_push_data(stitch,1.5,img_all);
    }

    memset(mm,0,w*h);
    memset(mm+(w*h),128,w*h*0.5);
    double t = time_get_ms();
    qtk_stitch_run_phase_run(stitch);
    // qtk_stitch_run_phase_run_reseammask(stitch);
    // qtk_stitch_run_phase_run_no_reseammask(stitch);
    wtk_debug("%f\n",time_get_ms()-t);

    sprintf(path,"%s/out_%d.png",ofn,idx);
    if(stitch->cfg->blender_type != QTK_STITCH_BLENDER_FEATHER && stitch->cfg->blender_type != QTK_STITCH_BLENDER_NO){
        int row = 0;
        int col = 0;
        void *data = qtk_stitch_run_phase_get_end(stitch, &row, &col);

        qtk_stitch_image_save_data(path,data,col,row,1);
    }else{
        qtk_stitch_image_save_data(path,mm,w,h*1.5,1);
    }
    ++idx;
    qtk_stitch_run_phase_img_clean(stitch);
    if(stitch->cfg->use_all_img == 0){
        wtk_free(img1);
        wtk_free(img2);
        wtk_free(img3);
    }else{
        wtk_free(img_all);
    }
    return 0;
}

int main(int argc,char **argv)
{
    return test_nv12(argc,argv);
    // return test(argc,argv);
}