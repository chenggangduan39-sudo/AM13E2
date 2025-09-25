#include "wtk_musicspec2.h"


wtk_musicspec2_t *wtk_musicspec2_new(int nbin,int channel,int npairs, int ang_num)
{
    wtk_musicspec2_t *musicspec2;

    musicspec2=(wtk_musicspec2_t *)wtk_malloc(sizeof(wtk_musicspec2_t));
    musicspec2->nbin=nbin;
    musicspec2->channel=channel;
    musicspec2->npairs=npairs;
    musicspec2->ang_num=ang_num;

    musicspec2->alpha=wtk_complex_new_p3(ang_num,nbin, npairs);

    return musicspec2;
}

void wtk_musicspec2_reset(wtk_musicspec2_t *musicspec2)
{
    wtk_complex_zero_p3(musicspec2->alpha, musicspec2->ang_num, musicspec2->nbin, musicspec2->npairs);
}

void wtk_musicspec2_delete(wtk_musicspec2_t *musicspec2)
{
    wtk_complex_delete_p3(musicspec2->alpha, musicspec2->ang_num, musicspec2->nbin);
    wtk_free(musicspec2);
}

void wtk_musicspec2_start(wtk_musicspec2_t *musicspec2, float **mic_pos, float sv, int rate, float theta, float phi ,float **pairs_m, int ang_idx)
{
    int npairs=musicspec2->npairs;
    int i,j,k,n;
    int nbin=musicspec2->nbin;
    // int channel=musicspec2->channel;
    float td,f;
    float x,y,z;
    wtk_complex_t **alpha=musicspec2->alpha[ang_idx];
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

void wtk_musicspec2_start2(wtk_musicspec2_t *musicspec2, float **mic_pos, float sv, int rate, float x, float y, float z, float **pairs_m, int ang_idx)
{
    int npairs=musicspec2->npairs;
    int i,j,k,n;
    int nbin=musicspec2->nbin;
    // int channel=musicspec2->channel;
    float td,f;
    wtk_complex_t **alpha=musicspec2->alpha[ang_idx];
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

float wtk_musicspec2_flush_spec_k(wtk_musicspec2_t *musicspec2, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx)
{
    int channel=musicspec2->channel;
    wtk_complex_t *alpha=musicspec2->alpha[ang_idx][k];
    int i,j,n;
    float tr,det,lambda,spec;
    wtk_complex_t r11,r22,r12,V2,c;
    int npairs=musicspec2->npairs;
    float tmp, tmp2;

    spec=0;
    for(n=0; n<npairs; ++n)
    {
        i=pairs_m[n][0];
        j=pairs_m[n][1];

        r11=cov[i*channel+i];
        r22=cov[j*channel+j];
        r12=cov[i*channel+j];
        tr=r11.a+r22.a;

        tmp=r12.a*r12.a+r12.b*r12.b;
        det=r11.a*r22.a - tmp;
        lambda = 0.5*(tr + sqrt(tr*tr - 4*det));
        V2.a=(lambda-r11.a)*r12.a/tmp;
        V2.b=(r11.a-lambda)*r12.b/tmp;
        c.a=V2.a*alpha[n].a+V2.b*alpha[n].b+1;
        c.b=-V2.a*alpha[n].b+V2.b*alpha[n].a;
        tmp=c.a*c.a+c.b*c.b;
        tmp2=1+V2.a*V2.a+V2.b*V2.b;
        spec+=1/(1-0.5*tmp/tmp2+1e-10);
    }
    return spec;
}