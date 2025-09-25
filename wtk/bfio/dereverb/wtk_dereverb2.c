#include "wtk_dereverb2.h" 

void wtk_dereverb2_on_stft(wtk_dereverb2_t *dereverb,wtk_stft2_msg_t *msg,int pos,int is_end);

wtk_dereverb2_t* wtk_dereverb2_new(wtk_dereverb2_cfg_t *cfg)
{
    wtk_dereverb2_t *dereverb;

    dereverb=(wtk_dereverb2_t *)wtk_malloc(sizeof(wtk_dereverb2_t));
    dereverb->ths=NULL;
    dereverb->notify=NULL;

    dereverb->cfg=cfg;
    dereverb->stft=wtk_stft2_new(&(cfg->stft));
    wtk_stft2_set_notify(dereverb->stft, dereverb, (wtk_stft2_notify_f)wtk_dereverb2_on_stft);

    dereverb->nbin=dereverb->stft->nbin;
    dereverb->channel=cfg->stft.channel;

    dereverb->input=NULL;
    if(cfg->preemph>0 || cfg->notch_radius>0)
    {
        dereverb->input=wtk_strbufs_new(cfg->stft.channel);
        dereverb->notch_mem=wtk_float_new_p2(cfg->stft.channel,2);
        dereverb->memD=(float *)wtk_malloc(sizeof(float)*cfg->stft.channel);
    }

    dereverb->nl=cfg->L*dereverb->channel;
    dereverb->norig=wtk_complex_new_p2(dereverb->nbin, dereverb->nl+cfg->D*dereverb->channel);

    dereverb->G=wtk_complex_new_p3(dereverb->nbin, dereverb->channel, dereverb->nl);
    dereverb->Q=wtk_complex_new_p3(dereverb->nbin, dereverb->nl, dereverb->nl);

    dereverb->K=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*dereverb->nl);
    dereverb->tmp=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*dereverb->nl);

    dereverb->E=wtk_complex_new_p2(dereverb->nbin, dereverb->channel);
    dereverb->out=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*dereverb->nbin);

    dereverb->Y=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*dereverb->channel);
    dereverb->Yf=(float *)wtk_malloc(sizeof(float)*dereverb->nbin);

    dereverb->wx=wtk_complex_new_p2(dereverb->nbin, dereverb->channel);

    dereverb->bf=wtk_bf_new(&(cfg->bf), cfg->stft.win);
    wtk_bf_update_ovec(dereverb->bf, cfg->theta, cfg->phi);
    wtk_bf_init_w(dereverb->bf);

    dereverb->Se=(float *)wtk_malloc(sizeof(float)*dereverb->nbin);
    dereverb->Sd=(float *)wtk_malloc(sizeof(float)*dereverb->nbin);
    dereverb->Sed=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*dereverb->nbin);

    dereverb->qmmse=NULL;
    if(cfg->use_post)
    {
        dereverb->qmmse=wtk_qmmse_new(&(cfg->qmmse));
    }

    wtk_dereverb2_reset(dereverb);

    return dereverb;
}

void wtk_dereverb2_reset(wtk_dereverb2_t *dereverb)
{
	int i,k;
    int channel=dereverb->channel;
    int nbin=dereverb->nbin;

    dereverb->nframe=0;
    dereverb->end_pos=0;

    wtk_stft2_reset(dereverb->stft);

    if(dereverb->input)
    {
        for(i=0;i<channel;++i)
        {
            memset(dereverb->notch_mem[i],0,2*sizeof(float));
        }
        memset(dereverb->memD,0,(channel)*sizeof(float));
        dereverb->memX=0;

        if(dereverb->input)
        {
            wtk_strbufs_reset(dereverb->input, dereverb->channel);
        }
    }

    wtk_complex_zero_p2(dereverb->norig, dereverb->nbin, dereverb->nl+dereverb->cfg->D*channel);
    wtk_complex_zero_p3(dereverb->G, dereverb->nbin, dereverb->channel, dereverb->nl);
    wtk_complex_zero_p3(dereverb->Q, dereverb->nbin, dereverb->nl, dereverb->nl);
    wtk_complex_zero_p2(dereverb->E, dereverb->nbin, dereverb->channel);

    memset(dereverb->Se, 0, sizeof(float)*dereverb->nbin);
    memset(dereverb->Sd, 0, sizeof(float)*dereverb->nbin);
    memset(dereverb->Sed, 0, sizeof(wtk_complex_t)*dereverb->nbin);

    for(k=0; k<nbin; ++k)
    {
        for(i=0; i<dereverb->nl; ++i)
        {
            dereverb->Q[k][i][i].a=1;
            dereverb->Q[k][i][i].b=0;
        }
    }
    if(dereverb->qmmse)
    {
        wtk_qmmse_reset(dereverb->qmmse);
    }
}

