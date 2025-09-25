#include "wtk_vvad.h" 

void wtk_vvad_hanning_window(float *f,int n)
{
	int i;
	float t1,t2;

	t1=2*PI/(n+1);
	for(i=0,t2=t1;i<n;++i,t2+=t1)
	{
		f[i]=0.5*(1-cos(t2));
	}
}

wtk_vvad_t* wtk_vvad_new()
{
	wtk_vvad_t *v;

	v=(wtk_vvad_t*)malloc(sizeof(wtk_vvad_t));
	v->rfft=wtk_drft_new2(WINS);
	v->fft=(wtk_complex_t *)malloc(sizeof(wtk_complex_t)*NBIN);
	v->amp=(float*)malloc(sizeof(float)*NBIN);
	v->window=(float*)malloc(sizeof(float)*WINS);
	v->rfft_in=(float*)malloc(sizeof(float)*WINS);
	v->mem=(float*)malloc(sizeof(float)*WINS);

	wtk_vvad_reset(v);
	return v;
}

void wtk_vvad_delete(wtk_vvad_t *v)
{
	wtk_drft_delete2(v->rfft);
	free(v->fft);
	free(v->amp);
	free(v->window);
	free(v->rfft_in);
	free(v->mem);
	free(v);
}


void wtk_vvad_reset(wtk_vvad_t *v)
{
	memset(v->fft,0,sizeof(wtk_complex_t)*NBIN);
	memset(v->amp,0,sizeof(float)*NBIN);
	wtk_vvad_hanning_window(v->window,WINS);
	memset(v->rfft_in,0,sizeof(float)*WINS);
	memset(v->mem,0,sizeof(float)*WINS);

	v->end_frames=END_FRAMES;
	v->thresh=THRESH;
	v->end_cnt=0;
	v->min_idx=floor(MIN_FREQ*NBIN/RATE)+1;
	v->max_idx=floor(MAX_FREQ*NBIN/RATE);
}

float wtk_vvad_energy(float *p,int n, int min_idx, int max_idx)
{
	float f,f2;
	int i;

	f=0;
	for(i=min_idx;i<=max_idx;++i)
	{
		f+=p[i];
	}
    f/=n;

    f2=0;
	for(i=min_idx;i<=max_idx;++i)
	{
		f2+=(p[i]-f)*(p[i]-f);
	}
	f2/=n;

	return f2;
}

int wtk_vvad_feed(wtk_vvad_t *v,short *data)
{
	float energy;
	wtk_drft_frame_analysis22(v->rfft, v->rfft_in, v->mem, v->fft, data, WINS, v->window);
	for(int i=0;i<NBIN;++i)
	{
		v->amp[i]=v->fft[i].a*v->fft[i].a+v->fft[i].b*v->fft[i].b;
	}
	energy = wtk_vvad_energy(v->amp, NBIN, v->min_idx, v->max_idx);
	// printf("%f\n",min(energy, 10000));
	if(energy>v->thresh){
		v->end_cnt=v->end_frames;
	}else{
		--v->end_cnt;
	}
	if(v->end_cnt<0){
		return 0;
	}
	return 1;
}

void wtk_vvad_set_conf(wtk_vvad_t *v,float thresh,int end_frames)
{
	v->thresh=thresh;
	v->end_frames=end_frames;
}
