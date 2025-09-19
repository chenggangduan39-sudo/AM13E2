#include "wtk_covm.h" 

wtk_covm_ncovmsg_t *wtk_covm_ncovmsg_new(wtk_covm_t *covm)
{
    wtk_covm_ncovmsg_t *ncovmsg;
    int nchan=covm->channel;

    ncovmsg=(wtk_covm_ncovmsg_t *)wtk_malloc(sizeof(wtk_covm_ncovmsg_t));
    ncovmsg->ncov=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*nchan*nchan);

    return ncovmsg;
}

void wtk_covm_ncovmsg_delete(wtk_covm_t *covm, wtk_covm_ncovmsg_t *ncovmsg)
{
    wtk_free(ncovmsg->ncov);
    wtk_free(ncovmsg);
}

wtk_covm_ncovmsg_t *wtk_covm_pop_ncov_msg(wtk_covm_t *covm)
{
    return (wtk_covm_ncovmsg_t *)wtk_hoard_pop(&(covm->ncov_hoard));
}

void wtk_covm_push_ncov_msg(wtk_covm_t *covm, wtk_covm_ncovmsg_t *msg)
{
    wtk_hoard_push(&(covm->ncov_hoard),msg);
}

wtk_covm_scovmsg_t *wtk_covm_scovmsg_new(wtk_covm_t *covm)
{
    wtk_covm_scovmsg_t *scovmsg;
    int nchan=covm->channel;
    scovmsg=(wtk_covm_scovmsg_t *)wtk_malloc(sizeof(wtk_covm_scovmsg_t));
    scovmsg->scov=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*nchan*nchan);

    return scovmsg;
}

void wtk_covm_scovmsg_delete(wtk_covm_t *covm, wtk_covm_scovmsg_t *scovmsg)
{
    wtk_free(scovmsg->scov);
    wtk_free(scovmsg);
}

wtk_covm_scovmsg_t *wtk_covm_pop_scov_msg(wtk_covm_t *covm)
{
    return (wtk_covm_scovmsg_t *)wtk_hoard_pop(&(covm->scov_hoard));
}

void wtk_covm_push_scov_msg(wtk_covm_t *covm, wtk_covm_scovmsg_t *msg)
{
    wtk_hoard_push(&(covm->scov_hoard),msg);
}

wtk_covm_t* wtk_covm_new(wtk_covm_cfg_t *cfg, int nbin, int channel)
{
    wtk_covm_t *covm;
    int i;

    covm=(wtk_covm_t *)wtk_malloc(sizeof(wtk_covm_t));
    covm->cfg=cfg;

    covm->nbin=nbin;
    covm->channel=channel;

    covm->ncov=wtk_complex_new_p2(nbin,channel*channel);
    covm->ncovtmp=wtk_complex_new_p2(nbin,channel*channel);
    covm->ncnt_sum=(float *)wtk_malloc(sizeof(float)*nbin);
    covm->n_mask=(float *)wtk_malloc(sizeof(float)*nbin);

    wtk_hoard_init2(&(covm->ncov_hoard),offsetof(wtk_covm_ncovmsg_t,hoard_n),10,
        (wtk_new_handler_t)wtk_covm_ncovmsg_new,
        (wtk_delete_handler2_t)wtk_covm_ncovmsg_delete,
        covm);

    covm->scov=NULL;
    covm->scovtmp=NULL;
    covm->scnt_sum=NULL;
    if(cfg->use_scov)
    {
        wtk_hoard_init2(&(covm->scov_hoard),offsetof(wtk_covm_ncovmsg_t,hoard_n),10,
            (wtk_new_handler_t)wtk_covm_scovmsg_new,
            (wtk_delete_handler2_t)wtk_covm_scovmsg_delete,
            covm);
        covm->scov=wtk_complex_new_p2(nbin,channel*channel);
        covm->scovtmp=wtk_complex_new_p2(nbin,channel*channel);
        covm->scnt_sum=(float *)wtk_malloc(sizeof(float)*nbin);
        covm->s_mask=(float *)wtk_malloc(sizeof(float)*nbin);
    }

    covm->qscov_q=covm->qncov_q=NULL;
    if(cfg->use_covhist)
    {
        if(covm->scov)
        {
            covm->qscov_q=(wtk_queue_t *)wtk_malloc(sizeof(wtk_queue_t)*nbin);
            for(i=0;i<nbin;++i)
            {
                wtk_queue_init(&(covm->qscov_q[i]));
            }
        }
        covm->qncov_q=(wtk_queue_t *)wtk_malloc(sizeof(wtk_queue_t)*nbin);
        for(i=0;i<nbin;++i)
        {
            wtk_queue_init(&(covm->qncov_q[i]));
        }
    }else if(cfg->ncov_flush_delay>0)
    {
        covm->qncov_q=(wtk_queue_t *)wtk_malloc(sizeof(wtk_queue_t)*nbin);
        for(i=0;i<nbin;++i)
        {
            wtk_queue_init(&(covm->qncov_q[i]));
        }
    }

    wtk_covm_reset(covm);

    return covm;
}

