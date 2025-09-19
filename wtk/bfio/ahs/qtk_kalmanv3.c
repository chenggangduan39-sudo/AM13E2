#include "qtk_kalmanv3.h"

void complex_dump(wtk_complex_t *c, int len){
    // int i;
    // for(i = 0; i < len; i++){
    //     printf("%f %f\n", c[i].a, c[i].b);
    // }
    int i;
    for(i = 0; i < len; i+=2){
        printf("%d %.10f %.10fj,",i, c[i].a, c[i].b);
        printf("%d %.10f %.10fj\n",i+1, c[i+1].a, c[i+1].b);
    }
}

void complex_dump3(wtk_complex_t *c, int len, int n){
    int i=0,j,z;
    int cnt = n;
    wtk_complex_t *tmp;
    for(i = 0; i < 129; i++){
        tmp = c + i;
        cnt = n;
        for(j = 0; j < n; j++){
            for(z = 0; z < cnt; z++){
                printf("(%.10f,%.10f),", (tmp->a),tmp->b);
                tmp += 129;
            }
            printf("\n");
            cnt--;
        }
        printf("\n");
    }
}

void complex_dump2(wtk_complex_t *c, int len, int n){
    int i,j;

    wtk_complex_t *tmp = wtk_malloc(len*sizeof(wtk_complex_t));
    wtk_complex_t *p1 = tmp;

    for(i = 0; i < 129; i++){
        for(j = 0; j < n; j++){
            p1->a = c[j*129+i].a;
            p1->b = c[j*129+i].b;
            p1++;
        }
    }

    for(i = 0; i < len; i+=2){
        printf("%d %.10f %.10fj,",i, tmp[i].a, tmp[i].b);
        printf("%d %.10f %.10fj\n",i+1, tmp[i+1].a, tmp[i+1].b);
    }

    wtk_free(tmp);
}

void complex_div(wtk_complex_t *a,wtk_complex_t *b){
    double tmp = b->a * b->a + b->b*b->b;
    float tmp1;
    float tmp2;

    tmp1 = (a->a*b->a + a->b*b->b)/tmp;
    tmp2 = (a->b*b->a - a->a*b->b)/tmp;

    b->a = tmp1;
    b->b = tmp2;
}

void complex_div_real(wtk_complex_t *a,float b){
    a->a = a->a/b;
    a->b = a->b/b;
}

void complex_backwards(wtk_complex_t *b){
    double tmp = b->a * b->a + b->b * b->b;
    float tmp1;
    float tmp2;

    tmp1 = b->a / tmp;
    tmp2 = -b->b / tmp;

    b->a = tmp1;
    b->b = tmp2;
}

void complex_div2(wtk_complex_t *a,wtk_complex_t *b,wtk_complex_t *c){
    double tmp = b->a * b->a + b->b*b->b;
    float tmp1;
    float tmp2;

    tmp1 = (a->a*b->a + a->b*b->b)/tmp;
    tmp2 = (a->b*b->a - a->a*b->b)/tmp;

    c->a = tmp1;
    c->b = tmp2;
}

double complex_abs(wtk_complex_t *c){
    double tmp = 0.0;
    tmp = c->a * c->a + c->b * c->b;
    return sqrt(tmp);
}

double complex_abs_scale(wtk_complex_t *c, float scale){
    double tmp = 0.0;

    tmp = scale * scale * (c->a * c->a  + c->b * c->b);
    return sqrt(tmp);
}

double complex_abs2(wtk_complex_t *c){
    double tmp = 0.0;
    tmp = c->a * c->a + c->b * c->b;
    return tmp;
}

qtk_res_cancel_t* _res_cancel_new(int nbin,float gamma){
    qtk_res_cancel_t *rc = (qtk_res_cancel_t*)wtk_malloc(sizeof(qtk_res_cancel_t));

    rc->nbin = nbin;

    rc->Se = (float*)wtk_calloc(nbin, sizeof(float));
    rc->hnled = (float*)wtk_calloc(nbin, sizeof(float));
    rc->Xed = (wtk_complex_t*)wtk_calloc(nbin, sizeof(wtk_complex_t));
    rc->gamma = gamma;
    return rc;
}

void _res_cancel_delete(qtk_res_cancel_t *rc){
    wtk_free(rc->hnled);
    wtk_free(rc->Se);
    wtk_free(rc->Xed);
    wtk_free(rc);
}

void _res_cancel_forward(qtk_res_cancel_t *rc,wtk_complex_t *out_frm, wtk_complex_t *ref_frm){
    float gamma = rc->gamma,alpha = 1.0 - gamma;
    int i;
    wtk_complex_t *p1,*p2,*p3;

    float A,B,C;
    p1 = rc->Xed;
    p3 = out_frm;
    p2 = ref_frm;

    for(i = 0; i < rc->nbin;i++){
        A = (p3->a + p3->b) * p2->a;
        B = (p2->a - p2->b) * p3->b;
        C = -(p3->b - p3->a) * p2->b;
        p1->a = gamma * p1->a + alpha * (A - B);
        p1->b = gamma * p1->b + alpha * (B - C);
        p1++;
        p3++;
        p2++;
    }
    //complex_dump(rc->Xed,rc->nbin);
    float *p = rc->Se;
    p1 = ref_frm;
    for(i = 0; i < rc->nbin;i++){
        *p = gamma * (*p) + alpha * (p1->a * p1->a + p1->b * p1->b);
        p++;
        p1++;
        p2++;
    }
    //print_float(rc->Se,rc->nbin);
    p = rc->hnled;
    p1 = rc->Xed;
    for(i = 0; i < rc->nbin;i++){
        *p = p1->a / (rc->Se[i] + 1e-10);
        if(*p < 0.05){
            *p = 0.05;
        }
        p++;
        p1++;
    }
    //print_float(rc->hnled,rc->nbin);
    p1 = out_frm;
    p2 = ref_frm;
    p = rc->hnled;
    for(i = 0; i < rc->nbin; i++){
        p1->a -= p2->a * (*p);
        p1->b -= p2->b * (*p);
        p++;
        p1++;
        p2++;
    }
    //complex_dump(out_frm,rc->nbin);
}

void qtk_kalman_tmp_reset(qtk_ahs_kalman_t *km){
    int i,m,n;
    float* cp = km->pweyetmp;

    if(km->cfg->use_dg)
    {
        memset(km->pweyetmp,0, km->dg_row * km->dg_col * sizeof(float));
        for(m = 0; m < km->dg_row; m++){
            for(n = 0; n < km->dg_col; n++){
                if(m == n){
                    *cp = 1.0;
                }
                cp++;
            }
        }
    }else
    {
        if(!km->cfg->use_symmetric_ph){
            memset(km->pweyetmp,0, km->Ph_shape[0] * km->Ph_shape[1] * km->Ph_shape[2] * sizeof(float));
            for(i = 0; i < km->Ph_shape[0]; i++){
                for(m = 0; m < km->Ph_shape[1]; m++){
                    for(n = 0; n < km->Ph_shape[2]; n++){
                        if(m == n){
                            *cp = 1.0;
                        }
                        cp++;
                    }
                }
            }
        }else{
            int cnt = km->L;
            for(m = 0; m < km->L; m++){
                for(n = 0; n < cnt; n++){
                    for(i = 0; i < km->Ph_shape[0]; i++){
                        if(n == 0){
                            *cp = 1.0;
                        }
                        cp++;
                    }
                }
                cnt--;
            }
        }
    }
}

qtk_ahs_kalman_t *qtk_kalman_new(qtk_ahs_kalman_cfg_t *cfg, int batchsize, int nbin){
    qtk_ahs_kalman_t* km = (qtk_ahs_kalman_t*)wtk_malloc(sizeof(qtk_ahs_kalman_t));

    int L = cfg->L;
    km->cfg = cfg;
    km->nbin=nbin;
    //param init
    km->type = cfg->kalman_type;
    km->nonlinear_order = cfg->kalman_order;
    km->L_ = L;
    km->L = L * km->nonlinear_order;
    km->pworg = cfg->pworg;
    km->pvorg = cfg->pvorg;
    km->a = cfg->kalman_a;
    km->pwtmp = 1.0 / km->L * (1 - 0.999992 * 0.999992);
    km->kalman_thresh = cfg->kalman_thresh;
    km->n = 2 * (nbin - 1);

    km->dg_row = cfg->dg_row;
    km->dg_col = cfg->dg_col;
    km->dg_cnt = km->L/km->dg_row;

    if(km->type == 0){
        //cache init
        km->Ph = (wtk_complex_t*)wtk_malloc(sizeof(wtk_complex_t) * (batchsize * nbin) * km->L * km->L);
        km->Ph_shape[0] = batchsize * nbin;
        km->Ph_shape[1] = km->L;
        km->Ph_shape[2] = km->L;
        //tmp init
        km->pweyetmp = (float*)wtk_malloc(sizeof(float) * (batchsize * nbin) * km->L * km->L);
        km->tmp1 = (wtk_complex_t*)wtk_malloc(sizeof(wtk_complex_t) * (batchsize * nbin));
        km->tmp2 = (wtk_complex_t*)wtk_malloc(sizeof(wtk_complex_t) * (batchsize * nbin) * km->L);
        km->tmp3 = (wtk_complex_t*)wtk_malloc(sizeof(wtk_complex_t) * (batchsize * nbin) * km->L * km->L);

    }else{
        km->Ph = (wtk_complex_t*)wtk_malloc(sizeof(wtk_complex_t) * (batchsize * nbin) * 1);
        km->Ph_shape[0] = batchsize * nbin;
        km->Ph_shape[1] = 1;
        km->Ph_shape[2] = 1;

        km->pweyetmp = NULL;
        km->tmp1 = NULL;
        km->tmp2 = (wtk_complex_t*)wtk_malloc(sizeof(wtk_complex_t) * (batchsize * nbin));
        km->tmp3 = NULL;
    }
    km->h_w = (wtk_complex_t*)wtk_malloc(sizeof(wtk_complex_t) * (batchsize * nbin) * km->L);
    km->Pv = (float*)wtk_malloc(sizeof(float) * (batchsize * nbin) * 1);
    km->Pw = (float*)wtk_malloc(sizeof(float) * (batchsize * nbin) * 1);
    //km->x_cache = (float*)wtk_malloc(sizeof(float) * 2 * (nbin - 1) * (L - 1));
    //km->x_inp = (float*)wtk_malloc(sizeof(float) * 2 * (nbin - 1) * L);
    km->x_cache = (wtk_complex_t*)wtk_malloc(sizeof(wtk_complex_t) * (batchsize * nbin) * km->L);
    km->x_inp = (wtk_complex_t*)wtk_malloc(sizeof(wtk_complex_t) * (batchsize * nbin) * km->L);

    km->e = (wtk_complex_t*)wtk_malloc(sizeof(wtk_complex_t) * (batchsize * nbin));
    km->r = (wtk_complex_t*)wtk_malloc(sizeof(wtk_complex_t) * (batchsize * nbin) * km->L);
    km->r2 = (wtk_complex_t*)wtk_malloc(sizeof(wtk_complex_t) * (batchsize * nbin));
    km->kg = (wtk_complex_t*)wtk_malloc(sizeof(wtk_complex_t) * (batchsize * nbin) * km->L);

    km->Pv_shape[0] = batchsize * nbin;
    km->Pv_shape[1] = 1;
    km->Pw_shape[0] = batchsize * nbin;
    km->Pw_shape[1] = 1;
    km->Pw_shape[2] = 1;
    km->h_w_shape[0] = batchsize * nbin;
    km->h_w_shape[1] = km->L;
    km->h_w_shape[2] = 1;
    km->x_cache_shape[0] = batchsize * nbin;
    km->x_cache_shape[1] = km->L;
    km->x_cache_len = (batchsize * nbin) * km->L;

    km->obser = (wtk_complex_t*)wtk_malloc(sizeof(wtk_complex_t) * (batchsize * nbin));
    km->NonLinear_SPEC = (wtk_complex_t*)wtk_malloc(sizeof(wtk_complex_t) * (batchsize * nbin) * km->L);
    km->fft_tmp = (float*)wtk_malloc(sizeof(float) * km->n * L);
    km->x_inp_len = 2 * (nbin - 1) * L;

    //output init
    km->lsty_hat = (wtk_complex_t*)wtk_malloc(sizeof(wtk_complex_t) * (batchsize * nbin));
    km->s_hat = (wtk_complex_t*)wtk_malloc(sizeof(wtk_complex_t) * (batchsize * nbin));

    km->sum = (float*)wtk_calloc(L,sizeof(float));

    km->rc = NULL;
    if(cfg->use_residual_cancellation){
        km->rc = _res_cancel_new(nbin, cfg->gamma);
    }

    km->Phi_SS = (float*)wtk_calloc(nbin, sizeof(float));
    km->Phi_EE = (float*)wtk_calloc(nbin, sizeof(float));
    km->Wiener = (float*)wtk_calloc(nbin, sizeof(float));
    km->HPH = (float*)wtk_calloc(nbin, sizeof(float));

    qtk_kalman_reset(km);
    if(km->type == 0){
        qtk_kalman_tmp_reset(km);
    }

    km->forPh = NULL;
    if(km->cfg->use_symmetric_ph){
        int i;
        km->forPh = (int*)wtk_malloc(sizeof(int) * km->L);
        km->forPh[0] = 0;
        for(i = 1; i < km->L; i++){
            km->forPh[i] = km->forPh[i - 1] + km->L - (i - 1);
        }
    }
    return km;
}

void qtk_kalman_reset(qtk_ahs_kalman_t *km){
    int i,m,n;
    wtk_complex_t* cp = km->Ph;
    float *p;

    for(i = 0; i < km->nbin; i++){
        km->Wiener[i] = 1.0;
    }

    wtk_complex_zero(km->r, km->nbin * km->L);
    //memset(km->x_cache,0,km->x_cache_shape[0] * km->x_cache_shape[1] * sizeof(float));
    wtk_complex_zero(km->x_cache,km->x_cache_shape[0] * km->x_cache_shape[1]);
    wtk_complex_zero(km->h_w,km->h_w_shape[0] * km->h_w_shape[1] * km->h_w_shape[2]);
    wtk_complex_zero(km->Ph,km->Ph_shape[0] * km->Ph_shape[1] * km->Ph_shape[2]);
    if(!km->cfg->use_symmetric_ph){
        for(i = 0; i < km->Ph_shape[0]; i++){
            for(m = 0; m < km->Ph_shape[1]; m++){
                for(n = 0; n < km->Ph_shape[2]; n++){
                    if(m == n){
                        cp->a = km->pworg;
                    }
                    cp++;
                }
            }
        }
    }else{
    int cnt = km->L;
        for(m = 0; m < km->L; m++){
            for(n = 0; n < cnt; n++){
                for(i = 0; i < km->Ph_shape[0]; i++){
                    if(n == 0){
                        cp->a = km->pworg;
                    }
                    cp++;
                }
            }
            cnt--;
        }
    }

    p = km->Pv;
    for(i = 0; i < km->Pv_shape[0] * km->Pv_shape[1]; i++){
        *p = km->pvorg;
        p++;
    }

    p = km->Pw;
    for(i = 0; i < km->Pw_shape[0] * km->Pw_shape[1] * km->Pw_shape[2]; i++){
        *p = km->pworg;
        p++;
    }

    km->mean = 0.0;
    km->last_idx = -4;
    memset(km->sum,0,sizeof(float)*km->L_);
}

