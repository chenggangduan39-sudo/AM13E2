#include "wtk_mvdrspec2.h"


wtk_mvdrspec2_t *wtk_mvdrspec2_new(int nbin,int channel,int npairs, int ang_num)
{
    wtk_mvdrspec2_t *mvdrspec2;

    mvdrspec2=(wtk_mvdrspec2_t *)wtk_malloc(sizeof(wtk_mvdrspec2_t));
    mvdrspec2->nbin=nbin;
    mvdrspec2->channel=channel;
    mvdrspec2->npairs=npairs;
    mvdrspec2->ang_num=ang_num;

    mvdrspec2->alpha=wtk_complex_new_p3(ang_num,nbin, npairs);

    return mvdrspec2;
}

void wtk_mvdrspec2_reset(wtk_mvdrspec2_t *mvdrspec2)
{
    wtk_complex_zero_p3(mvdrspec2->alpha, mvdrspec2->ang_num, mvdrspec2->nbin, mvdrspec2->npairs);
}

void wtk_mvdrspec2_delete(wtk_mvdrspec2_t *mvdrspec2)
{
    wtk_complex_delete_p3(mvdrspec2->alpha, mvdrspec2->ang_num, mvdrspec2->nbin);
    wtk_free(mvdrspec2);
}

void wtk_mvdrspec2_start(wtk_mvdrspec2_t *mvdrspec2, float **mic_pos, float sv, int rate, float theta, float phi ,float **pairs_m, int ang_idx)
{
    int npairs=mvdrspec2->npairs;
    int i,j,k,n;
    int nbin=mvdrspec2->nbin;
    float td,f;
    float x,y,z;
    wtk_complex_t **alpha=mvdrspec2->alpha[ang_idx];
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

void wtk_mvdrspec2_start2(wtk_mvdrspec2_t *mvdrspec2, float **mic_pos, float sv, int rate, float x, float y, float z, float **pairs_m, int ang_idx)
{
    int npairs=mvdrspec2->npairs;
    int i,j,k,n;
    int nbin=mvdrspec2->nbin;
    float td,f;
    wtk_complex_t **alpha=mvdrspec2->alpha[ang_idx];
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

float wtk_mvdrspec2_flush_spec_k(wtk_mvdrspec2_t *mvdrspec2, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx)
{
    int channel=mvdrspec2->channel;
    wtk_complex_t *alpha=mvdrspec2->alpha[ang_idx][k];
    int i,j,n;
    float num,tr,spec;
    wtk_complex_t r11,r22,r12,r21;
    int npairs=mvdrspec2->npairs;

    spec=0;
    for(n=0; n<npairs; ++n)
    {
        i=pairs_m[n][0];
        j=pairs_m[n][1];

        r11=cov[i*channel+i];
        r22=cov[j*channel+j];
        r12=cov[i*channel+j];
        r21=cov[j*channel+i];
        tr=r11.a+r22.a;
        num=(r11.a*r22.a - r12.a*r21.a + r12.b*r21.b) / (tr-2*(r12.a*alpha[n].a - r12.b*alpha[n].b));
        spec+=num/(0.5*tr-num);
    }

    return spec;
}