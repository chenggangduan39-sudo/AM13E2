#include "wtk_qform_pickup.h"

void wtk_qform_pickup_on_stft(wtk_qform_pickup_t *qform,wtk_stft_msg_t *msg,int pos,int is_end);

typedef struct
{
    wtk_queue_node_t q_n;
    wtk_stft_msg_t *smsg;
    float *cohv;
}wtk_qform_pickup_stftmsg_t;

wtk_qform_pickup_stftmsg_t * wtk_qform_pickup_stftmsg_new(wtk_stft_msg_t *smsg,float *cohv,int nbin)
{
    wtk_qform_pickup_stftmsg_t *msg;

    msg=(wtk_qform_pickup_stftmsg_t *)wtk_malloc(sizeof(wtk_qform_pickup_stftmsg_t));
    msg->smsg=smsg;
    msg->cohv=(float *)wtk_malloc(sizeof(float)*nbin);
    memcpy(msg->cohv,cohv,sizeof(float)*nbin);

    return msg;
}

void wtk_qform_pickup_stftmsg_delete(wtk_qform_pickup_stftmsg_t *msg)
{
    if(msg->cohv)
    {
        wtk_free(msg->cohv);
    }
    wtk_free(msg);
}

wtk_qform_pickup_t* wtk_qform_pickup_new(wtk_qform_pickup_cfg_t *cfg)
{
    wtk_qform_pickup_t *qform;
    int i;

    qform=(wtk_qform_pickup_t *)wtk_malloc(sizeof(wtk_qform_pickup_t));
    qform->cfg=cfg;
    qform->ths=NULL;
    qform->notify=NULL;

    qform->input=wtk_strbufs_new(cfg->bf.nmic);

    qform->stft=wtk_stft_new(&(cfg->stft));
    wtk_stft_set_notify(qform->stft,qform,(wtk_stft_notify_f)wtk_qform_pickup_on_stft);

    qform->bf=wtk_bf_new(&(cfg->bf),cfg->stft.win);

    qform->ovec=wtk_complex_new_p2(qform->bf->nbin,qform->bf->channel);
    qform->ovec_thresh=wtk_complex_new_p2(qform->bf->nbin,qform->bf->channel);
    qform->ovec_thresh2=wtk_complex_new_p2(qform->bf->nbin,qform->bf->channel);

    qform->novec=qform->novec_thresh1=qform->novec_thresh2=NULL;
    if(cfg->noise_block>0)
    {
        qform->novec=wtk_malloc(cfg->noise_block*sizeof(wtk_complex_t **));
        qform->novec_thresh1=wtk_malloc(cfg->noise_block*sizeof(wtk_complex_t **));
        qform->novec_thresh2=wtk_malloc(cfg->noise_block*sizeof(wtk_complex_t **));

        for(i=0;i<cfg->noise_block;++i)
        {
            qform->novec[i]=wtk_complex_new_p2(qform->bf->nbin,qform->bf->channel);
            qform->novec_thresh1[i]=wtk_complex_new_p2(qform->bf->nbin,qform->bf->channel);
            qform->novec_thresh2[i]=wtk_complex_new_p2(qform->bf->nbin,qform->bf->channel);
        }
    }

    qform->ncov=wtk_complex_new_p2(qform->bf->nbin,qform->bf->channelx2);
    // qform->ncovtmp=wtk_complex_new_p2(qform->bf->nbin,qform->bf->channelx2);
    qform->ncnt_sum=(int *)wtk_malloc(sizeof(int)*qform->bf->nbin);

    qform->scov=NULL;
    qform->scnt_sum=NULL;
    if(cfg->bf.use_gev || cfg->bf.use_eig_ovec || cfg->bf.use_r1_mwf || cfg->bf.use_sdw_mwf || cfg->bf.use_vs)
    {
        qform->scov=wtk_complex_new_p2(qform->bf->nbin,qform->bf->channelx2);
        qform->scnt_sum=(int *)wtk_malloc(sizeof(int)*qform->bf->nbin);
    }

    qform->cohv=(float *)wtk_malloc(sizeof(float)*qform->bf->nbin);

    qform->output=(short *)wtk_malloc((qform->bf->nbin-1)*sizeof(short));

    qform->qmmse=NULL;
    if(cfg->use_post)
    {
        qform->qmmse=wtk_qmmse_new(&(cfg->qmmse));
    }

    wtk_qform_pickup_reset(qform);

    return qform;
}

