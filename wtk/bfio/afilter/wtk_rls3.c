#include "wtk_rls3.h" 

wtk_rls3_t *wtk_rls3_new(wtk_rls3_cfg_t *cfg, int nbin)
{
    wtk_rls3_t *rls;

    rls=(wtk_rls3_t *)wtk_malloc(sizeof(wtk_rls3_t));
    wtk_rls3_init(rls, cfg, nbin);

    return rls;
}

void wtk_rls3_delete(wtk_rls3_t *rls)
{
    wtk_rls3_clean(rls);
    wtk_free(rls);
}

void wtk_rls3_init(wtk_rls3_t *rls, wtk_rls3_cfg_t *cfg, int nbin)
{
    rls->cfg=cfg;

    rls->xld=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->nl*nbin);

#ifdef USE_NEON
    rls->G=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->nl*cfg->channel*nbin);
    rls->Q=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->nl*cfg->nl*nbin);

    rls->K=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->nl*nbin);
    rls->tmp=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->nl*nbin);
#else
    rls->G=(wtk_dcomplex_t *)wtk_malloc(sizeof(wtk_dcomplex_t)*cfg->nl*cfg->channel*nbin);
    rls->Q=(wtk_dcomplex_t *)wtk_malloc(sizeof(wtk_dcomplex_t)*cfg->nl*cfg->nl*nbin);

    rls->K=(wtk_dcomplex_t *)wtk_malloc(sizeof(wtk_dcomplex_t)*cfg->nl*nbin);
    rls->tmp=(wtk_dcomplex_t *)wtk_malloc(sizeof(wtk_dcomplex_t)*cfg->nl*nbin);
#endif

    rls->w=(double *)wtk_malloc(sizeof(double)*nbin);

    rls->wx=NULL;
    rls->x2=rls->x3=rls->phrr=rls->phru=rls->phrd=NULL;
    rls->phrx=NULL;
    rls->u=rls->a=rls->z=NULL;
    if(cfg->use_wx)
    {
        rls->wx=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->channel*nbin);
    }else if(cfg->use_admm)
    {
        rls->x2=(float *)wtk_malloc(sizeof(float)*cfg->channel*nbin);
        rls->x3=(float *)wtk_malloc(sizeof(float)*cfg->channel*nbin);
        rls->phrr=(float *)wtk_malloc(sizeof(float)*cfg->channel*nbin);
        rls->phru=(float *)wtk_malloc(sizeof(float)*cfg->channel*nbin);

        rls->phrx=wtk_float_new_p2(cfg->nd+1,cfg->channel*nbin);  ////
        rls->phrd=(float *)wtk_malloc(sizeof(float)*cfg->channel*nbin);

    	rls->u=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->channel*nbin);
        rls->a=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->channel*nbin);
        rls->z=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->channel*nbin);
    }
    rls->out=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->channel*nbin);
    rls->lsty=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->channel*nbin);

    rls->lambda=(float *)wtk_malloc(sizeof(float)*nbin);
    rls->fa_1=(float *)wtk_malloc(sizeof(float)*nbin);
    rls->Q_eye_n=(float *)wtk_malloc(sizeof(float)*nbin);

    wtk_rls3_reset(rls, nbin);
}

void wtk_rls3_clean(wtk_rls3_t *rls)
{
    wtk_free(rls->xld);
    wtk_free(rls->G);
    wtk_free(rls->Q);
    wtk_free(rls->w);
    if(rls->wx)
    {
        wtk_free(rls->wx);
    }else if(rls->x2)
    {
        wtk_free(rls->x2);
        wtk_free(rls->x3);
        wtk_free(rls->phrr);
        wtk_free(rls->phru);
        wtk_free(rls->phrd);
        wtk_free(rls->u);
        wtk_free(rls->a);
        wtk_free(rls->z);
        wtk_float_delete_p2(rls->phrx, rls->cfg->nd+1);  ////
    }
    wtk_free(rls->out);
    wtk_free(rls->lsty);
    wtk_free(rls->K);
    wtk_free(rls->tmp);
    wtk_free(rls->lambda);
    wtk_free(rls->fa_1);
    wtk_free(rls->Q_eye_n);
}

void wtk_rls3_reset(wtk_rls3_t *rls, int nbin)
{
    int i,j;

    memset(rls->xld, 0, sizeof(wtk_complex_t)*rls->cfg->nl*nbin);

#ifdef USE_NEON
    memset(rls->G, 0, sizeof(wtk_complex_t)*rls->cfg->channel*rls->cfg->nl*nbin);
    memset(rls->Q, 0, sizeof(wtk_complex_t)*rls->cfg->nl*rls->cfg->nl*nbin);
#else
    memset(rls->G, 0, sizeof(wtk_dcomplex_t)*rls->cfg->channel*rls->cfg->nl*nbin);
    memset(rls->Q, 0, sizeof(wtk_dcomplex_t)*rls->cfg->nl*rls->cfg->nl*nbin);
#endif
    for(i=0;i<nbin;++i){
        for(j=0; j<rls->cfg->nl; ++j)
        {
            rls->Q[j*rls->cfg->nl+j+i*rls->cfg->nl*rls->cfg->nl].a=1;
        }
    }
    memset(rls->w, 0, sizeof(double)*nbin);
    if(rls->wx)
    {
        memset(rls->wx, 0, sizeof(wtk_complex_t)*rls->cfg->channel*nbin);
    }
    memset(rls->out, 0, sizeof(wtk_complex_t)*rls->cfg->channel*nbin);
    memset(rls->lsty, 0, sizeof(wtk_complex_t)*rls->cfg->channel*nbin);
#ifdef USE_NEON
    memset(rls->K, 0, sizeof(wtk_complex_t)*rls->cfg->nl*nbin);
    memset(rls->tmp, 0, sizeof(wtk_complex_t)*rls->cfg->nl*nbin);
#else
    memset(rls->K, 0, sizeof(wtk_dcomplex_t)*rls->cfg->nl*nbin);
    memset(rls->tmp, 0, sizeof(wtk_dcomplex_t)*rls->cfg->nl*nbin);
#endif

    for(i=0;i<nbin;++i){
        rls->lambda[i]=rls->cfg->lemma;
    }
    memset(rls->fa_1, 0, sizeof(float)*nbin);
    memset(rls->Q_eye_n, 0, sizeof(float)*nbin);
    rls->nframe=0;
}

