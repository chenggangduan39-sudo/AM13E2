#include "wtk_maskssl.h" 

wtk_maskssl_t *wtk_maskssl_new(wtk_maskssl_cfg_t *cfg)
{
    wtk_maskssl_t *mssl;
    float theta,phi;
    int tstep=cfg->theta_step, pstep=cfg->phi_step;
    int n;

    mssl=(wtk_maskssl_t *)wtk_malloc(sizeof(wtk_maskssl_t));
    mssl->notify=NULL;
    mssl->ths=NULL;
    mssl->cfg=cfg;
    mssl->channel=cfg->maskaspec.channel;
    mssl->nbin=cfg->wins/2+1;

    mssl->ntheta=0;
    mssl->nangle=0;
    for(theta=0; theta<=cfg->max_theta; theta+=tstep)
    {
        ++mssl->ntheta;
        for(phi=0; phi<=cfg->max_phi; phi+=pstep)
        {
            ++mssl->nangle;
        }
    }
    mssl->nphi=mssl->nangle/mssl->ntheta;

    mssl->nextp=(wtk_maskssl_extp_t *)wtk_malloc(sizeof(wtk_maskssl_extp_t)*mssl->nangle);
    mssl->nbest_extp=(wtk_maskssl_extp_t *)wtk_malloc(sizeof(wtk_maskssl_extp_t)*mssl->cfg->max_extp);
    mssl->max_idx=(int *)wtk_malloc(sizeof(int)*mssl->cfg->max_extp);

    mssl->maskaspec=wtk_maskaspec_new(&(cfg->maskaspec), mssl->nbin, mssl->nangle);
    wtk_maskaspec_reset(mssl->maskaspec);

    mssl->maskaspec->start_ang_num=mssl->nangle;
    for(theta=0,n=0; theta<=cfg->max_theta; theta+=tstep)
    {
        for(phi=0; phi<=cfg->max_phi; phi+=pstep,++n)
        {
            mssl->nextp[n].theta=theta;
            mssl->nextp[n].phi=phi;
            wtk_maskaspec_start(mssl->maskaspec, theta, phi, n);
        }
    }

    mssl->kmask_s=mssl->kmask_n=NULL;
    mssl->scov=mssl->ncov=NULL;
    if(mssl->maskaspec->need_cov)
    {
        mssl->kmask_s=(float *)wtk_malloc(sizeof(float)*mssl->nbin);
        mssl->kmask_n=(float *)wtk_malloc(sizeof(float)*mssl->nbin);
        mssl->scov=wtk_complex_new_p2(mssl->nbin, mssl->channel*mssl->channel);
        mssl->ncov=wtk_complex_new_p2(mssl->nbin, mssl->channel*mssl->channel);
    }

    mssl->inv_cov=NULL;
    mssl->tmp=NULL;
    if(mssl->maskaspec->need_inv_ncov)
    {
        mssl->inv_cov=(wtk_complex_t *)wtk_malloc(mssl->channel*mssl->channel*sizeof(wtk_complex_t));
        mssl->tmp=(wtk_dcomplex_t *)wtk_malloc(mssl->channel*mssl->channel*2*sizeof(wtk_dcomplex_t));
    }
    wtk_maskssl_reset(mssl);

    return mssl;
}

void wtk_maskssl_delete(wtk_maskssl_t *mssl)
{
    wtk_maskaspec_delete(mssl->maskaspec);

    if(mssl->scov)
    {
        wtk_free(mssl->kmask_s);
        wtk_free(mssl->kmask_n);
        wtk_complex_delete_p2(mssl->scov, mssl->nbin);
        wtk_complex_delete_p2(mssl->ncov, mssl->nbin);
    }

    if(mssl->inv_cov)
    {
        wtk_free(mssl->inv_cov);
        wtk_free(mssl->tmp);
    }

    wtk_free(mssl->nextp);
    wtk_free(mssl->max_idx);
    wtk_free(mssl->nbest_extp);
    wtk_free(mssl);
}

