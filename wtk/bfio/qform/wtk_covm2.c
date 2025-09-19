#include "wtk_covm2.h" 

wtk_covm2_ncovmsg_t *wtk_covm2_ncovmsg_new(int nchan)
{
    wtk_covm2_ncovmsg_t *ncovmsg;

    ncovmsg=(wtk_covm2_ncovmsg_t *)wtk_malloc(sizeof(wtk_covm2_ncovmsg_t));
    ncovmsg->ncov=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*nchan*nchan);
    ncovmsg->w=(float *)wtk_malloc(sizeof(float)*nchan);

    return ncovmsg;
}

void wtk_covm2_ncovmsg_delete(wtk_covm2_ncovmsg_t *ncovmsg)
{
    wtk_free(ncovmsg->ncov);
    wtk_free(ncovmsg);
}

wtk_covm2_scovmsg_t *wtk_covm2_scovmsg_new(int nchan)
{
    wtk_covm2_scovmsg_t *scovmsg;
    scovmsg=(wtk_covm2_scovmsg_t *)wtk_malloc(sizeof(wtk_covm2_scovmsg_t));
    scovmsg->scov=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*nchan*nchan);
    scovmsg->w=(float *)wtk_malloc(sizeof(float)*nchan);

    return scovmsg;
}

void wtk_covm2_scovmsg_delete(wtk_covm2_scovmsg_t *scovmsg)
{
    wtk_free(scovmsg->scov);
    wtk_free(scovmsg);
}

wtk_covm2_t* wtk_covm2_new(wtk_covm2_cfg_t *cfg, int nbin, int channel)
{
    wtk_covm2_t *covm2;
    int i;

    covm2=(wtk_covm2_t *)wtk_malloc(sizeof(wtk_covm2_t));
    covm2->cfg=cfg;

    covm2->nbin=nbin;
    covm2->channel=channel;

    covm2->ncov=wtk_complex_new_p2(nbin,channel*channel);
    covm2->ncnt_sum=(float *)wtk_malloc(sizeof(float)*nbin);

    covm2->scov=wtk_complex_new_p2(nbin,channel*channel);
    covm2->scnt_sum=(float *)wtk_malloc(sizeof(float)*nbin);

    covm2->qscov_q=(wtk_queue_t *)wtk_malloc(sizeof(wtk_queue_t)*nbin);
    for(i=0;i<nbin;++i)
    {
        wtk_queue_init(&(covm2->qscov_q[i]));
    }
    covm2->qncov_q=(wtk_queue_t *)wtk_malloc(sizeof(wtk_queue_t)*nbin);
    for(i=0;i<nbin;++i)
    {
        wtk_queue_init(&(covm2->qncov_q[i]));
    }

    wtk_covm2_reset(covm2);

    return covm2;
}

void wtk_covm2_delete(wtk_covm2_t *covm2)
{
    wtk_queue_node_t *qn;
    wtk_covm2_ncovmsg_t *qncmsg;
    wtk_covm2_scovmsg_t *qscmsg;
    int nbin=covm2->nbin;
    int i;

    if(covm2->qscov_q)
    {
        for(i=0;i<nbin;++i)
        {
            while(covm2->qscov_q[i].length>0)
            {
                qn=wtk_queue_pop(covm2->qscov_q+i);
                if(!qn){break;}
                qscmsg=(wtk_covm2_scovmsg_t *)data_offset(qn,wtk_covm2_scovmsg_t,q_n);
                wtk_covm2_scovmsg_delete(qscmsg);
            }
        }
        wtk_free(covm2->qscov_q);
    }
    if(covm2->qncov_q)
    {
        for(i=0;i<nbin;++i)
        {
            while(covm2->qncov_q[i].length>0)
            {
                qn=wtk_queue_pop(covm2->qncov_q+i);
                if(!qn){break;}
                qncmsg=(wtk_covm2_ncovmsg_t *)data_offset(qn,wtk_covm2_ncovmsg_t,q_n);
                wtk_covm2_ncovmsg_delete(qncmsg);
            }
        }
        wtk_free(covm2->qncov_q);
    }

    wtk_free(covm2->ncnt_sum);
    wtk_complex_delete_p2(covm2->ncov,nbin);

    wtk_free(covm2->scnt_sum);
    wtk_complex_delete_p2(covm2->scov,nbin);

    wtk_free(covm2);
}