void wtk_qform_pickup_delete(wtk_qform_pickup_t *qform)
{
    int i;
    wtk_qform_pickup_stftmsg_t* qsmsg;
    wtk_queue_node_t *qn;

    if(qform->stft_q.length > 0)
    {
        while(1)
        {
            qn=wtk_queue_pop(&(qform->stft_q));
            if(!qn){break;}
            qsmsg=(wtk_qform_pickup_stftmsg_t *)data_offset2(qn,wtk_qform_pickup_stftmsg_t,q_n);
            wtk_stft_push_msg(qform->stft,qsmsg->smsg);
            wtk_qform_pickup_stftmsg_delete(qsmsg);
        }
    }

    wtk_strbufs_delete(qform->input,qform->bf->channel);
    wtk_stft_delete(qform->stft);
    // if(qform->ncovtmp)
    // {
    //     wtk_complex_delete_p2(qform->ncovtmp,qform->bf->nbin);
    // }
    wtk_free(qform->cohv);
    wtk_free(qform->ncnt_sum);

    wtk_complex_delete_p2(qform->ovec,qform->bf->nbin);
    wtk_complex_delete_p2(qform->ovec_thresh,qform->bf->nbin);
    wtk_complex_delete_p2(qform->ovec_thresh2,qform->bf->nbin);

    if(qform->cfg->noise_block>0)
    {
        for(i=0;i<qform->cfg->noise_block;++i)
        {
            wtk_complex_delete_p2(qform->novec[i],qform->bf->nbin);
            wtk_complex_delete_p2(qform->novec_thresh1[i],qform->bf->nbin);
            wtk_complex_delete_p2(qform->novec_thresh2[i],qform->bf->nbin);
        }
        wtk_free(qform->novec);
        wtk_free(qform->novec_thresh1);
        wtk_free(qform->novec_thresh2);
    }

    wtk_complex_delete_p2(qform->ncov,qform->bf->nbin);

    if(qform->scnt_sum)
    {
        wtk_free(qform->scnt_sum);
        wtk_complex_delete_p2(qform->scov,qform->bf->nbin);
    }
    
    wtk_bf_delete(qform->bf);

    if(qform->qmmse)
    {
        wtk_qmmse_delete(qform->qmmse);
    }

    wtk_free(qform->output);
    wtk_free(qform);
}

void wtk_qform_pickup_reset(wtk_qform_pickup_t *qform)
{
    int i;
    int channel=qform->bf->channel;

    for(i=0;i<channel;++i)
    {
        memset(qform->notch_mem[i],0,2*sizeof(float));
    }
	memset(qform->memD,0,channel*sizeof(float));
    qform->memX=0;

    qform->theta=qform->phi=0;
    wtk_stft_reset(qform->stft);
    wtk_bf_reset(qform->bf);

    wtk_complex_zero_p2(qform->ovec,qform->bf->nbin,qform->bf->channel);
    wtk_complex_zero_p2(qform->ovec_thresh,qform->bf->nbin,qform->bf->channel);
    wtk_complex_zero_p2(qform->ovec_thresh2,qform->bf->nbin,qform->bf->channel);

    for(i=0;i<qform->cfg->noise_block;++i)
    {
        wtk_complex_zero_p2(qform->novec[i],qform->bf->nbin,qform->bf->channel);
        wtk_complex_zero_p2(qform->novec_thresh1[i],qform->bf->nbin,qform->bf->channel);
        wtk_complex_zero_p2(qform->novec_thresh2[i],qform->bf->nbin,qform->bf->channel);
    }

    memset(qform->cohv,0,sizeof(float)*qform->bf->nbin);
    memset(qform->ncnt_sum,0,sizeof(int)*qform->bf->nbin);
    qform->nframe=0;
    wtk_complex_zero_p2(qform->ncov,qform->bf->nbin,qform->bf->channelx2);
    // wtk_complex_zero_p2(qform->ncovtmp,qform->bf->nbin,qform->bf->channelx2);


    if(qform->scnt_sum)
    {
        memset(qform->scnt_sum,0,sizeof(int)*qform->bf->nbin);
        wtk_complex_zero_p2(qform->scov,qform->bf->nbin,qform->bf->channelx2);
    }

    wtk_queue_init(&(qform->stft_q));

    if(qform->qmmse)
    {
      wtk_qmmse_reset(qform->qmmse);
    }
}

