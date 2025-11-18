#include "qtk_kalman2.h"
static float complex_abs2(wtk_complex_t *c){
    float tmp = 0.0;
    tmp = c->a * c->a + c->b * c->b;
    return tmp;
}

// static void complex_div(float a,wtk_complex_t *b,wtk_complex_t *c){
//     wtk_complex_t tb;
//     tb.a = b->a + 1e-10;
//     tb.b = b->b;
//     double tmp = tb.a * tb.a + tb.b*tb.b;
//     float tmp1;
//     float tmp2;

//     tmp1 = a*tb.a/tmp;
//     tmp2 = -a*tb.b/tmp;

//     c->a = tmp1;
//     c->b = tmp2;
// }

void qtk_kalman2_reset(qtk_ahs_kalman2_t *km){
    int i,j;

    for(i = 0; i < km->nbin; i++){
        for(j = 0; j < km->B; j++){
            km->P_b[i * km->B + j] = km->cfg->p_initial;
        }
    }

    memset(km->half_window,0,sizeof(float)*(km->L + km->V));
    for(i = 0; i < km->L; i++){
        km->half_window[i] = 1;
    }
    km->index = 0;
    wtk_complex_zero(km->X_b,km->nbin * km->B);
    wtk_complex_zero(km->W_b,km->nbin * km->B);
    wtk_complex_zero(km->K_b,km->nbin * km->B);
    memset(km->Phi_ss,0,sizeof(float)*(km->nbin));
}

qtk_ahs_sound_effect_t* qtk_ahs_sound_effect_new(int frame_len) {
    qtk_ahs_sound_effect_t* se = (qtk_ahs_sound_effect_t*)wtk_malloc(sizeof(qtk_ahs_sound_effect_t));
    float rir_coeffs[] = {0.00389, -0.01002, 0.09672, 0.01157, -0.01086, -0.00118, -0.00547, -0.00372, -0.00398, -0.00387, -0.00377,
-0.00367, -0.00357, -0.00348, -0.00338, -0.00328, -0.00319, -0.00310, -0.00300, -0.00291, -0.00282,
-0.00273, -0.00264, -0.00256, -0.00247, -0.00239, -0.00242, -0.00157, -0.00407, 0.00248, -0.01301, 0.06506,
0.01534, -0.01203, -0.00321, -0.00159, -0.01513, 0.05581, 0.01456, -0.01508, -0.00321, -0.00825, -0.00606,
-0.00633, -0.00611, -0.00592, -0.00573, -0.00554, -0.00536, -0.00518, -0.00500, -0.00482, -0.00465,
-0.00448, -0.00432, -0.00415, -0.00399, -0.00384, -0.00368, -0.00353, -0.00346, -0.00273, -0.00464,
0.00066};
    se->N_tap = sizeof(rir_coeffs) / sizeof(float);

    se->rir = (float*)wtk_malloc(se->N_tap * sizeof(float));
    for (int i = 0; i < se->N_tap; i++) {
        se->rir[i] = 4.0f * rir_coeffs[i];
    }

    se->cache = (float*)wtk_calloc(se->N_tap - 1, sizeof(float));
    se->cache_size = se->N_tap - 1;

    se->Z = (float*)wtk_calloc(se->N_tap - 1, sizeof(float));
    se->x_extended = (float*)malloc((se->cache_size + frame_len) * sizeof(float));
    return se;
}

void qtk_ahs_sound_effect_delete(qtk_ahs_sound_effect_t *se){
    wtk_free(se->rir);
    wtk_free(se->cache);
    wtk_free(se->Z);
    wtk_free(se->x_extended);
    wtk_free(se);
}

void qtk_ahs_sound_effect_process(qtk_ahs_sound_effect_t* se, float* frame, int frame_len, float* output) {
    int chunk_size = frame_len;
    int N_tap = se->N_tap;

    float* x_extended = se->x_extended;
    memcpy(x_extended, se->cache, se->cache_size * sizeof(float));
    memcpy(x_extended + se->cache_size, frame, chunk_size * sizeof(float)); // 添加新帧
    float* Z = se->Z;
    memset(Z, 0, (N_tap - 1) * sizeof(float));

    for (int n = 0; n < se->cache_size + chunk_size; n++) {
        float xn = x_extended[n];
        float yn = se->rir[0] * xn + Z[0]; // b[0]*x[n] + Z0
        for (int i = 0; i < N_tap - 2; i++) {
            Z[i] = se->rir[i+1] * xn + Z[i+1];
        }
        Z[N_tap - 2] = se->rir[N_tap-1] * xn;
        if (n >= se->cache_size) {
            output[n - se->cache_size] = yn;
        }
    }
    memcpy(se->cache, x_extended + (se->cache_size + chunk_size - se->cache_size), se->cache_size * sizeof(float));
}

