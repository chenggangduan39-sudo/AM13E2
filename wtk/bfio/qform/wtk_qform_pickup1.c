#include "wtk_qform_pickup1.h" 

void wtk_qform_pickup1_on_stft2(wtk_qform_pickup1_t *qform_pickup1,wtk_stft2_msg_t *msg,int pos,int is_end);

wtk_qform_pickup1_stft2msg_t * wtk_qform_pickup1_stft2msg_new(wtk_stft2_msg_t *smsg,char *cohv,int nbin,int channel)
{
    wtk_qform_pickup1_stft2msg_t *msg;

    msg=(wtk_qform_pickup1_stft2msg_t *)wtk_malloc(sizeof(wtk_qform_pickup1_stft2msg_t));
    msg->smsg=wtk_stft2_msg_copy(smsg,channel,nbin);
    msg->cohv=(char *)wtk_malloc(sizeof(char)*nbin);
    memcpy(msg->cohv,cohv,sizeof(char)*nbin);

    return msg;
}


void wtk_qform_pickup1_stft2msg_delete(wtk_qform_pickup1_stft2msg_t *msg,wtk_stft2_t *stft2)
{
    wtk_free(msg->cohv);
    wtk_stft2_msg_delete(stft2,msg->smsg);
    wtk_free(msg);
}

wtk_qform_pickup1_ncovmsg_t *wtk_qform_pickup1_ncovmsg_new(int nchan)
{
    wtk_qform_pickup1_ncovmsg_t *ncovmsg;

    ncovmsg=(wtk_qform_pickup1_ncovmsg_t *)wtk_malloc(sizeof(wtk_qform_pickup1_ncovmsg_t));
    ncovmsg->ncov=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*nchan*nchan);

    return ncovmsg;
}

void wtk_qform_pickup1_ncovmsg_delete(wtk_qform_pickup1_ncovmsg_t *ncovmsg)
{
    wtk_free(ncovmsg->ncov);
    wtk_free(ncovmsg);
}

wtk_qform_pickup1_scovmsg_t *wtk_qform_pickup1_scovmsg_new(int nchan)
{
    wtk_qform_pickup1_scovmsg_t *scovmsg;
    scovmsg=(wtk_qform_pickup1_scovmsg_t *)wtk_malloc(sizeof(wtk_qform_pickup1_scovmsg_t));
    scovmsg->scov=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*nchan*nchan);

    return scovmsg;
}

void wtk_qform_pickup1_scovmsg_delete(wtk_qform_pickup1_scovmsg_t *scovmsg)
{
    wtk_free(scovmsg->scov);
    wtk_free(scovmsg);
}

wtk_qform_pickup1_t* wtk_qform_pickup1_new(wtk_qform_pickup1_cfg_t *cfg)
{
    wtk_qform_pickup1_t *qform_pickup1;
    int i;

    qform_pickup1=(wtk_qform_pickup1_t *)wtk_malloc(sizeof(wtk_qform_pickup1_t));
    qform_pickup1->cfg=cfg;
    qform_pickup1->ths=NULL;
    qform_pickup1->notify=NULL;

    qform_pickup1->input=NULL;
    if(cfg->preemph>0 || cfg->notch_radius>0)
    {
        qform_pickup1->input=wtk_strbufs_new(cfg->bf.nmic);
    }

    qform_pickup1->stft2=wtk_stft2_new(&(cfg->stft2));
    wtk_stft2_set_notify(qform_pickup1->stft2,qform_pickup1,(wtk_stft2_notify_f)wtk_qform_pickup1_on_stft2);

    qform_pickup1->bf=wtk_bf_new(&(cfg->bf),cfg->stft2.win);

    qform_pickup1->aspec=NULL;
    qform_pickup1->naspec=NULL;
    qform_pickup1->aspec_class[0]=qform_pickup1->aspec_class[1]=NULL;
    if(cfg->use_noiseblock)
    {
        qform_pickup1->naspec=(wtk_aspec_t **)wtk_malloc(sizeof(wtk_aspec_t *)*cfg->ntheta_num);
        for(i=0; i<cfg->ntheta_num; ++i)
        {
            qform_pickup1->naspec[i]=wtk_aspec_new(&(cfg->aspec), qform_pickup1->stft2->nbin, 3);
        }
    }else if(cfg->use_noiseblock2)
    {
        qform_pickup1->aspec=wtk_aspec_new(&(cfg->aspec), qform_pickup1->stft2->nbin, 3);        
        qform_pickup1->naspec=(wtk_aspec_t **)wtk_malloc(sizeof(wtk_aspec_t *)*cfg->ntheta_num);
        for(i=0; i<cfg->ntheta_num; ++i)
        {
            qform_pickup1->naspec[i]=wtk_aspec_new(&(cfg->aspec), qform_pickup1->stft2->nbin, 3);
        }
    }else if(cfg->use_two_aspecclass)
    {
        qform_pickup1->aspec_class[0]=wtk_aspec_new(&(cfg->aspec_class1), qform_pickup1->stft2->nbin, 3);        
        qform_pickup1->aspec_class[1]=wtk_aspec_new(&(cfg->aspec_class2), qform_pickup1->stft2->nbin, 3);        
    }else
    {
        qform_pickup1->aspec=wtk_aspec_new(&(cfg->aspec), qform_pickup1->stft2->nbin, 3);        
    }

    qform_pickup1->fftclass[0]=qform_pickup1->fftclass[1]=NULL;
    if(qform_pickup1->aspec_class[0] && !qform_pickup1->aspec_class[0]->need_cov)
    {
        qform_pickup1->fftclass[0]=wtk_complex_new_p2(qform_pickup1->stft2->nbin,qform_pickup1->aspec_class[0]->cfg->channel);
        qform_pickup1->fftclass[1]=wtk_complex_new_p2(qform_pickup1->stft2->nbin,qform_pickup1->aspec_class[1]->cfg->channel);
    }
    
    qform_pickup1->cov=NULL;
    qform_pickup1->covclass[0]=qform_pickup1->covclass[1]=NULL;
    wtk_queue_init(&(qform_pickup1->stft2_q));
    if((qform_pickup1->aspec && qform_pickup1->aspec->need_cov) || (qform_pickup1->naspec && qform_pickup1->naspec[0]->need_cov) )
    {
        qform_pickup1->cov=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->stft2.channel*cfg->stft2.channel);
        if(cfg->lt<=0)
        {
            qform_pickup1->wint=wtk_malloc(sizeof(float));
            qform_pickup1->wint[0]=1;
        }else
        {
            qform_pickup1->wint=wtk_math_create_hanning_window(2*cfg->lt+1);
        }

        if(cfg->lf<=0)
        {
            qform_pickup1->winf=wtk_malloc(sizeof(float));
            qform_pickup1->winf[0]=1;
        }else
        {
            qform_pickup1->winf=wtk_math_create_hanning_window(2*cfg->lf+1);
        }

        if(qform_pickup1->aspec_class[0])
        {
            qform_pickup1->covclass[0]=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*qform_pickup1->aspec_class[0]->cfg->channel*qform_pickup1->aspec_class[0]->cfg->channel);
            qform_pickup1->covclass[1]=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*qform_pickup1->aspec_class[1]->cfg->channel*qform_pickup1->aspec_class[1]->cfg->channel);
        }
    }
    qform_pickup1->inv_cov=NULL;
    qform_pickup1->invcovclass[0]=qform_pickup1->invcovclass[1]=NULL;
    qform_pickup1->tmp=NULL;
    if((qform_pickup1->aspec && qform_pickup1->aspec->need_inv_cov) || (qform_pickup1->naspec && qform_pickup1->naspec[0]->need_inv_cov))
    {
        qform_pickup1->inv_cov=(wtk_complex_t *)wtk_malloc(cfg->stft2.channel*cfg->stft2.channel*sizeof(wtk_complex_t));
        qform_pickup1->tmp=(wtk_dcomplex_t *)wtk_malloc(cfg->stft2.channel*cfg->stft2.channel*2*sizeof(wtk_dcomplex_t));
    }
    if(qform_pickup1->aspec_class[0] && qform_pickup1->aspec_class[0]->need_inv_cov)
    {
        qform_pickup1->invcovclass[0]=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*qform_pickup1->aspec_class[0]->cfg->channel*qform_pickup1->aspec_class[0]->cfg->channel);
        qform_pickup1->invcovclass[1]=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*qform_pickup1->aspec_class[1]->cfg->channel*qform_pickup1->aspec_class[1]->cfg->channel);
        i=max(qform_pickup1->aspec_class[0]->cfg->channel,qform_pickup1->aspec_class[1]->cfg->channel);
        qform_pickup1->tmp=(wtk_dcomplex_t *)wtk_malloc(i*i*2*sizeof(wtk_dcomplex_t));
    }

    qform_pickup1->ncov=wtk_complex_new_p2(qform_pickup1->bf->nbin,qform_pickup1->bf->channelx2);
    // qform_pickup1->ncovtmp=wtk_complex_new_p2(qform_pickup1->bf->nbin,qform_pickup1->bf->channelx2);
    qform_pickup1->ncnt_sum=(float *)wtk_malloc(sizeof(float)*qform_pickup1->bf->nbin);

    qform_pickup1->scov=NULL;
    qform_pickup1->scnt_sum=NULL;
    if(cfg->bf.use_gev || cfg->bf.use_eig_ovec || cfg->bf.use_r1_mwf || cfg->bf.use_sdw_mwf || cfg->bf.use_vs)
    {
        qform_pickup1->scov=wtk_complex_new_p2(qform_pickup1->bf->nbin,qform_pickup1->bf->channelx2);
        qform_pickup1->scnt_sum=(float *)wtk_malloc(sizeof(float)*qform_pickup1->bf->nbin);
    }

    qform_pickup1->qscov_q=qform_pickup1->qncov_q=NULL;
    if(cfg->use_covhist)
    {
        if(qform_pickup1->scov)
        {
            qform_pickup1->qscov_q=(wtk_queue_t *)wtk_malloc(sizeof(wtk_queue_t)*qform_pickup1->bf->nbin);
            for(i=0;i<qform_pickup1->bf->nbin;++i)
            {
                wtk_queue_init(&(qform_pickup1->qscov_q[i]));
            }
        }
        qform_pickup1->qncov_q=(wtk_queue_t *)wtk_malloc(sizeof(wtk_queue_t)*qform_pickup1->bf->nbin);
        for(i=0;i<qform_pickup1->bf->nbin;++i)
        {
            wtk_queue_init(&(qform_pickup1->qncov_q[i]));
        }
    }
    qform_pickup1->cohv=(char *)wtk_malloc(sizeof(char)*qform_pickup1->bf->nbin);

    // qform_pickup1->cohv_fn=NULL;
    // if(cfg->debug)
    // {
    //     qform_pickup1->cohv_fn=fopen("cohv.dat","w");
    // }

    wtk_queue_init(&(qform_pickup1->qsmsg_q));

    // qform_pickup1->output=(short *)wtk_malloc((qform_pickup1->stft2->nbin-1)*sizeof(short));

    wtk_qform_pickup1_reset(qform_pickup1);

    return qform_pickup1;
}

