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

    for(i = 0; i <= km->L; i++){
        for(j = 0; j < km->B; j++){
            km->P_b[i * km->B + j] = km->cfg->p_initial;
        }
    }

    for(i = 0; i < km->L; i++){
        km->half_window[i] = 1;
    }
    for(i = km->L; i < km->M; i++){
        km->half_window[i] = 0;
    }
}

qtk_ahs_kalman2_t *qtk_kalman2_new(qtk_ahs_kalman2_cfg_t *cfg){
    qtk_ahs_kalman2_t *km = (qtk_ahs_kalman2_t *)malloc(sizeof(qtk_ahs_kalman2_t));

    km->cfg = cfg;
    km->A2 = cfg->alpha * cfg->alpha;
    km->Phi_SS_smooth_factor= cfg->keep_m_gate;
    km->L = cfg->nbin - 1;
    km->M = 2 * km->L;
    km->B = cfg->B;

    km->x_b = (float*)wtk_calloc(km->M, sizeof(float));
    km->Phi_ss = (float*)wtk_calloc(cfg->nbin, sizeof(float));
    km->P_b = (float*)wtk_malloc(cfg->nbin * cfg->B * sizeof(float));
    km->X_b = (wtk_complex_t*)wtk_malloc(cfg->nbin * cfg->B * sizeof(wtk_complex_t));
    km->W_b = (wtk_complex_t*)wtk_malloc(cfg->nbin * cfg->B * sizeof(wtk_complex_t));
    km->mu_b = (float*)wtk_malloc(cfg->nbin * cfg->B * sizeof(float));
    km->E = (wtk_complex_t*)wtk_malloc(cfg->nbin * sizeof(wtk_complex_t));
    km->E_kb = (wtk_complex_t*)wtk_malloc(cfg->nbin * sizeof(wtk_complex_t));
    km->ifft_buf = (float*)wtk_malloc(km->M * sizeof(float));
    km->fft_buf = (wtk_complex_t*)wtk_malloc(cfg->nbin * sizeof(wtk_complex_t));
    km->half_window = (int*)wtk_malloc(km->M * sizeof(float));
    km->Phi_EE = (float*)wtk_malloc(cfg->nbin * sizeof(float));

    km->y = (float*)wtk_calloc(km->M, sizeof(float));
    km->e = (float*)wtk_calloc(km->L, sizeof(float));
    km->Y = (wtk_complex_t*)wtk_calloc(cfg->nbin, sizeof(wtk_complex_t));
    km->W = (float*)wtk_calloc(cfg->nbin, sizeof(float));
    km->E_res = (wtk_complex_t*)wtk_calloc(cfg->nbin, sizeof(wtk_complex_t));

    km->drft = wtk_drft_new2(km->M);
    qtk_kalman2_reset(km);
    return km;
}

