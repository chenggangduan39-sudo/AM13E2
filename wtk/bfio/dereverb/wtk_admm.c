#include "wtk_admm.h" 

void wtk_admm_on_stft(wtk_admm_t *admm,wtk_stft2_msg_t *msg,int pos,int is_end);

wtk_admm_t* wtk_admm_new(wtk_admm_cfg_t *cfg)
{
    wtk_admm_t *admm;
    int i;

    admm=(wtk_admm_t *)wtk_malloc(sizeof(wtk_admm_t));
    admm->ths=NULL;
    admm->notify=NULL;
    admm->notify2=NULL;

    admm->cfg=cfg;
    admm->stft=wtk_stft2_new(&(cfg->stft));
    wtk_stft2_set_notify(admm->stft, admm, (wtk_stft2_notify_f)wtk_admm_on_stft);

    admm->nbin=admm->stft->nbin;
    admm->channel=cfg->stft.channel;

	admm->norig=wtk_complex_new_p2(admm->nbin, (cfg->D+1)*admm->channel);

    admm->pad=wtk_float_new_p2(admm->channel, admm->stft->cfg->win);
    admm->output=wtk_float_new_p2(admm->channel, admm->stft->cfg->win);

	admm->rls=(wtk_rls_t *)wtk_malloc(sizeof(wtk_rls_t)*admm->nbin);
	for(i=0;i<admm->nbin;++i)
	{
		wtk_rls_init(admm->rls+i, &(admm->cfg->rls));
	}
    
    admm->out=wtk_complex_new_p2(admm->channel, admm->nbin);

    wtk_admm_reset(admm);

    return admm;
}

void wtk_admm_reset(wtk_admm_t *admm)
{
	int i;
    int nbin=admm->nbin;

    admm->nframe=0;
    admm->end_pos=0;

    wtk_stft2_reset(admm->stft);
    wtk_float_zero_p2(admm->output, admm->channel, admm->stft->cfg->win);
    wtk_float_zero_p2(admm->pad, admm->channel, admm->stft->cfg->win);

	wtk_complex_zero_p2(admm->norig, admm->nbin, (admm->cfg->D+1)*admm->channel);

    for(i=0;i<nbin;++i)
    {
		wtk_rls_reset(admm->rls+i);
    }
}

void wtk_admm_delete(wtk_admm_t *admm)
{
    int i;

	wtk_complex_delete_p2(admm->norig, admm->nbin);

    wtk_float_delete_p2(admm->output, admm->channel);
    wtk_float_delete_p2(admm->pad, admm->channel);
    wtk_stft2_delete(admm->stft);

	for(i=0;i<admm->nbin;++i)
	{
		wtk_rls_clean(admm->rls+i);
	}
	wtk_free(admm->rls);

    wtk_complex_delete_p2(admm->out, admm->channel);

    wtk_free(admm);
}

void wtk_admm_set_notify(wtk_admm_t *admm,void *ths,wtk_admm_notify_f notify)
{
    admm->notify=notify;
    admm->ths=ths;
}