void wtk_qform_pickup1_delete(wtk_qform_pickup1_t *qform_pickup1)
{
    wtk_queue_node_t *qn;
    wtk_qform_pickup1_ncovmsg_t *qncmsg;
    wtk_qform_pickup1_scovmsg_t *qscmsg;
    int nbin=qform_pickup1->bf->nbin;
    int i;
    wtk_qform_pickup1_stft2msg_t *qsmsg;

    if(qform_pickup1->input)
    {
        wtk_strbufs_delete(qform_pickup1->input, qform_pickup1->bf->channel);
    }

    if(qform_pickup1->cov)
    {
        wtk_free(qform_pickup1->cov);
        wtk_free(qform_pickup1->wint);
        wtk_free(qform_pickup1->winf);
    }
    if(qform_pickup1->inv_cov)
    {
        wtk_free(qform_pickup1->inv_cov);
    }
    if(qform_pickup1->tmp)
    {
        wtk_free(qform_pickup1->tmp);
    }
    if(qform_pickup1->covclass[0])
    {
        wtk_free(qform_pickup1->covclass[0]);
        wtk_free(qform_pickup1->covclass[1]);
    }
    if(qform_pickup1->invcovclass[0])
    {
        wtk_free(qform_pickup1->invcovclass[0]);
        wtk_free(qform_pickup1->invcovclass[1]);
    }
    if(qform_pickup1->fftclass[0])
    {
        wtk_complex_delete_p2(qform_pickup1->fftclass[0],qform_pickup1->stft2->nbin);
        wtk_complex_delete_p2(qform_pickup1->fftclass[1],qform_pickup1->stft2->nbin);
    }

    if(qform_pickup1->aspec)
    {
        wtk_aspec_delete(qform_pickup1->aspec);
    }
    if(qform_pickup1->naspec)
    {
        for(i=0; i<qform_pickup1->cfg->ntheta_num; ++i)
        {
            wtk_aspec_delete(qform_pickup1->naspec[i]);
        }
        wtk_free(qform_pickup1->naspec);
    }
    if(qform_pickup1->aspec_class[0])
    {
        wtk_aspec_delete(qform_pickup1->aspec_class[0]);
        wtk_aspec_delete(qform_pickup1->aspec_class[1]);
    }

    if(qform_pickup1->qscov_q)
    {
        for(i=0;i<nbin;++i)
        {
            while(qform_pickup1->qscov_q[i].length>0)
            {
                qn=wtk_queue_pop(qform_pickup1->qscov_q+i);
                if(!qn){break;}
                qscmsg=(wtk_qform_pickup1_scovmsg_t *)data_offset(qn,wtk_qform_pickup1_scovmsg_t,q_n);
                wtk_qform_pickup1_scovmsg_delete(qscmsg);
            }
        }
        wtk_free(qform_pickup1->qscov_q);
    }
    if(qform_pickup1->qncov_q)
    {
        for(i=0;i<nbin;++i)
        {
            while(qform_pickup1->qncov_q[i].length>0)
            {
                qn=wtk_queue_pop(qform_pickup1->qncov_q+i);
                if(!qn){break;}
                qncmsg=(wtk_qform_pickup1_ncovmsg_t *)data_offset(qn,wtk_qform_pickup1_ncovmsg_t,q_n);
                wtk_qform_pickup1_ncovmsg_delete(qncmsg);
            }
        }
        wtk_free(qform_pickup1->qncov_q);
    }

    while(qform_pickup1->qsmsg_q.length>0)
    {
        qn=wtk_queue_pop(&(qform_pickup1->qsmsg_q));
        if(!qn){break;}
        qsmsg=(wtk_qform_pickup1_stft2msg_t *)data_offset(qn,wtk_qform_pickup1_stft2msg_t,q_n);
        wtk_qform_pickup1_stft2msg_delete(qsmsg,qform_pickup1->stft2);
    }

    // if(qform_pickup1->cohv_fn)
    // {
    //     fclose(qform_pickup1->cohv_fn);
    // }

    wtk_free(qform_pickup1->cohv);
    wtk_free(qform_pickup1->ncnt_sum);

    if(qform_pickup1->scnt_sum)
    {
        wtk_free(qform_pickup1->scnt_sum);
        wtk_complex_delete_p2(qform_pickup1->scov,qform_pickup1->bf->nbin);
    }

    wtk_complex_delete_p2(qform_pickup1->ncov,qform_pickup1->bf->nbin);

    wtk_stft2_delete(qform_pickup1->stft2);
    wtk_bf_delete(qform_pickup1->bf);
    // wtk_free(qform_pickup1->output);
    wtk_free(qform_pickup1);
}