void wtk_rls3_reset2(wtk_rls3_t *rls, int nbin)
{
    memset(rls->xld, 0, sizeof(wtk_complex_t)*rls->cfg->nl*nbin);
}

#ifdef USE_NEON
void wtk_rls3_feed2_neon(wtk_rls3_t *rls, wtk_complex_t *in, wtk_complex_t *xld, int nbin)
{
    float sigma=rls->cfg->sigma;
    float p=rls->cfg->p;
    float w_alpha=rls->cfg->w_alpha;
    // float lambda=rls->cfg->lemma;
    // float lambdaf=1.0/lambda;
    float *lambda=rls->lambda;
    float lambdaf;
    float min_lemma=rls->cfg->min_lemma;
    float max_lemma=rls->cfg->max_lemma;
    float Q_eye=rls->cfg->Q_eye;
    float Q_eye_alpha=rls->cfg->Q_eye_alpha;
    float *Q_eye_n=rls->Q_eye_n;
    float delta=rls->cfg->delta;
    int nd=rls->cfg->nd;

    wtk_complex_t *Y=rls->lsty, *Ytmp;
    wtk_complex_t *E=rls->out, *Etmp;
    wtk_complex_t *G=rls->G, *Gtmp;
    wtk_complex_t *Q=rls->Q, *Qtmp;
    wtk_complex_t *wx=rls->wx, *wxtmp;
    float *x2=rls->x2, *x3=rls->x3, *phrr=rls->phrr, *phru=rls->phru;
    float **phrx=rls->phrx, *phrxtmp, *phrxtmp1;
    float *phrd=rls->phrd;
    int l,iters=rls->cfg->iters;
    wtk_complex_t *u=rls->u,*u1,*a=rls->a,*a1,*z=rls->z,*z1;

    wtk_complex_t *xldtmp, *xldtmp2;
    wtk_complex_t *intmp;

    wtk_complex_t *K=rls->K, *ktmp;
    wtk_complex_t *tmp=rls->tmp, *tmp2;
    float px=rls->cfg->px;
    int channel=rls->cfg->channel;
    int nl=rls->cfg->nl;
    int i,j,n;
    double fa,fa2,fb2;
    float *fa_1=rls->fa_1;
    double *w=rls->w;

    ++rls->nframe;

    int d;
    float ya, yb;
    float w_alpha_1=1-w_alpha;
    float32_t *n_x, *n_g, *n_q, *n_wx, *n_k;
    float32x4_t *nx4_in, *nx4_y, *nx4_e, *nx4_k;
    float32x4x2_t x4x2_1, x4x2_2;
    float32x4_t x4_1, x4_2;
    float32x2x2_t x2x2_1, x2x2_2;
    float32x2_t x2_1, x2_2;
    float32_t e_1, e_2, w_1;

    memset(Y, 0, sizeof(wtk_complex_t)*channel*nbin);
    Ytmp=Y;
    Etmp=E;
    Gtmp=G;
    intmp=in;

    n_g = (float32_t *)G;
    for(n=0;n<nbin;++n){
        for(i=0; i<channel; ++i, ++Ytmp, ++Etmp, ++intmp)
        {
            xldtmp=xld+n*nl;
            d = nl;
            n_x = (float32_t *)xldtmp;
            ya = yb = 0;
            while(d>=4){
                x4x2_1 = vld2q_f32(n_x);
                x4x2_2 = vld2q_f32(n_g);
                x4_1 = vmlsq_f32(vmulq_f32(x4x2_1.val[0], x4x2_2.val[0]), x4x2_1.val[1], x4x2_2.val[1]);
                x4_2 = vmlaq_f32(vmulq_f32(x4x2_1.val[0], x4x2_2.val[1]), x4x2_1.val[1], x4x2_2.val[0]);
                
                ya += vgetq_lane_f32(x4_1, 0) + vgetq_lane_f32(x4_1, 1) + vgetq_lane_f32(x4_1, 2) + vgetq_lane_f32(x4_1, 3);
                yb += vgetq_lane_f32(x4_2, 0) + vgetq_lane_f32(x4_2, 1) + vgetq_lane_f32(x4_2, 2) + vgetq_lane_f32(x4_2, 3);
                n_x+=8,n_g+=8,xldtmp+=4,Gtmp+=4;
                d-=4;
            }
            if(d>=2){
                x2x2_1 = vld2_f32(n_x);
                x2x2_2 = vld2_f32(n_g);
                x2_1 = vmls_f32(vmul_f32(x2x2_1.val[0], x2x2_2.val[0]), x2x2_1.val[1], x2x2_2.val[1]);
                x2_2 = vmla_f32(vmul_f32(x2x2_1.val[0], x2x2_2.val[1]), x2x2_1.val[1], x2x2_2.val[0]);
            
                ya += vget_lane_f32(x2_1, 0) + vget_lane_f32(x2_1, 1);
                yb += vget_lane_f32(x2_2, 0) + vget_lane_f32(x2_2, 1);
                n_x+=4,n_g+=4,xldtmp+=2,Gtmp+=2;
                d-=2;
            }
            Ytmp->a+=ya;
            Ytmp->b+=yb;
            for(j=d; j>0; --j, ++xldtmp, ++Gtmp)
            {
                Ytmp->a+=xldtmp->a*Gtmp->a-xldtmp->b*Gtmp->b;
                Ytmp->b+=xldtmp->a*Gtmp->b+xldtmp->b*Gtmp->a;
            }
        }
    }

    Ytmp=Y;
    Etmp=E;
    intmp=in;
    d = channel*nbin*2;  // *2 -> complex
    nx4_y = (float32x4_t *)Ytmp;
    nx4_in = (float32x4_t *)intmp;
    while(d>=4){
        x4_1 = vsubq_f32(nx4_in[0], nx4_y[0]);
        Etmp->a = vgetq_lane_f32(x4_1, 0);
        Etmp->b = vgetq_lane_f32(x4_1, 1);
        ++Etmp;
        Etmp->a = vgetq_lane_f32(x4_1, 2);
        Etmp->b = vgetq_lane_f32(x4_1, 3);
        ++Etmp;
        ++nx4_in;
        ++nx4_y;
        Ytmp+=2;
        intmp+=2;
        d-=4;
    }
    for(i=d;i>0;i-=2,++Ytmp,++Etmp,++intmp)
    {
        Etmp->a=intmp->a-Ytmp->a;
        Etmp->b=intmp->b-Ytmp->b;
    }

    if(wx)
    {
        wxtmp=wx;
        Etmp=E;
        n_wx=(float32_t *)wxtmp;
        if(rls->nframe==1)
        {
            for(n=0;n<nbin;++n){
                for(i=0; i<channel; ++i, ++Etmp, ++wxtmp)
                {
                    wxtmp->a=Etmp->a;
                    wxtmp->b=Etmp->b;
                }
            }
        }else
        {
            d = channel*nbin*2;
            nx4_e = (float32x4_t *)Etmp;
            // nx4_wx = (float32x4_t *)wxtmp;
            while(d>=4){
                x4_2 = vld1q_f32(n_wx);
                x4_1 = vmlaq_n_f32(vmulq_n_f32(x4_2, w_alpha_1), nx4_e[0], w_alpha);
                wxtmp->a = vgetq_lane_f32(x4_1, 0);
                wxtmp->b = vgetq_lane_f32(x4_1, 1);
                ++wxtmp;
                wxtmp->a = vgetq_lane_f32(x4_1, 2);
                wxtmp->b = vgetq_lane_f32(x4_1, 3);
                ++wxtmp;
                ++nx4_e;
                // ++nx4_wx;
                n_wx+=4;
                Etmp+=2;
                d-=4;
            }
            for(i=d;i>0;i-=2,++Etmp, ++wxtmp){
                wxtmp->a=w_alpha_1*wxtmp->a+w_alpha*Etmp->a;
                wxtmp->b=w_alpha_1*wxtmp->b+w_alpha*Etmp->b;
            }
            // for(n=0;n<nbin;++n){
            //     for(i=0; i<channel; ++i, ++Etmp, ++wxtmp)
            //     {
            //         wxtmp->a=(1-w_alpha)*wxtmp->a+w_alpha*Etmp->a;
            //         wxtmp->b=(1-w_alpha)*wxtmp->b+w_alpha*Etmp->b;
            //     }
            // }
        }

        wxtmp=wx;
        for(n=0;n<nbin;++n){
            w[n]=0;
            for(i=0; i<channel; ++i, ++wxtmp)
            {
                w[n]+=wxtmp->a*wxtmp->a+wxtmp->b*wxtmp->b;
            }
            w[n]=pow(w[n]/channel+sigma, p/2-1);
        }
    }else if(x2)
    {
        intmp=in;
        for(i=0;i<channel*nbin;++i,++intmp)
        {
            x2[i]=intmp->a*intmp->a+intmp->b*intmp->b;
        }
        for(i=0;i<nd;++i)
        {
            memcpy(phrx[i],phrx[i+1],sizeof(float)*channel*nbin);
        }

        phrxtmp=phrx[nd];
        phrxtmp1=phrx[nd-1];
        if(rls->nframe>1)
        {
            for(i=0;i<channel*nbin;++i)
            {
                phrxtmp[i]=(1-w_alpha)*phrxtmp1[i]+w_alpha*x2[i];
            }
        }else
        {
            for(i=0;i<channel*nbin;++i)
            {
                phrxtmp[i]=x2[i];
            }
        }

        if(rls->nframe>nd)
        {
            phrxtmp=phrx[0];
            for(i=0;i<channel*nbin;++i)
            {
                phrr[i]=exp(-2*delta)*phrxtmp[i];
            }
        }else
        {
            for(i=0;i<channel*nbin;++i)
            {
                phrr[i]=0;
            }
        }

        for(i=0;i<channel*nbin;++i)
        {
            phru[i]=min(sqrt(phrr[i]),sqrt(x2[i]));
            x3[i]=max(x2[i]-phrr[i],0);
        }

        if(rls->nframe>1)
        {
            for(i=0;i<channel*nbin;++i)
            {
                phrd[i]=(1-w_alpha)*phrd[i]+w_alpha*x3[i];
            }
        }else
        {
            for(i=0;i<channel*nbin;++i)
            {
                phrd[i]=x3[i];
            }
        }
        for(n=0;n<nbin;++n){
            w[n]=0;
            for(i=0;i<channel;++i)
            {
                w[n]+=phrd[i+n*channel];
            }
            w[n]=pow((w[n]/channel+sigma), p/2-1);
            // wtk_debug("%f\n", w[n]);
        }
    }else
    {
        for(n=0;n<nbin;++n){
            w[n]=1;
        }
    }

    Qtmp=Q;
    ktmp=K;
    xldtmp=xld;
    tmp2=tmp;
    n_q = (float32_t *)Qtmp;
    for(n=0;n<nbin;++n){
        fa=0;
        for(i=0; i<nl; ++i, ++ktmp, ++xldtmp, ++tmp2)
        {
            xldtmp2=xld+n*nl;

            fa2=fb2=0;

            d = nl;
            n_x = (float32_t *)xldtmp2;
            while(d>=4){
                x4x2_1 = vld2q_f32(n_q);
                x4x2_2 = vld2q_f32(n_x);
                x4_1 = vmlsq_f32(vmulq_f32(x4x2_1.val[0], x4x2_2.val[0]), x4x2_1.val[1], x4x2_2.val[1]);
                x4_2 = vmlaq_f32(vmulq_f32(x4x2_1.val[0], x4x2_2.val[1]), x4x2_1.val[1], x4x2_2.val[0]);
                fa2 += vgetq_lane_f32(x4_1, 0) + vgetq_lane_f32(x4_1, 1) + vgetq_lane_f32(x4_1, 2) + vgetq_lane_f32(x4_1, 3);
                fb2 += vgetq_lane_f32(x4_2, 0) + vgetq_lane_f32(x4_2, 1) + vgetq_lane_f32(x4_2, 2) + vgetq_lane_f32(x4_2, 3);
                n_q+=8,n_x+=8,xldtmp2+=4,Qtmp+=4;
                d-=4;
            }
            if(d>=2){
                x2x2_1 = vld2_f32(n_q);
                x2x2_2 = vld2_f32(n_x);
                x2_1 = vmls_f32(vmul_f32(x2x2_1.val[0], x2x2_2.val[0]), x2x2_1.val[1], x2x2_2.val[1]);
                x2_2 = vmla_f32(vmul_f32(x2x2_1.val[0], x2x2_2.val[1]), x2x2_1.val[1], x2x2_2.val[0]);
            
                fa2 += vget_lane_f32(x2_1, 0) + vget_lane_f32(x2_1, 1);
                fb2 += vget_lane_f32(x2_2, 0) + vget_lane_f32(x2_2, 1);
                n_q+=4,n_x+=4,xldtmp2+=2,Qtmp+=2;
                d-=2;
            }
            for(j=d; j>0; --j, ++Qtmp, ++xldtmp2)
            {
                fa2+=Qtmp->a*xldtmp2->a-Qtmp->b*xldtmp2->b;
                fb2+=-Qtmp->a*xldtmp2->b-Qtmp->b*xldtmp2->a;
                n_q+=2;
            }
            ktmp->a=fa2;
            ktmp->b=-fb2;

            tmp2->a=fa2;
            tmp2->b=fb2;
            
            fa+=fa2*xldtmp->a+fb2*xldtmp->b;
        }
        fa_1[n]=fa;
        w[n]=1.0/(fa+lambda[n]/w[n]);
        // wtk_debug("%f\n", w[n]);
    }

    ktmp=K;
    n_k = (float32_t *)ktmp;
    nx4_k = (float32x4_t *)ktmp;
    for(n=0;n<nbin;++n){
        ya=w[n];
        w_1 = (float32_t)ya;
        d=nl;
        while(d>=4){
            vst1q_f32(n_k, vmulq_n_f32(nx4_k[0], w_1));
            n_k+=4;
            ++nx4_k;
            vst1q_f32(n_k, vmulq_n_f32(nx4_k[0], w_1));
            n_k+=4;
            ++nx4_k;
            ktmp+=4;
            d-=4;
        }
        for(i=d; i>0; --i, ++ktmp)
        {
            ktmp->a*=ya;
            ktmp->b*=ya;
        }
    }

    for(n=0;n<nbin;++n){
        Q_eye_n[n] = Q_eye;
    }

    if(Q_eye_alpha!=-1){
        float input_energy;
        for(n=0;n<nbin;++n){
            xldtmp=xld+n*nl;
            input_energy = 0;
            for(j=0; j<nl; ++j, ++xldtmp)
            {
                input_energy += xldtmp->a * xldtmp->a + xldtmp->b * xldtmp->b;
            }
            Q_eye_n[n] += Q_eye_alpha * input_energy;
        }
    }

    Qtmp=Q;
    tmp2=tmp;
    for(n=0;n<nbin;++n){
        lambdaf = 1.0/lambda[n];
        for(i=0; i<nl; ++i, ++tmp2)
        {
            ktmp=K+i+n*nl;
            Qtmp+=i;
            for(j=i; j<nl; ++j, ++ktmp, ++Qtmp)
            {
                if(i!=j)
                {
                    Qtmp->a=(Qtmp->a-(ktmp->a*tmp2->a-ktmp->b*tmp2->b))*lambdaf;
                    Qtmp->b=(Qtmp->b-(ktmp->a*tmp2->b+ktmp->b*tmp2->a))*lambdaf;

                    Q[j*nl+i+n*nl*nl].a=Qtmp->a;
                    Q[j*nl+i+n*nl*nl].b=-Qtmp->b;
                }else
                {
                    Qtmp->a=(Qtmp->a-(ktmp->a*tmp2->a-ktmp->b*tmp2->b))*lambdaf+Q_eye_n[n];
                    Qtmp->b=0;
                }
            }
        }
    }

    Gtmp=G;
    Etmp=E;
    for(n=0;n<nbin;++n){
        for(j=0; j<channel; ++j, ++Etmp)
        {
            ktmp=K+n*nl;
            d=nl;
            n_k = (float32_t *)ktmp;
            e_1 = (float32_t)Etmp->a;
            e_2 = (float32_t)Etmp->b;
            while(d>=4){
                x4x2_1 = vld2q_f32(n_k);
                x4_1 = vmlsq_n_f32(vmulq_n_f32(x4x2_1.val[0], e_1), x4x2_1.val[1], e_2);
                x4_2 = vmlaq_n_f32(vmulq_n_f32(x4x2_1.val[0], e_2), x4x2_1.val[1], e_1);
                Gtmp->a += vgetq_lane_f32(x4_1, 0);
                Gtmp->b += vgetq_lane_f32(x4_2, 0);
                ++Gtmp;
                Gtmp->a += vgetq_lane_f32(x4_1, 1);
                Gtmp->b += vgetq_lane_f32(x4_2, 1);
                ++Gtmp;
                Gtmp->a += vgetq_lane_f32(x4_1, 2);
                Gtmp->b += vgetq_lane_f32(x4_2, 2);
                ++Gtmp;
                Gtmp->a += vgetq_lane_f32(x4_1, 3);
                Gtmp->b += vgetq_lane_f32(x4_2, 3);
                ++Gtmp;
                n_k+=8;
                ktmp+=4;
                d-=4;
            }
            for(i=d; i>0; --i, ++ktmp, ++Gtmp)
            {
                Gtmp->a+=ktmp->a*Etmp->a-ktmp->b*Etmp->b;
                Gtmp->b+=ktmp->a*Etmp->b+ktmp->b*Etmp->a;
            }
        }
    }

    if(min_lemma!= 1.0 || max_lemma!= 1.0){
        float grad_lambda;
        float delta_lambda;
        for(n=0;n<nbin;++n){
            grad_lambda = -2 * 2.718 * (fa_1[n] - 1.0) /(lambda[n] * lambda[n]);
            grad_lambda = max(-1e3, min(1e3, grad_lambda));
            delta_lambda = grad_lambda;  // 可加上学习率
            rls->lambda[n] = 1.0/(1.0 + expf(-delta_lambda));
            rls->lambda[n] = max(min_lemma, min(max_lemma, rls->lambda[n]));
        }
    }

    memset(Y, 0, sizeof(wtk_complex_t)*channel*nbin);
    Ytmp=Y;
    Etmp=E;
    Gtmp=G;
    intmp=in;
    n_g = (float32_t *)G;
    for(n=0;n<nbin;++n){
        for(i=0; i<channel; ++i, ++Ytmp, ++Etmp, ++intmp)
        {
            xldtmp=xld+n*nl;
            d = nl;
            n_x = (float32_t *)xldtmp;
            ya = yb = 0;
            while(d>=4){
                x4x2_1 = vld2q_f32(n_x);
                x4x2_2 = vld2q_f32(n_g);
                x4_1 = vmlsq_f32(vmulq_f32(x4x2_1.val[0], x4x2_2.val[0]), x4x2_1.val[1], x4x2_2.val[1]);
                x4_2 = vmlaq_f32(vmulq_f32(x4x2_1.val[0], x4x2_2.val[1]), x4x2_1.val[1], x4x2_2.val[0]);
                
                ya += vgetq_lane_f32(x4_1, 0) + vgetq_lane_f32(x4_1, 1) + vgetq_lane_f32(x4_1, 2) + vgetq_lane_f32(x4_1, 3);
                yb += vgetq_lane_f32(x4_2, 0) + vgetq_lane_f32(x4_2, 1) + vgetq_lane_f32(x4_2, 2) + vgetq_lane_f32(x4_2, 3);
                n_x+=8,n_g+=8,xldtmp+=4,Gtmp+=4;
                d-=4;
            }
            if(d>=2){
                x2x2_1 = vld2_f32(n_x);
                x2x2_2 = vld2_f32(n_g);
                x2_1 = vmls_f32(vmul_f32(x2x2_1.val[0], x2x2_2.val[0]), x2x2_1.val[1], x2x2_2.val[1]);
                x2_2 = vmla_f32(vmul_f32(x2x2_1.val[0], x2x2_2.val[1]), x2x2_1.val[1], x2x2_2.val[0]);
            
                ya += vget_lane_f32(x2_1, 0) + vget_lane_f32(x2_1, 1);
                yb += vget_lane_f32(x2_2, 0) + vget_lane_f32(x2_2, 1);
                n_x+=4,n_g+=4,xldtmp+=2,Gtmp+=2;
                d-=2;
            }
            Ytmp->a+=ya;
            Ytmp->b+=yb;
            for(j=d; j>0; --j, ++xldtmp, ++Gtmp)
            {
                Ytmp->a+=xldtmp->a*Gtmp->a-xldtmp->b*Gtmp->b;
                Ytmp->b+=xldtmp->a*Gtmp->b+xldtmp->b*Gtmp->a;
            }
        }
    }

    Ytmp=Y;
    Etmp=E;
    intmp=in;

    d = channel*nbin*2;  // *2 -> complex
    nx4_y = (float32x4_t *)Ytmp;
    nx4_in = (float32x4_t *)intmp;
    while(d>=4){
        x4_1 = vsubq_f32(nx4_in[0], nx4_y[0]);
        Etmp->a = vgetq_lane_f32(x4_1, 0);
        Etmp->b = vgetq_lane_f32(x4_1, 1);
        ++Etmp;
        Etmp->a = vgetq_lane_f32(x4_1, 2);
        Etmp->b = vgetq_lane_f32(x4_1, 3);
        ++Etmp;
        ++nx4_in;
        ++nx4_y;
        Ytmp+=2;
        intmp+=2;
        d-=4;
    }
    for(i=d;i>0;i-=2,++Ytmp,++Etmp,++intmp)
    {
        Etmp->a=intmp->a-Ytmp->a;
        Etmp->b=intmp->b-Ytmp->b;
    }

    if(rls->cfg->use_admm)
    {
        memcpy(u,Y,sizeof(wtk_complex_t)*channel*nbin);
        memset(a,0,sizeof(wtk_complex_t)*channel*nbin);
        memset(z,0,sizeof(wtk_complex_t)*channel*nbin);

        Qtmp=Q;
        ktmp=K;
        xldtmp=xld;
        for(n=0;n<nbin;++n){
            fa=0;
            for(i=0; i<nl; ++i, ++ktmp, ++xldtmp)
            {
                xldtmp2=xld+n*nl;

                fa2=fb2=0;
                for(j=0; j<nl; ++j, ++Qtmp, ++xldtmp2)
                {
                    fa2+=Qtmp->a*xldtmp2->a-Qtmp->b*xldtmp2->b;
                    fb2+=-Qtmp->a*xldtmp2->b-Qtmp->b*xldtmp2->a;
                }
                ktmp->a=fa2;
                ktmp->b=fb2;
                
                fa+=fa2*xldtmp->a-fb2*xldtmp->b;
            }
            w[n]=1.0/(fa+2/px);
            // wtk_debug("%f\n", w[n]);
        }

        ktmp=K;
        for(n=0;n<nbin;++n){
            for(i=0; i<nl; ++i, ++ktmp)
            {
                ktmp->a*=w[n];
                ktmp->b*=w[n];
            }
        }

        for(l=0;l<iters;++l)
        {
            Gtmp=G;
            u1=u;a1=a;z1=z;
            for(n=0;n<nbin;++n){
                for(j=0; j<channel; ++j, ++u1,++a1,++z1)
                {
                    fa2=z1->a+a1->a-u1->a;
                    fb2=z1->b+a1->b-u1->b;
                    ktmp=K+n*nl;
                    for(i=0; i<nl; ++i, ++ktmp, ++Gtmp)
                    {
                        Gtmp->a+=ktmp->a*fa2-ktmp->b*fb2;
                        Gtmp->b+=ktmp->a*fb2+ktmp->b*fa2;
                    }
                }
            }

            memset(u, 0, sizeof(wtk_complex_t)*channel*nbin);
            u1=u;
            Gtmp=G;
            for(n=0;n<nbin;++n){
                for(i=0; i<channel; ++i, ++u1)
                {
                    xldtmp=xld+n*nl;
                    for(j=0; j<nl; ++j, ++xldtmp, ++Gtmp)
                    {
                        u1->a+=xldtmp->a*Gtmp->a-xldtmp->b*Gtmp->b;
                        u1->b+=xldtmp->a*Gtmp->b+xldtmp->b*Gtmp->a;
                    }
                }
            }

            if(l==iters-1)
            {
                break;
            }
            z1=z,a1=a,u1=u;
            for(n=0;n<nbin;++n){
                for(i=0;i<channel;++i,++z1,++a1,++u1)
                {
                    z1->a=u1->a-a1->a;
                    z1->b=u1->b-a1->b;

                    w[n]=sqrt(z1->a*z1->a+z1->b*z1->b);
                    w[n]=min(phru[i+n*channel]/(w[n]+sigma),1);
                    z1->a*=w[n];
                    z1->b*=w[n];

                    a1->a+=z1->a-u1->a;
                    a1->b+=z1->b-u1->b;
                    // z1->a+=a1->a-u1->a;
                    // z1->b+=a1->b-u1->b;
                }
            }
        }
        Etmp=E;
        Ytmp=Y;
        if(rls->cfg->use_zvar)
        {
            u1=z;
        }else
        {
            u1=u;
        }
        intmp=in;
        for(n=0;n<nbin;++n){
            for(i=0; i<channel; ++i, ++Ytmp, ++Etmp, ++u1,++intmp)
            {
                Ytmp->a=u1->a;
                Ytmp->b=u1->b;
                Etmp->a=intmp->a-Ytmp->a;
                Etmp->b=intmp->b-Ytmp->b;
            }
        }
    }
}
#else
void wtk_rls3_feed2(wtk_rls3_t *rls, wtk_complex_t *in, wtk_complex_t *xld, int nbin)
{
    float sigma=rls->cfg->sigma;
    float p=rls->cfg->p;
    float w_alpha=rls->cfg->w_alpha;
    // float lambda=rls->cfg->lemma;
    // float lambdaf=1.0/lambda;
    float *lambda=rls->lambda;
    float lambdaf;
    float min_lemma=rls->cfg->min_lemma;
    float max_lemma=rls->cfg->max_lemma;
    float Q_eye=rls->cfg->Q_eye;
    float Q_eye_alpha=rls->cfg->Q_eye_alpha;
    float *Q_eye_n=rls->Q_eye_n;
    float delta=rls->cfg->delta;
    int nd=rls->cfg->nd;

    wtk_complex_t *Y=rls->lsty, *Ytmp;
    wtk_complex_t *E=rls->out, *Etmp;
    wtk_dcomplex_t *G=rls->G, *Gtmp;
    wtk_dcomplex_t *Q=rls->Q, *Qtmp;
    wtk_complex_t *wx=rls->wx, *wxtmp;
    float *x2=rls->x2, *x3=rls->x3, *phrr=rls->phrr, *phru=rls->phru;
    float **phrx=rls->phrx, *phrxtmp, *phrxtmp1;
    float *phrd=rls->phrd;
    int l,iters=rls->cfg->iters;
    wtk_complex_t *u=rls->u,*u1,*a=rls->a,*a1,*z=rls->z,*z1;

    wtk_complex_t *xldtmp, *xldtmp2;
    wtk_complex_t *intmp;

    wtk_dcomplex_t *K=rls->K, *ktmp;
    wtk_dcomplex_t *tmp=rls->tmp, *tmp2;
    float px=rls->cfg->px;
    int channel=rls->cfg->channel;
    int nl=rls->cfg->nl;
    int i,j,n;
    double fa,fa2,fb2;
    float *fa_1=rls->fa_1;
    double *w=rls->w;

    ++rls->nframe;

    memset(Y, 0, sizeof(wtk_complex_t)*channel*nbin);
    Ytmp=Y;
    Etmp=E;
    Gtmp=G;
    intmp=in;
    for(n=0;n<nbin;++n){
        for(i=0; i<channel; ++i, ++Ytmp, ++Etmp, ++intmp)
        {
            xldtmp=xld+n*nl;
            for(j=0; j<nl; ++j, ++xldtmp, ++Gtmp)
            {
                Ytmp->a+=xldtmp->a*Gtmp->a-xldtmp->b*Gtmp->b;
                Ytmp->b+=xldtmp->a*Gtmp->b+xldtmp->b*Gtmp->a;
            }

            Etmp->a=intmp->a-Ytmp->a;
            Etmp->b=intmp->b-Ytmp->b;
            // wtk_debug("%f %f\n", Etmp->a, Etmp->b);
        }
    }

    if(wx)
    {
        wxtmp=wx;
        Etmp=E;
        if(rls->nframe==1)
        {
            for(n=0;n<nbin;++n){
                for(i=0; i<channel; ++i, ++Etmp, ++wxtmp)
                {
                    wxtmp->a=Etmp->a;
                    wxtmp->b=Etmp->b;
                }
            }
        }else
        {
            for(n=0;n<nbin;++n){
                for(i=0; i<channel; ++i, ++Etmp, ++wxtmp)
                {
                    wxtmp->a=(1-w_alpha)*wxtmp->a+w_alpha*Etmp->a;
                    wxtmp->b=(1-w_alpha)*wxtmp->b+w_alpha*Etmp->b;
                }
            }
        }

        wxtmp=wx;
        for(n=0;n<nbin;++n){
            w[n]=0;
            for(i=0; i<channel; ++i, ++wxtmp)
            {
                w[n]+=wxtmp->a*wxtmp->a+wxtmp->b*wxtmp->b;
            }
            w[n]=pow(w[n]/channel+sigma, p/2-1);
            // wtk_debug("%f\n", w[n]);
        }
    }else if(x2)
    {
        intmp=in;
        for(i=0;i<channel*nbin;++i,++intmp)
        {
            x2[i]=intmp->a*intmp->a+intmp->b*intmp->b;
        }
        for(i=0;i<nd;++i)
        {
            memcpy(phrx[i],phrx[i+1],sizeof(float)*channel*nbin);
        }

        phrxtmp=phrx[nd];
        phrxtmp1=phrx[nd-1];
        if(rls->nframe>1)
        {
            for(i=0;i<channel*nbin;++i)
            {
                phrxtmp[i]=(1-w_alpha)*phrxtmp1[i]+w_alpha*x2[i];
            }
        }else
        {
            for(i=0;i<channel*nbin;++i)
            {
                phrxtmp[i]=x2[i];
            }
        }

        if(rls->nframe>nd)
        {
            phrxtmp=phrx[0];
            for(i=0;i<channel*nbin;++i)
            {
                phrr[i]=exp(-2*delta)*phrxtmp[i];
            }
        }else
        {
            for(i=0;i<channel*nbin;++i)
            {
                phrr[i]=0;
            }
        }

        for(i=0;i<channel*nbin;++i)
        {
            phru[i]=min(sqrt(phrr[i]),sqrt(x2[i]));
            x3[i]=max(x2[i]-phrr[i],0);
        }

        if(rls->nframe>1)
        {
            for(i=0;i<channel*nbin;++i)
            {
                phrd[i]=(1-w_alpha)*phrd[i]+w_alpha*x3[i];
            }
        }else
        {
            for(i=0;i<channel*nbin;++i)
            {
                phrd[i]=x3[i];
            }
        }
        for(n=0;n<nbin;++n){
            w[n]=0;
            for(i=0;i<channel;++i)
            {
                w[n]+=phrd[i+n*channel];
            }
            w[n]=pow((w[n]/channel+sigma), p/2-1);
            // wtk_debug("%f\n", w[n]);
        }
    }else
    {
        for(n=0;n<nbin;++n){
            w[n]=1;
        }
    }

    Qtmp=Q;
    ktmp=K;
    xldtmp=xld;
    tmp2=tmp;
    for(n=0;n<nbin;++n){
        fa=0;
        for(i=0; i<nl; ++i, ++ktmp, ++xldtmp, ++tmp2)
        {
            xldtmp2=xld+n*nl;

            fa2=fb2=0;
            for(j=0; j<nl; ++j, ++Qtmp, ++xldtmp2)
            {
                fa2+=Qtmp->a*xldtmp2->a-Qtmp->b*xldtmp2->b;
                fb2+=-Qtmp->a*xldtmp2->b-Qtmp->b*xldtmp2->a;
            }
            ktmp->a=fa2;
            ktmp->b=fb2;

            tmp2->a=fa2;
            tmp2->b=-fb2;
            
            fa+=fa2*xldtmp->a-fb2*xldtmp->b;
        }
        fa_1[n]=fa;
        w[n]=1.0/(fa+lambda[n]/w[n]);
        // wtk_debug("%f\n", w[n]);
    }

    ktmp=K;
    for(n=0;n<nbin;++n){
        for(i=0; i<nl; ++i, ++ktmp)
        {
            ktmp->a*=w[n];
            ktmp->b*=w[n];
        }
    }

    for(n=0;n<nbin;++n){
        Q_eye_n[n] = Q_eye;
    }

    if(Q_eye_alpha!=-1){
        float input_energy;
        for(n=0;n<nbin;++n){
            xldtmp=xld+n*nl;
            input_energy = 0;
            for(j=0; j<nl; ++j, ++xldtmp)
            {
                input_energy += xldtmp->a * xldtmp->a + xldtmp->b * xldtmp->b;
            }
            Q_eye_n[n] += Q_eye_alpha * input_energy;
        }
    }

    Qtmp=Q;
    tmp2=tmp;
    for(n=0;n<nbin;++n){
        lambdaf = 1.0/lambda[n];
        for(i=0; i<nl; ++i, ++tmp2)
        {
            ktmp=K+i+n*nl;
            Qtmp+=i;
            for(j=i; j<nl; ++j, ++ktmp, ++Qtmp)
            {
                if(i!=j)
                {
                    Qtmp->a=(Qtmp->a-(ktmp->a*tmp2->a-ktmp->b*tmp2->b))*lambdaf;
                    Qtmp->b=(Qtmp->b-(ktmp->a*tmp2->b+ktmp->b*tmp2->a))*lambdaf;

                    Q[j*nl+i+n*nl*nl].a=Qtmp->a;
                    Q[j*nl+i+n*nl*nl].b=-Qtmp->b;
                }else
                {
                    Qtmp->a=(Qtmp->a-(ktmp->a*tmp2->a-ktmp->b*tmp2->b))*lambdaf+Q_eye_n[n];
                    Qtmp->b=0;
                }
            }
        }
    }

    Gtmp=G;
    Etmp=E;
    for(n=0;n<nbin;++n){
        for(j=0; j<channel; ++j, ++Etmp)
        {
            ktmp=K+n*nl;
            for(i=0; i<nl; ++i, ++ktmp, ++Gtmp)
            {
                Gtmp->a+=ktmp->a*Etmp->a-ktmp->b*Etmp->b;
                Gtmp->b+=ktmp->a*Etmp->b+ktmp->b*Etmp->a;
            }
        }
    }

    if(min_lemma!= 1.0 || max_lemma!= 1.0){
        float grad_lambda;
        float delta_lambda;
        for(n=0;n<nbin;++n){
            grad_lambda = -2 * 2.718 * (fa_1[n] - 1.0) /(lambda[n] * lambda[n]);
            grad_lambda = max(-1e3, min(1e3, grad_lambda));
            delta_lambda = grad_lambda;  // 可加上学习率
            rls->lambda[n] = 1.0/(1.0 + expf(-delta_lambda));
            rls->lambda[n] = max(min_lemma, min(max_lemma, rls->lambda[n]));
        }
    }

    memset(Y, 0, sizeof(wtk_complex_t)*channel*nbin);
    Ytmp=Y;
    Etmp=E;
    Gtmp=G;
    intmp=in;
    for(n=0;n<nbin;++n){
        for(i=0; i<channel; ++i, ++Ytmp, ++Etmp,++intmp)
        {
            xldtmp=xld+n*nl;
            for(j=0; j<nl; ++j, ++xldtmp, ++Gtmp)
            {
                Ytmp->a+=xldtmp->a*Gtmp->a-xldtmp->b*Gtmp->b;
                Ytmp->b+=xldtmp->a*Gtmp->b+xldtmp->b*Gtmp->a;
            }

            Etmp->a=intmp->a-Ytmp->a;
            Etmp->b=intmp->b-Ytmp->b;
            // printf("%f %f\n", Etmp->a, Etmp->b);
        }
    }

    if(rls->cfg->use_admm)
    {
        memcpy(u,Y,sizeof(wtk_complex_t)*channel*nbin);
        memset(a,0,sizeof(wtk_complex_t)*channel*nbin);
        memset(z,0,sizeof(wtk_complex_t)*channel*nbin);

        Qtmp=Q;
        ktmp=K;
        xldtmp=xld;
        for(n=0;n<nbin;++n){
            fa=0;
            for(i=0; i<nl; ++i, ++ktmp, ++xldtmp)
            {
                xldtmp2=xld+n*nl;

                fa2=fb2=0;
                for(j=0; j<nl; ++j, ++Qtmp, ++xldtmp2)
                {
                    fa2+=Qtmp->a*xldtmp2->a-Qtmp->b*xldtmp2->b;
                    fb2+=-Qtmp->a*xldtmp2->b-Qtmp->b*xldtmp2->a;
                }
                ktmp->a=fa2;
                ktmp->b=fb2;
                
                fa+=fa2*xldtmp->a-fb2*xldtmp->b;
            }
            w[n]=1.0/(fa+2/px);
            // wtk_debug("%f\n", w[n]);
        }

        ktmp=K;
        for(n=0;n<nbin;++n){
            for(i=0; i<nl; ++i, ++ktmp)
            {
                ktmp->a*=w[n];
                ktmp->b*=w[n];
            }
        }

        for(l=0;l<iters;++l)
        {
            Gtmp=G;
            u1=u;a1=a;z1=z;
            for(n=0;n<nbin;++n){
                for(j=0; j<channel; ++j, ++u1,++a1,++z1)
                {
                    fa2=z1->a+a1->a-u1->a;
                    fb2=z1->b+a1->b-u1->b;
                    ktmp=K+n*nl;
                    for(i=0; i<nl; ++i, ++ktmp, ++Gtmp)
                    {
                        Gtmp->a+=ktmp->a*fa2-ktmp->b*fb2;
                        Gtmp->b+=ktmp->a*fb2+ktmp->b*fa2;
                    }
                }
            }

            memset(u, 0, sizeof(wtk_complex_t)*channel*nbin);
            u1=u;
            Gtmp=G;
            for(n=0;n<nbin;++n){
                for(i=0; i<channel; ++i, ++u1)
                {
                    xldtmp=xld+n*nl;
                    for(j=0; j<nl; ++j, ++xldtmp, ++Gtmp)
                    {
                        u1->a+=xldtmp->a*Gtmp->a-xldtmp->b*Gtmp->b;
                        u1->b+=xldtmp->a*Gtmp->b+xldtmp->b*Gtmp->a;
                    }
                }
            }

            if(l==iters-1)
            {
                break;
            }
            z1=z,a1=a,u1=u;
            for(n=0;n<nbin;++n){
                for(i=0;i<channel;++i,++z1,++a1,++u1)
                {
                    z1->a=u1->a-a1->a;
                    z1->b=u1->b-a1->b;

                    w[n]=sqrt(z1->a*z1->a+z1->b*z1->b);
                    w[n]=min(phru[i+n*channel]/(w[n]+sigma),1);
                    z1->a*=w[n];
                    z1->b*=w[n];

                    a1->a+=z1->a-u1->a;
                    a1->b+=z1->b-u1->b;
                    // z1->a+=a1->a-u1->a;
                    // z1->b+=a1->b-u1->b;
                }
            }
        }
        Etmp=E;
        Ytmp=Y;
        if(rls->cfg->use_zvar)
        {
            u1=z;
        }else
        {
            u1=u;
        }
        intmp=in;
        for(n=0;n<nbin;++n){
            for(i=0; i<channel; ++i, ++Ytmp, ++Etmp, ++u1,++intmp)
            {
                Ytmp->a=u1->a;
                Ytmp->b=u1->b;
                Etmp->a=intmp->a-Ytmp->a;
                Etmp->b=intmp->b-Ytmp->b;
            }
        }
    }
}
#endif