void qtk_kalman_delete(qtk_ahs_kalman_t *km){
    wtk_free(km->x_cache);
    wtk_free(km->h_w);
    wtk_free(km->Ph);
    wtk_free(km->Pv);
    wtk_free(km->Pw);
    wtk_free(km->x_inp);
    wtk_free(km->e);
    wtk_free(km->r);
    wtk_free(km->r2);
    wtk_free(km->kg);
    if(km->tmp1){
        wtk_free(km->pweyetmp);
        wtk_free(km->tmp1);
        wtk_free(km->tmp3);
    }
    wtk_free(km->tmp2);
    wtk_free(km->lsty_hat);
    wtk_free(km->s_hat);

    wtk_free(km->NonLinear_SPEC);
    wtk_free(km->obser);
    wtk_free(km->fft_tmp);
    wtk_free(km->sum);
    if(km->rc){
        _res_cancel_delete(km->rc);
    }
    if(km->forPh){
        wtk_free(km->forPh);
    }
    wtk_free(km->Phi_SS);
    wtk_free(km->Phi_EE);
    wtk_free(km->Wiener);
    wtk_free(km->HPH);
    wtk_free(km);
}

static float get_mean2(qtk_ahs_kalman_t *km, wtk_complex_t *input, int nbin){
    float mean,sum = 0.0;
    int i,idx,cnt = km->L_;

    wtk_complex_t *cpx = input;
    for(i = 0; i < nbin; i++){
        sum += sqrt(cpx->a * cpx->a + cpx->b * cpx->b);
        cpx++;
    }
    mean = sum / nbin;
    sum = 0.0;
    if(km->last_idx < 0){
        idx = km->last_idx + cnt;
        km->sum[idx] = mean;
        km->last_idx++;
        for(i = 0; i < cnt; i++){
            sum += km->sum[i];
            //wtk_debug("%d +%f\n",idx,km->sum[i]);
        }
    }else{
        for(i = 0; i < cnt; i++){
            sum += km->sum[i];
            //wtk_debug("+%f\n",km->sum[i]);
        }
        sum = sum - km->sum[km->last_idx] + mean;
        //wtk_debug("-%f +%f\n",km->sum[km->last_idx],mean);
        km->sum[km->last_idx] = mean;
        km->last_idx++;
        if(km->last_idx >= km->L_){
            km->last_idx = 0;
        }
    }
    return sum / cnt;;
}

static void observation_update(qtk_ahs_kalman_t *km, int nbin){
    wtk_complex_t *cpx, *spec2;

    int i,j,k;
    cpx = km->x_cache;
    for(j = 0; j < km->L_; j++){
        for(i = 0; i < km->nonlinear_order; i++){
            for(k = 0; k < nbin; k++){
                
                spec2 = km->NonLinear_SPEC + k * km->L + j + i * km->L_;
                spec2->a = cpx->a;
                spec2->b = cpx->b;
                cpx++;
            }
        }
    }
}

void qtk_kalman_update3(qtk_ahs_kalman_t *km, wtk_complex_t *y, int nbin, int batchsize, wtk_complex_t *x_fft){
    wtk_complex_t *x_cache = km->x_cache;
    float *Pv;
    wtk_complex_t *p,*p1, *p2, *o, *Y;
    float scale, scale2;
    int i,j,length = nbin * batchsize;
    //x_inp init
    memmove(x_cache,x_cache + nbin * km->nonlinear_order, sizeof(float) * nbin * (km->L - km->nonlinear_order));
    memcpy(x_cache + nbin * (km->L - km->nonlinear_order), x_fft, sizeof(float) * nbin * km->nonlinear_order);
    //print_float(x,256);
    //print_float(km->x_inp,km->x_inp_len);
    float mean = get_mean2(km,x_fft,nbin);
    //wtk_debug("%f\n",mean);
    double dtmp;
    if(mean > km->kalman_thresh){
        observation_update(km,nbin);
        wtk_complex_zero(km->obser,nbin);
        //obser += torch.matmul(SPEC.unsqueeze(1),h_w[n]).squeeze(-1)
        for(j = 0; j < nbin ; j++){
            p1 = km->NonLinear_SPEC + j * km->L;
            p2 = km->h_w + j * km->L;
            o = km->obser + j;
            for(i = 0; i < km->L; i++){
                o->a += p1->a * p2->a - p1->b * p2->b;
                o->b += p1->a * p2->b + p1->b * p2->a;
                p1++;
                p2++;
            }
        }
        //e = Y - self.observation(x_inp)
        //129 * (1*8 8*1 1*1) 
        wtk_complex_zero(km->e, nbin);
        for(i = 0; i < nbin; i++){
            p1 = km->obser + i;
            o = km->e + i;
            Y = y + i;
            o->a = Y->a - p1->a;
            o->b = Y->b - p1->b;
        }
        //complex_dump(km->e, nbin);
        //exit(0);
        //Pv = self.a * Pv + (1 - self.a) * abs(e) ** 2
        Pv = km->Pv;
        scale = km->a;
        scale2 = 1 - km->a;
        for(i = 0; i < nbin; i++){
            dtmp = complex_abs(km->e + i);
            *Pv = scale * *Pv + scale2 * dtmp *dtmp;
            Pv++;
        }
        //replace_pv(km->Pv);
        //print_float(km->Pv, nbin);
        //self.P_k * H_k.conj()
        //129 * (8*8 8*1 8*1)
        //wtk_debug("dump SPEC\n");
        //complex_dump(km->NonLinear_SPEC, nbin * km->L);
        wtk_complex_zero(km->r, nbin * km->L);
        for(i = 0; i < nbin; i++){
            p1 = km->Ph + i;
            p2 = km->NonLinear_SPEC + i * km->L;
            o = km->r + i * km->L;
            for(j = 0; j < km->L; j++){
                o->a = p1->a * p2->a + p1->b * p2->b;
                o->b = -p1->a * p2->b + p1->b * p2->a;
                p2++;
                o++;
            }
        }
        //complex_dump(km->r, nbin * km->L);
        //(self.P_k * torch.norm(H_k, dim=1, keepdim=True) ** 2 + self.R_k)
        //129 * (1*8 8*1 1*1)

        Pv = km->Pv;
        float ftmp;
        wtk_complex_zero(km->r2, nbin);
        for(i = 0; i < nbin; i++){
            p1 = km->Ph + i;
            p2 = km->NonLinear_SPEC + i * km->L;
            o = km->r2 + i;
            ftmp = 0.0;
            for(j = 0; j < km->L; j++){
                ftmp += p2->a * p2->a + p2->b * p2->b;
                p2++;
            }

            o->a = p1->a * ftmp + *Pv;
            o->b = p1->b * ftmp;
            Pv++;
        }
        //complex_dump(km->r2, nbin);
        //K = self.P_k * H_k.conj() / (self.P_k * torch.norm(H_k, dim=1, keepdim=True) ** 2 + self.R_k)  # FxL
        //129 * (8*1 1*1 8*1) 1*1 opmimize?

        wtk_complex_zero(km->kg, nbin * km->L);
        for(i = 0; i < nbin; i++){
            p1 = km->r + i * km->L;
            p2 = km->r2 + i;
            o = km->kg + i * km->L;
            for(j = 0; j < km->L; j++){
                complex_div2(p1,p2,o);
                p1++;
                o++;
            }
        }

        //self.X_k += K * Error  # FxL # X_k-> h_w
        for(i = 0; i < nbin; i++){
            p1 = km->kg + i * km->L;
            p2 = km->e + i;
            o = km->h_w + i * km->L;
            for(j = 0; j < km->L; j++){
                o->a += p1->a * p2->a - p1->b * p2->b;
                o->b += p1->a * p2->b + p1->b * p2->a;
                p1++;
                o++;
            }
        }
        //complex_dump(km->h_w,km->L * nbin);
        //torch.matmul(self.h_w.permute(0, 2, 1).conj(), self.h_w)
        //129 * (8*1 1*1 8*1)
       // wtk_debug("test\n");
        //scale = km->a;
        //scale2 = (1 - km->a) * (1 - 0.9999 * 0.9999);
        scale2 = 1/km->L * (1 - 0.999992 * 0.999992);
        float *Pw = km->Pw;
        for(i = 0; i < nbin; i++){
            p1 = km->h_w + i * km->L;
            ftmp = 0.0;
            for(j = 0; j < km->L; j++){
                ftmp += p1->a * p1->a + p1->b * p1->b;
                p1++;
            }
            ftmp = sqrtf(ftmp);
            //*Pw = scale * (*Pw) + scale2 * ftmp;//TODO
            *Pw = scale2 * ftmp;
            Pw++;
        }
        //print_float(km->Pw,nbin);
        //tmp2 = torch.matmul(x_inp.unsqueeze(1), self.Ph)
        //129 * (1*8 8*8 1*8)
        //self.P_k = self.P_k - 1 / self.L * torch.sum(H_k * K, dim=1, keepdim=True) * self.P_k + self.Q_k  # Fx1
        wtk_complex_t cpx;
        wtk_complex_zero(km->tmp2, nbin);
        for(i = 0; i < nbin; i++){
            o = km->tmp2 + i;
            p1 = km->NonLinear_SPEC + i * km->L;
            p2 = km->kg + i * km->L;
            for(j = 0; j < km->L; j++){
                o->a += p1->a * p2->a - p1->b * p2->b;
                o->b += p1->a * p2->b + p1->b * p2->a;
                p1++;
                p2++;
            }
            o->a = o->a / km->L;
            o->b = o->b / km->L;

            p = km->Ph + i;
            cpx.a = p->a - p->a * o->a - p->b * o->b + *(km->Pw + i);
            cpx.b = p->b - p->a * o->b + p->b * o->a;
            p->a = cpx.a;
            p->b = cpx.b;
        }

        //wtk_complex_zero(km->lsty_hat,length);
        wtk_complex_zero(km->obser,nbin);
        //obser += torch.matmul(SPEC.unsqueeze(1),h_w[n]).squeeze(-1)
        for(j = 0; j < nbin ; j++){
            p1 = km->NonLinear_SPEC + j * km->L;
            p2 = km->h_w + j * km->L;
            o = km->obser + j;
            for(i = 0; i < km->L; i++){
                o->a += p1->a * p2->a - p1->b * p2->b;
                o->b += p1->a * p2->b + p1->b * p2->a;
                p1++;
                p2++;
            }
        }

        o = km->lsty_hat;
        p1 = km->obser;
        for(i = 0; i < nbin; i++){
            o->a = p1->a;
            o->b = p1->b;
            o++;
            p1++;
        }
    }else{
        wtk_complex_zero(km->lsty_hat,length);
    }

    for(i = 0; i < nbin; i++){
        km->s_hat[i].a = y[i].a - km->lsty_hat[i].a;
        km->s_hat[i].b = y[i].b - km->lsty_hat[i].b;
    }
    //complex_dump(km->lsty_hat, nbin * batchsize);
    //complex_dump(km->s_hat, nbin * batchsize);
}