void wtk_maskssl_reset(wtk_maskssl_t *mssl)
{
    int nangle=mssl->nangle;
    int n;

    if(mssl->scov)
    {
        memset(mssl->kmask_s, 0, sizeof(float)*mssl->nbin);
        memset(mssl->kmask_n, 0, sizeof(float)*mssl->nbin);
        wtk_complex_zero_p2(mssl->scov, mssl->nbin,mssl->channel*mssl->channel);
        wtk_complex_zero_p2(mssl->ncov, mssl->nbin,mssl->channel*mssl->channel);
    }

    mssl->nframe=0;
    mssl->oframe=mssl->cfg->online_frame;
    mssl->nbest=0;

    for(n=0; n<nangle; ++n)
    {
        mssl->nextp[n].nspecsum=0;
        mssl->nextp[n].is_peak=0;
    }
}

void wtk_maskssl_start(wtk_maskssl_t *maskssl)
{

}

int wtk_maskssl_theta_dist(int th1, int th2)
{
    int dist;

    dist=fabs(th1-th2);
    if(dist>180)
    {
        dist=360-dist;
    }
    return dist;
}

void wtk_maskssl_flush_spec_ssl(wtk_maskssl_t *ssl)
{
    int i,n,j,b;
    int ntheta=ssl->ntheta;
    int nphi=ssl->nphi;
    int nangle=ssl->nangle;
    wtk_maskssl_extp_t *nextp=ssl->nextp;
    wtk_maskssl_extp_t *nbest_extp=ssl->nbest_extp;
    int max_extp=ssl->cfg->max_extp;
    float max_spec;
    int *max_idx=ssl->max_idx;
    float min_thetasub=ssl->cfg->min_thetasub;
    float specsum_thresh=ssl->cfg->specsum_thresh;

    if(ssl->cfg->use_line)
    {
        for(n=0; n<ntheta; ++n)
        {
            // printf("%d %f / ", theta,nextp[n].nspecsum);
            if(n==0)
            {
                if(nextp[0].nspecsum>=nextp[1].nspecsum)
                {
                    nextp[n].is_peak=1;
                }
            }else if(n==ntheta-1)
            {
                if(nextp[ntheta-1].nspecsum>=nextp[ntheta-2].nspecsum)
                {
                    nextp[n].is_peak=1;
                }
            }else
            {
                if(nextp[n].nspecsum>=nextp[n-1].nspecsum && nextp[n].nspecsum>=nextp[n+1].nspecsum)
                {
                    nextp[n].is_peak=1;
                }
            }
        }
        // printf("\n");
    }else
    {
        if(nphi<=1)
        {
            for(n=0; n<ntheta; ++n)
            {
                if(n==0)
                {
                    if(nextp[0].nspecsum>=nextp[1].nspecsum && nextp[0].nspecsum>=nextp[ntheta-1].nspecsum)
                    {
                        nextp[n].is_peak=1;
                    }
                }else if(n==ntheta-1)
                {
                    if(nextp[ntheta-1].nspecsum>=nextp[ntheta-2].nspecsum && nextp[ntheta-1].nspecsum>=nextp[0].nspecsum)
                    {
                        nextp[n].is_peak=1;
                    }
                }else
                {
                    if(nextp[n].nspecsum>=nextp[n-1].nspecsum && nextp[n].nspecsum>=nextp[n+1].nspecsum)
                    {
                        nextp[n].is_peak=1;
                    }
                }
            }
        }else
        {
            n=0;
            for(i=0; i<ntheta; ++i)
            {
                for(j=0; j<nphi; ++j, ++n)
                {
                    if(j==0)
                    {
                        if(i==0)
                        {
                            if(nextp[n].nspecsum>=nextp[(ntheta-1)*nphi+j].nspecsum && nextp[n].nspecsum>=nextp[(i+1)*nphi+j].nspecsum
                                        && nextp[n].nspecsum>=nextp[(ntheta-1)*nphi+j+1].nspecsum && nextp[n].nspecsum>=nextp[(i+1)*nphi+j+1].nspecsum
                                                    && nextp[n].nspecsum>=nextp[i*nphi+j+1].nspecsum)
                            {
                                nextp[n].is_peak=1;
                            }
                        }else if(i==ntheta-1)
                        {
                            if(nextp[n].nspecsum>=nextp[(i-1)*nphi+j].nspecsum && nextp[n].nspecsum>=nextp[j].nspecsum
                                        && nextp[n].nspecsum>=nextp[(i-1)*nphi+j+1].nspecsum && nextp[n].nspecsum>=nextp[j+1].nspecsum
                                                    && nextp[n].nspecsum>=nextp[i*nphi+j+1].nspecsum)
                            {
                                nextp[n].is_peak=1;
                            }
                        }else
                        {
                            if(nextp[n].nspecsum>=nextp[(i-1)*nphi+j].nspecsum && nextp[n].nspecsum>=nextp[(i+1)*nphi+j].nspecsum
                                        && nextp[n].nspecsum>=nextp[(i-1)*nphi+j+1].nspecsum && nextp[n].nspecsum>=nextp[(i+1)*nphi+j+1].nspecsum
                                                    && nextp[n].nspecsum>=nextp[i*nphi+j+1].nspecsum)
                            {
                                nextp[n].is_peak=1;
                            }
                        }
                    }else if(j==nphi-1)
                    {
                        if(i==0)
                        {
                            if(nextp[n].nspecsum>=nextp[(ntheta-1)*nphi+j].nspecsum && nextp[n].nspecsum>=nextp[(i+1)*nphi+j].nspecsum
                                        && nextp[n].nspecsum>=nextp[(ntheta-1)*nphi+j-1].nspecsum && nextp[n].nspecsum>=nextp[(i+1)*nphi+j-1].nspecsum
                                                    && nextp[n].nspecsum>=nextp[i*nphi+j-1].nspecsum)
                            {
                                nextp[n].is_peak=1;
                            }
                        }else if(i==ntheta-1)
                        {
                            if(nextp[n].nspecsum>=nextp[(i-1)*nphi+j].nspecsum && nextp[n].nspecsum>=nextp[j].nspecsum
                                        && nextp[n].nspecsum>=nextp[(i-1)*nphi+j-1].nspecsum && nextp[n].nspecsum>=nextp[j-1].nspecsum
                                                    && nextp[n].nspecsum>=nextp[i*nphi+j-1].nspecsum)
                            {
                                nextp[n].is_peak=1;
                            }
                        }else
                        {
                            if(nextp[n].nspecsum>=nextp[(i-1)*nphi+j].nspecsum && nextp[n].nspecsum>=nextp[(i+1)*nphi+j].nspecsum
                                        && nextp[n].nspecsum>=nextp[(i-1)*nphi+j-1].nspecsum && nextp[n].nspecsum>=nextp[(i+1)*nphi+j-1].nspecsum
                                                    && nextp[n].nspecsum>=nextp[i*nphi+j-1].nspecsum)
                            {
                                nextp[n].is_peak=1;
                            }
                        }
                    }else
                    {
                        if(i==0)
                        {
                            if(nextp[n].nspecsum>=nextp[(ntheta-1)*nphi+j].nspecsum && nextp[n].nspecsum>=nextp[(i+1)*nphi+j].nspecsum
                                        && nextp[n].nspecsum>=nextp[(ntheta-1)*nphi+j-1].nspecsum && nextp[n].nspecsum>=nextp[(i+1)*nphi+j-1].nspecsum
                                                    && nextp[n].nspecsum>=nextp[(ntheta-1)*nphi+j+1].nspecsum && nextp[n].nspecsum>=nextp[(i+1)*nphi+j+1].nspecsum
                                                                    && nextp[n].nspecsum>=nextp[(ntheta-1)*nphi+j].nspecsum && nextp[n].nspecsum>=nextp[(i+1)*nphi+j].nspecsum
                                                                                    && nextp[n].nspecsum>=nextp[i*nphi+j-1].nspecsum && nextp[n].nspecsum>=nextp[i*nphi+j+1].nspecsum)
                            {
                                nextp[n].is_peak=1;
                            }
                        }else if(i==ntheta-1)
                        {
                            if(nextp[n].nspecsum>=nextp[(i-1)*nphi+j].nspecsum && nextp[n].nspecsum>=nextp[j].nspecsum
                                        && nextp[n].nspecsum>=nextp[(i-1)*nphi+j-1].nspecsum && nextp[n].nspecsum>=nextp[j-1].nspecsum
                                                    && nextp[n].nspecsum>=nextp[(i-1)*nphi+j+1].nspecsum && nextp[n].nspecsum>=nextp[j+1].nspecsum
                                                                    && nextp[n].nspecsum>=nextp[(i-1)*nphi+j].nspecsum && nextp[n].nspecsum>=nextp[j].nspecsum
                                                                                    && nextp[n].nspecsum>=nextp[i*nphi+j-1].nspecsum && nextp[n].nspecsum>=nextp[i*nphi+j+1].nspecsum)
                            {
                                nextp[n].is_peak=1;
                            }
                        }else
                        {
                            if(nextp[n].nspecsum>=nextp[(i-1)*nphi+j].nspecsum && nextp[n].nspecsum>=nextp[(i+1)*nphi+j].nspecsum
                                        && nextp[n].nspecsum>=nextp[(i-1)*nphi+j-1].nspecsum && nextp[n].nspecsum>=nextp[(i+1)*nphi+j-1].nspecsum
                                                    && nextp[n].nspecsum>=nextp[(i-1)*nphi+j+1].nspecsum && nextp[n].nspecsum>=nextp[(i+1)*nphi+j+1].nspecsum
                                                                    && nextp[n].nspecsum>=nextp[(i-1)*nphi+j].nspecsum && nextp[n].nspecsum>=nextp[(i+1)*nphi+j].nspecsum
                                                                                    && nextp[n].nspecsum>=nextp[i*nphi+j-1].nspecsum && nextp[n].nspecsum>=nextp[i*nphi+j+1].nspecsum)
                            {
                                nextp[n].is_peak=1;
                            }
                        }
                    }
                }   
            }
            // printf("\n");
        }
    }
    // for(n=0; n<nextp2_num; ++n)
    // {
    //     printf("%d  %f /",nextp2[n].theta, nextp2[n].nspecsum);
    // }
    // printf("\n");
    for(j=0; j<max_extp; ++j)
    {
        max_idx[j]=-1;
        max_spec=-2000000000;
        for(i=0; i<nangle; ++i)
        {
            if(nextp[i].is_peak==1)
            {
                b=1;
                for(n=0; n<ssl->nbest; ++n)
                {
                    if(i == max_idx[n] || wtk_maskssl_theta_dist(nextp[i].theta, nextp[max_idx[n]].theta)<min_thetasub)
                    {
                        b=0;
                    }
                }
                if(b==1 && nextp[i].nspecsum>max_spec)
                {
                    max_spec=nextp[i].nspecsum;
                    max_idx[j]=i;
                }
            }
        }
        if(max_idx[j]>=0 && max_spec>=specsum_thresh)
        {
            nbest_extp[j].theta=nextp[max_idx[j]].theta;
            nbest_extp[j].phi=nextp[max_idx[j]].phi;
            nbest_extp[j].nspecsum=max_spec;
            ++ssl->nbest;
        }else
        {
            break;
        }
    }
}

