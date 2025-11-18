#include "wtk_stft.h" 


wtk_stft_msg_t* wtk_stft_msg_copy(wtk_stft_msg_t *msg,int channel,int nbin)
{
	wtk_stft_msg_t *vmsg;

	vmsg=(wtk_stft_msg_t*)wtk_malloc(sizeof(wtk_stft_msg_t));
	vmsg->hook=NULL;
	vmsg->fft=wtk_complex_new_p2(channel,nbin);
	vmsg->s=msg->s;
	wtk_complex_cpy_p2(vmsg->fft,msg->fft,channel,nbin);

	return vmsg;
}

wtk_stft_msg_t* wtk_stft_msg_new(wtk_stft_t *stft)
{
	wtk_stft_msg_t *msg;

	msg=(wtk_stft_msg_t*)wtk_malloc(sizeof(wtk_stft_msg_t));
	msg->hook=NULL;
	msg->fft=wtk_complex_new_p2(stft->cfg->channel,stft->nbin);
	return msg;
}

int wtk_stft_msg_bytes(int c,int nbin)
{
	int cnt;

	cnt=sizeof(wtk_stft_msg_t);
	cnt+=wtk_complex_bytes_p2(c,nbin);
	return cnt;
}

void wtk_stft_msg_delete(wtk_stft_t *stft,wtk_stft_msg_t *msg)
{
	wtk_complex_delete_p2(msg->fft,stft->cfg->channel);
	wtk_free(msg);
}

wtk_stft_msg_t* wtk_stft_pop_msg(wtk_stft_t *stft)
{
	return  (wtk_stft_msg_t*)wtk_hoard_pop(&(stft->msg_hoard));
}

void wtk_stft_push_msg(wtk_stft_t *stft,wtk_stft_msg_t *msg)
{
	//wtk_debug("push [%ld]=%p\n",(long)(msg->hook),msg);
	wtk_hoard_push(&(stft->msg_hoard),msg);
}

int wtk_stft_bytes(wtk_stft_t *stft)
{
	int cnt;

	cnt=sizeof(wtk_stft_t);
	cnt+=(stft->msg_hoard.use_length+stft->msg_hoard.cur_free)*wtk_stft_msg_bytes(stft->cfg->channel,stft->nbin);
	//if(stft->msg_hoard.use_length+stft->msg_hoard.cur_free>0)
	{
		wtk_debug("use=%d free=%d\n",stft->msg_hoard.use_length,stft->msg_hoard.cur_free);
	}
	if(stft->msg_hoard.use_length>0)
	{
		exit(0);
	}
	cnt+=wtk_float_bytes_p2(stft->cfg->channel,stft->cfg->win);
	cnt+=stft->cfg->output_channel*sizeof(float*)+stft->cfg->output_channel*stft->cfg->win*sizeof(float);
	cnt+=wtk_rfft_bytes(stft->rfft);
	cnt+=wtk_float_bytes_p2(stft->cfg->output_channel,stft->cfg->win);
	cnt+=stft->cfg->win*sizeof(float)*3;
	return cnt;
}

