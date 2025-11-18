#include "wtk_fixstft.h"

wtk_fixstft_msg_t* wtk_fixstft_msg_new(wtk_fixstft_t *fixstft)
{
	wtk_fixstft_msg_t *msg;

	msg=(wtk_fixstft_msg_t*)wtk_malloc(sizeof(wtk_fixstft_msg_t));

	msg->real=(short *)wtk_malloc(fixstft->nbin*sizeof(short));
	msg->imag=(short *)wtk_malloc(fixstft->nbin*sizeof(short));
	return msg;
}


void wtk_fixstft_msg_delete(wtk_fixstft_t *fixstft,wtk_fixstft_msg_t *msg)
{
	wtk_free(msg->real);
	wtk_free(msg->imag);
	wtk_free(msg);
}

wtk_fixstft_msg_t* wtk_fixfft_pop_msg(wtk_fixstft_t *fixstft)
{
	return  (wtk_fixstft_msg_t*)wtk_hoard_pop(&(fixstft->msg_hoard));
}

void wtk_fixstft_push_msg(wtk_fixstft_t *fixstft,wtk_fixstft_msg_t *msg)
{
	wtk_hoard_push(&(fixstft->msg_hoard),msg);
}


wtk_fixstft_t* wtk_fixstft_new(wtk_stft_cfg_t *cfg)
{
	wtk_fixstft_t *fixstft;

	fixstft=(wtk_fixstft_t*)wtk_malloc(sizeof(wtk_fixstft_t));
	fixstft->cfg=cfg;
	wtk_hoard_init2(&(fixstft->msg_hoard),offsetof(wtk_fixstft_msg_t,hoard_n),100,
			(wtk_new_handler_t)wtk_fixstft_msg_new,
			(wtk_delete_handler2_t)wtk_fixstft_msg_delete,
			fixstft);
	fixstft->input=(short*)wtk_malloc(cfg->win*sizeof(short));

	fixstft->fixfft=wtk_fixfft_new(cfg->win);

	fixstft->win=NULL;
	fixstft->fft_scale=1;

//	fixstft->input_win=(short*)wtk_malloc(cfg->win*sizeof(short));
	fixstft->input_fft=(short*)wtk_malloc(cfg->win*sizeof(short));


	fixstft->nbin=cfg->win/2+1;

	fixstft->notify=NULL;
	fixstft->notify_ths=NULL;


	wtk_fixstft_reset(fixstft);
	return fixstft;
}

void wtk_fixstft_delete(wtk_fixstft_t *fixstft)
{
	wtk_hoard_clean(&(fixstft->msg_hoard));
	wtk_free(fixstft->input);

	wtk_fixfft_delete(fixstft->fixfft);

	if(fixstft->win)
	{
		wtk_free(fixstft->win);
	}
//	wtk_free(fixstft->input_win);
	wtk_free(fixstft->input_fft);
	wtk_free(fixstft);
}

void wtk_fixstft_reset(wtk_fixstft_t *fixstft)
{
	fixstft->nframe=0;
	fixstft->pos=0;
	fixstft->xinput=0;
}


void wtk_fixstft_set_notify(wtk_fixstft_t *fixstft,void *ths,wtk_fixstft_notify_f notify)
{
	fixstft->notify_ths=ths;
	fixstft->notify=notify;
}


void wtk_fixstft_update(wtk_fixstft_t *fixstft,int is_end)
{
	wtk_stft_cfg_t *cfg=fixstft->cfg;
	int win=cfg->win;
	short *input=fixstft->input;
	short *fft=fixstft->input_fft;
	wtk_fixstft_msg_t *msg;
	int step=win>>1;
	short *real,*imag;
	int i,j;

	++fixstft->nframe;
	msg=wtk_fixfft_pop_msg(fixstft);
	wtk_fixfft_fft(fixstft->fixfft,input,fft);

	real=msg->real;
	imag=msg->imag;

	real[0]=fft[0]*win;
	imag[0]=0;
	real[step]=fft[win-1]*win;
	imag[step]=0;
	for(i=1,j=1;i<step;++i,j+=2)
	{
		real[i]=fft[j]*win;
		imag[i]=fft[j+1]*win;
	}
	if(fixstft->notify)
	{
		fixstft->notify(fixstft->notify_ths,msg,fixstft->pos,is_end);
	}
}



void wtk_fixstft_feed(wtk_fixstft_t *fixstft,short *pv,int n,int is_end)
{
	wtk_stft_cfg_t *cfg=fixstft->cfg;
	int win=cfg->win;
	short *input=fixstft->input;
	int len,step;
	int pos=fixstft->pos;
#ifdef DEBUG_WAV
	static wtk_wavfile_t *log=NULL;
	static int ki=0;

	ki+=n;
	//wtk_debug("len=%d end=%d ki=%d time=%f\n",len,is_end,ki,ki*1.0/16000);
	if(!log)
	{
		log=wtk_wavfile_new(16000);
		wtk_wavfile_set_channel(log,fixstft->cfg->channel);
		log->max_pend=0;
		wtk_wavfile_open2(log,"fixstft");
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
	//wtk_debug("channel=%d\n",channel);
	len=0;
	while(len<n)
	{
		step=min(n-len,win-pos);
		memcpy(input+pos,pv+len,step*sizeof(short));

		fixstft->xinput+=step;
		pos+=step;
		if(pos>=win)
		{
			fixstft->pos=pos;

			wtk_fixstft_update(fixstft,0);
			pos-=cfg->step;

			memmove(input,input+cfg->step,pos*sizeof(short));
		}
		len+=step;
	}
	if(is_end)
	{
		if(pos>0)
		{
			memset(input+pos,0,(win-pos)*sizeof(short));
			fixstft->pos=pos;

			wtk_fixstft_update(fixstft,1);
		}else
		{
			if(fixstft->notify)
			{
				fixstft->notify(fixstft->notify_ths,NULL,0,is_end);
			}
		}
		pos=0;
	}
	fixstft->pos=pos;
}