void wtk_qform_pickup_dc(wtk_qform_pickup_t *qform_pickup,float *mic,float *mem,int len)
{
	int i;
	float vin,vout;
	float radius=qform_pickup->cfg->notch_radius;
	float den2=qform_pickup->cfg->notch_radius_den;

	for(i=0;i<len;++i)
	{
		vin=mic[i];
		vout=mem[0]+vin;
		mic[i]=radius*vout;
		mem[0]=mem[1]+2*(-vin+mic[i]);
		mem[1]=vin-den2*vout;
	}
}

float wtk_qform_pickup_preemph(wtk_qform_pickup_t *qform_pickup,float *mic,int len,float memD)
{
	int i;
	float tmp;
	float preemph=qform_pickup->cfg->preemph;

	for(i=0;i<len;++i)
	{
		tmp=mic[i]-preemph*memD;
		memD=mic[i];
		mic[i]=tmp;
	}
	return memD;
}

float wtk_qform_pickup_preemph2(wtk_qform_pickup_t *qform_pickup,float *mic,int len,float memX)
{
	int i;
	float tmp;
	float preemph=qform_pickup->cfg->preemph;

	for(i=0;i<len;++i)
	{
		tmp=mic[i]+preemph*memX;
        mic[i]=tmp;
		memX=mic[i];
	}
	return memX;
}

void wtk_qform_pickup_set_notify(wtk_qform_pickup_t *qform,void *ths,wtk_qform_pickup_notify_f notify)
{
    qform->ths=ths;
    qform->notify=notify;
}

void wtk_qform_pickup_notify_data(wtk_qform_pickup_t *qform,float *data,int len)
{
    short *pv=qform->output;
    int i;

    if(qform->cfg->preemph>0)
    {
        qform->memX=wtk_qform_pickup_preemph2(qform,data,len,qform->memX);
    }
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
    qform->notify(qform->ths,pv,len);
}

void wtk_qform_pickup_update_rnn2(wtk_qform_pickup_t *qform,wtk_complex_t **fft,int k)
{
    int channel=qform->bf->channel;
    int i,j;
    wtk_complex_t *fft1,*fft2,*a;
    wtk_complex_t **scov=qform->scov;
    float alpha=qform->cfg->scov_alpha;
    float alpha_1=1-qform->cfg->scov_alpha;

    if(qform->scov==NULL)
    {
        return;
    }

    if(qform->scnt_sum[k]>=qform->cfg->init_scovnf)
    {
        ++qform->scnt_sum[k];
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
        ++qform->scnt_sum[k];
        for(i=0;i<channel;++i)
        {
            fft1=fft[i];
            for(j=i;j<channel;++j)
            {
                fft2=fft[j];

                a=scov[k]+i*channel+j;
                a->a+=fft1[k].a*fft2[k].a+fft1[k].b*fft2[k].b;
                a->b+=-fft1[k].a*fft2[k].b+fft1[k].b*fft2[k].a;
            }
        }
    }
    if(qform->scnt_sum[k]>5)
    {
        if(qform->bf->scov)
        {
            wtk_bf_update_scov(qform->bf,scov,k);
        }
    }
}


void wtk_qform_pickup_update_rnn1(wtk_qform_pickup_t *qform,wtk_complex_t **fft,int k)
{
    int channel=qform->bf->channel;
    int i,j;
    wtk_complex_t *fft1,*fft2,*a;//,*b;
    wtk_complex_t **ncov=qform->ncov;
    // wtk_complex_t **ncovtmp=qform->ncovtmp;
    float alpha=qform->cfg->ncov_alpha;
    float alpha_1=1-qform->cfg->ncov_alpha;

    if(qform->ncnt_sum[k]>=qform->cfg->init_ncovnf)
    {
        ++qform->ncnt_sum[k];
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
        ++qform->ncnt_sum[k];
        for(i=0;i<channel;++i)
        {
            fft1=fft[i];
            for(j=i;j<channel;++j)
            {
                fft2=fft[j];

                a=ncov[k]+i*channel+j;
                a->a+=fft1[k].a*fft2[k].a+fft1[k].b*fft2[k].b;
                a->b+=-fft1[k].a*fft2[k].b+fft1[k].b*fft2[k].a;

                // b=ncov[k]+i*channel+j;
                // b->a=a->a/qform->ncnt_sum[k];
                // b->b=a->b/qform->ncnt_sum[k];
            }
        }
    }
    if(qform->ncnt_sum[k]>5)
    {
        wtk_bf_update_ncov(qform->bf,ncov,k);
    }
}