wtk_stft_t* wtk_stft_new(wtk_stft_cfg_t *cfg)
{
	wtk_stft_t *stft;
	int i;

	stft=(wtk_stft_t*)wtk_malloc(sizeof(wtk_stft_t));
	stft->cfg=cfg;
	wtk_hoard_init2(&(stft->msg_hoard),offsetof(wtk_stft_msg_t,hoard_n),cfg->cache,
			(wtk_new_handler_t)wtk_stft_msg_new,
			(wtk_delete_handler2_t)wtk_stft_msg_delete,
			stft);
	stft->input=wtk_float_new_p2(cfg->channel,cfg->win);
	//stft->output=(short*)wtk_calloc(cfg->win,sizeof(short));
	stft->output=(float**)wtk_calloc(cfg->output_channel,sizeof(float*));
	for(i=0;i<cfg->output_channel;++i)
	{
		stft->output[i]=(float*)wtk_calloc(cfg->win,sizeof(float));
	}
	stft->rfft=wtk_rfft_new(cfg->win/2);
	stft->pad=wtk_float_new_p2(cfg->output_channel,cfg->win);
	if(cfg->use_sine)
	{
		stft->win=wtk_math_create_sine_window(cfg->win);
		if(cfg->use_fftscale)
		{
			stft->fft_scale=1.0/sqrt(cfg->win);
		}else
		{
			stft->fft_scale=1;
		}
	}else if(cfg->use_hamming)
	{
		stft->win=wtk_math_create_float_ham_window(cfg->win);
		if(cfg->use_fftscale)
		{
			stft->fft_scale=1.0/sqrt(cfg->win);
		}else
		{
			stft->fft_scale=1;
		}
	}else if(cfg->use_hann)
	{
		stft->win=wtk_math_create_hanning_window(cfg->win);
		if(cfg->use_fftscale)
		{
			stft->fft_scale=1.0/sqrt(cfg->win);
		}else
		{
			stft->fft_scale=1;
		}
	}else if(cfg->use_conj_window)
	{
		stft->win=wtk_math_create_conj_window(cfg->win);
		if(cfg->use_fftscale)
		{
			stft->fft_scale=1.0/sqrt(cfg->win);
		}else
		{
			stft->fft_scale=1;
		}
	}else
	{
		stft->win=NULL;
		stft->fft_scale=1;
	}

	stft->synthesis_win=NULL;
	if(cfg->use_synthesis_window)
	{
		int shift;
		int nshift,n;
		int i,j;

		shift=cfg->win*(1-cfg->overlap);
		nshift=cfg->win / shift;
		// wtk_debug("%d %d  %d\n",nshift,cfg->win,cfg->step);
		stft->synthesis_win=(float*)wtk_calloc(cfg->win,sizeof(float));
		for(i=0;i<shift;++i)
		{
			for(j=0;j<nshift+1;++j)
			{
				n = i + j * shift;
				if(n < cfg->win)
				{
					stft->synthesis_win[i] += stft->win[n]*stft->win[n];
				}
			}
		}
		for(i=1;i<nshift;++i)
		{
			for(j=0;j<shift;++j)
			{
				stft->synthesis_win[i*shift+j] = stft->synthesis_win[j];
			}
		}
		for(i=0;i<cfg->win;++i)
		{
			stft->synthesis_win[i]=stft->win[i]/stft->synthesis_win[i];
		}
	}
	//print_float(stft->win,10);//cfg->win);
	//exit(0);
	stft->input_win=(float*)wtk_calloc(cfg->win,sizeof(float));
	stft->input_fft=(float*)wtk_calloc(cfg->win,sizeof(float));
	//stft->fft_scale*=20;

	stft->nbin=cfg->win/2+1;

	stft->notify=NULL;
	stft->notify_ths=NULL;

	stft->notify_float=NULL;
	stft->notify_float_ths=NULL;

	stft->fft=(wtk_complex_t*)wtk_malloc(stft->nbin*sizeof(wtk_complex_t));
	wtk_stft_reset(stft);
	return stft;
}

void wtk_stft_delete(wtk_stft_t *stft)
{
	int i;

	wtk_free(stft->fft);
	for(i=0;i<stft->cfg->channel;++i)
	{
		wtk_free(stft->output[i]);
	}
	wtk_free(stft->output);
	wtk_hoard_clean(&(stft->msg_hoard));
	wtk_float_delete_p2(stft->input,stft->cfg->channel);
	//wtk_free(stft->output);
	wtk_rfft_delete(stft->rfft);
	wtk_float_delete_p2(stft->pad,stft->cfg->channel);
	if(stft->win)//stft->cfg->use_hamming || stft->cfg->use_hann || stft->cfg->use_sine)
	{
		wtk_free(stft->win);
	}
	if(stft->synthesis_win)
	{
		wtk_free(stft->synthesis_win);
	}
	wtk_free(stft->input_win);
	wtk_free(stft->input_fft);
	wtk_free(stft);
}

void wtk_stft_reset(wtk_stft_t *stft)
{
	//wtk_debug("hoard use=%d free=%d\n",stft->msg_hoard.use_length,stft->msg_hoard.cur_free);
	stft->nsample=0;
	stft->nframe=0;
	stft->pos=0;
	stft->cancel=0;
	stft->xinput=0;
	wtk_float_zero_p2(stft->pad,stft->cfg->channel,stft->cfg->win);
}

