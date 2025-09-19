#include "qtk_crnnoise_nnrt.h"

#define ASSIGN_SHAPE(shape, B, C, H, W)                                        \
    (shape)[0] = B;                                                            \
    (shape)[1] = C;                                                            \
    (shape)[2] = H;                                                            \
    (shape)[3] = W;

qtk_crnnoise_nnrt_t *qtk_crnnoise_new(qtk_nnrt_cfg_t *cfg){
    qtk_crnnoise_nnrt_t * nrt = (qtk_crnnoise_nnrt_t*)wtk_malloc(sizeof(qtk_crnnoise_nnrt_t));

    nrt->nnrt = qtk_nnrt_new(cfg);
    nrt->num_cache = 7;
    nrt->cache[0] = NULL;
    nrt->input = (float*)wtk_malloc(sizeof(float) * 129 *2);

    ASSIGN_SHAPE(nrt->shape[0], 1, 129, 1, 2);
    ASSIGN_SHAPE(nrt->shape[1], 1, 1, 1, 24);
    ASSIGN_SHAPE(nrt->shape[2], 1, 6, 3, 24);
    ASSIGN_SHAPE(nrt->shape[3], 1, 6, 3, 24);
    ASSIGN_SHAPE(nrt->shape[4], 1, 6, 1, 24);
    ASSIGN_SHAPE(nrt->shape[5], 1, 1, 16, 0);
    ASSIGN_SHAPE(nrt->shape[6], 1, 1, 24, 0);
    ASSIGN_SHAPE(nrt->shape[7], 1, 1, 48, 0);

    return nrt;
}

void qtk_crnnoise_delete(qtk_crnnoise_nnrt_t *nrt){
    int i;
    if(nrt->cache[0]) {
        for(i = 0; i < 7; i++){
            qtk_nnrt_value_release(nrt->nnrt, nrt->cache[i]);
        }
    }
    wtk_free(nrt->input);
    qtk_nnrt_delete(nrt->nnrt);
    wtk_free(nrt);
}

void qtk_crnnoise_reset(qtk_crnnoise_nnrt_t *nrt){

}

void qtk_crnnoise_run(qtk_crnnoise_nnrt_t *nrt, wtk_complex_t *x, wtk_complex_t *y){
    float *fake_cache[7],*out;
    fake_cache[0] = NULL;
    int i;
    qtk_nnrt_t *nnrt = nrt->nnrt;
    qtk_nnrt_value_t cache[7],input_val;
    qtk_nnrt_value_t output;
    int shape_len;

    float *p = nrt->input;

    for(i = 0; i < 129; i++){
        *p = x[i].a;
        p++;
        *p = x[i].b;
        p++;
    }

    if(nrt->cache[0]){
        for(i = 0; i < 7; i++){
            cache[i] = nrt->cache[i];
        }
    }else{
        int64_t *cache_shape;
        for(i = 0; i < 7; i++){
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

    for(i = 0; i < 7; i++) {
        qtk_nnrt_feed(nrt->nnrt, cache[i], i + 1);
    }
    qtk_nnrt_run(nrt->nnrt);

    qtk_nnrt_get_output(nrt->nnrt, &output, 0);

    out = qtk_nnrt_value_get_data(nnrt, output);

    p = (float*)out;
    for(i = 0; i < 129; i++){
        y[i].a = *p;
        p++;
        y[i].b = *p;
        p++;
    }

    for(i = 0; i < 7; i++) {
        qtk_nnrt_get_output(nrt->nnrt, &nrt->cache[i], i + 1);
    //     qtk_nnrt_value_get_shape(nnrt, nrt->cache[i],
    //                          shapex, 4);
    // wtk_debug("%ld %ld %ld %ld\n",shapex[0],shapex[1],shapex[2],shapex[3]);
    }

    qtk_nnrt_value_release(nrt->nnrt, input_val);
    for(i = 0; i < 7; i++) {
        qtk_nnrt_value_release(nrt->nnrt, cache[i]);
    }
     qtk_nnrt_value_release(nrt->nnrt, output);
    if (fake_cache[0]) {
        for(i = 0; i < 7; i++) {
            wtk_free(fake_cache[i]);
        }
    }
}