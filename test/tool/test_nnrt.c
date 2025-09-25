#include "qtk/nnrt/qtk_nnrt.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/wtk_riff.h"
#include "qtk/image/qtk_image.h"

typedef struct {
    qtk_nnrt_t *nnrt;
} ctx_t;
ctx_t ctx;

char *ofn;

static void do_(qtk_nnrt_t *nnrt, const char *dfn, int N) {
    char path[1024];
    snprintf(path, sizeof(path), "%s/audio.wav", dfn);
    qtk_image_desc_t desc;
    qtk_nnrt_value_t input, output;
    qtk_nnrt_value_t inputvar;
    int64_t shape[4] = {1, 3, 360, 640};
//     int64_t shape[4] = {1, 3, 640, 360};
    float *intmp;
    uint8_t *outtmp;
    intmp = malloc(sizeof(float) * 3 * shape[2] * shape[3]);
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
    for (int i = 0; i < N; i++) {
        uint8_t *I;
        if(N == 1)
        {
                I = qtk_image_load(&desc, dfn);   
        }else{
                snprintf(path, sizeof(path), "%s/%05d.jpeg", dfn, i + 1);
                wtk_debug("===================>>>>>>>>>>>>path=%s\n",path);
                I = qtk_image_load(&desc, path);
        }
        wtk_debug("=========>>>>>>>channel=%d height=%d width=%d\n",desc.channel,desc.height,desc.width);
#ifdef QTK_NNRT_RKNPU
        input = qtk_nnrt_value_create_external(nnrt, QTK_NNRT_VALUE_ELEM_U8, shape, 4, I) ;
#else
        int j,n=desc.height*desc.width;
        for(j=0;j<n;++j)
        {
            intmp[j] = I[3*j]/255.0;
            intmp[j+n] = I[3*j+1]/255.0;
            intmp[j+2*n] = I[3*j+2]/255.0;
            // wtk_debug("in_val1=%f/%d in_val2=%f/%d in_val3=%f/%d\n",intmp[j],I[3*j],intmp[j+n],I[3*j+1],intmp[j+2*n],I[3*j+2]);
        }
        input = inputvar;
#endif
        tm = time_get_ms();
#ifdef QTK_NNRT_RKNPU
        qtk_nnrt_feed_image(nnrt, input, 0);
#else
        qtk_nnrt_feed(nnrt, input, 0);
#endif
        wtk_debug("===================>>>>>>>>>>>>\n");
        qtk_nnrt_run(nnrt);

        wtk_debug("==================>>>>>>>>>>>>>>>>run time=%f\n",time_get_ms() - tm);
        qtk_nnrt_get_output(nnrt, &output, 0);
        output_val = qtk_nnrt_value_get_data(nnrt, output);
        wtk_debug("==================>>>>>>>>>>>>>>>>get time=%f\n",time_get_ms() - tm);
#ifdef QTK_NNRT_RKNPU
        int j,n=desc.height*desc.width;
        n=n*16;
        for(j=0;j<n;++j)
        {
        //     wtk_debug("out_val1=%f out_val2=%f out_val3=%f\n",output_val[j],output_val[j+n],output_val[j+2*n]);
            output_val[j] = output_val[j] > 1.0 ? 1.0 : output_val[j];
            output_val[j] = output_val[j] < 0.0 ? 0.0 : output_val[j];
            output_val[j+n] = output_val[j+n] > 1.0 ? 1.0 : output_val[j+n];
            output_val[j+n] = output_val[j+n] < 0.0 ? 0.0 : output_val[j+n];
            output_val[j+2*n] = output_val[j+2*n] > 1.0 ? 1.0 : output_val[j+2*n];
            output_val[j+2*n] = output_val[j+2*n] < 0.0 ? 0.0 : output_val[j+2*n];
            outtmp[3*j] = (uint8_t)(output_val[j]*255.0);
            outtmp[3*j+1] = (uint8_t)(output_val[j+n]*255.0);
            outtmp[3*j+2] = (uint8_t)(output_val[j+2*n]*255.0);
        }
        desc.height = desc.height * 4;
        desc.width = desc.width * 4;
        qtk_image_save_png(ofn, outtmp, &desc);
#endif  
        qtk_nnrt_value_release(nnrt, input);
        qtk_nnrt_value_release(nnrt, output);
        qtk_nnrt_reset(nnrt);

        wtk_debug("==================>>>>>>>>>>>>>>>>reset time=%f\n",time_get_ms() - tm);
        wtk_free(I);
    }
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
    wtk_arg_get_str_s(arg, "d", &dfn);
    wtk_arg_get_str_s(arg, "o", &ofn);
    wtk_arg_get_int_s(arg, "n", &N);

    qtk_nnrt_set_global_thread_pool(4, NULL);
    main_cfg = wtk_main_cfg_new_type(qtk_nnrt_cfg, cfn);
    ctx.nnrt = qtk_nnrt_new((qtk_nnrt_cfg_t *)main_cfg->cfg);
    do_(ctx.nnrt, dfn, N);
    qtk_nnrt_delete(ctx.nnrt);
    wtk_main_cfg_delete(main_cfg);
    wtk_arg_delete(arg);
}
