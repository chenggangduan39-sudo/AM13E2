#include "wtk_qform_pickup2.h"

void wtk_qform_pickup2_on_stft(wtk_qform_pickup2_t *qform,wtk_stft_msg_t *msg,int pos,int is_end);

wtk_qform_pickup2_cohv_t *wtk_qform_pickup2_cohv_new(wtk_qform_pickup2_t *qform, char *cohv, int len)
{
    wtk_qform_pickup2_cohv_t *cohvm;
    int nbin=qform->stft->nbin;
    int k,b,n;
    char *cohv2;

    cohvm=(wtk_qform_pickup2_cohv_t *)wtk_malloc(sizeof(wtk_qform_pickup2_cohv_t));
    cohvm->cohv=(char *)wtk_malloc(sizeof(char)*nbin);

    cohv2=cohvm->cohv;
    cohv2[0]=0;
    cohv2[nbin-1]=0;

    b=8;
    n=0;
    for(k=1;k<nbin-1;++k)
    {
        --b;
        cohv2[k]=(cohv[n]>>b)&0x01;
        if(b==0)
        {
            ++n;
            b=8;
        }
    }

    return cohvm;
}

void wtk_qform_pickup2_cohv_delete(wtk_qform_pickup2_cohv_t *cohvm)
{
    wtk_free(cohvm->cohv);
    wtk_free(cohvm);
}

void wtk_qform_pickup2_dc(wtk_qform_pickup2_t *qform_pickup2,float *mic,float *mem,int len)
{
	int i;
	float vin,vout;
	float radius=qform_pickup2->cfg->notch_radius;
	float den2=qform_pickup2->cfg->notch_radius_den;

	for(i=0;i<len;++i)
	{
		vin=mic[i];
		vout=mem[0]+vin;
		mic[i]=radius*vout;
		mem[0]=mem[1]+2*(-vin+mic[i]);
		mem[1]=vin-den2*vout;
	}
}

float wtk_qform_pickup2_preemph(wtk_qform_pickup2_t *qform_pickup2,float *mic,int len,float memD)
{
	int i;
	float tmp;
	float preemph=qform_pickup2->cfg->preemph;

	for(i=0;i<len;++i)
	{
		tmp=mic[i]-preemph*memD;
		memD=mic[i];
		mic[i]=tmp;
	}
	return memD;
}

float wtk_qform_pickup2_preemph2(wtk_qform_pickup2_t *qform_pickup2,float *mic,int len,float memX)
{
	int i;
	float tmp;
	float preemph=qform_pickup2->cfg->preemph;

	for(i=0;i<len;++i)
	{
		tmp=mic[i]+preemph*memX;
        mic[i]=tmp;
		memX=mic[i];
	}
	return memX;
}

wtk_qform_pickup2_t* wtk_qform_pickup2_new(wtk_qform_pickup2_cfg_t *cfg)
{
    wtk_qform_pickup2_t *qform;

    qform=(wtk_qform_pickup2_t *)wtk_malloc(sizeof(wtk_qform_pickup2_t));
    qform->cfg=cfg;
    qform->ths=NULL;
    qform->notify=NULL;

    qform->stft=wtk_stft_new(&(cfg->stft));
    wtk_stft_set_notify(qform->stft,qform,(wtk_stft_notify_f)wtk_qform_pickup2_on_stft);

    qform->input=NULL;
    if(cfg->preemph>0 || cfg->notch_radius>0)
    {
        qform->input=wtk_strbuf_new(1024,2);
    }

    wtk_queue_init(&(qform->cohv_q));
    qform->qmmse=wtk_qmmse_new(&(cfg->qmmse));

    wtk_qform_pickup2_reset(qform);

    return qform;
}

void wtk_qform_pickup2_delete(wtk_qform_pickup2_t *qform)
{
    wtk_qform_pickup2_cohv_t *cohvm;
    wtk_queue_node_t *qn;

    while(qform->cohv_q.length>0)
    {
        qn=wtk_queue_pop(&(qform->cohv_q));
        cohvm=(wtk_qform_pickup2_cohv_t *)data_offset2(qn, wtk_qform_pickup2_cohv_t, q_n);
        wtk_qform_pickup2_cohv_delete(cohvm);
    }
    if(qform->input)
    {
        wtk_strbuf_delete(qform->input);
    }
    wtk_stft_delete(qform->stft);    
    wtk_qmmse_delete(qform->qmmse);
    wtk_free(qform);
}

