#include "wtk_admm2.h" 

void wtk_admm2_on_stft(wtk_admm2_t *admm2,wtk_stft2_msg_t *msg,int pos,int is_end);

wtk_admm2_t* wtk_admm2_new(wtk_admm2_cfg_t *cfg)
{
    wtk_admm2_t *admm2;
    int i;

    admm2=(wtk_admm2_t *)wtk_malloc(sizeof(wtk_admm2_t));
    admm2->ths=NULL;
    admm2->notify=NULL;
    admm2->notify2=NULL;

    admm2->cfg=cfg;
    admm2->stft=wtk_stft2_new(&(cfg->stft));
    wtk_stft2_set_notify(admm2->stft, admm2, (wtk_stft2_notify_f)wtk_admm2_on_stft);

    admm2->nbin=admm2->stft->nbin;
    admm2->channel=cfg->stft.channel;

	admm2->norig=wtk_complex_new_p3(admm2->nbin, admm2->channel, cfg->D+1);

    admm2->pad=wtk_float_new_p2(admm2->channel, admm2->stft->cfg->win);
    admm2->output=wtk_float_new_p2(admm2->channel, admm2->stft->cfg->win);

	admm2->rls=(wtk_rls_t *)wtk_malloc(sizeof(wtk_rls_t)*admm2->nbin*admm2->channel);
	for(i=0;i<admm2->nbin*admm2->channel;++i)
	{
		wtk_rls_init(admm2->rls+i, &(admm2->cfg->rls));
	}
    
    admm2->out=wtk_complex_new_p2(admm2->channel, admm2->nbin);

    wtk_admm2_reset(admm2);

    return admm2;
}

void wtk_admm2_reset(wtk_admm2_t *admm2)
{
	int i;
    int nbin=admm2->nbin;
    int channel=admm2->channel;

    admm2->nframe=0;
    admm2->end_pos=0;

    wtk_stft2_reset(admm2->stft);
    wtk_float_zero_p2(admm2->output, channel, admm2->stft->cfg->win);
    wtk_float_zero_p2(admm2->pad, channel, admm2->stft->cfg->win);

	wtk_complex_zero_p3(admm2->norig, admm2->nbin, channel,  admm2->cfg->D+1);

    for(i=0;i<nbin*channel;++i)
    {
		wtk_rls_reset(admm2->rls+i);
    }
}

void wtk_admm2_delete(wtk_admm2_t *admm2)
{
    int i;

	wtk_complex_delete_p3(admm2->norig, admm2->nbin, admm2->channel);

    wtk_float_delete_p2(admm2->output, admm2->channel);
    wtk_float_delete_p2(admm2->pad, admm2->channel);
    wtk_stft2_delete(admm2->stft);

	for(i=0;i<admm2->nbin*admm2->channel;++i)
	{
		wtk_rls_clean(admm2->rls+i);
	}
	wtk_free(admm2->rls);

    wtk_complex_delete_p2(admm2->out, admm2->channel);

    wtk_free(admm2);
}

void wtk_admm2_set_notify(wtk_admm2_t *admm2,void *ths,wtk_admm2_notify_f notify)
{
    admm2->notify=notify;
    admm2->ths=ths;
}