void wtk_stft_reset_output_pad(wtk_stft_t *stft)
{
	wtk_float_zero_p2(stft->pad,stft->cfg->channel,stft->cfg->win);
}

void wtk_stft_set_notify(wtk_stft_t *stft,void *ths,wtk_stft_notify_f notify)
{
	stft->notify_ths=ths;
	stft->notify=notify;
}


void wtk_stft_set_float_notify(wtk_stft_t *stft,void *ths,wtk_stft_notify_float_f notify)
{
	stft->notify_float_ths=ths;
	stft->notify_float=notify;
}


void wtk_stft_update(wtk_stft_t *stft,int is_end)
{
	wtk_stft_cfg_t *cfg=stft->cfg;
	int win=cfg->win;
	int channel=cfg->channel;
	float *input;
	float *hwin=stft->win;
	float *input_win=stft->input_win;
	float *fft=stft->input_fft;
	int i,j;
	float scale=stft->fft_scale;
	wtk_stft_msg_t *msg;
	wtk_complex_t *c;
	int step=win>>1;

	++stft->nframe;
	msg=wtk_stft_pop_msg(stft);
#ifdef DEBUG_IDX
	//wtk_debug("xinput=%ld pos=%d is_end=%d\n",stft->xinput,stft->pos,is_end);
//	if(is_end)
//	{
//
//	}else
//	{
//		msg->s=stft->xinput-stft->cfg->win;
//	}
	msg->s=stft->xinput-stft->pos;
#endif
	for(i=0;i<channel;++i)
	{
		//wtk_debug("i=%d\n",i);
		input=stft->input[i];
		//print_float(input,win);
		//input *win;
		if(hwin)
		{
			for(j=0;j<win;++j)
			{
				input_win[j]=input[j]*hwin[j];
			}
			//input=> fft;
			wtk_rfft_process_fft(stft->rfft,fft,input_win);
		}else
		{
			wtk_rfft_process_fft(stft->rfft,fft,input);
		}
		//wtk_rfft_print_fft(fft,win);
		c=msg->fft[i];
		if(scale==1)
		{
			c[0].a=fft[0];
			c[0].b=0;
			for(j=1;j<step;++j)
			{
				c[j].a=fft[j];
				c[j].b=-fft[j+step];
			}
			c[step].a=fft[step];
			c[step].b=0;
		}else
		{
			c[0].a=fft[0]*scale;
			c[0].b=0;
			for(j=1;j<step;++j)
			{
				c[j].a=fft[j]*scale;
				c[j].b=-fft[j+step]*scale;
			}
			c[step].a=fft[step]*scale;
			c[step].b=0;
		}
	}
	if(stft->notify)
	{
		stft->notify(stft->notify_ths,msg,stft->pos,is_end);
	}
}

void wtk_stft_feed_float2(wtk_stft_t *stft,float **pv,int n,int is_end)
{
	wtk_stft_cfg_t *cfg=stft->cfg;
	int channel=cfg->channel;
	int win=cfg->win;
	float **input=stft->input;
	int len,step;
	int i,j;
	int pos=stft->pos;
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
		stft->xinput+=step;
		pos+=step;
		if(pos>=win)
		{
			//calc fft;
			stft->pos=pos;
			wtk_stft_update(stft,0);
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
			stft->pos=pos;
			//update;
			wtk_stft_update(stft,1);
		}else
		{
			if(stft->notify)
			{
				stft->notify(stft->notify_ths,NULL,0,is_end);
			}
		}
		pos=0;
	}
	stft->pos=pos;
}

#include "wtk/core/wtk_wavfile.h"

void wtk_stft_feed_int(wtk_stft_t *stft,int **pv,int n,int is_end)
{
	static float scale=1.0/(32768.0*32768.0);
	wtk_stft_cfg_t *cfg=stft->cfg;
	int channel=cfg->channel;
	int win=cfg->win;
	float **input=stft->input;
	int len,step;
	int i,j;
	int pos=stft->pos;
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
			stft->pos=pos;
			wtk_stft_update(stft,0);
			pos-=cfg->step;
			for(i=0;i<channel;++i)
			{
				memmove(input[i],input[i]+cfg->step,pos*sizeof(float));
			}
		}
		len+=step;
		if(stft->cancel)
		{
			break;
		}
	}
	if(is_end && !stft->cancel)
	{
		if(pos>0)
		{
			for(i=0;i<channel;++i)
			{
				memset(input[i]+pos,0,(win-pos)*sizeof(float));
			}
			stft->pos=pos;
			//update;
			wtk_stft_update(stft,1);
		}else
		{
			if(stft->notify)
			{
				stft->notify(stft->notify_ths,NULL,0,is_end);
			}
		}
		pos=0;
	}
	stft->pos=pos;
}

