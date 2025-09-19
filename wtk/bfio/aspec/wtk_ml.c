#include "wtk_ml.h"

wtk_ml_t *wtk_ml_new(int nbin,int channel,int ang_num)
{
    wtk_ml_t *ml;

    ml=(wtk_ml_t *)wtk_malloc(sizeof(wtk_ml_t));
    ml->nbin=nbin;
    ml->channel=channel;
    ml->ang_num=ang_num;
    ml->ovec=wtk_complex_new_p3(ang_num,nbin,channel);
    
    return ml;
}

void wtk_ml_reset(wtk_ml_t *ml)
{
    wtk_complex_zero_p3(ml->ovec, ml->ang_num,ml->nbin,ml->channel);
}

void wtk_ml_delete(wtk_ml_t *ml)
{
    wtk_complex_delete_p3(ml->ovec, ml->ang_num,ml->nbin);
    wtk_free(ml);
}

void wtk_ml_start(wtk_ml_t *ml, float **mic_pos, float sv, int rate, float theta, float phi, int ang_idx)
{
    wtk_complex_t **ovec=ml->ovec[ang_idx];
	float x,y,z;
	float t,f;
	float *mic;
	int i,j;
	int channel=ml->channel;
	float *tdoa;
	int nbin=ml->nbin;
	int win=(nbin-1)*2;
	wtk_complex_t *ovec1;

    phi*=PI/180;
    theta*=PI/180;
    x=cos(phi)*cos(theta);
    y=cos(phi)*sin(theta);
    z=sin(phi);

	tdoa=(float *)wtk_malloc(channel*sizeof(float));
	for(i=0;i<channel;++i)
	{
		mic=mic_pos[i];
		tdoa[i]=(mic[0]*x+mic[1]*y+mic[2]*z)/sv;
	}

    f=1.0/sqrt(channel);
	for(i=1;i<nbin-1;++i)
	{
		ovec1=ovec[i];
		t=2*PI*rate*1.0/win*i;
		for(j=0;j<channel;++j)
		{
			ovec1[j].a=cos(t*tdoa[j])*f;
			ovec1[j].b=sin(t*tdoa[j])*f;
		}
	}
	wtk_free(tdoa);
}

void wtk_ml_start2(wtk_ml_t *ml, float **mic_pos, float sv, int rate, float x, float y, float z, int ang_idx)
{
    wtk_complex_t **ovec=ml->ovec[ang_idx];
	float t,f;
	float *mic;
	int i,j;
	int channel=ml->channel;
	float *tdoa;
	int nbin=ml->nbin;
	int win=(nbin-1)*2;
	wtk_complex_t *ovec1;

	tdoa=(float *)wtk_malloc(channel*sizeof(float));

    for(i=0;i<channel;++i)
    {
        mic=mic_pos[i];
        tdoa[i]=sqrt((mic[0]-x)*(mic[0]-x)+(mic[1]-y)*(mic[1]-y)+(mic[2]-z)*(mic[2]-z))/sv;
        if(i>0)
        {
            tdoa[i]-=tdoa[0];
        }
    }
    tdoa[0]=0;

    f=1.0/sqrt(channel);
	for(i=1;i<nbin-1;++i)
	{
		ovec1=ovec[i];
		t=2*PI*rate*1.0/win*i;
		for(j=0;j<channel;++j)
		{
			ovec1[j].a=cos(t*tdoa[j])*f;
			ovec1[j].b=-sin(t*tdoa[j])*f;
		}
	}
	wtk_free(tdoa);
}

float wtk_ml_flush_spec_k(wtk_ml_t *ml, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx)
{
    float spec;
    float fa,fb;
    int channel=ml->channel;
    int i;
    wtk_complex_t *ovec=ml->ovec[ang_idx][k];

    fa=fb=0;
    for(i=0; i<channel; ++i)
    {
        fa+=fft[i][k].a*ovec->a+fft[i][k].b*ovec->b;
        fb+=-fft[i][k].a*ovec->b+fft[i][k].b*ovec->a;

        ++ovec;
    }
    spec=sqrt((fa*fa+fb*fb)/fftabs2);
    spec=-log((1-spec));

    return spec;
}

float wtk_ml_flush_spec_k2(wtk_ml_t *ml, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx)
{
    float spec;
    float fa,fb;
    int channel=ml->channel;
    int i;
    wtk_complex_t *ovec=ml->ovec[ang_idx][k];
    wtk_complex_t *fft2=fft[k];

    fa=fb=0;
    for(i=0; i<channel; ++i)
    {
        fa+=fft2->a*ovec->a+fft2->b*ovec->b;
        fb+=-fft2->a*ovec->b+fft2->b*ovec->a;

        ++ovec;
        ++fft2;
    }
    spec=sqrt((fa*fa+fb*fb)/fftabs2);
    spec=-log((1-spec));

    return spec;
}

