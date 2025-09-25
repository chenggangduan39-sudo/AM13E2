#include "wtk_aec.h" 

void wtk_aec_on_stft(wtk_aec_t *aec,wtk_stft2_msg_t *msg,int pos,int is_end);

wtk_aec_t* wtk_aec_new(wtk_aec_cfg_t *cfg)
{
    wtk_aec_t *aec;
    int i;

    aec=(wtk_aec_t *)wtk_malloc(sizeof(wtk_aec_t));
    aec->ths=NULL;
    aec->notify=NULL;
    aec->notify2=NULL;

    aec->cfg=cfg;
    aec->stft=wtk_stft2_new(&(cfg->stft));
    wtk_stft2_set_notify(aec->stft, aec, (wtk_stft2_notify_f)wtk_aec_on_stft);

    aec->nbin=aec->stft->nbin;
    aec->channel=cfg->stft.channel-aec->cfg->spchannel;

    aec->pad=wtk_float_new_p2(aec->channel, aec->stft->cfg->win);
    aec->output=wtk_float_new_p2(aec->channel, aec->stft->cfg->win);

    aec->input=NULL;
    if(cfg->use_preemph)
    {
        aec->input=wtk_strbufs_new(cfg->stft.channel);
        aec->notch_mem=wtk_float_new_p2(cfg->stft.channel,2);
        aec->memD=(float *)wtk_malloc(sizeof(float)*cfg->stft.channel);
        aec->memX=(float *)wtk_malloc(sizeof(float)*aec->channel);
    }

    aec->nlms=NULL;
    aec->rls=NULL;
    if(cfg->use_nlms)
    {
        aec->nlms=(wtk_nlms_t *)wtk_malloc(sizeof(wtk_nlms_t)*aec->nbin);
        for(i=0;i<aec->nbin;++i)
        {
            wtk_nlms_init(aec->nlms+i, &(aec->cfg->nlms));
        }
    }else
    {
        aec->rls=(wtk_rls_t *)wtk_malloc(sizeof(wtk_rls_t)*aec->nbin);
        for(i=0;i<aec->nbin;++i)
        {
            wtk_rls_init(aec->rls+i, &(aec->cfg->rls));
        }
    }
    
    aec->out=wtk_complex_new_p2(aec->channel, aec->nbin);

    aec->Yf=NULL;
    aec->qmmse=NULL;
    aec->Se=NULL;
    aec->Sd=NULL;
    aec->Sed=NULL;
    if(cfg->use_post || cfg->use_aec_post)
    {
        aec->Yf=wtk_float_new_p2(aec->channel, aec->nbin);
        if(cfg->use_rls)
        {
            aec->Se=wtk_float_new_p2(aec->nbin, aec->channel);
            aec->Sd=wtk_float_new_p2(aec->nbin, aec->channel);
            aec->Sed=wtk_complex_new_p2(aec->nbin, aec->channel);
        }

        aec->qmmse=(wtk_qmmse_t **)wtk_malloc(sizeof(wtk_qmmse_t *)*aec->channel);
        for(i=0; i<aec->channel; ++i)
        {
            aec->qmmse[i]=wtk_qmmse_new(&(cfg->qmmse));
        }
    }

    wtk_aec_reset(aec);

    return aec;
}