void wtk_qform_pickup1_reset(wtk_qform_pickup1_t *qform_pickup1)
{
    int i;
    wtk_queue_node_t *qn;
    wtk_qform_pickup1_ncovmsg_t *qncmsg;
    wtk_qform_pickup1_scovmsg_t *qscmsg;
    int nbin=qform_pickup1->bf->nbin;
    wtk_qform_pickup1_stft2msg_t *qsmsg;
    wtk_stft2_msg_t *smsg;
    int channel=qform_pickup1->bf->channel;

    qform_pickup1->end_pos=0;
    for(i=0;i<channel;++i)
    {
        memset(qform_pickup1->notch_mem[i],0,2*sizeof(float));
    }
	memset(qform_pickup1->memD,0,channel*sizeof(float));
    qform_pickup1->memX=0;

    if(qform_pickup1->input)
    {
        wtk_strbufs_reset(qform_pickup1->input, qform_pickup1->bf->channel);
    }

    if(qform_pickup1->aspec)
    {
        wtk_aspec_reset(qform_pickup1->aspec);
    }
    if(qform_pickup1->naspec)
    {
        for(i=0; i<qform_pickup1->cfg->ntheta_num; ++i)
        {
            wtk_aspec_reset(qform_pickup1->naspec[i]);
        }
    }
    if(qform_pickup1->aspec_class[0])
    {
        wtk_aspec_reset(qform_pickup1->aspec_class[0]);
        wtk_aspec_reset(qform_pickup1->aspec_class[1]);
    }
    
    while(qform_pickup1->stft2_q.length>0)
    {
        qn=wtk_queue_pop(&(qform_pickup1->stft2_q));
        if(!qn){break;}
        smsg=(wtk_stft2_msg_t *)data_offset(qn,wtk_stft2_msg_t,q_n);
        wtk_stft2_push_msg(qform_pickup1->stft2,smsg);
    }

    wtk_stft2_reset(qform_pickup1->stft2);
    wtk_bf_reset(qform_pickup1->bf);

    qform_pickup1->nframe=0;

    if(qform_pickup1->qscov_q)
    {
        for(i=0;i<nbin;++i)
        {
            while(qform_pickup1->qscov_q[i].length>0)
            {
                qn=wtk_queue_pop(qform_pickup1->qscov_q+i);
                if(!qn){break;}
                qscmsg=(wtk_qform_pickup1_scovmsg_t *)data_offset(qn,wtk_qform_pickup1_scovmsg_t,q_n);
                wtk_qform_pickup1_scovmsg_delete(qscmsg);
            }
        }
    }
    if(qform_pickup1->qncov_q)
    {
        for(i=0;i<nbin;++i)
        {
            while(qform_pickup1->qncov_q[i].length>0)
            {
                qn=wtk_queue_pop(qform_pickup1->qncov_q+i);
                if(!qn){break;}
                qncmsg=(wtk_qform_pickup1_ncovmsg_t *)data_offset(qn,wtk_qform_pickup1_ncovmsg_t,q_n);
                wtk_qform_pickup1_ncovmsg_delete(qncmsg);
            }
        }
    }
    while(qform_pickup1->qsmsg_q.length>0)
    {
        qn=wtk_queue_pop(&(qform_pickup1->qsmsg_q));
        if(!qn){break;}
        qsmsg=(wtk_qform_pickup1_stft2msg_t *)data_offset(qn,wtk_qform_pickup1_stft2msg_t,q_n);
        wtk_qform_pickup1_stft2msg_delete(qsmsg,qform_pickup1->stft2);
    }

    qform_pickup1->theta=qform_pickup1->phi=-1;

    memset(qform_pickup1->cohv,0,sizeof(char)*qform_pickup1->bf->nbin);
    memset(qform_pickup1->ncnt_sum,0,sizeof(int)*qform_pickup1->bf->nbin);

    wtk_complex_zero_p2(qform_pickup1->ncov,qform_pickup1->bf->nbin,qform_pickup1->bf->channelx2);

    if(qform_pickup1->scnt_sum)
    {
        memset(qform_pickup1->scnt_sum,0,sizeof(int)*qform_pickup1->bf->nbin);
        wtk_complex_zero_p2(qform_pickup1->scov,qform_pickup1->bf->nbin,qform_pickup1->bf->channelx2);
    }
}


void wtk_qform_pickup1_dc(wtk_qform_pickup1_t *qform_pickup1,float *mic,float *mem,int len)
{
	int i;
	float vin,vout;
	float radius=qform_pickup1->cfg->notch_radius;
	float den2=qform_pickup1->cfg->notch_radius_den;

	for(i=0;i<len;++i)
	{
		vin=mic[i];
		vout=mem[0]+vin;
		mic[i]=radius*vout;
		mem[0]=mem[1]+2*(-vin+mic[i]);
		mem[1]=vin-den2*vout;
	}
}

float wtk_qform_pickup1_preemph(wtk_qform_pickup1_t *qform_pickup1,float *mic,int len,float memD)
{
	int i;
	float tmp;
	float preemph=qform_pickup1->cfg->preemph;

	for(i=0;i<len;++i)
	{
		tmp=mic[i]-preemph*memD;
		memD=mic[i];
		mic[i]=tmp;
	}
	return memD;
}

float wtk_qform_pickup1_preemph2(wtk_qform_pickup1_t *qform_pickup1,float *mic,int len,float memX)
{
	int i;
	float tmp;
	float preemph=qform_pickup1->cfg->preemph;

	for(i=0;i<len;++i)
	{
		tmp=mic[i]+preemph*memX;
        mic[i]=tmp;
		memX=mic[i];
	}
	return memX;
}

void wtk_qform_pickup1_set_notify(wtk_qform_pickup1_t *qform_pickup1,void *ths,wtk_qform_pickup1_notify_f notify)
{
    qform_pickup1->ths=ths;
    qform_pickup1->notify=notify;
}

