#include "wtk_maskgccspec.h"

wtk_maskgccspec_t *wtk_maskgccspec_new(int nbin,int channel, int ang_num)
{
    wtk_maskgccspec_t *maskgccspec;

    maskgccspec=(wtk_maskgccspec_t *)wtk_malloc(sizeof(wtk_maskgccspec_t));
    maskgccspec->nbin=nbin;
    maskgccspec->channel=channel;
    maskgccspec->ang_num=ang_num;
    maskgccspec->ovec=wtk_complex_new_p3(ang_num,nbin,channel);

    return maskgccspec;
}

void wtk_maskgccspec_reset(wtk_maskgccspec_t *maskgccspec)
{
    wtk_complex_zero_p3(maskgccspec->ovec,maskgccspec->ang_num,maskgccspec->nbin,maskgccspec->channel);
}

void wtk_maskgccspec_delete(wtk_maskgccspec_t *maskgccspec)
{
    wtk_complex_delete_p3(maskgccspec->ovec,maskgccspec->ang_num,maskgccspec->nbin);
    wtk_free(maskgccspec);
}

void wtk_maskgccspec_start(wtk_maskgccspec_t *maskgccspec, float **mic_pos, float sv, int rate, float theta, float phi, int ang_idx)
{
    wtk_complex_t **ovec=maskgccspec->ovec[ang_idx];
	float x,y,z;
	float t;
	float *mic;
	int i,j;
	int channel=maskgccspec->channel;
	float *tdoa;
	int nbin=maskgccspec->nbin;
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

    x=1.0/sqrt(channel);
	for(i=0;i<nbin;++i)
	{
		ovec1=ovec[i];
		t=2*PI*rate*1.0/win*i;
		for(j=0;j<channel;++j)
		{
			ovec1[j].a=cos(t*tdoa[j])*x;
			ovec1[j].b=sin(t*tdoa[j])*x;
		}
	}
	wtk_free(tdoa);
}


float wtk_maskgccspec_flush_spec_k(wtk_maskgccspec_t *maskgccspec, wtk_complex_t *fft, float prob, int k, int ang_idx)
{
    float spec;
    float fa,fb;
    int channel=maskgccspec->channel;
    int i;
    wtk_complex_t *ovec=maskgccspec->ovec[ang_idx][k];
    float fftabs;

    fa=fb=0;
    fftabs=0;
    for(i=0; i<channel; ++i)
    {
        fftabs+=fft->a*fft->a+fft->b*fft->b;

        fa+=fft->a*ovec->a+fft->b*ovec->b;
        fb+=-fft->a*ovec->b+fft->b*ovec->a;

        ++ovec;
        ++fft;
    }
    spec=prob*sqrt((fa*fa+fb*fb)/fftabs);

    return spec;
}

void wtk_maskgccspec_flush_spec_k2(wtk_maskgccspec_t *maskgccspec, wtk_complex_t *fft, float prob, int k, int ang_idx, float *spec)
{
    float fa,fb;
    int channel=maskgccspec->channel;
    int i, n;
    wtk_complex_t *ovec;
    float fftabs;
    wtk_complex_t *fft1;

    for(n=0;n<ang_idx;++n,++spec){
        ovec=maskgccspec->ovec[n][k];
        fa=fb=0;
        fftabs=0;
        fft1=fft;
        for(i=0; i<channel; ++i)
        {
            fftabs+=fft1->a*fft1->a+fft1->b*fft1->b;

            fa+=fft1->a*ovec->a+fft1->b*ovec->b;
            fb+=-fft1->a*ovec->b+fft1->b*ovec->a;

            ++ovec;
            ++fft1;
        }
        *spec=prob*sqrt((fa*fa+fb*fb)/fftabs);
    }
}