qtk_ahs_kalman2_t *qtk_kalman2_new(qtk_ahs_kalman2_cfg_t *cfg){
    qtk_ahs_kalman2_t *km = (qtk_ahs_kalman2_t *)malloc(sizeof(qtk_ahs_kalman2_t));

    km->cfg = cfg;
    km->A2 = cfg->alpha * cfg->alpha;
    km->Phi_SS_smooth_factor= cfg->Phi_SS_smooth_factor;
    km->L = cfg->L;
    km->M = cfg->M;
    km->B = cfg->B;
    km->V = km->M - km->L;
    km->nbin = km->M/2 + 1;

    km->x_cache = (float*)wtk_calloc(km->M, sizeof(float));
    km->d_cache = (float*)wtk_calloc(km->M, sizeof(float));
    km->e_cache = (float*)wtk_calloc(km->V * 2, sizeof(float));

    km->Phi_ss = (float*)wtk_calloc(km->nbin, sizeof(float));
    km->P_b = (float*)wtk_malloc(km->nbin * cfg->B * sizeof(float));
    km->X_b = (wtk_complex_t*)wtk_malloc(km->nbin * cfg->B * sizeof(wtk_complex_t));
    km->W_b = (wtk_complex_t*)wtk_malloc(km->nbin * cfg->B * sizeof(wtk_complex_t));
    km->K_b = (wtk_complex_t*)wtk_malloc(km->nbin * cfg->B * sizeof(wtk_complex_t));

    km->E = (wtk_complex_t*)wtk_malloc(km->nbin * sizeof(wtk_complex_t));
    km->E_kb = (wtk_complex_t*)wtk_malloc(km->nbin * sizeof(wtk_complex_t));
    km->ifft_buf = (float*)wtk_malloc(km->M * sizeof(float));
    km->fft_buf = (wtk_complex_t*)wtk_malloc(km->nbin * sizeof(wtk_complex_t));
    km->half_window = (int*)wtk_malloc((km->L + km->V) * sizeof(float));
    km->Phi_EE = (float*)wtk_malloc(km->nbin * sizeof(float));

    km->y = (float*)wtk_calloc(km->M, sizeof(float));
    km->y2 = (float*)wtk_calloc(km->M, sizeof(float));
    km->e = (float*)wtk_calloc(km->M - km->L, sizeof(float));
    km->e2 = (float*)wtk_calloc(km->M - km->L * 2 , sizeof(float));
    km->Y = (wtk_complex_t*)wtk_calloc(km->nbin, sizeof(wtk_complex_t));
    km->W = (float*)wtk_calloc(km->nbin, sizeof(float));
    km->E_res = (wtk_complex_t*)wtk_calloc(km->nbin, sizeof(wtk_complex_t));

    int Noverlap = km->M - km->L * 2;
    int i;
    km->win_length = 2 * Noverlap;
    km->window = (float*)wtk_malloc(km->win_length * sizeof(float));
    km->win_gain = (float*)wtk_malloc(Noverlap * sizeof(float));
    for(i = 0; i < km->win_length; i++){
        km->window[i] = 0.5 * (1 - cos(WTK_TPI * i / (km->win_length - 1)));
    }
    km->L_win = km->window;
    km->R_win = km->window + Noverlap;
    for(i = 0; i < Noverlap; i++){
        km->win_gain[i] = km->L_win[i] + km->R_win[i];
    }
    km->mu_b = (float*)wtk_malloc(sizeof(float) * km->B * km->nbin);

    km->drft = wtk_drft_new2(km->M);
    km->log_w = NULL;
    qtk_kalman2_reset(km);
    if(cfg->wb_buf){
        memcpy(km->W_b, cfg->wb_buf->data, km->B * km->nbin * sizeof(wtk_complex_t));
        memcpy(km->K_b, cfg->wb_buf->data + km->B * km->nbin * sizeof(wtk_complex_t), km->B * km->nbin * sizeof(wtk_complex_t));
        memcpy(km->Phi_ss, cfg->wb_buf->data + km->B * km->nbin * sizeof(wtk_complex_t) * 2, km->nbin * sizeof(float));
        memcpy(km->P_b, cfg->wb_buf->data + km->B * km->nbin * sizeof(wtk_complex_t) * 2 + km->nbin * sizeof(float), km->B * km->nbin * sizeof(float));
    }

    km->sound_effect = NULL;
    km->freq_shift = NULL;

    if(cfg->use_fs){
        km->freq_shift = qtk_freq_shift_new(&(cfg->freq_shift));
    }
    if(cfg->use_se){
        km->sound_effect = qtk_ahs_sound_effect_new(km->L);
    }

    return km;
}