void wtk_aec_reset(wtk_aec_t *aec)
{
	int i;
    int channel=aec->channel;
    int spchannel=aec->cfg->spchannel;
    int nbin=aec->nbin;

    aec->nframe=0;
    aec->end_pos=0;
    aec->sp_sil=1;
    aec->sp_silcnt=0;

    wtk_stft2_reset(aec->stft);

    wtk_float_zero_p2(aec->output, aec->channel, aec->stft->cfg->win);
    wtk_float_zero_p2(aec->pad, aec->channel, aec->stft->cfg->win);

    for(i=0;i<nbin;++i)
    {
        if(aec->nlms)
        {
            wtk_nlms_reset(aec->nlms+i);
        }else if(aec->rls)
        {
            wtk_rls_reset(aec->rls+i);
        }
        if(aec->qmmse && aec->rls)
        {
            memset(aec->Se[i], 0, sizeof(float)*aec->channel);
            memset(aec->Sd[i], 0, sizeof(float)*aec->channel);
            memset(aec->Sed[i], 0, sizeof(wtk_complex_t)*aec->channel);
        }
    }

    if(aec->qmmse)
    {
        for(i=0; i<channel; ++i)
        {
            wtk_qmmse_reset(aec->qmmse[i]);
        }
    }

    if(aec->input)
    {
        for(i=0;i<channel+spchannel;++i)
        {
            memset(aec->notch_mem[i],0,2*sizeof(float));
        }
        memset(aec->memD,0,(spchannel+channel)*sizeof(float));
        memset(aec->memX,0,channel*sizeof(float));

        if(aec->input)
        {
            wtk_strbufs_reset(aec->input, aec->channel+spchannel);
        }
    }
}

void wtk_aec_delete(wtk_aec_t *aec)
{
    int i;

    wtk_float_delete_p2(aec->output, aec->channel);
    wtk_float_delete_p2(aec->pad, aec->channel);
    wtk_stft2_delete(aec->stft);

    if(aec->qmmse)
    {
        for(i=0; i<aec->channel; ++i)
        {
            wtk_qmmse_delete(aec->qmmse[i]);
        }
        wtk_free(aec->qmmse);
        if(aec->rls)
        {
            wtk_float_delete_p2(aec->Se, aec->nbin);
            wtk_float_delete_p2(aec->Sd, aec->nbin);
            wtk_complex_delete_p2(aec->Sed, aec->nbin);
        }
    }

    if(aec->nlms)
    {
        for(i=0;i<aec->nbin;++i)
        {
            wtk_nlms_clean(aec->nlms+i);
        }
        wtk_free(aec->nlms);
    }else if(aec->rls)
    {
        for(i=0;i<aec->nbin;++i)
        {
            wtk_rls_clean(aec->rls+i);
        }
        wtk_free(aec->rls);
    }

    wtk_complex_delete_p2(aec->out, aec->channel);
    if(aec->Yf)
    {
        wtk_float_delete_p2(aec->Yf, aec->channel);
    }

    if(aec->input)
    {
        wtk_strbufs_delete(aec->input, aec->channel+aec->cfg->spchannel);
        wtk_float_delete_p2(aec->notch_mem, aec->channel+aec->cfg->spchannel);
        wtk_free(aec->memD);
        wtk_free(aec->memX);
    }

    wtk_free(aec);
}

void wtk_aec_set_notify(wtk_aec_t *aec,void *ths,wtk_aec_notify_f notify)
{
    aec->notify=notify;
    aec->ths=ths;
}

void wtk_aec_feed_dc(wtk_aec_t *aec,short **data,int len,int is_end)
{   
    int i,j;
    int channel=aec->channel;
    int channel2=channel+aec->cfg->spchannel;
    float fv;
    float *fp[512];
    wtk_strbuf_t **input=aec->input;

    for(i=0; i<channel; ++i)
    {
        wtk_strbuf_reset(input[i]);
        for(j=0;j<len;++j)
        {
            fv=data[i][j];
            wtk_strbuf_push(input[i],(char *)(&fv),sizeof(float));
        }
        fp[i]=(float *)(input[i]->data);
        
        wtk_preemph_dc(fp[i], aec->notch_mem[i], len);
        aec->memD[i]=wtk_preemph_asis(fp[i], len, aec->memD[i]);
    }
    for(i=channel; i<channel2; ++i)
    {
        wtk_strbuf_reset(input[i]);
        for(j=0;j<len;++j)
        {
            fv=data[i][j];
            wtk_strbuf_push(input[i],(char *)(&fv),sizeof(float));
        }
        fp[i]=(float *)(input[i]->data);
        
        wtk_preemph_dc(fp[i], aec->notch_mem[i], len);
        aec->memD[i]=wtk_preemph_asis(fp[i], len, aec->memD[i]);
    }

    wtk_stft2_feed_float(aec->stft,fp,len,is_end);
}

