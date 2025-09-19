#include "wtk_maskssl2.h" 

wtk_maskssl2_msg_t* wtk_maskssl2_msg_new(wtk_maskssl2_t *maskssl2)
{
	wtk_maskssl2_msg_t *msg;

	msg=(wtk_maskssl2_msg_t*)wtk_malloc(sizeof(wtk_maskssl2_msg_t));
	msg->fft=wtk_complex_new_p2(maskssl2->nbin,maskssl2->channel);
    msg->mask=wtk_malloc(maskssl2->nbin*sizeof(float));
    msg->nspecsum=wtk_malloc(maskssl2->nangle*sizeof(float));
    msg->spec=wtk_float_new_p2(maskssl2->nbin, maskssl2->nangle);

	return msg;
}

void wtk_maskssl2_msg_delete(wtk_maskssl2_t *maskssl2,wtk_maskssl2_msg_t *msg)
{
	wtk_complex_delete_p2(msg->fft,maskssl2->nbin);
    wtk_free(msg->mask);
    wtk_free(msg->nspecsum);
    wtk_float_delete_p2(msg->spec,maskssl2->nbin);
	wtk_free(msg);
}

wtk_maskssl2_msg_t* wtk_maskssl2_pop_msg(wtk_maskssl2_t *maskssl2)
{
	return  (wtk_maskssl2_msg_t*)wtk_hoard_pop(&(maskssl2->msg_hoard));
}

void wtk_maskssl2_push_msg(wtk_maskssl2_t *maskssl2,wtk_maskssl2_msg_t *msg)
{
	wtk_hoard_push(&(maskssl2->msg_hoard),msg);
}

