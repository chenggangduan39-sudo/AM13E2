#include "wtk_rls2.h" 

wtk_rls2_t *wtk_rls2_new(wtk_rls2_cfg_t *cfg)
{
    wtk_rls2_t *rls2;

    rls2=(wtk_rls2_t *)wtk_malloc(sizeof(wtk_rls2_t));
    rls2->cfg=cfg;

    rls2->xld=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->nl);

    rls2->G=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->nl*cfg->channel);
    rls2->Q=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->nl*cfg->nl);

    rls2->K=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->nl);
    rls2->tmp=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->nl);

    rls2->wx=NULL;
    rls2->x2=rls2->x3=rls2->phrr=rls2->phru=rls2->phrd=NULL;
    rls2->phrx=NULL;
    rls2->u=rls2->a=rls2->z=NULL;
    if(cfg->use_wx)
    {
        rls2->wx=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->channel);
    }else if(cfg->use_admm)
    {
        rls2->x2=(float *)wtk_malloc(sizeof(float)*cfg->channel);
        rls2->x3=(float *)wtk_malloc(sizeof(float)*cfg->channel);
        rls2->phrr=(float *)wtk_malloc(sizeof(float)*cfg->channel);
        rls2->phru=(float *)wtk_malloc(sizeof(float)*cfg->channel);

        rls2->phrx=wtk_float_new_p2(cfg->nd+1,cfg->channel);
        rls2->phrd=(float *)wtk_malloc(sizeof(float)*cfg->channel);

    	rls2->u=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->channel);
        rls2->a=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->channel);
        rls2->z=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->channel);
    }
    rls2->out=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->channel);
    rls2->lsty=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->channel);

    wtk_rls2_reset(rls2);

    return rls2;
}

void wtk_rls2_delete(wtk_rls2_t *rls2)
{
    wtk_free(rls2->xld);
    wtk_free(rls2->G);
    wtk_free(rls2->Q);
    if(rls2->wx)
    {
        wtk_free(rls2->wx);
    }else if(rls2->x2)
    {
        wtk_free(rls2->x2);
        wtk_free(rls2->x3);
        wtk_free(rls2->phrr);
        wtk_free(rls2->phru);
        wtk_free(rls2->phrd);
        wtk_free(rls2->u);
        wtk_free(rls2->a);
        wtk_free(rls2->z);
        wtk_float_delete_p2(rls2->phrx, rls2->cfg->nd+1);
    }
    wtk_free(rls2->out);
    wtk_free(rls2->lsty);
    wtk_free(rls2->K);
    wtk_free(rls2->tmp);
    wtk_free(rls2);
}

void wtk_rls2_init(wtk_rls2_t *rls2, wtk_rls2_cfg_t *cfg)
{
    rls2->cfg=cfg;

    rls2->xld=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->nl);

    rls2->G=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->nl*cfg->channel);
    rls2->Q=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->nl*cfg->nl);

    rls2->K=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->nl);
    rls2->tmp=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->nl);

    rls2->wx=NULL;
    rls2->x2=rls2->x3=rls2->phrr=rls2->phru=rls2->phrd=NULL;
    rls2->phrx=NULL;
    rls2->u=rls2->a=rls2->z=NULL;
    if(cfg->use_wx)
    {
        rls2->wx=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->channel);
    }else if(cfg->use_admm)
    {
        rls2->x2=(float *)wtk_malloc(sizeof(float)*cfg->channel);
        rls2->x3=(float *)wtk_malloc(sizeof(float)*cfg->channel);
        rls2->phrr=(float *)wtk_malloc(sizeof(float)*cfg->channel);
        rls2->phru=(float *)wtk_malloc(sizeof(float)*cfg->channel);

        rls2->phrx=wtk_float_new_p2(cfg->nd+1,cfg->channel);
        rls2->phrd=(float *)wtk_malloc(sizeof(float)*cfg->channel);

    	rls2->u=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->channel);
        rls2->a=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->channel);
        rls2->z=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->channel);
    }
    rls2->out=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->channel);
    rls2->lsty=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->channel);

    wtk_rls2_reset(rls2);
}

void wtk_rls2_clean(wtk_rls2_t *rls2)
{
    wtk_free(rls2->xld);
    wtk_free(rls2->G);
    wtk_free(rls2->Q);
    if(rls2->wx)
    {
        wtk_free(rls2->wx);
    }else if(rls2->x2)
    {
        wtk_free(rls2->x2);
        wtk_free(rls2->x3);
        wtk_free(rls2->phrr);
        wtk_free(rls2->phru);
        wtk_free(rls2->phrd);
        wtk_free(rls2->u);
        wtk_free(rls2->a);
        wtk_free(rls2->z);
        wtk_float_delete_p2(rls2->phrx, rls2->cfg->nd+1);
    }
    wtk_free(rls2->out);
    wtk_free(rls2->lsty);
    wtk_free(rls2->K);
    wtk_free(rls2->tmp);
}

