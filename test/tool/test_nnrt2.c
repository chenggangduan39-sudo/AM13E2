#include "qtk/nnrt/qtk_nnrt.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/wtk_riff.h"
#include "qtk/image/qtk_image.h"

typedef struct {
    qtk_nnrt_t *nnrt;
} ctx_t;
ctx_t ctx;

char *ofn;

static void do_(qtk_nnrt_t *nnrt, const char *dfn) {
        char path[1024];
        qtk_image_desc_t desc;
        qtk_image_desc_t odesc;
        qtk_nnrt_value_t input, output;
        qtk_nnrt_value_t inputvar;
        int64_t shape[4] = {1, 3, 1080, 1920};
        //     int64_t shape[4] = {1, 3, 640, 360};
        float *intmp;
        uint8_t *outtmp;
        uint8_t *out2tmp;
        intmp = malloc(sizeof(uint8_t) * 3 * 360 * 640);
        out2tmp = malloc(sizeof(uint8_t) * 3 * 360 * 640*16);
        outtmp = malloc(sizeof(uint8_t) * 3 * shape[2]*4 * shape[3]*4);
        double tm=0.0;
        float *output_val;
#ifndef QTK_NNRT_RKNPU
        inputvar = qtk_nnrt_value_create_external(nnrt, QTK_NNRT_VALUE_ELEM_F32, shape, 4, intmp) ;
#else
        printf("set detect RKNN_NPU_CORE_2 mask\n");
        // rknn_core_mask core_mask2 = RKNN_NPU_CORE_2;
        rknn_core_mask core_mask1 = RKNN_NPU_CORE_0_1;//RKNN_NPU_CORE_0_1
        int ret = rknn_set_core_mask(nnrt->ctx, core_mask1);
        if(ret != 0) printf("set core2 mask failed\n");
#endif
        uint8_t *I= qtk_image_load(&desc, dfn);
        wtk_debug("=========>>>>>>>channel=%d height=%d width=%d\n",desc.channel,desc.height,desc.width);

        tm = time_get_ms();
        for (int _i = 0; _i < 3; _i++) {
                for (int _j = 0; _j < 3; _j++) {
                        qtk_image_roi_t roi;
                        roi.x = _j * 640;
                        roi.y = _i * 360;
                        roi.height = 360;
                        roi.width = 640;
                        qtk_image_sub_u8(&desc, &roi, intmp, I);
                        input = qtk_nnrt_value_create_external(nnrt, QTK_NNRT_VALUE_ELEM_U8, shape, 4, intmp);
                        qtk_nnrt_feed_image(nnrt, input, 0);
                        qtk_nnrt_run(nnrt);

                        wtk_debug("==================>>>>>>>>>>>>>>>>run time=%f\n",time_get_ms() - tm);
                        qtk_nnrt_get_output(nnrt, &output, 0);
                        output_val = qtk_nnrt_value_get_data(nnrt, output);
                        int n=360*640*16;
                        for(int j=0;j<n;++j)
                        {
                        //     wtk_debug("out_val1=%f out_val2=%f out_val3=%f\n",output_val[j],output_val[j+n],output_val[j+2*n]);
                                output_val[j] = output_val[j] > 1.0 ? 1.0 : output_val[j];
                                output_val[j] = output_val[j] < 0.0 ? 0.0 : output_val[j];
                                output_val[j+n] = output_val[j+n] > 1.0 ? 1.0 : output_val[j+n];
                                output_val[j+n] = output_val[j+n] < 0.0 ? 0.0 : output_val[j+n];
                                output_val[j+2*n] = output_val[j+2*n] > 1.0 ? 1.0 : output_val[j+2*n];
                                output_val[j+2*n] = output_val[j+2*n] < 0.0 ? 0.0 : output_val[j+2*n];
                                out2tmp[3*j] = (uint8_t)(output_val[j]*255.0);
                                out2tmp[3*j+1] = (uint8_t)(output_val[j+n]*255.0);
                                out2tmp[3*j+2] = (uint8_t)(output_val[j+2*n]*255.0);
                        }
                        
                        roi.x = _j * 640*4;
                        roi.y = _i * 360*4;
                        roi.height = 360*4;
                        roi.width = 640*4;
                        odesc.fmt = QBL_IMAGE_RGB24;
                        odesc.channel = 3;
                        odesc.height = 1080*4;
                        odesc.width = 1920 *4;
                        qtk_image_patch(&odesc, &roi, outtmp, out2tmp, 1);

                        qtk_nnrt_value_release(nnrt, input);
                        qtk_nnrt_value_release(nnrt, output);
                        qtk_nnrt_reset(nnrt);
                }

        }
        wtk_debug("==================>>>>>>>>>>>>>>>>end time=%f\n",time_get_ms() - tm);

        qtk_image_save_png(ofn, outtmp, &odesc);
        wtk_debug("==================>>>>>>>>>>>>>>>>save time=%f\n",time_get_ms() - tm);
        wtk_free(I);
        free(intmp);
        free(outtmp);
}

int main(int argc, char *argv[]) {
    wtk_arg_t *arg = wtk_arg_new(argc, argv);
    char *cfn;
    char *dfn;
    
    wtk_main_cfg_t *main_cfg;
    int N;

    wtk_arg_get_str_s(arg, "c", &cfn);
    wtk_arg_get_str_s(arg, "i", &dfn);
    wtk_arg_get_str_s(arg, "o", &ofn);

    qtk_nnrt_set_global_thread_pool(4, NULL);
    main_cfg = wtk_main_cfg_new_type(qtk_nnrt_cfg, cfn);
    ctx.nnrt = qtk_nnrt_new((qtk_nnrt_cfg_t *)main_cfg->cfg);
    do_(ctx.nnrt, dfn);
    qtk_nnrt_delete(ctx.nnrt);
    wtk_main_cfg_delete(main_cfg);
    wtk_arg_delete(arg);
}