void wtk_qform_pickup1_notify_data(wtk_qform_pickup1_t *qform_pickup1,float *data,int len,char *cohv)
{
    short *pv=(short *)data;
    int i;
    int k,n,b;
    int nbin=qform_pickup1->bf->nbin;
    char bb[8]={0x7f, 0xbf, 0xdf, 0xef, 0xf7, 0xfb, 0xfd, 0xfe};
    char cohv2[128];

    if(qform_pickup1->cfg->preemph>0)
    {
        qform_pickup1->memX=wtk_qform_pickup1_preemph2(qform_pickup1,data,len,qform_pickup1->memX);
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

    if(qform_pickup1->notify)
    {
        n=0;
        b=0;
        for(k=1;k<nbin-1;++k)
        {
            if(b==0)
            {
                cohv2[n]=0xff;
            }
            if(cohv[k]<0)
            {
                cohv2[n]=cohv2[n]&bb[b];
            }
            ++b;
            if(b==8)
            {
                b=0;
                ++n;
            }
        }
        qform_pickup1->notify(qform_pickup1->ths,pv,len,cohv2,(nbin-2)/8+1);
    }
}

void wtk_qform_pickup1_flush2(wtk_qform_pickup1_t *qform_pickup1,wtk_stft2_msg_t *smsg,char *cohv, int is_end)
{
    int k;
    wtk_complex_t *bf_out;
    // int nbin=qform_pickup1->bf->nbin;

    if(smsg && cohv)
    {
        bf_out=wtk_bf_output_fft2_msg(qform_pickup1->bf,smsg);
        k=wtk_stft2_output_ifft(qform_pickup1->stft2,bf_out,qform_pickup1->stft2->output,qform_pickup1->bf->pad,qform_pickup1->end_pos,is_end);
        if(k>=qform_pickup1->stft2->cfg->step)
        {
            wtk_qform_pickup1_notify_data(qform_pickup1,qform_pickup1->stft2->output,qform_pickup1->stft2->cfg->step,cohv);
        }
    }else
    {
        if(qform_pickup1->notify)
        {
            qform_pickup1->notify(qform_pickup1->ths,NULL,0,NULL,0);
        }
    } 
}

int wtk_qform_pickup1_update_rnn2(wtk_qform_pickup1_t *qform_pickup1, wtk_complex_t **fft,int k)
{
    int channel=qform_pickup1->bf->channel;
    int i,j;
    wtk_complex_t *fft1,*fft2,*fft3,*a;
    wtk_complex_t **scov=qform_pickup1->scov;
    float alpha=qform_pickup1->cfg->scov_alpha;
    float alpha_1=1-qform_pickup1->cfg->scov_alpha;
    // float ff;
    int ret=0;
    static int cnt[1024]={0};

    if(qform_pickup1->scov==NULL)
    {
        return ret;
    }

    // if(qform_pickup1->cfg->bf.use_eig_ovec)
    // {
    //     ff=0;
    //     for(i=0;i<channel;++i)
    //     {
    //         fft1=fft[i];
    //         ff+=fft1[k].a*fft1[k].a+fft1[k].b*fft1[k].b;
    //     }
    //     if(ff<0.0001)
    //     {
    //         return;
    //     }
    // }

    if(qform_pickup1->scnt_sum[k]>=qform_pickup1->cfg->init_scovnf)
    {
        ++qform_pickup1->scnt_sum[k];
        fft1=fft2=fft3=fft[k];
        for(i=0;i<channel;++i,++fft2)
        {
            fft3=fft1+i;
            for(j=i;j<channel;++j,++fft3)
            {
                a=scov[k]+i*channel+j;
                a->a=alpha_1*a->a+alpha*(fft2->a*fft3->a+fft2->b*fft3->b);
                a->b=alpha_1*a->b+alpha*(-fft2->a*fft3->b+fft2->b*fft3->a);
            }
        }
    }else
    {
        ++qform_pickup1->scnt_sum[k];
        fft1=fft2=fft3=fft[k];
        for(i=0;i<channel;++i,++fft2)
        {
            fft3=fft1+i;
            for(j=i;j<channel;++j,++fft3)
            {
                a=scov[k]+i*channel+j;
                a->a+=fft2->a*fft3->a+fft2->b*fft3->b;
                a->b+=-fft2->a*fft3->b+fft2->b*fft3->a;
            }
        }
    }
    if(qform_pickup1->scnt_sum[k]>5)
    {
        ++cnt[k];
        if(cnt[k]==qform_pickup1->cfg->flushcovgap)
        {
            wtk_bf_update_scov(qform_pickup1->bf,scov,k);
            cnt[k]=0;
            ret=1;
        }
    }

    return ret;
}


int wtk_qform_pickup1_update_rnn1(wtk_qform_pickup1_t *qform_pickup1,wtk_complex_t **fft,int k)
{
    int channel=qform_pickup1->bf->channel;
    int i,j;
    wtk_complex_t *fft1,*fft2,*fft3,*a;//,*b;
    wtk_complex_t **ncov=qform_pickup1->ncov;
    // wtk_complex_t **ncovtmp=qform_pickup1->ncovtmp;
    float alpha=qform_pickup1->cfg->ncov_alpha;
    float alpha_1=1-qform_pickup1->cfg->ncov_alpha;
    static int cnt[1024]={0};
    int ret;

    if(qform_pickup1->ncnt_sum[k]>=qform_pickup1->cfg->init_ncovnf)
    {
        ++qform_pickup1->ncnt_sum[k];
        fft1=fft2=fft3=fft[k];
        for(i=0;i<channel;++i,++fft2)
        {
            fft3=fft1+i;
            for(j=i;j<channel;++j,++fft3)
            {
                a=ncov[k]+i*channel+j;
                a->a=alpha_1*a->a+alpha*(fft2->a*fft3->a+fft2->b*fft3->b);
                a->b=alpha_1*a->b+alpha*(-fft2->a*fft3->b+fft2->b*fft3->a);
            }
        }
    }else
    {
        ++qform_pickup1->ncnt_sum[k];
        fft1=fft2=fft3=fft[k];
        for(i=0;i<channel;++i,++fft2)
        {
            fft3=fft1+i;
            for(j=i;j<channel;++j,++fft3)
            {
                a=ncov[k]+i*channel+j;
                a->a+=fft2->a*fft3->a+fft2->b*fft3->b;
                a->b+=-fft2->a*fft3->b+fft2->b*fft3->a;

                // b=ncov[k]+i*channel+j;
                // b->a=a->a/qform_pickup1->ncnt_sum[k];
                // b->b=a->b/qform_pickup1->ncnt_sum[k];
            }
        }
    }
    ret=0;
    if(qform_pickup1->ncnt_sum[k]>5)
    {
        ++cnt[k];
        if(cnt[k]==qform_pickup1->cfg->flushcovgap)
        {
            wtk_bf_update_ncov(qform_pickup1->bf,ncov,k);
            cnt[k]=0;
            ret=1;
        }
    }
    return ret;

}

void wtk_qform_pickup1_update_cov(wtk_complex_t *cov,wtk_complex_t **fft,int k, int channel)
{
    int i,j;
    wtk_complex_t *fft1,*fft2,*fft3,*a;//,*b;

    fft1=fft2=fft3=fft[k];
    for(i=0;i<channel;++i,++fft2)
    {
        fft3=fft1+i;
        for(j=i;j<channel;++j,++fft3)
        {
            a=cov+i*channel+j;
            a->a=fft2->a*fft3->a+fft2->b*fft3->b;
            a->b=-fft2->a*fft3->b+fft2->b*fft3->a;
        }
    }
}


void wtk_qform_pickup1_update_ncovmsg(wtk_queue_t *qncov_q, wtk_complex_t *ncov, int channel)
{
    int i,j,k;
    wtk_queue_node_t *qn;
    wtk_qform_pickup1_ncovmsg_t *msg;
    wtk_complex_t *a,*b;

    memset(ncov,0,sizeof(wtk_complex_t)*channel*channel);
    for(k=0; k<qncov_q->length; ++k)
    {
        qn=wtk_queue_peek(qncov_q, k);
        msg=(wtk_qform_pickup1_ncovmsg_t *)data_offset2(qn,wtk_qform_pickup1_ncovmsg_t,q_n);

        for(i=0;i<channel;++i)
        {
            for(j=i;j<channel;++j)
            {
                a=ncov+i*channel+j;
                b=msg->ncov+i*channel+j;
                a->a+=b->a;
                a->b+=b->b;
            }
        }
    }
}

void wtk_qform_pickup1_update_scovmsg(wtk_queue_t *qscov_q, wtk_complex_t *scov, int channel)
{
    int i,j,k;
    wtk_queue_node_t *qn;
    wtk_qform_pickup1_scovmsg_t *msg;
    wtk_complex_t *a,*b;

    memset(scov,0,sizeof(wtk_complex_t)*channel*channel);
    for(k=0; k<qscov_q->length; ++k)
    {
        qn=wtk_queue_peek(qscov_q, k);
        msg=(wtk_qform_pickup1_scovmsg_t *)data_offset2(qn,wtk_qform_pickup1_scovmsg_t,q_n);

        for(i=0;i<channel;++i)
        {
            for(j=i;j<channel;++j)
            {
                a=scov+i*channel+j;
                b=msg->scov+i*channel+j;
                a->a+=b->a;
                a->b+=b->b;
            }
        }
    }
}

void wtk_qform_pickup1_flush(wtk_qform_pickup1_t *qform_pickup1,wtk_stft2_msg_t *smsg, char *cohv, int is_end)
{
    int k;
    int nbin=qform_pickup1->bf->nbin;
    int i, channel=qform_pickup1->bf->channel;
    wtk_qform_pickup1_ncovmsg_t *qncmsg;
    wtk_qform_pickup1_scovmsg_t *qscmsg;
    wtk_queue_node_t *qn;
    wtk_queue_t *qsmsg_q=&(qform_pickup1->qsmsg_q);
    wtk_qform_pickup1_stft2msg_t *qsmsg;
    int b;

    if(qform_pickup1->cfg->use_covhist)
    {
        for(k=1;k<nbin-1;++k)
        {
            if(cohv[k]<0)
            {
                qncmsg=wtk_qform_pickup1_ncovmsg_new(channel);
                wtk_qform_pickup1_update_cov(qncmsg->ncov, smsg->fft, k ,channel);
                wtk_queue_push(&(qform_pickup1->qncov_q[k]), &(qncmsg->q_n));
                // wtk_qform_pickup1_update_ncovmsg(&(qform_pickup1->qncov_q[k]), qform_pickup1->ncov[k], channel);
                // qform_pickup1->ncnt_sum[k]=qform_pickup1->qncov_q[k].length;
                if(qform_pickup1->qncov_q[k].length == qform_pickup1->cfg->cov_hist)
                {
                    qn=wtk_queue_pop(&(qform_pickup1->qncov_q[k]));
                    qncmsg=(wtk_qform_pickup1_ncovmsg_t *)data_offset2(qn,wtk_qform_pickup1_ncovmsg_t,q_n);
                    wtk_qform_pickup1_ncovmsg_delete(qncmsg);
                }
            }else if(qform_pickup1->scov)
            {
                qscmsg=wtk_qform_pickup1_scovmsg_new(channel);
                wtk_qform_pickup1_update_cov(qscmsg->scov, smsg->fft, k ,channel);
                wtk_queue_push(&(qform_pickup1->qscov_q[k]), &(qscmsg->q_n));
                // wtk_qform_pickup1_update_scovmsg(&(qform_pickup1->qscov_q[k]), qform_pickup1->scov[k], channel);
                // qform_pickup1->scnt_sum[k]=qform_pickup1->qscov_q[k].length;
                if(qform_pickup1->qscov_q[k].length == qform_pickup1->cfg->cov_hist)
                {
                    qn=wtk_queue_pop(&(qform_pickup1->qscov_q[k]));
                    qscmsg=(wtk_qform_pickup1_scovmsg_t *)data_offset2(qn,wtk_qform_pickup1_scovmsg_t,q_n);
                    wtk_qform_pickup1_scovmsg_delete(qscmsg);
                }
            }
        }
        qsmsg=wtk_qform_pickup1_stft2msg_new(smsg,cohv,nbin,channel);
        wtk_queue_push(qsmsg_q,&(qsmsg->q_n));
        if(qsmsg_q->length == qform_pickup1->cfg->batch || is_end)
        {
            for(k=1;k<nbin-1;++k)
            {
                wtk_qform_pickup1_update_ncovmsg(&(qform_pickup1->qncov_q[k]), qform_pickup1->ncov[k], channel);
                qform_pickup1->ncnt_sum[k]=qform_pickup1->qncov_q[k].length;
                if(qform_pickup1->scov)
                {
                    wtk_qform_pickup1_update_scovmsg(&(qform_pickup1->qscov_q[k]), qform_pickup1->scov[k], channel);
                    qform_pickup1->scnt_sum[k]=qform_pickup1->qscov_q[k].length;
                }
                if(qform_pickup1->ncnt_sum[k]>5 && (qform_pickup1->scnt_sum==NULL ||  qform_pickup1->scnt_sum[k]>5))
                {
                    wtk_bf_update_ncov(qform_pickup1->bf,qform_pickup1->ncov,k);
                    if(qform_pickup1->scov)
                    {
                        wtk_bf_update_scov(qform_pickup1->bf,qform_pickup1->scov,k);
                    }
                    wtk_bf_update_w(qform_pickup1->bf,k);
                }
            } 
            while(qsmsg_q->length>0)
            {
                qn=wtk_queue_pop(qsmsg_q);
                if(!qn){break;}
                qsmsg=(wtk_qform_pickup1_stft2msg_t *)data_offset2(qn,wtk_qform_pickup1_stft2msg_t,q_n);
                wtk_qform_pickup1_flush2(qform_pickup1,qsmsg->smsg,qsmsg->cohv, (is_end && qsmsg_q->length==0)? 1:0);
                wtk_qform_pickup1_stft2msg_delete(qsmsg,qform_pickup1->stft2);
            }
        }
    }else
    {
        for(k=1;k<nbin-1;++k)
        {
            // wtk_debug("====>%d  %f\n",k, mask[k]);
            b=0;
            if(cohv[k]<0)
            {
               b=wtk_qform_pickup1_update_rnn1(qform_pickup1,smsg->fft,k);
                // for(i=0;i<channel;++i)
                // {
                //     smsg->fft[i][k].a=smsg->fft[i][k].b=0;
                // }
            }else
            {
                b=wtk_qform_pickup1_update_rnn2(qform_pickup1,smsg->fft,k);
            }
            if(b && qform_pickup1->ncnt_sum[k]>5 && (qform_pickup1->scnt_sum==NULL ||  qform_pickup1->scnt_sum[k]>5))
            {
                wtk_bf_update_w(qform_pickup1->bf,k);
            }

            if(qform_pickup1->cfg->debug)
            {
                if(cohv[k]<0)
                {
                    for(i=0;i<channel;++i)
                    {
                        qform_pickup1->bf->w[k][i].a=0;
                        qform_pickup1->bf->w[k][i].b=0;
                    }
                }else
                {
                    for(i=0;i<channel;++i)
                    {
                        qform_pickup1->bf->w[k][i].a=0;
                        qform_pickup1->bf->w[k][i].b=0;
                        if(i==0)
                        {
                            qform_pickup1->bf->w[k][i].a=1;
                        }
                    }
                }
            }
        } 
        wtk_qform_pickup1_flush2(qform_pickup1,smsg,cohv, is_end);
    }
}

void wtk_qform_pickup1_update_aspec2(wtk_qform_pickup1_t *qform_pickup1, wtk_aspec_t *aspec, wtk_complex_t *cov, 
                                                                                    wtk_complex_t *inv_cov, float cov_travg, int k, float *spec_k, char *cohv)
{
    int sang_num=aspec->start_ang_num;
    int n;

    for(n=0; n<sang_num; ++n)
    {
        spec_k[n]=wtk_aspec_flush_spec_k(aspec, NULL, 0, cov_travg, cov, inv_cov, k ,n);
    }

    cohv[k]=1;
    for(n=1; n<sang_num; ++n)
    {
        if(spec_k[0]<=spec_k[n])
        {
            cohv[k]=-1;
        }
    }
    if(qform_pickup1->cfg->cohv_thresh>0.0 && spec_k[0]<qform_pickup1->cfg->cohv_thresh)
    {
        cohv[k]=-1;
    }
}


void wtk_qform_pickup1_update_naspec2(wtk_qform_pickup1_t *qform_pickup1, wtk_aspec_t *naspec, wtk_complex_t *cov, 
                                                                                    wtk_complex_t *inv_cov, float cov_travg, int k, float *spec_k, char *cohv)
{
    int sang_num=naspec->start_ang_num;
    int n;

    for(n=0; n<sang_num; ++n)
    {
        spec_k[n]=wtk_aspec_flush_spec_k(naspec, NULL, 0, cov_travg, cov, inv_cov, k ,n);
    }

    cohv[k]=-1;
    for(n=1; n<sang_num; ++n)
    {
        if(spec_k[0]<=spec_k[n])
        {
            cohv[k]=1;
        }
    }
}

void wtk_qform_pickup1_flush_aspec_lt(wtk_qform_pickup1_t *qform_pickup1, int index, int is_end)
{
    float **mic_class=qform_pickup1->cfg->mic_class;
    wtk_complex_t *covclass, *invcovclass;
    wtk_queue_t *stft2_q=&(qform_pickup1->stft2_q);
    int lf=qform_pickup1->cfg->lf;
    int i,j,k,k2,tt,ff,c,c2;
    wtk_queue_node_t *qn;
    wtk_stft2_msg_t *smsg,*smsg_index;
    int nbin=qform_pickup1->stft2->nbin;
    int channel=qform_pickup1->stft2->cfg->channel, channel2;
    wtk_complex_t *cov=qform_pickup1->cov;
    wtk_complex_t **fft,*fft1,*fft2,*fft3,*a,*b;
    float *wint=qform_pickup1->wint;
    float *winf=qform_pickup1->winf;
    float wint2,wintf,winsum;
    wtk_complex_t *inv_cov=qform_pickup1->inv_cov;
    wtk_dcomplex_t *tmp=qform_pickup1->tmp;
    float cov_travg;
    int ret;
    char *cohv=qform_pickup1->cohv;
    float spec_k[3];//, specsum;
    // int specsum_ns=qform_pickup1->cfg->specsum_ns;
    // int specsum_ne=qform_pickup1->cfg->specsum_ne;
    int ntheta_num=qform_pickup1->cfg->ntheta_num;
    static int cnt=0;
    
    ++qform_pickup1->nframe;
    qn=wtk_queue_peek(stft2_q, index);
    smsg_index=data_offset2(qn,wtk_stft2_msg_t,q_n);
    
    // specsum=0;
    ++cnt;
    if(cnt==qform_pickup1->cfg->flushcohvgap)
    {
        cnt=0;
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

            if(qform_pickup1->aspec && qform_pickup1->naspec)
            {
                cov_travg=0;
                if(qform_pickup1->aspec->need_cov_travg) 
                {
                    for(i=0;i<channel;++i)
                    {
                        cov_travg+=cov[i*channel+i].a;
                    }
                    cov_travg/=channel;
                }

                wtk_qform_pickup1_update_aspec2(qform_pickup1, qform_pickup1->aspec, cov, inv_cov, cov_travg, k, spec_k, cohv);

                // if(k>=specsum_ns && k<=specsum_ne  && spec_k[0]>spec_k[1] && spec_k[0]>spec_k[2])
                // {
                //     specsum+=spec_k[0]*2-spec_k[1]-spec_k[2];
                // }

                if(cohv[k]>0)
                {
                    for(i=0; i<ntheta_num; ++i)
                    {
                        wtk_qform_pickup1_update_naspec2(qform_pickup1, qform_pickup1->naspec[i], cov, inv_cov, cov_travg, k, spec_k, cohv);

                        if(cohv[k]<0)
                        {
                            break;
                        }
                    }
                }
            }else if(qform_pickup1->aspec)
            {
                cov_travg=0;
                if(qform_pickup1->aspec->need_cov_travg) 
                {
                    for(i=0;i<channel;++i)
                    {
                        cov_travg+=cov[i*channel+i].a;
                    }
                    cov_travg/=channel;
                }

                wtk_qform_pickup1_update_aspec2(qform_pickup1, qform_pickup1->aspec, cov, inv_cov, cov_travg, k, spec_k, cohv);

                // if(k>=specsum_ns && k<=specsum_ne  && spec_k[0]>spec_k[1] && spec_k[0]>spec_k[2])
                // {
                //     specsum+=spec_k[0]*2-spec_k[1]-spec_k[2];
                // }
            }else if(qform_pickup1->naspec)
            {
                cov_travg=0;
                if(qform_pickup1->naspec[0]->need_cov_travg)
                {
                    for(i=0;i<channel;++i)
                    {
                        cov_travg+=cov[i*channel+i].a;
                    }
                    cov_travg/=channel;
                }

                for(i=0; i<ntheta_num; ++i)
                {
                    wtk_qform_pickup1_update_naspec2(qform_pickup1, qform_pickup1->naspec[i], cov, inv_cov, cov_travg, k, spec_k, cohv);

                    if(cohv[k]<0)
                    {
                        break;
                    }
                }
            }else if(qform_pickup1->aspec_class[0])
            {
                channel2=qform_pickup1->cfg->aspec_class1.channel;
                covclass=qform_pickup1->covclass[0];

                for(i=0;i<channel2;++i)
                {
                    c=mic_class[0][i];
                    for(j=i;j<channel2;++j)
                    {
                        c2=mic_class[0][j];
                        a=covclass+i*channel2+j;
                        b=cov+c*channel+c2;
                        a->a=b->a;
                        a->b=b->b;

                        if(i!=j)
                        {
                            b=covclass+j*channel2+i;
                            b->a=a->a;
                            b->b=-a->b;
                        }
                    }
                }

                invcovclass=qform_pickup1->invcovclass[0];
                if(invcovclass)
                {
                    ret=wtk_complex_invx4(covclass,tmp,channel2,invcovclass,1);            
                    if(ret!=0)
                    {
                        j=0;
                        for(i=0;i<channel2;++i)
                        {
                            covclass[j].a+=0.01;
                            j+=channel2+1;
                        }
                        wtk_complex_invx4(covclass,tmp,channel2,invcovclass,1);
                    }
                }

                cov_travg=0;
                if(qform_pickup1->aspec_class[0]->need_cov_travg)
                {
                    for(i=0;i<channel2;++i)
                    {
                        cov_travg+=covclass[i*channel2+i].a;
                    }
                    cov_travg/=channel2;
                }

                wtk_qform_pickup1_update_aspec2(qform_pickup1, qform_pickup1->aspec_class[0], covclass, invcovclass, cov_travg, k, spec_k, cohv);

                if(cohv[k]>0)
                {
                    channel2=qform_pickup1->cfg->aspec_class2.channel;
                    covclass=qform_pickup1->covclass[1];

                    for(i=0;i<channel2;++i)
                    {
                        c=mic_class[1][i];
                        for(j=i;j<channel2;++j)
                        {
                            c2=mic_class[1][j];
                            a=covclass+i*channel2+j;
                            b=cov+c*channel+c2;
                            a->a=b->a;
                            a->b=b->b;

                            if(i!=j)
                            {
                                b=covclass+j*channel2+i;
                                b->a=a->a;
                                b->b=-a->b;
                            }
                        }
                    }

                    invcovclass=qform_pickup1->invcovclass[1];
                    if(invcovclass)
                    {
                        ret=wtk_complex_invx4(covclass,tmp,channel2,invcovclass,1);            
                        if(ret!=0)
                        {
                            j=0;
                            for(i=0;i<channel2;++i)
                            {
                                covclass[j].a+=0.01;
                                j+=channel2+1;
                            }
                            wtk_complex_invx4(covclass,tmp,channel2,invcovclass,1);
                        }
                    }

                    cov_travg=0;
                    if(qform_pickup1->aspec_class[1]->need_cov_travg)
                    {
                        for(i=0;i<channel2;++i)
                        {
                            cov_travg+=covclass[i*channel2+i].a;
                        }
                        cov_travg/=channel2;
                    }

                    wtk_qform_pickup1_update_aspec2(qform_pickup1, qform_pickup1->aspec_class[1], covclass, invcovclass, cov_travg, k, spec_k, cohv);
                }
            }
        }
    }
    // if(qform_pickup1->cohv_fn)
    // {
    //     fprintf(qform_pickup1->cohv_fn,"%.0f %f\n",qform_pickup1->nframe,specsum);
    // }
    wtk_qform_pickup1_flush(qform_pickup1, smsg_index, cohv, is_end);
}