void wtk_maskssl_print(wtk_maskssl_t *ssl)
{
    int i;

    for(i=0;i<ssl->nbest;++i)
    {
        printf("ssl theta/phi [%d]=%d %d %f\n",i,ssl->nbest_extp[i].theta,ssl->nbest_extp[i].phi,ssl->nbest_extp[i].nspecsum);
    }
}

void wtk_maskssl_flush_spec(wtk_maskssl_t *mssl, wtk_complex_t **fft, float *mask, int flush)
{
    int k;
    int i,j,channel=mssl->channel;
    wtk_complex_t *fft1, *fft2;
    wtk_maskaspec_t *maskaspec=mssl->maskaspec;
    wtk_complex_t **scov=mssl->scov;
    wtk_complex_t **ncov=mssl->ncov;
    wtk_complex_t *inv_cov=mssl->inv_cov;
    wtk_dcomplex_t *tmp=mssl->tmp;
    wtk_complex_t *cov1,*cov2,*a,*b, *a2,*b2;
    float *kmask_s=mssl->kmask_s, kmask_sf, mask_sum;
    float *kmask_n=mssl->kmask_n, kmask_nf;
    float ftmp,ftmp2, prob;
    int n,nangle=mssl->nangle;
    wtk_maskssl_extp_t *nextp=mssl->nextp;
    int ret;
    int ns=mssl->cfg->specsum_ns;
    int ne=mssl->cfg->specsum_ne;

    if(fft)
    {
        if(scov)
        {
            for(k=ns;k<ne;++k)
            {
                cov1=scov[k];
                cov2=ncov[k];
                fft1=fft[k];
                kmask_sf=kmask_nf=1;
                for(i=0;i<channel;++i)
                {
                    kmask_sf*=mask[k];
                    kmask_nf*=1-mask[k];
                }
                kmask_s[k]+=kmask_sf;
                kmask_n[k]+=kmask_nf;

                for(i=0;i<channel;++i,++fft1)
                {
                    fft2=fft1;
                    for(j=i;j<channel;++j,++fft2)
                    {
                        a=cov1+i*channel+j;
                        a2=cov2+i*channel+j;
                        if(i!=j)
                        {
                            ftmp=fft1->a*fft2->a+fft1->b*fft2->b;
                            ftmp2=-fft1->a*fft2->b+fft1->b*fft2->a;
                            a->a+=ftmp*kmask_sf;
                            a->b+=ftmp2*kmask_sf;

                            a2->a+=ftmp*kmask_nf;
                            a2->b+=ftmp2*kmask_nf;

                            b=cov1+j*channel+i;
                            b->a=a->a;
                            b->b=-a->b;
                            b2=cov2+j*channel+i;
                            b2->a=a2->a;
                            b2->b=-a2->b;
                        }else
                        {
                            ftmp=fft1->a*fft2->a+fft1->b*fft2->b;
                            a->a+=ftmp*kmask_sf;
                            a->b=0;

                            a2->a+=ftmp*kmask_nf;
                            a2->b=0;
                        }
                    }
                }
            }
        }else
        {
            for(k=ns;k<ne;++k)
            {
                kmask_sf=1;
                for(i=0;i<channel;++i)
                {
                    kmask_sf*=mask[k];
                }
                for(n=0; n<nangle; ++n)
                {
                    nextp[n].nspecsum+=wtk_maskaspec_flush_spec_k(maskaspec, fft[k], NULL, NULL, NULL, kmask_sf, k, n);
                }
            }
        }
    }

    if(flush)
    {
        if(scov)
        {
            mask_sum=0;
            for(k=ns;k<ne;++k)
            {
                mask_sum+=kmask_s[k];
            }
            for(k=ns;k<ne;++k)
            {
                cov1=scov[k];
                cov2=ncov[k];
                for(i=0;i<channel;++i)
                {
                    for(j=0;j<channel;++j,++cov1,++cov2)
                    {
                        cov1->a/=kmask_s[k];
                        cov1->b/=kmask_s[k];

                        cov2->a/=kmask_n[k];
                        cov2->b/=kmask_n[k];
                    }
                }
                cov1=scov[k];
                cov2=ncov[k];

                if(inv_cov)
                {
                    ret=wtk_complex_invx4(cov2,tmp,channel,inv_cov,1);
                    if(ret!=0)
                    {
                        j=0;
                        for(i=0;i<channel;++i)
                        {
                            cov2[j].a+=0.03;
                            j+=channel+1;
                        }
                        wtk_complex_invx4(cov2,tmp,channel,inv_cov,1);
                    }
                }
                prob=kmask_s[k]/mask_sum;
                for(n=0; n<nangle; ++n)
                {
                    nextp[n].nspecsum+=wtk_maskaspec_flush_spec_k(maskaspec, NULL, cov1, cov2, inv_cov, prob, k, n);
                }
            }
        }

        wtk_maskssl_flush_spec_ssl(mssl);
    }
}

