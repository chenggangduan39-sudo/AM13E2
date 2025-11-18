#include "wtk_ssl.h" 

void wtk_ssl_on_stft2(wtk_ssl_t *ssl,wtk_stft2_msg_t *msg,int pos,int is_end);
int wtk_ssl_theta_dist(int th1, int th2);

wtk_ssl_t* wtk_ssl_new(wtk_ssl_cfg_t *cfg)
{
    wtk_ssl_t *ssl;
    float theta,phi;
    int tstep=cfg->theta_step, pstep=cfg->phi_step;
    int n;

    ssl=(wtk_ssl_t *)wtk_malloc(sizeof(wtk_ssl_t));
    ssl->cfg=cfg;
    ssl->ths=NULL;
    ssl->notify=NULL;

    ssl->stft2=wtk_stft2_new(&(cfg->stft2));
    wtk_stft2_set_notify(ssl->stft2,ssl,(wtk_stft2_notify_f)wtk_ssl_on_stft2);
    ssl->nbin=ssl->stft2->nbin;
    ssl->channel=cfg->stft2.channel;
    ssl->count_len=0;

    ssl->ntheta=0;
    ssl->nangle=0;
    for(theta=0; theta<=cfg->max_theta; theta+=tstep)
    {
        ++ssl->ntheta;
        for(phi=0; phi<=cfg->max_phi; phi+=pstep)
        {
            ++ssl->nangle;
        }
    }
    ssl->nphi=ssl->nangle/ssl->ntheta;
    ssl->nextp=(wtk_ssl_extp_t *)wtk_malloc(sizeof(wtk_ssl_extp_t)*ssl->nangle);
    ssl->max_idx=(int *)wtk_malloc(sizeof(int)*ssl->cfg->max_extp);

    ssl->aspec=wtk_aspec_new(&(cfg->aspec), ssl->nbin, ssl->nangle);
    wtk_aspec_reset(ssl->aspec);

    ssl->aspec->start_ang_num=ssl->nangle;
    for(theta=0,n=0; theta<=cfg->max_theta; theta+=tstep)
    {
        for(phi=0; phi<=cfg->max_phi; phi+=pstep,++n)
        {
            ssl->nextp[n].theta=theta;
            ssl->nextp[n].phi=phi;
            wtk_aspec_start(ssl->aspec, theta, phi, n);
        }
    }

    wtk_queue_init(&(ssl->stft2_q));
    ssl->cov=NULL;
    if(ssl->aspec->need_cov)
    {
        ssl->cov=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->aspec.channel*cfg->aspec.channel);
        if(cfg->lt<=0)
        {
            ssl->wint=wtk_malloc(sizeof(float));
            ssl->wint[0]=1;
        }else
        {
            ssl->wint=wtk_math_create_hanning_window(2*cfg->lt+1);
        }

        if(cfg->lf<=0)
        {
            ssl->winf=wtk_malloc(sizeof(float));
            ssl->winf[0]=1;
        }else
        {
            ssl->winf=wtk_math_create_hanning_window(2*cfg->lf+1);
        }
    }
    ssl->inv_cov=NULL;
    if(ssl->aspec->need_inv_cov)
    {
        ssl->inv_cov=(wtk_complex_t *)wtk_malloc(cfg->aspec.channel*cfg->aspec.channel*sizeof(wtk_complex_t));
        ssl->tmp=(wtk_dcomplex_t *)wtk_malloc(cfg->aspec.channel*cfg->aspec.channel*2*sizeof(wtk_dcomplex_t));
    }

    ssl->nbest_extp=(wtk_ssl_extp_t *)wtk_malloc(sizeof(wtk_ssl_extp_t)*ssl->cfg->max_extp);

    wtk_ssl_reset(ssl);

    return ssl;
}

void wtk_ssl_delete(wtk_ssl_t *ssl)
{
    if(ssl->cov)
    {
        wtk_free(ssl->cov);
        wtk_free(ssl->wint);
        wtk_free(ssl->winf);
        if(ssl->inv_cov)
        {
            wtk_free(ssl->inv_cov);
            wtk_free(ssl->tmp);
        }
    }

    wtk_aspec_delete(ssl->aspec);

    wtk_stft2_delete(ssl->stft2);

    wtk_free(ssl->nextp);
    wtk_free(ssl->max_idx);
    wtk_free(ssl->nbest_extp);
    wtk_free(ssl);
}