void wtk_stft_feed(wtk_stft_t *stft,short **pv,int n,int is_end)
{
	static float scale=1.0/32768.0;
	wtk_stft_cfg_t *cfg=stft->cfg;
	int channel=cfg->channel;
	int win=cfg->win;
	float **input=stft->input;
	int len,step;
	int i,j;
	int pos=stft->pos;
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
		wtk_wavfile_set_channel(log,stft->cfg->channel);
		log->max_pend=0;
		wtk_wavfile_open2(log,"stft");
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
	stft->nsample+=n;
	//wtk_debug("channel=%d\n",channel);
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
		stft->xinput+=step;
		pos+=step;
		if(pos>=win)
		{
			//calc fft;
			stft->pos=pos;
			//wtk_debug("input=%d %f\n",stft->xinput,stft->xinput*1.0/16000);
			wtk_stft_update(stft,0);
			pos-=cfg->step;
			for(i=0;i<channel;++i)
			{
				memmove(input[i],input[i]+cfg->step,pos*sizeof(float));
			}
		}
		len+=step;
		if(stft->cancel)
		{
			break;
		}
	}
	if(is_end && !stft->cancel)
	{
		if(pos>0)
		{
			for(i=0;i<channel;++i)
			{
				memset(input[i]+pos,0,(win-pos)*sizeof(float));
			}
			stft->pos=pos;
			//update;
			wtk_stft_update(stft,1);
		}else
		{
			if(stft->notify)
			{
				stft->notify(stft->notify_ths,NULL,0,is_end);
			}
		}
		pos=0;
	}
	stft->pos=pos;
}


void wtk_stft_feed3(wtk_stft_t *stft,short **pv,int n,int is_end)
{
	wtk_stft_cfg_t *cfg=stft->cfg;
	int channel=cfg->channel;
	int win=cfg->win;
	float **input=stft->input;
	int len,step;
	int i,j;
	int pos=stft->pos;
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
		wtk_wavfile_set_channel(log,stft->cfg->channel);
		log->max_pend=0;
		wtk_wavfile_open2(log,"stft");
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
	stft->nsample+=n;
	//wtk_debug("channel=%d\n",channel);
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
		stft->xinput+=step;
		pos+=step;
		if(pos>=win)
		{
			//calc fft;
			stft->pos=pos;
			//wtk_debug("input=%d %f\n",stft->xinput,stft->xinput*1.0/16000);
			wtk_stft_update(stft,0);
			pos-=cfg->step;
			for(i=0;i<channel;++i)
			{
				memmove(input[i],input[i]+cfg->step,pos*sizeof(float));
			}
		}
		len+=step;
		if(stft->cancel)
		{
			break;
		}
	}
	if(is_end && !stft->cancel)
	{
		if(pos>0)
		{
			for(i=0;i<channel;++i)
			{
				memset(input[i]+pos,0,(win-pos)*sizeof(float));
			}
			stft->pos=pos;
			//update;
			wtk_stft_update(stft,1);
		}else
		{
			if(stft->notify)
			{
				stft->notify(stft->notify_ths,NULL,0,is_end);
			}
		}
		pos=0;
	}
	stft->pos=pos;
}