void wtk_maskssl_flush_spec2(wtk_maskssl_t *mssl, wtk_complex_t **fft, float *mask, int flush)
{
    int k;
    int i,j,channel=mssl->channel;
    wtk_complex_t *fft1, *fft2;
    wtk_complex_t ffttmp[64]={0};
    wtk_maskaspec_t *maskaspec=mssl->maskaspec;
    wtk_complex_t **scov=mssl->scov;
    wtk_complex_t **ncov=mssl->ncov;
    wtk_complex_t *inv_cov=mssl->inv_cov;
    wtk_dcomplex_t *tmp=mssl->tmp;
    wtk_complex_t *cov1,*cov2,*a,*b, *a2,*b2;
    float *kmask_s=mssl->kmask_s, kmask_sf, mask_sum;
    float *kmask_n=mssl->kmask_n, kmask_nf;
    float ftmp,ftmp2, prob;
    int n,nangle=mssl->nangle;
    wtk_maskssl_extp_t *nextp=mssl->nextp;
    int ret;
    int ns=mssl->cfg->specsum_ns;
    int ne=mssl->cfg->specsum_ne;

    if(fft)
    {
        if(scov)
        {
            for(k=ns;k<ne;++k)
            {
                cov1=scov[k];
                cov2=ncov[k];
                kmask_sf=kmask_nf=1;
                for(i=0;i<channel;++i)
                {
                    kmask_sf*=mask[k];
                    kmask_nf*=1-mask[k];
                }
                kmask_s[k]+=kmask_sf;
                kmask_n[k]+=kmask_nf;

                for(i=0;i<channel;++i)
                {
                    fft1=fft[i]+k;
                    for(j=i;j<channel;++j)
                    {
                        fft2=fft[j]+k;
                        a=cov1+i*channel+j;
                        a2=cov2+i*channel+j;
                        if(i!=j)
                        {
                            ftmp=fft1->a*fft2->a+fft1->b*fft2->b;
                            ftmp2=-fft1->a*fft2->b+fft1->b*fft2->a;
                            a->a+=ftmp*kmask_sf;
                            a->b+=ftmp2*kmask_sf;

                            a2->a+=ftmp*kmask_nf;
                            a2->b+=ftmp2*kmask_nf;

                            b=cov1+j*channel+i;
                            b->a=a->a;
                            b->b=-a->b;
                            b2=cov2+j*channel+i;
                            b2->a=a2->a;
                            b2->b=-a2->b;
                        }else
                        {
                            ftmp=fft1->a*fft2->a+fft1->b*fft2->b;
                            a->a+=ftmp*kmask_sf;
                            a->b=0;

                            a2->a+=ftmp*kmask_nf;
                            a2->b=0;
                        }
                    }
                }
            }
        }else
        {
            for(k=ns;k<ne;++k)
            {
                kmask_sf=1;
                for(i=0;i<channel;++i)
                {
                    kmask_sf*=mask[k];
                    ffttmp[i]=fft[i][k];
                }
                for(n=0; n<nangle; ++n)
                {
                    nextp[n].nspecsum+=wtk_maskaspec_flush_spec_k(maskaspec, ffttmp, NULL, NULL, NULL, kmask_sf, k, n);
                }
            }
        }
    }

    if(flush)
    {
        if(scov)
        {
            mask_sum=0;
            for(k=ns;k<ne;++k)
            {
                mask_sum+=kmask_s[k];
            }
            for(k=ns;k<ne;++k)
            {
                cov1=scov[k];
                cov2=ncov[k];
                for(i=0;i<channel;++i)
                {
                    for(j=0;j<channel;++j,++cov1,++cov2)
                    {
                        cov1->a/=kmask_s[k];
                        cov1->b/=kmask_s[k];

                        cov2->a/=kmask_n[k];
                        cov2->b/=kmask_n[k];
                    }
                }
                cov1=scov[k];
                cov2=ncov[k];

                if(inv_cov)
                {
                    ret=wtk_complex_invx4(cov2,tmp,channel,inv_cov,1);
                    if(ret!=0)
                    {
                        j=0;
                        for(i=0;i<channel;++i)
                        {
                            cov2[j].a+=0.03;
                            j+=channel+1;
                        }
                        wtk_complex_invx4(cov2,tmp,channel,inv_cov,1);
                    }
                }
                prob=kmask_s[k]/mask_sum;
                for(n=0; n<nangle; ++n)
                {
                    nextp[n].nspecsum+=wtk_maskaspec_flush_spec_k(maskaspec, NULL, cov1, cov2, inv_cov, prob, k, n);
                }
            }
        }

        wtk_maskssl_flush_spec_ssl(mssl);
    }
}