void wtk_covm2_reset(wtk_covm2_t *covm2)
{
    int i;
    wtk_queue_node_t *qn;
    wtk_covm2_ncovmsg_t *qncmsg;
    wtk_covm2_scovmsg_t *qscmsg;
    int nbin=covm2->nbin;
    int channelx2=covm2->channel*covm2->channel;

    if(covm2->qscov_q)
    {
        for(i=0;i<nbin;++i)
        {
            while(covm2->qscov_q[i].length>0)
            {
                qn=wtk_queue_pop(covm2->qscov_q+i);
                if(!qn){break;}
                qscmsg=(wtk_covm2_scovmsg_t *)data_offset(qn,wtk_covm2_scovmsg_t,q_n);
                wtk_covm2_scovmsg_delete(qscmsg);
            }
        }
    }
    if(covm2->qncov_q)
    {
        for(i=0;i<nbin;++i)
        {
            while(covm2->qncov_q[i].length>0)
            {
                qn=wtk_queue_pop(covm2->qncov_q+i);
                if(!qn){break;}
                qncmsg=(wtk_covm2_ncovmsg_t *)data_offset(qn,wtk_covm2_ncovmsg_t,q_n);
                wtk_covm2_ncovmsg_delete(qncmsg);
            }
        }
    }

    memset(covm2->ncnt_sum,0,sizeof(float)*nbin);
    wtk_complex_zero_p2(covm2->ncov,nbin,channelx2);

    memset(covm2->scnt_sum,0,sizeof(float)*nbin);
    wtk_complex_zero_p2(covm2->scov,nbin,channelx2);
}

void wtk_covm2_update_scov(wtk_covm2_t *covm2, wtk_covm2_scovmsg_t *qscmsg2, wtk_complex_t *fft,int k, float *w, int channel)
{
    int i,j,n;
    wtk_complex_t *fft1,*fft2,*a,*b;
    wtk_complex_t *scov=covm2->scov[k];
    wtk_covm2_scovmsg_t *qscmsg;
    wtk_queue_node_t *qn;
    float w_sum;
    wtk_complex_t *cov;

    covm2->scnt_sum[k]=covm2->qscov_q[k].length;
    memcpy(qscmsg2->w, w, sizeof(float)*channel);
    cov=qscmsg2->scov;
    fft1=fft2=fft;
    for(i=0;i<channel;++i,++fft1)
    {
        fft2=fft1;
        for(j=i;j<channel;++j,++fft2)
        {
            a=cov+i*channel+j;
            a->a=fft1->a*fft2->a+fft1->b*fft2->b;
            a->b=-fft1->a*fft2->b+fft1->b*fft2->a;
        }
    }
    w_sum=0;
    memset(scov, 0, sizeof(wtk_complex_t)*channel*channel);
    for(n=0; n<covm2->qscov_q[k].length; ++n)
    {
        qn=wtk_queue_peek(&(covm2->qscov_q[k]), n);
        qscmsg=(wtk_covm2_scovmsg_t *)data_offset2(qn,wtk_covm2_scovmsg_t,q_n);
        cov=qscmsg->scov;
        w=qscmsg->w;
        for(i=0;i<channel;++i, ++w)
        {
            cov+=i;
            for(j=i;j<channel;++j,++cov)
            {
                if(i!=j)
                {
                    a=scov+i*channel+j;
                    a->a+=cov->a;
                    a->b+=cov->b;
                }else
                {
                    a=scov+i*channel+j;
                    a->a+=cov->a;
                    a->b=0;
                }
            }
            w_sum+=*w;
        }
    }
    for(i=0;i<channel;++i)
    {
        for(j=i;j<channel;++j)
        {
            if(i!=j)
            {
                a=scov+i*channel+j;
                a->a/=w_sum;
                a->b/=w_sum;

                b=scov+j*channel+i;
                b->a=a->a;
                b->b=-a->b;
            }else
            {
                a=scov+i*channel+j;
                a->a/=w_sum;
                a->b=0;
            }
        }
    }
}