void wtk_qform_pickup1_update_aspec(wtk_qform_pickup1_t *qform_pickup1, wtk_aspec_t *aspec, wtk_complex_t **fft, float fftabs2, 
                                                                                                                                    int k, float *spec_k, char *cohv)
{
    int sang_num=aspec->start_ang_num;
    int n;

    for(n=0; n<sang_num; ++n)
    {
        spec_k[n]=wtk_aspec_flush_spec_k(aspec, fft, fftabs2, 0, NULL, NULL, k ,n);
    }

    cohv[k]=1;
    for(n=1; n<sang_num; ++n)
    {
        if(spec_k[0]<=spec_k[n])
        {
            cohv[k]=-1;
        }
    }
    if(qform_pickup1->cfg->cohv_thresh>0.0 && spec_k[0]<qform_pickup1->cfg->cohv_thresh)
    {
        cohv[k]=-1;
    }
}


void wtk_qform_pickup1_update_naspec(wtk_qform_pickup1_t *qform_pickup1, wtk_aspec_t *naspec, wtk_complex_t **fft, float fftabs2, 
                                                                                                                                            int k, float *spec_k, char *cohv)
{
    int sang_num=naspec->start_ang_num;
    int n;

    for(n=0; n<sang_num; ++n)
    {
        spec_k[n]=wtk_aspec_flush_spec_k(naspec, fft, fftabs2, 0, NULL, NULL, k ,n);
    }

    cohv[k]=-1;
    for(n=1; n<sang_num; ++n)
    {
        if(spec_k[0]<=spec_k[n])
        {
            cohv[k]=1;
        }
    }
}