void wtk_maskssl_feed_fft(wtk_maskssl_t *mssl,wtk_complex_t **fft,float *mask,int is_sil)
{
    int n;
    int nangle=mssl->nangle;
    int ts,te;

    if(mssl->cfg->use_online)
    {
        if(is_sil)
        {
            if(fft)
            {
                ++mssl->nframe;
            }
            if(mssl->oframe*1.0/mssl->cfg->online_frame<=0.5)
            {
                wtk_maskssl_flush_spec(mssl, NULL, NULL, 1);

                if(mssl->notify)
                {
                    ts=((mssl->nframe-mssl->cfg->online_frame+mssl->oframe)*mssl->cfg->wins/2)/(mssl->cfg->rate/1000);
                    te=(mssl->nframe*mssl->cfg->wins/2)/(mssl->cfg->rate/1000);
                    mssl->notify(mssl->ths, mssl->nbest_extp, mssl->nbest, ts,te);
                }
            }

            mssl->oframe=mssl->cfg->online_frame;
            mssl->nbest=0;

            if(mssl->scov)
            {
                memset(mssl->kmask_s, 0, sizeof(float)*mssl->nbin);
                memset(mssl->kmask_n, 0, sizeof(float)*mssl->nbin);

                wtk_complex_zero_p2(mssl->scov, mssl->nbin,mssl->channel*mssl->channel);
                wtk_complex_zero_p2(mssl->ncov, mssl->nbin,mssl->channel*mssl->channel);
            }

            for(n=0; n<nangle; ++n)
            {
                mssl->nextp[n].nspecsum=0;
                mssl->nextp[n].is_peak=0;
            }  
        }else
        {
            if(fft)
            {
                ++mssl->nframe;
                --mssl->oframe;
            }

            wtk_maskssl_flush_spec(mssl, fft, mask, 0); 
            if(mssl->oframe==0)
            {
                wtk_maskssl_flush_spec(mssl, NULL, NULL, 1);

                if(mssl->notify)
                {
                    ts=((mssl->nframe-mssl->cfg->online_frame)*mssl->cfg->wins/2)/(mssl->cfg->rate/1000);
                    te=(mssl->nframe*mssl->cfg->wins/2)/(mssl->cfg->rate/1000);
                    mssl->notify(mssl->ths, mssl->nbest_extp, mssl->nbest, ts,te);
                }

                mssl->oframe=mssl->cfg->online_frame;
                mssl->nbest=0;

                if(mssl->scov)
                {
                    memset(mssl->kmask_s, 0, sizeof(float)*mssl->nbin);
                    memset(mssl->kmask_n, 0, sizeof(float)*mssl->nbin);

                    wtk_complex_zero_p2(mssl->scov, mssl->nbin,mssl->channel*mssl->channel);
                    wtk_complex_zero_p2(mssl->ncov, mssl->nbin,mssl->channel*mssl->channel);
                }

                for(n=0; n<nangle; ++n)
                {
                    mssl->nextp[n].nspecsum=0;
                    mssl->nextp[n].is_peak=0;
                }
            }
        }
    }else
    {
        if(fft)
        {
            ++mssl->nframe;
        }
        wtk_maskssl_flush_spec(mssl, fft, mask, is_sil); 
    }
}