void wtk_dereverb2_delete(wtk_dereverb2_t *dereverb)
{
    wtk_stft2_delete(dereverb->stft);
    if(dereverb->bf)
    {
        wtk_bf_delete(dereverb->bf);
    }
    if(dereverb->qmmse)
    {
        wtk_qmmse_delete(dereverb->qmmse);
    }

    wtk_complex_delete_p2(dereverb->norig, dereverb->nbin);
    wtk_complex_delete_p3(dereverb->G, dereverb->nbin, dereverb->channel);
    wtk_complex_delete_p3(dereverb->Q, dereverb->nbin, dereverb->nl);
    wtk_complex_delete_p2(dereverb->E, dereverb->nbin);

    wtk_free(dereverb->out);

    wtk_free(dereverb->Y);
    wtk_free(dereverb->Yf);

    wtk_complex_delete_p2(dereverb->wx, dereverb->nbin);
    wtk_free(dereverb->K);
    wtk_free(dereverb->tmp);

    wtk_free(dereverb->Se);
    wtk_free(dereverb->Sd);
    wtk_free(dereverb->Sed);

    if(dereverb->input)
    {
        wtk_strbufs_delete(dereverb->input, dereverb->channel);
        wtk_float_delete_p2(dereverb->notch_mem, dereverb->channel);
        wtk_free(dereverb->memD);
    }

    wtk_free(dereverb);
}

void wtk_dereverb2_set_notify(wtk_dereverb2_t *dereverb,void *ths,wtk_dereverb2_notify_f notify)
{
    dereverb->notify=notify;
    dereverb->ths=ths;
}

void wtk_dereverb2_dc(wtk_dereverb2_t *dereverb2,float *mic,float *mem,int len)
{
	int i;
	float vin,vout;
	float radius=dereverb2->cfg->notch_radius;
	float den2=dereverb2->cfg->notch_radius_den;

	for(i=0;i<len;++i)
	{
		vin=mic[i];
		vout=mem[0]+vin;
		mic[i]=radius*vout;
		mem[0]=mem[1]+2*(-vin+mic[i]);
		mem[1]=vin-den2*vout;
	}
}

float wtk_dereverb2_preemph(wtk_dereverb2_t *dereverb2,float *mic,int len,float memD)
{
	int i;
	float tmp;
	float preemph=dereverb2->cfg->preemph;

	for(i=0;i<len;++i)
	{
		tmp=mic[i]-preemph*memD;
		memD=mic[i];
		mic[i]=tmp;
	}
	return memD;
}

float wtk_dereverb2_preemph2(wtk_dereverb2_t *dereverb2,float *mic,int len,float memX)
{
	int i;
	float tmp;
	float preemph=dereverb2->cfg->preemph;

	for(i=0;i<len;++i)
	{
		tmp=mic[i]+preemph*memX;
        mic[i]=tmp;
		memX=mic[i];
	}
	return memX;
}


void wtk_dereverb2_feed_dc(wtk_dereverb2_t *dereverb,short **data,int len,int is_end)
{   
    int i,j;
    int channel=dereverb->channel;
    float fv;
    float *fp[512];
    wtk_strbuf_t **input=dereverb->input;

    for(i=0;i<channel;++i)
    {
        wtk_strbuf_reset(input[i]);
        for(j=0;j<len;++j)
        {
            fv=data[i][j];
            wtk_strbuf_push(input[i],(char *)(&fv),sizeof(float));
        }
        fp[i]=(float *)(input[i]->data);
        
        if(dereverb->cfg->notch_radius>0)
	    {
            wtk_dereverb2_dc(dereverb, fp[i], dereverb->notch_mem[i], len);
        }
        if(dereverb->cfg->preemph>0)
        {
            dereverb->memD[i]=wtk_dereverb2_preemph(dereverb, fp[i], len, dereverb->memD[i]);
        }
    }

    wtk_stft2_feed_float(dereverb->stft,fp,len,is_end);
}

