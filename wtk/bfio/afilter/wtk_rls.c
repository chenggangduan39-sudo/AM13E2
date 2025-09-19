#include "wtk_rls.h" 

wtk_rls_t *wtk_rls_new(wtk_rls_cfg_t *cfg)
{
    wtk_rls_t *rls;

    rls=(wtk_rls_t *)wtk_malloc(sizeof(wtk_rls_t));
    rls->cfg=cfg;

    rls->xld=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->nl);

#ifdef USE_NEON
    rls->G=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->nl*cfg->channel);
    rls->Q=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->nl*cfg->nl);

    rls->K=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->nl);
    rls->tmp=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->nl);
#else
    rls->G=(wtk_dcomplex_t *)wtk_malloc(sizeof(wtk_dcomplex_t)*cfg->nl*cfg->channel);
    rls->Q=(wtk_dcomplex_t *)wtk_malloc(sizeof(wtk_dcomplex_t)*cfg->nl*cfg->nl);

    rls->K=(wtk_dcomplex_t *)wtk_malloc(sizeof(wtk_dcomplex_t)*cfg->nl);
    rls->tmp=(wtk_dcomplex_t *)wtk_malloc(sizeof(wtk_dcomplex_t)*cfg->nl);
#endif

    rls->wx=NULL;
    rls->x2=rls->x3=rls->phrr=rls->phru=rls->phrd=NULL;
    rls->phrx=NULL;
    rls->u=rls->a=rls->z=NULL;
    if(cfg->use_wx)
    {
        rls->wx=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->channel);
    }else if(cfg->use_admm)
    {
        rls->x2=(float *)wtk_malloc(sizeof(float)*cfg->channel);
        rls->x3=(float *)wtk_malloc(sizeof(float)*cfg->channel);
        rls->phrr=(float *)wtk_malloc(sizeof(float)*cfg->channel);
        rls->phru=(float *)wtk_malloc(sizeof(float)*cfg->channel);

        rls->phrx=wtk_float_new_p2(cfg->nd+1,cfg->channel);
        rls->phrd=(float *)wtk_malloc(sizeof(float)*cfg->channel);

    	rls->u=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->channel);
        rls->a=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->channel);
        rls->z=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->channel);
    }
    rls->out=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->channel);
    rls->lsty=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->channel);

    wtk_rls_reset(rls);

    return rls;
}

void wtk_rls_delete(wtk_rls_t *rls)
{
    wtk_free(rls->xld);
    wtk_free(rls->G);
    wtk_free(rls->Q);
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
        wtk_float_delete_p2(rls->phrx, rls->cfg->nd+1);
    }
    wtk_free(rls->out);
    wtk_free(rls->lsty);
    wtk_free(rls->K);
    wtk_free(rls->tmp);
    wtk_free(rls);
}

void wtk_rls_init(wtk_rls_t *rls, wtk_rls_cfg_t *cfg)
{
    rls->cfg=cfg;

    rls->xld=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->nl);

#ifdef USE_NEON
    rls->G=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->nl*cfg->channel);
    rls->Q=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->nl*cfg->nl);

    rls->K=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->nl);
    rls->tmp=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->nl);
#else
    rls->G=(wtk_dcomplex_t *)wtk_malloc(sizeof(wtk_dcomplex_t)*cfg->nl*cfg->channel);
    rls->Q=(wtk_dcomplex_t *)wtk_malloc(sizeof(wtk_dcomplex_t)*cfg->nl*cfg->nl);

    rls->K=(wtk_dcomplex_t *)wtk_malloc(sizeof(wtk_dcomplex_t)*cfg->nl);
    rls->tmp=(wtk_dcomplex_t *)wtk_malloc(sizeof(wtk_dcomplex_t)*cfg->nl);
#endif

    rls->wx=NULL;
    rls->x2=rls->x3=rls->phrr=rls->phru=rls->phrd=NULL;
    rls->phrx=NULL;
    rls->u=rls->a=rls->z=NULL;
    if(cfg->use_wx)
    {
        rls->wx=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->channel);
    }else if(cfg->use_admm)
    {
        rls->x2=(float *)wtk_malloc(sizeof(float)*cfg->channel);
        rls->x3=(float *)wtk_malloc(sizeof(float)*cfg->channel);
        rls->phrr=(float *)wtk_malloc(sizeof(float)*cfg->channel);
        rls->phru=(float *)wtk_malloc(sizeof(float)*cfg->channel);

        rls->phrx=wtk_float_new_p2(cfg->nd+1,cfg->channel);
        rls->phrd=(float *)wtk_malloc(sizeof(float)*cfg->channel);

    	rls->u=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->channel);
        rls->a=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->channel);
        rls->z=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->channel);
    }
    rls->out=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->channel);
    rls->lsty=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->channel);

    wtk_rls_reset(rls);
}