void wtk_covm_delete(wtk_covm_t *covm)
{
    int nbin=covm->nbin;

	wtk_hoard_clean(&(covm->ncov_hoard));
    if(covm->scov){
    	wtk_hoard_clean(&(covm->scov_hoard));
    }
    if(covm->qscov_q)
    {
        wtk_free(covm->qscov_q);
    }
    if(covm->qncov_q)
    {
        wtk_free(covm->qncov_q);
    }

    wtk_free(covm->ncnt_sum);
    wtk_free(covm->n_mask);
    wtk_complex_delete_p2(covm->ncovtmp,nbin);
    wtk_complex_delete_p2(covm->ncov,nbin);

    if(covm->scnt_sum)
    {
        wtk_free(covm->scnt_sum);
        wtk_free(covm->s_mask);
        wtk_complex_delete_p2(covm->scovtmp,nbin);
        wtk_complex_delete_p2(covm->scov,nbin);
    }

    wtk_free(covm);
}

void wtk_covm_reset(wtk_covm_t *covm)
{
    int i;
    wtk_queue_node_t *qn;
    wtk_covm_ncovmsg_t *qncmsg;
    wtk_covm_scovmsg_t *qscmsg;
    int nbin=covm->nbin;
    int channelx2=covm->channel*covm->channel;

    if(covm->qscov_q)
    {
        for(i=0;i<nbin;++i)
        {
            while(covm->qscov_q[i].length>0)
            {
                qn=wtk_queue_pop(covm->qscov_q+i);
                if(!qn){break;}
                qscmsg=(wtk_covm_scovmsg_t *)data_offset(qn,wtk_covm_scovmsg_t,q_n);
                wtk_covm_push_scov_msg(covm, qscmsg);
            }
        }
    }
    if(covm->qncov_q)
    {
        for(i=0;i<nbin;++i)
        {
            while(covm->qncov_q[i].length>0)
            {
                qn=wtk_queue_pop(covm->qncov_q+i);
                if(!qn){break;}
                qncmsg=(wtk_covm_ncovmsg_t *)data_offset(qn,wtk_covm_ncovmsg_t,q_n);
                wtk_covm_push_ncov_msg(covm, qncmsg);
            }
        }
    }

    memset(covm->ncnt_sum,0,sizeof(int)*nbin);
    memset(covm->n_mask,0,sizeof(float)*nbin);
    wtk_complex_zero_p2(covm->ncovtmp,nbin,channelx2);
    wtk_complex_zero_p2(covm->ncov,nbin,channelx2);

    if(covm->scnt_sum)
    {
        memset(covm->scnt_sum,0,sizeof(int)*nbin);
        memset(covm->s_mask,0,sizeof(float)*nbin);
        wtk_complex_zero_p2(covm->scovtmp,nbin,channelx2);
        wtk_complex_zero_p2(covm->scov,nbin,channelx2);
    }
}

void wtk_covm_update_srnn2(wtk_covm_t *covm, wtk_complex_t **fft,int k)
{
    int channel=covm->channel;
    int i,j;
    wtk_complex_t *fft1,*fft2,*a,*b;
    wtk_complex_t **scov=covm->scov;
    wtk_complex_t **scovtmp=covm->scovtmp;
    float alpha=covm->cfg->scov_alpha;
    float alpha_1=1-covm->cfg->scov_alpha;

    if(covm->scov==NULL)
    {
        return;
    }

    if(covm->scnt_sum[k]>=covm->cfg->init_scovnf)
    {
        ++covm->scnt_sum[k];
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
        ++covm->scnt_sum[k];
        fft1=fft2=fft[k];
        for(i=0;i<channel;++i,++fft1)
        {
            fft2=fft1;
            for(j=i;j<channel;++j,++fft2)
            {
                a=scovtmp[k]+i*channel+j;
                a->a+=fft1->a*fft2->a+fft1->b*fft2->b;
                a->b+=-fft1->a*fft2->b+fft1->b*fft2->a;

                b=scov[k]+i*channel+j;
                b->a=a->a/covm->scnt_sum[k];
                b->b=a->b/covm->scnt_sum[k];
            }
        }
    }
}


