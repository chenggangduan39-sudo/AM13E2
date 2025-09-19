#include "wtk_mvdrwspec.h"

wtk_mvdrwspec_t *wtk_mvdrwspec_new(int nbin,int channel, int ang_num)
{
    wtk_mvdrwspec_t *mvdrwspec;

    mvdrwspec=(wtk_mvdrwspec_t *)wtk_malloc(sizeof(wtk_mvdrwspec_t));
    mvdrwspec->nbin=nbin;
    mvdrwspec->channel=channel;
    mvdrwspec->ang_num=ang_num;
    mvdrwspec->ovec=wtk_complex_new_p3(ang_num,nbin,channel);
    mvdrwspec->B=wtk_float_new_p2(ang_num,nbin);
    mvdrwspec->tmp=(wtk_complex_t *)wtk_malloc(channel*sizeof(wtk_complex_t));

    return mvdrwspec;
}

void wtk_mvdrwspec_reset(wtk_mvdrwspec_t *mvdrwspec)
{
    wtk_complex_zero_p3(mvdrwspec->ovec,mvdrwspec->ang_num,mvdrwspec->nbin,mvdrwspec->channel);
}

void wtk_mvdrwspec_delete(wtk_mvdrwspec_t *mvdrwspec)
{
    wtk_complex_delete_p3(mvdrwspec->ovec,mvdrwspec->ang_num,mvdrwspec->nbin);
    wtk_float_delete_p2(mvdrwspec->B,mvdrwspec->ang_num);
    wtk_free(mvdrwspec->tmp);
    wtk_free(mvdrwspec);
}

void wtk_mvdrwspec_start(wtk_mvdrwspec_t *mvdrwspec, float **mic_pos, float sv, int rate, float theta, float phi, int ang_idx)
{
    wtk_complex_t *LL, *LL_inv, *tmp;
    wtk_dcomplex_t *tmp_inv;
    int channel=mvdrwspec->channel;
    float *B=mvdrwspec->B[ang_idx];
    wtk_complex_t **ovec=mvdrwspec->ovec[ang_idx], *alpha;
    int nbin=mvdrwspec->nbin;
    int k, i, j;
    int win=2*(nbin-1);
    float f, f1, f2, f3, d, ff;
    int ret;
    float x,y,z;
	float t;
	float *mic;
	float *tdoa;

    LL=(wtk_complex_t *)wtk_malloc(channel*channel*sizeof(wtk_complex_t));
    LL_inv=(wtk_complex_t *)wtk_malloc(channel*channel*sizeof(wtk_complex_t));
    tmp_inv=(wtk_dcomplex_t *)wtk_malloc(channel*channel*2*sizeof(wtk_dcomplex_t));
    tmp=(wtk_complex_t *)wtk_malloc(channel*sizeof(wtk_complex_t));
    tdoa=(float *)wtk_malloc(channel*sizeof(float));

    phi*=PI/180;
    theta*=PI/180;
    x=cos(phi)*cos(theta);
    y=cos(phi)*sin(theta);
    z=sin(phi);

	for(i=0;i<channel;++i)
	{
		mic=mic_pos[i];
		tdoa[i]=(mic[0]*x+mic[1]*y+mic[2]*z)/sv;
	}
    for(k=1;k<nbin-1;++k)
    {
		alpha=ovec[k];
		t=2*PI*rate*1.0/win*k;
		for(i=0;i<channel;++i)
		{
			alpha[i].a=cos(t*tdoa[i]);
			alpha[i].b=sin(t*tdoa[i]);
		}

        f=rate*1.0/win*k;
        for(i=0;i<channel;++i)
        {
            LL[i*channel+i].a=1;
            LL[i*channel+i].b=0;
            for(j=i+1;j<channel;++j)
            {
                f1=mic_pos[i][0]-mic_pos[j][0];
                f2=mic_pos[i][1]-mic_pos[j][1];
                f3=mic_pos[i][2]-mic_pos[j][2];
                d=sqrt(f1*f1+f2*f2+f3*f3);
                ff=2*PI*(d/sv)*f;
                LL[i*channel+j].a=sin(ff)/ff;
                LL[j*channel+i].a=LL[i*channel+j].a;
                LL[i*channel+j].b=LL[j*channel+i].b=0;
            }
        }
        ret=wtk_complex_invx4(LL,tmp_inv,channel,LL_inv,1);
        if(ret!=0)
		{
            j=0;
            for(i=0;i<channel;++i)
            {
                LL[j].a+=0.01;
                j+=channel+1;
            }
            wtk_complex_invx4(LL,tmp_inv,channel,LL_inv,1);
		}
        memset(tmp,0,channel*sizeof(wtk_complex_t));
        for(i=0;i<channel;++i)
        {
            for(j=0;j<channel;++j)
            {
                tmp[i].a+=LL_inv[i*channel+j].a*alpha[j].a;
                tmp[i].b+=LL_inv[i*channel+j].a*alpha[j].b;
            }
        }

        B[k]=0;
        for(i=0;i<channel;++i)
        {
            B[k]+=tmp[i].a*alpha[i].a + tmp[i].b*alpha[i].b;
        }
    }

    wtk_free(LL);
    wtk_free(LL_inv);
    wtk_free(tmp_inv);
    wtk_free(tmp);
}

