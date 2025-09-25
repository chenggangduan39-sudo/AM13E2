#include "wtk_dswspec2.h"


wtk_dswspec2_t *wtk_dswspec2_new(int nbin,int channel,int npairs, int ang_num)
{
    wtk_dswspec2_t *dswspec2;

    dswspec2=(wtk_dswspec2_t *)wtk_malloc(sizeof(wtk_dswspec2_t));
    dswspec2->nbin=nbin;
    dswspec2->channel=channel;
    dswspec2->npairs=npairs;
    dswspec2->ang_num=ang_num;

    dswspec2->alpha=wtk_complex_new_p3(ang_num,nbin, npairs);
    dswspec2->sinc=wtk_float_new_p2(nbin, npairs);
    dswspec2->sinc_init=0;

    return dswspec2;
}

void wtk_dswspec2_reset(wtk_dswspec2_t *dswspec2)
{
    dswspec2->sinc_init=0;
    wtk_complex_zero_p3(dswspec2->alpha, dswspec2->ang_num, dswspec2->nbin, dswspec2->npairs);
}

void wtk_dswspec2_delete(wtk_dswspec2_t *dswspec2)
{
    wtk_float_delete_p2(dswspec2->sinc,dswspec2->nbin);
    wtk_complex_delete_p3(dswspec2->alpha, dswspec2->ang_num, dswspec2->nbin);
    wtk_free(dswspec2);
}

void wtk_dswspec2_start(wtk_dswspec2_t *dswspec2,  float **mic_pos, float sv, int rate, float theta, float phi ,float **pairs_m, int ang_idx)
{
    int npairs=dswspec2->npairs;
    int i,j,k,n;
    int nbin=dswspec2->nbin;
    float td,f,d;
    float x,y,z;
    wtk_complex_t **alpha=dswspec2->alpha[ang_idx];
    int win=(nbin-1)*2;
    float **sinc=dswspec2->sinc;

    phi*=PI/180;
    theta*=PI/180;
    x=cos(phi)*cos(theta);
    y=cos(phi)*sin(theta);
    z=sin(phi);

    if(dswspec2->sinc_init)
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

    dswspec2->sinc_init=1;
}

void wtk_dswspec2_start2(wtk_dswspec2_t *dswspec2, float **mic_pos, float sv, int rate, float x, float y, float z, float **pairs_m, int ang_idx)
{
    int npairs=dswspec2->npairs;
    int i,j,k,n;
    int nbin=dswspec2->nbin;
    float td,f,d;
    wtk_complex_t **alpha=dswspec2->alpha[ang_idx];
    int win=(nbin-1)*2;
    float **sinc=dswspec2->sinc;

    if(dswspec2->sinc_init)
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

    dswspec2->sinc_init=1;
}

float wtk_dswspec2_flush_spec_k(wtk_dswspec2_t *dswspec2, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx)
{
    int channel=dswspec2->channel;
    wtk_complex_t *alpha=dswspec2->alpha[ang_idx][k];
    int i,j,n;
    float tr,spec;
    wtk_complex_t r11,r22,r12;
    int npairs=dswspec2->npairs;
    float *sinc=dswspec2->sinc[k];
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
        tmp=(r12.a*alpha[n].a - r12.b*alpha[n].b);
        spec+=-(1+sinc[n])/2+(1-sinc[n])/2*(tr+2*tmp)/(tr-2*tmp);
    }

    return spec;
}