void wtk_aec_feed(wtk_aec_t *aec,short **mic,int len,int is_end)
{
#ifdef DEBUG_WAV
	static wtk_wavfile_t *mic_log=NULL;

	if(!mic_log)
	{
		mic_log=wtk_wavfile_new(16000);
		wtk_wavfile_set_channel(mic_log,aec->channel+aec->cfg->spchannel);
		wtk_wavfile_open2(mic_log,"aec");
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
    if(aec->cfg->use_preemph)
    {
        wtk_aec_feed_dc(aec,mic,len,is_end);
    }else
    {
        wtk_stft2_feed2(aec->stft, mic, len, is_end);
    }
}


void wtk_aec_notify_data(wtk_aec_t *aec,float **data,int len,int is_end)
{
    short *pv[512],*pv1;
    int i,j;
    int channel=aec->channel;
    float *data1;

    for(i=0;i<channel;++i)
    {
        data1=data[i];
        pv1=(short *)data1;
        pv[i]=pv1;

        if(aec->cfg->use_preemph)
        {
            aec->memX[i]=wtk_preemph_asis2(data1,len,aec->memX[i]);
        }

        for(j=0; j<len; ++j)
        {
            if(fabs(data1[j])<32767.0)
            {
                pv1[j]=data1[j];
            }else
            {
                if(data1[j]>0)
                {
                    pv1[j]=32767;
                }else
                {
                    pv1[j]=-32767;
                }
            }
        }
    }
    aec->notify(aec->ths,pv,len,is_end);
}

void wtk_aec_on_stft(wtk_aec_t *aec,wtk_stft2_msg_t *msg,int pos,int is_end)
{
    int channel=aec->channel;
    int spchannel=aec->cfg->spchannel;
    int nbin=aec->nbin;
    int i,j,len=0;
    wtk_complex_t **fft;
    wtk_nlms_t *nlms=aec->nlms, *nlmstmp;
    wtk_rls_t *rls=aec->rls, *rlstmp;
    wtk_complex_t **out=aec->out;
    float **Yf=aec->Yf;

    wtk_complex_t *E,*Y;
    float **Se=aec->Se, *se;
    float **Sd=aec->Sd, *sd;
    wtk_complex_t **Sed=aec->Sed, *sed;
    float yf,ef;
    float leak, leak_scale=aec->cfg->leak_scale;
    float coh_alpha=aec->cfg->coh_alpha;
    float spenr;
    float spenr_thresh=aec->cfg->spenr_thresh;
    int spenr_cnt=aec->cfg->spenr_cnt;

    if(!msg)
    {
        if(is_end && aec->notify)
        {
            aec->notify(aec->ths, NULL, 0, 1);
        }
        return;
    }
    if(is_end)
    {
        aec->end_pos=pos;
    }

    ++aec->nframe;

    fft=msg->fft;
    spenr=0;
    for(i=0; i<nbin; ++i)
    {
        for(j=channel;j<channel+spchannel;++j)
        {
            spenr+=fft[i][j].a*fft[i][j].a+fft[i][j].b*fft[i][j].b;
        }
    }
    spenr/=spchannel;  
    // printf("%.0f %f\n",aec->nframe,spenr);
    if(spenr>spenr_thresh)
    {
        aec->sp_sil=0;
        // printf("%.0f %f\n",aec->nframe,spenr);
        aec->sp_silcnt=spenr_cnt;
    }else if(aec->sp_sil==0)
    {
        aec->sp_silcnt-=1;
        if(aec->sp_silcnt==0)
        {
            // printf("===%.0f %f\n",aec->nframe,spenr);
            aec->sp_sil=1;
            if(aec->nlms)
            {
                nlmstmp=aec->nlms;
                for(i=0; i<nbin; ++i, ++nlmstmp)
                {
                    wtk_nlms_reset2(nlmstmp);
                }
            }else
            {
                rlstmp=aec->rls;
                for(i=0; i<nbin; ++i, ++rlstmp)
                {
                    wtk_rls_reset2(rlstmp);
                }
            }
        }
    }

    if(aec->sp_sil==0)
    {
        if(aec->nlms)
        {
            for(i=0; i<nbin; ++i, ++nlms)
            {
                wtk_nlms_feed(nlms, fft[i], fft[i]+channel);
                if(aec->qmmse)
                {
                    for(j=0; j<channel; ++j)
                    {
                        out[j][i]=nlms->out[j];
                        Yf[j][i]=nlms->lsty_power[j];
                    }
                }else
                {
                    for(j=0; j<channel; ++j)
                    {
                        out[j][i]=nlms->out[j];
                    }
                }
            }
        }else if(aec->rls)
        {
            for(i=0; i<nbin; ++i, ++rls)
            {
                wtk_rls_feed(rls, fft[i], fft[i]+channel);
                if(aec->qmmse)
                {
                    se=Se[i];
                    sd=Sd[i];
                    sed=Sed[i];
                    for(j=0; j<channel; ++j)
                    {
                        out[j][i]=rls->out[j];
                        E=rls->out+j;
                        Y=rls->lsty+j;
                        if(aec->nframe<=1)
                        {
                            se[j]+=E->a*E->a+E->b*E->b;
                            yf=Y->a*Y->a+Y->b*Y->b;
                            sd[j]+=yf;
                            sed[j].a+=Y->a*E->a+Y->b*E->b;
                            sed[j].b+=-Y->a*E->b+Y->b*E->a;

                            leak=(sed[j].a*sed[j].a+sed[j].b*sed[j].b)/(max(se[j],sd[j])*sd[j]+1e-9);
                            Yf[j][i]=leak*yf*leak_scale;
                        }else
                        {
                            ef=E->a*E->a+E->b*E->b;
                            yf=Y->a*Y->a+Y->b*Y->b;

                            se[j]=(1-coh_alpha)*se[j]+coh_alpha*ef;
                            sd[j]=(1-coh_alpha)*sd[j]+coh_alpha*yf;
                            sed[j].a=(1-coh_alpha)*sed[j].a+coh_alpha*(Y->a*E->a+Y->b*E->b);
                            sed[j].b=(1-coh_alpha)*sed[j].b+coh_alpha*(-Y->a*E->b+Y->b*E->a);

                            leak=(sed[j].a*sed[j].a+sed[j].b*sed[j].b)/(max(se[j],sd[j])*sd[j]+1e-9);
                            Yf[j][i]=leak*yf*leak_scale;
                        }
                    }
                }else
                {
                    for(j=0; j<channel; ++j)
                    {
                        out[j][i]=rls->out[j];
                    }
                }
            }
        }
        
        if(aec->qmmse && aec->cfg->use_aec_post)
        {
            for(i=0; i<channel; ++i)
            {
                wtk_qmmse_feed_echo_denoise(aec->qmmse[i], out[i], Yf[i]);
                len=wtk_stft2_output_ifft(aec->stft, out[i], aec->output[i], aec->pad[i], aec->end_pos, is_end);
            }
            if(aec->notify)
            {
                wtk_aec_notify_data(aec, aec->output, len, is_end);
            }
        }else
        {
            for(i=0; i<channel; ++i)
            {
                len=wtk_stft2_output_ifft(aec->stft, out[i], aec->output[i], aec->pad[i], aec->end_pos, is_end);
            }
            if(aec->notify)
            {
                wtk_aec_notify_data(aec, aec->output, len, is_end);
            }
        }
    }else
    {
        for(i=0; i<nbin; ++i)
        {
            for(j=0; j<channel; ++j)
            {
                out[j][i]=fft[i][j];
            }
        }
        for(i=0; i<channel; ++i)
        {
            // memset(Yf[i], 0 ,sizeof(float)*nbin);
            if(aec->qmmse && aec->cfg->use_post){
                wtk_qmmse_denoise(aec->qmmse[i], out[i]);
            }
            len=wtk_stft2_output_ifft(aec->stft, out[i], aec->output[i], aec->pad[i], aec->end_pos, is_end);
        }
        if(aec->notify)
        {
            wtk_aec_notify_data(aec, aec->output, len, is_end);
        }
    }

    wtk_stft2_push_msg(aec->stft, msg);
}






















wtk_aec_t* wtk_aec_new2(wtk_aec_cfg_t *cfg, wtk_stft2_t *stft)
{
    wtk_aec_t *aec;
    int i;

    aec=(wtk_aec_t *)wtk_malloc(sizeof(wtk_aec_t));
    aec->ths=NULL;
    aec->notify=NULL;
    aec->notify2=NULL;

    aec->cfg=cfg;
    aec->stft=stft;

    aec->nbin=aec->stft->nbin;
    aec->channel=aec->stft->cfg->channel;

    aec->pad=NULL;
    aec->output=NULL;
    aec->input=NULL;

    aec->nlms=NULL;
    aec->rls=NULL;
    if(cfg->use_nlms)
    {
        aec->nlms=(wtk_nlms_t *)wtk_malloc(sizeof(wtk_nlms_t)*aec->nbin);
        for(i=0;i<aec->nbin;++i)
        {
            wtk_nlms_init(aec->nlms+i, &(aec->cfg->nlms));
        }
    }else
    {
        aec->rls=(wtk_rls_t *)wtk_malloc(sizeof(wtk_rls_t)*aec->nbin);
        for(i=0;i<aec->nbin;++i)
        {
            wtk_rls_init(aec->rls+i, &(aec->cfg->rls));
        }
    }
    
    aec->Yf=wtk_float_new_p2(aec->channel, aec->nbin);
    aec->out=wtk_complex_new_p2(aec->channel, aec->nbin);

    aec->qmmse=NULL;
    aec->Se=NULL;
    aec->Sd=NULL;
    aec->Sed=NULL;
    if(cfg->use_post || cfg->use_aec_post)
    {
        if(cfg->use_rls)
        {
            aec->Se=wtk_float_new_p2(aec->nbin, aec->channel);
            aec->Sd=wtk_float_new_p2(aec->nbin, aec->channel);
            aec->Sed=wtk_complex_new_p2(aec->nbin, aec->channel);
        }

        aec->qmmse=(wtk_qmmse_t **)wtk_malloc(sizeof(wtk_qmmse_t *)*aec->channel);
        for(i=0; i<aec->channel; ++i)
        {
            aec->qmmse[i]=wtk_qmmse_new(&(cfg->qmmse));
        }
    }

    wtk_aec_reset2(aec);

    return aec;
}

void wtk_aec_reset2(wtk_aec_t *aec)
{
	int i;
    int channel=aec->channel;
    int nbin=aec->nbin;

    aec->nframe=0;
    aec->end_pos=0;
    aec->sp_sil=1;
    aec->sp_silcnt=0;

    for(i=0;i<nbin;++i)
    {
        if(aec->nlms)
        {
            wtk_nlms_reset(aec->nlms+i);
        }else if(aec->rls)
        {
            wtk_rls_reset(aec->rls+i);
        }
        if(aec->qmmse && aec->rls)
        {
            memset(aec->Se[i], 0, sizeof(float)*aec->channel);
            memset(aec->Sd[i], 0, sizeof(float)*aec->channel);
            memset(aec->Sed[i], 0, sizeof(wtk_complex_t)*aec->channel);
        }
    }

    if(aec->qmmse)
    {
        for(i=0; i<channel; ++i)
        {
            wtk_qmmse_reset(aec->qmmse[i]);
        }
    }
}

void wtk_aec_delete2(wtk_aec_t *aec)
{
    int i;

    if(aec->qmmse)
    {
        for(i=0; i<aec->channel; ++i)
        {
            wtk_qmmse_delete(aec->qmmse[i]);
        }
        wtk_free(aec->qmmse);
        if(aec->rls)
        {
            wtk_float_delete_p2(aec->Se, aec->nbin);
            wtk_float_delete_p2(aec->Sd, aec->nbin);
            wtk_complex_delete_p2(aec->Sed, aec->nbin);
        }
    }

    if(aec->nlms)
    {
        for(i=0;i<aec->nbin;++i)
        {
            wtk_nlms_clean(aec->nlms+i);
        }
        wtk_free(aec->nlms);
    }else if(aec->rls)
    {
        for(i=0;i<aec->nbin;++i)
        {
            wtk_rls_clean(aec->rls+i);
        }
        wtk_free(aec->rls);
    }

    wtk_complex_delete_p2(aec->out, aec->channel);
    wtk_float_delete_p2(aec->Yf, aec->channel);

    wtk_free(aec);
}

void wtk_aec_set_notify2(wtk_aec_t *aec,void *ths,wtk_aec_notify_f2 notify)
{
    aec->notify2=notify;
    aec->ths=ths;
}



void wtk_aec_feed_stftmsg(wtk_aec_t *aec,wtk_stft2_msg_t *msg,wtk_stft2_msg_t *sp_msg,int pos,int is_end)
{
    int channel=aec->channel;
    int spchannel=aec->cfg->spchannel;
    int nbin=aec->nbin;
    int i,j;
    wtk_complex_t **fft, **spfft;
    wtk_nlms_t *nlms=aec->nlms;
    wtk_rls_t *rls=aec->rls;
    wtk_complex_t **out=aec->out;
    float **Yf=aec->Yf;

    wtk_complex_t *E,*Y;
    float **Se=aec->Se, *se;
    float **Sd=aec->Sd, *sd;
    wtk_complex_t **Sed=aec->Sed, *sed;
    float yf,ef;
    float leak, leak_scale=aec->cfg->leak_scale;
    float coh_alpha=aec->cfg->coh_alpha;

    float spenr;
    float spenr_thresh=aec->cfg->spenr_thresh;
    int spenr_cnt=aec->cfg->spenr_cnt;
    if(!msg)
    {
        if(is_end && aec->notify)
        {
            aec->notify2(aec->ths, NULL, 0, 1);
        }
        return;
    }
    if(is_end)
    {
        aec->end_pos=pos;
    }

    ++aec->nframe;

    fft=msg->fft;
    spfft=sp_msg->fft;

    spenr=0;
    for(i=0; i<nbin; ++i)
    {
        for(j=0;j<spchannel;++j)
        {
            spenr+= spfft[i][j].a*spfft[i][j].a + spfft[i][j].b*spfft[i][j].b;
        }
    }
    spenr/=spchannel;  
    if(spenr>spenr_thresh)
    {
        // if(aec->sp_sil==1)
        // {
        //     printf("start %.0f %f\n",aec->nframe*512/16.000,spenr);
        // }
        aec->sp_sil=0;
        aec->sp_silcnt=spenr_cnt;
    }else if(aec->sp_sil==0)
    {
        aec->sp_silcnt-=1;
        if(aec->sp_silcnt==0)
        {
            // printf("end %.0f %f\n",aec->nframe*512/16.000,spenr);
            aec->sp_sil=1;
            if(aec->nlms)
            {
                nlms=aec->nlms;
                for(i=0; i<nbin; ++i, ++nlms)
                {
                    wtk_nlms_reset2(nlms);
                }
            }else
            {
                rls=aec->rls;
                for(i=0; i<nbin; ++i, ++rls)
                {
                    wtk_rls_reset2(rls);
                }
            }
        }
    }
    
    if(aec->sp_sil==0)
    {
        if(aec->nlms)
        {
            for(i=0; i<nbin; ++i, ++nlms)
            {
                wtk_nlms_feed(nlms, fft[i], spfft[i]);
                for(j=0; j<channel; ++j)
                {
                    // out[j][i].a=nlms->out[j].a*6;
                    // out[j][i].b=nlms->out[j].b*6;
                    out[j][i]=nlms->out[j];
                    Yf[j][i]=nlms->lsty_power[j];
                }
            }
        }else if(aec->rls)
        {
            for(i=0; i<nbin; ++i, ++rls)
            {
                wtk_rls_feed(rls, fft[i], spfft[i]);
                if(aec->qmmse)
                {
                    se=Se[i];
                    sd=Sd[i];
                    sed=Sed[i];
                    for(j=0; j<channel; ++j)
                    {
                        out[j][i]=rls->out[j];
                        E=rls->out+j;
                        Y=rls->lsty+j;
                        if(aec->nframe<=1)
                        {
                            se[j]+=E->a*E->a+E->b*E->b;
                            yf=Y->a*Y->a+Y->b*Y->b;
                            sd[j]+=yf;
                            sed[j].a+=Y->a*E->a+Y->b*E->b;
                            sed[j].b+=-Y->a*E->b+Y->b*E->a;

                            leak=(sed[j].a*sed[j].a+sed[j].b*sed[j].b)/(max(se[j],sd[j])*sd[j]+1e-9)*leak_scale;
                            Yf[j][i]=leak*yf;
                        }else
                        {
                            ef=E->a*E->a+E->b*E->b;
                            yf=Y->a*Y->a+Y->b*Y->b;

                            se[j]=(1-coh_alpha)*se[j]+coh_alpha*ef;
                            sd[j]=(1-coh_alpha)*sd[j]+coh_alpha*yf;
                            sed[j].a=(1-coh_alpha)*sed[j].a+coh_alpha*(Y->a*E->a+Y->b*E->b);
                            sed[j].b=(1-coh_alpha)*sed[j].b+coh_alpha*(-Y->a*E->b+Y->b*E->a);

                            leak=(sed[j].a*sed[j].a+sed[j].b*sed[j].b)/(max(se[j],sd[j])*sd[j]+1e-9)*leak_scale;
                            Yf[j][i]=leak*yf;
                        }
                    }
                }else
                {
                    for(j=0; j<channel; ++j)
                    {
                        out[j][i]=rls->out[j];
                    }
                }
            }
        }
        if(aec->qmmse && aec->cfg->use_aec_post)
        {
            for(i=0; i<channel; ++i)
            {
                wtk_qmmse_feed_echo_denoise(aec->qmmse[i], out[i], Yf[i]);
                wtk_qmmse_denoise(aec->qmmse[i], out[i]);
            }
        }
    }else
    {
        for(i=0; i<nbin; ++i)
        {
            for(j=0; j<channel; ++j)
            {
                out[j][i]=fft[i][j];
                Yf[j][i]=0;
            }
        }
        if (aec->qmmse && aec->cfg->use_post) {
            for (i = 0; i < channel; ++i) {
                // wtk_qmmse_feed_echo_denoise(aec->qmmse[i], out[i], Yf[i]);
                wtk_qmmse_denoise(aec->qmmse[i], out[i]);
            }
        }
    }
    if(aec->notify2)
    {        
        for(i=0; i<nbin; ++i)
        {
            for(j=0; j<channel; ++j)
            {
                fft[i][j]=out[j][i];
            }
        }
        aec->notify2(aec->ths, msg, pos ,is_end);
    }
}