void wtk_qform_pickup1_flush_aspec(wtk_qform_pickup1_t *qform_pickup1, wtk_stft2_msg_t *msg, int is_end)
{
    int k,i,c;
    int nbin=qform_pickup1->stft2->nbin;
    int channel=qform_pickup1->stft2->cfg->channel, ch1,ch2;
    wtk_complex_t **fft=msg->fft, *fft2;
    wtk_complex_t **fftclass, **fftclass2;
    float **mic_class=qform_pickup1->cfg->mic_class;
    float fftabs2;
    float spec_k[3];//, specsum;
    char *cohv=qform_pickup1->cohv;
    // int specsum_ns=qform_pickup1->cfg->specsum_ns;
    // int specsum_ne=qform_pickup1->cfg->specsum_ne;
    int ntheta_num=qform_pickup1->cfg->ntheta_num;
    static int cnt=0;

    ++qform_pickup1->nframe;

    if(qform_pickup1->aspec_class[0])
    {
        fftclass=qform_pickup1->fftclass[0];
        fftclass2=qform_pickup1->fftclass[1];
        ch1=qform_pickup1->cfg->aspec_class1.channel;
        ch2=qform_pickup1->cfg->aspec_class2.channel;

        for(k=1; k<nbin-1; ++k)
        {
            for(i=0;i<ch1;++i)
            {
                c=mic_class[0][i];
                fftclass[k][i]=fft[k][c];
            }
            for(i=0;i<ch2;++i)
            {
                c=mic_class[1][i];
                fftclass2[k][i]=fft[k][c];
            }
        }   
    }

    // specsum=0;
    ++cnt;
    if(cnt==qform_pickup1->cfg->flushcohvgap)
    {
        cnt=0;
        for(k=1; k<nbin-1; ++k)
        {
            if(qform_pickup1->aspec && qform_pickup1->naspec)
            {
                fftabs2=0;
                if(!qform_pickup1->aspec->cfg->use_quick)
                {
                    fft2=fft[k];
                    for(i=0; i<channel; ++i,++fft2)
                    {
                        fftabs2+=fft2->a*fft2->a+fft2->b*fft2->b;
                    }
                }

                wtk_qform_pickup1_update_aspec(qform_pickup1,qform_pickup1->aspec,fft,fftabs2,k,spec_k,cohv);

                // if(k>=specsum_ns && k<=specsum_ne && spec_k[0]>spec_k[1] &&spec_k[0]>spec_k[2])
                // {
                //     specsum+=spec_k[0]*2-spec_k[1]-spec_k[2];
                // }

                if(cohv[k]>0)
                {
                    for(i=0; i<ntheta_num; ++i)
                    {
                        wtk_qform_pickup1_update_naspec(qform_pickup1,qform_pickup1->naspec[i],fft,fftabs2,k,spec_k,cohv);

                        if(cohv[k]<0)
                        {
                            break;
                        }
                    }
                }
            }else if(qform_pickup1->aspec)
            {
                fftabs2=0;
                if(!qform_pickup1->aspec->cfg->use_quick)
                {
                    fft2=fft[k];
                    for(i=0; i<channel; ++i,++fft2)
                    {
                        fftabs2+=fft2->a*fft2->a+fft2->b*fft2->b;
                    }
                }

                wtk_qform_pickup1_update_aspec(qform_pickup1,qform_pickup1->aspec,fft,fftabs2,k,spec_k,cohv);

                // if(k>=specsum_ns && k<=specsum_ne && spec_k[0]>spec_k[1] && spec_k[0]>spec_k[2])
                // {
                //     specsum+=spec_k[0]*2-spec_k[1]-spec_k[2];
                // }
            }else if(qform_pickup1->naspec)
            {
                fftabs2=0;
                if(!qform_pickup1->aspec->cfg->use_quick)
                {
                    fft2=fft[k];
                    for(i=0; i<channel; ++i,++fft2)
                    {
                        fftabs2+=fft2->a*fft2->a+fft2->b*fft2->b;
                    }
                }

                for(i=0; i<ntheta_num; ++i)
                {
                    wtk_qform_pickup1_update_naspec(qform_pickup1,qform_pickup1->naspec[i],fft,fftabs2,k,spec_k,cohv);

                    if(cohv[k]<0)
                    {
                        break;
                    }
                }
            }else if(qform_pickup1->aspec_class[0])
            {
                fftclass=qform_pickup1->fftclass[0];
                ch1=qform_pickup1->cfg->aspec_class1.channel;

                fftabs2=0;
                fft2=fftclass[k];
                if(!qform_pickup1->aspec_class[0]->cfg->use_quick)
                {
                    for(i=0; i<ch1; ++i, ++fft2)
                    {
                        fftabs2+=fft2->a*fft2->a+fft2->b*fft2->b;
                    }
                }
                wtk_qform_pickup1_update_aspec(qform_pickup1,qform_pickup1->aspec_class[0],fftclass,fftabs2,k,spec_k,cohv);

                if(cohv[k]>0)
                {
                    fftclass2=qform_pickup1->fftclass[1];
                    ch2=qform_pickup1->cfg->aspec_class2.channel;

                    fftabs2=0;
                    fft2=fftclass2[k];
                    if(!qform_pickup1->aspec_class[1]->cfg->use_quick)
                    {
                        for(i=0; i<ch1; ++i, ++fft2)
                        {
                            fftabs2+=fft2->a*fft2->a+fft2->b*fft2->b;
                        }
                    }
                    wtk_qform_pickup1_update_aspec(qform_pickup1,qform_pickup1->aspec_class[1],fftclass2,fftabs2,k,spec_k,cohv);
                }
            }
        }
    }
    // if(qform_pickup1->cohv_fn)
    // {
    //     fprintf(qform_pickup1->cohv_fn,"%.0f %f\n",qform_pickup1->nframe,specsum);
    // }
    wtk_qform_pickup1_flush(qform_pickup1, msg, cohv, is_end);
}

