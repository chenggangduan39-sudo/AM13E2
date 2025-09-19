#include "wtk_zdsspec.h"

wtk_zdsspec_t *wtk_zdsspec_new(int nbin,int channel, int ang_num)
{
    wtk_zdsspec_t *zdsspec;

    zdsspec=(wtk_zdsspec_t *)wtk_malloc(sizeof(wtk_zdsspec_t));
    zdsspec->nbin=nbin;
    zdsspec->channel=channel;
    zdsspec->ang_num=ang_num;
    zdsspec->ovec=wtk_complex_new_p3(ang_num,nbin,channel);
    zdsspec->tmp=(wtk_complex_t *)wtk_malloc(channel*sizeof(wtk_complex_t));

    return zdsspec;
}

void wtk_zdsspec_reset(wtk_zdsspec_t *zdsspec)
{
    wtk_complex_zero_p3(zdsspec->ovec,zdsspec->ang_num,zdsspec->nbin,zdsspec->channel);
}

void wtk_zdsspec_delete(wtk_zdsspec_t *zdsspec)
{
    wtk_complex_delete_p3(zdsspec->ovec,zdsspec->ang_num,zdsspec->nbin);
    wtk_free(zdsspec->tmp);
    wtk_free(zdsspec);
}

void wtk_zdsspec_flush_ovec(wtk_zdsspec_t *zdsspec,  wtk_complex_t *ovec, float **mic_pos, float sv, int rate, 
                                                        float theta2, float phi2, int k, float *tdoa)
{
	float x,y,z;
	float t;
	float *mic;
	int j;
	int channel=zdsspec->channel;
	int win=(zdsspec->nbin-1)*2;
	wtk_complex_t *ovec1;
    float theta,phi;

    phi=phi2*PI/180;
    theta=theta2*PI/180;
    x=cos(phi)*cos(theta);
    y=cos(phi)*sin(theta);
    z=sin(phi);

	for(j=0;j<channel;++j)
	{
		mic=mic_pos[j];
		tdoa[j]=(mic[0]*x+mic[1]*y+mic[2]*z)/sv;
	}
    // x=1.0/sqrt(channel);
    ovec1=ovec;
    t=2*PI*rate*1.0/win*k;
    for(j=0;j<channel;++j)
    {
        ovec1[j].a=cos(t*tdoa[j]);
        ovec1[j].b=sin(t*tdoa[j]);
    }
}

void wtk_zdsspec_flush_ovec2(wtk_zdsspec_t *zdsspec,  wtk_complex_t *ovec, float **mic_pos, float sv, int rate, 
                                                        float x,float y,float z, int k, float *tdoa)
{
	float t;
	float *mic;
	int j;
	int channel=zdsspec->channel;
	int win=(zdsspec->nbin-1)*2;
	wtk_complex_t *ovec1;

    for(j=0;j<channel;++j)
    {
        mic=mic_pos[j];
        tdoa[j]=sqrt((mic[0]-x)*(mic[0]-x)+(mic[1]-y)*(mic[1]-y)+(mic[2]-z)*(mic[2]-z))/sv;
        if(j>0)
        {
            tdoa[j]-=tdoa[0];
        }
    }
    tdoa[0]=0;
    // x=1.0/sqrt(channel);
    ovec1=ovec;
    t=2*PI*rate*1.0/win*k;
    for(j=0;j<channel;++j)
    {
        ovec1[j].a=cos(t*tdoa[j]);
        ovec1[j].b=sin(t*tdoa[j]);
    }
}


void wtk_zdsspec_start(wtk_zdsspec_t *zdsspec,  float **mic_pos, float sv, int rate, float theta, float phi, int ang_idx, int use_line, float ls_eye)
{
    int channel=zdsspec->channel;
	int nbin=zdsspec->nbin;
    wtk_complex_t **ovec=zdsspec->ovec[ang_idx];
    wtk_complex_t *novec,*a,*b;
    wtk_complex_t *cov,*cov1,*cov2;
    int i,j,k;
    // int theta2;
    wtk_dcomplex_t *tmp_inv;
    wtk_complex_t *tmp=zdsspec->tmp;
    float *tdoa;
    float max_th=use_line?180.0:359.0;
    float th;

    tdoa=(float *)wtk_malloc(channel*sizeof(float));
    novec=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*channel);
    cov=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*channel*channel);
    tmp_inv=(wtk_dcomplex_t *)wtk_malloc(channel*(channel+1)*sizeof(wtk_dcomplex_t));
    // theta2=theta;
    for(k=1;k<nbin-1;++k)
    {
        wtk_zdsspec_flush_ovec(zdsspec, ovec[k],mic_pos,sv,rate,theta,0,k,tdoa);
        memset(cov,0,sizeof(wtk_complex_t)*channel*channel);
        for(th=0; th<=max_th; th+=1)
        {
            wtk_zdsspec_flush_ovec(zdsspec, novec, mic_pos,sv,rate,th,0,k,tdoa);
            a=novec;
            for(i=0;i<channel;++i,++a)
            {
                b=a;
                for(j=i;j<channel;++j,++b)
                {
                    cov1=cov+i*channel+j;
                    if(i!=j)
                    {
                        cov1->a+=a->a*b->a+a->b*b->b;
                        cov1->b+=-a->a*b->b+a->b*b->a;
                        cov2=cov+j*channel+i;
                        cov2->a=cov1->a;
                        cov2->b=-cov1->b;
                    }else
                    {
                        cov1->a+=a->a*b->a+a->b*b->b;
                        cov1->b=0;
                    }
                }
            }
        }
    	// ret=wtk_complex_guass_elimination_p1(cov, ovec[k], tmp_inv, channel, tmp);
        // if(ret!=0)
        // {
            for(i=0,j=0;i<channel;++i)
            {
                cov[j].a+=ls_eye;
                j+=channel+1;
            }
            wtk_complex_guass_elimination_p1(cov, ovec[k], tmp_inv, channel, tmp);
        // }
        memcpy(ovec[k], tmp, sizeof(wtk_complex_t)*channel);
    }

    wtk_free(tdoa);
    wtk_free(cov);
    wtk_free(tmp_inv);
    wtk_free(novec);
}

