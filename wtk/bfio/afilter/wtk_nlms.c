#include "wtk_nlms.h" 

wtk_nlms_t *wtk_nlms_new(wtk_nlms_cfg_t *cfg)
{
    wtk_nlms_t *nlms;

    nlms=(wtk_nlms_t *)wtk_malloc(sizeof(wtk_nlms_t));
    nlms->cfg=cfg;

    nlms->far_x=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->L);
    nlms->W=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->channel*cfg->L);
    nlms->prop=(float *)wtk_malloc(sizeof(float)*cfg->L);
    nlms->orth_agc=(float *)wtk_malloc(sizeof(float)*cfg->L);

    nlms->out=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->channel);
    nlms->lsty=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->channel);

    nlms->lsty_power=(float *)wtk_malloc(sizeof(float)*cfg->channel);
    nlms->Se=(float *)wtk_malloc(sizeof(float)*cfg->channel);
    nlms->Sd=(float *)wtk_malloc(sizeof(float)*cfg->channel);
    nlms->Sed=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->channel);
    nlms->leak=(float *)wtk_malloc(sizeof(float)*cfg->channel);

    wtk_nlms_reset(nlms);

    return nlms;
}

void wtk_nlms_delete(wtk_nlms_t *nlms)
{
    wtk_free(nlms->W);
    wtk_free(nlms->prop);
    wtk_free(nlms->orth_agc);
    wtk_free(nlms->far_x);
    wtk_free(nlms->out);
    wtk_free(nlms->lsty);
    wtk_free(nlms->lsty_power);
    wtk_free(nlms->Se);
    wtk_free(nlms->Sd);
    wtk_free(nlms->Sed);
    wtk_free(nlms->leak);
    wtk_free(nlms);
}

void wtk_nlms_init(wtk_nlms_t *nlms, wtk_nlms_cfg_t *cfg)
{
    nlms->cfg=cfg;
    nlms->far_x=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->L);
    nlms->W=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->channel*cfg->L);
    nlms->prop=(float *)wtk_malloc(sizeof(float)*cfg->L);
    nlms->orth_agc=(float *)wtk_malloc(sizeof(float)*cfg->L);

    nlms->out=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->channel);
    nlms->lsty=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->channel);

    nlms->lsty_power=(float *)wtk_malloc(sizeof(float)*cfg->channel);
    nlms->Se=(float *)wtk_malloc(sizeof(float)*cfg->channel);
    nlms->Sd=(float *)wtk_malloc(sizeof(float)*cfg->channel);
    nlms->Sed=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->channel);
    nlms->leak=(float *)wtk_malloc(sizeof(float)*cfg->channel);

    wtk_nlms_reset(nlms);
}

void wtk_nlms_clean(wtk_nlms_t *nlms)
{
    wtk_free(nlms->W);
    wtk_free(nlms->prop);
    wtk_free(nlms->orth_agc);
    wtk_free(nlms->far_x);
    wtk_free(nlms->out);
    wtk_free(nlms->lsty);
    wtk_free(nlms->lsty_power);
    wtk_free(nlms->Se);
    wtk_free(nlms->Sd);
    wtk_free(nlms->Sed);
    wtk_free(nlms->leak);
}

void wtk_nlms_reset(wtk_nlms_t *nlms)
{
    memset(nlms->prop, 0, sizeof(float)*nlms->cfg->L);
    memset(nlms->orth_agc, 0, sizeof(float)*nlms->cfg->L);
    memset(nlms->far_x, 0, sizeof(wtk_complex_t)*nlms->cfg->L);
    memset(nlms->W, 0, sizeof(wtk_complex_t)*nlms->cfg->channel*nlms->cfg->L);
    memset(nlms->out, 0, sizeof(wtk_complex_t)*nlms->cfg->channel);
    memset(nlms->lsty, 0, sizeof(wtk_complex_t)*nlms->cfg->channel);

    memset(nlms->Sed, 0, sizeof(wtk_complex_t)*nlms->cfg->channel);

    memset(nlms->Se, 0, sizeof(float)*nlms->cfg->channel);
    memset(nlms->Sd, 0, sizeof(float)*nlms->cfg->channel);
    memset(nlms->leak, 0, sizeof(float)*nlms->cfg->channel);
    memset(nlms->lsty_power, 0, sizeof(float)*nlms->cfg->channel);

    nlms->power_x=0;
    nlms->nframe=0;
    nlms->power_l=-1;
}

void wtk_nlms_reset2(wtk_nlms_t *nlms)
{
    memset(nlms->far_x, 0, sizeof(wtk_complex_t)*nlms->cfg->L);
}