void wtk_covm2_update_ncov(wtk_covm2_t *covm2, wtk_covm2_ncovmsg_t *qncmsg2, wtk_complex_t *fft,int k, float *w, int channel)
{
    int i,j,n;
    wtk_complex_t *fft1,*fft2,*a,*b;
    wtk_complex_t *ncov=covm2->ncov[k];
    wtk_covm2_ncovmsg_t *qncmsg;
    wtk_queue_node_t *qn;
    float w_sum;
    wtk_complex_t *cov;
    float eye=covm2->cfg->eye;

    covm2->scnt_sum[k]=covm2->qncov_q[k].length;
    memcpy(qncmsg2->w, w, sizeof(float)*channel);
    cov=qncmsg2->ncov;
    fft1=fft2=fft;
    for(i=0;i<channel;++i,++fft1)
    {
        fft2=fft1;
        for(j=i;j<channel;++j,++fft2)
        {
            a=cov+i*channel+j;
            a->a=(fft1->a*fft2->a+fft1->b*fft2->b)*w[j];
            a->b=(-fft1->a*fft2->b+fft1->b*fft2->a)*w[j];
        }
    }
    w_sum=0;
    memset(ncov, 0, sizeof(wtk_complex_t)*channel*channel);
    for(n=0; n<covm2->qncov_q[k].length; ++n)
    {
        qn=wtk_queue_peek(&(covm2->qncov_q[k]), n);
        qncmsg=(wtk_covm2_ncovmsg_t *)data_offset2(qn,wtk_covm2_ncovmsg_t,q_n);
        cov=qncmsg->ncov;
        w=qncmsg->w;
        for(i=0;i<channel;++i, ++w)
        {
            cov+=i;
            for(j=i;j<channel;++j,++cov)
            {
                if(i!=j)
                {
                    a=ncov+i*channel+j;
                    a->a+=cov->a;
                    a->b+=cov->b;
                }else
                {
                    a=ncov+i*channel+j;
                    a->a+=cov->a;
                    a->b=0;
                }
            }
            w_sum+=*w;
        }
    }

    for(i=0;i<channel;++i)
    {
        for(j=i;j<channel;++j)
        {
            if(i!=j)
            {
                a=ncov+i*channel+j;
                a->a/=w_sum;
                a->b/=w_sum;

                b=ncov+j*channel+i;
                b->a=a->a;
                b->b=-a->b;
            }else
            {
                a=ncov+i*channel+j;
                a->a/=w_sum;
                a->a+=eye;
                a->b=0;
            }
        }
    }
}

int wtk_covm2_feed_fft(wtk_covm2_t *covm2,wtk_complex_t *fft, int k, float *w, int is_ncov)
{
    int channel=covm2->channel;
    wtk_covm2_ncovmsg_t *qncmsg;
    wtk_covm2_scovmsg_t *qscmsg;
    wtk_queue_node_t *qn;

    if(is_ncov)
    {
        if(covm2->qncov_q[k].length == covm2->cfg->cov_hist)
        {
            qn=wtk_queue_pop(&(covm2->qncov_q[k]));
            qncmsg=(wtk_covm2_ncovmsg_t *)data_offset2(qn,wtk_covm2_ncovmsg_t,q_n);
            wtk_covm2_ncovmsg_delete(qncmsg);
        }

        qncmsg=wtk_covm2_ncovmsg_new(channel);
        wtk_queue_push(&(covm2->qncov_q[k]), &(qncmsg->q_n));
        wtk_covm2_update_ncov(covm2, qncmsg, fft, k , w, channel);
    }else
    {
        if(covm2->scov)
        {
            if(covm2->qscov_q[k].length == covm2->cfg->cov_hist)
            {        
                qn=wtk_queue_pop(&(covm2->qscov_q[k]));
                qscmsg=(wtk_covm2_scovmsg_t *)data_offset2(qn,wtk_covm2_scovmsg_t,q_n);
                wtk_covm2_scovmsg_delete(qscmsg);
            }

            qscmsg=wtk_covm2_scovmsg_new(channel);
            wtk_queue_push(&(covm2->qscov_q[k]), &(qscmsg->q_n));
            wtk_covm2_update_scov(covm2, qscmsg, fft, k , w, channel);
        }
    }

    return 0;
}