void wtk_ssl_reset(wtk_ssl_t *ssl)
{
    int nangle=ssl->nangle;
    int n;

    wtk_stft2_reset(ssl->stft2);

    ssl->nframe=0;
    ssl->nbest=0;

    for(n=0; n<nangle; ++n)
    {
        ssl->nextp[n].nspecsum=0;
        ssl->nextp[n].is_peak=0;
    }
}

void wtk_ssl_set_notify(wtk_ssl_t *ssl,void *ths,wtk_ssl_notify_f notify)
{
    ssl->ths=ths;
    ssl->notify=notify;
}

int wtk_ssl_theta_dist(int th1, int th2)
{
    int dist;

    dist=fabs(th1-th2);
    if(dist>180)
    {
        dist=360-dist;
    }
    return dist;
}

void wtk_ssl_flush_spec_ssl(wtk_ssl_t *ssl)
{
    int i,n,j,b;
    int ntheta=ssl->ntheta;
    int nphi=ssl->nphi;
    int nangle=ssl->nangle;
    wtk_ssl_extp_t *nextp=ssl->nextp;
    wtk_ssl_extp_t *nbest_extp=ssl->nbest_extp;
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
                    if(i == max_idx[n] || wtk_ssl_theta_dist(nextp[i].theta, nextp[max_idx[n]].theta)<min_thetasub)
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
void wtk_ssl_print(wtk_ssl_t *ssl)
{
    int i;

    for(i=0;i<ssl->nbest;++i)
    {
        printf("ssl theta/phi [%d]=%d %d %f\n",i,ssl->nbest_extp[i].theta,ssl->nbest_extp[i].phi,ssl->nbest_extp[i].nspecsum);
    }
}

void wtk_ssl_flush_aspec_lt(wtk_ssl_t *ssl)
{
    wtk_aspec_t *aspec=ssl->aspec;
    wtk_queue_t *stft2_q=&(ssl->stft2_q);
    int lf=ssl->cfg->lf;
    int i,j,k,k2,n,tt,ff;
    wtk_queue_node_t *qn;
    wtk_stft2_msg_t *smsg;
    int nbin=ssl->nbin;
    int channel=ssl->cfg->aspec.channel; 
    wtk_complex_t *cov=ssl->cov;
    wtk_complex_t **fft,*fft1,*fft2,*fft3,*a,*b;
    int nangle=ssl->nangle;
    float *wint=ssl->wint;
    float *winf=ssl->winf;
    float wint2,wintf,winsum;
    wtk_complex_t *inv_cov=ssl->inv_cov;
    wtk_dcomplex_t *tmp=ssl->tmp;
    float cov_travg;
    int ret;
    wtk_ssl_extp_t *nextp=ssl->nextp;
    int ns=ssl->cfg->specsum_ns;
    int ne=ssl->cfg->specsum_ne;

    ++ssl->nframe;

    for(k=ns;k<ne;++k)
    {
        memset(cov,0,sizeof(wtk_complex_t)*channel*channel);
        winsum=0;
        for(qn=stft2_q->pop,tt=0;qn;qn=qn->next,++tt)
        {
            wint2=wint[tt];
            smsg=data_offset2(qn,wtk_stft2_msg_t,q_n);
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
        for(n=0; n<nangle; ++n)
        {
            nextp[n].nspecsum+=wtk_aspec_flush_spec_k(aspec, NULL, 0, cov_travg, cov, inv_cov, k ,n);
        }
    }
}

void wtk_ssl_flush_aspec(wtk_ssl_t *ssl, wtk_stft2_msg_t *msg)
// void wtk_ssl_flush_aspec(wtk_ssl_t *ssl, wtk_stft2_msg_t *msg)
{
    wtk_aspec_t *aspec=ssl->aspec;
    int k,n,i;
    int nangle=ssl->nangle;
    int channel=ssl->channel;
    wtk_complex_t **fft=msg->fft, *fft2;
    float fftabs2;
    wtk_ssl_extp_t *nextp=ssl->nextp;
    int ns=ssl->cfg->specsum_ns;
    int ne=ssl->cfg->specsum_ne;

    ++ssl->nframe;

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

void wtk_ssl_on_stft2(wtk_ssl_t *ssl,wtk_stft2_msg_t *msg,int pos,int is_end)
{
    wtk_queue_t *stft2_q=&(ssl->stft2_q);
    int lt=ssl->cfg->lt;
    wtk_queue_node_t *qn;
    wtk_stft2_msg_t *smsg;

    if(msg)
    {
        if(ssl->cov)
        {
            wtk_queue_push(stft2_q,&(msg->q_n));
            if(stft2_q->length>=lt+1 && stft2_q->length<2*lt+1)
            {
                wtk_ssl_flush_aspec_lt(ssl);
            }else if(stft2_q->length==2*lt+1)
            {
                wtk_ssl_flush_aspec_lt(ssl);
                qn=wtk_queue_pop(stft2_q);
                smsg=data_offset2(qn,wtk_stft2_msg_t,q_n);
                wtk_stft2_push_msg(ssl->stft2,smsg);
            }
        }else
        {
            wtk_ssl_flush_aspec(ssl,msg);
            wtk_stft2_push_msg(ssl->stft2, msg);
        }
    }
    if(is_end)
    {
        wtk_ssl_flush_spec_ssl(ssl);
    }
}

void wtk_ssl_feed(wtk_ssl_t *ssl,short **data,int len,int is_end)
{
#ifdef DEBUG_WAV
	static wtk_wavfile_t *mic_log=NULL;

	if(!mic_log)
	{
		mic_log=wtk_wavfile_new(16000);
		wtk_wavfile_set_channel(mic_log,ssl->cfg->aspec.channel);
		wtk_wavfile_open2(mic_log,"ssl");
	}
	if(len>0)
	{
		wtk_wavfile_write_mc(mic_log,data,len);
	}
	if(is_end && mic_log)
	{
		wtk_wavfile_close(mic_log);
		wtk_wavfile_delete(mic_log);
		mic_log=NULL;
	}
#endif

    wtk_stft2_feed(ssl->stft2,data,len,is_end);
}

void wtk_ssl_feed_count(wtk_ssl_t *ssl, short **data, int len, int is_end){

    wtk_ssl_feed(ssl, data, len, is_end);
    ssl->count_len += len;
    if(ssl->count_len > ssl->cfg->notify_len){
        wtk_ssl_feed(ssl, NULL, 0, 1);
        if(ssl->notify && ssl->nbest_extp[0].nspecsum > ssl->cfg->min_energy){
            ssl->notify(ssl->ths, ssl->nbest_extp);
        }
        wtk_ssl_reset(ssl);
        ssl->count_len = 0;
    }
}

wtk_stft2_msg_t* wtk_ssl_stft2_msg_new(wtk_ssl_t *ssl)
{
	wtk_stft2_msg_t *msg;

	msg=(wtk_stft2_msg_t*)wtk_malloc(sizeof(wtk_stft2_msg_t));
	msg->hook=NULL;
	msg->fft=wtk_complex_new_p2(ssl->nbin,ssl->channel);
	return msg;
}

void wtk_ssl_stft2_msg_delete(wtk_ssl_t *ssl,wtk_stft2_msg_t *msg)
{
	wtk_complex_delete_p2(msg->fft,ssl->nbin);
	wtk_free(msg);
}

wtk_stft2_msg_t* wtk_ssl_pop_stft2_msg(wtk_ssl_t *ssl)
{
	return  (wtk_stft2_msg_t*)wtk_hoard_pop(&(ssl->msg_hoard));
}

void wtk_ssl_push_stft2_msg(wtk_ssl_t *ssl,wtk_stft2_msg_t *msg)
{
	wtk_hoard_push(&(ssl->msg_hoard),msg);
}

wtk_stft2_msg_t* wtk_ssl_stft2_msg_copy(wtk_ssl_t *ssl,wtk_stft2_msg_t *msg,int channel,int nbin)
{
	wtk_stft2_msg_t *vmsg;

	vmsg=wtk_ssl_pop_stft2_msg(ssl);
	vmsg->s=msg->s;
	wtk_complex_cpy_p2(vmsg->fft,msg->fft,nbin,channel);
	return vmsg;
}

wtk_ssl_t* wtk_ssl_new2(wtk_ssl_cfg_t *cfg,wtk_stft2_t *stft)
{
    wtk_ssl_t *ssl;
    float theta, phi;
    int tstep=cfg->theta_step;
    int pstep=cfg->phi_step;
    int n;

    ssl=(wtk_ssl_t *)wtk_malloc(sizeof(wtk_ssl_t));
    ssl->cfg=cfg;
    // ssl->ths=NULL;
    // ssl->notify=NULL;
    wtk_hoard_init2(&(ssl->msg_hoard),offsetof(wtk_stft2_msg_t,hoard_n),10,
        (wtk_new_handler_t)wtk_ssl_stft2_msg_new,
        (wtk_delete_handler2_t)wtk_ssl_stft2_msg_delete,
        ssl);

    ssl->stft2=stft;
    ssl->nbin=stft->nbin;
    ssl->channel=stft->cfg->channel;

    ssl->ntheta=0;
    ssl->nangle=0;
    for(theta=0; theta<=cfg->max_theta; theta+=tstep)
    {
        ++ssl->ntheta;
        for(phi=0; phi<=cfg->max_phi; phi+=pstep)
        {
            ++ssl->nangle;
        }
    }
    ssl->nphi=ssl->nangle/ssl->ntheta;
    ssl->nextp=(wtk_ssl_extp_t *)wtk_malloc(sizeof(wtk_ssl_extp_t)*ssl->nangle);
    ssl->max_idx=(int *)wtk_malloc(sizeof(int)*ssl->cfg->max_extp);
    
    ssl->aspec=wtk_aspec_new(&(cfg->aspec), ssl->nbin, ssl->nangle);
    wtk_aspec_reset(ssl->aspec);

    ssl->aspec->start_ang_num=ssl->nangle;
    for(theta=0,n=0; theta<=cfg->max_theta; theta+=tstep)
    {
        for(phi=0; phi<=cfg->max_phi; phi+=pstep,++n)
        {
            ssl->nextp[n].theta=theta;
            ssl->nextp[n].phi=phi;
            wtk_aspec_start(ssl->aspec, theta, phi, n);
        }
    }

    wtk_queue_init(&(ssl->stft2_q));
    ssl->cov=NULL;
    if(ssl->aspec->need_cov)
    {
        ssl->cov=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->aspec.channel*cfg->aspec.channel);
        if(cfg->lt<=0)
        {
            ssl->wint=wtk_malloc(sizeof(float));
            ssl->wint[0]=1;
        }else
        {
            ssl->wint=wtk_math_create_hanning_window(2*cfg->lt+1);
        }

        if(cfg->lf<=0)
        {
            ssl->winf=wtk_malloc(sizeof(float));
            ssl->winf[0]=1;
        }else
        {
            ssl->winf=wtk_math_create_hanning_window(2*cfg->lf+1);
        }
    }
    ssl->inv_cov=NULL;
    if(ssl->aspec->need_inv_cov)
    {
        ssl->inv_cov=(wtk_complex_t *)wtk_malloc(cfg->aspec.channel*cfg->aspec.channel*sizeof(wtk_complex_t));
        ssl->tmp=(wtk_dcomplex_t *)wtk_malloc(cfg->aspec.channel*cfg->aspec.channel*2*sizeof(wtk_dcomplex_t));
    }

    ssl->nbest_extp=(wtk_ssl_extp_t *)wtk_malloc(sizeof(wtk_ssl_extp_t)*ssl->cfg->max_extp);

    wtk_ssl_reset2(ssl);

    return ssl;
}

void wtk_ssl_delete2(wtk_ssl_t *ssl)
{
	wtk_hoard_clean(&(ssl->msg_hoard));
    if(ssl->cov)
    {
        wtk_free(ssl->cov);
        wtk_free(ssl->wint);
        wtk_free(ssl->winf);
        if(ssl->inv_cov)
        {
            wtk_free(ssl->inv_cov);
            wtk_free(ssl->tmp);
        }
    }

    wtk_aspec_delete(ssl->aspec);

    wtk_free(ssl->nextp);
    wtk_free(ssl->max_idx);
    wtk_free(ssl->nbest_extp);
    wtk_free(ssl);
}

void wtk_ssl_reset2(wtk_ssl_t *ssl)
{
    int nangle=ssl->nangle;
    int n;

    ssl->nframe=0;
    ssl->nbest=0;

    for(n=0; n<nangle; ++n)
    {
        ssl->nextp[n].nspecsum=0;
        ssl->nextp[n].is_peak=0;
    }
}


void wtk_ssl_feed_stft2msg(wtk_ssl_t *ssl,wtk_stft2_msg_t *smsg2,int is_end)
{
    wtk_queue_t *stft2_q=&(ssl->stft2_q);
    int lt=ssl->cfg->lt;
    wtk_stft2_msg_t *msg, *smsg;
    wtk_queue_node_t *qn;

#ifdef USE_LOG
    short *pv=NULL;
    int k=0;
    int i;
	static wtk_wavfile_t *log=NULL;
    wtk_complex_t tmp[1024];
    float pad[1024], data[1024];

    if(smsg2)
    {
        for(k=0;k<ssl->nbin;++k)
        {
            tmp[k]=smsg2->fft[k][0];
        }

        k=wtk_stft2_output_ifft(ssl->stft2,tmp,data,pad,512,is_end);
        pv=(short *)data;
        for(i=0;i<k;++i)
        {
            if(fabs(data[i])<32000.0)
            {
                pv[i]=data[i];
            }else
            {
                if(data[i]>0)
                {
                    pv[i]=32000;
                }else
                {
                    pv[i]=-32000;
                }
            }
        }
    }
    if(!log)
    {
        log=wtk_wavfile_new(16000);
        wtk_wavfile_open2(log,"ssl");
        log->max_pend=0;
    }
    if(k>0)
    {
        wtk_wavfile_write(log,(char *)pv,k<<1);
    }
    if(is_end)
    {
        wtk_debug("============ close ============\n");
        wtk_wavfile_close(log);
        wtk_wavfile_delete(log);
        log=NULL;
    }
#endif
    msg=NULL;
    if(smsg2)
    {
        msg = wtk_ssl_stft2_msg_copy(ssl, smsg2, ssl->stft2->cfg->channel, ssl->nbin);
    }

    if(msg)
    {
        if(ssl->cov)
        {
            wtk_queue_push(stft2_q,&(msg->q_n));
            if(stft2_q->length>=lt+1 && stft2_q->length<2*lt+1)
            {
                wtk_ssl_flush_aspec_lt(ssl);
            }else if(stft2_q->length==2*lt+1)
            {
                wtk_ssl_flush_aspec_lt(ssl);
                qn=wtk_queue_pop(stft2_q);
                smsg=data_offset2(qn,wtk_stft2_msg_t,q_n);
                wtk_ssl_push_stft2_msg(ssl, smsg);
            }
        }else
        {
            wtk_ssl_flush_aspec(ssl,msg);
            wtk_ssl_push_stft2_msg(ssl, msg);
        }
    }
    if(is_end)
    {
        wtk_ssl_flush_spec_ssl(ssl);
        while(stft2_q->length>0)
        {
            qn=wtk_queue_pop(stft2_q);
            smsg=data_offset2(qn,wtk_stft2_msg_t,q_n);
            wtk_ssl_push_stft2_msg(ssl, smsg);
        }
    }
}

static float wtk_theta_distance(float t1,float t2)
{
	float f;

	f=fabs(t1-t2);
	if(f>180)
	{
		f=360-f;
	}
	return f;
}


wtk_ssl_extp_t*  wtk_ssl_has_theta(wtk_ssl_t *ssl,float theta,float df)
{
	wtk_ssl_extp_t *item=NULL;
	float xf;
	int i;

	for(i=0;i<ssl->nbest;++i)
	{
		item=ssl->nbest_extp+i;
		xf=wtk_theta_distance(item->theta,theta);
		if(xf<df)
		{
			return item;
		}
	}
	return NULL;
}
