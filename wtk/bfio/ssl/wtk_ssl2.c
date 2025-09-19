#include "wtk_ssl2.h" 

int wtk_ssl2_theta_dist(int th1, int th2);


typedef struct
{
	wtk_queue_node_t hoard_n;
	wtk_queue_node_t q_n;
	wtk_complex_t **fft; // |nbin*channel|
}wtk_ssl2_msg_t;


wtk_ssl2_msg_t* wtk_ssl2_msg_new(wtk_ssl2_t *ssl2)
{
	wtk_ssl2_msg_t *msg;

	msg=(wtk_ssl2_msg_t*)wtk_malloc(sizeof(wtk_ssl2_msg_t));
	msg->fft=wtk_complex_new_p2(ssl2->nbin,ssl2->channel);
	return msg;
}

void wtk_ssl2_msg_delete(wtk_ssl2_t *ssl2,wtk_ssl2_msg_t *msg)
{
	wtk_complex_delete_p2(msg->fft,ssl2->nbin);
	wtk_free(msg);
}

wtk_ssl2_msg_t* wtk_ssl2_pop_msg(wtk_ssl2_t *ssl2)
{
	return  (wtk_ssl2_msg_t*)wtk_hoard_pop(&(ssl2->msg_hoard));
}

void wtk_ssl2_push_msg(wtk_ssl2_t *ssl2,wtk_ssl2_msg_t *msg)
{
	wtk_hoard_push(&(ssl2->msg_hoard),msg);
}


wtk_ssl2_t* wtk_ssl2_new(wtk_ssl2_cfg_t *cfg)
{
    wtk_ssl2_t *ssl2;
    float theta, phi;
    int tstep=cfg->theta_step;
    int pstep=cfg->phi_step;
    int n;

    ssl2=(wtk_ssl2_t *)wtk_malloc(sizeof(wtk_ssl2_t));
    ssl2->cfg=cfg;
    ssl2->ths=NULL;
    ssl2->notify=NULL;

    ssl2->nbin=cfg->wins/2+1;
    ssl2->channel=cfg->aspec.channel;
	wtk_hoard_init2(&(ssl2->msg_hoard),offsetof(wtk_ssl2_msg_t,hoard_n),10,
			(wtk_new_handler_t)wtk_ssl2_msg_new,
			(wtk_delete_handler2_t)wtk_ssl2_msg_delete,
			ssl2);

    ssl2->ntheta=0;
    ssl2->nangle=0;
    for(theta=0; theta<=cfg->max_theta; theta+=tstep)
    {
        ++ssl2->ntheta;
        for(phi=0; phi<=cfg->max_phi; phi+=pstep)
        {
            ++ssl2->nangle;
        }
    }
    ssl2->nphi=ssl2->nangle/ssl2->ntheta;
    ssl2->spec=(float *)wtk_malloc(sizeof(float)*ssl2->nangle);
    ssl2->nextp=(wtk_ssl2_extp_t *)wtk_malloc(sizeof(wtk_ssl2_extp_t)*ssl2->nangle);
    ssl2->max_idx=(int *)wtk_malloc(sizeof(int)*ssl2->cfg->max_extp);
    
    ssl2->aspec=wtk_aspec_new(&(cfg->aspec), ssl2->nbin, ssl2->nangle);
    wtk_aspec_reset(ssl2->aspec);

    ssl2->aspec->start_ang_num=ssl2->nangle;
    for(theta=0,n=0; theta<=cfg->max_theta; theta+=tstep)
    {
        for(phi=0; phi<=cfg->max_phi; phi+=pstep,++n)
        {
            ssl2->nextp[n].theta=theta;
            ssl2->nextp[n].phi=phi;
            wtk_aspec_start(ssl2->aspec, theta, phi, n);
        }
    }

    wtk_queue_init(&(ssl2->msg_q));
    ssl2->cov=NULL;
    if(ssl2->aspec->need_cov)
    {
        ssl2->cov=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->aspec.channel*cfg->aspec.channel);
        if(cfg->lt<=0)
        {
            ssl2->wint=wtk_malloc(sizeof(float));
            ssl2->wint[0]=1;
        }else
        {
            ssl2->wint=wtk_math_create_hanning_window(2*cfg->lt+1);
        }

        if(cfg->lf<=0)
        {
            ssl2->winf=wtk_malloc(sizeof(float));
            ssl2->winf[0]=1;
        }else
        {
            ssl2->winf=wtk_math_create_hanning_window(2*cfg->lf+1);
        }
    }
    ssl2->inv_cov=NULL;
    if(ssl2->aspec->need_inv_cov)
    {
        ssl2->inv_cov=(wtk_complex_t *)wtk_malloc(cfg->aspec.channel*cfg->aspec.channel*sizeof(wtk_complex_t));
        ssl2->tmp=(wtk_dcomplex_t *)wtk_malloc(cfg->aspec.channel*cfg->aspec.channel*2*sizeof(wtk_dcomplex_t));
    }

    ssl2->nbest_extp=(wtk_ssl2_extp_t *)wtk_malloc(sizeof(wtk_ssl2_extp_t)*ssl2->cfg->max_extp);

    wtk_ssl2_reset(ssl2);

    return ssl2;
}

