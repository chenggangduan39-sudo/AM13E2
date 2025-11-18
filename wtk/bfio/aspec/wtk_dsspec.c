#include "wtk_dsspec.h"

wtk_dsspec_t *wtk_dsspec_new(int nbin,int channel, int ang_num)
{
    wtk_dsspec_t *dsspec;

    dsspec=(wtk_dsspec_t *)wtk_malloc(sizeof(wtk_dsspec_t));
    dsspec->nbin=nbin;
    dsspec->channel=channel;
    dsspec->ang_num=ang_num;
    dsspec->ovec=wtk_complex_new_p3(ang_num,nbin,channel);
    dsspec->tmp=(wtk_complex_t *)wtk_malloc(channel*sizeof(wtk_complex_t));

    return dsspec;
}

void wtk_dsspec_reset(wtk_dsspec_t *dsspec)
{
    wtk_complex_zero_p3(dsspec->ovec,dsspec->ang_num,dsspec->nbin,dsspec->channel);
}

void wtk_dsspec_delete(wtk_dsspec_t *dsspec)
{
    wtk_complex_delete_p3(dsspec->ovec,dsspec->ang_num,dsspec->nbin);
    wtk_free(dsspec->tmp);
    wtk_free(dsspec);
}

void wtk_dsspec_start(wtk_dsspec_t *dsspec,  float **mic_pos, float sv, int rate, float theta, float phi, int ang_idx)
{
    wtk_complex_t **ovec=dsspec->ovec[ang_idx];
	float x,y,z;
	float t;
	float *mic;
	int i,j;
	int channel=dsspec->channel;
	float *tdoa;
	int nbin=dsspec->nbin;
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

	for(i=1;i<nbin-1;++i)
	{
		ovec1=ovec[i];
		t=2*PI*rate*1.0/win*i;
		for(j=0;j<channel;++j)
		{
			ovec1[j].a=cos(t*tdoa[j]);
			ovec1[j].b=sin(t*tdoa[j]);
		}
	}
	wtk_free(tdoa);
}

void wtk_dsspec_start2(wtk_dsspec_t *dsspec, float **mic_pos, float sv, int rate, float x, float y, float z, int ang_idx)
{
    wtk_complex_t **ovec=dsspec->ovec[ang_idx];
	float t;
	float *mic;
	int i,j;
	int channel=dsspec->channel;
	float *tdoa;
	int nbin=dsspec->nbin;
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

	for(i=1;i<nbin-1;++i)
	{
		ovec1=ovec[i];
		t=2*PI*rate*1.0/win*i;
		for(j=0;j<channel;++j)
		{
			ovec1[j].a=cos(t*tdoa[j]);
			ovec1[j].b=-sin(t*tdoa[j]);
		}
	}
	wtk_free(tdoa);
}

float wtk_dsspec_flush_spec_k(wtk_dsspec_t *dsspec, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx)
{
    wtk_complex_t *a,*b;
    float fa,fb,f;
    int channel=dsspec->channel;
    int channelx2=channel*channel;
    wtk_complex_t *ovec=dsspec->ovec[ang_idx][k];
    wtk_complex_t *tmp=dsspec->tmp;
    int i,j;

    a=cov;
    for(i=0;i<channel;++i)
    {
        fa=fb=0;
        for(j=0;j<channel;++j)
        {
            b=ovec+j;
            fa+=a->a*b->a-a->b*b->b;
            fb+=a->a*b->b+a->b*b->a;
            ++a;
        }
        tmp[i].a=fa;
        tmp[i].b=fb;
    }
    // fa=fb=0;
    fa=0;
    for(i=0;i<channel;++i)
    {
        a=ovec+i;
        b=tmp+i;
        fa+=a->a*b->a+a->b*b->b;
        // fb+=a->a*b->b-a->b*b->a;
    }
    // f=sqrt(fa*fa+fb*fb)/channelx2;
    f=fa/channelx2;
    if(cov_travg-f<1e-6)
    {
        f=f/1e-6;
    }else
    {
        f=f/(cov_travg-f);
    }
    
    return f;
}