void wtk_covm_update_nrnn2(wtk_covm_t *covm,wtk_complex_t **fft,int k)
{
    int channel=covm->channel;
    int i,j;
    wtk_complex_t *fft1,*fft2,*a,*b;
    wtk_complex_t **ncov=covm->ncov;
    wtk_complex_t **ncovtmp=covm->ncovtmp;
    float alpha=covm->cfg->ncov_alpha;
    float alpha_1=1-covm->cfg->ncov_alpha;

    if(covm->ncnt_sum[k]>=covm->cfg->init_ncovnf)
    {
        ++covm->ncnt_sum[k];
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
        ++covm->ncnt_sum[k];
        fft1=fft2=fft[k];
        for(i=0;i<channel;++i,++fft1)
        {
            fft2=fft1;
            for(j=i;j<channel;++j,++fft2)
            {
                a=ncovtmp[k]+i*channel+j;
                a->a+=fft1->a*fft2->a+fft1->b*fft2->b;
                a->b+=-fft1->a*fft2->b+fft1->b*fft2->a;

                b=ncov[k]+i*channel+j;
                b->a=a->a/covm->ncnt_sum[k];
                b->b=a->b/covm->ncnt_sum[k];
            }
        }
    }
}

void wtk_covm_update_cov2(wtk_covm_t *covm, wtk_complex_t *cov, wtk_complex_t **fft,int k, int channel, int is_ncov)
{
    int i,j;
    wtk_complex_t *fft1,*fft2,*a,*b,*c;
    wtk_complex_t *ncov;
    wtk_complex_t *scov;
    wtk_complex_t *ncovtmp;
    wtk_complex_t *scovtmp;
    int cnt;


    fft1=fft2=fft[k];
    if(is_ncov)
    {
        ncov=covm->ncov[k];
        ncovtmp=covm->ncovtmp[k];
        cnt=covm->qncov_q[k].length;
        covm->ncnt_sum[k]=cnt;
        for(i=0;i<channel;++i,++fft1)
        {
            fft2=fft1;
            for(j=i;j<channel;++j,++fft2)
            {
                a=cov+i*channel+j;
                a->a=fft1->a*fft2->a+fft1->b*fft2->b;
                a->b=-fft1->a*fft2->b+fft1->b*fft2->a;

                c=ncovtmp+i*channel+j;
                c->a+=a->a;
                c->b+=a->b;

                b=ncov+i*channel+j;
                b->a=c->a/cnt;
                b->b=c->b/cnt;
            }
        }
    }else
    {
        scov=covm->scov[k];
        scovtmp=covm->scovtmp[k];
        cnt=covm->qscov_q[k].length;
        covm->scnt_sum[k]=cnt;
        for(i=0;i<channel;++i,++fft1)
        {
            fft2=fft1;
            for(j=i;j<channel;++j,++fft2)
            {
                a=cov+i*channel+j;
                a->a=fft1->a*fft2->a+fft1->b*fft2->b;
                a->b=-fft1->a*fft2->b+fft1->b*fft2->a;

                c=scovtmp+i*channel+j;
                c->a+=a->a;
                c->b+=a->b;

                b=scov+i*channel+j;
                b->a=c->a/cnt;
                b->b=c->b/cnt;
            }
        }
    }
}


void wtk_covm_flush_covmsgq(wtk_covm_t *covm, int k, int is_ncov)
{
    int channel=covm->channel;
    wtk_covm_ncovmsg_t *qncmsg;
    wtk_covm_scovmsg_t *qscmsg;
    wtk_queue_node_t *qn;
    int i,j;
    wtk_complex_t *cov, *a, *b;
    wtk_complex_t *ncovtmp;
    wtk_complex_t *scovtmp;

    if(is_ncov)
    {
        ncovtmp=covm->ncovtmp[k];
        qn=wtk_queue_pop(&(covm->qncov_q[k]));
        qncmsg=(wtk_covm_ncovmsg_t *)data_offset2(qn,wtk_covm_ncovmsg_t,q_n);
        cov=qncmsg->ncov;
        for(i=0;i<channel;++i)
        {
            for(j=i;j<channel;++j)
            {
                a=cov+i*channel+j;
                b=ncovtmp+i*channel+j;
                b->a-=a->a;
                b->b-=a->b;
            }
        }
        wtk_covm_push_ncov_msg(covm, qncmsg);
    }else
    {
        scovtmp=covm->scovtmp[k];
        qn=wtk_queue_pop(&(covm->qscov_q[k]));
        qscmsg=(wtk_covm_scovmsg_t *)data_offset2(qn,wtk_covm_scovmsg_t,q_n);
        cov=qscmsg->scov;
        for(i=0;i<channel;++i)
        {
            for(j=i;j<channel;++j)
            {
                a=cov+i*channel+j;
                b=scovtmp+i*channel+j;
                b->a-=a->a;
                b->b-=a->b;
            }
        }
        wtk_covm_push_scov_msg(covm, qscmsg);
    }
}