void wtk_qform_pickup1_on_stft2(wtk_qform_pickup1_t *qform_pickup1,wtk_stft2_msg_t *msg,int pos,int is_end)
{
    wtk_queue_t *stft2_q=&(qform_pickup1->stft2_q);
    int lt=qform_pickup1->cfg->lt;
    wtk_queue_node_t *qn;
    wtk_stft2_msg_t *smsg;
    int i;

    if(is_end)
    {
        qform_pickup1->end_pos=pos;
    }
    if(qform_pickup1->cov)
    {
        if(msg)
        {
            wtk_queue_push(stft2_q,&(msg->q_n));
        }
        if(stft2_q->length>=lt+1 && stft2_q->length<2*lt+1)
        {
            wtk_qform_pickup1_flush_aspec_lt(qform_pickup1,stft2_q->length-lt-1, 0);
        }else if(stft2_q->length==2*lt+1)
        {
            wtk_qform_pickup1_flush_aspec_lt(qform_pickup1,stft2_q->length-lt-1, (is_end && lt==0)?1: 0);
            qn=wtk_queue_pop(stft2_q);
            smsg=data_offset2(qn,wtk_stft2_msg_t,q_n);
            wtk_stft2_push_msg(qform_pickup1->stft2,smsg);
        }else if(is_end && stft2_q->length==0)
        {
            wtk_qform_pickup1_flush2(qform_pickup1, NULL, NULL, 1);
        }
        if(is_end)
        {
            if(stft2_q->length>0)
            {
                if(stft2_q->length<lt+1)
                {
                    for(i=0; i<stft2_q->length-1; ++i)
                    {
                        wtk_qform_pickup1_flush_aspec_lt(qform_pickup1, i, 0);
                    }
                    wtk_qform_pickup1_flush_aspec_lt(qform_pickup1, stft2_q->length-1, 1);
                }else
                {
                    for(i=0; i<lt-1; ++i)
                    {
                        wtk_qform_pickup1_flush_aspec_lt(qform_pickup1,stft2_q->length-lt+i, 0);   
                    }
                    wtk_qform_pickup1_flush_aspec_lt(qform_pickup1,stft2_q->length-1, 1);
                }
            }
            while(qform_pickup1->stft2_q.length>0)
            {
                qn=wtk_queue_pop(&(qform_pickup1->stft2_q));
                if(!qn){break;}
                smsg=(wtk_stft2_msg_t *)data_offset(qn,wtk_stft2_msg_t,q_n);
                wtk_stft2_push_msg(qform_pickup1->stft2,smsg);
            }
        }
    }else
    {
        if(msg)
        {
            wtk_qform_pickup1_flush_aspec(qform_pickup1,msg,is_end);
            wtk_stft2_push_msg(qform_pickup1->stft2, msg); 
        }else if(is_end && !msg)
        {
            wtk_qform_pickup1_flush2(qform_pickup1, NULL, NULL, 1);
        }
    }   
}