void wtk_stft_feed_float(wtk_stft_t *stft,float *data,int n,int is_end)
{
	wtk_stft_cfg_t *cfg=stft->cfg;
	int channel=cfg->channel;
	int win=cfg->win;
	float **input=stft->input;
	float *xinput;
	int len,step;
	int i,j;
	int pos=stft->pos;

	len=0;
	if(channel==1)
	{
		xinput=input[0];
		while(len<n)
		{
			step=min(n-len,win-pos);
			for(i=0;i<step;++i)
			{
				xinput[pos+i]=*(data++);
			}
			pos+=step;
			if(pos>=win)
			{
				//calc fft;
				stft->pos=pos;
				wtk_stft_update(stft,0);
				pos-=cfg->step;
				memmove(xinput,xinput+cfg->step,pos*sizeof(float));
			}
			len+=step;
		}
		if(is_end)
		{
			if(pos>0)
			{
				memset(xinput,0,(win-pos)*sizeof(float));
				stft->pos=pos;
				//wtk_debug("pos=%d\n",stft->pos);
				//update;
				wtk_stft_update(stft,1);
			}else
			{
				if(stft->notify)
				{
					stft->notify(stft->notify_ths,NULL,0,is_end);
				}
			}
			pos=0;
		}
		stft->pos=pos;
	}else
	{
		while(len<n)
		{
			step=min(n-len,win-pos);
			for(i=0;i<step;++i)
			{
				for(j=0;j<channel;++j)
				{
					input[j][pos+i]=*(data++);
				}
			}
			pos+=step;
			if(pos>=win)
			{
				//calc fft;
				stft->pos=pos;
				wtk_stft_update(stft,0);
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
				stft->pos=pos;
				//wtk_debug("pos=%d\n",stft->pos);
				//update;
				wtk_stft_update(stft,1);
			}else
			{
				if(stft->notify)
				{
					stft->notify(stft->notify_ths,NULL,0,is_end);
				}
			}
			pos=0;
		}
		stft->pos=pos;
	}
}

void wtk_stft_feed2(wtk_stft_t *stft,short *pv,int n,int is_end)
{
	static float scale=1.0/32768.0;
	wtk_stft_cfg_t *cfg=stft->cfg;
	int channel=cfg->channel;
	int win=cfg->win;
	float **input=stft->input;
	float *xinput;
	int len,step;
	int i,j;
	int pos=stft->pos;

	len=0;
	if(channel==1)
	{
		xinput=input[0];
		while(len<n)
		{
			step=min(n-len,win-pos);
			for(i=0;i<step;++i)
			{
				xinput[pos+i]=*(pv++)*scale;
			}
			pos+=step;
			if(pos>=win)
			{
				//calc fft;
				stft->pos=pos;
				wtk_stft_update(stft,0);
				pos-=cfg->step;
				memmove(xinput,xinput+cfg->step,pos*sizeof(float));
				stft->xinput+=cfg->step;
			}
			len+=step;
		}
		if(is_end)
		{
			if(pos>0)
			{
				memset(xinput,0,(win-pos)*sizeof(float));
				stft->pos=pos;
				//wtk_debug("pos=%d\n",stft->pos);
				//update;
				wtk_stft_update(stft,1);
				stft->xinput+=pos;
			}else
			{
				if(stft->notify)
				{
					stft->notify(stft->notify_ths,NULL,0,is_end);
				}
			}
			pos=0;
		}
		stft->pos=pos;
	}else
	{
		while(len<n)
		{
			step=min(n-len,win-pos);
			for(i=0;i<step;++i)
			{
				for(j=0;j<channel;++j)
				{
					input[j][pos+i]=*(pv++)*scale;
				}
			}
			pos+=step;
			if(pos>=win)
			{
				//calc fft;
				stft->pos=pos;
				wtk_stft_update(stft,0);
				pos-=cfg->step;
				for(i=0;i<channel;++i)
				{
					memmove(input[i],input[i]+cfg->step,pos*sizeof(float));
				}
				stft->xinput+=cfg->step;
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
				stft->pos=pos;
				//wtk_debug("pos=%d\n",stft->pos);
				//update;
				wtk_stft_update(stft,1);
				stft->xinput+=pos;
			}else
			{
				if(stft->notify)
				{
					stft->notify(stft->notify_ths,NULL,0,is_end);
				}
			}
			pos=0;
		}
		stft->pos=pos;
	}
}
//
//void wtk_stft_notify_sample(wtk_stft_t *stft,float *y,int n,int channel)
//{
//	short *pv=stft->output;
//	int win=stft->cfg->win;
//	float *ps,*pe;
//	float scale=32767.0/2;//32768.0;
//	int step,i;
//
//	if(stft->notify_float)
//	{
//		stft->notify_float(stft->notify_float_ths,y,n,channel);
//		return;
//	}
//	ps=y;
//	pe=y+n;
//	while(ps<pe)
//	{
//		step=min(pe-ps,win);
//		for(i=0;i<step;++i)
//		{
//			pv[i]=(short)(ps[i]*scale);
//		}
//		//print_short(pv,step);
//		if(stft->notify_sample)
//		{
//			stft->notify_sample(stft->notify_sample_ths,(char*)pv,step<<1,channel);
//		}
//		ps+=step;
//	}
//}

