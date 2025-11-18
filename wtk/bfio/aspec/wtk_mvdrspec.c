#include "wtk_mvdrspec.h"

wtk_mvdrspec_t *wtk_mvdrspec_new(int nbin,int channel, int ang_num)
{
    wtk_mvdrspec_t *mvdrspec;

    mvdrspec=(wtk_mvdrspec_t *)wtk_malloc(sizeof(wtk_mvdrspec_t));
    mvdrspec->nbin=nbin;
    mvdrspec->channel=channel;
    mvdrspec->ang_num=ang_num;
    mvdrspec->ovec=wtk_complex_new_p3(ang_num,nbin,channel);

    mvdrspec->tmp=(wtk_complex_t *)wtk_malloc(channel*sizeof(wtk_complex_t));

    return mvdrspec;
}

void wtk_mvdrspec_reset(wtk_mvdrspec_t *mvdrspec)
{
    wtk_complex_zero_p3(mvdrspec->ovec,mvdrspec->ang_num,mvdrspec->nbin,mvdrspec->channel);
}

void wtk_mvdrspec_delete(wtk_mvdrspec_t *mvdrspec)
{
    wtk_complex_delete_p3(mvdrspec->ovec,mvdrspec->ang_num,mvdrspec->nbin);
    wtk_free(mvdrspec->tmp);
    wtk_free(mvdrspec);
}

void wtk_mvdrspec_start(wtk_mvdrspec_t *mvdrspec, float **mic_pos, float sv, int rate, float theta, float phi, int ang_idx)
{
    wtk_complex_t **ovec=mvdrspec->ovec[ang_idx];
	float x,y,z;
	float t;
	float *mic;
	int i,j;
	int channel=mvdrspec->channel;
	float *tdoa;
	int nbin=mvdrspec->nbin;
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

void wtk_mvdrspec_start2(wtk_mvdrspec_t *mvdrspec,  float **mic_pos, float sv, int rate, float x, float y, float z, int ang_idx)
{
    wtk_complex_t **ovec=mvdrspec->ovec[ang_idx];
	float t;
	float *mic;
	int i,j;
	int channel=mvdrspec->channel;
	float *tdoa;
	int nbin=mvdrspec->nbin;
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

    x=1.0/sqrt(channel);
	for(i=1;i<nbin-1;++i)
	{
		ovec1=ovec[i];
		t=2*PI*rate*1.0/win*i;
		for(j=0;j<channel;++j)
		{
			ovec1[j].a=cos(t*tdoa[j])*x;
			ovec1[j].b=-sin(t*tdoa[j])*x;
		}
	}
	wtk_free(tdoa);
}

float wtk_mvdrspec_flush_spec_k(wtk_mvdrspec_t *mvdrspec, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx)
{
    wtk_complex_t *a,*b;
    float fa,fb,f;
    int channel=mvdrspec->channel;
    wtk_complex_t *tmp=mvdrspec->tmp;
    wtk_complex_t *ovec=mvdrspec->ovec[ang_idx][k];
    int i,j;

    a=inv_cov;
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
        //(a-bi)*(c+di)=(ac+bd)+i(ad-bc);
        fa+=a->a*b->a+a->b*b->b;
        // fb+=a->a*b->b-a->b*b->a;
    }
    // f=1.0/sqrt(fa*fa+fb*fb);
    f=1.0/fa;
    // wtk_debug("%f %f %f\n",f,fa,fb);
    if(cov_travg-f<1e-6)
    {
        f=f/1e-6;
    }else
    {
        f=f/(cov_travg-f);
    }

    return f;
}