void wtk_admm2_feed(wtk_admm2_t *admm2,short **mic,int len,int is_end)
{
#ifdef DEBUG_WAV
	static wtk_wavfile_t *mic_log=NULL;

	if(!mic_log)
	{
		mic_log=wtk_wavfile_new(16000);
		wtk_wavfile_set_channel(mic_log,admm2->channel+1);
		wtk_wavfile_open2(mic_log,"admm2");
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
	wtk_stft2_feed(admm2->stft, mic, len, is_end);
}


void wtk_admm2_notify_data(wtk_admm2_t *admm2,float **data,int len,int is_end)
{
    short *pv[512],*pv1;
    int i,j;
    int channel=admm2->channel;
    float *data1;

    for(i=0;i<channel;++i)
    {
        data1=data[i];
        pv1=(short *)data1;
        pv[i]=pv1;

        for(j=0; j<len; ++j)
        {
            if(fabs(data1[j])<1.0)
            {
                pv1[j]=data1[j]*32767;
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
    admm2->notify(admm2->ths,pv,len,is_end);
}


void wtk_admm2_on_stft(wtk_admm2_t *admm2,wtk_stft2_msg_t *msg,int pos,int is_end)
{
    int channel=admm2->channel;
    int nbin=admm2->nbin;
    int i, j, k, len = -1;
    wtk_complex_t **fft, *fft1;
    wtk_rls_t *rls=admm2->rls;
	wtk_complex_t ***norig=admm2->norig, **norigtmp2, *norigtmp;
    wtk_complex_t **out=admm2->out;
    int D=admm2->cfg->D;

    if(!msg)
    {
        if(is_end && admm2->notify)
        {
            admm2->notify(admm2->ths, NULL, 0, 1);
        }
        return;
    }
    if(is_end)
    {
        admm2->end_pos=pos;
    }

    ++admm2->nframe;

    fft=msg->fft;
    for(k=0; k<nbin; ++k)
    {
        fft1=fft[k];
        norigtmp2=norig[k];

        for(j=0; j<channel; ++j)
        {
            norigtmp=norigtmp2[j];
            for(i=0; i<D; ++i)
            {
                norigtmp[i].a=norigtmp[i+1].a;
                norigtmp[i].b=norigtmp[i+1].b;
            }
            if(D>0)
            {
                norigtmp[D]=fft1[j];
            }
        }
    }

	for(i=0; i<nbin; ++i)
	{
		for(j=0; j<channel; ++j, ++rls)
        {
		    wtk_rls_feed(rls, fft[i]+j, norig[i][j]);
			out[j][i]=rls->out[0];
		}
	}
    
    for(i=0; i<channel; ++i)
    {
        len=wtk_stft2_output_ifft(admm2->stft, out[i], admm2->output[i], admm2->pad[i], admm2->end_pos, is_end);
    }
    if(admm2->notify)
    {
        wtk_admm2_notify_data(admm2, admm2->output, len, is_end);
    }

    wtk_stft2_push_msg(admm2->stft, msg);
}











wtk_admm2_t* wtk_admm2_new2(wtk_admm2_cfg_t *cfg, wtk_stft2_t *stft)
{
    wtk_admm2_t *admm2;
    int i;

    admm2=(wtk_admm2_t *)wtk_malloc(sizeof(wtk_admm2_t));
    admm2->ths=NULL;
    admm2->notify=NULL;
    admm2->notify2=NULL;

    admm2->cfg=cfg;
    admm2->stft=stft;

    admm2->nbin=admm2->stft->nbin;
    admm2->channel=admm2->stft->cfg->channel;

    admm2->pad=NULL;
    admm2->output=NULL;

	admm2->norig=wtk_complex_new_p3(admm2->nbin, admm2->channel, cfg->D+1);

	admm2->rls=(wtk_rls_t *)wtk_malloc(sizeof(wtk_rls_t)*admm2->nbin*admm2->channel);
	for(i=0;i<admm2->nbin*admm2->channel;++i)
	{
		wtk_rls_init(admm2->rls+i, &(admm2->cfg->rls));
	}

    admm2->out=wtk_complex_new_p2(admm2->channel, admm2->nbin);

    wtk_admm2_reset2(admm2);

    return admm2;
}

void wtk_admm2_reset2(wtk_admm2_t *admm2)
{
	int i;
    int nbin=admm2->nbin;
    int channel=admm2->channel;

    admm2->nframe=0;
    admm2->end_pos=0;

    wtk_complex_zero_p3(admm2->norig, admm2->nbin, channel,  admm2->cfg->D+1);

    for(i=0;i<nbin*channel;++i)
    {
		wtk_rls_reset(admm2->rls+i);
    }
}

void wtk_admm2_delete2(wtk_admm2_t *admm2)
{
    int i;

	wtk_complex_delete_p3(admm2->norig, admm2->nbin, admm2->channel);

	for(i=0;i<admm2->nbin*admm2->channel;++i)
	{
		wtk_rls_clean(admm2->rls+i);
	}

	wtk_free(admm2->rls);

    wtk_complex_delete_p2(admm2->out, admm2->channel);

    wtk_free(admm2);
}

void wtk_admm2_set_notify2(wtk_admm2_t *admm2,void *ths,wtk_admm2_notify_f2 notify)
{
    admm2->notify2=notify;
    admm2->ths=ths;
}



void wtk_admm2_feed_stftmsg(wtk_admm2_t *admm2,wtk_stft2_msg_t *msg,int pos,int is_end)
{
    int channel=admm2->channel;
    int nbin=admm2->nbin;
    int i,j,k;
    wtk_complex_t **fft, *fft1;
    wtk_rls_t *rls=admm2->rls;
	wtk_complex_t ***norig=admm2->norig, **norigtmp2, *norigtmp;
    // wtk_complex_t **out=admm2->out;
    // int L=admm2->cfg->L;
    int D=admm2->cfg->D;
	wtk_stft2_msg_t *err_msg;
	wtk_stft2_msg_t *lsty_msg;

    if(!msg)
    {
        if(is_end && admm2->notify)
        {
            admm2->notify2(admm2->ths, NULL, NULL, NULL, 0, 1);
        }
        return;
    }
    if(is_end)
    {
        admm2->end_pos=pos;
    }

    ++admm2->nframe;

    fft=msg->fft;
    for(k=0; k<nbin; ++k)
    {
        fft1=fft[k];
        norigtmp2=norig[k];

        for(j=0; j<channel; ++j)
        {
            norigtmp=norigtmp2[j];
            for(i=0; i<D; ++i)
            {
                norigtmp[i].a=norigtmp[i+1].a;
                norigtmp[i].b=norigtmp[i+1].b;
            }
            if(D>0)
            {
                norigtmp[D]=fft1[j];
            }
        }
    }

    if(admm2->notify2)
    {    
		err_msg=wtk_stft2_pop_msg(admm2->stft);    
		lsty_msg=wtk_stft2_pop_msg(admm2->stft);    
		for(i=0; i<nbin; ++i)
		{
			for(j=0; j<channel; ++j,++rls)
			{
                wtk_rls_feed(rls, fft[i]+j, norig[i][j]);
				err_msg->fft[i][j]=rls->out[0];
				lsty_msg->fft[i][j]=rls->lsty[0];
			}
		}

        admm2->notify2(admm2->ths, msg, err_msg, lsty_msg, pos ,is_end);
    }
}