int wtk_stft_output_ifft2(wtk_stft_t *stft,wtk_complex_t *c,int channel,int pos,int is_end,float *y,float *pad)
{
	int nbin=stft->nbin;
	int k,i;
	float *fft;
	float *t;
	float fx;
	int win=nbin-1;//mvdr->cfg->win;
	int n;

	fft=stft->input_fft;
	t=stft->input_win;
	//exit(0);
	fft[0]=c[0].a;
	for(i=1;i<win;++i)
	{
		fft[i]=c[i].a;
		fft[i+win]=-c[i].b;
	}
	fft[win]=c[i].a;
	//wtk_rfft_print_fft(fft,win*2);
	wtk_rfft_process_ifft(stft->rfft,fft,t);
	fx=1.0/stft->fft_scale;
	//y=fft;
	win=stft->cfg->win;
	n=(win-stft->cfg->step)*sizeof(float);
	memcpy(y,pad,win*sizeof(float));
	//wtk_debug("========== ki=%d =========\n",ki);
	if(is_end)
	{
		//wtk_stft_notify_sample(stft,y,pos,channel);
		//wtk_debug("pos=%d win=%d nbin=%d\n",pos,win,stft->nbin);
		if(pos<win)
		{
			//不足窗设置为0;
			//wtk_debug("========= 123 ============\n");
			//memset(y,0,pos*sizeof(float));
			memcpy(y,pad,win*sizeof(float));
			return pos;
		}
		if(stft->cfg->use_synthesis_window)
		{
			for(k=0;k<win;++k)
			{
				y[k]+=t[k]*fx*stft->synthesis_win[k];
			}
		}else
		{
			for(k=0;k<win;++k)
			{
				y[k]+=t[k]*fx;
			}
		}
		if(stft->cfg->keep_win)
		{
			return win;
		}else
		{
			return pos;
		}
	}else
	{
		if(stft->cfg->use_synthesis_window)
		{
			for(k=0;k<win;++k)
			{
				y[k]+=t[k]*fx*stft->synthesis_win[k];
			}
		}else
		{
			for(k=0;k<win;++k)
			{
				y[k]+=t[k]*fx;
			}
		}
		memcpy(pad,y+stft->cfg->step,n);
		return stft->cfg->step;
		//wtk_stft_notify_sample(stft,y,stft->cfg->step,channel);
	}
}

int wtk_stft_output_ifft(wtk_stft_t *stft,wtk_complex_t *c,int channel,int pos,int is_end,float *y)
{
	int nbin=stft->nbin;
	int k,i;
	float *fft;
	float *t;
	float fx;
	int win=nbin-1;//mvdr->cfg->win;
	float *pad=stft->pad[channel];
	int n;

	fft=stft->input_fft;
	t=stft->input_win;
	//exit(0);
	fft[0]=c[0].a;
	for(i=1;i<win;++i)
	{
		fft[i]=c[i].a;
		fft[i+win]=-c[i].b;
	}
	fft[win]=c[i].a;
	//wtk_rfft_print_fft(fft,win*2);
	wtk_rfft_process_ifft(stft->rfft,fft,t);
	fx=1.0/stft->fft_scale;
	//y=fft;
	win=stft->cfg->win;
	n=(win-stft->cfg->step)*sizeof(float);
	memcpy(y,pad,win*sizeof(float));
	//wtk_debug("========== ki=%d =========\n",ki);
	if(is_end)
	{
		//wtk_stft_notify_sample(stft,y,pos,channel);
		//wtk_debug("pos=%d win=%d nbin=%d\n",pos,win,stft->nbin);
		if(pos<win)
		{
			//不足窗设置为0;
			//wtk_debug("========= 123 ============\n");
			//memset(y,0,pos*sizeof(float));
			memcpy(y,pad,win*sizeof(float));
			return pos;
		}
		if(stft->cfg->use_synthesis_window)
		{
			for(k=0;k<win;++k)
			{
				y[k]+=t[k]*fx*stft->synthesis_win[k];
			}
		}else
		{
			for(k=0;k<win;++k)
			{
				y[k]+=t[k]*fx;
			}
		}
		if(stft->cfg->keep_win)
		{
			return win;
		}else
		{
			return pos;
		}
	}else
	{
		if(stft->cfg->use_synthesis_window)
		{
			for(k=0;k<win;++k)
			{
				y[k]+=t[k]*fx*stft->synthesis_win[k];
			}
		}else
		{
			for(k=0;k<win;++k)
			{
				y[k]+=t[k]*fx;
			}
		}
		memcpy(pad,y+stft->cfg->step,n);
		return stft->cfg->step;
		//wtk_stft_notify_sample(stft,y,stft->cfg->step,channel);
	}
}

