#include "wtk_wbf.h" 
void wtk_wbf_on_stft2(wtk_wbf_t *wbf,wtk_stft2_msg_t *msg,int pos,int is_end);

wtk_wbf_t* wtk_wbf_new(wtk_wbf_cfg_t *cfg)
{
    wtk_wbf_t *wbf;
    int i;

    wbf=(wtk_wbf_t *)wtk_malloc(sizeof(wtk_wbf_t));
    wbf->cfg=cfg;
    wbf->notify=NULL;
    wbf->notify2=NULL;
    wbf->ths=NULL;

    wbf->stft2=wtk_stft2_new(&(cfg->stft2));
    wtk_stft2_set_notify(wbf->stft2, wbf, (wtk_stft2_notify_f)wtk_wbf_on_stft2);
    wbf->nbin=wbf->stft2->nbin;
    wbf->channel=cfg->stft2.channel;

    wbf->theta=(int *)wtk_malloc(sizeof(int)*cfg->wbf_cnt);
    wbf->bf=(wtk_bf_t **)wtk_malloc(sizeof(wtk_bf_t *)*cfg->wbf_cnt);
    wbf->qmmse=NULL;
    if(cfg->use_post)
    {
        wbf->qmmse=(wtk_qmmse_t **)wtk_malloc(sizeof(wtk_qmmse_t *)*cfg->wbf_cnt);
    }
    for(i=0;i<cfg->wbf_cnt;++i)
    {
        wbf->bf[i]=wtk_bf_new(&(cfg->bf),cfg->stft2.win);    
        if(wbf->qmmse)
        {
            wbf->qmmse[i]=wtk_qmmse_new(&(cfg->qmmse));
        }
    }
    if(cfg->use_line)
    {
        wbf->theta_step=floor(180.0/(cfg->wbf_cnt-1));   
    }else
    {
        wbf->theta_step=floor(359.0/cfg->wbf_cnt)+1;   
    }
    wbf->theta_range=wbf->theta_step/2*(cfg->range_interval+1);
    
    wbf->ncov=wtk_complex_new_p3(cfg->wbf_cnt,wbf->nbin,wbf->channel*wbf->channel);
    wbf->ncnt_sum=wtk_float_new_p2(cfg->wbf_cnt,wbf->nbin);

    wbf->scov=NULL;
    wbf->scnt_sum=NULL;
    if(cfg->bf.use_gev || cfg->bf.use_eig_ovec || cfg->bf.use_r1_mwf || cfg->bf.use_sdw_mwf || cfg->bf.use_vs)
    {
        wbf->scov=wtk_complex_new_p3(cfg->wbf_cnt,wbf->nbin,wbf->channel*wbf->channel);
        wbf->scnt_sum=wtk_float_new_p2(cfg->wbf_cnt,wbf->nbin);
    }

    wbf->aspec=wtk_aspec_new(&(cfg->aspec), wbf->nbin, cfg->wbf_cnt);      
  
    wbf->cov=NULL;
    wtk_queue_init(&(wbf->stft2_q));
    if(wbf->aspec && wbf->aspec->need_cov)
    {
        wbf->cov=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->stft2.channel*cfg->stft2.channel);
        if(cfg->lt<=0)
        {
            wbf->wint=wtk_malloc(sizeof(float));
            wbf->wint[0]=1;
        }else
        {
            wbf->wint=wtk_math_create_hanning_window(2*cfg->lt+1);
        }

        if(cfg->lf<=0)
        {
            wbf->winf=wtk_malloc(sizeof(float));
            wbf->winf[0]=1;
        }else
        {
            wbf->winf=wtk_math_create_hanning_window(2*cfg->lf+1);
        }
    }
    wbf->inv_cov=NULL;
    wbf->tmp=NULL;
    if(wbf->aspec && wbf->aspec->need_inv_cov)
    {
        wbf->inv_cov=(wtk_complex_t *)wtk_malloc(cfg->stft2.channel*cfg->stft2.channel*sizeof(wtk_complex_t));
        wbf->tmp=(wtk_dcomplex_t *)wtk_malloc(cfg->stft2.channel*cfg->stft2.channel*2*sizeof(wtk_dcomplex_t));
    }

    wbf->spec_k=(float *)wtk_malloc(sizeof(float)*cfg->wbf_cnt);

    wbf->cohv=wtk_float_new_p2(cfg->wbf_cnt,wbf->nbin);

    wtk_wbf_reset(wbf);

    return wbf;
}

