#include "wtk_ngccspec2.h"


wtk_ngccspec2_t *wtk_ngccspec2_new(int nbin,int channel,int npairs,int ang_num)
{
    wtk_ngccspec2_t *ngccspec2;

    ngccspec2=(wtk_ngccspec2_t *)wtk_malloc(sizeof(wtk_ngccspec2_t));
    ngccspec2->nbin=nbin;
    ngccspec2->channel=channel;
    ngccspec2->npairs=npairs;
    ngccspec2->ang_num=ang_num;

    ngccspec2->alpha=wtk_complex_new_p3(ang_num,nbin,ngccspec2->npairs);

    ngccspec2->alpha_meth=wtk_float_new_p2(nbin, npairs);
    ngccspec2->meth_init=0;

    return ngccspec2;
}


void wtk_ngccspec2_reset(wtk_ngccspec2_t *ngccspec2)
{
    ngccspec2->meth_init=0;
    wtk_complex_zero_p3(ngccspec2->alpha, ngccspec2->ang_num, ngccspec2->nbin, ngccspec2->npairs);
}

void wtk_ngccspec2_delete(wtk_ngccspec2_t *ngccspec2)
{
    wtk_complex_delete_p3(ngccspec2->alpha, ngccspec2->ang_num, ngccspec2->nbin);
    wtk_float_delete_p2(ngccspec2->alpha_meth,ngccspec2->nbin);
    wtk_free(ngccspec2);
}

void wtk_ngccspec2_start(wtk_ngccspec2_t *ngccspec2, float **mic_pos, float sv, int rate, float theta, float phi ,float **pairs_m, int ang_idx)
{
    int npairs=ngccspec2->npairs;
    int i,j,k,n;
    int nbin=ngccspec2->nbin;
    // int channel=ngccspec2->channel;
    float td,f,d;
    float x,y,z;
    wtk_complex_t **alpha=ngccspec2->alpha[ang_idx];
    float **alpha_meth=ngccspec2->alpha_meth;
    int win=(nbin-1)*2;

    phi*=PI/180;
    theta*=PI/180;
    x=cos(phi)*cos(theta);
    y=cos(phi)*sin(theta);
    z=sin(phi);

    if(ngccspec2->meth_init)
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
                // f=rate*1.0/win*k*2*PI*d/sv;
                // alpha_meth[k][n]=sin(f)/f;

                alpha_meth[k][n]=10*sv/(d*rate);

                f=-2*rate*1.0/win*k*PI*td/sv;
                alpha[k][n].a=cos(f);
                alpha[k][n].b=sin(f);
            }
        }
    }
    ngccspec2->meth_init=1;
}

void wtk_ngccspec2_start2(wtk_ngccspec2_t *ngccspec2,  float **mic_pos, float sv, int rate, float x, float y, float z, float **pairs_m,int ang_idx)
{
    int npairs=ngccspec2->npairs;
    int i,j,k,n;
    int nbin=ngccspec2->nbin;
    // int channel=ngccspec2->channel;
    float td,f,d;
    wtk_complex_t **alpha=ngccspec2->alpha[ang_idx];
    float **alpha_meth=ngccspec2->alpha_meth;
    int win=(nbin-1)*2;

    if(ngccspec2->meth_init)
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
                // f=rate*1.0/win*k*2*PI*d/sv;
                // alpha_meth[k][n]=sin(f)/f;

                alpha_meth[k][n]=10*sv/(d*rate);

                f=-2*rate*1.0/win*k*PI*td/sv;
                alpha[k][n].a=cos(f);
                alpha[k][n].b=sin(f);
            }
        }
    }
    ngccspec2->meth_init=1;
}

float wtk_ngccspec2_flush_spec_k(wtk_ngccspec2_t *ngccspec2, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx)
{
    wtk_complex_t *alpha=ngccspec2->alpha[ang_idx][k];
    float *meth=ngccspec2->alpha_meth[k];
    int i,j,n;
    float spec;
    int npairs=ngccspec2->npairs;
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

        spec+=1-tanh(meth[n]*sqrt(fabs(2-2*(xx.a*alpha[n].a - xx.b*alpha[n].b)/f)));
    }

    return spec;
}

float wtk_ngccspec2_flush_spec_k2(wtk_ngccspec2_t *ngccspec2, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx)
{
    wtk_complex_t *alpha=ngccspec2->alpha[ang_idx][k];
    float *meth=ngccspec2->alpha_meth[k];
    int i,j,n;
    float spec;
    int npairs=ngccspec2->npairs;
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

        spec+=1-tanh(meth[n]*sqrt(fabs(2-2*(xx.a*alpha[n].a - xx.b*alpha[n].b)/f)));
    }

    return spec;
}