// fft: nbin*channel
int wtk_covm_feed_fft2(wtk_covm_t *covm,wtk_complex_t **fft, int k, int is_ncov)
{
    int channel=covm->channel;
    wtk_covm_ncovmsg_t *qncmsg;
    wtk_covm_scovmsg_t *qscmsg;
    int ret;

    if(covm->cfg->use_covhist)
    {
        if(is_ncov)
        {
            if(covm->qncov_q[k].length == covm->cfg->ncov_hist)
            {
                wtk_covm_flush_covmsgq(covm, k, 1);
            }

            qncmsg=wtk_covm_pop_ncov_msg(covm);
            wtk_queue_push(&(covm->qncov_q[k]), &(qncmsg->q_n));
            wtk_covm_update_cov2(covm, qncmsg->ncov, fft, k ,channel, 1);
        }else
        {
            if(covm->scov)
            {
                if(covm->qscov_q[k].length == covm->cfg->scov_hist)
                {
                    wtk_covm_flush_covmsgq(covm, k, 0);
                }
                qscmsg=wtk_covm_pop_scov_msg(covm);
                wtk_queue_push(&(covm->qscov_q[k]), &(qscmsg->q_n));
                wtk_covm_update_cov2(covm, qscmsg->scov, fft, k ,channel, 0);
            }
        }
    }else
    {
        if(is_ncov)
        {
            wtk_covm_update_nrnn2(covm,fft,k);
        }else
        {
            if(covm->scov)
            {
                wtk_covm_update_srnn2(covm,fft,k);
            }
        }
    }
    
    ret=0;
    if(is_ncov && covm->ncnt_sum[k]>0)
    {
        ret=1;
    }else if(!is_ncov && (covm->scov && covm->scnt_sum[k]>0))
    {
        ret=1;   
    }
    return ret;
}



void wtk_covm_update_srnn3(wtk_covm_t *covm, wtk_complex_t *fft,int k)
{
    int channel=covm->channel;
    int i,j;
    wtk_complex_t *fft1,*fft2,*a,*b;
    wtk_complex_t **scov=covm->scov;
    wtk_complex_t **scovtmp=covm->scovtmp;
    float alpha=covm->cfg->scov_alpha;
    float alpha_1=1-covm->cfg->scov_alpha;

    if(covm->scov==NULL)
    {
        return;
    }

    if(covm->scnt_sum[k]>=covm->cfg->init_scovnf)
    {
        ++covm->scnt_sum[k];
        fft1=fft2=fft;
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
        ++covm->scnt_sum[k];
        fft1=fft2=fft;
        for(i=0;i<channel;++i,++fft1)
        {
            fft2=fft1;
            for(j=i;j<channel;++j,++fft2)
            {
                a=scovtmp[k]+i*channel+j;
                a->a+=fft1->a*fft2->a+fft1->b*fft2->b;
                a->b+=-fft1->a*fft2->b+fft1->b*fft2->a;

                b=scov[k]+i*channel+j;
                b->a=a->a/covm->scnt_sum[k];
                b->b=a->b/covm->scnt_sum[k];
            }
        }
    }
}


void wtk_covm_update_nrnn3(wtk_covm_t *covm,wtk_complex_t *fft,int k)
{
    int channel=covm->channel;
    int i,j;
    wtk_complex_t *fft1,*fft2,*a,*b;
    wtk_complex_t **ncov=covm->ncov;
    wtk_complex_t **ncovtmp=covm->ncovtmp;
    float alpha=covm->cfg->ncov_alpha;
    float alpha_1=1-covm->cfg->ncov_alpha;

    if(covm->ncnt_sum[k]>=covm->cfg->init_ncovnf)
    {
        ++covm->ncnt_sum[k];
        fft1=fft2=fft;
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
        ++covm->ncnt_sum[k];
        fft1=fft2=fft;
        for(i=0;i<channel;++i,++fft1)
        {
            fft2=fft1;
            for(j=i;j<channel;++j,++fft2)
            {
                a=ncovtmp[k]+i*channel+j;
                a->a+=fft1->a*fft2->a+fft1->b*fft2->b;
                a->b+=-fft1->a*fft2->b+fft1->b*fft2->a;

                b=ncov[k]+i*channel+j;
                b->a=a->a/covm->ncnt_sum[k];
                b->b=a->b/covm->ncnt_sum[k];
            }
        }
    }
}

