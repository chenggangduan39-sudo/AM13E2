#include "qtk_freq_shift.h"

qtk_ahs_freq_shift_t *qtk_freq_shift_new(qtk_ahs_freq_shift_cfg_t* cfg){
    qtk_ahs_freq_shift_t* freq_shift = wtk_malloc(sizeof(qtk_ahs_freq_shift_t));
    freq_shift->cfg = cfg;

    int i,j,n,N_tap = cfg->N_tap;
    float *p;
    freq_shift->N_tap = N_tap;
    freq_shift->df = cfg->freq_shift;
    freq_shift->sample_rate = 16000;
    freq_shift->chunk_size = 128;
    n = N_tap  + freq_shift->chunk_size;
    //wtk_debug("%d\n",n);
    freq_shift->drft = wtk_drft_new(n);

    freq_shift->q_tap = wtk_calloc(N_tap, sizeof(float));
    freq_shift->blackman_win = wtk_calloc(N_tap, sizeof(float));
    freq_shift->Q_TAP = wtk_calloc(n/2 + 1, sizeof(wtk_complex_t));
    freq_shift->tmp_cpx = wtk_calloc(n/2 + 1, sizeof(wtk_complex_t));
    freq_shift->i_tap = wtk_calloc(N_tap/2 + 1, sizeof(float));

    p = freq_shift->blackman_win;
    j = 1 - N_tap;
    for(i = 0; i < N_tap; i++){
        *p = 0.42 + 0.5 * cos(PI*j/(N_tap - 1)) + 0.08 * cos(2*PI*j/(N_tap - 1));
        p++;
        j += 2;
    }
    //print_float(freq_shift->blackman_win, N_tap);
    p = freq_shift->q_tap;
    for(i = 0; i < N_tap; i++){
        *p = 2 * (i % 2)/ (PI * (i - N_tap/2) + 1e-9) * freq_shift->blackman_win[i];
        p++;
    }
    *(freq_shift->q_tap + N_tap/2) = 0;
    //print_float(freq_shift->q_tap, N_tap);
    freq_shift->tmp = wtk_calloc(n, sizeof(float));
    //wtk_debug("%d %d\n",n,n/2);
    freq_shift->Q = wtk_calloc(n, sizeof(float));
    freq_shift->I = wtk_calloc(N_tap/2 + freq_shift->chunk_size, sizeof(float));
    memcpy(freq_shift->tmp, freq_shift->q_tap, N_tap * sizeof(float));
    wtk_drft_fft2_x(freq_shift->drft, freq_shift->tmp, freq_shift->Q_TAP);
    for(i = 0; i < n/2 + 1; i++){
        freq_shift->Q_TAP[i].a *= n;
        freq_shift->Q_TAP[i].b *= n;
        //wtk_debug("%f %f\n",freq_shift->Q_TAP[i].a, freq_shift->Q_TAP[i].b);
    }

    freq_shift->I_cache = wtk_calloc(N_tap/2, sizeof(float));
    freq_shift->Q_cache = wtk_calloc(N_tap - 1, sizeof(float));
    freq_shift->offset = 0;
    freq_shift->n = n;
    freq_shift->theta = wtk_calloc(freq_shift->chunk_size, sizeof(float));
    return freq_shift;
}

void qtk_freq_shift_delete(qtk_ahs_freq_shift_t *freq_shift){
    wtk_drft_delete(freq_shift->drft);
    wtk_free(freq_shift->q_tap);
    wtk_free(freq_shift->Q_TAP);
    wtk_free(freq_shift->i_tap);
    wtk_free(freq_shift->I_cache);
    wtk_free(freq_shift->Q_cache);
    wtk_free(freq_shift->tmp);
    wtk_free(freq_shift->Q);
    wtk_free(freq_shift->tmp_cpx);
    wtk_free(freq_shift->I);
    wtk_free(freq_shift->theta);
    wtk_free(freq_shift->blackman_win);
    wtk_free(freq_shift);
}

void qtk_freq_shift_reset(qtk_ahs_freq_shift_t *freq_shift){

}

void qtk_freq_shift_feed(qtk_ahs_freq_shift_t *freq_shift, float *data){
    int i,n = freq_shift->n;
    float A,B,C;
    float *I = freq_shift->I;
    float *Q = freq_shift->Q;
    wtk_complex_t *p1 = freq_shift->tmp_cpx;
    wtk_complex_t *p2 = freq_shift->Q_TAP;
    //print_float(data,128);
    memset(freq_shift->I, 0, (freq_shift->N_tap/2 + freq_shift->chunk_size) * sizeof(float));
    memcpy(freq_shift->I + freq_shift->N_tap/2, data, freq_shift->chunk_size * sizeof(float));

    memset(freq_shift->tmp, 0, freq_shift->n * sizeof(float));
    memcpy(freq_shift->tmp, data, freq_shift->chunk_size * sizeof(float));
    wtk_drft_fft2_x(freq_shift->drft, freq_shift->tmp, freq_shift->tmp_cpx);
    for(i = 0; i < n/2 + 1; i++){
        p1->a *= n;
        p1->b *= n;
        A = (p1->a + p1->b) * p2->a;
        B = (p2->a + p2->b) * p1->b;
        C = (p1->b - p1->a) * p2->b;
        p1->a = A - B;//p1->a * p2->a - p1->b * p2->b;
        p1->b = B - C;//p1->a * p2->b + p1->b * p2->a;
        //wtk_debug("%f %f\n", p1->a, p1->b);
        p1++;
        p2++;
    }
    wtk_drft_ifft2_x(freq_shift->drft, freq_shift->tmp_cpx, freq_shift->Q);
    for(i = 0; i < freq_shift->n; ++i){
        *Q /= n;
        Q++;
    }
    //print_float(freq_shift->Q,freq_shift->n);
    //exit(0);
    for(i = 0; i < freq_shift->N_tap/2; i++){
        *I += freq_shift->I_cache[i];
        I++;
    }
    //print_float(freq_shift->I, freq_shift->N_tap/2 + freq_shift->chunk_size);
    for(i = 0; i <freq_shift->N_tap - 1; i++){
        freq_shift->Q[i] += freq_shift->Q_cache[i];
    }
    //print_float(freq_shift->Q, n);
    memcpy(freq_shift->I_cache, freq_shift->I + freq_shift->chunk_size, freq_shift->N_tap/2 * sizeof(float));
    memcpy(freq_shift->Q_cache, freq_shift->Q + freq_shift->chunk_size, (freq_shift->N_tap - 1) * sizeof(float));

    for(i = 0; i < freq_shift->chunk_size; i++){
        freq_shift->theta[i] = 2 * PI * freq_shift->df * ((freq_shift->offset + i) * 1.0f / freq_shift->sample_rate);
    }
    //print_float(freq_shift->theta, freq_shift->chunk_size);
    float *theta = freq_shift->theta;
    for(i = 0; i < freq_shift->chunk_size; i++){
        data[i] = freq_shift->I[i] * cosf(*theta) - freq_shift->Q[i] * sinf(*theta);
        theta++;
    }
    //print_float(data, freq_shift->chunk_size);
    freq_shift->offset += freq_shift->chunk_size;
    //exit(0);
}

void qtk_freq_shift_feed2(qtk_ahs_freq_shift_t *freq_shift, short *data){
    int i;
    float b[128];
    for(i = 0; i < 128; i++){
        b[i] = data[i]/32768.0f;
    }
    qtk_freq_shift_feed(freq_shift, b);
}