void qtk_kalman2_delete(qtk_ahs_kalman2_t *km){
    wtk_free(km->window);
    wtk_free(km->win_gain);
    wtk_free(km->x_cache);
    wtk_free(km->d_cache);
    wtk_free(km->e_cache);
    wtk_free(km->Phi_ss);
    wtk_free(km->P_b);
    wtk_free(km->X_b);
    wtk_free(km->W_b);
    wtk_free(km->K_b);
    wtk_free(km->half_window);
    wtk_free(km->E);
    wtk_free(km->E_kb);
    wtk_free(km->ifft_buf);
    wtk_free(km->fft_buf);
    wtk_free(km->Phi_EE);
    wtk_free(km->Y);
    wtk_free(km->y);
    wtk_free(km->y2);
    wtk_free(km->e);
    wtk_free(km->e2);
    wtk_free(km->E_res);
    wtk_free(km->W);
    wtk_drft_delete2(km->drft);
    wtk_free(km->log_w);
    wtk_free(km->mu_b);
    if(km->sound_effect){
        qtk_ahs_sound_effect_delete(km->sound_effect);
    }
    if(km->freq_shift){
        qtk_freq_shift_delete(km->freq_shift);
    }
    wtk_free(km);
}
static void complex_dump(wtk_complex_t *c, int len){
    int i;
    for(i = 0; i < len; i++){
        printf("[%d]=%.6g %.6g\n", i,c[i].a, c[i].b);
    }
}

static float get_mean(wtk_complex_t *cpx, int len){
    int i;
    float sum = 0.0;
    for(i = 0; i < len; i++,cpx++){
        sum += sqrt(cpx->a * cpx->a + cpx->b * cpx->b);
    }
    return sum/len;
}

void qtk_kalman2_update(qtk_ahs_kalman2_t *km){

    if(get_mean(km->X_b,km->nbin * km->B) < km->cfg->update_threshold){
        return;
    }

    float *ifft_buf = km->ifft_buf;
    wtk_complex_t *fft_buf = km->fft_buf;
    wtk_complex_t *E_Kb = km->E_kb;
    wtk_complex_t *E = km->E;
    wtk_complex_t K_b;
    float A,B,C;
    int b,k,i;

    wtk_complex_t *cp1 = km->K_b;
    wtk_complex_t *cp2 = km->X_b;
    float *p3 = km->P_b;
    wtk_complex_t *cp4 = km->W_b;
    wtk_complex_t *cp5 = km->W_b;
    float Beta = 1 - km->A2;
    float fa,fb;
    float scale = 1.0 * km->V/km->M;
    for (b = 0; b < km->B; b++) {
        for (k = 0; k < km->nbin; k++, cp1++, cp2++, p3++, cp4++) {
            //wtk_complex_t X_conj = complex_conj(km->X_b[b][k]);
            //wtk_complex_t K_b = complex_mul(km->mu_b[b][k], X_conj);
            fa = cp2->a * scale;
            fb = cp2->b * scale;
            K_b.a = fa * cp1->a - fb * cp1->b;
            float term = 1.0f - K_b.a;
            //*p3 = km->A2 * term * (*p3) + Beta * complex_abs2(cp4) + 1e-7;
            *p3 = km->A2 * term * (*p3) + Beta * complex_abs2(cp4);
            if(*p3 > 1e3){
                *p3 = 1e3;
            }else if(*p3 < 1e-7){
                *p3 = 1e-7;
            }

            A = (E[k].a + E[k].b) * cp1->a;
            B = (cp1->a + cp1->b) * E[k].b;
            C = (E[k].b - E[k].a) * cp1->b;
            E_Kb[k].a = A - B;
            E_Kb[k].b = B - C;
        }
        wtk_drft_ifft22(km->drft, E_Kb, ifft_buf);
        for (i = 0; i < km->M; i++) {
            ifft_buf[i] *= km->half_window[i] * 1.0 / km->M;
        }
        wtk_drft_fft22(km->drft, ifft_buf, fft_buf);
        for (k = 0; k < km->nbin; k++, cp5++) {
            cp5->a += fft_buf[k].a * km->M;
            cp5->b += fft_buf[k].b * km->M;
        }
    }
    // wtk_debug("dump P_b\n");
    // print_float(km->P_b, km->B * (km->L + 1));
    // wtk_debug("dump W_b\n");
    // complex_dump(km->W_b, km->B * (km->L + 1));
}

