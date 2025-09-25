#include "wtk_dsspec2.h"


wtk_dsspec2_t *wtk_dsspec2_new(int nbin,int channel,int npairs, int ang_num)
{
    wtk_dsspec2_t *dsspec2;

    dsspec2=(wtk_dsspec2_t *)wtk_malloc(sizeof(wtk_dsspec2_t));
    dsspec2->nbin=nbin;
    dsspec2->channel=channel;
    dsspec2->npairs=npairs;
    dsspec2->ang_num=ang_num;

    dsspec2->alpha=wtk_complex_new_p3(ang_num,nbin, npairs);

    return dsspec2;
}

void wtk_dsspec2_reset(wtk_dsspec2_t *dsspec2)
{
    wtk_complex_zero_p3(dsspec2->alpha, dsspec2->ang_num, dsspec2->nbin, dsspec2->npairs);
}

void wtk_dsspec2_delete(wtk_dsspec2_t *dsspec2)
{
    wtk_complex_delete_p3(dsspec2->alpha, dsspec2->ang_num, dsspec2->nbin);
    wtk_free(dsspec2);
}

void wtk_dsspec2_start(wtk_dsspec2_t *dsspec2, float **mic_pos, float sv, int rate, float theta, float phi ,float **pairs_m, int ang_idx)
{
    int npairs=dsspec2->npairs;
    int i,j,k,n;
    int nbin=dsspec2->nbin;
    // int channel=dsspec2->channel;
    float td,f;
    float x,y,z;
    wtk_complex_t **alpha=dsspec2->alpha[ang_idx];
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

void wtk_dsspec2_start2(wtk_dsspec2_t *dsspec2,  float **mic_pos, float sv, int rate, float x, float y, float z, float **pairs_m, int ang_idx)
{
    int npairs=dsspec2->npairs;
    int i,j,k,n;
    int nbin=dsspec2->nbin;
    // int channel=dsspec2->channel;
    float td,f;
    wtk_complex_t **alpha=dsspec2->alpha[ang_idx];
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

float wtk_dsspec2_flush_spec_k(wtk_dsspec2_t *dsspec2, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx)
{
    int channel=dsspec2->channel;
    wtk_complex_t *alpha=dsspec2->alpha[ang_idx][k];
    int i,j,n;
    float tr,spec;
    wtk_complex_t r11,r22,r12;
    int npairs=dsspec2->npairs;
    float tmp;

    spec=0;
    for(n=0; n<npairs; ++n)
    {
        i=pairs_m[n][0];
        j=pairs_m[n][1];

        r11=cov[i*channel+i];
        r22=cov[j*channel+j];
        r12=cov[i*channel+j];
        tr=r11.a+r22.a;
        tmp=r12.a*alpha[n].a - r12.b*alpha[n].b;
        spec+=(tr+2*tmp)/(tr-2*tmp);
    }

    return spec;
}