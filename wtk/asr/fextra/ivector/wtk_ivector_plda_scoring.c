#include "wtk_ivector_plda_scoring.h"
#include "wtk/core/math/wtk_math.h"
#ifdef USE_NEON
#include "wtk/os/asm/arm/wtk_neon_math.h"
#endif
wtk_ivector_plda_scoring_t* wtk_ivector_plda_scoring_new(wtk_ivector_plda_scoring_cfg_t *cfg)
{
    wtk_ivector_plda_scoring_t *s;

    s=wtk_malloc(sizeof(*s));
    s->cfg=cfg;
    s->tmp=wtk_vecf_new(cfg->offset->len);
    s->tmp2=wtk_vecf_new(cfg->psi->len);
    s->tmp3=wtk_vecf_new(cfg->offset->len);
    s->ivector=wtk_vecf_new(cfg->offset->len);

    return s;
}

void wtk_ivector_plda_scoring_delete(wtk_ivector_plda_scoring_t *s)
{
    wtk_vecf_delete(s->ivector);
    wtk_vecf_delete(s->tmp);
    wtk_vecf_delete(s->tmp2);
    wtk_vecf_delete(s->tmp3);
    wtk_free(s);
}

float wtk_ivector_plda_scoring_loglikelihood(wtk_ivector_plda_scoring_t *s,wtk_vecf_t *feat1,int cnt,wtk_vecf_t *feat2)
{
    wtk_vecf_t *mean=s->tmp;
    wtk_vecf_t *var=s->tmp2;
    wtk_vecf_t *tmp3=s->tmp3;
    wtk_vecf_t *psi=s->cfg->psi;
    int i;
    float logdet;
    float loglike_given_class,loglike_without_class;
    float sum;
    float loglike_ratio;
    int dim;
    dim=mean->len;
    for(i=0;i<mean->len;++i)
    {
        mean->p[i] = cnt * psi->p[i]/(cnt*psi->p[i]+1.0f) * feat1->p[i];
        // printf("%d %f\n",i,feat2->p[i]);

        var->p[i] = 1.0f + psi->p[i]/(cnt*psi->p[i] + 1.0f);
        // printf("%d %f\n",i,var->p[i]);
    }

    logdet=wtk_vecf_sum_log(var);
    // printf("%f\n",logdet);
#ifdef USE_NEON
    wtk_neon_math_vec_sub_square(mean->p,mean->p,feat2->p,mean->len);
//    wtk_neon_math_vec_sub(mean->p,mean->p,feat2->p,mean->len);
//    wtk_neon_math_vec_mul(mean->p,mean->p,mean->p,mean->len);
#else
    for(i=0;i<mean->len;++i)
    {
        // printf("%d %f\n",i,feat2->p[i]);
        mean->p[i]=mean->p[i]-feat2->p[i];
        mean->p[i]=mean->p[i]*mean->p[i];

        // printf("%d %f\n",i,mean->p[i]);
    }
#endif

    wtk_vecf_invert(var,var);

    // for(i=0;i<var->len;++i)
    // {
    //     printf("%d %f\n",i,var->p[i]);
    // }
#ifdef USE_NEON
    wtk_neon_math_vec_mul_sum(&sum,mean->p,var->p,mean->len);
#else
    sum=wtk_vecf_vecvec(mean,var);
#endif
    // printf("sum %f\n",sum);

    loglike_given_class=-0.5f*(logdet+M_LOG_2PI*dim+sum);

//////////////////////////////////////////////////////////////////////////
#ifdef USE_NEON
    wtk_neon_math_vec_square(tmp3->p,feat2->p,feat2->len);
#else
    for(i=0;i<feat2->len;++i)
    {
        tmp3->p[i]=feat2->p[i]*feat2->p[i];
        // printf("%d %f\n",i,feat2->p[i]);
    }
#endif

#ifdef USE_NEON
    wtk_neon_math_vec_add_const(var->p,psi->p,1.0f,psi->len);
#else
    for(i=0;i<psi->len;++i)
    {
        var->p[i]=1.0f+psi->p[i];
    }
#endif

    logdet=wtk_vecf_sum_log(var);


    wtk_vecf_invert(var,var);

#ifdef USE_NEON
    wtk_neon_math_vec_mul_sum(&sum,var->p,tmp3->p,var->len);
#else
    sum=wtk_vecf_vecvec(var,tmp3);
#endif

    loglike_without_class=-0.5f*(logdet+M_LOG_2PI*dim+sum);

    // printf("%f %f\n",loglike_given_class,loglike_without_class);

    loglike_ratio=loglike_given_class-loglike_without_class;

    // printf("%f\n",loglike_ratio);
    return loglike_ratio;
}


wtk_vecf_t* wtk_ivector_plda_scoring_transfrom_ivector(wtk_ivector_plda_scoring_t *s,wtk_vecf_t *xvector)
{
    int i,j;
    float *p;

#ifdef USE_NEON
    wtk_neon_math_vecf_muti_matf_transf2_add_vec(s->ivector->p,s->cfg->transform->p,\
    											xvector->p,s->cfg->offset->p,\
												s->cfg->transform->row,s->cfg->transform->col);
#else
    memcpy(s->ivector->p,s->cfg->offset->p,s->ivector->len*sizeof(float));
    p=s->cfg->transform->p;
    for(i=0;i<s->cfg->transform->row;++i)
    {
        for(j=0;j<xvector->len;++j)
        {
            s->ivector->p[i]+=p[j]*xvector->p[j];
        }
        // printf("%d %f\n",i,s->ivector->p[i]);
        p+=s->cfg->transform->col;
    }
#endif

    return s->ivector;
}

float wtk_ivector_plda_scoring_get_normalization_factor(wtk_ivector_plda_scoring_t *s,int cnt)
{
    int i;
    float f;
    float sum;

    f=1.0f/cnt;
#ifdef USE_NEON
    wtk_neon_math_vec_square(s->tmp->p,s->ivector->p,s->ivector->len);
#else
    for(i=0;i<s->ivector->len;++i)
    {
        s->tmp->p[i]=s->ivector->p[i]*s->ivector->p[i];
        // printf("%f\n",s->tmp->p[i]);
    }
#endif

    for(i=0;i<s->cfg->psi->len;++i)
    {
        s->tmp2->p[i]=1.0f/(s->cfg->psi->p[i]+f);
        // printf("%f\n",s->tmp2->p[i]);
    }

#ifdef USE_NEON
    wtk_neon_math_vec_mul_sum(&sum,s->tmp->p,s->tmp2->p,s->tmp->len);
#else
    sum=0.0f;
    for(i=0;i<s->tmp->len;++i)
    {
        sum+=s->tmp->p[i]*s->tmp2->p[i];
    }
    // printf("sum %f\n",sum);
#endif

    return sqrt(s->ivector->len/sum);
}

wtk_vecf_t* wtk_ivector_plda_scoring_ivector_normalization(wtk_ivector_plda_scoring_t *s,int cnt)
{
    float f;
    int i;

    f=wtk_ivector_plda_scoring_get_normalization_factor(s,cnt);
#ifdef USE_NEON
    wtk_neon_math_vec_mul_const(s->ivector->p,s->ivector->p,f,s->ivector->len);
#else
    for(i=0;i<s->ivector->len;++i)
    {
        s->ivector->p[i]*=f;
        // printf("%d %f\n",i,s->ivector->p[i]);
    }
#endif
    return s->ivector;
} 