void wtk_wbf_delete(wtk_wbf_t *wbf)
{
    int i;

    wtk_stft2_delete(wbf->stft2);
    for(i=0;i<wbf->cfg->wbf_cnt;++i)
    {
        wtk_bf_delete(wbf->bf[i]);
        if(wbf->qmmse)
        {
            wtk_qmmse_delete(wbf->qmmse[i]);
        }
    }
    wtk_free(wbf->bf);
    wtk_free(wbf->qmmse);

    wtk_complex_delete_p3(wbf->ncov, wbf->cfg->wbf_cnt, wbf->nbin);
    wtk_float_delete_p2(wbf->ncnt_sum, wbf->cfg->wbf_cnt);
    if(wbf->scov)
    {
        wtk_complex_delete_p3(wbf->scov, wbf->cfg->wbf_cnt, wbf->nbin);
        wtk_float_delete_p2(wbf->scnt_sum, wbf->cfg->wbf_cnt);
    }
    if(wbf->cov)
    {
        wtk_free(wbf->cov);
        wtk_free(wbf->wint);
        wtk_free(wbf->winf);
    }
    if(wbf->inv_cov)
    {
        wtk_free(wbf->inv_cov);
    }
    if(wbf->tmp)
    {
        wtk_free(wbf->tmp);
    }

    wtk_aspec_delete(wbf->aspec);

    wtk_free(wbf->spec_k);
    wtk_float_delete_p2(wbf->cohv, wbf->cfg->wbf_cnt);

    wtk_free(wbf->theta);

    wtk_free(wbf);
}

void wtk_wbf_reset(wtk_wbf_t *wbf)
{
    int i;

    wbf->end_pos=0;
    wtk_stft2_reset(wbf->stft2);
    for(i=0;i<wbf->cfg->wbf_cnt;++i)
    {
        wtk_bf_reset(wbf->bf[i]);
        if(wbf->qmmse)
        {
            wtk_qmmse_reset(wbf->qmmse[i]);
        }
    }

    memset(wbf->theta, 0, sizeof(float)*wbf->cfg->wbf_cnt);
    wtk_complex_zero_p3(wbf->ncov, wbf->cfg->wbf_cnt, wbf->nbin, wbf->channel*wbf->channel);
    wtk_float_zero_p2(wbf->ncnt_sum, wbf->cfg->wbf_cnt, wbf->nbin);
    if(wbf->scov)
    {
        wtk_complex_zero_p3(wbf->scov, wbf->cfg->wbf_cnt, wbf->nbin, wbf->channel*wbf->channel);
        wtk_float_zero_p2(wbf->scnt_sum, wbf->cfg->wbf_cnt, wbf->nbin);
    }

    wtk_aspec_reset(wbf->aspec);
}

void wtk_wbf_set_notify(wtk_wbf_t *wbf,void *ths,wtk_wbf_notify_f notify)
{
    wbf->ths=ths;
    wbf->notify=notify;
}

void wtk_wbf_start(wtk_wbf_t *wbf)
{
    int i;
    int theta=0;

    wbf->aspec->start_ang_num=wbf->cfg->wbf_cnt;
    for(i=0;i<wbf->cfg->wbf_cnt;++i, theta+=wbf->theta_step)
    {
        wbf->theta[i]=theta;
        wtk_aspec_start(wbf->aspec, theta, 0, i);
        wtk_bf_update_ovec(wbf->bf[i], theta, 0);
        wtk_bf_init_w(wbf->bf[i]);
    }
}

void wtk_wbf_feed(wtk_wbf_t *wbf,short **data,int len,int is_end)
{
    wtk_stft2_feed(wbf->stft2, data, len, is_end);
}

int wtk_wbf_update_rnn2(wtk_wbf_t *wbf, wtk_complex_t **fft,int bf_cnt,int k)
{
    int channel=wbf->channel;
    int i,j;
    wtk_complex_t *fft1,*fft2,*a;
    wtk_complex_t **scov;
    float *scnt_sum;
    float alpha=wbf->cfg->scov_alpha;
    float alpha_1=1-wbf->cfg->scov_alpha;
    wtk_bf_t *bf=wbf->bf[bf_cnt];
    // float ff;
    int ret=0;
    static int flushcnt[1024]={0};
    int flushbfgap=wbf->cfg->flushbfgap;

    if(wbf->scov==NULL)
    {
        return ret;
    }
    scov=wbf->scov[bf_cnt];
    scnt_sum=wbf->scnt_sum[bf_cnt];
    if(scnt_sum[k]>=wbf->cfg->init_scovnf)
    {
        ++scnt_sum[k];
        fft1=fft2=fft[k];
        for(i=0;i<channel;++i,++fft1)
        {
            fft2=fft1;
            for(j=i;j<channel;++j,++fft2)
            {
                a=scov[k]+i*channel+j;
                a->a=alpha_1*a->a+alpha*(fft1->a*fft2->a+fft1->b*fft2->b);
                a->b=alpha_1*a->b+alpha*(-fft1->a*fft2->b+fft1->b*fft2->a);
            }
        }
    }else
    {
        ++scnt_sum[k];
        fft1=fft2=fft[k];
        for(i=0;i<channel;++i,++fft1)
        {
            fft2=fft1;
            for(j=i;j<channel;++j,++fft2)
            {
                a=scov[k]+i*channel+j;
                a->a+=fft1->a*fft2->a+fft1->b*fft2->b;
                a->b+=-fft1->a*fft2->b+fft1->b*fft2->a;
            }
        }
    }
    ret=0;
    if(scnt_sum[k]>5)
    {
        ++flushcnt[k];
        if(flushcnt[k]==flushbfgap)
        {
            flushcnt[k]=0;
            wtk_bf_update_scov(bf,scov,k);
            ret=1;
        }
    }

    return ret;
}


