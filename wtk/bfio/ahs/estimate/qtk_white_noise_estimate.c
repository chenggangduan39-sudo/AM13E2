#include "qtk_white_noise_estimate.h"

void qtk_white_noise_estimate_ref_load(qtk_white_noise_estimate_t* d_est,char *fn){
    d_est->ref = (float*)wtk_malloc(sizeof(float) * d_est->n);
    d_est->file = fopen(fn,"rb");

    //fread(d_est->ref,sizeof(float),d_est->n * d_est->n,file);
    //fclose(file);
}

qtk_white_noise_estimate_t *qtk_white_noise_estimate_new(qtk_rir_estimate_cfg_t* cfg){
    qtk_white_noise_estimate_t *d_est = (qtk_white_noise_estimate_t*)wtk_malloc(sizeof(qtk_white_noise_estimate_t));

    d_est->rt60 = 0.2;
    int n = 0.2 * cfg->rate;
    d_est->n = n;
    d_est->N = 2 * d_est->n;// -1;

    d_est->mic = wtk_strbuf_new(1024, 1);
    d_est->play = wtk_strbuf_new(1024, 1);

    d_est->xn_ = wtk_calloc(d_est->N, sizeof(float));
    d_est->yn_ = wtk_calloc(d_est->N, sizeof(float));

    //d_est->fft = wtk_rfft_new(d_est->N / 2);
    //d_est->F = wtk_calloc(d_est->N, sizeof(float));
    d_est->xk_conj = wtk_malloc(sizeof(wtk_complex_t) * (n + 1) * 2);
    d_est->yk_ = wtk_malloc(sizeof(wtk_complex_t) * (n + 1) * 2);

    d_est->drft = wtk_drft_new2(d_est->N);
    d_est->st = 0;

    d_est->res = wtk_calloc(d_est->n, sizeof(float));

    if(cfg->ref_fn){
        qtk_white_noise_estimate_ref_load(d_est,cfg->ref_fn);
    }

    return d_est;
}

void qtk_white_noise_estimate_delete(qtk_white_noise_estimate_t *d_est){
    wtk_strbuf_delete(d_est->mic);
    wtk_strbuf_delete(d_est->play);
    wtk_drft_delete2(d_est->drft);
    wtk_free(d_est->xk_conj);
    wtk_free(d_est->yk_);
    wtk_free(d_est->ref);
    wtk_free(d_est->res);
    wtk_free(d_est->xn_);
    wtk_free(d_est->yn_);
    wtk_free(d_est);
}

void qtk_white_noise_estimate_reset(qtk_white_noise_estimate_t *d_est){
    wtk_strbuf_reset(d_est->mic);
    wtk_strbuf_reset(d_est->play);
}

int qtk_white_noise_estimate_feed_float(qtk_white_noise_estimate_t *d_est, float *data, int len, int is_end){
    int i;
    float fv1,fv2;
    int channel = 2;
    for (i = 0; i < len; ++i) {
        fv1 = data[0];
        fv2 = data[1];
        wtk_strbuf_push(d_est->mic, (char *)&(fv1), sizeof(float));
        wtk_strbuf_push(d_est->play, (char *)&(fv2), sizeof(float));
        data += channel;
    }
    return 0;
}

static float *cross_correlation_(qtk_white_noise_estimate_t *d_est, float *xn, float *yn){
    int i;
    float *output = (float*)wtk_malloc(sizeof(float) * d_est->N);
    wtk_complex_t cpx;

    wtk_drft_fft22(d_est->drft,d_est->xn_,d_est->xk_conj);
    for(i = 0; i < d_est->n + 1; ++i){
        d_est->xk_conj[i].a = d_est->xk_conj[i].a * 6400;
        d_est->xk_conj[i].b = -d_est->xk_conj[i].b * 6400;
        //wtk_debug("%d %f %f\n",i,d_est->xk_conj[i].a,d_est->xk_conj[i].b);
    }
    wtk_drft_fft22(d_est->drft,d_est->yn_,d_est->yk_);
    for(i = 0; i < d_est->n + 1; ++i){
        d_est->yk_[i].a = d_est->yk_[i].a * 6400;
        d_est->yk_[i].b = d_est->yk_[i].b * 6400;
        //wtk_debug("%d %f %f\n",i,d_est->yk_[i].a,d_est->yk_[i].b);
        cpx.a = d_est->xk_conj[i].a * d_est->yk_[i].a - d_est->xk_conj[i].b * d_est->yk_[i].b;
        cpx.b = d_est->xk_conj[i].a * d_est->yk_[i].b + d_est->xk_conj[i].b * d_est->yk_[i].a;
        d_est->xk_conj[i].a = cpx.a;
        d_est->xk_conj[i].b = cpx.b;
        //wtk_debug("%d %f %f\n",i,d_est->xk_conj[i].a,d_est->xk_conj[i].b);
    }

    wtk_drft_ifft22(d_est->drft, d_est->xk_conj, output);
    for(i = 0; i < d_est->N; ++i){
        output[i] = output[i] / d_est->N;
    }
    //print_float(output, d_est->N);
    return output;
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

int qtk_white_noise_estimate(qtk_white_noise_estimate_t *d_est){
    memcpy(d_est->xn_, d_est->play->data + d_est->st, d_est->n * sizeof(float));
    memcpy(d_est->yn_, d_est->mic->data + d_est->st, d_est->n * sizeof(float));
    float *R_yf = cross_correlation_(d_est, d_est->xn_, d_est->yn_);

    //TODO
    //generte ref 
    // memcpy(d_est->xn_, d_est->play->data + d_est->st, d_est->n * sizeof(float));
    // memcpy(d_est->yn_, d_est->play->data + d_est->st, d_est->n * sizeof(float));
    // float *r_ff = cross_correlation_(d_est, d_est->xn_, d_est->yn_);
    // toeplitz_inverse

    //R_yf * ref
    int i,j,idx = 0;
    float *o = d_est->res;
    float *p, max = -1000.0;
    for(i = 0; i < d_est->n; ++i){
        p = R_yf;
        fread(d_est->ref,sizeof(float),d_est->n,d_est->file);
        for(j = 0; j < d_est->n; ++j){
            *o += *p * *(d_est->ref + j);
            p++;
        }
        o++;
    }

    i = d_est->n * 0.01;
    int *idxs = topk(d_est->res, d_est->n, i);
    float thresh = fabs(*(d_est->res + *(idxs + i - 1)));
    int a = *(idxs + i);
    int b = d_est->n - 1;
    i = (((a) < (b)) ? (a) : (b));

    p = d_est->res;
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
    p = d_est->res;
    for(i = 0; i < *(idxs) + 1; i++){
        if(*p > 1e-3){
            if((i - 16) > 0){
                st = p - 16;
                cnt = 16;
            }else{
                st = p;
                cnt = i + 1;
            }
            if((i + 16) > d_est->n){
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
    p = d_est->res;
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

    d_est->recommend_delay = idx;
    wtk_free(R_yf);
    wtk_free(idxs);
    fclose(d_est->file);
    return 0;
}

int qtk_white_noise_estimate_feed(qtk_white_noise_estimate_t *d_est, short *data, int len, int is_end){
    int i;
    float fv1,fv2;
    int channel = 2;
    for (i = 0; i < len; ++i) {
        fv1 = data[0] * 1.0 / 32768.0;
        fv2 = data[1] * 1.0 / 32768.0;
        wtk_strbuf_push(d_est->mic, (char *)&(fv1), sizeof(float));
        wtk_strbuf_push(d_est->play, (char *)&(fv2), sizeof(float));
        data += channel;
    }
    return 0;
}