void wtk_rls_clean(wtk_rls_t *rls)
{
    wtk_free(rls->xld);
    wtk_free(rls->G);
    wtk_free(rls->Q);
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
        wtk_float_delete_p2(rls->phrx, rls->cfg->nd+1);
    }
    wtk_free(rls->out);
    wtk_free(rls->lsty);
    wtk_free(rls->K);
    wtk_free(rls->tmp);
}

void wtk_rls_reset(wtk_rls_t *rls)
{
    int i;

    memset(rls->xld, 0, sizeof(wtk_complex_t)*rls->cfg->nl);

#ifdef USE_NEON
    memset(rls->G, 0, sizeof(wtk_complex_t)*rls->cfg->channel*rls->cfg->nl);
    memset(rls->Q, 0, sizeof(wtk_complex_t)*rls->cfg->nl*rls->cfg->nl);
#else
    memset(rls->G, 0, sizeof(wtk_dcomplex_t)*rls->cfg->channel*rls->cfg->nl);
    memset(rls->Q, 0, sizeof(wtk_dcomplex_t)*rls->cfg->nl*rls->cfg->nl);
#endif
    for(i=0; i<rls->cfg->nl; ++i)
    {
        rls->Q[i*rls->cfg->nl+i].a=1;
    }
    if(rls->wx)
    {
        memset(rls->wx, 0, sizeof(wtk_complex_t)*rls->cfg->channel);
    }
    memset(rls->out, 0, sizeof(wtk_complex_t)*rls->cfg->channel);
    memset(rls->lsty, 0, sizeof(wtk_complex_t)*rls->cfg->channel);
#ifdef USE_NEON
    memset(rls->K, 0, sizeof(wtk_complex_t)*rls->cfg->nl);
    memset(rls->tmp, 0, sizeof(wtk_complex_t)*rls->cfg->nl);
#else
    memset(rls->K, 0, sizeof(wtk_dcomplex_t)*rls->cfg->nl);
    memset(rls->tmp, 0, sizeof(wtk_dcomplex_t)*rls->cfg->nl);
#endif

    rls->nframe=0;
    rls->lambda=rls->cfg->lemma;
}

void wtk_rls_reset2(wtk_rls_t *rls)
{
    memset(rls->xld, 0, sizeof(wtk_complex_t)*rls->cfg->nl);
}