void wtk_ssl2_delete(wtk_ssl2_t *ssl2)
{
    wtk_hoard_clean(&(ssl2->msg_hoard));
    if(ssl2->cov)
    {
        wtk_free(ssl2->cov);
        wtk_free(ssl2->wint);
        wtk_free(ssl2->winf);
        if(ssl2->inv_cov)
        {
            wtk_free(ssl2->inv_cov);
            wtk_free(ssl2->tmp);
        }
    }

    wtk_aspec_delete(ssl2->aspec);

    wtk_free(ssl2->nextp);
    wtk_free(ssl2->max_idx);
    wtk_free(ssl2->nbest_extp);
    wtk_free(ssl2->spec);
    wtk_free(ssl2);
}

void wtk_ssl2_reset(wtk_ssl2_t *ssl2)
{
    int nangle=ssl2->nangle;
    int n;
    wtk_ssl2_msg_t *smsg;
    wtk_queue_node_t *qn;

    ssl2->nframe=0;
    ssl2->nbest=0;

    for(n=0; n<nangle; ++n)
    {
        ssl2->nextp[n].nspecsum=0;
        ssl2->nextp[n].is_peak=0;
        ssl2->spec[n]=0;
    }
    while(ssl2->msg_q.length>0)
    {
        qn=wtk_queue_pop(&(ssl2->msg_q));
        smsg=data_offset2(qn,wtk_ssl2_msg_t,q_n);
        wtk_ssl2_push_msg(ssl2, smsg);
    }
    ssl2->oframe=ssl2->cfg->online_frame;
}

void wtk_ssl2_set_notify(wtk_ssl2_t *ssl, void *ths, wtk_ssl2_notify_f notify)
{
    ssl->notify=notify;
    ssl->ths=ths;
}

int wtk_ssl2_theta_dist(int th1, int th2)
{
    int dist;

    dist=fabs(th1-th2);
    if(dist>180)
    {
        dist=360-dist;
    }
    return dist;
}