void wtk_qform_pickup1_start_aspec1(wtk_aspec_t *aspec, float theta, float phi, float theta_range)
{
    float theta2, theta3;

    theta2=theta+2*theta_range;
    if(theta2>=360)
    {
        theta2-=360;
    }
    theta3=theta-2*theta_range;
    if(theta3<0)
    {
        theta3+=360;
    }
    aspec->start_ang_num=3;
    wtk_aspec_start(aspec, theta, 0, 0);
    wtk_aspec_start(aspec, theta2, 0, 1);
    wtk_aspec_start(aspec, theta3, 0, 2);   
}

void wtk_qform_pickup1_start_aspec2(wtk_aspec_t *aspec, float theta, float phi, float theta_range)
{
    float theta2, theta3;

    if(theta==0 || theta==180)
    {
        theta2=theta+2*theta_range;
        if(theta2>180)
        {
            theta2=360-theta2;
        }

        aspec->start_ang_num=2;
        wtk_aspec_start(aspec, theta, 0, 0);
        wtk_aspec_start(aspec, theta2, 0, 1);
    }else
    {
        theta2=theta+2*theta_range;
        if(theta2>180)
        {
            if(theta+theta_range>=180)
            {
                theta2=-1;
            }else
            {
                theta2=180;   
            }
        }
        theta3=theta-2*theta_range;
        if(theta3<0)
        {
            if(theta-theta_range<=0)
            {
                theta3=-1;
            }else
            {
                theta3=0;
            }
        }
        if(theta2==-1 || theta3==-1)
        {
            aspec->start_ang_num=2;
            wtk_aspec_start(aspec, theta, 0, 0);
            wtk_aspec_start(aspec, theta2==-1?theta3:theta2, 0, 1);
        }else
        {
            aspec->start_ang_num=3;
            wtk_aspec_start(aspec, theta, 0, 0);
            wtk_aspec_start(aspec, theta2, 0, 1);
            wtk_aspec_start(aspec, theta3, 0, 2);   
        }
    }
}

void wtk_qform_pickup1_start(wtk_qform_pickup1_t *qform_pickup1, float theta, float phi)
{
    int i;

    qform_pickup1->theta=theta;
    qform_pickup1->phi=phi;

    wtk_bf_update_ovec(qform_pickup1->bf,theta,phi);
    wtk_bf_init_w(qform_pickup1->bf);

    if(qform_pickup1->cfg->use_two_aspecclass)
    {
        if(qform_pickup1->cfg->use_cline1)
        {
            wtk_qform_pickup1_start_aspec2(qform_pickup1->aspec_class[0], theta, 0, qform_pickup1->cfg->theta_range);
        }else
        {      
            wtk_qform_pickup1_start_aspec1(qform_pickup1->aspec_class[0], theta, 0, qform_pickup1->cfg->theta_range);
        }

        if(qform_pickup1->cfg->use_cline2)
        {
            wtk_qform_pickup1_start_aspec2(qform_pickup1->aspec_class[1], phi, 0, qform_pickup1->cfg->theta_range);
        }else
        {      
            wtk_qform_pickup1_start_aspec1(qform_pickup1->aspec_class[1], phi, 0, qform_pickup1->cfg->theta_range);
        }
    }else
    {
        if(qform_pickup1->cfg->use_line)
        {
            if(qform_pickup1->cfg->use_noiseblock || qform_pickup1->cfg->use_noiseblock2)
            {
                for(i=0; i<qform_pickup1->cfg->ntheta_num; ++i)
                {
                    wtk_qform_pickup1_start_aspec2(qform_pickup1->naspec[i], qform_pickup1->cfg->ntheta_center[i], phi, qform_pickup1->cfg->ntheta_range);
                }
            }

            if(!qform_pickup1->cfg->use_noiseblock || qform_pickup1->cfg->use_noiseblock2)
            {
                wtk_qform_pickup1_start_aspec2(qform_pickup1->aspec, theta, phi, qform_pickup1->cfg->theta_range);
            }
        }else
        {
            if(qform_pickup1->cfg->use_noiseblock || qform_pickup1->cfg->use_noiseblock2)
            {
                for(i=0; i<qform_pickup1->cfg->ntheta_num; ++i)
                {
                    wtk_qform_pickup1_start_aspec1(qform_pickup1->naspec[i], qform_pickup1->cfg->ntheta_center[i], phi, qform_pickup1->cfg->ntheta_range);
                }
            }

            if(!qform_pickup1->cfg->use_noiseblock || qform_pickup1->cfg->use_noiseblock2)
            {         
                wtk_qform_pickup1_start_aspec1(qform_pickup1->aspec, theta, phi, qform_pickup1->cfg->theta_range);
            }
        }
    }
}

void wtk_qform_pickup1_feed2(wtk_qform_pickup1_t *qform,short **data,int len,int is_end)
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
            wtk_qform_pickup1_dc(qform,fp[i],qform->notch_mem[i],len);
        }
        if(qform->cfg->preemph>0)
        {
            qform->memD[i]=wtk_qform_pickup1_preemph(qform,fp[i],len,qform->memD[i]);
        }
    }
    wtk_stft2_feed_float(qform->stft2,fp,len,is_end);
}

void wtk_qform_pickup1_feed(wtk_qform_pickup1_t *qform_pickup1,short **data,int len,int is_end)
{
#ifdef DEBUG_WAV
	static wtk_wavfile_t *mic_log=NULL;

	if(!mic_log)
	{
		mic_log=wtk_wavfile_new(16000);
		wtk_wavfile_set_channel(mic_log,qform_pickup1->bf->channel);
		wtk_wavfile_open2(mic_log,"qform_pickup1");
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
    if(qform_pickup1->cfg->preemph>0 || qform_pickup1->cfg->notch_radius>0)
    {
        wtk_qform_pickup1_feed2(qform_pickup1,data,len,is_end);
    }else
    {
        wtk_stft2_feed(qform_pickup1->stft2,data,len,is_end);
    }
}


void wtk_qform_pickup1_set_grav(wtk_qform_pickup1_t *qform,short x,short y,short z)
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
        wtk_qform_pickup1_feed(qform,NULL,0,1);
        wtk_qform_pickup1_reset(qform);
        wtk_qform_pickup1_start(qform,theta,0);
    }
}