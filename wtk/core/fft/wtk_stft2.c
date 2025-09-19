#include "wtk_stft2.h" 

wtk_stft2_msg_t* wtk_stft2_msg_copy(wtk_stft2_msg_t *msg,int channel,int nbin)
{
	wtk_stft2_msg_t *vmsg;

	vmsg=(wtk_stft2_msg_t*)wtk_malloc(sizeof(wtk_stft2_msg_t));
	vmsg->hook=NULL;
	vmsg->fft=wtk_complex_new_p2(nbin,channel);
	vmsg->s=msg->s;
	wtk_complex_cpy_p2(vmsg->fft,msg->fft,nbin,channel);

	return vmsg;
}

wtk_stft2_msg_t* wtk_stft2_msg_new(wtk_stft2_t *stft2)
{
	wtk_stft2_msg_t *msg;

	msg=(wtk_stft2_msg_t*)wtk_malloc(sizeof(wtk_stft2_msg_t));
	msg->hook=NULL;
	msg->fft=wtk_complex_new_p2(stft2->nbin,stft2->cfg->channel);
	return msg;
}

void wtk_stft2_msg_delete(wtk_stft2_t *stft2,wtk_stft2_msg_t *msg)
{
	wtk_complex_delete_p2(msg->fft,stft2->nbin);
	wtk_free(msg);
}

wtk_stft2_msg_t* wtk_stft2_pop_msg(wtk_stft2_t *stft2)
{
	return  (wtk_stft2_msg_t*)wtk_hoard_pop(&(stft2->msg_hoard));
}

void wtk_stft2_push_msg(wtk_stft2_t *stft2,wtk_stft2_msg_t *msg)
{
	//wtk_debug("push [%ld]=%p\n",(long)(msg->hook),msg);
	wtk_hoard_push(&(stft2->msg_hoard),msg);
}

wtk_stft2_t* wtk_stft2_new(wtk_stft2_cfg_t *cfg)
{
	wtk_stft2_t *stft2;
	int i,j;
	int shift;
	int nshift,n;

	stft2=(wtk_stft2_t*)wtk_malloc(sizeof(wtk_stft2_t));
	stft2->cfg=cfg;
	wtk_hoard_init2(&(stft2->msg_hoard),offsetof(wtk_stft2_msg_t,hoard_n),10,
			(wtk_new_handler_t)wtk_stft2_msg_new,
			(wtk_delete_handler2_t)wtk_stft2_msg_delete,
			stft2);
	stft2->input=wtk_float_new_p2(cfg->channel,cfg->win);
	stft2->output=(float *)wtk_malloc(sizeof(float)*cfg->win);
	stft2->rfft=wtk_rfft_new(cfg->win/2);
	if(cfg->use_sine)
	{
		stft2->win=wtk_math_create_sine_window(cfg->win);
		if(cfg->use_fftscale)
		{
			stft2->fft_scale=1.0/sqrt(cfg->win);
		}else
		{
			stft2->fft_scale=1;
		}
	}else if(cfg->use_hamming)
	{
		stft2->win=wtk_math_create_float_ham_window(cfg->win);
		if(cfg->use_fftscale)
		{
			stft2->fft_scale=1.0/sqrt(cfg->win);
		}else
		{
			stft2->fft_scale=1;
		}
	}else if(cfg->use_hann)
	{
		stft2->win=wtk_math_create_hanning_window(cfg->win);
		if(cfg->use_fftscale)
		{
			stft2->fft_scale=1.0/sqrt(cfg->win);
		}else
		{
			stft2->fft_scale=1;
		}
	}else if(cfg->use_conj_window)
	{
		stft2->win=wtk_math_create_conj_window(cfg->win);
		if(cfg->use_fftscale)
		{
			stft2->fft_scale=1.0/sqrt(cfg->win);
		}else
		{
			stft2->fft_scale=1;
		}
	}else
	{
		stft2->win=NULL;
		stft2->fft_scale=1;
	}

	stft2->synthesis_win=NULL;
	if(cfg->use_synthesis_window)
	{
		shift=cfg->win*(1-cfg->overlap);
		nshift=cfg->win / shift;
		// wtk_debug("%d %d  %d\n",nshift,cfg->win,cfg->step);
		stft2->synthesis_win=(float*)wtk_calloc(cfg->win,sizeof(float));
		for(i=0;i<shift;++i)
		{
			for(j=0;j<nshift+1;++j)
			{
				n = i+j*shift;
				if(n < cfg->win)
				{
					stft2->synthesis_win[i] += stft2->win[n]*stft2->win[n];
				}
			}
		}
		for(i=1;i<nshift;++i)
		{
			for(j=0;j<shift;++j)
			{
				stft2->synthesis_win[i*shift+j] = stft2->synthesis_win[j];
			}
		}
		for(i=0;i<cfg->win;++i)
		{
			stft2->synthesis_win[i]=stft2->win[i]/stft2->synthesis_win[i];
		}
	}

	stft2->input_win=(float*)wtk_calloc(cfg->win,sizeof(float));
	stft2->input_fft=(float*)wtk_calloc(cfg->win,sizeof(float));

	stft2->nbin=cfg->win/2+1;

	stft2->notify=NULL;
	stft2->notify_ths=NULL;
	wtk_stft2_reset(stft2);
	return stft2;
}