void qtk_kalman2_delete(qtk_ahs_kalman2_t *km){
    wtk_free(km->x_b);
    wtk_free(km->Phi_ss);
    wtk_free(km->P_b);
    wtk_free(km->X_b);
    wtk_free(km->W_b);
    wtk_free(km->mu_b);
    wtk_free(km->half_window);
    wtk_free(km->E);
    wtk_free(km->E_kb);
    wtk_free(km->ifft_buf);
    wtk_free(km->fft_buf);
    wtk_free(km->Phi_EE);
    wtk_free(km->Y);
    wtk_free(km->y);
    wtk_free(km->e);
    wtk_free(km->E_res);
    wtk_free(km->W);
    wtk_drft_delete2(km->drft);
    wtk_free(km);
}
static void complex_dump(wtk_complex_t *c, int len){
    int i;
    for(i = 0; i < len; i++){
        printf("%.10f %.10f\n", c[i].a, c[i].b);
    }
}
void qtk_kalman2_update(qtk_ahs_kalman2_t *km){
    float *ifft_buf = km->ifft_buf;
    wtk_complex_t *fft_buf = km->fft_buf;
    wtk_complex_t *E_Kb = km->E_kb;
    wtk_complex_t *E = km->E;
    wtk_complex_t K_b;
    float A,B,C;
    int b,k,i;

    float *p1 = km->mu_b;
    wtk_complex_t *cp2 = km->X_b;
    float *p3 = km->P_b;
    wtk_complex_t *cp4 = km->W_b;
    wtk_complex_t *cp5 = km->W_b;
    float Beta = 1 - km->A2;
    for (b = 0; b < km->B; b++) {
        for (k = 0; k <= km->L; k++, p1++, cp2++, p3++, cp4++) {
            //wtk_complex_t X_conj = complex_conj(km->X_b[b][k]);
            //wtk_complex_t K_b = complex_mul(km->mu_b[b][k], X_conj);
            K_b.a = *p1 * cp2->a;
            K_b.b = *p1 * -cp2->b;
            float term = 1.0f - 0.5f * *p1 * (cp2->a * cp2->a + cp2->b * cp2->b);
            //wtk_debug("%.10f %.10f\n",km->A2 * term * (*p3) , Beta * complex_abs2(cp4));//,Beta * complex_abs2(cp4));
            *p3 = km->A2 * term * (*p3) + Beta * complex_abs2(cp4);

            A = (E[k].a + E[k].b) * K_b.a;
            B = (K_b.a + K_b.b) * E[k].b;
            C = (E[k].b - E[k].a) * K_b.b;
            E_Kb[k].a = A - B;
            E_Kb[k].b = B - C;
        }
        wtk_drft_ifft22(km->drft, E_Kb, ifft_buf);
        for (i = 0; i < km->M; i++) {
            ifft_buf[i] *= km->half_window[i] * 1.0 / km->M;
        }
        wtk_drft_fft22(km->drft, ifft_buf, fft_buf);
        for (k = 0; k <= km->L; k++, cp5++) {
            cp5->a = km->cfg->alpha * (cp5->a + fft_buf[k].a * km->M);
            cp5->b = km->cfg->alpha * (cp5->b + fft_buf[k].b * km->M);
        }
    }
    //wtk_debug("dump P_b\n");
    //print_float(km->P_b, km->B * (km->L + 1));
    //wtk_debug("dump W_b\n");
    //complex_dump(km->W_b, km->B * (km->L + 1));
}
void qtk_kalman2_feed(qtk_ahs_kalman2_t *km, float *x, float *d){
    int b,k,i;

    memmove(km->X_b + km->cfg->nbin, km->X_b, km->cfg->nbin * (km->cfg->B - 1) * sizeof(wtk_complex_t));
    wtk_drft_fft22(km->drft, x, km->X_b);
    for(i = 0; i < km->cfg->nbin; i++){
        km->X_b[i].a *= km->M;
        km->X_b[i].b *= km->M;
    }
    //wtk_debug("dump X_b\n");
    //complex_dump(km->X_b, km->B * (km->L + 1));
    wtk_complex_t *cp1 = km->W_b;
    wtk_complex_t *cp2 = km->X_b;
    wtk_complex_t *cp3 = km->Y;
    wtk_complex_zero(cp3, km->cfg->nbin);
    float A,B,C;
    for (b = 0; b < km->B; b++) {
        for (k = 0; k <= km->L; k++,cp1++,cp2++) {
            A = (cp1->a + cp1->b) * cp2->a;
            B = (cp2->a + cp2->b) * cp1->b;
            C = (cp1->b - cp1->a) * cp2->b;
            cp3[k].a += A - B;
            cp3[k].b += B - C;
        }
    }

    wtk_drft_ifft22(km->drft, cp3, km->y);
    for(i = 0; i < km->M; i++){
        km->y[i] /= km->M;
    }
    for(i = 0; i < km->L; i++){
        km->e[i] = d[i] - km->y[km->L + i];
    }

    memset(km->ifft_buf, 0, km->M * sizeof(float));
    memcpy(km->ifft_buf + km->L, km->e, km->L * sizeof(float));
    wtk_drft_fft22(km->drft, km->ifft_buf, km->E);

    float *p = km->Phi_ss;
    float scale = 1 - km->Phi_SS_smooth_factor;
    for(i = 0; i <= km->L; i++, p++){
        km->E[i].a *= km->M;
        km->E[i].b *= km->M;
        *p = km->Phi_SS_smooth_factor * *p + scale * complex_abs2(km->E + i);
    }
   //wtk_debug("dump Phi_ss\n");
   //print_float(km->Phi_ss, km->L + 1);
    cp1 = km->X_b;
    p = km->P_b;
    float *p2 = km->Phi_EE;
    float *p3 = km->mu_b;
    memset(km->Phi_EE,0, km->cfg->nbin*sizeof(float));
    for (b = 0; b < km->B; b++) {
        for (k = 0; k <= km->L; k++,cp1++,p++) {
            p2[k] += (cp1->a * cp1->a + cp1->b * cp1->b) * *p;
        }
    }
    for(k = 0; k <= km->L; k++){
        km->Phi_EE[k] += 2 * km->Phi_ss[k] / km->B;
    }
    //wtk_debug("dump Phi_EE\n");
    //print_float(km->Phi_EE, km->cfg->nbin);
    p = km->P_b;
    for (b = 0; b < km->B; b++) {
        for (k = 0; k <= km->L; k++,p3++,p++) {
            *p3 = *p / (km->Phi_EE[k] + 1e-10);
        }
    }
    //wtk_debug("dump mu_b\n");
    //print_float(km->mu_b, (km->L + 1) * km->B);
    if(km->cfg->use_res){
        memset(km->W,0, km->cfg->nbin * sizeof(float));
        float *p1 = km->mu_b;
        cp2 = km->X_b;
        for (b = 0; b < km->B; b++) {
            for (k = 0; k <= km->L; k++,p1++,cp2++) {
                km->W[k] += *p1 * complex_abs2(cp2);
            }
        }

        cp1 = km->E_res;
        p1 = km->W;
        cp2 = km->E;
        for(k = 0; k<= km->L; k++,cp1++,cp2++,p1++){
            *p1 = 1.0 - *p1;
            cp1->a = cp2->a * *p1;
            cp1->b = cp2->b * *p1;
        }
        //wtk_debug("dump E_res\n");
        //complex_dump(km->E_res, km->cfg->nbin);
        wtk_drft_ifft22(km->drft, km->E_res, km->y);
        for(i = 0; i < km->L; i++){
            km->e[i] = km->y[km->L + i] / km->M;
            km->y[i] = d[i] - km->e[i];
        }
        //print_float(km->e, km->L);
        //print_float(km->y, km->L);
    }
}