void wtk_stft_process_ifft(wtk_stft_t *stft,wtk_complex_t ***fft,int n,int last_pos,int is_end)
{
	int channel=stft->cfg->channel;
	int i,j;
	float **output=stft->output;
	int len=0;

	if(n>0)
	{
		if(is_end)
		{
			for(i=0;i<n;++i)
			{
				len=0;
				if(i==n-1)
				{
					for(j=0;j<channel;++j)
					{
						len=wtk_stft_output_ifft(stft,fft[i][j],j,last_pos,is_end,output[j]);
					}
				}else
				{
					for(j=0;j<channel;++j)
					{
						len=wtk_stft_output_ifft(stft,fft[i][j],j,0,0,output[j]);
					}
				}
				if(len>0 && stft->notify_float)
				{
					stft->notify_float(stft->notify_float_ths,output,len);
				}
			}
		}else
		{
			for(i=0;i<n;++i)
			{
				len=0;
				for(j=0;j<channel;++j)
				{
					len=wtk_stft_output_ifft(stft,fft[i][j],j,0,0,output[j]);
				}
				if(len>0 && stft->notify_float)
				{
					stft->notify_float(stft->notify_float_ths,output,len);
				}
			}
		}
	}
//	if(is_end)
//	{
//		j=stft->cfg->win-stft->cfg->step;
//		for(i=0;i<channel;++i)
//		{
//			wtk_stft_notify_sample(stft,stft->pad[i],j,i);
//		}
//	}
}

void wtk_stft_msg_update_rnn(wtk_stft_msg_t *msg,wtk_complex_t **rnn,int c,int nbin)
{
	wtk_complex_t *fft1,*fft2,*a;
	int i,j,k;

	for(i=0;i<c;++i)
	{
		fft1=msg->fft[i];
		for(j=i;j<c;++j)
		{
			fft2=msg->fft[j];
			for(k=0;k<nbin;++k)
			{
				//(a+bi)*(c-di)=(ac+bd)+i(-ad+bc);
				a=rnn[k]+i*c+j;
				a->a+=fft1[k].a*fft2[k].a +fft1[k].b*fft2[k].b;
				a->b+=-fft1[k].a*fft2[k].b +fft1[k].b*fft2[k].a;
			}
		}
	}
}



void wtk_stft_msg_update_rnn3(wtk_stft_msg_t *msg,wtk_complex_t **rnn,int c,int nbin,float alpha2)
{
	wtk_complex_t *fft1,*fft2,*a;
	int i,j,k;

	for(i=0;i<c;++i)
	{
		fft1=msg->fft[i];
		for(j=i;j<c;++j)
		{
			fft2=msg->fft[j];
			for(k=0;k<nbin;++k)
			{
				//(a+bi)*(c-di)=(ac+bd)+i(-ad+bc);
				a=rnn[k]+i*c+j;
				a->a+=alpha2*(fft1[k].a*fft2[k].a +fft1[k].b*fft2[k].b);
				a->b+=alpha2*(-fft1[k].a*fft2[k].b +fft1[k].b*fft2[k].a);

			}
		}
	}
}