void wtk_covm_update_srnn4(wtk_covm_t *covm, wtk_complex_t *fft,int k,float scale)
{
    int channel=covm->channel;
    int i,j;
    wtk_complex_t *fft1,*fft2,*a,*b;
    wtk_complex_t **scov=covm->scov;
    float alpha=covm->cfg->scov_alpha;
    float alpha_1=1-covm->cfg->scov_alpha;

    if(covm->scov==NULL)
    {
        return;
    }

    if(covm->scnt_sum[k]>=covm->cfg->init_scovnf)
    {
        ++covm->scnt_sum[k];
        fft1=fft2=fft;
        for(i=0;i<channel;++i,++fft1)
        {
            fft2=fft1;
            for(j=i;j<channel;++j,++fft2)
            {
                a=scov[k]+i*channel+j;
                a->a=alpha_1*a->a+alpha*(fft1->a*fft2->a+fft1->b*fft2->b)*scale;
                a->b=alpha_1*a->b+alpha*(-fft1->a*fft2->b+fft1->b*fft2->a)*scale;
            }
        }
        covm->s_mask[k] = alpha_1 * covm->s_mask[k] + alpha * scale;
    }else
    {
        ++covm->scnt_sum[k];
        fft1=fft2=fft;
        for(i=0;i<channel;++i,++fft1)
        {
            fft2=fft1;
            for(j=i;j<channel;++j,++fft2)
            {
                a=scov[k]+i*channel+j;
                a->a+=(fft1->a*fft2->a+fft1->b*fft2->b)*scale;
                a->b+=(-fft1->a*fft2->b+fft1->b*fft2->a)*scale;
            }
        }
        covm->s_mask[k] += scale;
    }
    fft1=fft2=fft;
    for(i=0;i<channel;++i,++fft1)
    {
        fft2=fft1;
        for(j=i;j<channel;++j,++fft2)
        {
            a=scov[k]+i*channel+j;
            b=scov[k]+j*channel+i;
            b->a=a->a;
            b->b=-a->b;
        }
    }
}


void wtk_covm_update_nrnn4(wtk_covm_t *covm,wtk_complex_t *fft,int k,float scale)
{
    int channel=covm->channel;
    int i,j;
    wtk_complex_t *fft1,*fft2,*a,*b;
    wtk_complex_t **ncov=covm->ncov;
    float alpha=covm->cfg->ncov_alpha;
    float alpha_1=1-covm->cfg->ncov_alpha;

    if(covm->ncnt_sum[k]>=covm->cfg->init_ncovnf)
    {
        ++covm->ncnt_sum[k];
        fft1=fft2=fft;
        for(i=0;i<channel;++i,++fft1)
        {
            fft2=fft1;
            for(j=i;j<channel;++j,++fft2)
            {
                a=ncov[k]+i*channel+j;
                a->a=alpha_1*a->a+alpha*(fft1->a*fft2->a+fft1->b*fft2->b)*scale;
                a->b=alpha_1*a->b+alpha*(-fft1->a*fft2->b+fft1->b*fft2->a)*scale;
            }
        }
        covm->n_mask[k] =  alpha_1 * covm->n_mask[k] + alpha * scale;
    }else
    {
        ++covm->ncnt_sum[k];
        fft1=fft2=fft;
        for(i=0;i<channel;++i,++fft1)
        {
            fft2=fft1;
            for(j=i;j<channel;++j,++fft2)
            {
                a=ncov[k]+i*channel+j;
                a->a+=(fft1->a*fft2->a+fft1->b*fft2->b)*scale;
                a->b+=(-fft1->a*fft2->b+fft1->b*fft2->a)*scale;
            }
        }
        covm->n_mask[k] += scale;
    }

    fft1=fft2=fft;
    for(i=0;i<channel;++i,++fft1)
    {
        fft2=fft1;
        for(j=i;j<channel;++j,++fft2)
        {
            a=ncov[k]+i*channel+j;
            b=ncov[k]+j*channel+i;
            b->a=a->a;
            b->b=-a->b;
        }
    }
}