void wtk_stft2_delete(wtk_stft2_t *stft2)
{
	wtk_hoard_clean(&(stft2->msg_hoard));
	wtk_float_delete_p2(stft2->input,stft2->cfg->channel);
	wtk_free(stft2->output);
	wtk_rfft_delete(stft2->rfft);
	if(stft2->win)
	{
		wtk_free(stft2->win);
	}
	if(stft2->synthesis_win)
	{
		wtk_free(stft2->synthesis_win);
	}
	wtk_free(stft2->input_win);
	wtk_free(stft2->input_fft);
	wtk_free(stft2);
}

void wtk_stft2_reset(wtk_stft2_t *stft2)
{
	//wtk_debug("hoard use=%d free=%d\n",stft2->msg_hoard.use_length,stft2->msg_hoard.cur_free);
	stft2->nframe=0;
	stft2->pos=0;
	stft2->xinput=0;
}

void wtk_stft2_set_notify(wtk_stft2_t *stft2,void *ths,wtk_stft2_notify_f notify)
{
	stft2->notify_ths=ths;
	stft2->notify=notify;
}

void wtk_stft2_update(wtk_stft2_t *stft2,int is_end)
{
	wtk_stft2_cfg_t *cfg=stft2->cfg;
	int win=cfg->win;
	int channel=cfg->channel;
	float *input;
	float *hwin=stft2->win;
	float *input_win=stft2->input_win;
	float *fft=stft2->input_fft;
	int i,j;
	float scale=stft2->fft_scale;
	wtk_stft2_msg_t *msg;
	wtk_complex_t **c;
	int step=win>>1;

	++stft2->nframe;
	msg=wtk_stft2_pop_msg(stft2);
	msg->s=stft2->xinput-stft2->pos;

	for(i=0;i<channel;++i)
	{
		//wtk_debug("i=%d\n",i);
		input=stft2->input[i];
		//print_float(input,win);
		//input *win;
		if(hwin)
		{
			for(j=0;j<win;++j)
			{
				input_win[j]=input[j]*hwin[j];
			}
			//input=> fft;
			wtk_rfft_process_fft(stft2->rfft,fft,input_win);
		}else
		{
			wtk_rfft_process_fft(stft2->rfft,fft,input);
		}
		//wtk_rfft_print_fft(fft,win);
		c=msg->fft;
		if(scale==1)
		{
			c[0][i].a=fft[0];
			c[0][i].b=0;
			for(j=1;j<step;++j)
			{
				c[j][i].a=fft[j];
				c[j][i].b=-fft[j+step];
			}
			c[step][i].a=fft[step];
			c[step][i].b=0;
		}else
		{
			c[0][i].a=fft[0]*scale;
			c[0][i].b=0;
			for(j=1;j<step;++j)
			{
				c[j][i].a=fft[j]*scale;
				c[j][i].b=-fft[j+step]*scale;
			}
			c[step][i].a=fft[step]*scale;
			c[step][i].b=0;
		}
	}
	if(stft2->notify)
	{
		stft2->notify(stft2->notify_ths,msg,stft2->pos,is_end);
	}
}

