#include "wtk_rfft2.h" 

wtk_fft_item_t* wtk_fft_item_new(int n)
{
	wtk_fft_item_t *item;

	item=(wtk_fft_item_t*)wtk_malloc(sizeof(wtk_fft_item_t));
	item->f=(float*)wtk_calloc(n,sizeof(float));
	return item;
}

void wtk_fft_item_delete(wtk_fft_item_t *item)
{
	wtk_free(item->f);
	wtk_free(item);
}

wtk_rfft2_t* wtk_rfft2_new(wtk_rfft2_cfg_t *cfg)
{
	wtk_rfft2_t *fft;

	fft=(wtk_rfft2_t*)wtk_malloc(sizeof(wtk_rfft2_t));
	fft->cfg=cfg;
	fft->rfft=wtk_rfft_new(cfg->win/2);
	fft->buf=(float*)wtk_calloc(cfg->win,sizeof(float));
	fft->input=(float*)wtk_calloc(cfg->win,sizeof(float));
	fft->fft=(float*)wtk_calloc(cfg->win,sizeof(float));
	fft->notify=NULL;
	fft->notify_ths=NULL;
	wtk_rfft2_reset(fft);
	return fft;
}

void wtk_rfft2_delete(wtk_rfft2_t *fft)
{
	wtk_free(fft->buf);
	wtk_free(fft->input);
	wtk_free(fft->fft);
	wtk_rfft_delete(fft->rfft);
	wtk_free(fft);
}


void wtk_rfft2_reset(wtk_rfft2_t *fft)
{
	fft->pos=0;
}

void wtk_rfft2_set_notify(wtk_rfft2_t *fft,void *ths,wtk_rfft2_notify_f notify)
{
	fft->notify_ths=ths;
	fft->notify=notify;

}

void wtk_rfft2_feed_frame(wtk_rfft2_t *fft,float *x)
{
	int N=fft->cfg->win;
	int i;
	float *winf=fft->cfg->winf;
	float *input=fft->input;
	float *f=fft->fft;

	//print_float(x,N);
	for(i=0;i<N;++i)
	{
		input[i]=x[i]*winf[i];
	}
	//print_float(input,N);
	//exit(0);
	wtk_rfft_process_fft(fft->rfft,f,input);
	if(fft->notify)
	{
		fft->notify(fft->notify_ths,f);
	}else
	{
		wtk_rfft_print_fft(f,N);
		exit(0);
	}
	//exit(0);
}

void wtk_rfft2_feed(wtk_rfft2_t *fft,float *data,int len,int is_end)
{
	int N=fft->cfg->win;
	float *buf=fft->buf;
	float *s,*e;
	int step,left;

	s=data;
	e=s+len;
	while(s<e)
	{
		step=min(e-s,N-fft->pos);
		memcpy(buf+fft->pos,s,step*sizeof(float));
		fft->pos+=step;
		if(fft->pos>=N)
		{
			//wtk_debug("pos=%d step=%d\n",fft->pos,step);
			wtk_rfft2_feed_frame(fft,buf);
			left=N*(1-fft->cfg->overlap);
			fft->pos=N-left;
			memmove(buf,buf+left,fft->pos*sizeof(float));
			//wtk_debug("pos=%d\n",fft->pos);
		}
		s+=step;
	}
	if(is_end)
	{
		if(fft->pos>0)
		{
			memset(buf+fft->pos,0,(N-fft->pos)*sizeof(float));
			wtk_rfft2_feed_frame(fft,buf);
		}
	}
}


void wtk_rfft2_feed2(wtk_rfft2_t *senn,short *data,int len,int is_end)
{
#define N 256
	float buf[N];
	int i;
	short *s,*e;
	int step;
	float fx=1.0/32768;

	s=data;
	e=s+len;
	while(s<e)
	{
		//wtk_debug("data=%p/%p\n",s,e);
		step=min(e-s,N);
		for(i=0;i<step;++i)
		{
			buf[i]=s[i]*fx;
		}
		wtk_rfft2_feed(senn,buf,step,0);
		s+=step;
	}
	if(is_end)
	{
		wtk_rfft2_feed(senn,NULL,0,1);
	}
}
