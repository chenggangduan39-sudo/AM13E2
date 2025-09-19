#include "wtk_gccspec.h"

wtk_gccspec_t *wtk_gccspec_new(int nbin,int channel,int ang_num)
{
    wtk_gccspec_t *gccspec;

    gccspec=(wtk_gccspec_t *)wtk_malloc(sizeof(wtk_gccspec_t));
    gccspec->nbin=nbin;
    gccspec->channel=channel;
    gccspec->ang_num=ang_num;
    gccspec->ovec=wtk_complex_new_p3(ang_num,nbin,channel);
    
    return gccspec;
}

void wtk_gccspec_reset(wtk_gccspec_t *gccspec)
{
    wtk_complex_zero_p3(gccspec->ovec, gccspec->ang_num,gccspec->nbin,gccspec->channel);
}

void wtk_gccspec_delete(wtk_gccspec_t *gccspec)
{
    wtk_complex_delete_p3(gccspec->ovec, gccspec->ang_num,gccspec->nbin);
    wtk_free(gccspec);
}

void wtk_gccspec_start(wtk_gccspec_t *gccspec, float **mic_pos, float sv, int rate, float theta, float phi, int ang_idx)
{
    wtk_complex_t **ovec=gccspec->ovec[ang_idx];
	float x,y,z;
	float t,f;
	float *mic;
	int i,j;
	int channel=gccspec->channel;
	float *tdoa;
	int nbin=gccspec->nbin;
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


void wtk_gccspec_start2(wtk_gccspec_t *gccspec, float **mic_pos, float sv, int rate, float x, float y, float z, int ang_idx)
{
    wtk_complex_t **ovec=gccspec->ovec[ang_idx];
	float t,f;
	float *mic;
	int i,j;
	int channel=gccspec->channel;
	float *tdoa;
	int nbin=gccspec->nbin;
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

float wtk_gccspec_flush_spec_k(wtk_gccspec_t *gccspec, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx)
{
    float spec;
    float fa,fb;
    int channel=gccspec->channel;
    int i;
    wtk_complex_t *ovec=gccspec->ovec[ang_idx][k];

    fa=fb=0;
    for(i=0; i<channel; ++i)
    {
        fa+=fft[i][k].a*ovec->a+fft[i][k].b*ovec->b;
        fb+=-fft[i][k].a*ovec->b+fft[i][k].b*ovec->a;

        ++ovec;
    }
    spec=sqrt((fa*fa+fb*fb)/fftabs2);

    return spec;
}

float wtk_gccspec_flush_spec_k2(wtk_gccspec_t *gccspec, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx)
{
    float spec;
    float fa,fb;
    int channel=gccspec->channel;
    int i;
    wtk_complex_t *ovec=gccspec->ovec[ang_idx][k];

    fa=fb=0;
    for(i=0; i<channel; ++i)
    {
        fa+=fft[i][k].a*ovec->a+fft[i][k].b*ovec->b;
        fb+=-fft[i][k].a*ovec->b+fft[i][k].b*ovec->a;

        ++ovec;
    }
    spec=fa*fa+fb*fb;

    return spec;
}


float wtk_gccspec_flush_spec_k3(wtk_gccspec_t *gccspec, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx)
{
    float spec;
    float fa,fb;
    int channel=gccspec->channel;
    int i;
    wtk_complex_t *ovec=gccspec->ovec[ang_idx][k];
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

    return spec;
}


float wtk_gccspec_flush_spec_k4(wtk_gccspec_t *gccspec, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx)
{
    float spec;
    float fa,fb;
    int channel=gccspec->channel;
    int i;
    wtk_complex_t *ovec=gccspec->ovec[ang_idx][k];
    wtk_complex_t *fft2=fft[k];

    fa=fb=0;
    for(i=0; i<channel; ++i)
    {
        fa+=fft2->a*ovec->a+fft2->b*ovec->b;
        fb+=-fft2->a*ovec->b+fft2->b*ovec->a;

        ++ovec;
        ++fft2;
    }
    spec=fa*fa+fb*fb;

    return spec;
}

void wtk_gccspec_flush_spec_k_2(wtk_gccspec_t *gccspec, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx, float *spec)
{
    float fa,fb;
    int channel=gccspec->channel;
    int i, n;
    wtk_complex_t *ovec;

    for(n=0;n<ang_idx;++n,++spec){
        ovec=gccspec->ovec[n][k];
        fa=fb=0;
        for(i=0; i<channel; ++i)
        {
            fa+=fft[i][k].a*ovec->a+fft[i][k].b*ovec->b;
            fb+=-fft[i][k].a*ovec->b+fft[i][k].b*ovec->a;

            ++ovec;
        }
        *spec=sqrt((fa*fa+fb*fb)/fftabs2);
    }
}

void wtk_gccspec_flush_spec_k2_2(wtk_gccspec_t *gccspec, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx, float *spec)
{
    float fa,fb;
    int channel=gccspec->channel;
    int i,n;
    wtk_complex_t *ovec;

    for(n=0;n<ang_idx;++n,++spec){
        ovec=gccspec->ovec[n][k];
        fa=fb=0;
        for(i=0; i<channel; ++i)
        {
            fa+=fft[i][k].a*ovec->a+fft[i][k].b*ovec->b;
            fb+=-fft[i][k].a*ovec->b+fft[i][k].b*ovec->a;

            ++ovec;
        }
        *spec=fa*fa+fb*fb;
    }
}


void wtk_gccspec_flush_spec_k3_2(wtk_gccspec_t *gccspec, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx, float *spec)
{
    float fa,fb;
    int channel=gccspec->channel;
    int i,n;
    wtk_complex_t *ovec;
    wtk_complex_t *fft2;

    for(n=0;n<ang_idx;++n,++spec){
        ovec=gccspec->ovec[n][k];
        fft2=fft[k];
        fa=fb=0;
        for(i=0; i<channel; ++i)
        {
            fa+=fft2->a*ovec->a+fft2->b*ovec->b;
            fb+=-fft2->a*ovec->b+fft2->b*ovec->a;

            ++ovec;
            ++fft2;
        }
        *spec=sqrt((fa*fa+fb*fb)/fftabs2);
    }
}


void wtk_gccspec_flush_spec_k4_2(wtk_gccspec_t *gccspec, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx, float *spec)
{
    float fa,fb;
    int channel=gccspec->channel;
    int i,n;
    wtk_complex_t *ovec;
    wtk_complex_t *fft2;

    for(n=0;n<ang_idx;++n,++spec){
        ovec=gccspec->ovec[n][k];
        fft2=fft[k];
        fa=fb=0;
        for(i=0; i<channel; ++i)
        {
            fa+=fft2->a*ovec->a+fft2->b*ovec->b;
            fb+=-fft2->a*ovec->b+fft2->b*ovec->a;

            ++ovec;
            ++fft2;
        }
        *spec=fa*fa+fb*fb;
    }
}