void wtk_stft2_feed_float(wtk_stft2_t *stft2,float **pv,int n,int is_end)
{
	wtk_stft2_cfg_t *cfg=stft2->cfg;
	int channel=cfg->channel;
	int win=cfg->win;
	float **input=stft2->input;
	int len,step;
	int i,j;
	int pos=stft2->pos;
	float *pf;
	float *sf;
#ifdef DEBUG_BUF
	static wtk_strbuf_t **bufs=NULL;

	if(!bufs)
	{
		bufs=wtk_strbufs_new(channel);
	}
	wtk_strbufs_push_float(bufs,channel,pv,n);
	if(is_end)
	{
		wave_write_file_float3("x1.wav",channel,16000,bufs);
		wtk_strbufs_delete(bufs,channel);
		bufs=NULL;
	}
#endif

	len=0;
	while(len<n)
	{
		step=min(n-len,win-pos);
		for(i=0;i<channel;++i)
		{
			pf=input[i]+pos;
			sf=pv[i]+len;
			for(j=0;j<step;++j)
			{
				pf[j]=sf[j];
			}
		}
		stft2->xinput+=step;
		pos+=step;
		if(pos>=win)
		{
			//calc fft;
			stft2->pos=pos;
			wtk_stft2_update(stft2,0);
			pos-=cfg->step;
			for(i=0;i<channel;++i)
			{
				memmove(input[i],input[i]+cfg->step,pos*sizeof(float));
			}
		}
		len+=step;
	}
	if(is_end)
	{
		if(pos>0)
		{
			for(i=0;i<channel;++i)
			{
				memset(input[i]+pos,0,(win-pos)*sizeof(float));
			}
			stft2->pos=pos;
			//update;
			wtk_stft2_update(stft2,1);
		}else
		{
			if(stft2->notify)
			{
				stft2->notify(stft2->notify_ths,NULL,0,is_end);
			}
		}
		pos=0;
	}
	stft2->pos=pos;
}

void wtk_stft2_feed_int(wtk_stft2_t *stft2,int **pv,int n,int is_end)
{
	static float scale=1.0/(32768.0*32768.0);
	wtk_stft2_cfg_t *cfg=stft2->cfg;
	int channel=cfg->channel;
	int win=cfg->win;
	float **input=stft2->input;
	int len,step;
	int i,j;
	int pos=stft2->pos;
	float *pf;
	int *sf;

	len=0;
	while(len<n)
	{
		step=min(n-len,win-pos);
		for(i=0;i<channel;++i)
		{
			pf=input[i]+pos;
			sf=pv[i]+len;
			for(j=0;j<step;++j)
			{
				pf[j]=sf[j]*scale;
			}
		}
		pos+=step;
		if(pos>=win)
		{
			//calc fft;
			stft2->pos=pos;
			wtk_stft2_update(stft2,0);
			pos-=cfg->step;
			for(i=0;i<channel;++i)
			{
				memmove(input[i],input[i]+cfg->step,pos*sizeof(float));
			}
		}
		len+=step;
	}
	if(is_end)
	{
		if(pos>0)
		{
			for(i=0;i<channel;++i)
			{
				memset(input[i]+pos,0,(win-pos)*sizeof(float));
			}
			stft2->pos=pos;
			//update;
			wtk_stft2_update(stft2,1);
		}else
		{
			if(stft2->notify)
			{
				stft2->notify(stft2->notify_ths,NULL,0,is_end);
			}
		}
		pos=0;
	}
	stft2->pos=pos;
}

void wtk_stft2_feed(wtk_stft2_t *stft2,short **pv,int n,int is_end)
{
	static float scale=1.0/32768.0;
	wtk_stft2_cfg_t *cfg=stft2->cfg;
	int channel=cfg->channel;
	int win=cfg->win;
	float **input=stft2->input;
	int len,step;
	int i,j;
	int pos=stft2->pos;
	float *pf;
	short *sf;
#ifdef DEBUG_WAV
	static wtk_wavfile_t *log=NULL;
	static int ki=0;

	ki+=n;
	//wtk_debug("len=%d end=%d ki=%d time=%f\n",len,is_end,ki,ki*1.0/16000);
	if(!log)
	{
		log=wtk_wavfile_new(16000);
		wtk_wavfile_set_channel(log,stft2->cfg->channel);
		log->max_pend=0;
		wtk_wavfile_open2(log,"stft2");
	}
	if(n>0)
	{
		wtk_wavfile_write_mc(log,pv,n);
	}
	if(is_end)
	{
		wtk_wavfile_close(log);
		wtk_wavfile_delete(log);
		log=NULL;
		ki=0;
	}
#endif
	len=0;
	while(len<n)
	{
		step=min(n-len,win-pos);
		for(i=0;i<channel;++i)
		{
			pf=input[i]+pos;
			sf=pv[i]+len;
			for(j=0;j<step;++j)
			{
				pf[j]=sf[j]*scale;
			}
		}
		stft2->xinput+=step;
		pos+=step;
		if(pos>=win)
		{
			stft2->pos=pos;
			//wtk_debug("input=%d %f\n",stft2->xinput,stft2->xinput*1.0/16000);
			wtk_stft2_update(stft2,0);
			pos-=cfg->step;
			for(i=0;i<channel;++i)
			{
				memmove(input[i],input[i]+cfg->step,pos*sizeof(float));
			}
		}
		len+=step;
	}
	if(is_end)
	{
		if(pos>0)
		{
			for(i=0;i<channel;++i)
			{
				memset(input[i]+pos,0,(win-pos)*sizeof(float));
			}
			stft2->pos=pos;
			//update;
			wtk_stft2_update(stft2,1);
		}else
		{
			if(stft2->notify)
			{
				stft2->notify(stft2->notify_ths,NULL,0,is_end);
			}
		}
		pos=0;
	}
	stft2->pos=pos;
}