wtk_maskssl2_t *wtk_maskssl2_new(wtk_maskssl2_cfg_t *cfg)
{
    wtk_maskssl2_t *mssl;
    float theta,phi;
    int tstep=cfg->theta_step, pstep=cfg->phi_step;
    int n;

    mssl=(wtk_maskssl2_t *)wtk_malloc(sizeof(wtk_maskssl2_t));
    mssl->notify=NULL;
    mssl->ths=NULL;
    mssl->cfg=cfg;
    mssl->channel=cfg->maskaspec.channel;
    mssl->nbin=cfg->wins/2+1;

	wtk_hoard_init2(&(mssl->msg_hoard),offsetof(wtk_maskssl2_msg_t,hoard_n),10,
			(wtk_new_handler_t)wtk_maskssl2_msg_new,
			(wtk_delete_handler2_t)wtk_maskssl2_msg_delete,
			mssl);

    mssl->ntheta=0;
    mssl->nphi=0;
    mssl->nangle=0;
    if(cfg->use_simple_phi){
        for(theta=cfg->min_theta; theta<=cfg->max_theta; theta+=tstep)
        {
            ++mssl->ntheta;
        }
        mssl->nangle=mssl->ntheta*2;
    }else{
        for(theta=cfg->min_theta; theta<=cfg->max_theta; theta+=tstep)
        {
            ++mssl->ntheta;
            for(phi=cfg->min_phi; phi<=cfg->max_phi; phi+=pstep)
            {
                ++mssl->nangle;
            }
        }
        mssl->nphi=mssl->nangle/mssl->ntheta;
    }

    mssl->nextp=(wtk_maskssl2_extp_t *)wtk_malloc(sizeof(wtk_maskssl2_extp_t)*mssl->nangle);
    mssl->nbest_extp=(wtk_maskssl2_extp_t *)wtk_malloc(sizeof(wtk_maskssl2_extp_t)*mssl->cfg->max_extp);
    mssl->max_idx=(int *)wtk_malloc(sizeof(int)*mssl->cfg->max_extp);

    mssl->maskaspec=wtk_maskaspec_new(&(cfg->maskaspec), mssl->nbin, mssl->nangle);
    wtk_maskaspec_reset(mssl->maskaspec);

    mssl->maskaspec->start_ang_num=mssl->nangle;
    if(cfg->use_simple_phi){
        for(theta=cfg->min_theta,n=0; theta<=cfg->max_theta; theta+=tstep,++n){
            mssl->nextp[n].theta=theta;
            mssl->nextp[n].phi=0;
            wtk_maskaspec_start(mssl->maskaspec, theta, cfg->max_phi, n);
        }
        for(theta=cfg->min_theta; theta<=cfg->max_theta; theta+=tstep,++n){
            mssl->nextp[n].theta=theta;
            mssl->nextp[n].phi=0;
            wtk_maskaspec_start2(mssl->maskaspec, theta, cfg->min_phi, n);
        }
    }else{
        for(theta=cfg->min_theta,n=0; theta<=cfg->max_theta; theta+=tstep)
        {
            for(phi=cfg->min_phi; phi<=cfg->max_phi; phi+=pstep,++n)
            {
                mssl->nextp[n].theta=theta;
                mssl->nextp[n].phi=phi;
                wtk_maskaspec_start(mssl->maskaspec, theta, phi, n);
            }
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

    mssl->spec=NULL;
    if(cfg->use_spec_k2){
        mssl->spec=(float *)wtk_malloc(sizeof(float)*mssl->nangle);
    }

    wtk_maskssl2_reset(mssl);

    return mssl;
}

void wtk_maskssl2_delete(wtk_maskssl2_t *mssl)
{
    wtk_hoard_clean(&(mssl->msg_hoard));
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
    if(mssl->spec)
    {
        wtk_free(mssl->spec);
    }
    wtk_free(mssl);
}

void wtk_maskssl2_reset(wtk_maskssl2_t *mssl)
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
    mssl->nbest=0;

    for(n=0; n<nangle; ++n)
    {
        mssl->nextp[n].nspecsum=0;
        mssl->nextp[n].is_peak=0;
    }
    if(mssl->spec)
    {
        memset(mssl->spec, 0, sizeof(float)*nangle);
    }

    wtk_queue_init(&(mssl->msg_q));
    mssl->theta=-1;
    mssl->theta2=-1;
    mssl->change_count=0;
    mssl->delay_cnt=0;
    mssl->sil_delay=0;
}

void wtk_maskssl2_start(wtk_maskssl2_t *maskssl2)
{

}

int wtk_maskssl2_theta_dist(int th1, int th2)
{
    int dist;

    dist=fabs(th1-th2);
    if(dist>180)
    {
        dist=360-dist;
    }
    return dist;
}

void wtk_maskssl2_flush_spec_ssl(wtk_maskssl2_t *ssl)
{
    int i,n,j,b;
    int ntheta=ssl->ntheta;
    int nphi=ssl->nphi;
    int nangle=ssl->nangle;
    wtk_maskssl2_extp_t *nextp=ssl->nextp;
    wtk_maskssl2_extp_t *nbest_extp=ssl->nbest_extp;
    int max_extp=ssl->cfg->max_extp;
    float max_spec;
    int *max_idx=ssl->max_idx;
    float min_thetasub=ssl->cfg->min_thetasub;
    float specsum_thresh=ssl->cfg->specsum_thresh;

    if(ssl->cfg->use_line)
    {
        for(n=0; n<nangle; ++n)
        {
            // printf("%d %f / ", theta,nextp[n].nspecsum);
            if(n==0)
            {
                if(nextp[0].nspecsum>=nextp[1].nspecsum)
                {
                    nextp[n].is_peak=1;
                }
            }else if(n==nangle-1)
            {
                if(nextp[nangle-1].nspecsum>=nextp[nangle-2].nspecsum)
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
                    if(i == max_idx[n] || wtk_maskssl2_theta_dist(nextp[i].theta, nextp[max_idx[n]].theta)<min_thetasub)
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

void wtk_maskssl2_flush_spec_ssl2(wtk_maskssl2_t *ssl)
{
    int i,n,j,b;
    int ntheta=ssl->ntheta;
    int nangle=ssl->nangle;
    wtk_maskssl2_extp_t *nextp=ssl->nextp;
    wtk_maskssl2_extp_t *nbest_extp=ssl->nbest_extp;
    int max_extp=ssl->cfg->max_extp;
    float max_spec;
    int *max_idx=ssl->max_idx;
    float min_thetasub=ssl->cfg->min_thetasub;
    float specsum_thresh=ssl->cfg->specsum_thresh;
    float nbese_tmp=0;

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
        // printf("%d %f\n", n, nextp[n].nspecsum);
    }
    for(n=ntheta; n<nangle; ++n){
        if(n==ntheta)
        {
            if(nextp[ntheta].nspecsum>=nextp[ntheta+1].nspecsum)
            {
                nextp[n].is_peak=1;
            }
        }else if(n==nangle-1)
        {
            if(nextp[nangle-1].nspecsum>=nextp[nangle-2].nspecsum)
            {
                nextp[n].is_peak=1;
            }
        }else
        {
            if(nextp[n].nspecsum>=nextp[n-1].nspecsum && nextp[n].nspecsum>=nextp[n+1].nspecsum)
            {
                // printf("===\n");
                nextp[n].is_peak=1;
            }
        }
        // printf("%d %d %d %f %d\n", n, ntheta, nangle, nextp[n].nspecsum, nextp[n].is_peak);
    }
    // printf("\n");

    // for(n=0; n<nextp2_num; ++n)
    // {
    //     printf("%d  %f /",nextp2[n].theta, nextp2[n].nspecsum);
    // }
    // printf("\n");
    for(j=0; j<max_extp; ++j)
    {
        max_idx[j]=-1;
        max_spec=-2000000000;
        for(i=0; i<ntheta; ++i)
        {
            if(nextp[i].is_peak==1)
            {
                b=1;
                for(n=0; n<ssl->nbest; ++n)
                {
                    if(i == max_idx[n] || wtk_maskssl2_theta_dist(nextp[i].theta, nextp[max_idx[n]].theta)<min_thetasub)
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
            nbest_extp[j].nspecsum=max_spec;
            ++ssl->nbest;
        }else
        {
            break;
        }
    }
    if(ssl->nbest>0){
        nbese_tmp = ssl->nbest;
        ssl->nbest = 0;
        for(j=0; j<max_extp; ++j)
        {
            max_idx[j]=-1;
            max_spec=-2000000000;
            for(i=ntheta; i<nangle; ++i)
            {
                if(nextp[i].is_peak==1)
                {
                    b=1;
                    for(n=0; n<ssl->nbest; ++n)
                    {
                        if(i == max_idx[n] || wtk_maskssl2_theta_dist(nextp[i].theta, nextp[max_idx[n]].theta)<min_thetasub)
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
                nbest_extp[j].phi=nextp[max_idx[j]].theta;
                nbest_extp[j].nspecsum=max_spec;
                ++ssl->nbest;
            }else
            {
                break;
            }
        }
        ssl->nbest = nbese_tmp;
    }
}

void wtk_maskssl2_print(wtk_maskssl2_t *ssl)
{
    int i;

    for(i=0;i<ssl->nbest;++i)
    {
        printf("ssl theta/phi [%d]=%d %d %f\n",i,ssl->nbest_extp[i].theta,ssl->nbest_extp[i].phi,ssl->nbest_extp[i].nspecsum);
    }
}

void wtk_maskssl2_flush_spec(wtk_maskssl2_t *mssl, wtk_maskssl2_msg_t *msg, int flush)
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
    wtk_maskssl2_extp_t *nextp=mssl->nextp;
    int ret;
    int ns=mssl->cfg->specsum_ns;
    int ne=mssl->cfg->specsum_ne;
    int online_frame=mssl->cfg->online_frame;
    wtk_complex_t **fft;
    float *mask;
    float spec;
    int freq_skip=mssl->cfg->freq_skip;
    float gcc_scale=1.0/(online_frame*(ne-ns+1)/freq_skip);

    if(msg)
    {
        fft=msg->fft;
        mask=msg->mask;
        if(scov)
        {
            for(k=ns;k<ne;k+=freq_skip)
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
            if(msg->nspecsum[0]==-1){
                memset(msg->nspecsum, 0, sizeof(float)*nangle);
                for(k=ns;k<ne;k+=freq_skip)
                {
                    kmask_sf=1;
                    for(i=0;i<channel;++i)
                    {
                        kmask_sf*=mask[k];
                    }
                    if(mssl->cfg->use_each_comp){
                        for(n=0; n<nangle; ++n){
                            spec=msg->spec[k][n];
                            nextp[n].nspecsum+=spec;
                            msg->nspecsum[n]+=spec;
                        }
                    }else if(mssl->spec){
                        wtk_maskaspec_flush_spec_k2(maskaspec, fft[k], NULL, NULL, NULL, kmask_sf, k, nangle, mssl->spec);
                        for(n=0; n<nangle; ++n){
                            nextp[n].nspecsum+=mssl->spec[n];
                            msg->nspecsum[n]+=mssl->spec[n];
                        }
                    }else{
                        for(n=0; n<nangle; ++n)
                        {
                            spec=wtk_maskaspec_flush_spec_k(maskaspec, fft[k], NULL, NULL, NULL, kmask_sf, k, n);
                            nextp[n].nspecsum+=spec;
                            msg->nspecsum[n]+=spec;
                        }
                    }
                }
            }else{
                for(n=0; n<nangle; ++n)
                {
                    nextp[n].nspecsum+=msg->nspecsum[n];
                }
            }
        }
    }

    if(flush)
    {
        if(scov)
        {
            mask_sum=0;
            for(k=ns;k<ne;k+=freq_skip)
            {
                mask_sum+=kmask_s[k];
            }
            for(k=ns;k<ne;k+=freq_skip)
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
                if(mssl->spec){
                    wtk_maskaspec_flush_spec_k2(maskaspec, NULL, cov1, cov2, inv_cov, prob, k, nangle, mssl->spec);
                    for(n=0; n<nangle; ++n){
                        nextp[n].nspecsum+=mssl->spec[n];
                    }
                }else{
                    for(n=0; n<nangle; ++n)
                    {
                        nextp[n].nspecsum+=wtk_maskaspec_flush_spec_k(maskaspec, NULL, cov1, cov2, inv_cov, prob, k, n);
                    }
                }
            }
        }else{
            for(n=0; n<nangle; ++n)
            {
                nextp[n].nspecsum*=gcc_scale;
            }
        }
        if(mssl->cfg->use_simple_phi){
            wtk_maskssl2_flush_spec_ssl2(mssl);
        }else{
            wtk_maskssl2_flush_spec_ssl(mssl);
        }
    }
}

void wtk_maskssl2_notify_data(wtk_maskssl2_t *mssl)
{
    int online_frame_step=mssl->cfg->online_frame_step;
    int ts,te;
	int theta = 0;

    if(mssl->cfg->use_smooth){
        if(mssl->nbest == 1){
            if(mssl->theta == -1){
                mssl->delay_cnt = mssl->cfg->delay_cnt;
            }
            if(mssl->delay_cnt > 0){
                mssl->theta = -2;
                --mssl->delay_cnt;
                mssl->nbest=0;
            }else{
                if(mssl->theta == -2){
                    if(mssl->nbest_extp[0].nspecsum > mssl->cfg->specsum_thresh2){
                        mssl->theta = mssl->nbest_extp[0].theta;
                    }else{
                        mssl->nbest=0;
                    }
                }else if(abs(mssl->theta - mssl->nbest_extp[0].theta)<mssl->cfg->smooth_theta){
                    mssl->theta2 = mssl->nbest_extp[0].theta;
                    mssl->change_count=0;
                }else if(mssl->nbest_extp[0].nspecsum > mssl->cfg->specsum_thresh2){
                    ++mssl->change_count;
                }
                if(mssl->change_count>=mssl->cfg->smooth_cnt && abs(mssl->theta2 - mssl->nbest_extp[0].theta)<mssl->cfg->smooth_theta){
                    mssl->theta = mssl->nbest_extp[0].theta;
                    mssl->change_count=0;
                }else if(mssl->change_count>0){
                    mssl->theta2 = mssl->nbest_extp[0].theta;
                }
                theta = floor(mssl->theta);
                mssl->nbest_extp[0].theta = theta;
            }
        }else if(mssl->nbest == 0){
            mssl->theta = -1;
        }
    }
    if(mssl->notify)
    {
        ts=((mssl->nframe-online_frame_step)*mssl->cfg->wins/2)/(mssl->cfg->rate/1000);
        te=(mssl->nframe*mssl->cfg->wins/2)/(mssl->cfg->rate/1000);
        mssl->notify(mssl->ths, mssl->nbest_extp, mssl->nbest, ts,te);
    }
}

void wtk_maskssl2_feed_fft(wtk_maskssl2_t *mssl,wtk_complex_t **fft,float *mask,int is_sil)
{
    wtk_maskssl2_msg_t *msg;
    wtk_queue_t *msg_q=&(mssl->msg_q);
    wtk_queue_node_t *qn;
    int nbin=mssl->nbin;
    int channel=mssl->channel;
    int online_frame=mssl->cfg->online_frame;
    int online_frame_step=mssl->cfg->online_frame_step;
    int i;
    int nangle=mssl->nangle;


    if(fft)
    {
        ++mssl->nframe;

        msg=wtk_maskssl2_pop_msg(mssl);
        wtk_complex_cpy_p2(msg->fft, fft, nbin, channel);
        memcpy(msg->mask, mask, sizeof(float)*nbin);
        msg->nspecsum[0]=-1;
        wtk_queue_push(msg_q, &(msg->q_n));
    }
    if(is_sil)
    {
        if(msg_q->length==online_frame)
        {
            mssl->nbest=0;
            for(i=0; i<nangle; ++i)
            {
                mssl->nextp[i].nspecsum=0;
                mssl->nextp[i].is_peak=0;
            }
            wtk_maskssl2_notify_data(mssl);

            for(i=0;i<online_frame_step; ++i)
            {
                qn=wtk_queue_pop(msg_q);
                msg=data_offset2(qn,wtk_maskssl2_msg_t,q_n);
                wtk_maskssl2_push_msg(mssl,msg);
            }
        }
    }else
    {
        if(msg_q->length==online_frame)
        {
            for(qn=msg_q->pop; qn; qn=qn->next)
            {
                msg=data_offset2(qn,wtk_maskssl2_msg_t,q_n);
                wtk_maskssl2_flush_spec(mssl, msg, 0);
            }
            wtk_maskssl2_flush_spec(mssl, NULL, 1); 
            wtk_maskssl2_notify_data(mssl);

            mssl->nbest=0;
            for(i=0; i<nangle; ++i)
            {
                mssl->nextp[i].nspecsum=0;
                mssl->nextp[i].is_peak=0;
            }

            if(mssl->scov)
            {
                memset(mssl->kmask_s, 0, sizeof(float)*mssl->nbin);
                memset(mssl->kmask_n, 0, sizeof(float)*mssl->nbin);

                wtk_complex_zero_p2(mssl->scov, mssl->nbin,mssl->channel*mssl->channel);
                wtk_complex_zero_p2(mssl->ncov, mssl->nbin,mssl->channel*mssl->channel);
            }

            for(i=0;i<online_frame_step; ++i)
            {
                qn=wtk_queue_pop(msg_q);
                msg=data_offset2(qn,wtk_maskssl2_msg_t,q_n);
                wtk_maskssl2_push_msg(mssl,msg);
            }
        }
    }
}


void wtk_maskssl2_feed_fft2(wtk_maskssl2_t *mssl,wtk_complex_t **fft,float *mask,int is_sil)
{
    wtk_maskssl2_msg_t *msg;
    wtk_queue_t *msg_q=&(mssl->msg_q);
    wtk_queue_node_t *qn;
    int nbin=mssl->nbin;
    int channel=mssl->channel;
    int online_frame=mssl->cfg->online_frame;
    int online_frame_step=mssl->cfg->online_frame_step;
    int i,k;
    int nangle=mssl->nangle;
    int no_sil_cnt=0;
    int freq_skip=mssl->cfg->freq_skip;
    if(fft)
    {
        ++mssl->nframe;

        msg=wtk_maskssl2_pop_msg(mssl);
        for(i=0; i<channel; ++i)
        {
            for(k=0; k<nbin; ++k)
            {
                msg->fft[k][i]=fft[i][k];
            }
        }
        memcpy(msg->mask, mask, sizeof(float)*nbin);
        msg->nspecsum[0]=-1;
        if(!mssl->scov && mssl->cfg->use_each_comp){
            if(is_sil){
                int ns=mssl->cfg->specsum_ns;
                int ne=mssl->cfg->specsum_ne;
                for(k=ns;k<ne;k+=freq_skip)
                {
                    memset(msg->spec[k], 0, sizeof(float)*nangle);
                }
            }else{
                wtk_maskaspec_t *maskaspec=mssl->maskaspec;
                float kmask_sf;
                int ns=mssl->cfg->specsum_ns;
                int ne=mssl->cfg->specsum_ne;
                int n;
                for(k=ns;k<ne;k+=freq_skip)
                {
                    kmask_sf=1;
                    for(i=0;i<channel;++i)
                    {
                        kmask_sf*=mask[k];
                    }
                    if(mssl->spec){
                        wtk_maskaspec_flush_spec_k2(maskaspec, msg->fft[k], NULL, NULL, NULL, kmask_sf, k, nangle, mssl->spec);
                        for(n=0; n<nangle; ++n){
                            msg->spec[k][n]=mssl->spec[n];
                        }
                    }else{
                        for(n=0; n<nangle; ++n)
                        {
                            msg->spec[k][n]=wtk_maskaspec_flush_spec_k(maskaspec, msg->fft[k], NULL, NULL, NULL, kmask_sf, k, n);
                        }
                    }
                }
            }
        }

        wtk_queue_push(msg_q, &(msg->q_n));
    }
    if(is_sil)
    {
        if(msg_q->length==online_frame)
        {
            mssl->nbest=0;
            for(i=0; i<nangle; ++i)
            {
                mssl->nextp[i].nspecsum=0;
                mssl->nextp[i].is_peak=0;
            }
            wtk_maskssl2_notify_data(mssl);

            for(i=0;i<online_frame_step; ++i)
            {
                qn=wtk_queue_pop(msg_q);
                msg=data_offset2(qn,wtk_maskssl2_msg_t,q_n);
                wtk_maskssl2_push_msg(mssl,msg);
            }
        }
        mssl->sil_delay = online_frame - online_frame_step;
    }else
    {
        --mssl->sil_delay;
        if(msg_q->length==online_frame)
        {
            no_sil_cnt = 0;
            for(qn=msg_q->pop; qn; qn=qn->next)
            {
                msg=data_offset2(qn,wtk_maskssl2_msg_t,q_n);
                ++no_sil_cnt;
                if(no_sil_cnt >= mssl->sil_delay || mssl->cfg->use_sil_delay==0){
                    wtk_maskssl2_flush_spec(mssl, msg, 0);
                }
            }
            wtk_maskssl2_flush_spec(mssl, NULL, 1); 
            wtk_maskssl2_notify_data(mssl);

            mssl->nbest=0;
            for(i=0; i<nangle; ++i)
            {
                mssl->nextp[i].nspecsum=0;
                mssl->nextp[i].is_peak=0;
            }

            if(mssl->scov)
            {
                memset(mssl->kmask_s, 0, sizeof(float)*mssl->nbin);
                memset(mssl->kmask_n, 0, sizeof(float)*mssl->nbin);

                wtk_complex_zero_p2(mssl->scov, mssl->nbin,mssl->channel*mssl->channel);
                wtk_complex_zero_p2(mssl->ncov, mssl->nbin,mssl->channel*mssl->channel);
            }

            for(i=0;i<online_frame_step; ++i)
            {
                qn=wtk_queue_pop(msg_q);
                msg=data_offset2(qn,wtk_maskssl2_msg_t,q_n);
                wtk_maskssl2_push_msg(mssl,msg);
            }
        }
    }
}

void wtk_maskssl2_set_notify(wtk_maskssl2_t *ssl, void *ths, wtk_maskssl2_notify_f notify)
{
    ssl->notify=notify;
    ssl->ths=ths;
}