static void km2_log_print(qtk_ahs_kalman2_t *km){
    float wpower = 0.0,ppower = 0.0;
    int i,j;
    if(!km->log_w){
        km->log_w = (float*)wtk_malloc(km->M * km->cfg->B * sizeof(float));
    }

    float *p;
    wtk_complex_t *cp = km->W_b;
    for(i = 0; i < km->B; i++){
        wtk_drft_ifft22(km->drft, cp, km->log_w);
        for(j = 0; j < km-> L; j++){
        printf("%f ",km->log_w[j]/km->M);
        }
        printf("\n");
        for(j = 0; j < km->nbin; j++, cp++){
        wpower += sqrtf(cp->a * cp->a + cp->b * cp->b);
        }
    }
    wpower /= (km->nbin * km->B);
    p = km->P_b;
    for(i = 0; i < km->B; i++){
        for(j = 0; j < km->nbin; j++,p++){
        ppower += fabs(*p);
        }
    }
    wtk_debug("wpower=%f,ppower=%f\n",wpower,ppower);
}

void qtk_kalman2_feed(qtk_ahs_kalman2_t *km, float *x, float *d){
    int b,k,i;
    int nb = km->M - km->L;
    km->index++;
    memmove(km->x_cache, km->x_cache + km->L, nb * sizeof(float));
    memcpy(km->x_cache + nb, x, km->L * sizeof(float));
    memmove(km->X_b + km->nbin, km->X_b, km->nbin * (km->cfg->B - 1) * sizeof(wtk_complex_t));
    wtk_drft_fft22(km->drft, km->x_cache, km->X_b);
    for(i = 0; i < km->nbin; i++){
        km->X_b[i].a *= km->M;
        km->X_b[i].b *= km->M;
    }

    memmove(km->d_cache, km->d_cache + km->L, nb * sizeof(float));
    memcpy(km->d_cache + nb, d, km->L * sizeof(float));

    //wtk_debug("dump X_b\n");
    //complex_dump(km->X_b, km->B * km->nbin);
    wtk_complex_t *cp1 = km->W_b;
    wtk_complex_t *cp2 = km->X_b;
    wtk_complex_t *cp3 = km->Y;
    wtk_complex_zero(cp3, km->nbin);
    float A,B,C;
    for (b = 0; b < km->B; b++) {
        for (k = 0; k < km->nbin; k++,cp1++,cp2++) {
            A = (cp1->a + cp1->b) * cp2->a;
            B = (cp2->a + cp2->b) * cp1->b;
            C = (cp1->b - cp1->a) * cp2->b;
            cp3[k].a += A - B;
            cp3[k].b += B - C;
        }
    }
    //wtk_debug("dump Y\n");
    //complex_dump(cp3, km->nbin);
    wtk_drft_ifft22(km->drft, cp3, km->y);
    for(i = 0; i < km->M; i++){
        km->y[i] /= km->M;
    }
    for(i = 0; i < nb; i++){
        km->e[i] = km->d_cache[km->L + i] - km->y[km->L + i];
    }

    memset(km->ifft_buf, 0, km->M * sizeof(float));
    memcpy(km->ifft_buf + km->L, km->e, nb * sizeof(float));
    wtk_drft_fft22(km->drft, km->ifft_buf, km->E);

    float *p = km->Phi_ss;
    float scale = 1 - km->Phi_SS_smooth_factor;
    for(i = 0; i < km->nbin; i++, p++){
        km->E[i].a *= km->M;
        km->E[i].b *= km->M;
        *p = km->Phi_SS_smooth_factor * *p + scale * complex_abs2(km->E + i);
    }
    //wtk_debug("dump Phi_ss\n");
    //print_float(km->Phi_ss, km->nbin);
    cp1 = km->X_b;
    p = km->P_b;
    float *p2 = km->Phi_EE;
    memset(km->Phi_EE,0, km->nbin*sizeof(float));
    for (b = 0; b < km->B; b++) {
        for (k = 0; k < km->nbin; k++,cp1++,p++) {
            p2[k] += (cp1->a * cp1->a + cp1->b * cp1->b) * *p;
        }
    }
    scale = km->V * 1.0 /km->M;
    for(k = 0; k < km->nbin; k++){
        km->Phi_EE[k] =  scale * km->Phi_EE[k] + km->Phi_ss[k];
    }
    //wtk_debug("dump Phi_EE\n");
    //print_float(km->Phi_EE, km->nbin);
    cp3 = km->K_b;
    cp1 = km->X_b;
    p = km->P_b;
    for (b = 0; b < km->B; b++) {
        for (k = 0; k < km->nbin; k++,cp3++,p++,cp1++) {
            cp3->a = *p * cp1->a * scale / (km->Phi_EE[k] + 1e-10);
            cp3->b = *p * -cp1->b * scale / (km->Phi_EE[k] + 1e-10);
        }
    }
    // wtk_debug("dump K_b\n");
    // complex_dump(km->K_b, km->nbin * km->B);
    if(km->cfg->use_res){
        float *p3 = km->mu_b;
        p = km->P_b;
        for (b = 0; b < km->B; b++) {
            for (k = 0; k < km->nbin; k++,p3++,p++) {
                *p3 =scale * *p / (km->Phi_EE[k] + 1e-10);
            }
        }

        memset(km->W,0, km->nbin * sizeof(float));
        float *p1 = km->mu_b;
        cp2 = km->X_b;
        for (b = 0; b < km->B; b++) {
            for (k = 0; k < km->nbin; k++,p1++,cp2++) {
                km->W[k] += *p1 * complex_abs2(cp2);
            }
        }

        cp1 = km->E_res;
        p1 = km->W;
        cp2 = km->E;
        for(k = 0; k < km->nbin; k++,cp1++,cp2++,p1++){
            *p1 = 1.0 - *p1;
            cp1->a = cp2->a * *p1;
            cp1->b = cp2->b * *p1;
        }
        //wtk_debug("dump E_res\n");
        //complex_dump(km->E_res, km->cfg->nbin);
        wtk_drft_ifft22(km->drft, km->E_res, km->y);
        for(i = 0; i < km->V; i++){
            km->e[i] = km->y[km->L + i] / km->M;
            km->y[i] = km->d_cache[km->L + i] - km->e[i];
        }
        //wtk_debug("dump e\n");
        //print_float(km->e, km->L);
        //print_float(km->y, km->L);
    }

    b = km->M - km->L * 2;
    if(km->index > 1){
        for(k = 0; k < b; k++){
            km->e[k] = (km->e[k] * km->L_win[k] + km->e2[k] * km->R_win[k]) / km->win_gain[k];
            km->y[k] = (km->y[k] * km->L_win[k] + km->y2[k] * km->R_win[k]) / km->win_gain[k];
        }
        memcpy(km->e2,km->e + km->L, b * sizeof(float));
        memcpy(km->y2,km->y + km->L, b * sizeof(float));

        if(km->cfg->use_se){
            qtk_ahs_sound_effect_process(km->sound_effect, km->e, km->L, km->e);
        }else if(km->cfg->use_fs){
            qtk_freq_shift_feed(km->freq_shift, km->e);
        }
    }else{
        memcpy(km->e2,km->e + km->L, b * sizeof(float));
        memmove(km->e,km->e + b, km->L * sizeof(float));
        memcpy(km->y2,km->y + km->L, b * sizeof(float));
        memmove(km->y,km->y + b, km->L * sizeof(float));
    }
}