void wtk_maskssl_feed_fft2(wtk_maskssl_t *mssl,wtk_complex_t **fft,float *mask,int is_sil)
{
    int n;
    int nangle=mssl->nangle;
    int ts,te;

    if(mssl->cfg->use_online)
    {
        if(is_sil)
        {
            if(fft)
            {
                ++mssl->nframe;
            }
            if(mssl->oframe*1.0/mssl->cfg->online_frame<=0.5)
            {
                wtk_maskssl_flush_spec2(mssl, NULL, NULL, 1);

                if(mssl->notify)
                {
                    ts=((mssl->nframe-mssl->cfg->online_frame+mssl->oframe)*mssl->cfg->wins/2)/(mssl->cfg->rate/1000);
                    te=(mssl->nframe*mssl->cfg->wins/2)/(mssl->cfg->rate/1000);
                    mssl->notify(mssl->ths, mssl->nbest_extp, mssl->nbest, ts,te);
                }
            }

            mssl->oframe=mssl->cfg->online_frame;
            mssl->nbest=0;

            if(mssl->scov)
            {
                memset(mssl->kmask_s, 0, sizeof(float)*mssl->nbin);
                memset(mssl->kmask_n, 0, sizeof(float)*mssl->nbin);

                wtk_complex_zero_p2(mssl->scov, mssl->nbin,mssl->channel*mssl->channel);
                wtk_complex_zero_p2(mssl->ncov, mssl->nbin,mssl->channel*mssl->channel);
            }

            for(n=0; n<nangle; ++n)
            {
                mssl->nextp[n].nspecsum=0;
                mssl->nextp[n].is_peak=0;
            }  
        }else
        {
            if(fft)
            {
                ++mssl->nframe;
                --mssl->oframe;
            }

            wtk_maskssl_flush_spec2(mssl, fft, mask, 0); 
            if(mssl->oframe==0)
            {
                wtk_maskssl_flush_spec2(mssl, NULL, NULL, 1);

                if(mssl->notify)
                {
                    ts=((mssl->nframe-mssl->cfg->online_frame)*mssl->cfg->wins/2)/(mssl->cfg->rate/1000);
                    te=(mssl->nframe*mssl->cfg->wins/2)/(mssl->cfg->rate/1000);
                    mssl->notify(mssl->ths, mssl->nbest_extp, mssl->nbest, ts,te);
                }

                mssl->oframe=mssl->cfg->online_frame;
                mssl->nbest=0;

                if(mssl->scov)
                {
                    memset(mssl->kmask_s, 0, sizeof(float)*mssl->nbin);
                    memset(mssl->kmask_n, 0, sizeof(float)*mssl->nbin);

                    wtk_complex_zero_p2(mssl->scov, mssl->nbin,mssl->channel*mssl->channel);
                    wtk_complex_zero_p2(mssl->ncov, mssl->nbin,mssl->channel*mssl->channel);
                }

                for(n=0; n<nangle; ++n)
                {
                    mssl->nextp[n].nspecsum=0;
                    mssl->nextp[n].is_peak=0;
                }
            }
        }
    }else
    {
        if(fft)
        {
            ++mssl->nframe;
        }
        wtk_maskssl_flush_spec2(mssl, fft, mask, is_sil); 
    }
}

void wtk_maskssl_set_notify(wtk_maskssl_t *ssl, void *ths, wtk_maskssl_notify_f notify)
{
    ssl->notify=notify;
    ssl->ths=ths;
}