void wtk_qform_pickup_flush2(wtk_qform_pickup_t *qform,wtk_stft_msg_t *smsg,float *cohv)
{
    int k;
    wtk_complex_t *bf_out;
    
    bf_out=wtk_bf_output_fft_msg2(qform->bf,smsg,cohv);
    if(qform->cfg->use_post)
    {
        wtk_qmmse_feed_cohv(qform->qmmse,bf_out,cohv);      
    }
    k=wtk_stft_output_ifft2(qform->stft,bf_out,0,0,0,qform->stft->output[0],qform->bf->pad);
    wtk_qform_pickup_notify_data(qform,qform->stft->output[0],k);
}

void wtk_qform_pickup_flush(wtk_qform_pickup_t *qform,wtk_stft_msg_t *smsg)
{
    float *cohv=qform->cohv;
    int k;
    int nbin=qform->bf->nbin;
    wtk_qform_pickup_stftmsg_t *qsmsg;
    wtk_queue_node_t *qn;
    // int i,channel=qform->bf->channel;
    // float cohV;
    // int nf=qform->nframe;

    // cohV=0;
    // for(k=1;k<nbin-1;++k)
    // {
    //     cohV+=cohv[k];
    // }

    // fprintf(qform->cohv_fn,"%d\t%f\n",nf,cohV);
    
    for(k=1;k<nbin-1;++k)
    {
        // wtk_debug("====> %.0f %d\n",qform->nframe,k+1);
        if((cohv[k]<0))// && cohV<0) || cohV<-175)
        {
            wtk_qform_pickup_update_rnn1(qform,smsg->fft,k);
            // for(i=0;i<channel;++i)
            // {
            //     smsg->fft[i][k].a=smsg->fft[i][k].b=0;
            // }
        }else
        {
            wtk_qform_pickup_update_rnn2(qform,smsg->fft,k);
        }
        if(qform->ncnt_sum[k]>5 && (qform->scnt_sum==NULL ||  qform->scnt_sum[k]>5))
        {
            wtk_bf_update_w(qform->bf,k);
        }   
    }
    if(qform->cfg->nbatch>0)
    {
        qsmsg=wtk_qform_pickup_stftmsg_new(smsg,qform->cohv,qform->bf->nbin);
        wtk_queue_push(&(qform->stft_q),&(qsmsg->q_n));
        if(qform->stft_q.length == qform->cfg->nbatch)
        {
            for(k=0;k<qform->stft_q.length/2;++k)
            {
                qn=wtk_queue_pop(&(qform->stft_q));
                if(!qn){break;}
                qsmsg=(wtk_qform_pickup_stftmsg_t *)data_offset2(qn,wtk_qform_pickup_stftmsg_t,q_n);
                wtk_qform_pickup_flush2(qform,qsmsg->smsg,qsmsg->cohv);
                wtk_stft_push_msg(qform->stft,qsmsg->smsg);
                wtk_qform_pickup_stftmsg_delete(qsmsg);
            }
        }
    }else
    {
        wtk_qform_pickup_flush2(qform,smsg,cohv);
        wtk_stft_push_msg(qform->stft,smsg);
    }
}

