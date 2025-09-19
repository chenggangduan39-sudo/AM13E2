#include "wtk_vframe.h"
#include <math.h>
#include "wtk/core/wtk_alloc.h"
//#define wtk_float_t float
#define wtk_float_t double

wtk_vframe_t* wtk_vframe_new(int frame_size,int frame_step)
{
	wtk_vframe_t *f;

	f=(wtk_vframe_t*)wtk_malloc(sizeof(*f));
	f->frame_size=frame_size;
	f->frame_step=frame_step;
	f->sample_data=(float*)wtk_malloc(sizeof(float)*frame_size);
	f->wav_data=(short*)wtk_malloc(sizeof(short)*frame_step);
	wtk_frame_reset(f);
	return f;
}

wtk_vframe_t* wtk_vframe_new2(int frame_size,int frame_step)
{
	wtk_vframe_t *f;

	f=(wtk_vframe_t*)wtk_malloc(sizeof(*f));
	f->frame_size=frame_size;
	f->frame_step=frame_step;
	f->sample_data=NULL;
	f->wav_data=(short*)wtk_malloc(sizeof(short*)*frame_step);
	wtk_frame_reset(f);
	return f;
}

wtk_vframe_t* wtk_vframe_new3(int frame_size,int frame_step)
{
	wtk_vframe_t *f;

	f=(wtk_vframe_t*)wtk_malloc(sizeof(*f));
	f->frame_size=frame_size;
	f->frame_step=frame_step;
	f->sample_data=NULL;
	f->wav_data=NULL;
	wtk_frame_reset(f);
	return f;
}

int wtk_frame_reset(wtk_vframe_t *f)
{
	f->energy=0;
	f->speechlike=0;
	f->index=-1;
	f->state=wtk_vframe_sil;
	f->raw_state=wtk_vframe_sil;
	return 0;
}

int wtk_vframe_delete(wtk_vframe_t *f)
{
	if(f->sample_data)
	{
		wtk_free(f->sample_data);
	}
	if(f->wav_data)
	{
		wtk_free(f->wav_data);
	}
	wtk_free(f);
	return 0;
}

double wtk_vframe_wav_mean(wtk_vframe_t *v)
{
	double m;
	int i;

	for(i=0,m=0;i<v->frame_step;++i)
	{
		m+=v->wav_data[i];
	}
	return m;
}

double wtk_vframe_wav_energy(wtk_vframe_t *v,double mean)
{
	double s,t;
	int i;

	for(i=0,s=0;i<v->frame_step;++i)
	{
		t=(v->wav_data[i]-mean);
		s+=t*t;
	}
	return s;
}

float wtk_vframe_calc_snr(wtk_vframe_t *v)
{
	double m,t,s;
	int i;

	for(i=0,m=0;i<v->frame_step;++i)
	{
		m+=v->wav_data[i];
	}
	m/=v->frame_step;
	for(i=0,s=0;i<v->frame_step;++i)
	{
		t=(v->wav_data[i]-m);
		s+=t*t;
	}
	return sqrt(s/v->frame_step);
}

wtk_float_t wtk_vframe_mean2(wtk_vframe_t *f)
{
	wtk_float_t m=0;
	int i;

	for(i=0;i<f->frame_size;++i)
	{
		m+=f->sample_data[i];
	}
	return m/f->frame_size;
}

wtk_float_t wtk_vframe_mean(wtk_vframe_t *f)
{
	wtk_float_t m=0;
	float *ps,*pe;

	ps=f->sample_data;
	pe=ps+f->frame_size;
	while(ps<pe)
	{
		m+=*(ps++);
	}
	return m/f->frame_size;
}

void wtk_vframe_calc_energy(wtk_vframe_t *f)
{
	wtk_float_t t,m,e=0;
	int i;

	//printf("############### %d ###############\n",f->index);
	m=wtk_vframe_mean(f);
	//wtk_debug("m=%f\n",m);
	for(i=0;i<f->frame_size;++i)
	{
		t=(f->sample_data[i]-m);
		e+=t*t;
		//wtk_debug("v[%d]=%f,s=%f,m=%f\n",i+1,e,f->sample_data[i],t*t);
	}
	e+=400000;
	f->energy=e;
}


void wtk_vframe_calc_energy2(wtk_vframe_t *f)
{
	wtk_float_t t,m,e=0;
	int i;

	//printf("############### %d ###############\n",f->index);
	m=wtk_vframe_mean(f);
	//wtk_debug("m=%f\n",m);
	for(i=0;i<f->frame_size;++i)
	{
		t=(f->sample_data[i]-m);
		e+=t*t;
	}
	f->energy=e;
}

float wtk_vframe_calc_energy3(wtk_vframe_t *f)
{
	float t;
	int i;
	float *pv;

	t=0;
	pv=f->sample_data;
	for(i=0;i<f->frame_size;++i)
	{
		t+=pv[i]*pv[i];
		//wtk_debug("v[%d]=%f\n",i,pv[i]);
	}
	t/=f->frame_size;
	return t;
}

void wtk_vframe_print(wtk_vframe_t *v)
{
	printf("[%d]=%s\n",v->index,v->state==wtk_vframe_sil?"sil":"speech");
}

wtk_string_t* wtk_vframe_state_to_string(wtk_vframe_state_t state)
{
	static wtk_string_t states[]={
			wtk_string("sil"),wtk_string("speech"),wtk_string("speech_end")
	};
	return &(states[state]);
}