void wtk_rls3_feed(wtk_rls3_t *rls, wtk_complex_t *in, wtk_complex_t *u, int nbin)
{
    int L=rls->cfg->L;
    int N=rls->cfg->N;
    wtk_complex_t *xld=rls->xld, *xldtmp;
    int n;

    for(n=0;n<nbin;++n){
        xldtmp = xld+n*L*N;
        memmove(xldtmp+N, xldtmp, sizeof(wtk_complex_t)*N*(L-1));
        memcpy(xldtmp, u+n*N, sizeof(wtk_complex_t)*N);
        // wtk_debug("%d %f %f\n", n, u[n].a, u[n].b);
    }
    // exit(0);
    // int c;
    // for(n=0;n<nbin;++n){
    //     printf("%d ", n);
    //     for(c=0;c<N;++c){
    //         printf("%f %f ", xld[n*L*N+c].a, xld[n*L*N+c].b);
    //     }
    //     printf("\n");
    // }

#ifdef USE_NEON
    wtk_rls3_feed2_neon(rls, in, xld, nbin);
#else
    wtk_rls3_feed2(rls, in, xld, nbin);
#endif
}

void wtk_rls3_feed3(wtk_rls3_t *rls, wtk_complex_t *in, wtk_complex_t *u, int switch_b, int nbin)
{
    int L=rls->cfg->L;
    int N=rls->cfg->N;
    wtk_complex_t *xld=rls->xld, *xldtmp;
    int n;

    for(n=0;n<nbin;++n){
        xldtmp = xld+n*L*N;
        memmove(xldtmp+N, xldtmp, sizeof(wtk_complex_t)*N*(L-1));
        memcpy(xldtmp, u+n*N, sizeof(wtk_complex_t)*N);
    }

    if(switch_b)
    {
#ifdef USE_NEON
        wtk_rls3_feed2_neon(rls, in, xld, nbin);
#else
        wtk_rls3_feed2(rls, in, xld, nbin);
#endif
    }
}