void wtk_covm_update_delay_nrnn3(wtk_covm_t *covm,int k)
{
    int channel=covm->channel;
    int i,j;
    wtk_complex_t *fft1,*fft2,*a,*b,*c;
    wtk_complex_t **ncov=covm->ncov;
    wtk_complex_t **ncovtmp=covm->ncovtmp;
    wtk_complex_t *cov;
    float alpha=covm->cfg->ncov_alpha;
    float alpha_1=1-covm->cfg->ncov_alpha;
    wtk_covm_ncovmsg_t *qncmsg;
    wtk_queue_node_t *qn;

    qn=wtk_queue_pop(&(covm->qncov_q[k]));
    qncmsg=(wtk_covm_ncovmsg_t *)data_offset2(qn,wtk_covm_ncovmsg_t,q_n);
    cov=qncmsg->ncov;
    if(covm->ncnt_sum[k]>=covm->cfg->init_ncovnf)
    {
        ++covm->ncnt_sum[k];
        for(i=0;i<channel;++i,++fft1)
        {
            fft2=fft1;
            for(j=i;j<channel;++j,++fft2)
            {
                a=ncov[k]+i*channel+j;
                b=cov+i*channel+j;
                a->a=alpha_1*a->a+alpha*b->a;
                a->b=alpha_1*a->b+alpha*b->b;
            }
        }
    }else
    {
        ++covm->ncnt_sum[k];
        for(i=0;i<channel;++i,++fft1)
        {
            fft2=fft1;
            for(j=i;j<channel;++j,++fft2)
            {
                a=ncovtmp[k]+i*channel+j;
                b=cov+i*channel+j;
                a->a+=b->a;
                a->b+=b->b;

                c=ncov[k]+i*channel+j;
                c->a=a->a/covm->ncnt_sum[k];
                c->b=a->b/covm->ncnt_sum[k];
            }
        }
    }
    wtk_covm_push_ncov_msg(covm, qncmsg);
}


void wtk_covm_update_delay_ncov3(wtk_covm_t *covm, wtk_complex_t *cov, wtk_complex_t *fft,int k, int channel)
{
    int i,j;
    wtk_complex_t *fft1,*fft2,*a;

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
}


void wtk_covm_update_cov3(wtk_covm_t *covm, wtk_complex_t *cov, wtk_complex_t *fft,int k, int channel, int is_ncov)
{
    int i,j;
    wtk_complex_t *fft1,*fft2,*a,*b,*c;
    wtk_complex_t *ncov=covm->ncov[k];
    wtk_complex_t *scov=covm->scov[k];
    wtk_complex_t *ncovtmp=covm->ncovtmp[k];
    wtk_complex_t *scovtmp=covm->scovtmp[k];
    int cnt;


    fft1=fft2=fft;
    if(is_ncov)
    {
        cnt=covm->qncov_q[k].length;
        covm->ncnt_sum[k]=cnt;
        for(i=0;i<channel;++i,++fft1)
        {
            fft2=fft1;
            for(j=i;j<channel;++j,++fft2)
            {
                a=cov+i*channel+j;
                a->a=fft1->a*fft2->a+fft1->b*fft2->b;
                a->b=-fft1->a*fft2->b+fft1->b*fft2->a;

                c=ncovtmp+i*channel+j;
                c->a+=a->a;
                c->b+=a->b;

                b=ncov+i*channel+j;
                b->a=c->a/cnt;
                b->b=c->b/cnt;
            }
        }
    }else
    {
        cnt=covm->qscov_q[k].length;
        covm->scnt_sum[k]=cnt;
        for(i=0;i<channel;++i,++fft1)
        {
            fft2=fft1;
            for(j=i;j<channel;++j,++fft2)
            {
                a=cov+i*channel+j;
                a->a=fft1->a*fft2->a+fft1->b*fft2->b;
                a->b=-fft1->a*fft2->b+fft1->b*fft2->a;

                c=scovtmp+i*channel+j;
                c->a+=a->a;
                c->b+=a->b;

                b=scov+i*channel+j;
                b->a=c->a/cnt;
                b->b=c->b/cnt;
            }
        }
    }
}