void qtk_kalman_update2_3(qtk_ahs_kalman_t *km, wtk_complex_t *y,
                        int nbin, int batchsize,
                        wtk_complex_t *x_fft)
{
    wtk_complex_t tc = {1.0, 0.0};
    wtk_complex_t *x_cache = km->x_cache;
    wtk_complex_t *Ph = km->Ph;
    float *Pv;
    float *pweyetmp;
    wtk_complex_t *p1, *p2, *o, *Y, *o2;
    float scale, scale2;
    int i,j,k,m,n,z,length = nbin * batchsize;
    int step=8;

    memmove(x_cache,x_cache + nbin * km->nonlinear_order, sizeof(wtk_complex_t) * nbin * (km->L - km->nonlinear_order));
    memcpy(x_cache + nbin * (km->L - km->nonlinear_order), x_fft, sizeof(wtk_complex_t) * nbin * km->nonlinear_order);
    //print_float(x,256);
    //print_float(km->x_inp,km->x_inp_len);
    float mean = get_mean2(km,x_fft,nbin);
    //wtk_debug("================%f %d\n",mean,nbin * (km->L - km->nonlinear_order));
    double dtmp;
    float A,B,C;
    if(mean > km->kalman_thresh){
        //complex_dump(x_fft,nbin*km->nonlinear_order);
        //complex_dump(x_cache,nbin*km->L);
        observation_update(km,nbin);
        wtk_complex_zero(km->obser,nbin);
        p1 = km->NonLinear_SPEC;
        p2 = km->h_w;
        o = km->obser;
        o2 = km->e;
        Y = y;
        //obser += torch.matmul(SPEC.unsqueeze(1),h_w[n]).squeeze(-1)
        //e = Y - self.observation(x_inp)
        for(j = 0; j < nbin ; j++){
            for(i = 0; i < km->L; i++){
                A = (p1->a + p1->b) * p2->a;
                B = (p2->a + p2->b) * p1->b;
                C = (p1->b - p1->a) * p2->b;
                o->a += A - B;//p1->a * p2->a - p1->b * p2->b;
                o->b += B - C;//p1->a * p2->b + p1->b * p2->a;
                p1++;
                p2++;
            }
            o2->a = Y->a - o->a;
            o2->b = Y->b - o->b;
            o2++;
            Y++;
            o++;
        }
        //complex_dump(km->e, nbin);
        //Pv = self.a * Pv + (1 - self.a) * abs(e) ** 2
        Pv = km->Pv;
        scale = km->a;
        scale2 = 1 - km->a;
        for(i = 0; i < nbin; i++){
            dtmp = complex_abs2(km->e + i);
            *Pv = scale * *Pv + scale2 * dtmp;
            Pv++;
        }
        //exit(0);
        //replace_pv(km->Pv);
        //print_float(km->Pv, nbin);
        //r = torch.matmul(Ph, x_inp.conj().unsqueeze(-1))
        //129 * (8*8 8*1 8*1)
        //wtk_debug("dump SPEC\n");
        //complex_dump(km->NonLinear_SPEC, nbin * km->L);
        wtk_complex_zero(km->r, nbin * km->L);
        // qtk_kalman_array_zero(km,km->r,nbin,km->L,sl,el);
        p1 = km->Ph;
        o = km->r;
#if 1
        for(i = 0; i < nbin; i++){
            p1 = km->Ph+i*km->L*km->L;
            o = km->r+i*km->L;
            for(j = 0; j < km->L; j+=step)
            {
                p2 = km->NonLinear_SPEC + i * km->L + j;
                for(k=0;k<step;k++)
                {
                    for(z = 0; z < step; z++)
                    {
                        //p2->b = -p2->b;
                        A = (p1->a + p1->b) * p2->a;
                        B = (p2->a - p2->b) * p1->b;
                        C = -(p1->b - p1->a) * p2->b;
                        o->a += A - B;//p1->a * p2->a + p1->b * p2->b;
                        o->b += B - C;//-p1->a * p2->b + p1->b * p2->a;
                        p1++;
                        p2++;
                    }
                    p2-=step;
                    p1+=(km->L-step);
                    o++;
                }
                p1 = p1+step;
            }
        }
    #else
        for(i = 0; i < nbin; i++){
            for(j = 0; j < km->L; j++){
                p2 = km->NonLinear_SPEC + i * km->L;
                for(z = 0; z < km->L; z++){
                    //p2->b = -p2->b;
                    A = (p1->a + p1->b) * p2->a;
                    B = (p2->a - p2->b) * p1->b;
                    C = -(p1->b - p1->a) * p2->b;
                    o->a += A - B;//p1->a * p2->a + p1->b * p2->b;
                    o->b += B - C;//-p1->a * p2->b + p1->b * p2->a;
                    p1++;
                    p2++;
                }
                o++;
            }
        }
    #endif
        //complex_dump(km->r, nbin * km->L);
        //r2 = 1.0 / (torch.matmul(x_inp.unsqueeze(1), r) + Pv.unsqueeze(-1))
        //129 * (1*8 8*1 1*1)

        Pv = km->Pv;
        wtk_complex_zero(km->r2, nbin);
        p1 = km->NonLinear_SPEC;
        p2 = km->r;
        o = km->r2;
        for(i = 0; i < nbin; i++){
            for(j = 0; j < km->L; j++){
                A = (p1->a + p1->b) * p2->a;
                B = (p2->a + p2->b) * p1->b;
                C = (p1->b - p1->a) * p2->b;
                o->a += A - B;//p1->a * p2->a - p1->b * p2->b;
                o->b += B - C;//p1->a * p2->b + p1->b * p2->a;
                p1++;
                p2++;
            }
            o->a += *Pv;
            complex_div(&tc,o);
            Pv++;
            o++;
        }
        //complex_dump(km->r2, nbin);
        //kg = torch.matmul(r, r2)
        //129 * (8*1 1*1 8*1) 1*1 opmimize?

        wtk_complex_zero(km->kg, nbin * km->L);
        p1 = km->r;
        p2 = km->r2;
        o = km->kg;
        for(i = 0; i < nbin; i++){
            for(j = 0; j < km->L; j++){
                A = (p1->a + p1->b) * p2->a;
                B = (p2->a + p2->b) * p1->b;
                C = (p1->b - p1->a) * p2->b;
                o->a += A - B;//p1->a * p2->a - p1->b * p2->b;
                o->b += B - C;//p1->a * p2->b + p1->b * p2->a;
                p1++;
                o++;
            }
            p2++;
        }

        p1 = km->kg;
        p2 = km->e;
        o = km->h_w;
        for(i = 0; i < nbin; i++){
            for(j = 0; j < km->L; j++){
                A = (p1->a + p1->b) * p2->a;
                B = (p2->a + p2->b) * p1->b;
                C = (p1->b - p1->a) * p2->b;
                o->a += A - B;//p1->a * p2->a - p1->b * p2->b;
                o->b += B - C;//p1->a * p2->b + p1->b * p2->a;
                p1++;
                o++;
            }
            p2++;
        }
        //complex_dump(km->h_w,km->L * nbin);
        //torch.matmul(self.h_w.permute(0, 2, 1).conj(), self.h_w)
        //129 * (8*1 1*1 8*1)
       // wtk_debug("test\n");
        wtk_complex_zero(km->tmp1,nbin);
        scale = km->a;
        scale2 = (1 - km->a) * (1 - 0.9999 * 0.9999);
        float *Pw = km->Pw;
        p1 = km->h_w;
        p2 = km->h_w;
        for(i = 0; i < nbin; i++){
            o = km->tmp1 + i;
            for(j = 0; j < km->L; j++){
                A = (p1->a + p1->b) * p2->a;
                B = (p2->a - p2->b) * p1->b;
                C = -(p1->b - p1->a) * p2->b;
                o->a += A - B;//p1->a * p2->a + p1->b * p2->b;
                o->b += B - C;//-p1->a * p2->b + p1->b * p2->a;
                p1++;
                p2++;
            }
            *Pw = scale * (*Pw) + scale2 * o->a;//TODO
            Pw++;
        }
        //print_float(km->Pw,nbin);
        //tmp2 = torch.matmul(x_inp.unsqueeze(1), self.Ph)
        //129 * (1*8 8*8 1*8)
        wtk_complex_zero(km->tmp2, nbin * km->L);
    #if 1
        for(i = 0; i < nbin; i++){
            p2 = km->Ph + i * km->L * km->L;
            o = km->tmp2 + i * km->L;
            for(z = 0; z < km->L; z+=step){
                p1 = km->NonLinear_SPEC + i * km->L + z;
                o = km->tmp2 + i * km->L + z;
                for(k=0;k<step;k++)
                {
                    for(j = 0; j < step; j+=1)
                    {
                        A = (p1->a + p1->b) * p2->a;
                        B = (p2->a + p2->b) * p1->b;
                        C = (p1->b - p1->a) * p2->b;
                        o->a += A - B;//p1->a * p2->a - p1->b * p2->b;
                        o->b += B - C;//p1->a * p2->b + p1->b * p2->a;
                        p2++;
                        o++;
                        //p2+=km->L;
                    }
                    p1++;
                    o-=step;
                    p2+=(km->L-step);
                }
                p2=p2+step;
            }
        }
    #else
        for(i = 0; i < nbin; i++){
            p2 = km->Ph + i * km->L * km->L;
            p1 = km->NonLinear_SPEC + i * km->L;
            for(z = 0; z < km->L; z++){
                o = km->tmp2 + i * km->L;
                for(j = 0; j < km->L; j+=1){
                    A = (p1->a + p1->b) * p2->a;
                    B = (p2->a + p2->b) * p1->b;
                    C = (p1->b - p1->a) * p2->b;
                    o->a += A - B;//p1->a * p2->a - p1->b * p2->b;
                    o->b += B - C;//p1->a * p2->b + p1->b * p2->a;
                    o++;
                    p2++;
                    //p2+=km->L;
                }
                p1++;
            //o++;
            }
        }
    #endif
        //torch.matmul(kg, tmp2)
        //self.Ph = self.Ph - torch.matmul(kg, torch.matmul(x_inp.unsqueeze(1), self.Ph)) + self.pweyetmp * self.Pw
        //129 * (8*1 1*8 8*8)
        // wtk_complex_zero(km->tmp3,km->L * km->L * nbin);
        p1 = km->kg;
        o = km->tmp3;
    #if 1
        for(i = 0; i < nbin; i++)
        {
            o = km->tmp3 + i*km->L*km->L;
            for(j = 0; j < km->L; j+=step)
            {
                p2 = km->tmp2 + i * km->L+j;
                for(m=0;m<step;++m)
                {
                    for(z = 0; z < step; z++)
                    {
                        A = (p1->a + p1->b) * p2->a;
                        B = (p2->a + p2->b) * p1->b;
                        C = (p1->b - p1->a) * p2->b;
                        o->a = A - B;//p1->a * p2->a - p1->b * p2->b;
                        o->b = B - C;//p1->a * p2->b + p1->b * p2->a;
                        p2++;
                        o++;
                    }

                    p2-=step;

                    o+=(km->L-step);
                    p1++;
                }

                o+=step;
            }
        }
    #else
        for(i = 0; i < nbin; i++){
            for(j = 0; j < km->L; j++){
                p2 = km->tmp2 + i * km->L;
                for(z = 0; z < km->L; z++){
                    A = (p1->a + p1->b) * p2->a;
                    B = (p2->a + p2->b) * p1->b;
                    C = (p1->b - p1->a) * p2->b;
                    o->a = A - B;//p1->a * p2->a - p1->b * p2->b;
                    o->b = B - C;//p1->a * p2->b + p1->b * p2->a;
                    p2++;
                    o++;
                }
                // exit(0);
                p1++;
            }
        }
    #endif

        //complex_dump(km->Ph, nbin * km->L * km->L);
        wtk_complex_t *tmp;
        Pw = km->Pw;
        for(i = 0; i < nbin; i++){
            pweyetmp = km->pweyetmp+i*km->L*km->L;
            Ph = km->Ph+i*km->L*km->L;
            tmp = km->tmp3+i*km->L*km->L;
            for(m = 0; m < km->L; m+=step)
            {
                for(k=0;k<step;++k)
                {
                    for(n = 0; n < step; n++){
                        Ph->a = Ph->a - tmp->a + *pweyetmp * *Pw;
                        Ph->b = Ph->b - tmp->b;
                        Ph++;
                        tmp++;
                        pweyetmp++;
                    }
                    Ph+=(km->L-step);
                    tmp+=(km->L-step);
                    pweyetmp+=(km->L-step);
                }
                Ph+=step;
                tmp+=step;
                pweyetmp+=step;
            }
            Pw++;
        }

        //wtk_complex_zero(km->lsty_hat,length);
        wtk_complex_zero(km->lsty_hat,nbin);
        //obser += torch.matmul(SPEC.unsqueeze(1),h_w[n]).squeeze(-1)
        p1 = km->NonLinear_SPEC;
        p2 = km->h_w ;
        o = km->lsty_hat;
        for(j = 0; j < nbin ; j++){
            for(i = 0; i < km->L; i++){
                A = (p1->a + p1->b) * p2->a;
                B = (p2->a + p2->b) * p1->b;
                C = (p1->b - p1->a) * p2->b;
                o->a += A - B;//p1->a * p2->a - p1->b * p2->b;
                o->b += B - C;//p1->a * p2->b + p1->b * p2->a;
                p1++;
                p2++;
            }
            o++;
        }
    }else{
        wtk_complex_zero(km->lsty_hat,length);
    }

    for(i = 0; i < nbin; i++){
        km->s_hat[i].a = y[i].a - km->lsty_hat[i].a;
        km->s_hat[i].b = y[i].b - km->lsty_hat[i].b;
    }
    //complex_dump(km->lsty_hat, nbin * batchsize);
    //complex_dump(km->s_hat, nbin * batchsize);
}


