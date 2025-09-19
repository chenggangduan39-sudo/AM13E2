#include "qtk_crnnoise_nnrtv2.h"

#define ASSIGN_SHAPE(shape, B, C, H, W)                                        \
    (shape)[0] = B;                                                            \
    (shape)[1] = C;                                                            \
    (shape)[2] = H;                                                            \
    (shape)[3] = W;

qtk_crnnoise_nnrt2_t *qtk_crnnoise2_new(qtk_nnrt_cfg_t *cfg){
    qtk_crnnoise_nnrt2_t * nrt = (qtk_crnnoise_nnrt2_t*)wtk_malloc(sizeof(qtk_crnnoise_nnrt2_t));

    nrt->nnrt = qtk_nnrt_new(cfg);
    nrt->num_cache = 8;
    nrt->cache[0] = NULL;
    nrt->input = (float*)wtk_malloc(sizeof(float) * 129 * 3);

    ASSIGN_SHAPE(nrt->shape[0], 1, 3, 1, 129);
    ASSIGN_SHAPE(nrt->shape[1], 1, 16, 2, 17);
    ASSIGN_SHAPE(nrt->shape[2], 1, 16, 4, 17);
    ASSIGN_SHAPE(nrt->shape[3], 1, 1, 128, 0);
    ASSIGN_SHAPE(nrt->shape[4], 1, 1, 128, 0);
    ASSIGN_SHAPE(nrt->shape[5], 1, 1, 128, 0);
    ASSIGN_SHAPE(nrt->shape[6], 1, 1, 128, 0);
    ASSIGN_SHAPE(nrt->shape[7], 1, 16, 2, 17);
    ASSIGN_SHAPE(nrt->shape[8], 1, 16, 4, 17);

    return nrt;
}

void qtk_crnnoise2_delete(qtk_crnnoise_nnrt2_t *nrt){
    int i;
    if(nrt->cache[0]) {
        for(i = 0; i < 8; i++){
            qtk_nnrt_value_release(nrt->nnrt, nrt->cache[i]);
        }
    }
    wtk_free(nrt->input);
    qtk_nnrt_delete(nrt->nnrt);
    wtk_free(nrt);
}

void qtk_crnnoise2_reset(qtk_crnnoise_nnrt2_t *nrt){

}

void qtk_crnnoise2_run(qtk_crnnoise_nnrt2_t *nrt, wtk_complex_t *x, wtk_complex_t *z){
    float *fake_cache[8],*out;
    fake_cache[0] = NULL;
    int i;
    qtk_nnrt_t *nnrt = nrt->nnrt;
    qtk_nnrt_value_t cache[8],input_val;
    qtk_nnrt_value_t output;
    int shape_len;

    // FILE* fp = fopen("/home/madch/work/ebnfc/Acoustic_Howling_Suppression2/tt.bin","rb");
    // fread(x,sizeof(wtk_complex_t),129,fp);
    // fclose(fp);

    float *p = nrt->input;
    for(i = 0; i < 129; i++){
        *p = sqrtf(x[i].a * x[i].a + x[i].b * x[i].b);
        p++;
    }

    for(i = 0; i < 129; i++){
        *p = x[i].a;
        p++;
    }

    for(i = 0; i < 129; i++){
        *p = x[i].b;
        p++;
    }
    //print_float(nrt->input,129*3);
    if(nrt->cache[0]){
        for(i = 0; i < 8; i++){
            cache[i] = nrt->cache[i];
        }
    }else{
        int64_t *cache_shape;
        for(i = 0; i < 8; i++){
            cache_shape = nrt->shape[i+1];
            shape_len = cache_shape[3] == 0 ? 3:4;
            if(shape_len == 3){
                fake_cache[i] = wtk_calloc(
                    cache_shape[2] * cache_shape[1] * cache_shape[0],sizeof(float));
            }else{
                fake_cache[i] = wtk_calloc(
                    cache_shape[3] * cache_shape[2] * cache_shape[1] * cache_shape[0],sizeof(float));
            }
            cache[i] = qtk_nnrt_value_create_external(
                nrt->nnrt, QTK_NNRT_VALUE_ELEM_F32, cache_shape, shape_len, fake_cache[i]);
        }
    }

    input_val = qtk_nnrt_value_create_external(
        nnrt, QTK_NNRT_VALUE_ELEM_F32, nrt->shape[0], 4, nrt->input);

    qtk_nnrt_feed(nnrt, input_val, 0);

    for(i = 0; i < 8; i++) {
        qtk_nnrt_feed(nrt->nnrt, cache[i], i + 1);
    }
    qtk_nnrt_run(nrt->nnrt);

    qtk_nnrt_get_output(nrt->nnrt, &output, 0);

    out = qtk_nnrt_value_get_data(nnrt, output);

    p = (float*)out;
    for(i = 0; i < 129; i++){
        z[i].a = *p;
        p++;
    }

    for(i = 0; i < 129; i++){
        z[i].b = *p;
        p++;
    }

    for(i = 0; i < 8; i++) {
        qtk_nnrt_get_output(nrt->nnrt, &nrt->cache[i], i + 1);
        
    }

    qtk_nnrt_value_release(nrt->nnrt, input_val);
    for(i = 0; i < 8; i++) {
        qtk_nnrt_value_release(nrt->nnrt, cache[i]);
    }
     qtk_nnrt_value_release(nrt->nnrt, output);
    if (fake_cache[0]) {
        for(i = 0; i < 8; i++) {
            wtk_free(fake_cache[i]);
        }
    }
}