int wtk_wbf_update_rnn1(wtk_wbf_t *wbf,wtk_complex_t **fft,int bf_cnt,int k)
{
    int channel=wbf->channel;
    int i,j;
    wtk_complex_t *fft1,*fft2,*a;//,*b;
    wtk_complex_t **ncov=wbf->ncov[bf_cnt];
    float *ncnt_sum=wbf->ncnt_sum[bf_cnt];
    // wtk_complex_t **ncovtmp=wbf->ncovtmp;
    float alpha=wbf->cfg->ncov_alpha;
    float alpha_1=1-wbf->cfg->ncov_alpha;
    wtk_bf_t *bf=wbf->bf[bf_cnt];
    int ret=0;
    static int flushcnt[1024]={0};
    int flushbfgap=wbf->cfg->flushbfgap;

    if(ncnt_sum[k]>=wbf->cfg->init_ncovnf)
    {
        ++ncnt_sum[k];
        fft1=fft2=fft[k];
        for(i=0;i<channel;++i,++fft1)
        {
            fft2=fft1;
            for(j=i;j<channel;++j,++fft2)
            {
                a=ncov[k]+i*channel+j;
                a->a=alpha_1*a->a+alpha*(fft1->a*fft2->a+fft1->b*fft2->b);
                a->b=alpha_1*a->b+alpha*(-fft1->a*fft2->b+fft1->b*fft2->a);
            }
        }
    }else
    {
        ++ncnt_sum[k];
        fft1=fft2=fft[k];
        for(i=0;i<channel;++i,++fft1)
        {
            fft2=fft1;
            for(j=i;j<channel;++j,++fft2)
            {
                a=ncov[k]+i*channel+j;
                a->a+=fft1->a*fft2->a+fft1->b*fft2->b;
                a->b+=-fft1->a*fft2->b+fft1->b*fft2->a;
            }
        }
    }
    ret=0;
    if(ncnt_sum[k]>5)
    {
        ++flushcnt[k];
        if(flushcnt[k]==flushbfgap)
        {
            flushcnt[k]=0;
            wtk_bf_update_ncov(bf,ncov,k);
            ret=1;
        }
    }

    return ret;
}