void wtk_qform_pickup_thresh_flush(wtk_qform_pickup_t *qform,wtk_stft_msg_t *msg,int pos,int is_end)
{
    int i,j;
    int nbin=qform->bf->nbin;
    int channel=qform->bf->channel;
    wtk_complex_t **fft;
    wtk_complex_t coh,coh2;
    wtk_complex_t **ovec=qform->ovec;
    wtk_complex_t **ovec_thresh=qform->ovec_thresh;
    wtk_complex_t *overtmp,*overtmp2;
    float *cohv=qform->cohv;
    float f,ff;

    if(!msg){return;}
    if(is_end)
    {
        wtk_stft_push_msg(qform->stft,msg);
        return;
    }
    
    fft=msg->fft;
    ++qform->nframe;
    if(qform->nframe>20000)
	{
		qform->nframe=20000;
	}

    for(i=1;i<nbin-1;++i)
    {
        // wtk_debug("%d  %f-%f ms\n",i,(qform->nframe-1)*16,qform->nframe*16);
        coh.a=coh.b=0;
        coh2.a=coh2.b=0;
        overtmp=ovec[i];
        overtmp2=ovec_thresh[i];

        f=0;
        for(j=0;j<channel;++j)
        {
            coh.a+=fft[j][i].a*overtmp->a+fft[j][i].b*overtmp->b;
            coh.b+=-fft[j][i].a*overtmp->b+fft[j][i].b*overtmp->a;

            coh2.a+=fft[j][i].a*overtmp2->a+fft[j][i].b*overtmp2->b;
            coh2.b+=-fft[j][i].a*overtmp2->b+fft[j][i].b*overtmp2->a;

            f+=fft[j][i].a*fft[j][i].a+fft[j][i].b*fft[j][i].b;

            ++overtmp;
            ++overtmp2;
        }
        // wtk_debug("%f+%fi, %f\n",coh.a,coh.b,sqrt(f*f2));
        ff=sqrt((coh.a*coh.a+coh.b*coh.b)/f);
        if(ff-sqrt((coh2.a*coh2.a+coh2.b*coh2.b)/f)>0)
        {
            cohv[i]=ff>qform->cfg->cohv_thresh?ff:-1;
        }else
        {
            cohv[i]=-1;
        }
    }
    wtk_qform_pickup_flush(qform,msg);
}