void wtk_mvdrwspec_start2(wtk_mvdrwspec_t *mvdrwspec, float **mic_pos, float sv, int rate, float x, float y, float z, int ang_idx)
{
    wtk_complex_t *LL, *LL_inv, *tmp;
    wtk_dcomplex_t *tmp_inv;
    int channel=mvdrwspec->channel;
    float *B=mvdrwspec->B[ang_idx];
    wtk_complex_t **ovec=mvdrwspec->ovec[ang_idx], *alpha;
    int nbin=mvdrwspec->nbin;
    int k, i, j;
    int win=2*(nbin-1);
    float f, f1, f2, f3, d, ff;
    int ret;
	float t;
	float *mic;
	float *tdoa;

    LL=(wtk_complex_t *)wtk_malloc(channel*channel*sizeof(wtk_complex_t));
    LL_inv=(wtk_complex_t *)wtk_malloc(channel*channel*sizeof(wtk_complex_t));
    tmp_inv=(wtk_dcomplex_t *)wtk_malloc(channel*channel*2*sizeof(wtk_dcomplex_t));
    tmp=(wtk_complex_t *)wtk_malloc(channel*sizeof(wtk_complex_t));
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

    for(k=1;k<nbin-1;++k)
    {
		alpha=ovec[k];
		t=2*PI*rate*1.0/win*k;
		for(i=0;i<channel;++i)
		{
			alpha[i].a=cos(t*tdoa[i]);
			alpha[i].b=-sin(t*tdoa[i]);
		}

        f=rate*1.0/win*k;
        for(i=0;i<channel;++i)
        {
            LL[i*channel+i].a=1;
            LL[i*channel+i].b=0;
            for(j=i+1;j<channel;++j)
            {
                f1=mic_pos[i][0]-mic_pos[j][0];
                f2=mic_pos[i][1]-mic_pos[j][1];
                f3=mic_pos[i][2]-mic_pos[j][2];
                d=sqrt(f1*f1+f2*f2+f3*f3);
                ff=2*PI*(d/sv)*f;
                LL[i*channel+j].a=sin(ff)/ff;
                LL[j*channel+i].a=LL[i*channel+j].a;
                LL[i*channel+j].b=LL[j*channel+i].b=0;
            }
        }
        ret=wtk_complex_invx4(LL,tmp_inv,channel,LL_inv,1);
        if(ret!=0)
		{
            j=0;
            for(i=0;i<channel;++i)
            {
                LL[j].a+=0.01;
                j+=channel+1;
            }
            wtk_complex_invx4(LL,tmp_inv,channel,LL_inv,1);
		}
        memset(tmp,0,channel*sizeof(wtk_complex_t));
        for(i=0;i<channel;++i)
        {
            for(j=0;j<channel;++j)
            {
                tmp[i].a+=LL_inv[i*channel+j].a*alpha[j].a;
                tmp[i].b+=LL_inv[i*channel+j].a*alpha[j].b;
            }
        }

        B[k]=0;
        for(i=0;i<channel;++i)
        {
            B[k]+=tmp[i].a*alpha[i].a + tmp[i].b*alpha[i].b;
        }
    }

    wtk_free(LL);
    wtk_free(LL_inv);
    wtk_free(tmp_inv);
    wtk_free(tmp);
}

float wtk_mvdrwspec_flush_spec_k(wtk_mvdrwspec_t *mvdrwspec, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx)
{
    wtk_complex_t *a,*b;
    float fa,fb,f;
    int channel=mvdrwspec->channel;
    wtk_complex_t *tmp=mvdrwspec->tmp;
    wtk_complex_t *ovec=mvdrwspec->ovec[ang_idx][k];
    float B=mvdrwspec->B[ang_idx][k];
    int i,j;
    float spec;

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
    // wtk_debug("%f %f %f %f\n",tr,f,fa,fb);
    if(cov_travg-f<1e-6)
    {
        f=f/1e-6;
    }else
    {
        f=f/(cov_travg-f);
    }
    spec=(f*(B-1)-1)/B;

    return spec;
}