void wtk_stft2_feed2(wtk_stft2_t *stft2,short **pv,int n,int is_end)
{
	wtk_stft2_cfg_t *cfg=stft2->cfg;
	int channel=cfg->channel;
	int win=cfg->win;
	float **input=stft2->input;
	int len,step;
	int i,j;
	int pos=stft2->pos;
	float *pf;
	short *sf;
#ifdef DEBUG_WAV
	static wtk_wavfile_t *log=NULL;
	static int ki=0;

	ki+=n;
	//wtk_debug("len=%d end=%d ki=%d time=%f\n",len,is_end,ki,ki*1.0/16000);
	if(!log)
	{
		log=wtk_wavfile_new(16000);
		wtk_wavfile_set_channel(log,stft2->cfg->channel);
		log->max_pend=0;
		wtk_wavfile_open2(log,"stft2");
	}
	if(n>0)
	{
		wtk_wavfile_write_mc(log,pv,n);
	}
	if(is_end)
	{
		wtk_wavfile_close(log);
		wtk_wavfile_delete(log);
		log=NULL;
		ki=0;
	}
#endif
	len=0;
	while(len<n)
	{
		step=min(n-len,win-pos);
		for(i=0;i<channel;++i)
		{
			pf=input[i]+pos;
			sf=pv[i]+len;
			for(j=0;j<step;++j)
			{
				pf[j]=sf[j];
			}
		}
		stft2->xinput+=step;
		pos+=step;
		if(pos>=win)
		{
			stft2->pos=pos;
			//wtk_debug("input=%d %f\n",stft2->xinput,stft2->xinput*1.0/16000);
			wtk_stft2_update(stft2,0);
			pos-=cfg->step;
			for(i=0;i<channel;++i)
			{
				memmove(input[i],input[i]+cfg->step,pos*sizeof(float));
			}
		}
		len+=step;
	}
	if(is_end)
	{
		if(pos>0)
		{
			for(i=0;i<channel;++i)
			{
				memset(input[i]+pos,0,(win-pos)*sizeof(float));
			}
			stft2->pos=pos;
			//update;
			wtk_stft2_update(stft2,1);
		}else
		{
			if(stft2->notify)
			{
				stft2->notify(stft2->notify_ths,NULL,0,is_end);
			}
		}
		pos=0;
	}
	stft2->pos=pos;
}

int wtk_stft2_output_ifft(wtk_stft2_t *stft2,wtk_complex_t *c,float *y,float *pad,int pos,int is_end)
{
	int nbin=stft2->nbin;
	int k,i;
	float *fft;
	float *t;
	float fx;
	int win=nbin-1;
	int n;

	fft=stft2->input_fft;
	t=stft2->input_win;

	fft[0]=c[0].a;
	for(i=1;i<win;++i)
	{
		fft[i]=c[i].a;
		fft[i+win]=-c[i].b;
	}
	fft[win]=c[i].a;
	//wtk_rfft_print_fft(fft,win*2);
	wtk_rfft_process_ifft(stft2->rfft,fft,t);
	fx=1.0/stft2->fft_scale;

	win=stft2->cfg->win;
	n=(win-stft2->cfg->step)*sizeof(float);
	memcpy(y,pad,win*sizeof(float));
	if(is_end)
	{
		if(stft2->cfg->use_synthesis_window)
		{
			for(k=0;k<win;++k)
			{
				y[k]+=t[k]*fx*stft2->synthesis_win[k];
			}
		}else
		{
			for(k=0;k<win;++k)
			{
				y[k]+=t[k]*fx;
			}
		}
		return pos;
	}else
	{
		if(stft2->cfg->use_synthesis_window)
		{
			for(k=0;k<win;++k)
			{
				y[k]+=t[k]*fx*stft2->synthesis_win[k];
			}
		}else
		{
			for(k=0;k<win;++k)
			{
				y[k]+=t[k]*fx;
			}
		}
		memcpy(pad,y+stft2->cfg->step,n);

		return stft2->cfg->step;
	}
}