void wtk_qform_pickup_thresh_flush2(wtk_qform_pickup_t *qform,wtk_stft_msg_t *msg,int pos,int is_end)
{
    int i,j,k;
    int nbin=qform->bf->nbin;
    int channel=qform->bf->channel;
    wtk_complex_t **fft;
    wtk_complex_t coh,coh2,coh3;
    wtk_complex_t **ovec=qform->ovec;
    wtk_complex_t **ovec_thresh=qform->ovec_thresh;
    wtk_complex_t **ovec_thresh2=qform->ovec_thresh2;
    wtk_complex_t *overtmp,*overtmp2,*overtmp3;

   wtk_complex_t ncoh[10],ncoh2[10],ncoh3[10];
    wtk_complex_t ***novec=qform->novec;
    wtk_complex_t ***novec_thresh1=qform->novec_thresh1;
    wtk_complex_t ***novec_thresh2=qform->novec_thresh2;
    wtk_complex_t *novertmp[10],*novertmp2[10],*novertmp3[10];

    float *cohv=qform->cohv;
    float f,f2,f3,f4,f5,ff;
    int noise_block=qform->cfg->noise_block;
    float cohvsum=0,ncohvsum[10]={0};
    int b;

    if(!msg){return;}
    if(is_end)
    {
        wtk_stft_push_msg(qform->stft,msg);
        return;
    }

    fft=msg->fft;
    ++qform->nframe;
    if(qform->nframe>20000)
	{
		qform->nframe=20000;
	}

    for(i=1;i<nbin-1;++i)
    {
        // wtk_debug("%d  %f-%f ms\n",i,(qform->nframe-1)*16,qform->nframe*16);

        coh.a=coh.b=0;
        coh2.a=coh2.b=0;
        coh3.a=coh3.b=0;
        overtmp=ovec[i];
        overtmp2=ovec_thresh[i];
        overtmp3=ovec_thresh2[i];

        for(k=0;k<noise_block;++k)
        {
            ncoh[k].a=ncoh[k].b=0;
            ncoh2[k].a=ncoh2[k].b=0;
            ncoh3[k].a=ncoh3[k].b=0;
            novertmp[k]=novec[k][i];
            novertmp2[k]=novec_thresh1[k][i];
            novertmp3[k]=novec_thresh2[k][i];
        }

        f=0;
        for(j=0;j<channel;++j)
        {
            coh.a+=fft[j][i].a*overtmp->a+fft[j][i].b*overtmp->b;
            coh.b+=-fft[j][i].a*overtmp->b+fft[j][i].b*overtmp->a;

            coh2.a+=fft[j][i].a*overtmp2->a+fft[j][i].b*overtmp2->b;
            coh2.b+=-fft[j][i].a*overtmp2->b+fft[j][i].b*overtmp2->a;

            coh3.a+=fft[j][i].a*overtmp3->a+fft[j][i].b*overtmp3->b;
            coh3.b+=-fft[j][i].a*overtmp3->b+fft[j][i].b*overtmp3->a;

            f+=fft[j][i].a*fft[j][i].a+fft[j][i].b*fft[j][i].b;

            for(k=0;k<noise_block;++k)
            {
                ncoh[k].a+=fft[j][i].a*novertmp[k]->a+fft[j][i].b*novertmp[k]->b;
                ncoh[k].b+=-fft[j][i].a*novertmp[k]->b+fft[j][i].b*novertmp[k]->a;

                ncoh2[k].a+=fft[j][i].a*novertmp2[k]->a+fft[j][i].b*novertmp2[k]->b;
                ncoh2[k].b+=-fft[j][i].a*novertmp2[k]->b+fft[j][i].b*novertmp2[k]->a;

                ncoh3[k].a+=fft[j][i].a*novertmp3[k]->a+fft[j][i].b*novertmp3[k]->b;
                ncoh3[k].b+=-fft[j][i].a*novertmp3[k]->b+fft[j][i].b*novertmp3[k]->a;

                ++novertmp[k];
                ++novertmp2[k];
                ++novertmp3[k];
            }

            ++overtmp;
            ++overtmp2;
            ++overtmp3;

        }
        // wtk_debug("%f+%fi, %f\n",coh.a,coh.b,sqrt(f*f2));

        // if(fabs(coh.a)>1)
        // {
        //     printf("%f\n",coh.a);
        // }
        f*=channel;
        ff=sqrt((coh.a*coh.a+coh.b*coh.b)/f);
        f2=ff-sqrt((coh2.a*coh2.a+coh2.b*coh2.b)/f);
        f3=ff-sqrt((coh3.a*coh3.a+coh3.b*coh3.b)/f);
        if(qform->cfg->use_cohvsum && i>qform->cfg->fs && i<qform->cfg->fe)
        {
            cohvsum+=ff;
        }
        cohv[i]=ff;
        // wtk_debug("%f\n",ff);
        if(f2<0||f3<0||ff<qform->cfg->cohv_thresh)
        {
            cohv[i]=-1.0; 
        }
        for(k=0;k<noise_block;++k)
        {
            ff=sqrt((ncoh[k].a*ncoh[k].a+ncoh[k].b*ncoh[k].b)/f);
            f4=ff-sqrt((ncoh2[k].a*ncoh2[k].a+ncoh2[k].b*ncoh2[k].b)/f);
            f5=ff-sqrt((ncoh3[k].a*ncoh3[k].a+ncoh3[k].b*ncoh3[k].b)/f);
            if(qform->cfg->use_cohvsum && i>qform->cfg->fs && i<qform->cfg->fe)
            {
                ncohvsum[k]+=ff;
            }
            if(f4>0&&f5>0)
            {
                cohv[i]=-1;
            }
        }
    }
    if(qform->cfg->use_cohvsum && noise_block>0)
    {
        b=0;
        for(k=0;k<noise_block;++k)
        {
            if(ncohvsum[k]<cohvsum)
            {
                b=1;
            }
        }
        if(b==0)
        {
            // wtk_debug("%f %f %f %f\n",qform->nframe*qform->stft->cfg->step/16,ncohvsum[0],ncohvsum[1],cohvsum);
            for(i=1;i<nbin-1;++i)
            {
                cohv[i]=-1;
            }
        }
    }
    wtk_qform_pickup_flush(qform,msg);
}


void wtk_qform_pickup_on_stft(wtk_qform_pickup_t *qform,wtk_stft_msg_t *msg,int pos,int is_end)
{
    wtk_qform_pickup_thresh_flush2(qform,msg,pos,is_end);
}

void wtk_qform_pickup_set_ovec(wtk_qform_pickup_t *qform,float theta,float phi)
{
    int i,nbin=qform->bf->nbin;

    if(qform->bf->cfg->use_eig_ovec || !qform->cfg->bf.use_mvdr){return;}
    
    wtk_bf_update_ovec(qform->bf,theta,phi);
    for(i=1;i<nbin;++i)
    {
        wtk_bf_update_w(qform->bf,i);
    }
}