void qtk_kalman_update2_4(qtk_ahs_kalman_t *km, wtk_complex_t *y,
                        int nbin, int batchsize,
                        wtk_complex_t *x_fft)
{
    wtk_complex_t tc = {1.0, 0.0};
    wtk_complex_t *x_cache = km->x_cache;
    wtk_complex_t *Ph = km->Ph;
    float *Pv;
    float *pweyetmp;
    wtk_complex_t *p1, *p2, *o, *Y, *o2;
    float scale, scale2;
    int i,j,k,m,n,z,length = nbin * batchsize;
    int step=8;

    memmove(x_cache,x_cache + nbin * km->nonlinear_order, sizeof(wtk_complex_t) * nbin * (km->L - km->nonlinear_order));
    memcpy(x_cache + nbin * (km->L - km->nonlinear_order), x_fft, sizeof(wtk_complex_t) * nbin * km->nonlinear_order);
    //print_float(x,256);
    //print_float(km->x_inp,km->x_inp_len);
    float mean = get_mean2(km,x_fft,nbin);
    //wtk_debug("================%f %d\n",mean,nbin * (km->L - km->nonlinear_order));
    double dtmp;
    float A,B,C;

    observation_update(km,nbin);
    wtk_complex_zero(km->obser,nbin);
    p1 = km->NonLinear_SPEC;
    p2 = km->h_w;
    o = km->obser;
    o2 = km->e;
    Y = y;
    //obser += torch.matmul(SPEC.unsqueeze(1),h_w[n]).squeeze(-1)
    //e = Y - self.observation(x_inp)
    for(j = 0; j < nbin ; j++){
        for(i = 0; i < km->L; i++){
            A = (p1->a + p1->b) * p2->a;
            B = (p2->a + p2->b) * p1->b;
            C = (p1->b - p1->a) * p2->b;
            o->a += A - B;//p1->a * p2->a - p1->b * p2->b;
            o->b += B - C;//p1->a * p2->b + p1->b * p2->a;
            p1++;
            p2++;
        }
        o2->a = Y->a - o->a;
        o2->b = Y->b - o->b;
        o2++;
        Y++;
        o++;
    }

    if(mean > km->kalman_thresh){
        //complex_dump(x_fft,nbin*km->nonlinear_order);
        //complex_dump(x_cache,nbin*km->L);

        // for(j = 0; j < km->L; j++){
        //     o = km->obser;
        //     for(i = 0; i < nbin; i++){
        //         A = (p1->a + p1->b) * p2->a;
        //         B = (p2->a + p2->b) * p1->b;
        //         C = (p1->b - p1->a) * p2->b;
        //         o->a += A - B;//p1->a * p2->a - p1->b * p2->b;
        //         o->b += B - C;//p1->a * p2->b + p1->b * p2->a;
        //         o++;
        //         p1++;
        //         p2++;
        //     }
        // }
        // for(i = 0; i < nbin; i++){
        //     o2->a = Y->a - o->a;
        //     o2->b = Y->b - o->b;
        //     o++;
        //     o2++;
        // }


        //complex_dump(km->e, nbin);
        //Pv = self.a * Pv + (1 - self.a) * abs(e) ** 2
        Pv = km->Pv;
        scale = km->a;
        scale2 = 1 - km->a;
        for(i = 0; i < nbin; i++){
            dtmp = complex_abs2(km->e + i);
            *Pv = scale * *Pv + scale2 * dtmp;
            Pv++;
        }
        //exit(0);
        //replace_pv(km->Pv);
        //print_float(km->Pv, nbin);
        //r = torch.matmul(Ph, x_inp.conj().unsqueeze(-1))
        //129 * (8*8 8*1 8*1)
        //wtk_debug("dump SPEC\n");
        //complex_dump(km->NonLinear_SPEC, nbin * km->L);
        wtk_complex_zero(km->r, nbin * km->L);
        // qtk_kalman_array_zero(km,km->r,nbin,km->L,sl,el);
        p1 = km->Ph;
        o = km->r;
#if 1
        for(i = 0; i < nbin; i++){
            // p1 = km->Ph+i*km->dg_row*km->dg_col;
            o = km->r+i*km->L;
            for(j = 0; j < km->L; j+=km->dg_row)
            {
                p2 = km->NonLinear_SPEC + i * km->L + j;
                for(k=0;k<km->dg_row;k++)
                {
                    for(z = 0; z < km->dg_col; z++)
                    {
                        //p2->b = -p2->b;
                        A = (p1->a + p1->b) * p2->a;
                        B = (p2->a - p2->b) * p1->b;
                        C = -(p1->b - p1->a) * p2->b;
                        o->a += A - B;//p1->a * p2->a + p1->b * p2->b;
                        o->b += B - C;//-p1->a * p2->b + p1->b * p2->a;
                        p1++;
                        p2++;
                    }
                    p2-=km->dg_col;
                    // p1+=(km->L-step);
                    o++;
                }
                // p1 = p1+step;
            }
        }
    #else
        for(i = 0; i < nbin; i++){
            for(j = 0; j < km->L; j++){
                p2 = km->NonLinear_SPEC + i * km->L;
                for(z = 0; z < km->L; z++){
                    //p2->b = -p2->b;
                    A = (p1->a + p1->b) * p2->a;
                    B = (p2->a - p2->b) * p1->b;
                    C = -(p1->b - p1->a) * p2->b;
                    o->a += A - B;//p1->a * p2->a + p1->b * p2->b;
                    o->b += B - C;//-p1->a * p2->b + p1->b * p2->a;
                    p1++;
                    p2++;
                }
                o++;
            }
        }
    #endif
        //complex_dump(km->r, nbin * km->L);
        //r2 = 1.0 / (torch.matmul(x_inp.unsqueeze(1), r) + Pv.unsqueeze(-1))
        //129 * (1*8 8*1 1*1)

        Pv = km->Pv;
        wtk_complex_zero(km->r2, nbin);
        p1 = km->NonLinear_SPEC;
        p2 = km->r;
        o = km->r2;
        for(i = 0; i < nbin; i++){
            float sum_a = 0;
            float sum_b = 0;
            for(j = 0; j < km->L; j++){
                A = (p1->a + p1->b) * p2->a;
                B = (p2->a + p2->b) * p1->b;
                C = (p1->b - p1->a) * p2->b;
                sum_a += A - B;//p1->a * p2->a - p1->b * p2->b;
                sum_b += B - C;//p1->a * p2->b + p1->b * p2->a;
                p1++;
                p2++;
            }
            km->HPH[i] = sum_a;
            o->a = o->a + sum_a + *Pv;
            o->b += sum_b;
            complex_div(&tc,o);
            Pv++;
            o++;
        }
        //complex_dump(km->r2, nbin);
        //kg = torch.matmul(r, r2)
        //129 * (8*1 1*1 8*1) 1*1 opmimize?

        wtk_complex_zero(km->kg, nbin * km->L);
        p1 = km->r;
        p2 = km->r2;
        o = km->kg;
        for(i = 0; i < nbin; i++){
            for(j = 0; j < km->L; j++){
                A = (p1->a + p1->b) * p2->a;
                B = (p2->a + p2->b) * p1->b;
                C = (p1->b - p1->a) * p2->b;
                o->a += A - B;//p1->a * p2->a - p1->b * p2->b;
                o->b += B - C;//p1->a * p2->b + p1->b * p2->a;
                p1++;
                o++;
            }
            p2++;
        }

        p1 = km->kg;
        p2 = km->e;
        o = km->h_w;
        for(i = 0; i < nbin; i++){
            for(j = 0; j < km->L; j++){
                A = (p1->a + p1->b) * p2->a;
                B = (p2->a + p2->b) * p1->b;
                C = (p1->b - p1->a) * p2->b;
                o->a += A - B;//p1->a * p2->a - p1->b * p2->b;
                o->b += B - C;//p1->a * p2->b + p1->b * p2->a;
                p1++;
                o++;
            }
            p2++;
        }
        //complex_dump(km->h_w,km->L * nbin);
        //torch.matmul(self.h_w.permute(0, 2, 1).conj(), self.h_w)
        //129 * (8*1 1*1 8*1)
       // wtk_debug("test\n");
        wtk_complex_zero(km->tmp1,nbin);
        scale = km->a;
        scale2 = (1 - km->a) * (1 - 0.9999 * 0.9999);
        float *Pw = km->Pw;
        p1 = km->h_w;
        p2 = km->h_w;
        for(i = 0; i < nbin; i++){
            o = km->tmp1 + i;
            for(j = 0; j < km->L; j++){
                A = (p1->a + p1->b) * p2->a;
                B = (p2->a - p2->b) * p1->b;
                C = -(p1->b - p1->a) * p2->b;
                o->a += A - B;//p1->a * p2->a + p1->b * p2->b;
                o->b += B - C;//-p1->a * p2->b + p1->b * p2->a;
                p1++;
                p2++;
            }
            *Pw = scale * (*Pw) + scale2 * o->a;//TODO
            Pw++;
        }
        //print_float(km->Pw,nbin);
        //tmp2 = torch.matmul(x_inp.unsqueeze(1), self.Ph)
        //129 * (1*8 8*8 1*8)
        wtk_complex_zero(km->tmp2, nbin * km->L);
    #if 1
        p2=km->Ph;
        for(i = 0; i < nbin; i++){
            // p2 = km->Ph + i * km->L * km->L;
            o = km->tmp2 + i * km->L;
            for(z = 0; z < km->L; z+=km->dg_row){
                p1 = km->NonLinear_SPEC + i * km->L + z;
                o = km->tmp2 + i * km->L + z;
                for(k=0;k<km->dg_row;k++)
                {
                    for(j = 0; j < km->dg_col; j+=1)
                    {
                        A = (p1->a + p1->b) * p2->a;
                        B = (p2->a + p2->b) * p1->b;
                        C = (p1->b - p1->a) * p2->b;
                        o->a += A - B;//p1->a * p2->a - p1->b * p2->b;
                        o->b += B - C;//p1->a * p2->b + p1->b * p2->a;
                        p2++;
                        o++;
                        //p2+=km->L;
                    }
                    p1++;
                    o-=km->dg_col;
                    // p2+=(km->L-step);
                }
                // p2=p2+step;
            }
        }
    #else
        for(i = 0; i < nbin; i++){
            p2 = km->Ph + i * km->L * km->L;
            p1 = km->NonLinear_SPEC + i * km->L;
            for(z = 0; z < km->L; z++){
                o = km->tmp2 + i * km->L;
                for(j = 0; j < km->L; j+=1){
                    A = (p1->a + p1->b) * p2->a;
                    B = (p2->a + p2->b) * p1->b;
                    C = (p1->b - p1->a) * p2->b;
                    o->a += A - B;//p1->a * p2->a - p1->b * p2->b;
                    o->b += B - C;//p1->a * p2->b + p1->b * p2->a;
                    o++;
                    p2++;
                    //p2+=km->L;
                }
                p1++;
            //o++;
            }
        }
    #endif
        //torch.matmul(kg, tmp2)
        //self.Ph = self.Ph - torch.matmul(kg, torch.matmul(x_inp.unsqueeze(1), self.Ph)) + self.pweyetmp * self.Pw
        //129 * (8*1 1*8 8*8)
        // wtk_complex_zero(km->tmp3,km->L * km->L * nbin);
        p1 = km->kg;
        o = km->tmp3;
    #if 1
        for(i = 0; i < nbin; i++)
        {
            // o = km->tmp3 + i*km->L*km->L;
            for(j = 0; j < km->L; j+=km->dg_row)
            {
                p2 = km->tmp2 + i * km->L+j;
                for(m=0;m<km->dg_row;++m)
                {
                    for(z = 0; z < km->dg_col; z++)
                    {
                        A = (p1->a + p1->b) * p2->a;
                        B = (p2->a + p2->b) * p1->b;
                        C = (p1->b - p1->a) * p2->b;
                        o->a = A - B;//p1->a * p2->a - p1->b * p2->b;
                        o->b = B - C;//p1->a * p2->b + p1->b * p2->a;
                        p2++;
                        o++;
                    }

                    p2-=step;

                    // o+=(km->L-step);
                    p1++;
                }

                // o+=step;
            }
        }
    #else
        for(i = 0; i < nbin; i++){
            for(j = 0; j < km->L; j++){
                p2 = km->tmp2 + i * km->L;
                for(z = 0; z < km->L; z++){
                    A = (p1->a + p1->b) * p2->a;
                    B = (p2->a + p2->b) * p1->b;
                    C = (p1->b - p1->a) * p2->b;
                    o->a = A - B;//p1->a * p2->a - p1->b * p2->b;
                    o->b = B - C;//p1->a * p2->b + p1->b * p2->a;
                    p2++;
                    o++;
                }
                // exit(0);
                p1++;
            }
        }
    #endif

        //complex_dump(km->Ph, nbin * km->L * km->L);
        wtk_complex_t *tmp;
        Pw = km->Pw;
        Ph = km->Ph;
        tmp = km->tmp3;
        for(i = 0; i < nbin; i++){
            // pweyetmp = km->pweyetmp+i*km->L*km->L;
            // Ph = km->Ph+i*km->L*km->L;
            // tmp = km->tmp3+i*km->L*km->L;
            for(m = 0; m < km->L; m+=km->dg_row)
            {
                pweyetmp = km->pweyetmp;
                for(k=0;k<km->dg_row;++k)
                {
                    for(n = 0; n < km->dg_col; n++){
                        Ph->a = Ph->a - tmp->a + *pweyetmp * *Pw;
                        Ph->b = Ph->b - tmp->b;
                        Ph++;
                        tmp++;
                        pweyetmp++;
                    }
                    // Ph+=(km->L-step);
                    // tmp+=(km->L-step);
                    // pweyetmp+=(km->L-step);
                }
                // Ph+=step;
                // tmp+=step;
                pweyetmp+=step;
            }
            Pw++;
        }

        //wtk_complex_zero(km->lsty_hat,length);
        wtk_complex_zero(km->lsty_hat,nbin);
        //obser += torch.matmul(SPEC.unsqueeze(1),h_w[n]).squeeze(-1)
        p1 = km->NonLinear_SPEC;
        p2 = km->h_w ;
        o = km->lsty_hat;
        for(j = 0; j < nbin ; j++){
            for(i = 0; i < km->L; i++){
                A = (p1->a + p1->b) * p2->a;
                B = (p2->a + p2->b) * p1->b;
                C = (p1->b - p1->a) * p2->b;
                o->a += A - B;//p1->a * p2->a - p1->b * p2->b;
                o->b += B - C;//p1->a * p2->b + p1->b * p2->a;
                p1++;
                p2++;
            }
            o++;
        }

        for(i = 0; i < nbin; i++){
            km->s_hat[i].a = y[i].a - km->lsty_hat[i].a;
            km->s_hat[i].b = y[i].b - km->lsty_hat[i].b;
        }

        if(km->cfg->use_res){
            scale = km->cfg->Phi_SS_smooth_factor;
            scale2 = 1 - scale;
            for(i = 0; i < nbin; i++){
                km->Phi_SS[i] = scale * km->Phi_SS[i] + scale2 * complex_abs2(km->s_hat+i);
                km->Phi_EE[i] = km->Phi_SS[i] + km->HPH[i];
                km->Wiener[i] = km->Wiener[i] *0.9 + 0.1 *km->Phi_SS[i]/(km->Phi_EE[i] + 1e-10);
                km->s_hat[i].a *= km->Wiener[i];
                km->s_hat[i].b *= km->Wiener[i];
            }
        }
    }else{
        //wtk_complex_zero(km->lsty_hat,length);
        for(i = 0; i < nbin; i++){
            km->s_hat[i].a = km->e[i].a;
            km->s_hat[i].b = km->e[i].b;
        }
    }

    //complex_dump(km->lsty_hat, nbin * batchsize);
    //complex_dump(km->s_hat, nbin * batchsize);
}
#ifdef USE_NEON
void qtk_kalman_update2_4_neon(qtk_ahs_kalman_t *km, wtk_complex_t *y,
                        int nbin, int batchsize,
                        wtk_complex_t *x_fft)
{
    wtk_complex_t tc = {1.0, 0.0};
    wtk_complex_t *x_cache = km->x_cache;
    wtk_complex_t *Ph = km->Ph;
    float *Pv;
    float *pweyetmp;
    wtk_complex_t *p1, *p2, *o, *Y, *o2;
    float scale, scale2;
    int i,j,k,m,n,z,length = nbin * batchsize;
    int step=8;

    memmove(x_cache,x_cache + nbin * km->nonlinear_order, sizeof(wtk_complex_t) * nbin * (km->L - km->nonlinear_order));
    memcpy(x_cache + nbin * (km->L - km->nonlinear_order), x_fft, sizeof(wtk_complex_t) * nbin * km->nonlinear_order);
    //print_float(x,256);
    //print_float(km->x_inp,km->x_inp_len);
    float mean = get_mean2(km,x_fft,nbin);
    //wtk_debug("================%f %d\n",mean,nbin * (km->L - km->nonlinear_order));
    double dtmp;
    float A,B,C;
    float *tmp1,*tmp2;

    observation_update(km,nbin);
        wtk_complex_zero(km->obser,nbin);
        //p1 = km->NonLinear_SPEC;
        //p2 = km->h_w;
        o = km->obser;
        o2 = km->e;
        Y = y;
        //obser += torch.matmul(SPEC.unsqueeze(1),h_w[n]).squeeze(-1)
        //e = Y - self.observation(x_inp)
        tmp1 = (float32_t *)km->NonLinear_SPEC;
        tmp2 = (float32_t *)km->h_w;
        float32x4x2_t a_1, b_1, a_2, b_2;
        float32x4_t c_a_1, c_b_1, c_a_2, c_b_2;
        float y_t_a, y_t_b;
        for(j = 0; j < nbin ; j++){
            y_t_a = y_t_b = 0;
            for(i = 0; i < km->L; i+=4){
                // A = (p1->a + p1->b) * p2->a;
                // B = (p2->a + p2->b) * p1->b;
                // C = (p1->b - p1->a) * p2->b;
                // o->a += A - B;//p1->a * p2->a - p1->b * p2->b;
                // o->b += B - C;//p1->a * p2->b + p1->b * p2->a;
                // p1++;
                // p2++;
                a_1 = vld2q_f32(tmp1);
                tmp1 += 8;
                b_1 = vld2q_f32(tmp2);
                tmp2 += 8;
                c_a_1 = vsubq_f32(vmulq_f32(a_1.val[0], b_1.val[0]), vmulq_f32(a_1.val[1], b_1.val[1]));
                c_b_1 = vaddq_f32(vmulq_f32(a_1.val[0], b_1.val[1]), vmulq_f32(a_1.val[1], b_1.val[0]));
                //c_a_1 = vmlsq_f32(vmulq_f32(a_1.val[0], b_1.val[0]), a_1.val[1], b_1.val[1]);
                //c_b_1 = vmlaq_f32(vmulq_f32(a_1.val[0], b_1.val[1]), a_1.val[1], b_1.val[0]);
                y_t_a += vgetq_lane_f32(c_a_1, 0) + vgetq_lane_f32(c_a_1, 1) + vgetq_lane_f32(c_a_1, 2) + vgetq_lane_f32(c_a_1, 3);
                y_t_b += vgetq_lane_f32(c_b_1, 0) + vgetq_lane_f32(c_b_1, 1) + vgetq_lane_f32(c_b_1, 2) + vgetq_lane_f32(c_b_1, 3);
            }
            o2->a = Y->a - y_t_a;
            o2->b = Y->b - y_t_b;
            o2++;
            Y++;
            o++;
        }

    
    if(mean > km->kalman_thresh){
        //complex_dump(x_fft,nbin*km->nonlinear_order);
        //complex_dump(x_cache,nbin*km->L);
        

        //complex_dump(km->e, nbin);
        //Pv = self.a * Pv + (1 - self.a) * abs(e) ** 2
        Pv = km->Pv;
        scale = km->a;
        scale2 = 1 - km->a;
        for(i = 0; i < nbin; i++){
            dtmp = complex_abs2(km->e + i);
            *Pv = scale * *Pv + scale2 * dtmp;
            Pv++;
        }
        //exit(0);
        //replace_pv(km->Pv);
        //print_float(km->Pv, nbin);
        //r = torch.matmul(Ph, x_inp.conj().unsqueeze(-1))
        //129 * (8*8 8*1 8*1)
        //wtk_debug("dump SPEC\n");
        //complex_dump(km->NonLinear_SPEC, nbin * km->L);
        wtk_complex_zero(km->r, nbin * km->L);
        // qtk_kalman_array_zero(km,km->r,nbin,km->L,sl,el);
        p1 = km->Ph;
        o = km->r;
#if 1
        for(i = 0; i < nbin; i++){
            // p1 = km->Ph+i*km->dg_row*km->dg_col;
            o = km->r+i*km->L;
            for(j = 0; j < km->L; j+=km->dg_row)
            {
                p2 = km->NonLinear_SPEC + i * km->L + j;
                for(k=0;k<km->dg_row;k++)
                {
                    tmp1 = (float32_t*)p1;
                    tmp2 = (float32_t*)p2;
                    a_1 = vld2q_f32(tmp1);
                    a_2 = vld2q_f32(tmp1 + 8);
                    b_1 = vld2q_f32(tmp2);
                    b_2 = vld2q_f32(tmp2 + 8);

                    c_a_1 = vaddq_f32(vmulq_f32(a_1.val[0], b_1.val[0]), vmulq_f32(a_1.val[1], b_1.val[1]));
                    c_b_1 = vsubq_f32(vmulq_f32(a_1.val[1], b_1.val[0]),vmulq_f32(a_1.val[0], b_1.val[1]));
                    c_a_2 = vaddq_f32(vmulq_f32(a_2.val[0], b_2.val[0]), vmulq_f32(a_2.val[1], b_2.val[1]));
                    c_b_2 = vsubq_f32(vmulq_f32(a_2.val[1], b_2.val[0]),vmulq_f32(a_2.val[0], b_2.val[1]));
                    o->a += vgetq_lane_f32(c_a_1, 0) + vgetq_lane_f32(c_a_1, 1) + vgetq_lane_f32(c_a_1, 2) + vgetq_lane_f32(c_a_1, 3);
                    o->a += vgetq_lane_f32(c_a_2, 0) + vgetq_lane_f32(c_a_2, 1) + vgetq_lane_f32(c_a_2, 2) + vgetq_lane_f32(c_a_2, 3);
                    o->b += vgetq_lane_f32(c_b_1, 0) + vgetq_lane_f32(c_b_1, 1) + vgetq_lane_f32(c_b_1, 2) + vgetq_lane_f32(c_b_1, 3);
                    o->b += vgetq_lane_f32(c_b_2, 0) + vgetq_lane_f32(c_b_2, 1) + vgetq_lane_f32(c_b_2, 2) + vgetq_lane_f32(c_b_2, 3);
                    // for(z = 0; z < km->dg_col; z++)
                    // {
                    //     //p2->b = -p2->b;
                    //     A = (p1->a + p1->b) * p2->a;
                    //     B = (p2->a - p2->b) * p1->b;
                    //     C = -(p1->b - p1->a) * p2->b;
                    //     o->a += A - B;//p1->a * p2->a + p1->b * p2->b;
                    //     o->b += B - C;//-p1->a * p2->b + p1->b * p2->a;
                    //     p1++;
                    //     p2++;
                    // }
                    p1 += km->dg_col;
                    //p2-=km->dg_col;
                    // p1+=(km->L-step);
                    o++;
                }
                // p1 = p1+step;
            }
        }
    #else
        for(i = 0; i < nbin; i++){
            for(j = 0; j < km->L; j++){
                p2 = km->NonLinear_SPEC + i * km->L;
                for(z = 0; z < km->L; z++){
                    //p2->b = -p2->b;
                    A = (p1->a + p1->b) * p2->a;
                    B = (p2->a - p2->b) * p1->b;
                    C = -(p1->b - p1->a) * p2->b;
                    o->a += A - B;//p1->a * p2->a + p1->b * p2->b;
                    o->b += B - C;//-p1->a * p2->b + p1->b * p2->a;
                    p1++;
                    p2++;
                }
                o++;
            }
        }
    #endif
        //complex_dump(km->r, nbin * km->L);
        //r2 = 1.0 / (torch.matmul(x_inp.unsqueeze(1), r) + Pv.unsqueeze(-1))
        //129 * (1*8 8*1 1*1)

        Pv = km->Pv;
        wtk_complex_zero(km->r2, nbin);
        tmp1 = (float32_t*)km->NonLinear_SPEC;
        tmp2 = (float32_t*)km->r;
        o = km->r2;
        for(i = 0; i < nbin; i++){
            y_t_a = y_t_b = 0;
            for(j = 0; j < km->L; j+=4){
                a_1 = vld2q_f32(tmp1);
                tmp1 += 8;
                b_1 = vld2q_f32(tmp2);
                tmp2 += 8;
                c_a_1 = vsubq_f32(vmulq_f32(a_1.val[0], b_1.val[0]), vmulq_f32(a_1.val[1], b_1.val[1]));
                c_b_1 = vaddq_f32(vmulq_f32(a_1.val[0], b_1.val[1]), vmulq_f32(a_1.val[1], b_1.val[0]));
                //c_a_1 = vmlsq_f32(vmulq_f32(a_1.val[0], b_1.val[0]), a_1.val[1], b_1.val[1]);
                //c_b_1 = vmlaq_f32(vmulq_f32(a_1.val[0], b_1.val[1]), a_1.val[1], b_1.val[0]);
                y_t_a += vgetq_lane_f32(c_a_1, 0) + vgetq_lane_f32(c_a_1, 1) + vgetq_lane_f32(c_a_1, 2) + vgetq_lane_f32(c_a_1, 3);
                y_t_b += vgetq_lane_f32(c_b_1, 0) + vgetq_lane_f32(c_b_1, 1) + vgetq_lane_f32(c_b_1, 2) + vgetq_lane_f32(c_b_1, 3);
            }
            km->HPH[i] = y_t_a;
            o->a += y_t_a + *Pv;
            o->b += y_t_b;
            complex_backwards(o);
            Pv++;
            o++;
        }
        //complex_dump(km->r2, nbin);
        //kg = torch.matmul(r, r2)
        //129 * (8*1 1*1 8*1) 1*1 opmimize?

        wtk_complex_zero(km->kg, nbin * km->L);
        p1 = km->r;
        p2 = km->r2;
        o = km->kg;
        for(i = 0; i < nbin; i++){
            for(j = 0; j < km->L; j++){
                A = (p1->a + p1->b) * p2->a;
                B = (p2->a + p2->b) * p1->b;
                C = (p1->b - p1->a) * p2->b;
                o->a += A - B;//p1->a * p2->a - p1->b * p2->b;
                o->b += B - C;//p1->a * p2->b + p1->b * p2->a;
                p1++;
                o++;
            }
            p2++;
        }

        p1 = km->kg;
        p2 = km->e;
        o = km->h_w;
        for(i = 0; i < nbin; i++){
            for(j = 0; j < km->L; j++){
                A = (p1->a + p1->b) * p2->a;
                B = (p2->a + p2->b) * p1->b;
                C = (p1->b - p1->a) * p2->b;
                o->a += A - B;//p1->a * p2->a - p1->b * p2->b;
                o->b += B - C;//p1->a * p2->b + p1->b * p2->a;
                p1++;
                o++;
            }
            p2++;
        }
        //complex_dump(km->h_w,km->L * nbin);
        //torch.matmul(self.h_w.permute(0, 2, 1).conj(), self.h_w)
        //129 * (8*1 1*1 8*1)
       // wtk_debug("test\n");
        wtk_complex_zero(km->tmp1,nbin);
        scale = km->a;
        scale2 = (1 - km->a) * (1 - 0.9999 * 0.9999);
        float *Pw = km->Pw;
        p1 = km->h_w;
        p2 = km->h_w;
        for(i = 0; i < nbin; i++){
            o = km->tmp1 + i;
            for(j = 0; j < km->L; j++){
                A = (p1->a + p1->b) * p2->a;
                B = (p2->a - p2->b) * p1->b;
                C = -(p1->b - p1->a) * p2->b;
                o->a += A - B;//p1->a * p2->a + p1->b * p2->b;
                o->b += B - C;//-p1->a * p2->b + p1->b * p2->a;
                p1++;
                p2++;
            }
            *Pw = scale * (*Pw) + scale2 * o->a;//TODO
            Pw++;
        }
        //print_float(km->Pw,nbin);
        //tmp2 = torch.matmul(x_inp.unsqueeze(1), self.Ph)
        //129 * (1*8 8*8 1*8)
        wtk_complex_zero(km->tmp2, nbin * km->L);
    #if 1
        p2=km->Ph;
        for(i = 0; i < nbin; i++){
            // p2 = km->Ph + i * km->L * km->L;
            o = km->tmp2 + i * km->L;
            for(z = 0; z < km->L; z+=km->dg_row){
                p1 = km->NonLinear_SPEC + i * km->L + z;
                o = km->tmp2 + i * km->L + z;
                for(k=0;k<km->dg_row;k++)
                {
                    for(j = 0; j < km->dg_col; j+=1)
                    {
                        A = (p1->a + p1->b) * p2->a;
                        B = (p2->a + p2->b) * p1->b;
                        C = (p1->b - p1->a) * p2->b;
                        o->a += A - B;//p1->a * p2->a - p1->b * p2->b;
                        o->b += B - C;//p1->a * p2->b + p1->b * p2->a;
                        p2++;
                        o++;
                        //p2+=km->L;
                    }
                    p1++;
                    o-=km->dg_col;
                    // p2+=(km->L-step);
                }
                // p2=p2+step;
            }
        }
    #else
        for(i = 0; i < nbin; i++){
            p2 = km->Ph + i * km->L * km->L;
            p1 = km->NonLinear_SPEC + i * km->L;
            for(z = 0; z < km->L; z++){
                o = km->tmp2 + i * km->L;
                for(j = 0; j < km->L; j+=1){
                    A = (p1->a + p1->b) * p2->a;
                    B = (p2->a + p2->b) * p1->b;
                    C = (p1->b - p1->a) * p2->b;
                    o->a += A - B;//p1->a * p2->a - p1->b * p2->b;
                    o->b += B - C;//p1->a * p2->b + p1->b * p2->a;
                    o++;
                    p2++;
                    //p2+=km->L;
                }
                p1++;
            //o++;
            }
        }
    #endif
        //torch.matmul(kg, tmp2)
        //self.Ph = self.Ph - torch.matmul(kg, torch.matmul(x_inp.unsqueeze(1), self.Ph)) + self.pweyetmp * self.Pw
        //129 * (8*1 1*8 8*8)
        // wtk_complex_zero(km->tmp3,km->L * km->L * nbin);
        p1 = km->kg;
        o = km->tmp3;
    #if 1
        for(i = 0; i < nbin; i++)
        {
            // o = km->tmp3 + i*km->L*km->L;
            for(j = 0; j < km->L; j+=km->dg_row)
            {
                p2 = km->tmp2 + i * km->L+j;
                for(m=0;m<km->dg_row;++m)
                {
                    for(z = 0; z < km->dg_col; z++)
                    {
                        A = (p1->a + p1->b) * p2->a;
                        B = (p2->a + p2->b) * p1->b;
                        C = (p1->b - p1->a) * p2->b;
                        o->a = A - B;//p1->a * p2->a - p1->b * p2->b;
                        o->b = B - C;//p1->a * p2->b + p1->b * p2->a;
                        p2++;
                        o++;
                    }

                    p2-=step;

                    // o+=(km->L-step);
                    p1++;
                }

                // o+=step;
            }
        }
    #else
        for(i = 0; i < nbin; i++){
            for(j = 0; j < km->L; j++){
                p2 = km->tmp2 + i * km->L;
                for(z = 0; z < km->L; z++){
                    A = (p1->a + p1->b) * p2->a;
                    B = (p2->a + p2->b) * p1->b;
                    C = (p1->b - p1->a) * p2->b;
                    o->a = A - B;//p1->a * p2->a - p1->b * p2->b;
                    o->b = B - C;//p1->a * p2->b + p1->b * p2->a;
                    p2++;
                    o++;
                }
                // exit(0);
                p1++;
            }
        }
    #endif

        //complex_dump(km->Ph, nbin * km->L * km->L);
        wtk_complex_t *tmp;
        Pw = km->Pw;
        Ph = km->Ph;
        tmp = km->tmp3;
        for(i = 0; i < nbin; i++){
            // pweyetmp = km->pweyetmp+i*km->L*km->L;
            // Ph = km->Ph+i*km->L*km->L;
            // tmp = km->tmp3+i*km->L*km->L;
            for(m = 0; m < km->L; m+=km->dg_row)
            {
                pweyetmp = km->pweyetmp;
                for(k=0;k<km->dg_row;++k)
                {
                    for(n = 0; n < km->dg_col; n++){
                        Ph->a = Ph->a - tmp->a + *pweyetmp * *Pw;
                        Ph->b = Ph->b - tmp->b;
                        Ph++;
                        tmp++;
                        pweyetmp++;
                    }
                    // Ph+=(km->L-step);
                    // tmp+=(km->L-step);
                    // pweyetmp+=(km->L-step);
                }
                // Ph+=step;
                // tmp+=step;
                pweyetmp+=step;
            }
            Pw++;
        }

        //wtk_complex_zero(km->lsty_hat,length);
        wtk_complex_zero(km->lsty_hat,nbin);
        //obser += torch.matmul(SPEC.unsqueeze(1),h_w[n]).squeeze(-1)
        //p1 = km->NonLinear_SPEC;
        //p2 = km->h_w ;
        tmp1 = (float32_t*)km->NonLinear_SPEC;
        tmp2 = (float32_t*)km->h_w ;
        o = km->lsty_hat;
        for(j = 0; j < nbin ; j++){
            y_t_a = y_t_b = 0;
            for(i = 0; i < km->L; i+=4){
                a_1 = vld2q_f32(tmp1);
                tmp1 += 8;
                b_1 = vld2q_f32(tmp2);
                tmp2 += 8;
                c_a_1 = vsubq_f32(vmulq_f32(a_1.val[0], b_1.val[0]), vmulq_f32(a_1.val[1], b_1.val[1]));
                c_b_1 = vaddq_f32(vmulq_f32(a_1.val[0], b_1.val[1]), vmulq_f32(a_1.val[1], b_1.val[0]));
                //c_a_1 = vmlsq_f32(vmulq_f32(a_1.val[0], b_1.val[0]), a_1.val[1], b_1.val[1]);
                //c_b_1 = vmlaq_f32(vmulq_f32(a_1.val[0], b_1.val[1]), a_1.val[1], b_1.val[0]);
                y_t_a += vgetq_lane_f32(c_a_1, 0) + vgetq_lane_f32(c_a_1, 1) + vgetq_lane_f32(c_a_1, 2) + vgetq_lane_f32(c_a_1, 3);
                y_t_b += vgetq_lane_f32(c_b_1, 0) + vgetq_lane_f32(c_b_1, 1) + vgetq_lane_f32(c_b_1, 2) + vgetq_lane_f32(c_b_1, 3);
            }
            o->a += y_t_a;
            o->b += y_t_b;
            o++;
        }

        for(i = 0; i < nbin; i++){
            km->s_hat[i].a = y[i].a - km->lsty_hat[i].a;
            km->s_hat[i].b = y[i].b - km->lsty_hat[i].b;
        }

        if(km->cfg->use_res){
            scale = km->cfg->Phi_SS_smooth_factor;
            scale2 = 1 - scale;
            for(i = 0; i < nbin; i++){
                km->Phi_SS[i] = scale * km->Phi_SS[i] + scale2 * complex_abs2(km->s_hat+i);
                km->Phi_EE[i] = km->Phi_SS[i] + km->HPH[i];
                km->Wiener[i] = km->Wiener[i] *0.9 + 0.1 *km->Phi_SS[i]/(km->Phi_EE[i] + 1e-10);
                km->s_hat[i].a *= km->Wiener[i];
                km->s_hat[i].b *= km->Wiener[i];
            }
        }
    }else{
        //wtk_complex_zero(km->lsty_hat,length);
        for(i = 0; i < nbin; i++){
            km->s_hat[i].a = km->e[i].a;
            km->s_hat[i].b = km->e[i].b;
        }
    }
    //complex_dump(km->lsty_hat, nbin * batchsize);
    //complex_dump(km->s_hat, nbin * batchsize);
}
#endif
void qtk_kalman_update22(qtk_ahs_kalman_t *km, wtk_complex_t *y, int nbin, int batchsize, wtk_complex_t *x_fft){
    wtk_complex_t tc = {1.0, 0.0};
    wtk_complex_t *x_cache = km->x_cache;
    wtk_complex_t *Ph = km->Ph;
    float *Pv;
    float *pweyetmp;
    wtk_complex_t *p1, *p2, *o, *Y, *o2;
    float scale, scale2;
    int i,j,z,length = nbin * batchsize;
    memmove(x_cache,x_cache + nbin * km->nonlinear_order, sizeof(wtk_complex_t) * nbin * (km->L - km->nonlinear_order));
    memcpy(x_cache + nbin * (km->L - km->nonlinear_order), x_fft, sizeof(wtk_complex_t) * nbin * km->nonlinear_order);
    //print_float(x,256);
    //print_float(km->x_inp,km->x_inp_len);
    float mean = get_mean2(km,x_fft,nbin);
    //wtk_debug("================%f %d\n",mean,nbin * (km->L - km->nonlinear_order));
    double dtmp;
    float A,B,C;
    if(mean > km->kalman_thresh){
        //complex_dump(x_fft,nbin*km->nonlinear_order);
        //complex_dump(x_cache,nbin*km->L);
        //observation_update(km,nbin);
        wtk_complex_zero(km->obser,nbin);
        p1 = km->x_cache;//km->NonLinear_SPEC;
        p2 = km->h_w;
        o = km->obser;
        o2 = km->e;
        Y = y;

        for(j = 0; j < km->L; j++){
            o = km->obser;
            for(i = 0; i < nbin; i++){
                A = (p1->a + p1->b) * p2->a;
                B = (p2->a + p2->b) * p1->b;
                C = (p1->b - p1->a) * p2->b;
                o->a += A - B;//p1->a * p2->a - p1->b * p2->b;
                o->b += B - C;//p1->a * p2->b + p1->b * p2->a;
                o++;
                p1++;
                p2++;
            }
        }
        o = km->obser;
        for(i = 0; i < nbin; i++){
            o2->a = Y->a - o->a;
            o2->b = Y->b - o->b;
            Y++;
            o++;
            o2++;
        }
        //printf("dump e\n");
        //complex_dump(km->e, nbin);
        //exit(0);
        //Pv = self.a * Pv + (1 - self.a) * abs(e) ** 2
        Pv = km->Pv;
        scale = km->a;
        scale2 = 1 - km->a;
        for(i = 0; i < nbin; i++){
            dtmp = complex_abs2(km->e + i);
            *Pv = scale * *Pv + scale2 * dtmp;
            Pv++;
        }
        //exit(0);
        //replace_pv(km->Pv);
        //print_float(km->Pv, nbin);
        //r = torch.matmul(Ph, x_inp.conj().unsqueeze(-1))
        //129 * (8*8 8*1 8*1)
        //wtk_debug("dump SPEC\n");
        wtk_complex_zero(km->r, nbin * km->L);
        p1 = km->Ph;
        //cnt = km->L;
        //sum = 0.0;
        //printf("dump Ph\n");
        //complex_dump3(km->Ph,km->L * km->L * nbin,km->L);
        for(j = 0; j < km->L; j++){
            for(z = 0; z < km->L; z++){
                p2 = km->x_cache + z * nbin;
                o = km->r + j * nbin;
                if(z >= j){
                    //wtk_debug("%d %d %d\n",j,z,(km->forPh[j] + z - j));
                    p1 = km->Ph + (km->forPh[j] + z - j) * nbin;
                    for(i = 0; i < nbin; i++){
                    //p2->b = -p2->b;
                        A = (p1->a + p1->b) * p2->a;
                        B = (p2->a - p2->b) * p1->b;
                        C = -(p1->b - p1->a) * p2->b;
                        o->a += A - B;//p1->a * p2->a + p1->b * p2->b;
                        o->b += B - C;//-p1->a * p2->b + p1->b * p2->a;
                        p1++;
                        p2++;
                        o++;
                    }
                }else{
                    //wtk_debug("%d %d %d\n",j,z,(km->forPh[z] + j - z));
                    p1 = km->Ph + (km->forPh[z] + j - z) * nbin;
                    for(i = 0; i < nbin; i++){
                    //p2->b = -p2->b;
                        A = (p1->a - p1->b) * p2->a;
                        B = -(p2->a - p2->b) * p1->b;
                        C = -(-p1->b - p1->a) * p2->b;
                        o->a += A - B;//p1->a * p2->a + p1->b * p2->b;
                        o->b += B - C;//-p1->a * p2->b + p1->b * p2->a;
                        p1++;
                        p2++;
                        o++;
                    }
                }
                // if(p1->a > 0.0){
                //     wtk_debug("%d %d %d %f\n",j,z,km->forPh[z],p1->a);
                // }
            }
            //sum += cnt;
            //cnt--;
        }

        //printf("dump r\n");
        //complex_dump(km->r, nbin * km->L);
        //complex_dump2(km->r, nbin * km->L,km->L);
        //exit(0);
        //r2 = 1.0 / (torch.matmul(x_inp.unsqueeze(1), r) + Pv.unsqueeze(-1))
        //129 * (1*8 8*1 1*1)

        Pv = km->Pv;
        wtk_complex_zero(km->r2, nbin);
        p1 = km->x_cache;//km->NonLinear_SPEC;
        p2 = km->r;

        for(j = 0; j < km->L; j++){
            o = km->r2;
            for(i = 0; i < nbin; i++){
                A = (p1->a + p1->b) * p2->a;
                B = (p2->a + p2->b) * p1->b;
                C = (p1->b - p1->a) * p2->b;
                o->a += A - B;//p1->a * p2->a - p1->b * p2->b;
                o->b += B - C;//p1->a * p2->b + p1->b * p2->a;
                p1++;
                p2++;
                o++;
            }
        }

        o = km->r2;
        for(i = 0; i < nbin; i++){
            o->a += *Pv;
            complex_div(&tc,o);
            Pv++;
            o++;
        }
        //printf("dump r2\n");
        //complex_dump(km->r2, nbin);
        //exit(0);
        //kg = torch.matmul(r, r2)
        //129 * (8*1 1*1 8*1) 1*1 opmimize?

        wtk_complex_zero(km->kg, nbin * km->L);
        p1 = km->r;
        p2 = km->r2;
        o = km->kg;

        for(j = 0; j < km->L; j++){
            p2 = km->r2;
            for(i = 0; i < nbin; i++){
                A = (p1->a + p1->b) * p2->a;
                B = (p2->a + p2->b) * p1->b;
                C = (p1->b - p1->a) * p2->b;
                o->a += A - B;//p1->a * p2->a - p1->b * p2->b;
                o->b += B - C;//p1->a * p2->b + p1->b * p2->a;
                p1++;
                o++;
                p2++;
            }
        }
        //printf("dump kg\n");
        //complex_dump2(km->kg, nbin * km->L,km->L);
        //exit(0);

        p1 = km->kg;
        o = km->h_w;

        for(j = 0; j < km->L; j++){
            p2 = km->e;
            for(i = 0; i < nbin; i++){
                A = (p1->a + p1->b) * p2->a;
                B = (p2->a + p2->b) * p1->b;
                C = (p1->b - p1->a) * p2->b;
                o->a += A - B;//p1->a * p2->a - p1->b * p2->b;
                o->b += B - C;//p1->a * p2->b + p1->b * p2->a;
                p1++;
                o++;
                p2++;
            }
        }
        //printf("dump h_w\n");
        //complex_dump2(km->h_w,km->L * nbin,km->L);
        //exit(0);
        //torch.matmul(self.h_w.permute(0, 2, 1).conj(), self.h_w)
        //129 * (8*1 1*1 8*1)
       // wtk_debug("test\n");
        wtk_complex_zero(km->tmp1,nbin);
        scale = km->a;
        scale2 = (1 - km->a) * (1 - 0.9999 * 0.9999);
        float *Pw = km->Pw;
        p1 = km->h_w;
        p2 = km->h_w;

        for(j = 0; j < km->L; j++){
            o = km->tmp1;
            for(i = 0; i < nbin; i++){
                A = (p1->a + p1->b) * p2->a;
                B = (p2->a - p2->b) * p1->b;
                C = -(p1->b - p1->a) * p2->b;
                o->a += A - B;//p1->a * p2->a + p1->b * p2->b;
                o->b += B - C;//-p1->a * p2->b + p1->b * p2->a;
                p1++;
                p2++;
                o++;
            }
        }

        o = km->tmp1;
        for(i = 0; i < nbin; i++){
            *Pw = scale * (*Pw) + scale2 * o->a;//TODO
            o++;
            Pw++;
        }
        //printf("dump Pw\n");
        //print_float(km->Pw,nbin);
        //exit(0);
        //tmp2 = torch.matmul(x_inp.unsqueeze(1), self.Ph)
        //129 * (1*8 8*8 1*8)
        wtk_complex_zero(km->tmp2, nbin * km->L);
        p2 = km->Ph;
        //cnt = km->L;
        //sum = 0.0;
        for(j = 0; j < km->L; j++){
            //p1 = km->NonLinear_SPEC + z * nbin;
            for(z = 0; z < km->L; z++){
                o = km->tmp2 + z * nbin;
                p1 = km->x_cache + j * nbin;
                if(z >= j){
                    p2 = km->Ph + (km->forPh[j] + z - j) * nbin;
                    for(i = 0; i < nbin; i++){
                        A = (p1->a + p1->b) * p2->a;
                        B = (p2->a + p2->b) * p1->b;
                        C = (p1->b - p1->a) * p2->b;
                        o->a += A - B;//p1->a * p2->a - p1->b * p2->b;
                        o->b += B - C;//p1->a * p2->b + p1->b * p2->a;
                        p1++;
                        p2++;
                        o++;
                    }
                }else{
                    p2 = km->Ph + (km->forPh[z] + j - z) * nbin;
                    for(i = 0; i < nbin; i++){
                        A = (p1->a + p1->b) * p2->a;
                        B = (p2->a - p2->b) * p1->b;
                        C = -(p1->b - p1->a) * p2->b;
                        o->a += A - B;//p1->a * p2->a - p1->b * p2->b;
                        o->b += B - C;//p1->a * p2->b + p1->b * p2->a;
                        p1++;
                        p2++;
                        o++;
                    }
                }
            }
            //sum += cnt;
            //cnt--;
        }
        //printf("dump tmp2\n");
        //complex_dump2(km->tmp2,km->L * nbin,km->L);
        //exit(0);
        //torch.matmul(kg, tmp2)
        //self.Ph = self.Ph - torch.matmul(kg, torch.matmul(x_inp.unsqueeze(1), self.Ph)) + self.pweyetmp * self.Pw
        //129 * (8*1 1*8 8*8)
        wtk_complex_zero(km->tmp3,km->L * km->L * nbin);
        p1 = km->kg;
        o = km->tmp3;
        //cnt = km->L;
        for(i = 0; i < km->L; i++){
            //p1 = km->NonLinear_SPEC + i * nbin;
            for(j = 0; j < km->L; j++){
                if(i > j){
                    continue;
                }
                p1 = km->kg + i * nbin;
                p2 = km->tmp2 + j * nbin;
                for(z = 0; z < nbin; z++){
                    A = (p1->a + p1->b) * p2->a;
                    B = (p2->a + p2->b) * p1->b;
                    C = (p1->b - p1->a) * p2->b;
                    o->a = A - B;//p1->a * p2->a - p1->b * p2->b;
                    o->b = B - C;//p1->a * p2->b + p1->b * p2->a;
                    p2++;
                    o++;
                    p1++;
                }
            }
        }
        //printf("dump tmp3\n");
        //complex_dump3(km->tmp3,km->L * km->L * nbin,km->L);
        //complex_dump2(km->tmp3,km->L * km->L * nbin,km->L * km->L);
        //exit(0);
        wtk_complex_t *tmp = km->tmp3;
        pweyetmp = km->pweyetmp;

        Ph = km->Ph;
        Pw = km->Pw;
        int m,n;

        for(i = 0; i < km->L; i++){
            for(m = 0; m < km->L; m++){
                if(i > m){
                    continue;
                }
                Pw = km->Pw;
                for(n = 0; n < nbin; n++){
                    Ph->a = Ph->a - tmp->a + *pweyetmp * *Pw;
                    Ph->b = Ph->b - tmp->b;
                    Ph++;
                    tmp++;
                    pweyetmp++;
                    Pw++;
                }
            }
        }
        //printf("dump Ph\n");
        //complex_dump3(km->Ph,km->L * km->L * nbin,km->L);
        //complex_dump2(km->Ph, nbin * km->L * km->L,km->L * km->L);
        //exit(0);
        //wtk_complex_zero(km->lsty_hat,length);
        wtk_complex_zero(km->lsty_hat,nbin);
        //obser += torch.matmul(SPEC.unsqueeze(1),h_w[n]).squeeze(-1)
        p1 = km->x_cache;//km->NonLinear_SPEC;
        p2 = km->h_w ;

        for(j = 0; j < km->L ; j++){
            o = km->lsty_hat;
            for(i = 0; i < nbin; i++){
                A = (p1->a + p1->b) * p2->a;
                B = (p2->a + p2->b) * p1->b;
                C = (p1->b - p1->a) * p2->b;
                o->a += A - B;//p1->a * p2->a - p1->b * p2->b;
                o->b += B - C;//p1->a * p2->b + p1->b * p2->a;
                p1++;
                p2++;
                o++;
            }
        }
        //complex_dump(km->lsty_hat, nbin * batchsize);
       //if(xxx2 >= 9){
        //    exit(0);
        //}
    }else{
        wtk_complex_zero(km->lsty_hat,length);
    }

    for(i = 0; i < nbin; i++){
        km->s_hat[i].a = y[i].a - km->lsty_hat[i].a;
        km->s_hat[i].b = y[i].b - km->lsty_hat[i].b;
    }
    //complex_dump(km->lsty_hat, nbin * batchsize);
    //complex_dump(km->s_hat, nbin * batchsize);
}

