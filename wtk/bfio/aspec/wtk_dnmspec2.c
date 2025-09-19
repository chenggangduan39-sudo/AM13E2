#include "wtk_dnmspec2.h"


wtk_dnmspec2_t *wtk_dnmspec2_new(int nbin,int channel,int npairs, int ang_num)
{
    wtk_dnmspec2_t *dnmspec2;

    dnmspec2=(wtk_dnmspec2_t *)wtk_malloc(sizeof(wtk_dnmspec2_t));
    dnmspec2->nbin=nbin;
    dnmspec2->channel=channel;
    dnmspec2->npairs=npairs;
    dnmspec2->ang_num=ang_num;

    dnmspec2->invA1=wtk_float_new_p3(ang_num,nbin,dnmspec2->npairs);
    dnmspec2->invA2=wtk_float_new_p3(ang_num,nbin,dnmspec2->npairs);
    dnmspec2->invAx=wtk_complex_new_p3(ang_num,nbin,dnmspec2->npairs);
    dnmspec2->invAx2=wtk_complex_new_p3(ang_num,nbin,dnmspec2->npairs);
    dnmspec2->invL1=wtk_float_new_p3(ang_num,nbin,dnmspec2->npairs);
    dnmspec2->invL2=wtk_float_new_p3(ang_num,nbin,dnmspec2->npairs);
    
    return dnmspec2;
}

void wtk_dnmspec2_reset(wtk_dnmspec2_t *dnmspec2)
{

}

void wtk_dnmspec2_delete(wtk_dnmspec2_t *dnmspec2)
{
    wtk_float_delete_p3(dnmspec2->invA1,dnmspec2->ang_num,dnmspec2->nbin);
    wtk_float_delete_p3(dnmspec2->invA2,dnmspec2->ang_num,dnmspec2->nbin);
    wtk_complex_delete_p3(dnmspec2->invAx,dnmspec2->ang_num,dnmspec2->nbin);
    wtk_complex_delete_p3(dnmspec2->invAx2,dnmspec2->ang_num,dnmspec2->nbin);
    wtk_float_delete_p3(dnmspec2->invL1,dnmspec2->ang_num,dnmspec2->nbin);
    wtk_float_delete_p3(dnmspec2->invL2,dnmspec2->ang_num,dnmspec2->nbin);

    wtk_free(dnmspec2);
}



void wtk_dnmspec2_start(wtk_dnmspec2_t *dnmspec2, float **mic_pos, float sv, int rate, float theta, float phi ,float **pairs_m, int ang_idx)
{
    int i,j,k,n;
    int nbin=dnmspec2->nbin;
    int npairs=dnmspec2->npairs;
    // int channel=dnmspec2->channel;
    float d,sinc,f,sinc2,td;
    int win=(nbin-1)*2;
    float x,y,z, ff;
    wtk_complex_t EXP, P, inva11, inva12, tmp, tmp2, tmp3;

    float **invA1=dnmspec2->invA1[ang_idx];
    float **invA2=dnmspec2->invA2[ang_idx];
    wtk_complex_t **invAx=dnmspec2->invAx[ang_idx];
    wtk_complex_t **invAx2=dnmspec2->invAx2[ang_idx];
    float **invL1=dnmspec2->invL1[ang_idx];
    float **invL2=dnmspec2->invL2[ang_idx];

    // printf("====%f %f====\n",theta,phi);

    phi*=PI/180;
    theta*=PI/180;
    x=cos(phi)*cos(theta);
    y=cos(phi)*sin(theta);
    z=sin(phi);

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
            sinc=sin(f)/f;
            sinc2=sinc*sinc;

            f=-2*rate*1.0/win*k*PI*td/sv;
            EXP.a=cos(f);
            EXP.b=sin(f);

            P.a=EXP.a * sinc;
            P.b=EXP.b * sinc;

            ff=sqrt(0.5) / ( 1- P.a);
            inva11.a=ff * (1-P.a);
            inva11.b=ff * P.b;

            tmp.a=sinc-EXP.a;
            tmp.b=-EXP.b;
            ff=tmp.a*tmp.a+tmp.b*tmp.b;
            tmp2.a=-1+P.a;
            tmp2.b=P.b;
            tmp3.a=(tmp2.a*tmp.a+tmp2.b*tmp.b)/ff;
            tmp3.b=(-tmp2.a*tmp.b+tmp2.b*tmp.a)/ff;
            inva12.a=tmp3.a*inva11.a-tmp3.b*inva11.b;
            inva12.b=tmp3.a*inva11.b+tmp3.b*inva11.a;

            invA1[k][n]=(inva11.a*inva11.a + inva11.b*inva11.b);
            invA2[k][n]=(inva12.a*inva12.a + inva12.b*inva12.b);

            invAx[k][n].a=inva11.a*inva12.a-inva11.b*inva12.b;
            invAx[k][n].b=inva11.a*inva12.b+inva11.b*inva12.a;
            invAx2[k][n].a=inva11.a*inva12.a+inva11.b*inva12.b;
            invAx2[k][n].b=-inva11.a*inva12.b+inva11.b*inva12.a;


            ff=1-2*P.a+sinc2;
            if(ff < 1e-12)
            {
                ff+=1e-12;
            }
            ff= 0.5/ff;
            invL1[k][n]= (sinc2-1) * ff;
            invL2[k][n]= 2*(1-P.a) * ff;
        }
    }
}