void wtk_qform_pickup_set_thresh_ovec(wtk_qform_pickup_t *qform,float theta,float theta_range,
            wtk_complex_t **ovec,wtk_complex_t **ovec_th1,wtk_complex_t **ovec_th2)
{
    float theta2,theta3;

    wtk_bf_update_ovec3(qform->bf,theta,0,ovec);
    
    theta2=theta+2*theta_range;
    if(qform->cfg->use_line)
    {
        if(theta2>180)
        {
            theta2=360-theta2;
            // wtk_debug("ovec error : theta range overfloor\n");
        }
    }
    wtk_bf_update_ovec3(qform->bf,theta2,0,ovec_th1);

    theta3=theta-2*theta_range;
    if(qform->cfg->use_line)
    {
        if(theta3<0)
        {
            theta3=-theta3;
            // wtk_debug("ovec error : theta range overfloor\n");
        }
    }
    wtk_bf_update_ovec3(qform->bf,theta3,0,ovec_th2);
}

void wtk_qform_pickup_start(wtk_qform_pickup_t *qform,float theta,float phi)
{
    int i;

    qform->theta=theta;
    qform->phi=phi;
    wtk_bf_update_ovec(qform->bf,theta,phi);
    // wtk_qform_pickup_update_zero_rnn(qform,qform->cfg->theta_range,180);
    wtk_bf_init_w(qform->bf);

    wtk_qform_pickup_set_thresh_ovec(qform,theta,qform->cfg->theta_range,qform->ovec,qform->ovec_thresh,qform->ovec_thresh2);
    for(i=0;i<qform->cfg->noise_block;++i)
    {
        qform->ntheta[i]=theta+qform->cfg->ntheta[i];
        if(qform->ntheta[i]>=360)
        {
            qform->ntheta[i]-=360;
        }
        wtk_debug("==============%f %f===========\n",qform->ntheta[i],qform->cfg->ntheta_range);
        wtk_qform_pickup_set_thresh_ovec(qform,qform->ntheta[i],qform->cfg->ntheta_range,qform->novec[i],qform->novec_thresh1[i],qform->novec_thresh2[i]);
    }
}

void wtk_qform_pickup_feed2(wtk_qform_pickup_t *qform,short **data,int len,int is_end)
{   
    int i,j;
    int channel=qform->bf->channel;
    float fv;
    float *fp[10];
    wtk_strbuf_t **input=qform->input;

    for(i=0;i<channel;++i)
    {
        wtk_strbuf_reset(input[i]);
        for(j=0;j<len;++j)
        {
            fv=data[i][j]/32768.0;
            wtk_strbuf_push(input[i],(char *)(&fv),sizeof(float));
        }
        fp[i]=(float *)(input[i]->data);
        
        if(qform->cfg->notch_radius>0)
	    {
            wtk_qform_pickup_dc(qform,fp[i],qform->notch_mem[i],len);
        }
        if(qform->cfg->preemph>0)
        {
            qform->memD[i]=wtk_qform_pickup_preemph(qform,fp[i],len,qform->memD[i]);
        }
    }

    wtk_stft_feed_float2(qform->stft,fp,len,is_end);
}

void wtk_qform_pickup_feed(wtk_qform_pickup_t *qform,short **data,int len,int is_end)
{
#ifdef DEBUG_WAV
	static wtk_wavfile_t *mic_log=NULL;

	if(!mic_log)
	{
		mic_log=wtk_wavfile_new(16000);
		wtk_wavfile_set_channel(mic_log,qform->bf->channel);
		wtk_wavfile_open2(mic_log,"qform_pickup");
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
    if(qform->cfg->preemph>0 || qform->cfg->notch_radius>0)
    {
        wtk_qform_pickup_feed2(qform,data,len,is_end);
    }else
    {
        wtk_stft_feed(qform->stft,data,len,is_end);
    }
}


void wtk_qform_pickup_set_grav(wtk_qform_pickup_t *qform,short x,short y,short z)
{
    float theta;
    // float phi;
    float f;

    f = atan2(y*1.0,x*1.0);
    
    theta = f/PI*180;
    // phi = 90 - atan2(y*1.0,z*1.0*sin(f))/PI*180;
    // wtk_debug("theta = %f \n",theta);

    f=fabs(theta - qform->theta);
    if(f >= 360 )
    {
        f=f-360;
    }
    if(f > 10)
    {
        wtk_qform_pickup_feed(qform,NULL,0,1);
        wtk_qform_pickup_reset(qform);
        wtk_qform_pickup_start(qform,theta,0);
    }else
    {
        // wtk_qform_pickup_set_ovec(qform,theta,0);
    }
}