void wtk_nlms_feed2(wtk_nlms_t *nlms, wtk_complex_t *n_near, wtk_complex_t *W, 
                                                                wtk_complex_t *E, float *Se, float *Sd, wtk_complex_t *Sed, float *leak, wtk_complex_t *lsty, float *lsty_power)
{
    int L=nlms->cfg->L;
    int M=nlms->cfg->M;
    wtk_complex_t *far_x=nlms->far_x, *fartmp;
    wtk_complex_t *wtmp;
    float leak_scale=nlms->cfg->leak_scale;
    float max_u=nlms->cfg->max_u;
    float min_u=nlms->cfg->min_u;
    float power_l;
    float yf, ef, r, e;
    float coh_alpha=nlms->cfg->coh_alpha;
    int i;
    float power_x=nlms->power_x;
    float *prop=nlms->prop;
    float f;
	float max_sum=1e-9;
    float *orth_agc=nlms->orth_agc;
    float orth_m=nlms->cfg->orth_m;
    float orth_s=nlms->cfg->orth_s;
    float orth2_m=nlms->cfg->orth2_m;
    float orth2_s=nlms->cfg->orth2_s;
    wtk_complex_t orth;
    float near_en;
    float far_en=0;
    float far_agc=1.0;
    float orth_agc2=1.0;
    near_en = n_near->a*n_near->a+n_near->b*n_near->b;

    lsty->a=lsty->b=0;
    fartmp=far_x;
    wtmp=W;
    for(i=0; i<L; ++i, ++fartmp, ++wtmp)
    {
        orth.a = fartmp->a*wtmp->a-fartmp->b*wtmp->b;
        orth.b = fartmp->a*wtmp->b+fartmp->b*wtmp->a;
        lsty->a+=orth.a;
        lsty->b+=orth.b;
        if(nlms->cfg->use_en_step){
            orth_agc[i]=orth_m+orth_s - min(max((orth.a*orth.a+orth.b*orth.b)/near_en*L*2, 0), orth_s*2);
            far_en+=fartmp->a*fartmp->a+fartmp->b*fartmp->b;
        }
    }

    if(nlms->cfg->use_en_step){
        far_en*=1.0/L;
        far_agc = min(max(far_en*1.0/1000, 0.1), 1.0);
        orth_agc2 = orth2_m+orth2_s - min(max((lsty->a*lsty->a+lsty->b*lsty->b)/near_en*2, 0), orth2_s*2);
    }

    E->a=n_near->a-lsty->a;
    E->b=n_near->b-lsty->b;

    if(nlms->nframe<=M)
    {
        *Se+=E->a*E->a+E->b*E->b;
        yf=lsty->a*lsty->a+lsty->b*lsty->b;
        *Sd+=yf;
        Sed->a+=lsty->a*E->a+lsty->b*E->b;
        Sed->b+=-lsty->a*E->b+lsty->b*E->a;

        *leak=(Sed->a*Sed->a+Sed->b*Sed->b)/(max(*Se,*Sd)*(*Sd)+1e-9)*leak_scale;
        *lsty_power=(*leak)*yf;

        power_l=min_u/(power_x+1e-9);

        if(nlms->nframe==M)
        {
            (*Se)/=M;
            (*Sd)/=M;
            Sed->a/=M;
            Sed->b/=M;
        }
    }else
    {
        ef=E->a*E->a+E->b*E->b;
        yf=lsty->a*lsty->a+lsty->b*lsty->b;

        *Se=(1-coh_alpha)* (*Se)+coh_alpha*ef;
        *Sd=(1-coh_alpha)* (*Sd)+coh_alpha*yf;
        Sed->a=(1-coh_alpha)*Sed->a+coh_alpha*(lsty->a*E->a+lsty->b*E->b);
        Sed->b=(1-coh_alpha)*Sed->b+coh_alpha*(-lsty->a*E->b+lsty->b*E->a);

        *leak=(Sed->a*Sed->a+Sed->b*Sed->b)/(max(*Se,*Sd)*(*Sd)+1e-9);
        r=(*leak)*yf;
        *lsty_power=r*leak_scale;

        e=max_u*ef;
        r=min(r,e);
        e=min_u*ef;
        r=max(r,e);
        power_l=r/(ef*power_x+1e-9);
    }

    if(nlms->cfg->power_l_scale > 0){
        if(nlms->power_l!=-1){
            power_l = min(power_l, nlms->cfg->power_l_scale * nlms->power_l);
        }
        nlms->power_l = max(power_l, nlms->power_l);
    }

    wtmp=W;
    for(i=0;i<L;++i,++wtmp)
    {
		f=1e-9;
        f+=wtmp->a*wtmp->a+wtmp->b*wtmp->b;
		prop[i]=sqrt(f);
		if(prop[i]>max_sum)
		{
			max_sum=prop[i];
		}
    }
    max_sum*=0.1;
	f=1e-9;
	for(i=0;i<L;++i)
	{
		f+=prop[i]+=max_sum;
	}
	f=0.99/f;
	for(i=0;i<L;++i)
	{
		prop[i]*=f;
	}

    wtmp=W;
    fartmp=far_x;
    for(i=0; i<L; ++i, ++wtmp, ++fartmp)
    {
        if(nlms->cfg->use_en_step){
            r=prop[i]*power_l*orth_agc[i]*orth_agc2*far_agc;
        }else{
            r=prop[i]*power_l;
        }
        wtmp->a+=r*(fartmp->a*E->a+fartmp->b*E->b);
        wtmp->b+=r*(-fartmp->b*E->a+fartmp->a*E->b);
    }

    if(nlms->cfg->use_sec_iter){
        fartmp=far_x;
        wtmp=W;
        lsty->a=lsty->b=0;
        for(i=0; i<L; ++i, ++fartmp, ++wtmp)
        {
            lsty->a+=fartmp->a*wtmp->a-fartmp->b*wtmp->b;
            lsty->b+=fartmp->a*wtmp->b+fartmp->b*wtmp->a;
        }
        E->a=n_near->a-lsty->a;
        E->b=n_near->b-lsty->b;
    }
}