void wtk_dereverb2_feed(wtk_dereverb2_t *dereverb,short **mic,int len,int is_end)
{
#ifdef DEBUG_WAV
	static wtk_wavfile_t *mic_log=NULL;

	if(!mic_log)
	{
		mic_log=wtk_wavfile_new(16000);
		wtk_wavfile_set_channel(mic_log,dereverb->channel);
		wtk_wavfile_open2(mic_log,"dereverb");
	}
	if(len>0)
	{
		wtk_wavfile_write_mc(mic_log,mic,len);
	}
	if(is_end && mic_log)
	{
		wtk_wavfile_close(mic_log);
		wtk_wavfile_delete(mic_log);
		mic_log=NULL;
	}
#endif
    if(dereverb->cfg->preemph>0 || dereverb->cfg->notch_radius>0)
    {
        wtk_dereverb2_feed_dc(dereverb,mic,len,is_end);
    }else
    {
        wtk_stft2_feed2(dereverb->stft, mic, len, is_end);
    }
}

void wtk_dereverb2_update(wtk_dereverb2_t *dereverb, wtk_complex_t **fft)
{
    wtk_complex_t **norig=dereverb->norig, *norigtmp, *norigtmp2;
    wtk_complex_t ***G=dereverb->G, **Gtmp, *Gtmp2;
    wtk_complex_t ***Q=dereverb->Q, **Qtmp, *Qtmp2;
    wtk_complex_t *K=dereverb->K, *ktmp;
    wtk_complex_t **E=dereverb->E, *Etmp;

    wtk_complex_t *ffttmp;

    wtk_complex_t **wx=dereverb->wx, *wxtmp;
    double w;

    float *Yf=dereverb->Yf;
    wtk_complex_t *tmp=dereverb->tmp, *tmp2;

    float *Se=dereverb->Se;
    float *Sd=dereverb->Sd;
    wtk_complex_t *Sed=dereverb->Sed;

    wtk_complex_t **bfw, *bfwtmp;
    wtk_complex_t *out=dereverb->out, *outtmp;

    wtk_complex_t *Y=dereverb->Y, *Ytmp;
    wtk_complex_t Yout;

    int i,j,k;
    int channel=dereverb->channel;
    int nbin=dereverb->nbin;
    int nl=dereverb->nl;
    float sigma=dereverb->cfg->sigma;
    float p=dereverb->cfg->p;

    float wx_alpha=dereverb->cfg->wx_alpha;
    float coh_alpha=dereverb->cfg->coh_alpha;
    float lambda=dereverb->cfg->lambda;
    float lambdaf=1.0/lambda;
    float leak;
    float ef, yf;
    float leak_scale=dereverb->cfg->leak_scale;
    double fa,fa2,fb2;

    for(k=0; k<nbin; ++k)
    {
        norigtmp=norig[k];
        Gtmp=G[k];
        Etmp=E[k];
        ffttmp=fft[k];

        memset(Y, 0, sizeof(wtk_complex_t)*channel);
        Ytmp=Y;
        for(i=0; i<channel; ++i, ++Ytmp, ++Etmp)
        {
            Gtmp2=Gtmp[i];
            norigtmp2=norigtmp;
            for(j=0; j<nl; ++j, ++norigtmp2, ++Gtmp2)
            {
                Ytmp->a+=norigtmp2->a*Gtmp2->a-norigtmp2->b*Gtmp2->b;
                Ytmp->b+=norigtmp2->a*Gtmp2->b+norigtmp2->b*Gtmp2->a;
            }

            Etmp->a=ffttmp[i].a-Ytmp->a;
            Etmp->b=ffttmp[i].b-Ytmp->b;
        }

        wxtmp=wx[k];
        Etmp=E[k];
        if(dereverb->nframe==1)
        {
            memcpy(wxtmp, Etmp, sizeof(wtk_complex_t)*channel);
        }else
        {
            for(i=0; i<channel; ++i, ++Etmp, ++wxtmp)
            {
                wxtmp->a=(1-wx_alpha)*wxtmp->a+wx_alpha*Etmp->a;
                wxtmp->b=(1-wx_alpha)*wxtmp->b+wx_alpha*Etmp->b;
            }
        }

        wxtmp=wx[k];
        w=0;
        for(i=0; i<channel; ++i, ++wxtmp)
        {
            w+=wxtmp->a*wxtmp->a+wxtmp->b*wxtmp->b;
        }
        w=pow(w/channel+sigma, p/2-1);

        Qtmp=Q[k];
        ktmp=K;
        fa=0;
        norigtmp=norig[k];
        tmp2=tmp;

        for(i=0; i<nl; ++i, ++ktmp, ++norigtmp, ++tmp2)
        {
            norigtmp2=norig[k];
            Qtmp2=Qtmp[i];

            fa2=fb2=0;
            for(j=0; j<nl; ++j, ++Qtmp2, ++norigtmp2)
            {
                fa2+=Qtmp2->a*norigtmp2->a-Qtmp2->b*norigtmp2->b;
                fb2+=-Qtmp2->a*norigtmp2->b-Qtmp2->b*norigtmp2->a;
            }
            ktmp->a=fa2;
            ktmp->b=fb2;

            tmp2->a=fa2;
            tmp2->b=-fb2;
            
            fa+=fa2*norigtmp->a-fb2*norigtmp->b;
        }
        fa=1.0/(fa+lambda/w);

        ktmp=K;
        for(i=0; i<nl; ++i, ++ktmp)
        {
            ktmp->a*=fa;
            ktmp->b*=fa;
        }

        Qtmp=Q[k];
        tmp2=tmp;
        for(i=0; i<nl; ++i, ++tmp2)
        {
            ktmp=K+i;
            for(j=i; j<nl; ++j, ++ktmp)
            {
                if(i!=j)
                {
                    Qtmp2=Qtmp[i]+j;
                    Qtmp2->a=(Qtmp2->a-(ktmp->a*tmp2->a-ktmp->b*tmp2->b))*lambdaf;
                    Qtmp2->b=(Qtmp2->b-(ktmp->a*tmp2->b+ktmp->b*tmp2->a))*lambdaf;

                    Qtmp[j][i].a=Qtmp2->a;
                    Qtmp[j][i].b=-Qtmp2->b;
                }else
                {
                    Qtmp2=Qtmp[i]+j;
                    Qtmp2->a=(Qtmp2->a-(ktmp->a*tmp2->a-ktmp->b*tmp2->b))*lambdaf;
                    Qtmp2->b=0;
                }
            }
        }

        Gtmp=G[k];
        Etmp=E[k];
        for(j=0; j<channel; ++j, ++Etmp)
        {
            Gtmp2=Gtmp[j];
            ktmp=K;
            for(i=0; i<nl; ++i, ++ktmp, ++Gtmp2)
            {
                Gtmp2->a+=ktmp->a*Etmp->a-ktmp->b*Etmp->b;
                Gtmp2->b+=ktmp->a*Etmp->b+ktmp->b*Etmp->a;
            }
        }

        if(dereverb->channel>1)
        {
            bfw=dereverb->bf->w;
            bfwtmp=bfw[k];
            Etmp=E[k];
            outtmp=out+k;
            outtmp->a=outtmp->b=0;
            Ytmp=Y;
            Yout.a=Yout.b=0;
            for(i=0; i<channel; ++i, ++Etmp, ++Ytmp, ++bfwtmp)
            {
                outtmp->a+=bfwtmp->a*Etmp->a + bfwtmp->b*Etmp->b;
                outtmp->b+=bfwtmp->a*Etmp->b - bfwtmp->b*Etmp->a;

                Yout.a+=bfwtmp->a*Ytmp->a + bfwtmp->b*Ytmp->b;
                Yout.b+=bfwtmp->a*Ytmp->b - bfwtmp->b*Ytmp->a;
            }
        }else
        {
            Etmp=E[k];
            outtmp=out+k;
            Ytmp=Y;

            outtmp->a=Etmp->a;
            outtmp->b=Etmp->b;
            Yout.a=Ytmp->a;
            Yout.b=Ytmp->a;
        }
        
        if(dereverb->nframe==1)
        {
            Se[k]+=outtmp->a*outtmp->a+outtmp->b*outtmp->b;
            yf=Yout.a*Yout.a+Yout.b*Yout.b;
            Sd[k]+=yf;
            Sed[k].a+=Yout.a*outtmp->a+Yout.b*outtmp->b;
            Sed[k].b+=-Yout.a*outtmp->b+Yout.b*outtmp->a;

            leak=(Sed[k].a*Sed[k].a+Sed[k].b*Sed[k].b)/(Se[k]*Sd[k]+1e-9)*leak_scale;
            leak=max(0.005, leak);
            Yf[k]=leak*yf;
        }else
        {
            ef=outtmp->a*outtmp->a+outtmp->b*outtmp->b;
            yf=Yout.a*Yout.a+Yout.b*Yout.b;

            Se[k]=(1-coh_alpha)*Se[k]+coh_alpha*ef;
            Sd[k]=(1-coh_alpha)*Sd[k]+coh_alpha*yf;
            Sed[k].a=(1-coh_alpha)*Sed[k].a+coh_alpha*(Yout.a*outtmp->a+Yout.b*outtmp->b);
            Sed[k].b=(1-coh_alpha)*Sed[k].b+coh_alpha*(-Yout.a*outtmp->b+Yout.b*outtmp->a);

            leak=(Sed[k].a*Sed[k].a+Sed[k].b*Sed[k].b)/(Se[k]*Sd[k]+1e-9)*leak_scale;
            leak=max(0.005, leak);

            Yf[k]=leak*yf;
        }
    }
}


