#include "wtk_gccspec2.h"


wtk_gccspec2_t *wtk_gccspec2_new(int nbin,int channel,int npairs,int ang_num)
{
    wtk_gccspec2_t *gccspec2;

    gccspec2=(wtk_gccspec2_t *)wtk_malloc(sizeof(wtk_gccspec2_t));
    gccspec2->nbin=nbin;
    gccspec2->channel=channel;
    gccspec2->npairs=npairs;
    gccspec2->ang_num=ang_num;

    gccspec2->alpha=wtk_complex_new_p3(ang_num,nbin,gccspec2->npairs);

    return gccspec2;
}


void wtk_gccspec2_reset(wtk_gccspec2_t *gccspec2)
{
    wtk_complex_zero_p3(gccspec2->alpha, gccspec2->ang_num, gccspec2->nbin, gccspec2->npairs);
}

void wtk_gccspec2_delete(wtk_gccspec2_t *gccspec2)
{
    wtk_complex_delete_p3(gccspec2->alpha, gccspec2->ang_num, gccspec2->nbin);
    wtk_free(gccspec2);
}

void wtk_gccspec2_start(wtk_gccspec2_t *gccspec2,  float **mic_pos, float sv, int rate, float theta, float phi ,float **pairs_m, int ang_idx)
{
    int npairs=gccspec2->npairs;
    int i,j,k,n;
    int nbin=gccspec2->nbin;
    float td,f;
    float x,y,z;
    wtk_complex_t **alpha=gccspec2->alpha[ang_idx];
    int win=(nbin-1)*2;

    phi*=PI/180;
    theta*=PI/180;
    x=cos(phi)*cos(theta);
    y=cos(phi)*sin(theta);
    z=sin(phi);

    for(n=0; n<npairs; ++n)
    {
        i=pairs_m[n][0];
        j=pairs_m[n][1];

        td=(mic_pos[i][0]*x+mic_pos[i][1]*y+mic_pos[i][2]*z)-(mic_pos[j][0]*x+mic_pos[j][1]*y+mic_pos[j][2]*z);

        for(k=1;k<nbin-1;++k)
        {
            f=-2*rate*1.0/win*k*PI*td/sv;
            alpha[k][n].a=cos(f);
            alpha[k][n].b=sin(f);
        }
    }
}

void wtk_gccspec2_start2(wtk_gccspec2_t *gccspec2, float **mic_pos, float sv, int rate, float x, float y, float z, float **pairs_m,int ang_idx)
{
    int npairs=gccspec2->npairs;
    int i,j,k,n;
    int nbin=gccspec2->nbin;
    float td,f;
    wtk_complex_t **alpha=gccspec2->alpha[ang_idx];
    int win=(nbin-1)*2;

    for(n=0; n<npairs; ++n)
    {
        i=pairs_m[n][0];
        j=pairs_m[n][1];

        td=sqrt((mic_pos[i][0]-x)*(mic_pos[i][0]-x)+(mic_pos[i][1]-y)*(mic_pos[i][1]-y)+(mic_pos[i][2]-z)*(mic_pos[i][2]-z))-
                    sqrt((mic_pos[j][0]-x)*(mic_pos[j][0]-x)+(mic_pos[j][1]-y)*(mic_pos[j][1]-y)+(mic_pos[j][2]-z)*(mic_pos[j][2]-z));

        for(k=1;k<nbin-1;++k)
        {
            f=-2*rate*1.0/win*k*PI*td/sv;
            alpha[k][n].a=cos(f);
            alpha[k][n].b=sin(f);
        }
    }
}

float wtk_gccspec2_flush_spec_k(wtk_gccspec2_t *gccspec2, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx)
{
    wtk_complex_t *alpha=gccspec2->alpha[ang_idx][k];
    int i,j,n;
    float spec;
    int npairs=gccspec2->npairs;
    wtk_complex_t xx;
    float f;

    spec=0;
    for(n=0; n<npairs; ++n)
    {
        i=pairs_m[n][0];
        j=pairs_m[n][1];

        xx.a = fft[i][k].a*fft[j][k].a+fft[i][k].b*fft[j][k].b;
        xx.b = -fft[i][k].a*fft[j][k].b+fft[i][k].b*fft[j][k].a;
        f = sqrt(xx.a*xx.a+xx.b*xx.b);

        spec+=(xx.a*alpha[n].a - xx.b*alpha[n].b)/f;
    }

    return spec;
}


float wtk_gccspec2_flush_spec_k2(wtk_gccspec2_t *gccspec2, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx)
{
    wtk_complex_t *alpha=gccspec2->alpha[ang_idx][k];
    int i,j,n;
    float spec;
    int npairs=gccspec2->npairs;
    wtk_complex_t xx;
    float f;
    wtk_complex_t *fft2=fft[k], *fft3, *fft4;

    spec=0;
    for(n=0; n<npairs; ++n)
    {
        i=pairs_m[n][0];
        j=pairs_m[n][1];

        fft3=fft2+i;
        fft4=fft2+j;

        xx.a = fft3->a*fft4->a+fft3->b*fft4->b;
        xx.b = -fft3->a*fft4->b+fft3->b*fft4->a;
        f = sqrt(xx.a*xx.a+xx.b*xx.b);

        spec+=(xx.a*alpha[n].a - xx.b*alpha[n].b)/f;
    }

    return spec;
}