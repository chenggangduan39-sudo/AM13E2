#include "qtk_linear_conv.h"

qtk_linear_conv_t *qtk_linear_conv_new(float *weight, int len, int hop_size){
    qtk_linear_conv_t *lc = wtk_malloc(sizeof(qtk_linear_conv_t));
    int i,j;
    wtk_debug("weight_len:%d\n", len);

    lc->hop_size = hop_size;
    lc->len_weight = len;
    lc->B = len;
    lc->N_BLK = floorf(len/hop_size + 0.5);

    int tmp = lc->N_BLK * hop_size - len;
    float *tmp_weight = (float*)wtk_calloc((tmp + len) * 2, sizeof(float));
    for(i = 0; i < lc->N_BLK; i++){
        memcpy(tmp_weight + i * hop_size * 2,weight + i * hop_size, hop_size * sizeof(float));
    }
    lc->weight = wtk_malloc(sizeof(wtk_complex_t) * lc->N_BLK * (hop_size + 1));
    lc->OUTPUT_CACHE = wtk_malloc(sizeof(wtk_complex_t) * lc->N_BLK * (hop_size + 1));
    wtk_complex_zero(lc->OUTPUT_CACHE,lc->N_BLK * (hop_size + 1));
    lc->output_cache = wtk_calloc(hop_size,sizeof(float));
    lc->drft = wtk_drft_new2(hop_size * 2);
    lc->out = wtk_malloc(sizeof(wtk_complex_t) * (hop_size + 1));
    lc->win = wtk_malloc(sizeof(float) * hop_size * 2);
    lc->in = wtk_calloc(hop_size * 2, sizeof(float));
    wtk_complex_t *cpx;
    for(i = 0; i < lc->N_BLK; i++){
        cpx = lc->weight + i * (hop_size + 1);
        //print_float(tmp_weight + i * hop_size * 2,hop_size * 2);
        wtk_drft_fft22(lc->drft, tmp_weight + i * hop_size * 2, cpx);
        for(j = 0; j < hop_size + 1; j++){
            cpx->a *= hop_size * 2;
            cpx->b *= hop_size * 2;
            //wtk_debug("%d %d %.10f %.10f\n",i , j, cpx->a, cpx->b);
            cpx++;
        }
    }

    wtk_free(tmp_weight);
    return lc;
}

static int* topk(float *f, int n, int k){
    float *p = f;
    float *probs = wtk_calloc(n,sizeof(float));
    int i,j,*indexes = wtk_calloc(n,sizeof(int));
    int cnt = 0;
    for(i = 0; i < n; i++,p++){
        for(j = 0; j < k; j++){
            if(fabs(*p) > *(probs+j)){
                cnt = k - 1 -j;
                if(cnt >= 0){
                    memmove(probs + j + 1,probs + j,sizeof(float)*cnt);
                    memmove(indexes + j + 1, indexes + j,sizeof(int)*cnt);
                }
                *(probs + j) = fabs(*p);
                *(indexes + j) = i;
                break;
            }
        }
    }
    wtk_free(probs);
    return indexes;
}

static float get_max(float *st, int cnt){
    float max = 0.0;
    float *p = st;
    int i;
    for(i = 0; i < cnt; i++){
        if(*p > max){
            max = *p;
        }
        p++;
    }

    return max;
}
int qtk_linear_conv_idx_find(float *weight, int len){
    int i = len * 0.01, idx = 0;
    int *idxs = topk(weight, len, i);
    float thresh = fabs(*(weight + *(idxs + i - 1)));
    float max = -1000.0;
    int a = *(idxs + i);
    int b = len - 1;
    i = (((a) < (b)) ? (a) : (b));

    float *p = weight;
    for(i = 0; i < *(idxs); i++){
        if(fabs(*p) < thresh){
            *p = 0.0;
        }else{
            *p = fabs(*p);
        }
        p++;
    }

    float *st;
    int cnt;
    p = weight;
    for(i = 0; i < *(idxs) + 1; i++){
        if(*p > 1e-3){
            if((i - 16) > 0){
                st = p - 16;
                cnt = 16;
            }else{
                st = p;
                cnt = i + 1;
            }
            if((i + 16) > len){
                cnt += *(idxs) - 1 - i;
            }else{
                cnt += 16;                
            }
            if( *p < get_max(st,cnt)){
                *p = 0.0;
            }
            if(*p > max){
                max = *p;
                idx = i;
            }
        }
        p++;
    }

    float thresh2 = 0.4 * max;
    int flag = 1;
    p = weight;
    for(i = 0; i < *(idxs) + 1; i++){
        if(*p > thresh2){
            if(flag){
                idx = i;
                flag = 0;
            }
        }else{
            *p = 0.0;
        }
        p++;
    }

    wtk_free(idxs);
    return idx;
}

