#include "wtk_maskdsspec.h"

wtk_maskdsspec_t *wtk_maskdsspec_new(int nbin,int channel, int ang_num)
{
    wtk_maskdsspec_t *maskdsspec;

    maskdsspec=(wtk_maskdsspec_t *)wtk_malloc(sizeof(wtk_maskdsspec_t));
    maskdsspec->nbin=nbin;
    maskdsspec->channel=channel;
    maskdsspec->ang_num=ang_num;
    maskdsspec->ovec=wtk_complex_new_p3(ang_num,nbin,channel);

    return maskdsspec;
}

void wtk_maskdsspec_reset(wtk_maskdsspec_t *maskdsspec)
{
    wtk_complex_zero_p3(maskdsspec->ovec,maskdsspec->ang_num,maskdsspec->nbin,maskdsspec->channel);
}

void wtk_maskdsspec_delete(wtk_maskdsspec_t *maskdsspec)
{
    wtk_complex_delete_p3(maskdsspec->ovec,maskdsspec->ang_num,maskdsspec->nbin);
    wtk_free(maskdsspec);
}

void wtk_maskdsspec_start(wtk_maskdsspec_t *maskdsspec, float **mic_pos, float sv, int rate, float theta, float phi, int ang_idx)
{
    wtk_complex_t **ovec=maskdsspec->ovec[ang_idx];
	float x,y,z;
	float t;
	float *mic;
	int i,j;
	int channel=maskdsspec->channel;
	float *tdoa;
	int nbin=maskdsspec->nbin;
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


float wtk_maskdsspec_flush_spec_k(wtk_maskdsspec_t *maskdsspec, wtk_complex_t *scov,wtk_complex_t *ncov, float prob, int k, int ang_idx)
{
    wtk_complex_t *b,*c;
    float fa,fb,fa2,fb2,f1,f2,f;
    int channel=maskdsspec->channel;
    wtk_complex_t *ovec=maskdsspec->ovec[ang_idx][k];
    int i,j;

    f1=f2=0;
    for(i=0;i<channel;++i)
    {
        fa=fb=0;
        fa2=fb2=0;
        for(j=0;j<channel;++j)
        {
            b=ovec+j;
            fa+=scov->a*b->a-scov->b*b->b;
            fb+=scov->a*b->b+scov->b*b->a;
            ++scov;

            fa2+=ncov->a*b->a-ncov->b*b->b;
            fb2+=ncov->a*b->b+ncov->b*b->a;
            ++ncov;
        }

        c=ovec+i;
        f1+=fa*c->a+fb*c->b;
        f2+=fa2*c->a+fb2*c->b;
    }

    f=prob * f1/(f1+f2);

    return f;
}

void wtk_maskdsspec_flush_spec_k2(wtk_maskdsspec_t *maskdsspec, wtk_complex_t *scov,wtk_complex_t *ncov, float prob, int k, int ang_idx, float *spec)
{
    wtk_complex_t *b,*c;
    float fa,fb,fa2,fb2,f1,f2,f;
    int channel=maskdsspec->channel;
    wtk_complex_t *ovec;
    int i,j,n;
    wtk_complex_t *scov1, *ncov1;

    for(n=0;n<ang_idx;++n,++spec){
        ovec=maskdsspec->ovec[n][k];
        f1=f2=0;
        scov1=scov;
        ncov1=ncov;
        for(i=0;i<channel;++i)
        {
            fa=fb=0;
            fa2=fb2=0;
            for(j=0;j<channel;++j)
            {
                b=ovec+j;
                fa+=scov1->a*b->a-scov1->b*b->b;
                fb+=scov1->a*b->b+scov1->b*b->a;
                ++scov1;

                fa2+=ncov1->a*b->a-ncov1->b*b->b;
                fb2+=ncov1->a*b->b+ncov1->b*b->a;
                ++ncov1;
            }

            c=ovec+i;
            f1+=fa*c->a+fb*c->b;
            f2+=fa2*c->a+fb2*c->b;
        }

        f=prob * f1/(f1+f2);
        *spec=f;
    }
}
