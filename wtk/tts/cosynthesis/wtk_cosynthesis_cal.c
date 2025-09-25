#include "wtk_cosynthesis_cal.h"

float kld_dur_cal(float *mean1,float *mean2,float*var1,float*var2,int nstate,int nphone)
{
    int i,j;
    float euclidean;
    float factor=0.5;
    float sum1,sum2,kld=0;

    for(i=0;i<nphone;++i)
    {
    	sum1=0;
    	sum2=0;
        for(j=0;j<nstate;++j)
        {
            euclidean = powf(mean1[i*nstate+j]-mean2[i*nstate+j],2);
            sum1 += (euclidean + var1[i*nstate+j])/var2[i*nstate+j];
            sum2 += (euclidean + var2[i*nstate+j])/var1[i*nstate+j];
        }
        kld +=factor*(sum1+sum2-10);
        //printf("kld_dur=%f\n", kld);
    }

    return kld;
}

float kld_mcep_cal(float *mean1,float *mean2,float*var1,float*var2,int n,int nphone)
{
    int i,j;
    float euclidean;
    float factor=0.5;
    float sum1,sum2,kld=0;

    for(i=0;i<nphone;++i)
    {
    	sum1=0;
    	sum2=0;
        for(j=0;j<n;++j)
        {
            euclidean = powf(mean1[i*n+j]-mean2[i*n+j],2);
            sum1 += (euclidean + var1[i*n+j])/var2[i*n+j];
            sum2 += (euclidean + var2[i*n+j])/var1[i*n+j];
        }
        kld +=factor*(sum1+sum2-2*n);
        //printf("kld_mcep=%f\n", kld);
    }

    return kld;
}

float kld_lf0_cal(float *mean1,float *mean2,float*var1,float*var2,float *weight1,float *weight2,int n,int nphone,float*constant)
{
    int i,j;
    float euclidean,const1;
    float factor=0.5;
    float sum1,sum2,sum;
    float kld=0;

    for(i=0;i<nphone;++i)
    {
        for(j=0;j<n;++j)
        {
            euclidean = powf(mean1[i*n+j]-mean2[i*n+j],2);
            const1 = logf(var1[i]) - logf(weight1[i*n+j]) + logf(1-weight1[i*n+j]);
            sum1 = weight1[i*n+j]*(euclidean+var1[i*n+j])/var2[i*n+j];
            sum2 = weight2[i*n+j]*(euclidean+var2[i*n+j])/var1[i*n+j];
            sum = factor*((weight2[i*n+j]-weight1[i*n+j])*(const1-constant[i*n+j])+sum1+sum2-weight1[i*n+j]-weight2[i*n+j]);
        }
        kld+=sum;
        //printf("kld_lf0=%f\n", kld);
    }

    return kld;
}

float kld_preselect(float*candi_dur_mean,float *candi_dur_var,float* candi_lf0_mean,float *candi_lf0_var,
float* candi_lf0_weight,float *candi_mcep_mean,float *candi_mcep_var,float *dur_mean,float *dur_var,
float*lf0_mean,float *lf0_var,float* lf0_weight,float *mcep_mean,float *mcep_var,int nphone,float* constant,
int dur_len,int lf0_len,int mcep_len)
{
    float w_spec_kld = 0.5;
    float w_pitch_kld = 1;
    float w_dur_kld = 0.2;
    float tot_kld=0;
    float dur_kld=0;
    float lf0_kld=0;
    float mcep_kld=0;
    dur_kld = kld_dur_cal(dur_mean,candi_dur_mean,dur_var,candi_dur_var,dur_len,nphone);
    lf0_kld = kld_lf0_cal(lf0_mean,candi_lf0_mean,lf0_var,candi_lf0_var,lf0_weight,candi_lf0_weight,lf0_len,nphone,constant);
    mcep_kld = kld_mcep_cal(mcep_mean,candi_mcep_mean,mcep_var,candi_mcep_var,mcep_len,nphone);
    // wtk_debug("%f %f %f\n",dur_kld,lf0_kld,mcep_kld);
    tot_kld = w_pitch_kld*lf0_kld + w_dur_kld*dur_kld + w_spec_kld*mcep_kld;
    return tot_kld;
}
