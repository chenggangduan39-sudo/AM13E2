#include "qtk_gcc_estimate.h"

qtk_gcc_estimate_t *qtk_gcc_estimate_new(int rate){
    qtk_gcc_estimate_t *d_est = (qtk_gcc_estimate_t*)wtk_malloc(sizeof(qtk_gcc_estimate_t));

    int n = 32850;
    d_est->n = n;
    d_est->N = 2 * d_est->n;// -1;

    d_est->mic = wtk_strbuf_new(1024, 1);
    d_est->play = wtk_strbuf_new(1024, 1);

    d_est->drft = wtk_drft_new(d_est->N);
    d_est->st = 0;

    d_est->xn_ = wtk_calloc(d_est->N, sizeof(float));
    d_est->yn_ = wtk_calloc(d_est->N, sizeof(float));

    d_est->xk_conj = wtk_malloc(sizeof(wtk_complex_t) * (n + 1) * 2);
    d_est->yk_ = wtk_malloc(sizeof(wtk_complex_t) * (n + 1) * 2);    

    d_est->fs = rate;
    return d_est;
}

void qtk_gcc_estimate_delete(qtk_gcc_estimate_t *d_est){
    wtk_strbuf_delete(d_est->mic);
    wtk_strbuf_delete(d_est->play);
    wtk_drft_delete(d_est->drft);    
    wtk_free(d_est->xk_conj);
    wtk_free(d_est->yk_);
    wtk_free(d_est->xn_);
    wtk_free(d_est->yn_);
    wtk_free(d_est);
}

void qtk_gcc_estimate_reset(qtk_gcc_estimate_t *d_est){
    wtk_strbuf_reset(d_est->mic);
    wtk_strbuf_reset(d_est->play);
}

int qtk_gcc_estimate_feed_float(qtk_gcc_estimate_t *d_est, float *data, int len, int is_end){
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

int qtk_gcc_estimate(qtk_gcc_estimate_t *d_est){
    int i;
    wtk_complex_t cpx;
    float *output = (float*)wtk_malloc(sizeof(float) * d_est->N);
    float tmp;
    memcpy(d_est->xn_, d_est->play->data + d_est->st, d_est->n * sizeof(float));
    memcpy(d_est->yn_, d_est->mic->data + d_est->st, d_est->n * sizeof(float));
    wtk_drft_fft2(d_est->drft,d_est->xn_,d_est->xk_conj);
    for(i = 0; i < d_est->n + 1; ++i){
        d_est->xk_conj[i].a = d_est->xk_conj[i].a * d_est->N;
        d_est->xk_conj[i].b = d_est->xk_conj[i].b * d_est->N;
        //wtk_debug("%d %f %f\n",i,d_est->xk_conj[i].a,d_est->xk_conj[i].b);
    }
    wtk_drft_fft2(d_est->drft,d_est->yn_,d_est->yk_);
    for(i = 0; i < d_est->n + 1; ++i){
        d_est->yk_[i].a = d_est->yk_[i].a * d_est->N;
        d_est->yk_[i].b = -d_est->yk_[i].b * d_est->N;
        //wtk_debug("%d %f %f\n",i,d_est->yk_[i].a,d_est->yk_[i].b);
        cpx.a = d_est->xk_conj[i].a * d_est->yk_[i].a - d_est->xk_conj[i].b * d_est->yk_[i].b;
        cpx.b = d_est->xk_conj[i].a * d_est->yk_[i].b + d_est->xk_conj[i].b * d_est->yk_[i].a;
        tmp = sqrtf(cpx.a * cpx.a + cpx.b *cpx.b);
        d_est->xk_conj[i].a = cpx.a/tmp;
        d_est->xk_conj[i].b = cpx.b/tmp;
        //wtk_debug("%d %f %f\n",i,d_est->xk_conj[i].a,d_est->xk_conj[i].b);
    }

    wtk_drft_ifft2(d_est->drft, d_est->xk_conj, output);
    for(i = 0; i < d_est->N; ++i){
        output[i] = output[i] / d_est->N;
    }
    float *output2 = (float*)wtk_malloc(sizeof(float) * (d_est->N + 1));
    memcpy(output2,output + d_est->n,d_est->n*sizeof(float));
    memcpy(output2 + d_est->n,output,(d_est->n + 1)*sizeof(float));

    int idx = 0;
    float max = -1000;
    for(i = 0; i < d_est->N + 1; i++){
        if(fabs(output2[i]) > max){
            max = fabs(output2[i]);
            idx = i;
        }
    }

    d_est->recommend_delay = idx - d_est->n;
    if(d_est->recommend_delay < 0){
        d_est->recommend_delay = -d_est->recommend_delay;
    }
    // FILE *fp = fopen("rir1.txt","w+");
    // for(i = 0; i < d_est->N+1; i++){
    //     fprintf(fp, "%f ", output2[i]);
    // }
    // fclose(fp);
    d_est->res = output2;
    wtk_free(output);
    // wtk_free(output2);

    return 0;
}

int qtk_gcc_estimate_feed(qtk_gcc_estimate_t *d_est, short *data, int len, int is_end){
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
