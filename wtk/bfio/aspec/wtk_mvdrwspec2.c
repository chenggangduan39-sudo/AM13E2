#include "wtk_mvdrwspec2.h"


wtk_mvdrwspec2_t *wtk_mvdrwspec2_new(int nbin,int channel,int npairs,int ang_num)
{
    wtk_mvdrwspec2_t *mvdrwspec2;

    mvdrwspec2=(wtk_mvdrwspec2_t *)wtk_malloc(sizeof(wtk_mvdrwspec2_t));
    mvdrwspec2->nbin=nbin;
    mvdrwspec2->channel=channel;
    mvdrwspec2->npairs=npairs;
    mvdrwspec2->ang_num=ang_num;

    mvdrwspec2->alpha=wtk_complex_new_p3(ang_num, nbin, npairs);
    mvdrwspec2->sinc=wtk_float_new_p2(nbin, npairs);
    mvdrwspec2->sinc_init=0;

    return mvdrwspec2;
}

void wtk_mvdrwspec2_reset(wtk_mvdrwspec2_t *mvdrwspec2)
{
    mvdrwspec2->sinc_init=0;
    wtk_complex_zero_p3(mvdrwspec2->alpha, mvdrwspec2->ang_num, mvdrwspec2->nbin, mvdrwspec2->npairs);
}

void wtk_mvdrwspec2_delete(wtk_mvdrwspec2_t *mvdrwspec2)
{
    wtk_complex_delete_p3(mvdrwspec2->alpha,mvdrwspec2->ang_num,mvdrwspec2->nbin);
    wtk_float_delete_p2(mvdrwspec2->sinc,mvdrwspec2->nbin);
    wtk_free(mvdrwspec2);
}

void wtk_mvdrwspec2_start(wtk_mvdrwspec2_t *mvdrwspec2, float **mic_pos, float sv, int rate, float theta, float phi ,float **pairs_m, int ang_idx)
{
    int npairs=mvdrwspec2->npairs;
    int i,j,k,n;
    int nbin=mvdrwspec2->nbin;
    // int channel=mvdrwspec2->channel;
    float d,td,f;
    float x,y,z;
    wtk_complex_t **alpha=mvdrwspec2->alpha[ang_idx];
    float **sinc=mvdrwspec2->sinc;
    int win=(nbin-1)*2;

    phi*=PI/180;
    theta*=PI/180;
    x=cos(phi)*cos(theta);
    y=cos(phi)*sin(theta);
    z=sin(phi);

    if(mvdrwspec2->sinc_init)
    {
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
    }else
    {
        for(n=0; n<npairs; ++n)
        {
            i=pairs_m[n][0];
            j=pairs_m[n][1];

            d=sqrt((mic_pos[i][0]-mic_pos[j][0])*(mic_pos[i][0]-mic_pos[j][0])+
                                    (mic_pos[i][1]-mic_pos[j][1])*(mic_pos[i][1]-mic_pos[j][1])+(mic_pos[i][2]-mic_pos[j][2])*(mic_pos[i][2]-mic_pos[j][2]));
            
            td=(mic_pos[i][0]*x+mic_pos[i][1]*y+mic_pos[i][2]*z)-(mic_pos[j][0]*x+mic_pos[j][1]*y+mic_pos[j][2]*z);

            for(k=1;k<nbin-1;++k)
            {
                f=rate*1.0/win*k*2*PI*d/sv;
                sinc[k][n]=sin(f)/f;

                f=-2*rate*1.0/win*k*PI*td/sv;
                alpha[k][n].a=cos(f);
                alpha[k][n].b=sin(f);
            }
        }        
    }

    mvdrwspec2->sinc_init=1;
}

void wtk_mvdrwspec2_start2(wtk_mvdrwspec2_t *mvdrwspec2, float **mic_pos, float sv, int rate, float x, float y, float z, float **pairs_m, int ang_idx)
{
    int npairs=mvdrwspec2->npairs;
    int i,j,k,n;
    int nbin=mvdrwspec2->nbin;
    // int channel=mvdrwspec2->channel;
    float d,td,f;
    wtk_complex_t **alpha=mvdrwspec2->alpha[ang_idx];
    float **sinc=mvdrwspec2->sinc;
    int win=(nbin-1)*2;

    if(mvdrwspec2->sinc_init)
    {
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
    }else
    {
        for(n=0; n<npairs; ++n)
        {
            i=pairs_m[n][0];
            j=pairs_m[n][1];

            d=sqrt((mic_pos[i][0]-mic_pos[j][0])*(mic_pos[i][0]-mic_pos[j][0])+
                                    (mic_pos[i][1]-mic_pos[j][1])*(mic_pos[i][1]-mic_pos[j][1])+(mic_pos[i][2]-mic_pos[j][2])*(mic_pos[i][2]-mic_pos[j][2]));
            
            td=sqrt((mic_pos[i][0]-x)*(mic_pos[i][0]-x)+(mic_pos[i][1]-y)*(mic_pos[i][1]-y)+(mic_pos[i][2]-z)*(mic_pos[i][2]-z))-
                        sqrt((mic_pos[j][0]-x)*(mic_pos[j][0]-x)+(mic_pos[j][1]-y)*(mic_pos[j][1]-y)+(mic_pos[j][2]-z)*(mic_pos[j][2]-z));

            for(k=1;k<nbin-1;++k)
            {
                f=rate*1.0/win*k*2*PI*d/sv;
                sinc[k][n]=sin(f)/f;

                f=-2*rate*1.0/win*k*PI*td/sv;
                alpha[k][n].a=cos(f);
                alpha[k][n].b=sin(f);
            }
        }        
    }

    mvdrwspec2->sinc_init=1;
}

float wtk_mvdrwspec2_flush_spec_k(wtk_mvdrwspec2_t *mvdrwspec2, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx)
{
    int channel=mvdrwspec2->channel;
    wtk_complex_t *alpha=mvdrwspec2->alpha[ang_idx][k];
    float *sinc=mvdrwspec2->sinc[k];
    int i,j,n;
    float num,tr,spec;
    wtk_complex_t r11,r22,r12,r21;
    int npairs=mvdrwspec2->npairs;
 
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
        spec+=-(1+sinc[n])/2+(1-sinc[n])/2*num/(0.5*tr-num);
    }

    return spec;
}