void wtk_zdsspec_start2(wtk_zdsspec_t *zdsspec, float **mic_pos, float sv, int rate,float x, float y, float z, int ang_idx, int use_line, float ls_eye)
{
    int channel=zdsspec->channel;
	int nbin=zdsspec->nbin;
    wtk_complex_t **ovec=zdsspec->ovec[ang_idx];
    wtk_complex_t *novec,*a,*b;
    wtk_complex_t *cov,*cov1,*cov2;
    int i,j,k;
    wtk_dcomplex_t *tmp_inv;
    wtk_complex_t *tmp=zdsspec->tmp;
    float *tdoa;
    float max_th=use_line?180.0:359.0;
    float th;

    tdoa=(float *)wtk_malloc(channel*sizeof(float));
    novec=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*channel);
    cov=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*channel*channel);
    tmp_inv=(wtk_dcomplex_t *)wtk_malloc(channel*(channel+1)*sizeof(wtk_dcomplex_t));
    for(k=1;k<nbin-1;++k)
    {
        wtk_zdsspec_flush_ovec2(zdsspec, ovec[k],mic_pos,sv,rate,x,y,z,k,tdoa);
        memset(cov,0,sizeof(wtk_complex_t)*channel*channel);
        for(th=0; th<=max_th; th+=1)
        {
            wtk_zdsspec_flush_ovec(zdsspec, novec, mic_pos,sv,rate,th,0,k,tdoa);
            a=novec;
            for(i=0;i<channel;++i,++a)
            {
                b=a;
                for(j=i;j<channel;++j,++cov1,++b)
                {
                    cov1=cov+i*channel+j;
                    if(i!=j)
                    {
                        cov1->a+=a->a*b->a+a->b*b->b;
                        cov1->b+=-a->a*b->b+a->b*b->a;
                        cov2=cov+j*channel+i;
                        cov2->a=cov1->a;
                        cov2->b=-cov1->b;
                    }else
                    {
                        cov1->a+=a->a*b->a+a->b*b->b;
                        cov1->b=0;
                    }
                }
            }
        }
        // for(i=0,j=0;i<channel;++i)
        // {
        //     cov[j].a+=0.03;
        //     j+=channel+1;
        // }
    	// ret=wtk_complex_guass_elimination_p1(cov, ovec[k], tmp_inv, channel, tmp);
        // if(ret!=0)
        // {
            for(i=0,j=0;i<channel;++i)
            {
                cov[j].a+=ls_eye;
                j+=channel+1;
            }
            wtk_complex_guass_elimination_p1(cov, ovec[k], tmp_inv, channel, tmp);
        // }
        memcpy(ovec[k], tmp, sizeof(wtk_complex_t)*channel);
    }

    wtk_free(tdoa);
    wtk_free(cov);
    wtk_free(tmp_inv);
    wtk_free(novec);
}

float wtk_zdsspec_flush_spec_k(wtk_zdsspec_t *zdsspec, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx)
{
    wtk_complex_t *a,*b;
    float fa,fb,f;
    int channel=zdsspec->channel;
    wtk_complex_t *ovec=zdsspec->ovec[ang_idx][k];
    wtk_complex_t *tmp=zdsspec->tmp;
    int i,j;

    a=cov;
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
    }
    // fa=fb=0;
    fa=0;
    for(i=0;i<channel;++i)
    {
        a=ovec+i;
        b=tmp+i;
        fa+=a->a*b->a+a->b*b->b;
        // fb+=a->a*b->b-a->b*b->a;
    }
    f=fa;
    if(cov_travg-f<1e-6)
    {
        f=f/1e-6;
    }else
    {
        f=f/(cov_travg-f);
    }
    
    return f;
}
