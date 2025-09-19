#include "qtk_stitch_humanseg.h"
qtk_stitch_humanseg_t* qtk_stitch_humanseg_new(qtk_stitch_humanseg_cfg_t *cfg)
{
    qtk_stitch_humanseg_t *seg = (qtk_stitch_humanseg_t*)wtk_malloc(sizeof(qtk_stitch_humanseg_t));
    memset(seg, 0, sizeof(qtk_stitch_humanseg_t));
#ifndef IPU_DEC
    seg->rt = qtk_nnrt_new(&cfg->nnrt);
    seg->in = (float*)wtk_malloc(cfg->in_w * cfg->in_h * 3*sizeof(float));
#else
    qtk_ipu_device_new(NULL,&cfg->fn,1);
    seg->ipu = qtk_ipu_channel_new(cfg->fn);
#endif
    seg->cfg = cfg;
    return seg;
}

int qtk_stitch_humanseg_feed(qtk_stitch_humanseg_t *seg, void *data, void *out)
{
    unsigned char *img = (unsigned char*)data;
    int img_w = seg->cfg->in_w;
    int img_h = seg->cfg->in_h;
    int n = img_h*img_w;
    float *output = NULL;
    unsigned char *outimg = (unsigned char*)out;
#ifndef IPU_DEC
    int64_t shape[4] = {0,};
    qtk_nnrt_value_t value;
    qtk_nnrt_value_t value1;
    float mean = 0.5;
    float std = 0.5;
    float *input = seg->in;
    
    for (int i = 0; i < n; i++) {
        input[i] = (img[i * 3 + 0]/255.0f - mean) / std;
        input[i + n] = (img[i * 3 + 1]/255.0f - mean) / std;
        input[i + n * 2] = (img[i * 3 + 2]/255.0f - mean) / std;
        // printf("[ %d %d %d ]\n",img[i * 3 + 0],img[i * 3 + 1],img[i * 3 + 2]);
    }
    shape[2] = img_w; shape[3] = img_h;shape[1] = 3;shape[0] = 1;
    value = qtk_nnrt_value_create_external(seg->rt,
        QTK_NNRT_VALUE_ELEM_F32, shape, 4,
        input);
    qtk_nnrt_feed(seg->rt, value, 0);
    qtk_nnrt_run(seg->rt);
    qtk_nnrt_get_output(seg->rt, &value1, 0);
    // int ret = qtk_nnrt_value_get_shape(seg->rt, value1, shape, 4);
    // printf("%d  %d %d %d %d\n",ret,shape[0],shape[1],shape[2],shape[3]);
    output = (float*)qtk_nnrt_value_get_data(seg->rt, value1);
    
    for(int i = 0; i < n; ++i){
        // outimg[i] = output[i] > output[i+n] ? 255:0;
        // if(outimg[i] == 0){
        //     printf("[ %f %f ]\n",output[i],output[i+n]);
        // }
        outimg[i] = 255;
        if(output[i+n] > output[i] && output[i+n] > 0.9){
            outimg[i] = 0;
        }
    }
    qtk_nnrt_value_release(seg->rt, value);
    qtk_nnrt_value_release(seg->rt, value1);
    qtk_nnrt_reset(seg->rt);
#else
    output = qtk_ipu_single_invoke(seg->ipu,img);
    for(int i = 0; i < n; ++i){
        // outimg[i] = output[i*2] > output[i*2+1] ? 255:0;
        outimg[i] = 255;
        if(output[i*2+1] > output[i*2] && output[i*2+1] > 0.9){
            outimg[i] = 0;
        }
    }
#endif
    return 0;
}

int qtk_stitch_humanseg_delete(qtk_stitch_humanseg_t *seg)
{
    if(seg){
#ifndef IPU_DEC
        wtk_free(seg->in);
        qtk_nnrt_delete(seg->rt);
#else
        qtk_ipu_channel_delete(seg->ipu);
        qtk_ipu_device_delete();
#endif
        wtk_free(seg);
    }
    return 0;
}