void wtk_wbf_notify_data(wtk_wbf_t *wbf,float *data,int len, int idx)
{
    short *pv=(short *)data;
    int i;

    for(i=0;i<len;++i)
    {
        if(fabs(data[i])<1.0)
        {
            pv[i]=data[i]*32000;
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

    if(wbf->notify)
    {
        wbf->notify(wbf->ths,pv,len,idx,0);
    }
}

void wtk_wbf_flush2(wtk_wbf_t *wbf,wtk_stft2_msg_t *smsg,float **cohv, int is_end)
{
    int i,k;
    wtk_complex_t *bf_out=NULL;
    int wbf_cnt=wbf->cfg->wbf_cnt;
    float *cohv1;
    wtk_bf_t *bf;
    wtk_qmmse_t *qmmse=NULL;

    for(i=0;i<wbf_cnt;++i)
    {
        if(smsg)
        {
            bf=wbf->bf[i];
            if(wbf->qmmse)
            {
                qmmse=wbf->qmmse[i];
            }
            cohv1=cohv[i];

            bf_out=wtk_bf_output_fft2_msg2(bf,smsg,cohv1);
            if(qmmse)
            {
                wtk_qmmse_feed_cohv(qmmse,bf_out,cohv1);      
            }
        }
        if(wbf->notify)
        {
            if(bf_out)
            {
                k=wtk_stft2_output_ifft(wbf->stft2,bf_out,wbf->stft2->output,bf->pad,wbf->end_pos,is_end);
                wtk_wbf_notify_data(wbf,wbf->stft2->output,k,i);
            }
            if(is_end)
            {
                wbf->notify(wbf->ths,NULL,0,0,1);
            }
        }else if(wbf->notify2 && bf_out)
        {
            wbf->notify2(wbf->ths, bf_out, i, is_end);
        }
    }
}

void wtk_wbf_flush(wtk_wbf_t *wbf,  wtk_stft2_msg_t *smsg, float **cohv, int is_end)
{
    int nbin=wbf->nbin;
    int i,k,c,b;
    int wbf_cnt=wbf->cfg->wbf_cnt;
    float *cohv1;
    int channel=wbf->channel;
    wtk_bf_t *bf;
    float *ncnt_sum, *scnt_sum=NULL;

    for(i=0;i<wbf_cnt;++i)
    {
        bf=wbf->bf[i];
        cohv1=cohv[i];
        ncnt_sum=wbf->ncnt_sum[i];
        if(wbf->scnt_sum)
        {
            scnt_sum=wbf->scnt_sum[i];
        }
        for(k=1;k<nbin-1;++k)
        {
            b=0;
            if(cohv1[k]<0.0)
            {
                b=wtk_wbf_update_rnn1(wbf,smsg->fft,i,k);
            }else
            {
                b=wtk_wbf_update_rnn2(wbf,smsg->fft,i,k);
            }
            if(b && ncnt_sum[k]>5 && (scnt_sum==NULL ||  scnt_sum[k]>5))
            {
                wtk_bf_update_w(bf,k);
            }

            if(wbf->cfg->debug)
            {
                if(cohv1[k]<0.0)
                {
                    for(c=0;c<channel;++c)
                    {
                        bf->w[k][c].a=0;
                        bf->w[k][c].b=0;
                    }
                }else
                {
                    for(c=0;c<channel;++c)
                    {
                        bf->w[k][c].a=0;
                        bf->w[k][c].b=0;
                        if(c==0)
                        {
                            bf->w[k][c].a=1;
                        }
                    }
                }
            }
        }
    }

    wtk_wbf_flush2(wbf,smsg,cohv,is_end);
}

void wtk_wbf_update_aspec2(wtk_wbf_t *wbf, wtk_aspec_t *aspec, wtk_complex_t *cov, 
                                                                                                        wtk_complex_t *inv_cov, float cov_travg, int k, float *specsum)
{
    float *spec_k=wbf->spec_k;
    float **cohv=wbf->cohv,cohvf;
    int sang_num=aspec->start_ang_num;
    int n;
    int interval=wbf->cfg->range_interval+1;
    int specsum_ns=wbf->cfg->specsum_ns;
    int specsum_ne=wbf->cfg->specsum_ne;

    for(n=0; n<sang_num; ++n)
    {
        spec_k[n]=wtk_aspec_flush_spec_k(aspec, NULL, 0, cov_travg, cov, inv_cov, k ,n);
    }

    for(n=0; n<sang_num; ++n)
    {
        cohvf=1;
        if(wbf->cfg->use_line==1)
        {
            if(n+interval<sang_num && n-interval>=0)
            {
                if(spec_k[n]<spec_k[n+interval] || spec_k[n]<spec_k[n-interval])
                {
                    cohvf=-1;
                }else if(k>=specsum_ns && k<=specsum_ne)
                {
                    specsum[n]+=spec_k[n]*2-spec_k[n+interval] -spec_k[n-interval];
                }
            }else if(n+interval>=sang_num && n-interval>=0)
            {
                if(spec_k[n]<spec_k[n-interval])
                {
                    cohvf=-1;
                }else if(k>=specsum_ns && k<=specsum_ne)
                {
                    specsum[n]+=spec_k[n]-spec_k[n-interval];
                }
            }else if(n+interval<sang_num && n-interval<0)
            {
                if(spec_k[n]<spec_k[n+interval])
                {
                    cohvf=-1;
                }else if(k>=specsum_ns && k<=specsum_ne)
                {
                    specsum[n]+=spec_k[n]-spec_k[n+interval];
                }
            }   
        }else
        {
            if(n+interval<sang_num && n-interval>=0)
            {
                if(spec_k[n]<spec_k[n+interval] || spec_k[n]<spec_k[n-interval])
                {
                    cohvf=-1;
                }else if(k>=specsum_ns && k<=specsum_ne)
                {
                    specsum[n]+=spec_k[n]*2-spec_k[n+interval] -spec_k[n-interval];
                }
            }else if(n+interval>=sang_num && n-interval>=0)
            {
                if(spec_k[n]<spec_k[n+interval-sang_num] || spec_k[n]<spec_k[n-interval])
                {
                    cohvf=-1;
                }else if(k>=specsum_ns && k<=specsum_ne)
                {
                    specsum[n]+=spec_k[n]*2-spec_k[n+interval-sang_num] -spec_k[n-interval];
                }
            }else if(n+interval<sang_num && n-interval<0)
            {
                if(spec_k[n]<spec_k[n+interval] || spec_k[n]<spec_k[n-interval+sang_num])
                {
                    cohvf=-1;
                }else if(k>=specsum_ns && k<=specsum_ne)
                {
                    specsum[n]+=spec_k[n]*2-spec_k[n+interval] -spec_k[n-interval+sang_num];
                }
            }   
        }
        cohv[n][k]=cohvf;
    }
}

void wtk_wbf_flush_aspec_lt(wtk_wbf_t *wbf, int index, int is_end)
{
    wtk_queue_t *stft2_q=&(wbf->stft2_q);
    int lf=wbf->cfg->lf;
    int i,j,k,k2,tt,ff;
    wtk_queue_node_t *qn;
    wtk_stft2_msg_t *smsg,*smsg_index;
    int nbin=wbf->nbin;
    int channel=wbf->channel;
    wtk_complex_t *cov=wbf->cov;
    wtk_complex_t **fft,*fft1,*fft2,*fft3,*a,*b;
    float *wint=wbf->wint;
    float *winf=wbf->winf;
    float wint2,wintf,winsum;
    wtk_complex_t *inv_cov=wbf->inv_cov;
    wtk_dcomplex_t *tmp=wbf->tmp;
    float cov_travg;
    int ret;
    float **cohv=wbf->cohv;
    float specsum[512]={0};
    int sang_num=wbf->aspec->start_ang_num;
    int n;

    qn=wtk_queue_peek(stft2_q, index);
    smsg_index=data_offset2(qn,wtk_stft2_msg_t,q_n);
    
    for(k=1;k<nbin-1;++k)
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
        if(inv_cov)
        {
            ret=wtk_complex_invx4(cov,tmp,channel,inv_cov,1);            
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
        cov_travg=0;
        if(wbf->aspec->need_cov_travg) 
        {
            for(i=0;i<channel;++i)
            {
                cov_travg+=cov[i*channel+i].a;
            }
            cov_travg/=channel;
        }

        wtk_wbf_update_aspec2(wbf, wbf->aspec, cov, inv_cov, cov_travg, k, specsum);
    }
    if(wbf->cfg->use_specsum_bl)
    {
        for(n=0; n<sang_num; ++n)
        {
            if(specsum[n]<=wbf->cfg->specsum_bl)
            {
                for(k=0;k<nbin;++k)
                {
                    cohv[n][k]=-1;
                }
            }
        }
    }
    wtk_wbf_flush(wbf, smsg_index, cohv, is_end);
}

void wtk_wbf_update_aspec(wtk_wbf_t *wbf, wtk_aspec_t *aspec, wtk_complex_t **fft, float fftabs2, int k, float *specsum)
{
    float *spec_k=wbf->spec_k;
    float **cohv=wbf->cohv,cohvf;
    int sang_num=aspec->start_ang_num;
    int n;
    int interval=wbf->cfg->range_interval+1;
    int specsum_ns=wbf->cfg->specsum_ns;
    int specsum_ne=wbf->cfg->specsum_ne;


    for(n=0; n<sang_num; ++n)
    {
        spec_k[n]=wtk_aspec_flush_spec_k(aspec, fft, fftabs2, 0, NULL, NULL, k ,n);
    }

    for(n=0; n<sang_num; ++n)
    {
        cohvf=1;
        if(wbf->cfg->use_line==1)
        {
            if(n+interval<sang_num && n-interval>=0)
            {
                if(spec_k[n]<spec_k[n+interval] || spec_k[n]<spec_k[n-interval])
                {
                    cohvf=-1;
                }else if(k>=specsum_ns && k<=specsum_ne)
                {
                    specsum[n]+=spec_k[n]*2-spec_k[n+interval] -spec_k[n-interval];
                }
            }else if(n+interval>=sang_num && n-interval>=0)
            {
                if(spec_k[n]<spec_k[n-interval])
                {
                    cohvf=-1;
                }else if(k>=specsum_ns && k<=specsum_ne)
                {
                    specsum[n]+=spec_k[n] -spec_k[n-interval];
                }
            }else if(n+interval<sang_num && n-interval<0)
            {
                if(spec_k[n]<spec_k[n+interval])
                {
                    cohvf=-1;
                }else if(k>=specsum_ns && k<=specsum_ne)
                {
                    specsum[n]+=spec_k[n]-spec_k[n+interval];
                }
            }   
        }else
        {
            if(n+interval<sang_num && n-interval>=0)
            {
                if(spec_k[n]<spec_k[n+interval] || spec_k[n]<spec_k[n-interval])
                {
                    cohvf=-1;
                }else if(k>=specsum_ns && k<=specsum_ne)
                {
                    specsum[n]+=spec_k[n]*2-spec_k[n+interval] -spec_k[n-interval];
                }
            }else if(n+interval>=sang_num && n-interval>=0)
            {
                if(spec_k[n]<spec_k[n+interval-sang_num] || spec_k[n]<spec_k[n-interval])
                {
                    cohvf=-1;
                }else if(k>=specsum_ns && k<=specsum_ne)
                {
                    specsum[n]+=spec_k[n]*2-spec_k[n+interval-sang_num] -spec_k[n-interval];
                }
            }else if(n+interval<sang_num && n-interval<0)
            {
                if(spec_k[n]<spec_k[n+interval] || spec_k[n]<spec_k[n-interval+sang_num])
                {
                    cohvf=-1;
                }else if(k>=specsum_ns && k<=specsum_ne)
                {
                    specsum[n]+=spec_k[n]*2-spec_k[n+interval] -spec_k[n-interval+sang_num];
                }
            }   
        }
        cohv[n][k]=cohvf;
    }
}

void wtk_wbf_flush_aspec(wtk_wbf_t *wbf, wtk_stft2_msg_t *msg, int is_end)
{
    int k,i;
    int nbin=wbf->nbin;
    int channel=wbf->channel;
    wtk_complex_t **fft, *fft2;
    float fftabs2;
    float **cohv=wbf->cohv;
    float specsum[512]={0};
    int sang_num=wbf->aspec->start_ang_num;
    int n;

    fft=msg->fft;
    for(k=1; k<nbin-1; ++k)
    {
        fftabs2=0;
        fft2=fft[k];
        for(i=0; i<channel; ++i,++fft2)
        {
            fftabs2+=fft2->a*fft2->a+fft2->b*fft2->b;
        }

        wtk_wbf_update_aspec(wbf,wbf->aspec,fft,fftabs2,k, specsum);
    }
    if(wbf->cfg->use_specsum_bl)
    {
        for(n=0; n<sang_num; ++n)
        {
            if(specsum[n]<=wbf->cfg->specsum_bl)
            {
                for(k=0;k<nbin;++k)
                {
                    cohv[n][k]=-1;
                }
            }
        }
    }
    wtk_wbf_flush(wbf, msg, cohv, is_end);
}

void wtk_wbf_on_stft2(wtk_wbf_t *wbf,wtk_stft2_msg_t *msg,int pos,int is_end)
{
    wtk_queue_t *stft2_q=&(wbf->stft2_q);
    int lt=wbf->cfg->lt;
    wtk_queue_node_t *qn;
    wtk_stft2_msg_t *smsg;
    int i;

    if(is_end)
    {
        wbf->end_pos=pos;
    }
    if(wbf->cov)
    {
        if(msg)
        {
            wtk_queue_push(stft2_q,&(msg->q_n));
        }
        if(stft2_q->length>=lt+1 && stft2_q->length<2*lt+1)
        {
            wtk_wbf_flush_aspec_lt(wbf,stft2_q->length-lt-1, 0);
        }else if(stft2_q->length==2*lt+1)
        {
            wtk_wbf_flush_aspec_lt(wbf,stft2_q->length-lt-1, (is_end && lt==0)?1: 0);
            qn=wtk_queue_pop(stft2_q);
            smsg=data_offset2(qn,wtk_stft2_msg_t,q_n);
            wtk_stft2_push_msg(wbf->stft2,smsg);
        }else if(is_end && stft2_q->length==0)
        {
            wtk_wbf_flush2(wbf, NULL, NULL, 1);
        }
        if(is_end)
        {
            if(stft2_q->length>0)
            {
                if(stft2_q->length<lt+1)
                {
                    for(i=0; i<stft2_q->length-1; ++i)
                    {
                        wtk_wbf_flush_aspec_lt(wbf, i, 0);
                    }
                    wtk_wbf_flush_aspec_lt(wbf, stft2_q->length-1, 1);
                }else
                {
                    for(i=0; i<lt-1; ++i)
                    {
                        wtk_wbf_flush_aspec_lt(wbf,stft2_q->length-lt+i, 0);   
                    }
                    wtk_wbf_flush_aspec_lt(wbf,stft2_q->length-1, 1);
                }
            }
            while(wbf->stft2_q.length>0)
            {
                qn=wtk_queue_pop(&(wbf->stft2_q));
                if(!qn){break;}
                smsg=(wtk_stft2_msg_t *)data_offset(qn,wtk_stft2_msg_t,q_n);
                wtk_stft2_push_msg(wbf->stft2,smsg);
            }
        }
    }else
    {
        if(msg)
        {
            wtk_wbf_flush_aspec(wbf,msg,is_end);
            wtk_stft2_push_msg(wbf->stft2, msg); 
        }else if(is_end && !msg)
        {
            wtk_wbf_flush2(wbf, NULL, NULL, 1);
        }
    }   
}

wtk_stft2_msg_t* wtk_wbf_stft2_msg_new(wtk_wbf_t *wbf)
{
	wtk_stft2_msg_t *msg;

	msg=(wtk_stft2_msg_t*)wtk_malloc(sizeof(wtk_stft2_msg_t));
	msg->hook=NULL;
	msg->fft=wtk_complex_new_p2(wbf->nbin,wbf->channel);
	return msg;
}

void wtk_wbf_stft2_msg_delete(wtk_wbf_t *wbf,wtk_stft2_msg_t *msg)
{
	wtk_complex_delete_p2(msg->fft,wbf->nbin);
	wtk_free(msg);
}

wtk_stft2_msg_t* wtk_wbf_pop_stft2_msg(wtk_wbf_t *wbf)
{
	return  (wtk_stft2_msg_t*)wtk_hoard_pop(&(wbf->msg_hoard));
}

void wtk_wbf_push_stft2_msg(wtk_wbf_t *wbf,wtk_stft2_msg_t *msg)
{
	wtk_hoard_push(&(wbf->msg_hoard),msg);
}

wtk_stft2_msg_t* wtk_wbf_stft2_msg_copy(wtk_wbf_t *wbf,wtk_stft2_msg_t *msg,int channel,int nbin)
{
	wtk_stft2_msg_t *vmsg;

	vmsg=wtk_wbf_pop_stft2_msg(wbf);
	vmsg->s=msg->s;
	wtk_complex_cpy_p2(vmsg->fft,msg->fft,nbin,channel);
	return vmsg;
}

wtk_wbf_t* wtk_wbf_new2(wtk_wbf_cfg_t *cfg, wtk_stft2_t *stft2)
{
    wtk_wbf_t *wbf;
    int i;

    wbf=(wtk_wbf_t *)wtk_malloc(sizeof(wtk_wbf_t));
    wbf->cfg=cfg;
    wbf->notify=NULL;
    wbf->notify2=NULL;
    wbf->ths=NULL;

    wbf->stft2=stft2;
    wbf->nbin=wbf->stft2->nbin;
    wbf->channel=cfg->stft2.channel;

    wbf->theta=(int *)wtk_malloc(sizeof(int)*cfg->wbf_cnt);
    wbf->bf=(wtk_bf_t **)wtk_malloc(sizeof(wtk_bf_t *)*cfg->wbf_cnt);
    wbf->qmmse=NULL;
    if(cfg->use_post)
    {
        wbf->qmmse=(wtk_qmmse_t **)wtk_malloc(sizeof(wtk_qmmse_t *)*cfg->wbf_cnt);
    }
    for(i=0;i<cfg->wbf_cnt;++i)
    {
        wbf->bf[i]=wtk_bf_new(&(cfg->bf),cfg->stft2.win);    
        if(wbf->qmmse)
        {
            wbf->qmmse[i]=wtk_qmmse_new(&(cfg->qmmse));
        }
    }
    if(cfg->use_line)
    {
        wbf->theta_step=floor(180.0/(cfg->wbf_cnt-1));   
    }else
    {
        wbf->theta_step=floor(359.0/cfg->wbf_cnt)+1;   
    }
    wbf->theta_range=wbf->theta_step/2*(cfg->range_interval+1);

    wbf->ncov=wtk_complex_new_p3(cfg->wbf_cnt,wbf->nbin,wbf->channel*wbf->channel);
    wbf->ncnt_sum=wtk_float_new_p2(cfg->wbf_cnt,wbf->nbin);

    wbf->scov=NULL;
    wbf->scnt_sum=NULL;
    if(cfg->bf.use_gev || cfg->bf.use_eig_ovec || cfg->bf.use_r1_mwf || cfg->bf.use_sdw_mwf || cfg->bf.use_vs)
    {
        wbf->scov=wtk_complex_new_p3(cfg->wbf_cnt,wbf->nbin,wbf->channel*wbf->channel);
        wbf->scnt_sum=wtk_float_new_p2(cfg->wbf_cnt,wbf->nbin);
    }

    wbf->aspec=wtk_aspec_new(&(cfg->aspec), wbf->nbin, cfg->wbf_cnt);      
  
    wbf->cov=NULL;
    wtk_queue_init(&(wbf->stft2_q));
    if(wbf->aspec && wbf->aspec->need_cov)
    {
        wbf->cov=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->stft2.channel*cfg->stft2.channel);
        if(cfg->lt<=0)
        {
            wbf->wint=wtk_malloc(sizeof(float));
            wbf->wint[0]=1;
        }else
        {
            wbf->wint=wtk_math_create_hanning_window(2*cfg->lt+1);
        }

        if(cfg->lf<=0)
        {
            wbf->winf=wtk_malloc(sizeof(float));
            wbf->winf[0]=1;
        }else
        {
            wbf->winf=wtk_math_create_hanning_window(2*cfg->lf+1);
        }
    }
    wbf->inv_cov=NULL;
    wbf->tmp=NULL;
    if(wbf->aspec && wbf->aspec->need_inv_cov)
    {
        wbf->inv_cov=(wtk_complex_t *)wtk_malloc(cfg->stft2.channel*cfg->stft2.channel*sizeof(wtk_complex_t));
        wbf->tmp=(wtk_dcomplex_t *)wtk_malloc(cfg->stft2.channel*cfg->stft2.channel*2*sizeof(wtk_dcomplex_t));
    }

    wbf->spec_k=(float *)wtk_malloc(sizeof(float)*cfg->wbf_cnt);

    wbf->cohv=wtk_float_new_p2(cfg->wbf_cnt,wbf->nbin);

    wtk_wbf_reset(wbf);

    return wbf;
}

void wtk_wbf_delete2(wtk_wbf_t *wbf)
{
    int i;

    for(i=0;i<wbf->cfg->wbf_cnt;++i)
    {
        wtk_bf_delete(wbf->bf[i]);
        if(wbf->qmmse)
        {
            wtk_qmmse_delete(wbf->qmmse[i]);
        }
    }
    wtk_free(wbf->bf);
    wtk_free(wbf->qmmse);

    wtk_complex_delete_p3(wbf->ncov, wbf->cfg->wbf_cnt, wbf->nbin);
    wtk_float_delete_p2(wbf->ncnt_sum, wbf->cfg->wbf_cnt);
    if(wbf->scov)
    {
        wtk_complex_delete_p3(wbf->scov, wbf->cfg->wbf_cnt, wbf->nbin);
        wtk_float_delete_p2(wbf->scnt_sum, wbf->cfg->wbf_cnt);
    }
    if(wbf->cov)
    {
        wtk_free(wbf->cov);
        wtk_free(wbf->wint);
        wtk_free(wbf->winf);
    }
    if(wbf->inv_cov)
    {
        wtk_free(wbf->inv_cov);
    }
    if(wbf->tmp)
    {
        wtk_free(wbf->tmp);
    }

    wtk_aspec_delete(wbf->aspec);

    wtk_free(wbf->spec_k);
    wtk_float_delete_p2(wbf->cohv, wbf->cfg->wbf_cnt);

    wtk_free(wbf->theta);

    wtk_free(wbf);
}

void wtk_wbf_reset2(wtk_wbf_t *wbf)
{
    int i;

    wbf->end_pos=0;
    for(i=0;i<wbf->cfg->wbf_cnt;++i)
    {
        wtk_bf_reset(wbf->bf[i]);
        if(wbf->qmmse)
        {
            wtk_qmmse_reset(wbf->qmmse[i]);
        }
    }

    memset(wbf->theta, 0, sizeof(float)*wbf->cfg->wbf_cnt);
    wtk_complex_zero_p3(wbf->ncov, wbf->cfg->wbf_cnt, wbf->nbin, wbf->channel*wbf->channel);
    wtk_float_zero_p2(wbf->ncnt_sum, wbf->cfg->wbf_cnt, wbf->nbin);
    if(wbf->scov)
    {
        wtk_complex_zero_p3(wbf->scov, wbf->cfg->wbf_cnt, wbf->nbin, wbf->channel*wbf->channel);
        wtk_float_zero_p2(wbf->scnt_sum, wbf->cfg->wbf_cnt, wbf->nbin);
    }

    wtk_aspec_reset(wbf->aspec);
}

void wtk_wbf_set_notify2(wtk_wbf_t *wbf,void *ths,wtk_wbf_notify_f2 notify)
{
    wbf->ths=ths;
    wbf->notify2=notify;
}

void wtk_wbf_start2(wtk_wbf_t *wbf)
{
    wtk_wbf_start(wbf);
}

void wtk_wbf_feed_smsg(wtk_wbf_t *wbf,wtk_stft2_msg_t *smsg2,int pos,int is_end)
{
    wtk_queue_t *stft2_q=&(wbf->stft2_q);
    int lt=wbf->cfg->lt;
    wtk_queue_node_t *qn;
    int i;
    wtk_stft2_msg_t *msg, *smsg;

    if(is_end)
    {
        wbf->end_pos=pos;
    }
    msg=NULL;
    if(smsg2)
    {
        msg=wtk_wbf_stft2_msg_copy(wbf, smsg2, wbf->channel, wbf->nbin);
    }

    if(wbf->cov)
    {
        if(msg)
        {
            wtk_queue_push(stft2_q,&(msg->q_n));
        }
        if(stft2_q->length>=lt+1 && stft2_q->length<2*lt+1)
        {
            wtk_wbf_flush_aspec_lt(wbf,stft2_q->length-lt-1, 0);
        }else if(stft2_q->length==2*lt+1)
        {
            wtk_wbf_flush_aspec_lt(wbf,stft2_q->length-lt-1, (is_end && lt==0)?1: 0);
            qn=wtk_queue_pop(stft2_q);
            smsg=data_offset2(qn,wtk_stft2_msg_t,q_n);
            wtk_wbf_push_stft2_msg(wbf, smsg);
        }else if(is_end && stft2_q->length==0)
        {
            wtk_wbf_flush2(wbf, NULL, NULL, 1);
        }
        if(is_end)
        {
            if(stft2_q->length>0)
            {
                if(stft2_q->length<lt+1)
                {
                    for(i=0; i<stft2_q->length-1; ++i)
                    {
                        wtk_wbf_flush_aspec_lt(wbf, i, 0);
                    }
                    wtk_wbf_flush_aspec_lt(wbf, stft2_q->length-1, 1);
                }else
                {
                    for(i=0; i<lt-1; ++i)
                    {
                        wtk_wbf_flush_aspec_lt(wbf,stft2_q->length-lt+i, 0);   
                    }
                    wtk_wbf_flush_aspec_lt(wbf,stft2_q->length-1, 1);
                }
            }
            while(wbf->stft2_q.length>0)
            {
                qn=wtk_queue_pop(&(wbf->stft2_q));
                if(!qn){break;}
                smsg=(wtk_stft2_msg_t *)data_offset(qn,wtk_stft2_msg_t,q_n);
                wtk_wbf_push_stft2_msg(wbf, smsg);
            }
        }
    }else
    {
        if(msg)
        {
            wtk_wbf_flush_aspec(wbf,msg,is_end);
            wtk_wbf_push_stft2_msg(wbf, msg);
        }else if(is_end && !msg)
        {
            wtk_wbf_flush2(wbf, NULL, NULL, 1);
        }
    } 
}