void wtk_ssl2_flush_spec_ssl2(wtk_ssl2_t *ssl)
{
    int i,n,j,b;
    int ntheta=ssl->ntheta;
    int nphi=ssl->nphi;
    int nangle=ssl->nangle;
    wtk_ssl2_extp_t *nextp=ssl->nextp;
    wtk_ssl2_extp_t *nbest_extp=ssl->nbest_extp;
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
                    if(i == max_idx[n] || wtk_ssl2_theta_dist(nextp[i].theta, nextp[max_idx[n]].theta)<min_thetasub)
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

void wtk_ssl2_print(wtk_ssl2_t *ssl2)
{
    int i;

    for(i=0;i<ssl2->nbest;++i)
    {
        printf("ssl2 theta/phi [%d]=%d %d %f\n",i,ssl2->nbest_extp[i].theta,ssl2->nbest_extp[i].phi,ssl2->nbest_extp[i].nspecsum);
    }
}

void wtk_ssl2_flush_aspec_lt(wtk_ssl2_t *ssl2)
{
    wtk_aspec_t *aspec=ssl2->aspec;
    wtk_queue_t *msg_q=&(ssl2->msg_q);
    int lf=ssl2->cfg->lf;
    int i,j,k,k2,n,tt,ff;
    wtk_queue_node_t *qn;
    wtk_ssl2_msg_t *smsg;
    int nbin=ssl2->nbin;
    int channel=ssl2->cfg->aspec.channel; 
    wtk_complex_t *cov=ssl2->cov;
    wtk_complex_t **fft,*fft1,*fft2,*fft3,*a,*b;
    int nangle=ssl2->nangle;
    float *wint=ssl2->wint;
    float *winf=ssl2->winf;
    float wint2,wintf,winsum;
    wtk_complex_t *inv_cov=ssl2->inv_cov;
    wtk_dcomplex_t *tmp=ssl2->tmp;
    float cov_travg;
    int ret;
    wtk_ssl2_extp_t *nextp=ssl2->nextp;
    int ns=ssl2->cfg->specsum_ns;
    int ne=ssl2->cfg->specsum_ne;
    float *spec=ssl2->spec;

    ++ssl2->nframe;
    --ssl2->oframe;
    for(k=ns;k<ne;++k)
    {
        memset(cov,0,sizeof(wtk_complex_t)*channel*channel);
        winsum=0;
        for(qn=msg_q->pop,tt=0;qn;qn=qn->next,++tt)
        {
            wint2=wint[tt];
            smsg=data_offset2(qn,wtk_ssl2_msg_t,q_n);
            fft=smsg->fft;
            for(k2=max(1,k-lf),ff=0;k2<min(nbin-1,k+lf+1);++k2,++ff)
            {
                wintf=wint2*winf[ff];
                winsum+=wintf;
                fft1=fft2=fft3=fft[k2];
                for(i=0;i<channel;++i,++fft2)
                {
                    fft3=fft1+i;
                    for(j=i;j<channel;++j,++fft3)
                    {
                        a=cov+i*channel+j;
                        if(i!=j)
                        {
                            a->a+=(fft2->a*fft3->a+fft2->b*fft3->b)*wintf;
                            a->b+=(-fft2->a*fft3->b+fft2->b*fft3->a)*wintf;
                        }else
                        {
                            a->a+=(fft2->a*fft3->a+fft2->b*fft3->b)*wintf;
                            a->b+=0;
                        }
                    }
                }
            }
        }
        winsum=1.0/winsum;
        for(i=0;i<channel;++i)
        {
            for(j=i;j<channel;++j)
            {
                a=cov+i*channel+j;
                a->a*=winsum;
                a->b*=winsum;

                if(i!=j)
                {
                    b=cov+j*channel+i;
                    b->a=a->a;
                    b->b=-a->b;
                }
            }
        }

        cov_travg=0;
        if(aspec->need_cov_travg)
        {
            for(i=0;i<channel;++i)
            {
                cov_travg+=cov[i*channel+i].a;
            }
            cov_travg/=channel;
        }
        if(aspec->need_inv_cov)
        {
            ret=wtk_complex_invx4(cov,tmp,channel,inv_cov,1);
             // //wtk_complex_print2(inv_cov,channel,channel);
            if(ret!=0)
            {
                j=0;
                for(i=0;i<channel;++i)
                {
                    cov[j].a+=0.01;
                    j+=channel+1;
                }
                wtk_complex_invx4(cov,tmp,channel,inv_cov,1);
            }
        }
        if(aspec->dswspec || aspec->gccspec){
            wtk_aspec_flush_spec_k2(aspec, NULL, 0, cov_travg, cov, inv_cov, k , nangle, spec);
            for(n=0; n<nangle; ++n)
            {
                nextp[n].nspecsum+=spec[n];
            }
        }else{
            for(n=0; n<nangle; ++n)
            {
                nextp[n].nspecsum+=wtk_aspec_flush_spec_k(aspec, NULL, 0, cov_travg, cov, inv_cov, k ,n);
            }
        }
    }
}

void wtk_ssl2_flush_aspec(wtk_ssl2_t *ssl2, wtk_ssl2_msg_t *msg)
{
    wtk_aspec_t *aspec=ssl2->aspec;
    int k,n,i;
    int nangle=ssl2->nangle;
    int channel=ssl2->channel;
    wtk_complex_t **fft=msg->fft, *fft2;
    float fftabs2;
    wtk_ssl2_extp_t *nextp=ssl2->nextp;
    int ns=ssl2->cfg->specsum_ns;
    int ne=ssl2->cfg->specsum_ne;

    ++ssl2->nframe;
    --ssl2->oframe;
    for(k=ns; k<ne; ++k)
    {
        fftabs2=0;
        fft2=fft[k];
        for(i=0; i<channel; ++i,++fft2)
        {
            fftabs2+=fft2->a*fft2->a+fft2->b*fft2->b;
        }
        for(n=0; n<nangle; ++n)
        {
            nextp[n].nspecsum+=wtk_aspec_flush_spec_k(aspec, fft, fftabs2, 0, NULL, NULL, k ,n);
        }
    }
}

void wtk_ssl2_feed_fft(wtk_ssl2_t *ssl2,wtk_complex_t **fft,int is_end)
{
    wtk_queue_t *msg_q=&(ssl2->msg_q);
    int lt=ssl2->cfg->lt;
    wtk_ssl2_msg_t *msg, *smsg;
    wtk_queue_node_t *qn;
    int nbin=ssl2->nbin;
    int channel=ssl2->channel;
    int i,k;
    int nangle=ssl2->nangle;
    int n;
    int ts,te;

    if(fft)
    {
        msg=wtk_ssl2_pop_msg(ssl2);
        for(k=0; k<nbin; ++k)
        {
            for(i=0; i<channel; ++i)
            {
                msg->fft[k][i]=fft[i][k];
            }
        }

        if(ssl2->cov)
        {
            wtk_queue_push(msg_q,&(msg->q_n));
            if(msg_q->length>=lt+1 && msg_q->length<2*lt+1)
            {
                wtk_ssl2_flush_aspec_lt(ssl2);
            }else if(msg_q->length==2*lt+1)
            {
                wtk_ssl2_flush_aspec_lt(ssl2);
                qn=wtk_queue_pop(msg_q);
                smsg=data_offset2(qn,wtk_ssl2_msg_t,q_n);
                wtk_ssl2_push_msg(ssl2, smsg);
            }
        }else
        {
            wtk_ssl2_flush_aspec(ssl2,msg);
            wtk_ssl2_push_msg(ssl2, msg);
        }
    }
    if(ssl2->cfg->use_online)
    {
        if(ssl2->oframe==0)
        {
            wtk_ssl2_flush_spec_ssl2(ssl2);
            if(ssl2->notify)
            {
                ts=((ssl2->nframe-ssl2->cfg->online_frame)*ssl2->cfg->wins/2)/(ssl2->cfg->rate/1000);
                te=(ssl2->nframe*ssl2->cfg->wins/2)/(ssl2->cfg->rate/1000);
                ssl2->notify(ssl2->ths, ssl2->nbest_extp, ssl2->nbest, ts,te);
            }

            ssl2->nbest=0;
            ssl2->oframe=ssl2->cfg->online_frame;

            for(n=0; n<nangle; ++n)
            {
                ssl2->nextp[n].nspecsum=0; 
                ssl2->nextp[n].is_peak=0; 
            }
        }else if(is_end)
        {
            wtk_ssl2_flush_spec_ssl2(ssl2);
            if(ssl2->oframe*1.0/ssl2->cfg->online_frame<=0.5 && ssl2->notify)
            {
                ts=((ssl2->nframe-ssl2->cfg->online_frame+ssl2->oframe)*ssl2->cfg->wins/2)/(ssl2->cfg->rate/1000);
                te=(ssl2->nframe*ssl2->cfg->wins/2)/(ssl2->cfg->rate/1000);
                ssl2->notify(ssl2->ths, ssl2->nbest_extp, ssl2->nbest, ts,te);
            }

            ssl2->nbest=0;
            ssl2->oframe=ssl2->cfg->online_frame;

            for(n=0; n<nangle; ++n)
            {
                ssl2->nextp[n].nspecsum=0;
                ssl2->nextp[n].is_peak=0; 
            }
        }
    }else if(is_end)
    {
        wtk_ssl2_flush_spec_ssl2(ssl2);
        while(msg_q->length>0)
        {
            qn=wtk_queue_pop(msg_q);
            smsg=data_offset2(qn,wtk_ssl2_msg_t,q_n);
            wtk_ssl2_push_msg(ssl2, smsg);
        }
    }
}



void wtk_ssl2_feed_fft2(wtk_ssl2_t *ssl2,wtk_complex_t **fft,int is_end)
{
    wtk_queue_t *msg_q=&(ssl2->msg_q);
    int lt=ssl2->cfg->lt;
    wtk_ssl2_msg_t *msg, *smsg;
    wtk_queue_node_t *qn;
    int nbin=ssl2->nbin;
    int channel=ssl2->channel;
    int i,k;
    int nangle=ssl2->nangle;
    int n;
    int ts,te;

    if(fft)
    {
        msg=wtk_ssl2_pop_msg(ssl2);
        for(k=0; k<nbin; ++k)
        {
            for(i=0; i<channel; ++i)
            {
                msg->fft[k][i]=fft[k][i];
            }
        }

        if(ssl2->cov)
        {
            wtk_queue_push(msg_q,&(msg->q_n));
            if(msg_q->length>=lt+1 && msg_q->length<2*lt+1)
            {
                wtk_ssl2_flush_aspec_lt(ssl2);
            }else if(msg_q->length==2*lt+1)
            {
                wtk_ssl2_flush_aspec_lt(ssl2);
                qn=wtk_queue_pop(msg_q);
                smsg=data_offset2(qn,wtk_ssl2_msg_t,q_n);
                wtk_ssl2_push_msg(ssl2, smsg);
            }
        }else
        {
            wtk_ssl2_flush_aspec(ssl2,msg);
            wtk_ssl2_push_msg(ssl2, msg);
        }
    }
    if(ssl2->cfg->use_online)
    {
        if(ssl2->oframe==0)
        {
            wtk_ssl2_flush_spec_ssl2(ssl2);
            if(ssl2->notify)
            {
                ts=((ssl2->nframe-ssl2->cfg->online_frame)*ssl2->cfg->wins/2)/(ssl2->cfg->rate/1000);
                te=(ssl2->nframe*ssl2->cfg->wins/2)/(ssl2->cfg->rate/1000);
                ssl2->notify(ssl2->ths, ssl2->nbest_extp, ssl2->nbest, ts,te);
            }

            ssl2->nbest=0;
            ssl2->oframe=ssl2->cfg->online_frame;

            for(n=0; n<nangle; ++n)
            {
                ssl2->nextp[n].nspecsum=0;
                ssl2->nextp[n].is_peak=0; 
            }
        }else if(is_end)
        {
            wtk_ssl2_flush_spec_ssl2(ssl2);
            if(ssl2->oframe*1.0/ssl2->cfg->online_frame<=0.5 && ssl2->notify)
            {
                ts=((ssl2->nframe-ssl2->cfg->online_frame+ssl2->oframe)*ssl2->cfg->wins/2)/(ssl2->cfg->rate/1000);
                te=(ssl2->nframe*ssl2->cfg->wins/2)/(ssl2->cfg->rate/1000);
                ssl2->notify(ssl2->ths, ssl2->nbest_extp, ssl2->nbest, ts,te);
            }

            ssl2->nbest=0;
            ssl2->oframe=ssl2->cfg->online_frame;

            for(n=0; n<nangle; ++n)
            {
                ssl2->nextp[n].nspecsum=0;
                ssl2->nextp[n].is_peak=0; 
            }
        }
    }else if(is_end)
    {
        wtk_ssl2_flush_spec_ssl2(ssl2);
        while(msg_q->length>0)
        {
            qn=wtk_queue_pop(msg_q);
            smsg=data_offset2(qn,wtk_ssl2_msg_t,q_n);
            wtk_ssl2_push_msg(ssl2, smsg);
        }
    }
}