void wtk_admm_feed(wtk_admm_t *admm,short **mic,int len,int is_end)
{
#ifdef DEBUG_WAV
	static wtk_wavfile_t *mic_log=NULL;

	if(!mic_log)
	{
		mic_log=wtk_wavfile_new(16000);
		wtk_wavfile_set_channel(mic_log,admm->channel+1);
		wtk_wavfile_open2(mic_log,"admm");
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
	wtk_stft2_feed(admm->stft, mic, len, is_end);
}


void wtk_admm_notify_data(wtk_admm_t *admm,float **data,int len,int is_end)
{
    short *pv[512],*pv1;
    int i,j;
    int channel=admm->channel;
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
    admm->notify(admm->ths,pv,len,is_end);
}


void wtk_admm_on_stft(wtk_admm_t *admm,wtk_stft2_msg_t *msg,int pos,int is_end)
{
    int channel=admm->channel;
    int nbin=admm->nbin;
    int i,j,k,n1,n2,len=0;
    wtk_complex_t **fft, *fft1;
    wtk_rls_t *rls=admm->rls;
	wtk_complex_t **norig=admm->norig, *norigtmp;
    wtk_complex_t **out=admm->out;
    int D=admm->cfg->D;

    if(!msg)
    {
        if(is_end && admm->notify)
        {
            admm->notify(admm->ths, NULL, 0, 1);
        }
        return;
    }
    if(is_end)
    {
        admm->end_pos=pos;
    }

    ++admm->nframe;

    fft=msg->fft;
    for(k=0; k<nbin; ++k)
    {
        fft1=fft[k];
        norigtmp=norig[k];

        for(i=0; i<D; ++i)
        {
            n1=i*channel;
            n2=(i+1)*channel;
            for(j=0; j<channel; ++j, ++n1,++n2)
            {
                norigtmp[n1].a=norigtmp[n2].a;
                norigtmp[n1].b=norigtmp[n2].b;
            }
        }
        if(D>0)
        {
            n1=D*channel;
            for(j=0; j<channel; ++j, ++n1)
            {
                norigtmp[n1].a=fft1[j].a;
                norigtmp[n1].b=fft1[j].b;
            }
        }
    }

	for(i=0; i<nbin; ++i, ++rls)
	{
		wtk_rls_feed(rls, fft[i], norig[i]);
		// if(i==1)
		// {
		// 	wtk_complex_print(rls->xld, 16);
		// }
		for(j=0; j<channel; ++j)
		{
			out[j][i]=rls->out[j];
		}
	}
    
    for(i=0; i<channel; ++i)
    {
        len=wtk_stft2_output_ifft(admm->stft, out[i], admm->output[i], admm->pad[i], admm->end_pos, is_end);
    }
    if(admm->notify)
    {
        wtk_admm_notify_data(admm, admm->output, len, is_end);
    }

    wtk_stft2_push_msg(admm->stft, msg);
}











wtk_admm_t* wtk_admm_new2(wtk_admm_cfg_t *cfg, wtk_stft2_t *stft)
{
    wtk_admm_t *admm;
    int i;

    admm=(wtk_admm_t *)wtk_malloc(sizeof(wtk_admm_t));
    admm->ths=NULL;
    admm->notify=NULL;
    admm->notify2=NULL;

    admm->cfg=cfg;
    admm->stft=stft;

    admm->nbin=admm->stft->nbin;
    admm->channel=admm->stft->cfg->channel;

    admm->pad=NULL;
    admm->output=NULL;

	admm->rls=(wtk_rls_t *)wtk_malloc(sizeof(wtk_rls_t)*admm->nbin);
	for(i=0;i<admm->nbin;++i)
	{
		wtk_rls_init(admm->rls+i, &(admm->cfg->rls));
	}

	admm->norig=wtk_complex_new_p2(admm->nbin, (cfg->D+1)*admm->channel);

    admm->out=wtk_complex_new_p2(admm->channel, admm->nbin);

    wtk_admm_reset2(admm);

    return admm;
}

void wtk_admm_reset2(wtk_admm_t *admm)
{
	int i;
    int nbin=admm->nbin;

    admm->nframe=0;
    admm->end_pos=0;

    for(i=0;i<nbin;++i)
    {
		wtk_rls_reset(admm->rls+i);
    }
	wtk_complex_zero_p2(admm->norig, admm->nbin, (admm->cfg->D+1)*admm->channel);
}

void wtk_admm_delete2(wtk_admm_t *admm)
{
    int i;

	wtk_complex_delete_p2(admm->norig, admm->nbin);

	for(i=0;i<admm->nbin;++i)
	{
		wtk_rls_clean(admm->rls+i);
	}
	wtk_free(admm->rls);

    wtk_complex_delete_p2(admm->out, admm->channel);

    wtk_free(admm);
}

void wtk_admm_set_notify2(wtk_admm_t *admm,void *ths,wtk_admm_notify_f2 notify)
{
    admm->notify2=notify;
    admm->ths=ths;
}



void wtk_admm_feed_stftmsg(wtk_admm_t *admm,wtk_stft2_msg_t *msg,int pos,int is_end)
{
    int channel=admm->channel;
    int nbin=admm->nbin;
    int i,j,k,n1,n2;
    wtk_complex_t **fft, *fft1;
    wtk_rls_t *rls=admm->rls;
	wtk_complex_t **norig=admm->norig, *norigtmp;
    // wtk_complex_t **out=admm->out;
    // int L=admm->cfg->L;
    int D=admm->cfg->D;
	wtk_stft2_msg_t *err_msg;
	wtk_stft2_msg_t *lsty_msg;

    if(!msg)
    {
        if(is_end && admm->notify)
        {
            admm->notify2(admm->ths, NULL, NULL, NULL, 0, 1);
        }
        return;
    }
    if(is_end)
    {
        admm->end_pos=pos;
    }

    ++admm->nframe;

    fft=msg->fft;
    for(k=0; k<nbin; ++k)
    {
        fft1=fft[k];
        norigtmp=norig[k];

        for(i=0; i<D; ++i)
        {
            n1=i*channel;
            n2=(i+1)*channel;
            for(j=0; j<channel; ++j, ++n1,++n2)
            {
                norigtmp[n1].a=norigtmp[n2].a;
                norigtmp[n1].b=norigtmp[n2].b;
            }
        }
        if(D>0)
        {
            n1=D*channel;
            for(j=0; j<channel; ++j, ++n1)
            {
                norigtmp[n1].a=fft1[j].a;
                norigtmp[n1].b=fft1[j].b;
            }
        }
    }


    if(admm->notify2)
    {    
		err_msg=wtk_stft2_pop_msg(admm->stft);    
		lsty_msg=wtk_stft2_pop_msg(admm->stft);    
		for(i=0; i<nbin; ++i, ++rls)
		{
			wtk_rls_feed(rls, fft[i], norig[i]);
			for(j=0; j<channel; ++j)
			{
				err_msg->fft[i][j]=rls->out[j];
				lsty_msg->fft[i][j]=rls->lsty[j];
			}
		}

        admm->notify2(admm->ths, msg, err_msg, lsty_msg, pos ,is_end);
    }
}
