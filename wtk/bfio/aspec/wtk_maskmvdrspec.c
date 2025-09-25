#include "wtk_maskmvdrspec.h"

wtk_maskmvdrspec_t *wtk_maskmvdrspec_new(int nbin,int channel, int ang_num)
{
    wtk_maskmvdrspec_t *maskmvdrspec;

    maskmvdrspec=(wtk_maskmvdrspec_t *)wtk_malloc(sizeof(wtk_maskmvdrspec_t));
    maskmvdrspec->nbin=nbin;
    maskmvdrspec->channel=channel;
    maskmvdrspec->ang_num=ang_num;
    maskmvdrspec->ovec=wtk_complex_new_p3(ang_num,nbin,channel);

    maskmvdrspec->tmp=(wtk_complex_t *)wtk_malloc(channel*sizeof(wtk_complex_t));

    return maskmvdrspec;
}

void wtk_maskmvdrspec_reset(wtk_maskmvdrspec_t *maskmvdrspec)
{
    wtk_complex_zero_p3(maskmvdrspec->ovec,maskmvdrspec->ang_num,maskmvdrspec->nbin,maskmvdrspec->channel);
}

void wtk_maskmvdrspec_delete(wtk_maskmvdrspec_t *maskmvdrspec)
{
    wtk_complex_delete_p3(maskmvdrspec->ovec,maskmvdrspec->ang_num,maskmvdrspec->nbin);
    wtk_free(maskmvdrspec->tmp);
    wtk_free(maskmvdrspec);
}

void wtk_maskmvdrspec_start(wtk_maskmvdrspec_t *maskmvdrspec, float **mic_pos, float sv, int rate, float theta, float phi, int ang_idx)
{
    wtk_complex_t **ovec=maskmvdrspec->ovec[ang_idx];
	float x,y,z;
	float t;
	float *mic;
	int i,j;
	int channel=maskmvdrspec->channel;
	float *tdoa;
	int nbin=maskmvdrspec->nbin;
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


float wtk_maskmvdrspec_flush_spec_k(wtk_maskmvdrspec_t *maskmvdrspec, wtk_complex_t *scov,wtk_complex_t *inv_cov, float prob, int k, int ang_idx)
{
    wtk_complex_t *a,*b,*c;
    float fa,fb,f1,f2,f;
    int channel=maskmvdrspec->channel;
    wtk_complex_t *tmp=maskmvdrspec->tmp;
    wtk_complex_t *ovec=maskmvdrspec->ovec[ang_idx][k];
    int i,j;

    // wtk_complex_print2(inv_cov, channel,channel);
    // wtk_complex_print2(scov, channel,channel);
// wtk_complex_print(ovec, channel);
    a=inv_cov;
    f=0;
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

        c=ovec+i;
        f+=fa*c->a+fb*c->b;
    }
    // printf("%f\n",f);

    f2=1.0/f;
    for(i=0;i<channel;++i)
    {
        tmp[i].a*=f2;
        tmp[i].b*=f2;
    }

    a=scov;
    f1=0;
    for(i=0;i<channel;++i)
    {
        fa=fb=0;
        for(j=0;j<channel;++j)
        {
            b=tmp+j;
            fa+=a->a*b->a-a->b*b->b;
            fb+=a->a*b->b+a->b*b->a;
            ++a;
        }

        c=tmp+i;
        f1+=fa*c->a+fb*c->b;
    }
    // wtk_complex_print2(scov,channel,channel);
    // wtk_complex_print(tmp,channel);
    // printf("%f %f %f\n",prob,f1,f2);
    f=prob * f1/(f1+f2);

    return f;
}

void wtk_maskmvdrspec_flush_spec_k2(wtk_maskmvdrspec_t *maskmvdrspec, wtk_complex_t *scov,wtk_complex_t *inv_cov, float prob, int k, int ang_idx, float *spec)
{
    wtk_complex_t *a,*b,*c;
    float fa,fb,f1,f2,f;
    int channel=maskmvdrspec->channel;
    wtk_complex_t *tmp=maskmvdrspec->tmp;
    wtk_complex_t *ovec;
    int i,j,n;

    // wtk_complex_print2(inv_cov, channel,channel);
    // wtk_complex_print2(scov, channel,channel);
// wtk_complex_print(ovec, channel);
    for(n=0;n<ang_idx;++n,++spec){
        ovec=maskmvdrspec->ovec[n][k];
        a=inv_cov;
        f=0;
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

            c=ovec+i;
            f+=fa*c->a+fb*c->b;
        }
        // printf("%f\n",f);

        f2=1.0/f;
        for(i=0;i<channel;++i)
        {
            tmp[i].a*=f2;
            tmp[i].b*=f2;
        }

        a=scov;
        f1=0;
        for(i=0;i<channel;++i)
        {
            fa=fb=0;
            for(j=0;j<channel;++j)
            {
                b=tmp+j;
                fa+=a->a*b->a-a->b*b->b;
                fb+=a->a*b->b+a->b*b->a;
                ++a;
            }

            c=tmp+i;
            f1+=fa*c->a+fb*c->b;
        }
        // wtk_complex_print2(scov,channel,channel);
        // wtk_complex_print(tmp,channel);
        // printf("%f %f %f\n",prob,f1,f2);
        f=prob * f1/(f1+f2);
        *spec=f;
    }
}