int wtk_covm_feed_fft3(wtk_covm_t *covm,wtk_complex_t *fft, int k, int is_ncov)
{
    int channel=covm->channel;
    wtk_covm_ncovmsg_t *qncmsg;
    wtk_covm_scovmsg_t *qscmsg;
    int ret;

    if(covm->cfg->use_covhist)
    {
        if(is_ncov)
        {
            if(covm->qncov_q[k].length == covm->cfg->ncov_hist)
            {
                wtk_covm_flush_covmsgq(covm, k, 1);
            }
            qncmsg=wtk_covm_pop_ncov_msg(covm);
            wtk_queue_push(&(covm->qncov_q[k]), &(qncmsg->q_n));
            wtk_covm_update_cov3(covm, qncmsg->ncov, fft, k ,channel, 1);
        }else
        {
            if(covm->scov)
            {
                if(covm->qscov_q[k].length == covm->cfg->scov_hist)
                {
                    wtk_covm_flush_covmsgq(covm, k, 0);
                }
                qscmsg=wtk_covm_pop_scov_msg(covm);
                wtk_queue_push(&(covm->qscov_q[k]), &(qscmsg->q_n));
                wtk_covm_update_cov3(covm, qscmsg->scov, fft, k ,channel, 0);
            }
        }
    }else
    {
        if(is_ncov)
        {
            if(covm->cfg->ncov_flush_delay>0)
            {
                qncmsg=wtk_covm_pop_ncov_msg(covm);
                wtk_queue_push(&(covm->qncov_q[k]), &(qncmsg->q_n));
                wtk_covm_update_delay_ncov3(covm, qncmsg->ncov, fft, k ,channel);
                if(covm->qncov_q[k].length == covm->cfg->ncov_flush_delay)
                {
                    wtk_covm_update_delay_nrnn3(covm,k);
                }
            }else
            {
                wtk_covm_update_nrnn3(covm,fft,k);
            }
        }else
        {
            if(covm->scov)
            {
                wtk_covm_update_srnn3(covm,fft,k);
            }
        }
    }
    ret=0;
    if(is_ncov && covm->ncnt_sum[k]>0)
    {
        ret=1;
    }else if(!is_ncov && (covm->scov && covm->scnt_sum[k]>0))
    {
        ret=1;   
    }
    return ret;
}


int wtk_covm_feed_fft4(wtk_covm_t *covm,wtk_complex_t *fft, int k, int is_ncov, float scale)
{
    int ret;

    if(is_ncov)
    {
        wtk_covm_update_nrnn4(covm,fft,k,scale);
    }else
    {
        if(covm->scov)
        {
            wtk_covm_update_srnn4(covm,fft,k,scale);
        }
    }
    ret=0;
    if(is_ncov && covm->ncnt_sum[k]>0)
    {
        ret=1;
    }else if(!is_ncov && (covm->scov && covm->scnt_sum[k]>0))
    {
        ret=1;   
    }
    return ret;
}




void wtk_covm_update_srnn(wtk_covm_t *covm, wtk_complex_t **fft,int k)
{
    int channel=covm->channel;
    int i,j;
    wtk_complex_t *fft1,*fft2,*a,*b;
    wtk_complex_t **scov=covm->scov;
    wtk_complex_t **scovtmp=covm->scovtmp;
    float alpha=covm->cfg->scov_alpha;
    float alpha_1=1-covm->cfg->scov_alpha;

    if(covm->scov==NULL)
    {
        return;
    }

    if(covm->scnt_sum[k]>=covm->cfg->init_scovnf)
    {
        ++covm->scnt_sum[k];
        for(i=0;i<channel;++i)
        {
            fft1=fft[i];
            for(j=i;j<channel;++j)
            {
                fft2=fft[j];

                a=scov[k]+i*channel+j;
                a->a=alpha_1*a->a+alpha*(fft1[k].a*fft2[k].a+fft1[k].b*fft2[k].b);
                a->b=alpha_1*a->b+alpha*(-fft1[k].a*fft2[k].b+fft1[k].b*fft2[k].a);
            }
        }
    }else
    {
        ++covm->scnt_sum[k];
        for(i=0;i<channel;++i)
        {
            fft1=fft[i];
            for(j=i;j<channel;++j)
            {
                fft2=fft[j];

                a=scovtmp[k]+i*channel+j;
                a->a+=fft1[k].a*fft2[k].a+fft1[k].b*fft2[k].b;
                a->b+=-fft1[k].a*fft2[k].b+fft1[k].b*fft2[k].a;

                b=scov[k]+i*channel+j;
                b->a=a->a/covm->scnt_sum[k];
                b->b=a->b/covm->scnt_sum[k];
            }
        }
    }
}


void wtk_covm_update_nrnn(wtk_covm_t *covm,wtk_complex_t **fft,int k)
{
    int channel=covm->channel;
    int i,j;
    wtk_complex_t *fft1,*fft2,*a,*b;
    wtk_complex_t **ncov=covm->ncov;
    wtk_complex_t **ncovtmp=covm->ncovtmp;
    float alpha=covm->cfg->ncov_alpha;
    float alpha_1=1-covm->cfg->ncov_alpha;

    if(covm->ncnt_sum[k]>=covm->cfg->init_ncovnf)
    {
        ++covm->ncnt_sum[k];
        for(i=0;i<channel;++i)
        {
            fft1=fft[i];
            for(j=i;j<channel;++j)
            {
                fft2=fft[j];

                a=ncov[k]+i*channel+j;
                a->a=alpha_1*a->a+alpha*(fft1[k].a*fft2[k].a+fft1[k].b*fft2[k].b);
                a->b=alpha_1*a->b+alpha*(-fft1[k].a*fft2[k].b+fft1[k].b*fft2[k].a);
            }
        }
    }else
    {
        ++covm->ncnt_sum[k];
        for(i=0;i<channel;++i)
        {
            fft1=fft[i];
            for(j=i;j<channel;++j)
            {
                fft2=fft[j];

                a=ncovtmp[k]+i*channel+j;
                a->a+=fft1[k].a*fft2[k].a+fft1[k].b*fft2[k].b;
                a->b+=-fft1[k].a*fft2[k].b+fft1[k].b*fft2[k].a;

                b=ncov[k]+i*channel+j;
                b->a=a->a/covm->ncnt_sum[k];
                b->b=a->b/covm->ncnt_sum[k];
            }
        }
    }
}

