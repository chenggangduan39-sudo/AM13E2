#include "wtk_resample.h"
#include "libresample.h"
#include "wtk/core/wavehdr.h"
#include "wtk/core/wtk_os.h"
#include "wtk/core/math/wtk_math.h"

wtk_resample_t* wtk_resample_new(int cache)
{
	wtk_resample_t *r;

	r=(wtk_resample_t*)wtk_calloc(1,sizeof(*r));
	r->tmp=wtk_strbuf_new(1024,1);
	r->input_size=r->output_size=cache;
	r->input=(float*)wtk_calloc(r->input_size,sizeof(float));
	r->output=(float*)wtk_calloc(r->output_size,sizeof(float));
	r->soutput=(short*)wtk_calloc(r->output_size,sizeof(short));
	r->handle=0;
	r->notify=NULL;
	r->notify_ths=NULL;
	r->max_out=32700.0;
	r->use_control_bs=0;
	return r;
}

/*
len: input sample length
src_rate: input sample rate
dst_rate: output sample rate
output_len: the size of output is "factor" times of the size of input
*/
wtk_resample_t* wtk_resample_new2(int len, int src_rate, int dst_rate)
{
	wtk_resample_t *r;
	double factor=dst_rate*1.0/src_rate;

	r=(wtk_resample_t*)wtk_malloc(sizeof(wtk_resample_t));
	// r->tmp=wtk_strbuf_new(1024,1);
	//r->input_size=r->output_size=char_len;
	r->input=(float*)wtk_malloc(len*sizeof(float));
	// r->output = NULL;
	r->output=(float*)wtk_malloc((len+1)*sizeof(float)*factor);
	// r->soutput=(short*)wtk_calloc(r->output_size,sizeof(short));
	r->handle=0;
	r->notify=NULL;
	r->notify_ths=NULL;
	r->max_out=32700.0;
	r->use_control_bs=0;
	return r;
}

int wtk_resample_delete(wtk_resample_t *r)
{
	wtk_resample_close(r);
	wtk_strbuf_delete(r->tmp);
	wtk_free(r->input);
	wtk_free(r->output);
	wtk_free(r->soutput);
	wtk_free(r);
	return 0;
}

int wtk_resample_delete2(wtk_resample_t *r)
{
	wtk_resample_close(r);
	wtk_free(r->input);
	wtk_free(r->output);
	wtk_free(r);
	return 0;
}

int wtk_resample_close(wtk_resample_t *r)
{
	if(r->handle)
	{
		resample_close(r->handle);
		r->handle=0;
	}
	return 0;
}

void wtk_resample_set_notify(wtk_resample_t *r,void *ths,wtk_resample_notify_f notify)
{
	r->notify_ths=ths;
	r->notify=notify;
}

void wtk_resample_set_notify2(wtk_resample_t *r,void *ths,wtk_resample_notify_f notify)
{
	r->notify_ths=ths;
	r->notify=notify;
}

//static int x=0;

int wtk_resample_start(wtk_resample_t *r,int src_rate,int dst_rate)
{
	double factor;

	wtk_resample_close(r);
	factor=dst_rate*1.0/src_rate;
	r->handle=resample_open(1,factor,factor);
	r->factor=factor;
	wtk_strbuf_reset(r->tmp);
	//x=y=0;
	return r->handle?0:-1;
}

int wtk_resample_start2(wtk_resample_t *r,int src_rate,int dst_rate)
{
	double factor;

	wtk_resample_close(r);
	factor=dst_rate*1.0/src_rate;
	r->handle=resample_open(1,factor,factor);
	r->factor=factor;
	return r->handle?0:-1;
}

void wtk_resample_control_bs(wtk_resample_t *f, float *out, int len)
{
	float out_max;
	static float scale=1.0;
	static float last_scale=1.0;
	static int max_cnt=0;
	int i;
	float max_out=f->max_out;

	if(1)
	{
		out_max=wtk_float_abs_max(out, len);
		if(out_max>max_out)
		{
			scale=max_out/out_max;
			if(scale<last_scale)
			{
				last_scale=scale;
			}else
			{
				scale=last_scale;
			}
			max_cnt=5;
		}
		for(i=0; i<len; ++i)
		{
			out[i]*=scale;
		}
		if(max_cnt>0)
		{
			--max_cnt;
		}
		if(max_cnt<=0 && scale<1.0)
		{
			scale*=1.1f;
			last_scale=scale;
			if(scale>1.0)
			{
				scale=1.0;
				last_scale=1.0;
			}
		}
	}else
	{
		scale=1.0;
		last_scale=1.0;
		max_cnt=0;
	}
} 