void wtk_nlms_feed(wtk_nlms_t *nlms, wtk_complex_t *n_near, wtk_complex_t *n_far)
{
    int channel=nlms->cfg->channel;
    int i;
    int L=nlms->cfg->L;
    int M=nlms->cfg->M;
    int N=nlms->cfg->N;
    wtk_complex_t *far_x=nlms->far_x;
    float power_x; 
    wtk_complex_t *W=nlms->W;
    wtk_complex_t *out=nlms->out;
    float *lsty_power=nlms->lsty_power;
    wtk_complex_t *lsty=nlms->lsty;
    float *Se=nlms->Se;
    float *Sd=nlms->Sd;
    wtk_complex_t *Sed=nlms->Sed;
    float *leak=nlms->leak;
    float x_alpha=nlms->cfg->x_alpha;

    ++nlms->nframe;
    memmove(far_x+N, far_x, (M-1)*N*sizeof(wtk_complex_t));
    memcpy(far_x, n_far, N*sizeof(wtk_complex_t));

    power_x=0;
    for(i=0; i<N; ++i, ++n_far)
    {
        power_x+=n_far->a*n_far->a+n_far->b*n_far->b;
    }
    if(nlms->nframe<=1)
    {
        nlms->power_x+=power_x;
    }else
    {
        nlms->power_x=(1-x_alpha)* nlms->power_x+x_alpha*power_x;
    }

    for(i=0; i<channel; ++i, ++n_near, W+=L, ++out, ++Se, ++Sd, ++Sed, ++leak, ++lsty, ++lsty_power)
    {
        wtk_nlms_feed2(nlms, n_near, W, out, Se, Sd, Sed, leak, lsty, lsty_power);
    }
}

void wtk_nlms_feed3(wtk_nlms_t *nlms, wtk_complex_t *n_near, wtk_complex_t *n_far, int switch_b)
{
    int channel=nlms->cfg->channel;
    int i;
    int L=nlms->cfg->L;
    int M=nlms->cfg->M;
    int N=nlms->cfg->N;
    wtk_complex_t *far_x=nlms->far_x;
    float power_x; 
    wtk_complex_t *W=nlms->W;
    wtk_complex_t *out=nlms->out;
    float *lsty_power=nlms->lsty_power;
    wtk_complex_t *lsty=nlms->lsty;
    float *Se=nlms->Se;
    float *Sd=nlms->Sd;
    wtk_complex_t *Sed=nlms->Sed;
    float *leak=nlms->leak;
    float x_alpha=nlms->cfg->x_alpha;

    if(nlms->nframe<20000)
    {
        ++nlms->nframe;
    }
    memmove(far_x+N, far_x, (M-1)*N*sizeof(wtk_complex_t));
    memcpy(far_x, n_far, N*sizeof(wtk_complex_t));

    power_x=0;
    for(i=0; i<N; ++i, ++n_far)
    {
        power_x+=n_far->a*n_far->a+n_far->b*n_far->b;
    }
    if(nlms->nframe<=1)
    {
        nlms->power_x+=power_x;
    }else
    {
        nlms->power_x=(1-x_alpha)* nlms->power_x+x_alpha*power_x;
    }

    if(switch_b)
    {
        for(i=0; i<channel; ++i, ++n_near, W+=L, ++out, ++Se, ++Sd, ++Sed, ++leak, ++lsty, ++lsty_power)
        {
            wtk_nlms_feed2(nlms, n_near, W, out, Se, Sd, Sed, leak, lsty, lsty_power);
        }
    }
}