void wtk_covm_update_cov(wtk_covm_t *covm, wtk_complex_t *cov, wtk_complex_t **fft,int k, int channel, int is_ncov)
{
    int i,j;
    wtk_complex_t *fft1,*fft2,*a,*b,*c;
    wtk_complex_t *ncov=covm->ncov[k];
    wtk_complex_t *scov=covm->scov[k];
    wtk_complex_t *ncovtmp=covm->ncovtmp[k];
    wtk_complex_t *scovtmp=covm->scovtmp[k];
    int cnt;

    if(is_ncov)
    {
        cnt=covm->qncov_q[k].length;
        covm->ncnt_sum[k]=cnt;

        for(i=0;i<channel;++i)
        {
            fft1=fft[i];
            for(j=i;j<channel;++j)
            {
                fft2=fft[j];

                a=cov+i*channel+j;
                a->a=fft1[k].a*fft2[k].a+fft1[k].b*fft2[k].b;
                a->b=-fft1[k].a*fft2[k].b+fft1[k].b*fft2[k].a;

                c=ncovtmp+i*channel+j;
                c->a+=a->a;
                c->b+=a->b;

                b=ncov+i*channel+j;
                b->a=c->a/cnt;
                b->b=c->b/cnt;
            }
        }
    }else
    {
        cnt=covm->qscov_q[k].length;
        covm->scnt_sum[k]=cnt;
        for(i=0;i<channel;++i)
        {
            fft1=fft[i];
            for(j=i;j<channel;++j)
            {
                fft2=fft[j];

                a=cov+i*channel+j;
                a->a=fft1[k].a*fft2[k].a+fft1[k].b*fft2[k].b;
                a->b=-fft1[k].a*fft2[k].b+fft1[k].b*fft2[k].a;

                c=scovtmp+i*channel+j;
                c->a+=a->a;
                c->b+=a->b;

                b=scov+i*channel+j;
                b->a=c->a/cnt;
                b->b=c->b/cnt;
            }
        }
    }
}

// fft: channel*nbin
int wtk_covm_feed_fft(wtk_covm_t *covm,wtk_complex_t **fft, int k, int is_ncov)
{
    int channel=covm->channel;
    wtk_covm_ncovmsg_t *qncmsg;
    wtk_covm_scovmsg_t *qscmsg;
    int ret;

    if(covm->cfg->use_covhist)
    {
        if(is_ncov)
        {
            if(covm->qncov_q[k].length == covm->cfg->ncov_hist)
            {
                wtk_covm_flush_covmsgq(covm, k, 1);
            }

            qncmsg=wtk_covm_pop_ncov_msg(covm);
            wtk_queue_push(&(covm->qncov_q[k]), &(qncmsg->q_n));
            wtk_covm_update_cov(covm, qncmsg->ncov, fft, k ,channel, 1);
        }else
        {
            if(covm->scov)
            {
                if(covm->qscov_q[k].length == covm->cfg->scov_hist)
                {
                    wtk_covm_flush_covmsgq(covm, k, 0);
                }
                qscmsg=wtk_covm_pop_scov_msg(covm);
                wtk_queue_push(&(covm->qscov_q[k]), &(qscmsg->q_n));
                wtk_covm_update_cov(covm, qscmsg->scov, fft, k ,channel,0);
            }
        }
    }else
    {
        if(is_ncov)
        {
            wtk_covm_update_nrnn(covm,fft,k);
        }else
        {
            if(covm->scov)
            {
                wtk_covm_update_srnn(covm,fft,k);
            }
        }
    }
    
    ret=0;
    if(is_ncov && covm->ncnt_sum[k]>5)
    {
        ret=1;
    }else if(!is_ncov && (covm->scov && covm->scnt_sum[k]>5))
    {
        ret=1;   
    }
    return ret;
}