void wtk_dereverb2_notify_data(wtk_dereverb2_t *dereverb,float *data,int len,int is_end)
{
    short *pv;
    int i;

    if(dereverb->cfg->preemph>0)
    {
        dereverb->memX=wtk_dereverb2_preemph2(dereverb,data,len,dereverb->memX);
    }

    pv=(short *)data;
    for(i=0; i<len; ++i)
    {
        if(fabs(data[i])<32767.0)
        {
            pv[i]=data[i];
        }else
        {
            if(data[i]>0)
            {
                pv[i]=32767;
            }else
            {
                pv[i]=-32767;
            }
        }
    }
    dereverb->notify(dereverb->ths,pv,len,is_end);
}

void wtk_dereverb2_on_stft(wtk_dereverb2_t *dereverb,wtk_stft2_msg_t *msg,int pos,int is_end)
{
    int channel=dereverb->channel;
    int nbin=dereverb->nbin;
    int L=dereverb->cfg->L;
    int D=dereverb->cfg->D;
    int i,j,k,n1,n2;
    wtk_complex_t **fft, *fft1;
    wtk_complex_t **norig=dereverb->norig, *norigtmp;
    int len;

    if(!msg)
    {
        if(is_end && dereverb->notify)
        {
            dereverb->notify(dereverb->ths, NULL, 0, 1);
        }
        return;
    }
    if(is_end)
    {
        dereverb->end_pos=pos;
    }

    ++dereverb->nframe;
    fft=msg->fft;

    for(k=0; k<nbin; ++k)
    {
        fft1=fft[k];
        norigtmp=norig[k];

        for(i=0; i<L+D-1; ++i)
        {
            n1=i*channel;
            n2=(i+1)*channel;
            for(j=0; j<channel; ++j, ++n1,++n2)
            {
                norigtmp[n1].a=norigtmp[n2].a;
                norigtmp[n1].b=norigtmp[n2].b;
            }
        }
        n1=(L+D-1)*channel;
        for(j=0; j<channel; ++j, ++n1)
        {
            norigtmp[n1].a=fft1[j].a;
            norigtmp[n1].b=fft1[j].b;
        }
    }
    wtk_dereverb2_update(dereverb, fft);
    if(dereverb->qmmse)
    {
        wtk_qmmse_feed_echo_denoise(dereverb->qmmse, dereverb->out, dereverb->Yf);
    }
    len=wtk_stft2_output_ifft(dereverb->stft, dereverb->out, dereverb->stft->output, dereverb->bf->pad, dereverb->end_pos, is_end);
    if(dereverb->notify)
    {
        wtk_dereverb2_notify_data(dereverb, dereverb->stft->output, len, is_end);
    }

    wtk_stft2_push_msg(dereverb->stft, msg);
}