void qtk_kalman_update2(qtk_ahs_kalman_t *km, wtk_complex_t *y, int nbin, int batchsize, wtk_complex_t *x_fft){
    wtk_complex_t tc = {1.0, 0.0};
    wtk_complex_t *x_cache = km->x_cache;
    wtk_complex_t *Ph = km->Ph;
    float *Pv;
    float *pweyetmp;
    wtk_complex_t *p1, *p2, *o, *Y, *o2;
    float scale, scale2;
    int i,j,z,length = nbin * batchsize;
    memmove(x_cache,x_cache + nbin * km->nonlinear_order, sizeof(wtk_complex_t) * nbin * (km->L - km->nonlinear_order));
    memcpy(x_cache + nbin * (km->L - km->nonlinear_order), x_fft, sizeof(wtk_complex_t) * nbin * km->nonlinear_order);
    //print_float(x,256);
    //print_float(km->x_inp,km->x_inp_len);
    float mean = get_mean2(km,x_fft,nbin);
    //wtk_debug("================%f %d\n",mean,nbin * (km->L - km->nonlinear_order));
    double dtmp;
    float A,B,C;

    observation_update(km,nbin);
    wtk_complex_zero(km->obser,nbin);
    p1 = km->NonLinear_SPEC;
    p2 = km->h_w;
    o = km->obser;
    o2 = km->e;
    Y = y;
    //obser += torch.matmul(SPEC.unsqueeze(1),h_w[n]).squeeze(-1)
    //e = Y - self.observation(x_inp)
    for(j = 0; j < nbin ; j++){
        for(i = 0; i < km->L; i++){
            A = (p1->a + p1->b) * p2->a;
            B = (p2->a + p2->b) * p1->b;
            C = (p1->b - p1->a) * p2->b;
            o->a += A - B;//p1->a * p2->a - p1->b * p2->b;
            o->b += B - C;//p1->a * p2->b + p1->b * p2->a;
            p1++;
            p2++;
        }
        o2->a = Y->a - o->a;
        o2->b = Y->b - o->b;
        o2++;
        Y++;
        o++;
    }

    if(mean > km->kalman_thresh){
        //complex_dump(x_fft,nbin*km->nonlinear_order);
        //complex_dump(x_cache,nbin*km->L);

        //complex_dump(km->e, nbin);
        //Pv = self.a * Pv + (1 - self.a) * abs(e) ** 2
        Pv = km->Pv;
        scale = km->a;
        scale2 = 1 - km->a;
        for(i = 0; i < nbin; i++){
            dtmp = complex_abs2(km->e + i);
            *Pv = scale * *Pv + scale2 * dtmp;
            Pv++;
        }
        //exit(0);
        //replace_pv(km->Pv);
        //print_float(km->Pv, nbin);
        //r = torch.matmul(Ph, x_inp.conj().unsqueeze(-1))
        //129 * (8*8 8*1 8*1)
        //wtk_debug("dump SPEC\n");
        //complex_dump(km->NonLinear_SPEC, nbin * km->L);
        wtk_complex_zero(km->r, nbin * km->L);
        p1 = km->Ph;
        o = km->r;
        for(i = 0; i < nbin; i++){
            for(j = 0; j < km->L; j++){
                p2 = km->NonLinear_SPEC + i * km->L;
                for(z = 0; z < km->L; z++){
                    //p2->b = -p2->b;
                    A = (p1->a + p1->b) * p2->a;
                    B = (p2->a - p2->b) * p1->b;
                    C = -(p1->b - p1->a) * p2->b;
                    o->a += A - B;//p1->a * p2->a + p1->b * p2->b;
                    o->b += B - C;//-p1->a * p2->b + p1->b * p2->a;
                    p1++;
                    p2++;
                }
                o++;
            }
        }
        //complex_dump(km->r, nbin * km->L);
        //r2 = 1.0 / (torch.matmul(x_inp.unsqueeze(1), r) + Pv.unsqueeze(-1))
        //129 * (1*8 8*1 1*1)

        Pv = km->Pv;
        wtk_complex_zero(km->r2, nbin);
        p1 = km->NonLinear_SPEC;
        p2 = km->r;
        o = km->r2;
        for(i = 0; i < nbin; i++){
            float sum_a = 0;
            float sum_b = 0;
            for(j = 0; j < km->L; j++){
                A = (p1->a + p1->b) * p2->a;
                B = (p2->a + p2->b) * p1->b;
                C = (p1->b - p1->a) * p2->b;
                sum_a += A - B;//p1->a * p2->a - p1->b * p2->b;
                sum_b += B - C;//p1->a * p2->b + p1->b * p2->a;
                p1++;
                p2++;
            }
            km->HPH[i] = sum_a;
            o->a = o->a + sum_a + *Pv;
            o->b += sum_b;
            complex_div(&tc,o);
            Pv++;
            o++;
        }
        //complex_dump(km->r2, nbin);
        //kg = torch.matmul(r, r2)
        //129 * (8*1 1*1 8*1) 1*1 opmimize?

        wtk_complex_zero(km->kg, nbin * km->L);
        p1 = km->r;
        p2 = km->r2;
        o = km->kg;
        for(i = 0; i < nbin; i++){
            for(j = 0; j < km->L; j++){
                A = (p1->a + p1->b) * p2->a;
                B = (p2->a + p2->b) * p1->b;
                C = (p1->b - p1->a) * p2->b;
                o->a += A - B;//p1->a * p2->a - p1->b * p2->b;
                o->b += B - C;//p1->a * p2->b + p1->b * p2->a;
                p1++;
                o++;
            }
            p2++;
        }

        p1 = km->kg;
        p2 = km->e;
        o = km->h_w;
        for(i = 0; i < nbin; i++){
            for(j = 0; j < km->L; j++){
                A = (p1->a + p1->b) * p2->a;
                B = (p2->a + p2->b) * p1->b;
                C = (p1->b - p1->a) * p2->b;
                o->a += A - B;//p1->a * p2->a - p1->b * p2->b;
                o->b += B - C;//p1->a * p2->b + p1->b * p2->a;
                p1++;
                o++;
            }
            p2++;
        }
        //complex_dump(km->h_w,km->L * nbin);
        //torch.matmul(self.h_w.permute(0, 2, 1).conj(), self.h_w)
        //129 * (8*1 1*1 8*1)
       // wtk_debug("test\n");
        wtk_complex_zero(km->tmp1,nbin);
        scale = km->a;
        scale2 = (1 - km->a) * (1 - 0.9999 * 0.9999);
        float *Pw = km->Pw;
        p1 = km->h_w;
        p2 = km->h_w;
        for(i = 0; i < nbin; i++){
            o = km->tmp1 + i;
            for(j = 0; j < km->L; j++){
                A = (p1->a + p1->b) * p2->a;
                B = (p2->a - p2->b) * p1->b;
                C = -(p1->b - p1->a) * p2->b;
                o->a += A - B;//p1->a * p2->a + p1->b * p2->b;
                o->b += B - C;//-p1->a * p2->b + p1->b * p2->a;
                p1++;
                p2++;
            }
            *Pw = scale * (*Pw) + scale2 * o->a;//TODO
            Pw++;
        }
        //print_float(km->Pw,nbin);
        //tmp2 = torch.matmul(x_inp.unsqueeze(1), self.Ph)
        //129 * (1*8 8*8 1*8)
        wtk_complex_zero(km->tmp2, nbin * km->L);
        for(i = 0; i < nbin; i++){
            p2 = km->Ph + i * km->L * km->L;
            p1 = km->NonLinear_SPEC + i * km->L;
            for(z = 0; z < km->L; z++){
                o = km->tmp2 + i * km->L;
                for(j = 0; j < km->L; j+=4){
                    A = (p1->a + p1->b) * p2->a;
                    B = (p2->a + p2->b) * p1->b;
                    C = (p1->b - p1->a) * p2->b;
                    o->a += A - B;//p1->a * p2->a - p1->b * p2->b;
                    o->b += B - C;//p1->a * p2->b + p1->b * p2->a;
                    o++;
                    p2++;

                    A = (p1->a + p1->b) * p2->a;
                    B = (p2->a + p2->b) * p1->b;
                    C = (p1->b - p1->a) * p2->b;
                    o->a += A - B;//p1->a * p2->a - p1->b * p2->b;
                    o->b += B - C;//p1->a * p2->b + p1->b * p2->a;
                    o++;
                    p2++;

                    A = (p1->a + p1->b) * p2->a;
                    B = (p2->a + p2->b) * p1->b;
                    C = (p1->b - p1->a) * p2->b;
                    o->a += A - B;//p1->a * p2->a - p1->b * p2->b;
                    o->b += B - C;//p1->a * p2->b + p1->b * p2->a;
                    o++;
                    p2++;

                    A = (p1->a + p1->b) * p2->a;
                    B = (p2->a + p2->b) * p1->b;
                    C = (p1->b - p1->a) * p2->b;
                    o->a += A - B;//p1->a * p2->a - p1->b * p2->b;
                    o->b += B - C;//p1->a * p2->b + p1->b * p2->a;
                    p2++;
                    o++;
                    //p2+=km->L;
                }
                p1++;
            //o++;
            }
        }
        //torch.matmul(kg, tmp2)
        //self.Ph = self.Ph - torch.matmul(kg, torch.matmul(x_inp.unsqueeze(1), self.Ph)) + self.pweyetmp * self.Pw
        //129 * (8*1 1*8 8*8)
        //wtk_complex_zero(km->tmp3,km->L * km->L * nbin);
        p1 = km->kg;
        o = km->tmp3;
        for(i = 0; i < nbin; i++){
            for(j = 0; j < km->L; j++){
                p2 = km->tmp2 + i * km->L;
                for(z = 0; z < km->L; z++){
                    A = (p1->a + p1->b) * p2->a;
                    B = (p2->a + p2->b) * p1->b;
                    C = (p1->b - p1->a) * p2->b;
                    o->a = A - B;//p1->a * p2->a - p1->b * p2->b;
                    o->b = B - C;//p1->a * p2->b + p1->b * p2->a;
                    p2++;
                    o++;
                }
                p1++;
            }
        }
        //complex_dump(km->Ph, nbin * km->L * km->L);
        wtk_complex_t *tmp = km->tmp3;
        pweyetmp = km->pweyetmp;
        Ph = km->Ph;
        Pw = km->Pw;
        int m,n;
        for(i = 0; i < nbin; i++){
            for(m = 0; m < km->L; m++){
                for(n = 0; n < km->L; n++){
                    Ph->a = Ph->a - tmp->a + *pweyetmp * *Pw;
                    Ph->b = Ph->b - tmp->b;
                    Ph++;
                    tmp++;
                    pweyetmp++;
                }
            }
            Pw++;
        }

        //wtk_complex_zero(km->lsty_hat,length);
        wtk_complex_zero(km->lsty_hat,nbin);
        //obser += torch.matmul(SPEC.unsqueeze(1),h_w[n]).squeeze(-1)
        p1 = km->NonLinear_SPEC;
        p2 = km->h_w ;
        o = km->lsty_hat;
        for(j = 0; j < nbin ; j++){
            for(i = 0; i < km->L; i++){
                A = (p1->a + p1->b) * p2->a;
                B = (p2->a + p2->b) * p1->b;
                C = (p1->b - p1->a) * p2->b;
                o->a += A - B;//p1->a * p2->a - p1->b * p2->b;
                o->b += B - C;//p1->a * p2->b + p1->b * p2->a;
                p1++;
                p2++;
            }
            o++;
        }

        for(i = 0; i < nbin; i++){
            km->s_hat[i].a = y[i].a - km->lsty_hat[i].a;
            km->s_hat[i].b = y[i].b - km->lsty_hat[i].b;
        }

        if(km->cfg->use_res){
            scale = km->cfg->Phi_SS_smooth_factor;
            scale2 = 1 - scale;
            for(i = 0; i < nbin; i++){
                km->Phi_SS[i] = scale * km->Phi_SS[i] + scale2 * complex_abs2(km->s_hat+i);
                km->Phi_EE[i] = km->Phi_SS[i] + km->HPH[i];
                km->Wiener[i] = km->Wiener[i] *0.9 + 0.1 *km->Phi_SS[i]/(km->Phi_EE[i] + 1e-10);
                km->s_hat[i].a *= km->Wiener[i];
                km->s_hat[i].b *= km->Wiener[i];
            }
        }
    }else{
        //wtk_complex_zero(km->lsty_hat,length);
        for(i = 0; i < nbin; i++){
            km->s_hat[i].a = km->e[i].a;
            km->s_hat[i].b = km->e[i].b;
        }
    }

    //complex_dump(km->lsty_hat, nbin * batchsize);
    //complex_dump(km->s_hat, nbin * batchsize);
}
#ifdef USE_NEON
void qtk_kalman_update_neon(qtk_ahs_kalman_t *km, wtk_complex_t *y, int nbin, int batchsize, wtk_complex_t *x_fft){
    wtk_complex_t *x_cache = km->x_cache;
    wtk_complex_t *Ph = km->Ph;
    float *Pv;
    float *pweyetmp;
    //float *x_inp = km->x_inp;
    wtk_complex_t *p1, *p2, *o, *Y, *o2;
    float scale, scale2;
    int i,j,z,length = nbin * batchsize;

    memmove(x_cache,x_cache + nbin * km->nonlinear_order, sizeof(wtk_complex_t) * nbin * (km->L - km->nonlinear_order));
    memcpy(x_cache + nbin * (km->L - km->nonlinear_order), x_fft, sizeof(wtk_complex_t) * nbin * km->nonlinear_order);
    //print_float(km->x_inp,km->x_inp_len);
    float mean = get_mean2(km,x_fft,nbin);
    //wtk_debug("%f\n",mean);
    double dtmp;
    float A,B,C;
    float *tmp1,*tmp2;

    observation_update(km,nbin);
    wtk_complex_zero(km->obser,nbin);
    //p1 = km->NonLinear_SPEC;
    //p2 = km->h_w;
    o = km->obser;
    o2 = km->e;
    Y = y;
    //obser += torch.matmul(SPEC.unsqueeze(1),h_w[n]).squeeze(-1)
    //e = Y - self.observation(x_inp)
    tmp1 = (float32_t *)km->NonLinear_SPEC;
    tmp2 = (float32_t *)km->h_w;
    float32x4x2_t a_1, b_1;
    float32x4_t c_a_1, c_b_1;
    float y_t_a, y_t_b;
    for(j = 0; j < nbin ; j++){
        y_t_a = y_t_b = 0;
        for(i = 0; i < km->L; i+=4){
            // A = (p1->a + p1->b) * p2->a;
            // B = (p2->a + p2->b) * p1->b;
            // C = (p1->b - p1->a) * p2->b;
            // o->a += A - B;//p1->a * p2->a - p1->b * p2->b;
            // o->b += B - C;//p1->a * p2->b + p1->b * p2->a;
            // p1++;
            // p2++;
            a_1 = vld2q_f32(tmp1);
            tmp1 += 8;
            b_1 = vld2q_f32(tmp2);
            tmp2 += 8;
            c_a_1 = vsubq_f32(vmulq_f32(a_1.val[0], b_1.val[0]), vmulq_f32(a_1.val[1], b_1.val[1]));
            c_b_1 = vaddq_f32(vmulq_f32(a_1.val[0], b_1.val[1]), vmulq_f32(a_1.val[1], b_1.val[0]));
            //c_a_1 = vmlsq_f32(vmulq_f32(a_1.val[0], b_1.val[0]), a_1.val[1], b_1.val[1]);
            //c_b_1 = vmlaq_f32(vmulq_f32(a_1.val[0], b_1.val[1]), a_1.val[1], b_1.val[0]);
            y_t_a += vgetq_lane_f32(c_a_1, 0) + vgetq_lane_f32(c_a_1, 1) + vgetq_lane_f32(c_a_1, 2) + vgetq_lane_f32(c_a_1, 3);
            y_t_b += vgetq_lane_f32(c_b_1, 0) + vgetq_lane_f32(c_b_1, 1) + vgetq_lane_f32(c_b_1, 2) + vgetq_lane_f32(c_b_1, 3);
        }
        o2->a = Y->a - y_t_a;
        o2->b = Y->b - y_t_b;
        o2++;
        Y++;
        o++;
    }

    if(mean > km->kalman_thresh){
        //complex_dump(km->e, nbin);
        //Pv = self.a * Pv + (1 - self.a) * abs(e) ** 2
        Pv = km->Pv;
        scale = km->a;
        scale2 = 1 - km->a;
        for(i = 0; i < nbin; i++){
            dtmp = complex_abs2(km->e + i);
            *Pv = scale * *Pv + scale2 * dtmp;
            Pv++;
        }
        //replace_pv(km->Pv);
        //print_float(km->Pv, nbin);
        //r = torch.matmul(Ph, x_inp.conj().unsqueeze(-1))
        //129 * (8*8 8*1 8*1)
        //wtk_debug("dump SPEC\n");
        //complex_dump(km->NonLinear_SPEC, nbin * km->L);
        wtk_complex_zero(km->r, nbin * km->L);
        //p1 = km->Ph;
        tmp1 = (float32_t*)km->Ph;
        o = km->r;
        for(i = 0; i < nbin; i++){
            for(j = 0; j < km->L; j++){
                //p2 = km->NonLinear_SPEC + i * km->L;
                tmp2 = (float32_t*)(km->NonLinear_SPEC + i * km->L);
                y_t_a = y_t_b = 0;
                for(z = 0; z < km->L; z+=4){
                    a_1 = vld2q_f32(tmp1);
                    tmp1 += 8;
                    b_1 = vld2q_f32(tmp2);
                    tmp2 += 8;
                    c_a_1 = vaddq_f32(vmulq_f32(a_1.val[0], b_1.val[0]), vmulq_f32(a_1.val[1], b_1.val[1]));
                    c_b_1 = vsubq_f32(vmulq_f32(a_1.val[1], b_1.val[0]),vmulq_f32(a_1.val[0], b_1.val[1]));
                    //c_a_1 = vmlsq_f32(vmulq_f32(a_1.val[0], b_1.val[0]), a_1.val[1], b_1.val[1]);
                    //c_b_1 = vmlaq_f32(vmulq_f32(a_1.val[0], b_1.val[1]), a_1.val[1], b_1.val[0]);
                    y_t_a += vgetq_lane_f32(c_a_1, 0) + vgetq_lane_f32(c_a_1, 1) + vgetq_lane_f32(c_a_1, 2) + vgetq_lane_f32(c_a_1, 3);
                    y_t_b += vgetq_lane_f32(c_b_1, 0) + vgetq_lane_f32(c_b_1, 1) + vgetq_lane_f32(c_b_1, 2) + vgetq_lane_f32(c_b_1, 3);
                }
                o->a += y_t_a;
                o->b += y_t_b;
                o++;
            }
        }
        //complex_dump(km->r, nbin * km->L);
        //r2 = 1.0 / (torch.matmul(x_inp.unsqueeze(1), r) + Pv.unsqueeze(-1))
        //129 * (1*8 8*1 1*1)

        Pv = km->Pv;
        wtk_complex_zero(km->r2, nbin);
        tmp1 = (float32_t*)km->NonLinear_SPEC;
        tmp2 = (float32_t*)km->r;
        o = km->r2;
        for(i = 0; i < nbin; i++){
            y_t_a = y_t_b = 0;
            for(j = 0; j < km->L; j+=4){
                a_1 = vld2q_f32(tmp1);
                tmp1 += 8;
                b_1 = vld2q_f32(tmp2);
                tmp2 += 8;
                c_a_1 = vsubq_f32(vmulq_f32(a_1.val[0], b_1.val[0]), vmulq_f32(a_1.val[1], b_1.val[1]));
                c_b_1 = vaddq_f32(vmulq_f32(a_1.val[0], b_1.val[1]), vmulq_f32(a_1.val[1], b_1.val[0]));
                //c_a_1 = vmlsq_f32(vmulq_f32(a_1.val[0], b_1.val[0]), a_1.val[1], b_1.val[1]);
                //c_b_1 = vmlaq_f32(vmulq_f32(a_1.val[0], b_1.val[1]), a_1.val[1], b_1.val[0]);
                y_t_a += vgetq_lane_f32(c_a_1, 0) + vgetq_lane_f32(c_a_1, 1) + vgetq_lane_f32(c_a_1, 2) + vgetq_lane_f32(c_a_1, 3);
                y_t_b += vgetq_lane_f32(c_b_1, 0) + vgetq_lane_f32(c_b_1, 1) + vgetq_lane_f32(c_b_1, 2) + vgetq_lane_f32(c_b_1, 3);
            }
            km->HPH[i] = y_t_a;
            o->a += y_t_a + *Pv;
            o->b += y_t_b;
            complex_backwards(o);
            Pv++;
            o++;
        }
        //complex_dump(km->r2, nbin);
        //kg = torch.matmul(r, r2)
        //129 * (8*1 1*1 8*1) 1*1 opmimize?

        wtk_complex_zero(km->kg, nbin * km->L);
        p1 = km->r;
        p2 = km->r2;
        o = km->kg;
        for(i = 0; i < nbin; i++){
            for(j = 0; j < km->L; j++){
                A = (p1->a + p1->b) * p2->a;
                B = (p2->a + p2->b) * p1->b;
                C = (p1->b - p1->a) * p2->b;
                o->a += A - B;//p1->a * p2->a - p1->b * p2->b;
                o->b += B - C;//p1->a * p2->b + p1->b * p2->a;
                p1++;
                o++;
            }
            p2++;
        }

        p1 = km->kg;
        p2 = km->e;
        o = km->h_w;
        for(i = 0; i < nbin; i++){
            for(j = 0; j < km->L; j++){
                A = (p1->a + p1->b) * p2->a;
                B = (p2->a + p2->b) * p1->b;
                C = (p1->b - p1->a) * p2->b;
                o->a += A - B;//p1->a * p2->a - p1->b * p2->b;
                o->b += B - C;//p1->a * p2->b + p1->b * p2->a;
                p1++;
                o++;
            }
            p2++;
        }
        //complex_dump(km->h_w,km->L * nbin);
        //torch.matmul(self.h_w.permute(0, 2, 1).conj(), self.h_w)
        //129 * (8*1 1*1 8*1)
       // wtk_debug("test\n");
        wtk_complex_zero(km->tmp1,nbin);
        scale = km->a;
        scale2 = (1 - km->a) * (1 - 0.9999 * 0.9999);
        float *Pw = km->Pw;
        p1 = km->h_w;
        for(i = 0; i < nbin; i++){
            o = km->tmp1 + i;
            for(j = 0; j < km->L; j++){
                //A = (p1->a + p1->b) * p2->a;
                //B = (p2->a - p2->b) * p1->b;
                //C = -(p1->b - p1->a) * p2->b;
                o->a += p1->a * p1->a + p1->b * p1->b;
                //o->b += B - C;//-p1->a * p2->b + p1->b * p2->a;
                p1++;
            }
            *Pw = scale * (*Pw) + scale2 * o->a;//TODO
            Pw++;
        }
        //print_float(km->Pw,nbin);
        //tmp2 = torch.matmul(x_inp.unsqueeze(1), self.Ph)
        //129 * (1*8 8*8 1*8)

        wtk_complex_zero(km->tmp2, nbin * km->L);
        wtk_complex_t tmp_cpx[4];
        tmp1 = (float32_t*)tmp_cpx;
        for(i = 0; i < nbin; i++){
            o = km->tmp2 + i * km->L;
            tmp2 = (float32_t*)(km->Ph + i * km->L * km->L);
            p1 = km->NonLinear_SPEC + i * km->L;
            for(z = 0; z < km->L; z++){
                tmp_cpx[0].a = p1->a;
                tmp_cpx[0].b = p1->b;
                tmp_cpx[1].a = p1->a;
                tmp_cpx[1].b = p1->b;
                tmp_cpx[2].a = p1->a;
                tmp_cpx[2].b = p1->b;
                tmp_cpx[3].a = p1->a;
                tmp_cpx[3].b = p1->b;
                o = km->tmp2 + i * km->L;
                a_1 = vld2q_f32(tmp1);
                for(j = 0; j < km->L; j+=4){
                    b_1 = vld2q_f32(tmp2);
                    tmp2 += 8;

                    c_a_1 = vsubq_f32(vmulq_f32(a_1.val[0], b_1.val[0]), vmulq_f32(a_1.val[1], b_1.val[1]));
                    c_b_1 = vaddq_f32(vmulq_f32(a_1.val[0], b_1.val[1]), vmulq_f32(a_1.val[1], b_1.val[0]));

                    o->a += vgetq_lane_f32(c_a_1, 0);
                    o->b += vgetq_lane_f32(c_b_1, 0);
                    o++;
                    o->a += vgetq_lane_f32(c_a_1, 1);
                    o->b += vgetq_lane_f32(c_b_1, 1);
                    o++;
                    o->a += vgetq_lane_f32(c_a_1, 2);
                    o->b += vgetq_lane_f32(c_b_1, 2);
                    o++;
                    o->a += vgetq_lane_f32(c_a_1, 3);
                    o->b += vgetq_lane_f32(c_b_1, 3);
                    o++;
                }
                p1++;
            }
        }

        //torch.matmul(kg, tmp2)
        //self.Ph = self.Ph - torch.matmul(kg, torch.matmul(x_inp.unsqueeze(1), self.Ph)) + self.pweyetmp * self.Pw
        //129 * (8*1 1*8 8*8)
        //wtk_complex_zero(km->tmp3,km->L * km->L * nbin);
        p1 = km->kg;
        pweyetmp = km->pweyetmp;
        Ph = km->Ph;
        Pw = km->Pw;

        for(i = 0; i < nbin; i++){
            for(j = 0; j < km->L; j++){
                p2 = km->tmp2 + i * km->L;
                for(z = 0; z < km->L; z++){
                    A = (p1->a + p1->b) * p2->a;
                    B = (p2->a + p2->b) * p1->b;
                    C = (p1->b - p1->a) * p2->b;
                    Ph->a = Ph->a - A + B + *pweyetmp * *Pw;
                    Ph->b = Ph->b - B + C;
                    Ph++;
                    pweyetmp++;
                    p2++;
                }
                p1++;
            }
            Pw++;
        }

        //wtk_complex_zero(km->lsty_hat,length);
        wtk_complex_zero(km->lsty_hat,nbin);
        //obser += torch.matmul(SPEC.unsqueeze(1),h_w[n]).squeeze(-1)
        //p1 = km->NonLinear_SPEC;
        //p2 = km->h_w ;
        tmp1 = (float32_t*)km->NonLinear_SPEC;
        tmp2 = (float32_t*)km->h_w ;
        o = km->lsty_hat;
        for(j = 0; j < nbin ; j++){
            y_t_a = y_t_b = 0;
            for(i = 0; i < km->L; i+=4){
                a_1 = vld2q_f32(tmp1);
                tmp1 += 8;
                b_1 = vld2q_f32(tmp2);
                tmp2 += 8;
                c_a_1 = vsubq_f32(vmulq_f32(a_1.val[0], b_1.val[0]), vmulq_f32(a_1.val[1], b_1.val[1]));
                c_b_1 = vaddq_f32(vmulq_f32(a_1.val[0], b_1.val[1]), vmulq_f32(a_1.val[1], b_1.val[0]));
                //c_a_1 = vmlsq_f32(vmulq_f32(a_1.val[0], b_1.val[0]), a_1.val[1], b_1.val[1]);
                //c_b_1 = vmlaq_f32(vmulq_f32(a_1.val[0], b_1.val[1]), a_1.val[1], b_1.val[0]);
                y_t_a += vgetq_lane_f32(c_a_1, 0) + vgetq_lane_f32(c_a_1, 1) + vgetq_lane_f32(c_a_1, 2) + vgetq_lane_f32(c_a_1, 3);
                y_t_b += vgetq_lane_f32(c_b_1, 0) + vgetq_lane_f32(c_b_1, 1) + vgetq_lane_f32(c_b_1, 2) + vgetq_lane_f32(c_b_1, 3);                
            }
            o->a += y_t_a;
            o->b += y_t_b;
            o++;
        }
        for(i = 0; i < nbin; i++){
            km->s_hat[i].a = y[i].a - km->lsty_hat[i].a;
            km->s_hat[i].b = y[i].b - km->lsty_hat[i].b;
        }

        if(km->cfg->use_res){
            scale = km->cfg->Phi_SS_smooth_factor;
            scale2 = 1 - scale;
            for(i = 0; i < nbin; i++){
                km->Phi_SS[i] = scale * km->Phi_SS[i] + scale2 * complex_abs2(km->s_hat+i);
                km->Phi_EE[i] = km->Phi_SS[i] + km->HPH[i];
                km->Wiener[i] = km->Wiener[i] *0.9 + 0.1 *km->Phi_SS[i]/(km->Phi_EE[i] + 1e-10);
                km->s_hat[i].a *= km->Wiener[i];
                km->s_hat[i].b *= km->Wiener[i];
            }
        }
    }else{
        // wtk_complex_zero(km->lsty_hat,length);
        for(i = 0; i < nbin; i++){
            km->s_hat[i].a = km->e[i].a;
            km->s_hat[i].b = km->e[i].b;
        }
    }
    //complex_dump(km->lsty_hat, nbin * batchsize);
    //complex_dump(km->s_hat, nbin * batchsize);
}
#endif

void qtk_kalman_update(qtk_ahs_kalman_t *km, wtk_complex_t *y, int nbin, int batchsize, wtk_complex_t *x_fft){
    if(km->type == 0){
        if(km->cfg->use_dg){
#ifdef USE_NEON
            qtk_kalman_update2_4_neon(km,y,nbin,batchsize,x_fft);
#else
            qtk_kalman_update2_4(km,y,nbin,batchsize,x_fft);
#endif
        }else if(km->cfg->use_symmetric_ph){
            qtk_kalman_update22(km,y,nbin,batchsize,x_fft);
        }else{
#ifdef USE_NEON
            qtk_kalman_update_neon(km,y,nbin,batchsize,x_fft);
#else
            qtk_kalman_update2(km,y,nbin,batchsize,x_fft);
#endif
        }
        if(km->rc){
            _res_cancel_forward(km->rc,km->s_hat,x_fft);
        }
    }else{
        qtk_kalman_update3(km,y,nbin,batchsize,x_fft);
        if(km->rc){
            _res_cancel_forward(km->rc,km->s_hat,x_fft);
        }
    }
}