int wtk_resample_process(wtk_resample_t *r,int is_last,short *data,int len,int *consumed,wtk_strbuf_t *buf)
{
	int i,n;
	short *s,*e,*p;
	int ret,used;
	int last;
	short d;

	//test_resample2(r);
	s=data;
	e=data+len;
	last=0;
	while(1)
	{
		if(!is_last && (e-s)<r->input_size)
		{
			//wtk_debug("break: %d,input=%d,len=%d,left=%d\n",is_last,r->input_size,len,(e-s));
			break;
		}
		p=s;
		for(i=0,n=0;i<r->input_size &&(p<e);++i)
		{
			r->input[i]=*p;
			++p;++n;
		}
		if(is_last && n<r->input_size)
		{
			last=1;
		}
		//wtk_debug("last=%d\n",last);
		used=0;
		ret=resample_process(r->handle,r->factor,r->input,n,last,&used,r->output,r->output_size);
		//wtk_debug("handler=%p,ret=%d,f=%f,%d,last=%d\n",r->handle,ret,r->factor,n,last);
		s+=used;
		//wtk_debug("ret=%d\n",ret);
		if(r->use_control_bs){
			wtk_resample_control_bs(r, r->output, ret);
			for(i=0;i<ret;++i)
			{
				d=floor(0.5+r->output[i]);
				wtk_strbuf_push(buf,(char*)&d,sizeof(short));
			}
		}else{
			for(i=0;i<ret;++i)
			{
				if(fabs(r->output[i])>32767)
				{
					d=r->output[i]>0? 32767:-32767;
				}else
				{
					d=floor(0.5+r->output[i]);
				}
				wtk_strbuf_push(buf,(char*)&d,sizeof(short));
			}
		}
		if(ret<=0){break;}
	}
	*consumed=s-data;
	//x+=*consumed;
	//wtk_debug("x=%d\n",x);
	return 0;
}

int wtk_resample_feed(wtk_resample_t *f,char *data,int len,int is_end)
{
	wtk_strbuf_t *buf=f->tmp;
	short *ps,*pe;
	int step;
	int i;
	float *input=f->input;
	float *output=f->output;
	short *soutput=f->soutput;
	int used;
	int ret;
	int is_lst=0;

	wtk_strbuf_push(buf,data,len);
	ps=(short*)(buf->data);
	pe=(short*)(buf->data+buf->pos);
	while(1)
	{
		step=min(pe-ps,f->input_size);
		if(step<f->input_size)
		{
			if(!is_end)
			{
				break;
			}else
			{
				is_lst=1;
			}
		}
		for(i=0;i<step;++i)
		{
			input[i]=ps[i];
		}
		ret=resample_process(f->handle,f->factor,input,step,is_lst,&used,output,f->output_size);
		ps+=used;
		if(ret>0)
		{
			if(f->use_control_bs){
				wtk_resample_control_bs(f, output, ret);
				for(i=0;i<ret;++i)
				{
					soutput[i]=floor(0.5+output[i]);
				}
			}else{
				for(i=0;i<ret;++i)
				{
					if(fabs(output[i])>32767)
					{
						soutput[i]=output[i]>0? 32767:-32767;
					}else
					{
						soutput[i]=floor(0.5+output[i]);
					}
				}
			}

			if(f->notify)
			{
				f->notify(f->notify_ths,(char*)soutput,ret<<1);
			}
		}
		if(is_lst)
		{
			break;
		}
	}
	ret=(char*)ps -buf->data;
	wtk_strbuf_pop(buf,NULL,ret);
	return 0;
}

int wtk_resample_feed2(wtk_resample_t *f,short *data,int len,int is_end)
{
	int i;
	float *input=f->input;
	float *output=f->output;
	short *soutput=(short *)f->output;
	int ret;
	int is_lst=0;
	int used;

	for(int i=0;i<len;i++){
		input[i]=data[i];
	}

	ret=resample_process(f->handle,f->factor,input,len, is_lst, &used, output, len*f->factor);
	// printf("ret %d\n",ret);
	for(i=0;i<ret;++i)
	{
		if(fabs(output[i])>32767)
		{
			soutput[i]=output[i]>0? 32767:-32767;
		}else
		{
			soutput[i]=floor(0.5+output[i]);
		}
	}
	if(f->notify)
	{
		f->notify(f->notify_ths,(char*)soutput,ret<<1);
	}
	return 0;
}

void wtk_resample_set_control_bs(wtk_resample_t *r, int use_control_bs)
{
	r->use_control_bs=use_control_bs;
}

void wtk_resample_set_max_out(wtk_resample_t *r, float max_out)
{
	r->max_out = max(min(max_out, 32700), 30000);
}