void wtk_qform_pickup2_reset(wtk_qform_pickup2_t *qform)
{
    wtk_qform_pickup2_cohv_t *cohvm;
    wtk_queue_node_t *qn;

    while(qform->cohv_q.length>0)
    {
        qn=wtk_queue_pop(&(qform->cohv_q));
        cohvm=(wtk_qform_pickup2_cohv_t *)data_offset2(qn, wtk_qform_pickup2_cohv_t, q_n);
        wtk_qform_pickup2_cohv_delete(cohvm);
    }
    memset(qform->notch_mem,0,2*sizeof(float));
    qform->memD=0;
    qform->memX=0;

    if(qform->input)
    {
        wtk_strbuf_reset(qform->input);
    }
    wtk_stft_reset(qform->stft);
    wtk_qmmse_reset(qform->qmmse);
}

void wtk_qform_pickup2_set_notify(wtk_qform_pickup2_t *qform,void *ths,wtk_qform_pickup2_notify_f notify)
{
    qform->ths=ths;
    qform->notify=notify;
}

void wtk_qform_pickup2_notify_data(wtk_qform_pickup2_t *qform,float *data,int len)
{
    short *pv=(short *)data;
    int i;

    if(qform->cfg->preemph>0)
    {
        qform->memX=wtk_qform_pickup2_preemph2(qform,data,len,qform->memX);
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

void wtk_qform_pickup2_on_stft(wtk_qform_pickup2_t *qform,wtk_stft_msg_t *msg,int pos,int is_end)
{
    int k;
    wtk_qform_pickup2_cohv_t *cohvm;
    wtk_queue_node_t *qn;

    qn=wtk_queue_pop(&(qform->cohv_q));
    cohvm=(wtk_qform_pickup2_cohv_t *)data_offset2(qn, wtk_qform_pickup2_cohv_t, q_n);
    wtk_qmmse_feed_cohv2(qform->qmmse,msg->fft[0],cohvm->cohv);      
    k=wtk_stft_output_ifft2(qform->stft,msg->fft[0],1,pos,is_end,qform->stft->output[0],qform->stft->pad[0]);
    wtk_qform_pickup2_notify_data(qform,qform->stft->output[0],k);   
    wtk_qform_pickup2_cohv_delete(cohvm);
}

void wtk_qform_pickup2_feed2(wtk_qform_pickup2_t *qform,short *data,int len,int is_end)
{   
    int i;
    float fv;
    float *fp[1];
    wtk_strbuf_t *input=qform->input;

    wtk_strbuf_reset(input);
    for(i=0;i<len;++i)
    {
        fv=data[i]/32768.0;
        wtk_strbuf_push(input,(char *)(&fv),sizeof(float));
    }
    fp[0]=(float *)(input->data);
        
    if(qform->cfg->notch_radius>0)
    {
        wtk_qform_pickup2_dc(qform,fp[0],qform->notch_mem,len);
    }
    if(qform->cfg->preemph>0)
    {
        qform->memD=wtk_qform_pickup2_preemph(qform,fp[0],len,qform->memD);
    }

    wtk_stft_feed_float2(qform->stft,fp,len,is_end);
}

void wtk_qform_pickup2_feed(wtk_qform_pickup2_t *qform,short *data,int len,char *cohv,int len2,int is_end)
{
    wtk_qform_pickup2_cohv_t *cohvm;
    short *pv[1];
    // int nbin=qform->stft->nbin;

#ifdef DEBUG_WAV
	static wtk_wavfile_t *mic_log=NULL;

	if(!mic_log)
	{
		mic_log=wtk_wavfile_new(16000);
		wtk_wavfile_open2(mic_log,"qform_pickup2");
	}
	if(len>0)
	{
		wtk_wavfile_write(mic_log,(char *)data,len<<1);
	}
	if(is_end && mic_log)
	{
		wtk_wavfile_close(mic_log);
		wtk_wavfile_delete(mic_log);
		mic_log=NULL;
	}
#endif

    if(cohv)
    {
        cohvm=wtk_qform_pickup2_cohv_new(qform,cohv, len2);
        wtk_queue_push(&(qform->cohv_q), &(cohvm->q_n));
    }
    if(qform->cfg->preemph>0 || qform->cfg->notch_radius>0)
    {
        wtk_qform_pickup2_feed2(qform,data,len,is_end);
    }else
    {
        pv[0]=data;
        wtk_stft_feed(qform->stft,pv,len,is_end);
    }
}