void wtk_dnmspec2_start2(wtk_dnmspec2_t *dnmspec2, float **mic_pos, float sv, int rate, float x, float y, float z, float **pairs_m, int ang_idx)
{
    int i,j,k,n;
    int nbin=dnmspec2->nbin;
    int npairs=dnmspec2->npairs;
    // int channel=dnmspec2->channel;
    float d,sinc,f,sinc2,td;
    int win=(nbin-1)*2;
    float ff;
    wtk_complex_t EXP, P, inva11, inva12, tmp, tmp2, tmp3;

    float **invA1=dnmspec2->invA1[ang_idx];
    float **invA2=dnmspec2->invA2[ang_idx];
    wtk_complex_t **invAx=dnmspec2->invAx[ang_idx];
    wtk_complex_t **invAx2=dnmspec2->invAx2[ang_idx];
    float **invL1=dnmspec2->invL1[ang_idx];
    float **invL2=dnmspec2->invL2[ang_idx];

    // printf("====%f %f====\n",theta,phi);

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
            sinc=sin(f)/f;
            sinc2=sinc*sinc;

            f=-2*rate*1.0/win*k*PI*td/sv;
            EXP.a=cos(f);
            EXP.b=sin(f);

            P.a=EXP.a * sinc;
            P.b=EXP.b * sinc;

            ff=sqrt(0.5) / ( 1- P.a);
            inva11.a=ff * (1-P.a);
            inva11.b=ff * P.b;

            tmp.a=sinc-EXP.a;
            tmp.b=-EXP.b;
            ff=tmp.a*tmp.a+tmp.b*tmp.b;
            tmp2.a=-1+P.a;
            tmp2.b=P.b;
            tmp3.a=(tmp2.a*tmp.a+tmp2.b*tmp.b)/ff;
            tmp3.b=(-tmp2.a*tmp.b+tmp2.b*tmp.a)/ff;
            inva12.a=tmp3.a*inva11.a-tmp3.b*inva11.b;
            inva12.b=tmp3.a*inva11.b+tmp3.b*inva11.a;

            invA1[k][n]=(inva11.a*inva11.a + inva11.b*inva11.b);
            invA2[k][n]=(inva12.a*inva12.a + inva12.b*inva12.b);

            invAx[k][n].a=inva11.a*inva12.a-inva11.b*inva12.b;
            invAx[k][n].b=inva11.a*inva12.b+inva11.b*inva12.a;
            invAx2[k][n].a=inva11.a*inva12.a+inva11.b*inva12.b;
            invAx2[k][n].b=-inva11.a*inva12.b+inva11.b*inva12.a;


            ff=1-2*P.a+sinc2;
            if(ff < 1e-12)
            {
                ff+=1e-12;
            }
            ff= 0.5/ff;
            invL1[k][n]= (sinc2-1) * ff;
            invL2[k][n]= 2*(1-P.a) * ff;
        }
    }
}

float wtk_dnmspec2_flush_spec_k(wtk_dnmspec2_t *dnmspec2, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx)
{
    float *invA1=dnmspec2->invA1[ang_idx][k];
    float *invA2=dnmspec2->invA2[ang_idx][k];
    wtk_complex_t *invAx=dnmspec2->invAx[ang_idx][k];
    wtk_complex_t *invAx2=dnmspec2->invAx2[ang_idx][k];
    float *invL1=dnmspec2->invL1[ang_idx][k];
    float *invL2=dnmspec2->invL2[ang_idx][k];
    int channel=dnmspec2->channel;
    int i,j,n;
    float spec;
    float ARA1,ARA2,vs,vb;
    wtk_complex_t tmp;
    int npairs=dnmspec2->npairs;
    int n11,n22,n12,n21;

    spec=0;
    for(n=0; n<npairs; ++n)
    {
        i=pairs_m[n][0];
        j=pairs_m[n][1];
        n11=i*channel+i;
        n22=j*channel+j;
        n12=i*channel+j;
        n21=j*channel+i;

        ARA1 = invA1[n] * cov[n11].a + invA2[n] * cov[n22].a;
        tmp=cov[n21];
        ARA2 = ARA1 - 2 * (tmp.a*invAx[n].a - tmp.b*invAx[n].b);
        tmp=cov[n12];
        ARA1 = ARA1 + 2 * (tmp.a*invAx2[n].a - tmp.b*invAx2[n].b);
        vs = 0.5*ARA1 + invL1[n]*ARA2;
        vb = invL2[n]*ARA2;

        if(vs >0 && vb >0)
        {
            spec += vs/vb;
        }
    }

    return spec;
}