qtk_linear_conv_t *qtk_linear_conv_new2(float *weight, int len, int hop_size,float rt60){
    qtk_linear_conv_t *lc = wtk_malloc(sizeof(qtk_linear_conv_t));
    float *tmpw = (float *)wtk_malloc(sizeof(float) * len);
    memcpy(tmpw, weight, sizeof(float) * len);
    int i,j;

    // print_float(weight,len);
    int idx = qtk_linear_conv_idx_find(weight,len);
    wtk_debug("idx:%d, rt60:%f\n", idx, rt60);
    len = rt60 * 16000 - 128;
    wtk_debug("len:%d\n", len);
    weight = tmpw + idx + 128;
    FILE *fp = fopen("weight.txt", "w");
    for (i = 0; i < len; i++) {
        fprintf(fp, "%f\n", weight[i]);
        // weight[i+80]=0.0;
    }
    fclose(fp);
    lc->hop_size = hop_size;
    lc->len_weight = len;
    lc->B = len;
    lc->N_BLK = floorf(len/hop_size + 0.5);

    int tmp = lc->N_BLK * hop_size - len;
    float *tmp_weight = (float*)wtk_calloc((tmp + len) * 2, sizeof(float));
    for(i = 0; i < lc->N_BLK; i++){
        memcpy(tmp_weight + i * hop_size * 2,weight + i * hop_size, hop_size * sizeof(float));
    }
    lc->weight = wtk_malloc(sizeof(wtk_complex_t) * lc->N_BLK * (hop_size + 1));
    lc->OUTPUT_CACHE = wtk_malloc(sizeof(wtk_complex_t) * lc->N_BLK * (hop_size + 1));
    wtk_complex_zero(lc->OUTPUT_CACHE,lc->N_BLK * (hop_size + 1));
    lc->output_cache = wtk_calloc(hop_size,sizeof(float));
    lc->drft = wtk_drft_new2(hop_size * 2);
    lc->out = wtk_malloc(sizeof(wtk_complex_t) * (hop_size + 1));
    lc->win = wtk_malloc(sizeof(float) * hop_size * 2);
    lc->in = wtk_calloc(hop_size * 2, sizeof(float));
    wtk_complex_t *cpx;
    for(i = 0; i < lc->N_BLK; i++){
        cpx = lc->weight + i * (hop_size + 1);
        //print_float(tmp_weight + i * hop_size * 2,hop_size * 2);
        wtk_drft_fft22(lc->drft, tmp_weight + i * hop_size * 2, cpx);
        for(j = 0; j < hop_size + 1; j++){
            cpx->a *= hop_size * 2;
            cpx->b *= hop_size * 2;
            //wtk_debug("%d %d %.10f %.10f\n",i , j, cpx->a, cpx->b);
            cpx++;
        }
    }

    wtk_free(tmp_weight);
    wtk_free(tmpw);
    return lc;
}

void qtk_linear_conv_delete(qtk_linear_conv_t *lc){
    wtk_drft_delete2(lc->drft);
    wtk_free(lc->weight);
    wtk_free(lc->OUTPUT_CACHE);
    wtk_free(lc->output_cache);
    wtk_free(lc->out);
    wtk_free(lc->win);
    wtk_free(lc->in);
    wtk_free(lc);
}

void qtk_linear_conv_reset(qtk_linear_conv_t *lc){

}

void qtk_linear_conv_conv1d_calc(qtk_linear_conv_t* lc, float *input){
    int i,j;
    int hop_size = lc->hop_size;
    wtk_complex_t *cpx,*cpx2,*cpx3;
    float scale = hop_size * 2;
    float A,B,C;

    memcpy(lc->in, input, hop_size * sizeof(float));
    cpx = lc->out;
    wtk_drft_fft22(lc->drft, lc->in, cpx);
    for(i = 0; i < hop_size + 1; i++){
        cpx->a *= scale;
        cpx->b *= scale;
        //wtk_debug("%d %.10f %.10f\n",i , cpx->a, cpx->b);
        cpx++;
    }

    cpx = lc->OUTPUT_CACHE;
    cpx2 = lc->weight;
    for(i = 0; i < lc->N_BLK; i++){
        cpx3 = lc->out;
        for(j = 0; j < hop_size + 1; j++){
            A = (cpx3->a + cpx3->b) * cpx2->a;
            B = (cpx2->a + cpx2->b) * cpx3->b;
            C = (cpx3->b - cpx3->a) * cpx2->b;
            cpx->a += A - B;
            cpx->b += B - C;
            cpx++;
            cpx2++;
            cpx3++;
        }
    }

    float *output = lc->win;
    wtk_drft_ifft22(lc->drft, lc->OUTPUT_CACHE, output);
    for(i = 0; i < hop_size * 2; ++i){
        output[i] = output[i] / scale;
    }

    output = lc->win;
    for(i = 0; i < hop_size; ++i){
        input[i] = lc->output_cache[i] + *output;
        //wtk_debug("%d %.10f %.10f %.10f\n",i,input[i],lc->output_cache[i],*output);
        output++;
    }
    memcpy(lc->output_cache,lc->win + hop_size, hop_size * sizeof(float));
    memmove(lc->OUTPUT_CACHE,lc->OUTPUT_CACHE + hop_size + 1, (hop_size + 1) * (lc->N_BLK - 1) * sizeof(wtk_complex_t));
    memset(lc->OUTPUT_CACHE + (hop_size + 1) * (lc->N_BLK - 1), 0 , (hop_size + 1) * sizeof(wtk_complex_t));
    //complex_dump(lc->OUTPUT_CACHE,lc->N_BLK * (hop_size + 1));
    //print_float(lc->output_cache,128);
}