void wtk_rls2_reset(wtk_rls2_t *rls2)
{
    int i;

    memset(rls2->xld, 0, sizeof(wtk_complex_t)*rls2->cfg->nl);

    memset(rls2->G, 0, sizeof(wtk_complex_t)*rls2->cfg->channel*rls2->cfg->nl);
    memset(rls2->Q, 0, sizeof(wtk_complex_t)*rls2->cfg->nl*rls2->cfg->nl);
    for(i=0; i<rls2->cfg->nl; ++i)
    {
        rls2->Q[i*rls2->cfg->nl+i].a=1;
    }
    if(rls2->wx)
    {
        memset(rls2->wx, 0, sizeof(wtk_complex_t)*rls2->cfg->channel);
    }
    memset(rls2->out, 0, sizeof(wtk_complex_t)*rls2->cfg->channel);
    memset(rls2->lsty, 0, sizeof(wtk_complex_t)*rls2->cfg->channel);
    memset(rls2->K, 0, sizeof(wtk_complex_t)*rls2->cfg->nl);
    memset(rls2->tmp, 0, sizeof(wtk_complex_t)*rls2->cfg->nl);

    rls2->nframe=0;
    rls2->lambda=rls2->cfg->lemma;
}

void wtk_rls2_reset2(wtk_rls2_t *rls2)
{
    memset(rls2->xld, 0, sizeof(wtk_complex_t)*rls2->cfg->nl);
}

void wtk_rls2_feed2(wtk_rls2_t *rls2, wtk_complex_t *in, wtk_complex_t *xld)
{
    float sigma=rls2->cfg->sigma;
    float p=rls2->cfg->p;
    float w_alpha=rls2->cfg->w_alpha;
    // float lambda=rls2->cfg->lemma;
    float lambda=rls2->lambda;
    float lambdaf=1.0/lambda;
    float min_lemma=rls2->cfg->min_lemma;
    float max_lemma=rls2->cfg->max_lemma;
    float Q_eye=rls2->cfg->Q_eye;
    float Q_eye_alpha=rls2->cfg->Q_eye_alpha;
    float delta=rls2->cfg->delta;
    int nd=rls2->cfg->nd;

    wtk_complex_t *Y=rls2->lsty, *Ytmp;
    wtk_complex_t *E=rls2->out, *Etmp;
    wtk_complex_t *G=rls2->G, *Gtmp;
    wtk_complex_t *Q=rls2->Q, *Qtmp;

    wtk_complex_t *wx=rls2->wx, *wxtmp;
    float *x2=rls2->x2, *x3=rls2->x3, *phrr=rls2->phrr, *phru=rls2->phru;
    float **phrx=rls2->phrx, *phrxtmp, *phrxtmp1;
    float *phrd=rls2->phrd;
    int l,iters=rls2->cfg->iters;
    wtk_complex_t *u=rls2->u,*u1,*a=rls2->a,*a1,*z=rls2->z,*z1;

    wtk_complex_t *xldtmp, *xldtmp2;
    wtk_complex_t *K=rls2->K, *ktmp;
    wtk_complex_t *tmp=rls2->tmp, *tmp2;
    float px=rls2->cfg->px;
    int channel=rls2->cfg->channel;
    int nl=rls2->cfg->nl;
    int i,j;
    float w,fa,fa2,fb2;
    float fa_1;

    ++rls2->nframe;

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
        if(rls2->nframe==1)
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
        if(rls2->nframe>1)
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

        if(rls2->nframe>nd)
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

        if(rls2->nframe>1)
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
        rls2->lambda = 1.0/(1.0 + expf(-delta_lambda));
        rls2->lambda = max(min_lemma, min(max_lemma, rls2->lambda));
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

    if(rls2->cfg->use_admm)
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
        if(rls2->cfg->use_zvar)
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


void wtk_rls2_feed(wtk_rls2_t *rls2, wtk_complex_t *in, wtk_complex_t *u)
{
    int L=rls2->cfg->L;
    int N=rls2->cfg->N;
    wtk_complex_t *xld=rls2->xld;

    memmove(xld+N, xld, sizeof(wtk_complex_t)*N*(L-1));
    memcpy(xld, u, sizeof(wtk_complex_t)*N);

    wtk_rls2_feed2(rls2, in, xld);
}