#ifdef USE_NEON
void wtk_rls_feed2_neon(wtk_rls_t *rls, wtk_complex_t *in, wtk_complex_t *xld)
{
    float sigma=rls->cfg->sigma;
    float p=rls->cfg->p;
    float w_alpha=rls->cfg->w_alpha;
    // float lambda=rls->cfg->lemma;
    float lambda=rls->lambda;
    float lambdaf=1.0/lambda;
    float min_lemma=rls->cfg->min_lemma;
    float max_lemma=rls->cfg->max_lemma;
    float Q_eye=rls->cfg->Q_eye;
    float Q_eye_alpha=rls->cfg->Q_eye_alpha;
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
    wtk_complex_t *K=rls->K, *ktmp;
    wtk_complex_t *tmp=rls->tmp, *tmp2;

    float px=rls->cfg->px;
    int channel=rls->cfg->channel;
    int nl=rls->cfg->nl;
    int i,j;
    double w,fa,fa2,fb2;
    float fa_1;

    int nl_tmp;
    int chn_tmp;
    nl_tmp = (int)(nl >> 2) << 2;
    chn_tmp = (int)(channel >> 2) << 2;
    float y_t_a, y_t_b;

    float32_t *g, *x, *q, *k, *x_2, *f_in, *y;
    float32x4_t *k1;
    float32_t t1;
    float32_t e_a, e_b;
    float32x4_t a_a_1, a_b_1;
    float32x4_t c_a_1, c_b_1;

    float32x4x2_t a_1, b_1;

    ++rls->nframe;

    memset(Y, 0, sizeof(wtk_complex_t)*channel);
    Ytmp=Y;
    Etmp=E;
    Gtmp=G;

    g = (float32_t *)G;
    for(i=0; i<channel; ++i, ++Ytmp)
    {
        xldtmp=xld;
        x = (float32_t *)xld;

        y_t_a = y_t_b = 0;
        for(j=0; j<nl_tmp; j+=4, xldtmp+=4, Gtmp+=4){
            a_1 = vld2q_f32(x);
            x+=8;
            b_1 = vld2q_f32(g);
            g+=8;
            // c_a_1 = vsubq_f32(vmulq_f32(a_1.val[0], b_1.val[0]), vmulq_f32(a_1.val[1], b_1.val[1]));
            // c_b_1 = vaddq_f32(vmulq_f32(a_1.val[0], b_1.val[1]), vmulq_f32(a_1.val[1], b_1.val[0]));
            c_a_1 = vmlsq_f32(vmulq_f32(a_1.val[0], b_1.val[0]), a_1.val[1], b_1.val[1]);
            c_b_1 = vmlaq_f32(vmulq_f32(a_1.val[0], b_1.val[1]), a_1.val[1], b_1.val[0]);
            
            y_t_a += vgetq_lane_f32(c_a_1, 0) + vgetq_lane_f32(c_a_1, 1) + vgetq_lane_f32(c_a_1, 2) + vgetq_lane_f32(c_a_1, 3);
            y_t_b += vgetq_lane_f32(c_b_1, 0) + vgetq_lane_f32(c_b_1, 1) + vgetq_lane_f32(c_b_1, 2) + vgetq_lane_f32(c_b_1, 3);
        }
        for(j=nl_tmp; j<nl; ++j, ++xldtmp, ++Gtmp){
            y_t_a += xldtmp->a*Gtmp->a-xldtmp->b*Gtmp->b;
            y_t_b += xldtmp->a*Gtmp->b+xldtmp->b*Gtmp->a;
        }
        Ytmp->a += y_t_a;
        Ytmp->b += y_t_b;
    }


    Ytmp=Y;
    f_in=(float32_t *)in;
    y=(float32_t *)Ytmp;
    for(i=0;i<chn_tmp;i+=4,Ytmp+=4){
        a_1 = vld2q_f32(f_in);
        f_in+=8;
        b_1 = vld2q_f32(y);
        y+=8;
        a_a_1 = vsubq_f32(a_1.val[0], b_1.val[0]);
        a_b_1 = vsubq_f32(a_1.val[1], b_1.val[1]);
        Etmp->a = vgetq_lane_f32(a_a_1, 0);
        Etmp->b = vgetq_lane_f32(a_b_1, 0);
        ++Etmp;
        Etmp->a = vgetq_lane_f32(a_a_1, 1);
        Etmp->b = vgetq_lane_f32(a_b_1, 1);
        ++Etmp;
        Etmp->a = vgetq_lane_f32(a_a_1, 2);
        Etmp->b = vgetq_lane_f32(a_b_1, 2);
        ++Etmp;
        Etmp->a = vgetq_lane_f32(a_a_1, 3);
        Etmp->b = vgetq_lane_f32(a_b_1, 3);
        ++Etmp;
    }
    for(i=chn_tmp;i<channel;++i,++Ytmp,++Etmp){
        Etmp->a=in[i].a-Ytmp->a;
        Etmp->b=in[i].b-Ytmp->b;
    }


    if(wx)
    {
        wxtmp=wx;
        Etmp=E;
        if(rls->nframe==1)
        {
            for(i=0; i<channel; ++i, ++Etmp, ++wxtmp)
            {
                wxtmp->a=Etmp->a;
                wxtmp->b=Etmp->b;
            }
        }else
        {
            for(i=0; i<channel; ++i, ++Etmp, ++wxtmp)
            {
                wxtmp->a=(1-w_alpha)*wxtmp->a+w_alpha*Etmp->a;
                wxtmp->b=(1-w_alpha)*wxtmp->b+w_alpha*Etmp->b;
            }
        }

        wxtmp=wx;
        w=0;
        for(i=0; i<channel; ++i, ++wxtmp)
        {
            w+=wxtmp->a*wxtmp->a+wxtmp->b*wxtmp->b;
        }
        w=pow(w/channel+sigma, p/2-1);
    }else if(x2)
    {
        for(i=0;i<channel;++i)
        {
            x2[i]=in[i].a*in[i].a+in[i].b*in[i].b;
        }

        for(i=0;i<nd;++i)
        {
            memcpy(phrx[i],phrx[i+1],sizeof(float)*channel);
        }

        phrxtmp=phrx[nd];
        phrxtmp1=phrx[nd-1];
        if(rls->nframe>1)
        {
            for(i=0;i<channel;++i)
            {
                phrxtmp[i]=(1-w_alpha)*phrxtmp1[i]+w_alpha*x2[i];
            }
        }else
        {
            for(i=0;i<channel;++i)
            {
                phrxtmp[i]=x2[i];
            }
        }

        if(rls->nframe>nd)
        {
            phrxtmp=phrx[0];
            for(i=0;i<channel;++i)
            {
                phrr[i]=exp(-2*delta)*phrxtmp[i];
            }
        }else
        {
            for(i=0;i<channel;++i)
            {
                phrr[i]=0;
            }
        }

        for(i=0;i<channel;++i)
        {
            phru[i]=min(sqrt(phrr[i]),sqrt(x2[i]));
            x3[i]=max(x2[i]-phrr[i],0);
        }

        if(rls->nframe>1)
        {
            for(i=0;i<channel;++i)
            {
                phrd[i]=(1-w_alpha)*phrd[i]+w_alpha*x3[i];
            }
        }else
        {
            for(i=0;i<channel;++i)
            {
                phrd[i]=x3[i];
            }
        }
        w=0;
        for(i=0;i<channel;++i)
        {
            w+=phrd[i];
        }
        w=pow((w/channel+sigma), p/2-1);
    }else
    {
        w=1;
    }

    Qtmp=Q;
    ktmp=K;
    q = (float32_t *)Q;
    fa=0;
    xldtmp=xld;
    tmp2=tmp;
    for(i=0; i<nl; ++i, ++ktmp, ++xldtmp, ++tmp2)
    {
        xldtmp2=xld;
        x_2 = (float32_t *)xld;

        fa2=fb2=0;
        for(j=0;j<nl_tmp;j+=4, Qtmp+=4, xldtmp2+=4){
            a_1 = vld2q_f32(q);
            q+=8;
            b_1 = vld2q_f32(x_2);
            x_2+=8;
            // c_a_1 = vsubq_f32(vmulq_f32(a_1.val[0], b_1.val[0]), vmulq_f32(a_1.val[1], b_1.val[1]));
            // c_b_1 = vaddq_f32(vmulq_f32(a_1.val[0], b_1.val[1]), vmulq_f32(a_1.val[1], b_1.val[0]));
            c_a_1 = vmlsq_f32(vmulq_f32(a_1.val[0], b_1.val[0]), a_1.val[1], b_1.val[1]);
            c_b_1 = vmlaq_f32(vmulq_f32(a_1.val[0], b_1.val[1]), a_1.val[1], b_1.val[0]);
            fa2 += vgetq_lane_f32(c_a_1, 0) + vgetq_lane_f32(c_a_1, 1) + vgetq_lane_f32(c_a_1, 2) + vgetq_lane_f32(c_a_1, 3);
            fb2 += -(vgetq_lane_f32(c_b_1, 0) + vgetq_lane_f32(c_b_1, 1) + vgetq_lane_f32(c_b_1, 2) + vgetq_lane_f32(c_b_1, 3));
        }
        for(j=nl_tmp;j<nl;++j, ++xldtmp2, ++Qtmp){
            fa2 += Qtmp->a*xldtmp2->a-Qtmp->b*xldtmp2->b;
            fb2 += -Qtmp->a*xldtmp2->b-Qtmp->b*xldtmp2->a;
            q+=2;
        }
        ktmp->a=fa2;
        ktmp->b=fb2;

        tmp2->a=fa2;
        tmp2->b=-fb2;
        
        fa+=fa2*xldtmp->a-fb2*xldtmp->b;
    }
    fa_1 = fa;
    fa=1.0/(fa+lambda/w);

    ktmp=K;
    k=(float32_t *)K;
    k1=(float32x4_t *)K;
    t1=(float32_t)fa;
    for(i=0;i<nl_tmp;i+=4){
        vst1q_f32(k, vmulq_n_f32(k1[0], t1));
        k+=4;
        ++k1;
        vst1q_f32(k, vmulq_n_f32(k1[0], t1));
        k+=4;
        ++k1;
        ktmp+=4;
    }
    for(i=nl_tmp;i<nl;++i,++ktmp){
        ktmp->a*=fa;
        ktmp->b*=fa;
    }

    if(Q_eye_alpha!=-1){
        float input_energy = 0;
        xldtmp=xld;
        for(j=0; j<nl; ++j, ++xldtmp)
        {
            input_energy += xldtmp->a * xldtmp->a + xldtmp->b * xldtmp->b;
        }
        Q_eye += Q_eye_alpha * input_energy;
    }

    Qtmp=Q;
    tmp2=tmp;
    for(i=0; i<nl; ++i, ++tmp2)
    {
        ktmp=K+i;
        Qtmp+=i;
        for(j=i; j<nl; ++j, ++ktmp, ++Qtmp)
        {
            if(i!=j)
            {
                Qtmp->a=(Qtmp->a-(ktmp->a*tmp2->a-ktmp->b*tmp2->b))*lambdaf;
                Qtmp->b=(Qtmp->b-(ktmp->a*tmp2->b+ktmp->b*tmp2->a))*lambdaf;

                Q[j*nl+i].a=Qtmp->a;
                Q[j*nl+i].b=-Qtmp->b;
            }else
            {
                Qtmp->a=(Qtmp->a-(ktmp->a*tmp2->a-ktmp->b*tmp2->b))*lambdaf+Q_eye;
                Qtmp->b=0;
            }
        }
    }

    Gtmp=G;
    Etmp=E;

    for(j=0; j<channel; ++j, ++Etmp)
    {
        ktmp=K;
        k = (float32_t *)K;
        e_a = (float32_t)Etmp->a;
        e_b = (float32_t)Etmp->b;
        for(i=0; i<nl_tmp; i+=4, ktmp+=4){
            a_1 = vld2q_f32(k);
            k+=8;
            // c_a_1 = vsubq_f32(vmulq_n_f32(a_1.val[0], e_a), vmulq_n_f32(a_1.val[1], e_b));
            // c_b_1 = vaddq_f32(vmulq_n_f32(a_1.val[0], e_b), vmulq_n_f32(a_1.val[1], e_a));
            c_a_1 = vmlsq_n_f32(vmulq_n_f32(a_1.val[0], e_a), a_1.val[1], e_b);
            c_b_1 = vmlaq_n_f32(vmulq_n_f32(a_1.val[0], e_b), a_1.val[1], e_a);
            Gtmp->a += vgetq_lane_f32(c_a_1, 0);
            Gtmp->b += vgetq_lane_f32(c_b_1, 0);
            ++Gtmp;
            Gtmp->a += vgetq_lane_f32(c_a_1, 1);
            Gtmp->b += vgetq_lane_f32(c_b_1, 1);
            ++Gtmp;
            Gtmp->a += vgetq_lane_f32(c_a_1, 2);
            Gtmp->b += vgetq_lane_f32(c_b_1, 2);
            ++Gtmp;
            Gtmp->a += vgetq_lane_f32(c_a_1, 3);
            Gtmp->b += vgetq_lane_f32(c_b_1, 3);
            ++Gtmp;
        }
        for(i=nl_tmp;i<nl;++i, ++ktmp, ++Gtmp){
            Gtmp->a += ktmp->a*Etmp->a-ktmp->b*Etmp->b;
            Gtmp->b += ktmp->a*Etmp->b+ktmp->b*Etmp->a;
        }
    }

    if(min_lemma!= 1.0 || max_lemma!= 1.0){
        float grad_lambda;
        float delta_lambda;
        grad_lambda = -2 * 2.718 * (fa_1 - 1.0) /(lambda * lambda);
        grad_lambda = max(-1e3, min(1e3, grad_lambda));
        delta_lambda = grad_lambda;  // 可加上学习率
        rls->lambda = 1.0/(1.0 + expf(-delta_lambda));
        rls->lambda = max(min_lemma, min(max_lemma, rls->lambda));
    }

    memset(Y, 0, sizeof(wtk_complex_t)*channel);
    Ytmp=Y;
    Etmp=E;
    Gtmp=G;

    g = (float32_t *)G;
    for(i=0; i<channel; ++i, ++Ytmp)
    {
        xldtmp=xld;
        x = (float32_t *)xld;

        y_t_a = y_t_b = 0;
        for(j=0; j<nl_tmp; j+=4, xldtmp+=4, Gtmp+=4){
            a_1 = vld2q_f32(x);
            x+=8;
            b_1 = vld2q_f32(g);
            g+=8;
            // c_a_1 = vsubq_f32(vmulq_f32(a_1.val[0], b_1.val[0]), vmulq_f32(a_1.val[1], b_1.val[1]));
            // c_b_1 = vaddq_f32(vmulq_f32(a_1.val[0], b_1.val[1]), vmulq_f32(a_1.val[1], b_1.val[0]));
            c_a_1 = vmlsq_f32(vmulq_f32(a_1.val[0], b_1.val[0]), a_1.val[1], b_1.val[1]);
            c_b_1 = vmlaq_f32(vmulq_f32(a_1.val[0], b_1.val[1]), a_1.val[1], b_1.val[0]);
            y_t_a += vgetq_lane_f32(c_a_1, 0) + vgetq_lane_f32(c_a_1, 1) + vgetq_lane_f32(c_a_1, 2) + vgetq_lane_f32(c_a_1, 3);
            y_t_b += vgetq_lane_f32(c_b_1, 0) + vgetq_lane_f32(c_b_1, 1) + vgetq_lane_f32(c_b_1, 2) + vgetq_lane_f32(c_b_1, 3);
        }
        for(j=nl_tmp; j<nl; ++j, ++xldtmp, ++Gtmp){
            y_t_a += xldtmp->a*Gtmp->a-xldtmp->b*Gtmp->b;
            y_t_b += xldtmp->a*Gtmp->b+xldtmp->b*Gtmp->a;
        }
        Ytmp->a += y_t_a;
        Ytmp->b += y_t_b;
    }

    Ytmp=Y;
    f_in=(float32_t *)in;
    y=(float32_t *)Ytmp;
    for(i=0;i<chn_tmp;i+=4,Ytmp+=4){
        a_1 = vld2q_f32(f_in);
        f_in+=8;
        b_1 = vld2q_f32(y);
        y+=8;
        a_a_1 = vsubq_f32(a_1.val[0], b_1.val[0]);
        a_b_1 = vsubq_f32(a_1.val[1], b_1.val[1]);
        Etmp->a = vgetq_lane_f32(a_a_1, 0);
        Etmp->b = vgetq_lane_f32(a_b_1, 0);
        ++Etmp;
        Etmp->a = vgetq_lane_f32(a_a_1, 1);
        Etmp->b = vgetq_lane_f32(a_b_1, 1);
        ++Etmp;
        Etmp->a = vgetq_lane_f32(a_a_1, 2);
        Etmp->b = vgetq_lane_f32(a_b_1, 2);
        ++Etmp;
        Etmp->a = vgetq_lane_f32(a_a_1, 3);
        Etmp->b = vgetq_lane_f32(a_b_1, 3);
        ++Etmp;
    }
    for(i=chn_tmp;i<channel;++i,++Ytmp,++Etmp){
        Etmp->a=in[i].a-Ytmp->a;
        Etmp->b=in[i].b-Ytmp->b;
    }

    if(rls->cfg->use_admm)
    {
        memcpy(u,Y,sizeof(wtk_complex_t)*channel);
        memset(a,0,sizeof(wtk_complex_t)*channel);
        memset(z,0,sizeof(wtk_complex_t)*channel);

        Qtmp=Q;
        ktmp=K;
        fa=0;
        xldtmp=xld;
        for(i=0; i<nl; ++i, ++ktmp, ++xldtmp)
        {
            xldtmp2=xld;

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
        fa=1.0/(fa+2/px);

        ktmp=K;
        for(i=0; i<nl; ++i, ++ktmp)
        {
            ktmp->a*=fa;
            ktmp->b*=fa;
        }

        for(l=0;l<iters;++l)
        {
            Gtmp=G;
            u1=u;a1=a;z1=z;
            for(j=0; j<channel; ++j, ++u1,++a1,++z1)
            {
                fa2=z1->a+a1->a-u1->a;
                fb2=z1->b+a1->b-u1->b;
                ktmp=K;
                for(i=0; i<nl; ++i, ++ktmp, ++Gtmp)
                {
                    Gtmp->a+=ktmp->a*fa2-ktmp->b*fb2;
                    Gtmp->b+=ktmp->a*fb2+ktmp->b*fa2;
                }
            }

            memset(u, 0, sizeof(wtk_complex_t)*channel);
            u1=u;
            Gtmp=G;
            for(i=0; i<channel; ++i, ++u1)
            {
                xldtmp=xld;
                for(j=0; j<nl; ++j, ++xldtmp, ++Gtmp)
                {
                    u1->a+=xldtmp->a*Gtmp->a-xldtmp->b*Gtmp->b;
                    u1->b+=xldtmp->a*Gtmp->b+xldtmp->b*Gtmp->a;
                }
            }

            if(l==iters-1)
            {
                break;
            }
            for(z1=z,a1=a,u1=u,i=0;i<channel;++i,++z1,++a1,++u1)
            {
                z1->a=u1->a-a1->a;
                z1->b=u1->b-a1->b;

                w=sqrt(z1->a*z1->a+z1->b*z1->b);
                w=min(phru[i]/(w+sigma),1);
                z1->a*=w;
                z1->b*=w;

                a1->a+=z1->a-u1->a;
                a1->b+=z1->b-u1->b;
                // z1->a+=a1->a-u1->a;
                // z1->b+=a1->b-u1->b;
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
        for(i=0; i<channel; ++i, ++Ytmp, ++Etmp, ++u1)
        {
            Ytmp->a=u1->a;
            Ytmp->b=u1->b;
            Etmp->a=in[i].a-Ytmp->a;
            Etmp->b=in[i].b-Ytmp->b;
        }
    }
}
#else
void wtk_rls_feed2(wtk_rls_t *rls, wtk_complex_t *in, wtk_complex_t *xld)
{
    float sigma=rls->cfg->sigma;
    float p=rls->cfg->p;
    float w_alpha=rls->cfg->w_alpha;
    // float lambda=rls->cfg->lemma;
    float lambda=rls->lambda;
    float lambdaf=1.0/lambda;
    float min_lemma=rls->cfg->min_lemma;
    float max_lemma=rls->cfg->max_lemma;
    float Q_eye=rls->cfg->Q_eye;
    float Q_eye_alpha=rls->cfg->Q_eye_alpha;
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

    wtk_dcomplex_t *K=rls->K, *ktmp;
    wtk_dcomplex_t *tmp=rls->tmp, *tmp2;
    float px=rls->cfg->px;
    int channel=rls->cfg->channel;
    int nl=rls->cfg->nl;
    int i,j;
    double w,fa,fa2,fb2;
    float fa_1;

    ++rls->nframe;

    memset(Y, 0, sizeof(wtk_complex_t)*channel);
    Ytmp=Y;
    Etmp=E;
    Gtmp=G;
    for(i=0; i<channel; ++i, ++Ytmp, ++Etmp)
    {
        xldtmp=xld;
        for(j=0; j<nl; ++j, ++xldtmp, ++Gtmp)
        {
            Ytmp->a+=xldtmp->a*Gtmp->a-xldtmp->b*Gtmp->b;
            Ytmp->b+=xldtmp->a*Gtmp->b+xldtmp->b*Gtmp->a;
        }

        Etmp->a=in[i].a-Ytmp->a;
        Etmp->b=in[i].b-Ytmp->b;
    }

    if(wx)
    {
        wxtmp=wx;
        Etmp=E;
        if(rls->nframe==1)
        {
            for(i=0; i<channel; ++i, ++Etmp, ++wxtmp)
            {
                wxtmp->a=Etmp->a;
                wxtmp->b=Etmp->b;
            }
        }else
        {
            for(i=0; i<channel; ++i, ++Etmp, ++wxtmp)
            {
                wxtmp->a=(1-w_alpha)*wxtmp->a+w_alpha*Etmp->a;
                wxtmp->b=(1-w_alpha)*wxtmp->b+w_alpha*Etmp->b;
            }
        }

        wxtmp=wx;
        w=0;
        for(i=0; i<channel; ++i, ++wxtmp)
        {
            w+=wxtmp->a*wxtmp->a+wxtmp->b*wxtmp->b;
        }
        w=pow(w/channel+sigma, p/2-1);
    }else if(x2)
    {
        for(i=0;i<channel;++i)
        {
            x2[i]=in[i].a*in[i].a+in[i].b*in[i].b;
        }

        for(i=0;i<nd;++i)
        {
            memcpy(phrx[i],phrx[i+1],sizeof(float)*channel);
        }

        phrxtmp=phrx[nd];
        phrxtmp1=phrx[nd-1];
        if(rls->nframe>1)
        {
            for(i=0;i<channel;++i)
            {
                phrxtmp[i]=(1-w_alpha)*phrxtmp1[i]+w_alpha*x2[i];
            }
        }else
        {
            for(i=0;i<channel;++i)
            {
                phrxtmp[i]=x2[i];
            }
        }

        if(rls->nframe>nd)
        {
            phrxtmp=phrx[0];
            for(i=0;i<channel;++i)
            {
                phrr[i]=exp(-2*delta)*phrxtmp[i];
            }
        }else
        {
            for(i=0;i<channel;++i)
            {
                phrr[i]=0;
            }
        }

        for(i=0;i<channel;++i)
        {
            phru[i]=min(sqrt(phrr[i]),sqrt(x2[i]));
            x3[i]=max(x2[i]-phrr[i],0);
        }

        if(rls->nframe>1)
        {
            for(i=0;i<channel;++i)
            {
                phrd[i]=(1-w_alpha)*phrd[i]+w_alpha*x3[i];
            }
        }else
        {
            for(i=0;i<channel;++i)
            {
                phrd[i]=x3[i];
            }
        }
        w=0;
        for(i=0;i<channel;++i)
        {
            w+=phrd[i];
        }
        w=pow((w/channel+sigma), p/2-1);
    }else
    {
        w=1;
    }

    Qtmp=Q;
    ktmp=K;
    fa=0;
    xldtmp=xld;
    tmp2=tmp;
    for(i=0; i<nl; ++i, ++ktmp, ++xldtmp, ++tmp2)
    {
        xldtmp2=xld;

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
    fa_1 = fa;
    fa=1.0/(fa+lambda/w);

    ktmp=K;
    for(i=0; i<nl; ++i, ++ktmp)
    {
        ktmp->a*=fa;
        ktmp->b*=fa;
    }

    if(Q_eye_alpha!=-1){
        float input_energy = 0;
        xldtmp=xld;
        for(j=0; j<nl; ++j, ++xldtmp)
        {
            input_energy += xldtmp->a * xldtmp->a + xldtmp->b * xldtmp->b;
        }
        Q_eye += Q_eye_alpha * input_energy;
    }

    Qtmp=Q;
    tmp2=tmp;
    for(i=0; i<nl; ++i, ++tmp2)
    {
        ktmp=K+i;
        Qtmp+=i;
        for(j=i; j<nl; ++j, ++ktmp, ++Qtmp)
        {
            if(i!=j)
            {
                Qtmp->a=(Qtmp->a-(ktmp->a*tmp2->a-ktmp->b*tmp2->b))*lambdaf;
                Qtmp->b=(Qtmp->b-(ktmp->a*tmp2->b+ktmp->b*tmp2->a))*lambdaf;

                Q[j*nl+i].a=Qtmp->a;
                Q[j*nl+i].b=-Qtmp->b;
            }else
            {
                Qtmp->a=(Qtmp->a-(ktmp->a*tmp2->a-ktmp->b*tmp2->b))*lambdaf+Q_eye;
                Qtmp->b=0;
            }
        }
    }

    Gtmp=G;
    Etmp=E;
    for(j=0; j<channel; ++j, ++Etmp)
    {
        ktmp=K;
        for(i=0; i<nl; ++i, ++ktmp, ++Gtmp)
        {
            Gtmp->a+=ktmp->a*Etmp->a-ktmp->b*Etmp->b;
            Gtmp->b+=ktmp->a*Etmp->b+ktmp->b*Etmp->a;
        }
    }

    if(min_lemma!= 1.0 || max_lemma!= 1.0){
        float grad_lambda;
        float delta_lambda;
        grad_lambda = -2 * 2.718 * (fa_1 - 1.0) /(lambda * lambda);
        grad_lambda = max(-1e3, min(1e3, grad_lambda));
        delta_lambda = grad_lambda;  // 可加上学习率
        rls->lambda = 1.0/(1.0 + expf(-delta_lambda));
        rls->lambda = max(min_lemma, min(max_lemma, rls->lambda));
    }

    memset(Y, 0, sizeof(wtk_complex_t)*channel);
    Ytmp=Y;
    Etmp=E;
    Gtmp=G;
    for(i=0; i<channel; ++i, ++Ytmp, ++Etmp)
    {
        xldtmp=xld;
        for(j=0; j<nl; ++j, ++xldtmp, ++Gtmp)
        {
            Ytmp->a+=xldtmp->a*Gtmp->a-xldtmp->b*Gtmp->b;
            Ytmp->b+=xldtmp->a*Gtmp->b+xldtmp->b*Gtmp->a;
        }

        Etmp->a=in[i].a-Ytmp->a;
        Etmp->b=in[i].b-Ytmp->b;
    }

    if(rls->cfg->use_admm)
    {
        memcpy(u,Y,sizeof(wtk_complex_t)*channel);
        memset(a,0,sizeof(wtk_complex_t)*channel);
        memset(z,0,sizeof(wtk_complex_t)*channel);

        Qtmp=Q;
        ktmp=K;
        fa=0;
        xldtmp=xld;
        for(i=0; i<nl; ++i, ++ktmp, ++xldtmp)
        {
            xldtmp2=xld;

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
        fa=1.0/(fa+2/px);

        ktmp=K;
        for(i=0; i<nl; ++i, ++ktmp)
        {
            ktmp->a*=fa;
            ktmp->b*=fa;
        }

        for(l=0;l<iters;++l)
        {
            Gtmp=G;
            u1=u;a1=a;z1=z;
            for(j=0; j<channel; ++j, ++u1,++a1,++z1)
            {
                fa2=z1->a+a1->a-u1->a;
                fb2=z1->b+a1->b-u1->b;
                ktmp=K;
                for(i=0; i<nl; ++i, ++ktmp, ++Gtmp)
                {
                    Gtmp->a+=ktmp->a*fa2-ktmp->b*fb2;
                    Gtmp->b+=ktmp->a*fb2+ktmp->b*fa2;
                }
            }

            memset(u, 0, sizeof(wtk_complex_t)*channel);
            u1=u;
            Gtmp=G;
            for(i=0; i<channel; ++i, ++u1)
            {
                xldtmp=xld;
                for(j=0; j<nl; ++j, ++xldtmp, ++Gtmp)
                {
                    u1->a+=xldtmp->a*Gtmp->a-xldtmp->b*Gtmp->b;
                    u1->b+=xldtmp->a*Gtmp->b+xldtmp->b*Gtmp->a;
                }
            }

            if(l==iters-1)
            {
                break;
            }
            for(z1=z,a1=a,u1=u,i=0;i<channel;++i,++z1,++a1,++u1)
            {
                z1->a=u1->a-a1->a;
                z1->b=u1->b-a1->b;

                w=sqrt(z1->a*z1->a+z1->b*z1->b);
                w=min(phru[i]/(w+sigma),1);
                z1->a*=w;
                z1->b*=w;

                a1->a+=z1->a-u1->a;
                a1->b+=z1->b-u1->b;
                // z1->a+=a1->a-u1->a;
                // z1->b+=a1->b-u1->b;
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
        for(i=0; i<channel; ++i, ++Ytmp, ++Etmp, ++u1)
        {
            Ytmp->a=u1->a;
            Ytmp->b=u1->b;
            Etmp->a=in[i].a-Ytmp->a;
            Etmp->b=in[i].b-Ytmp->b;
        }
    }
}
#endif

void wtk_rls_feed(wtk_rls_t *rls, wtk_complex_t *in, wtk_complex_t *u)
{
    int L=rls->cfg->L;
    int N=rls->cfg->N;
    wtk_complex_t *xld=rls->xld;

    memmove(xld+N, xld, sizeof(wtk_complex_t)*N*(L-1));
    memcpy(xld, u, sizeof(wtk_complex_t)*N);

#ifdef USE_NEON
    wtk_rls_feed2_neon(rls, in, xld);
#else
    wtk_rls_feed2(rls, in, xld);
#endif
}

void wtk_rls_feed3(wtk_rls_t *rls, wtk_complex_t *in, wtk_complex_t *u, int switch_b)
{
    int L=rls->cfg->L;
    int N=rls->cfg->N;
    wtk_complex_t *xld=rls->xld;

    memmove(xld+N, xld, sizeof(wtk_complex_t)*N*(L-1));
    memcpy(xld, u, sizeof(wtk_complex_t)*N);

    if(switch_b)
    {
#ifdef USE_NEON
        wtk_rls_feed2_neon(rls, in, xld);
#else
        wtk_rls_feed2(rls, in, xld);
#endif
    }
}
