#include "wtk_maskzdsspec.h"

wtk_maskzdsspec_t *wtk_maskzdsspec_new(int nbin,int channel, int ang_num)
{
    wtk_maskzdsspec_t *maskzdsspec;

    maskzdsspec=(wtk_maskzdsspec_t *)wtk_malloc(sizeof(wtk_maskzdsspec_t));
    maskzdsspec->nbin=nbin;
    maskzdsspec->channel=channel;
    maskzdsspec->ang_num=ang_num;
    maskzdsspec->ovec=wtk_complex_new_p3(ang_num,nbin,channel);
    maskzdsspec->tmp=(wtk_complex_t *)wtk_malloc(channel*sizeof(wtk_complex_t));

    return maskzdsspec;
}

void wtk_maskzdsspec_reset(wtk_maskzdsspec_t *maskzdsspec)
{
    wtk_complex_zero_p3(maskzdsspec->ovec,maskzdsspec->ang_num,maskzdsspec->nbin,maskzdsspec->channel);
}

void wtk_maskzdsspec_delete(wtk_maskzdsspec_t *maskzdsspec)
{
    wtk_complex_delete_p3(maskzdsspec->ovec,maskzdsspec->ang_num,maskzdsspec->nbin);
    wtk_free(maskzdsspec->tmp);
    wtk_free(maskzdsspec);
}

void wtk_maskzdsspec_flush_ovec(wtk_maskzdsspec_t *maskzdsspec,  wtk_complex_t *ovec, float **mic_pos, float sv, int rate, 
                                                        float theta2, float phi2, int k, float *tdoa)
{
	float x,y,z;
	float t;
	float *mic;
	int j;
	int channel=maskzdsspec->channel;
	int win=(maskzdsspec->nbin-1)*2;
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
    x=1.0/sqrt(channel);
    ovec1=ovec;
    t=2*PI*rate*1.0/win*k;
    for(j=0;j<channel;++j)
    {
        ovec1[j].a=cos(t*tdoa[j])*x;
        ovec1[j].b=sin(t*tdoa[j])*x;
    }
}

void wtk_maskzdsspec_start(wtk_maskzdsspec_t *maskzdsspec,  float **mic_pos, float sv, int rate, float theta, float phi, int ang_idx, int use_line, float ls_eye, int th_step)
{
    int channel=maskzdsspec->channel;
	int nbin=maskzdsspec->nbin;
    wtk_complex_t **ovec=maskzdsspec->ovec[ang_idx];
    wtk_complex_t *novec,*a,*b;
    wtk_complex_t *cov,*cov1,*cov2;
    int i,j,k;
    // int theta2;
    wtk_dcomplex_t *tmp_inv;
    wtk_complex_t *tmp=maskzdsspec->tmp;
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
        wtk_maskzdsspec_flush_ovec(maskzdsspec, ovec[k],mic_pos,sv,rate,theta,0,k,tdoa);
        memset(cov,0,sizeof(wtk_complex_t)*channel*channel);
        for(th=0; th<=max_th; th+=th_step)
        {
            wtk_maskzdsspec_flush_ovec(maskzdsspec, novec, mic_pos,sv,rate,th,0,k,tdoa);
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


wtk_maskzdsspec_t *wtk_maskzdsspec_new2(int nbin,int channel, int ang_num, float **mic_pos,  float sv, int rate, int use_line)
{
    wtk_maskzdsspec_t *maskzdsspec;
    int i,j,k;
    int win=(nbin-1)*2;
    int max_theta=use_line?180:359;
    float t_tmp=2*PI*rate*1.0/win;
    float x_tmp=1.0/sqrt(channel);
    float *mic, *td, x, y, z, theta, t;
    float tmp;
    wtk_complex_t **ovec1;
    wtk_complex_t *ovec2;

    maskzdsspec=(wtk_maskzdsspec_t *)wtk_malloc(sizeof(wtk_maskzdsspec_t));
    maskzdsspec->nbin=nbin;
    maskzdsspec->channel=channel;
    maskzdsspec->ang_num=ang_num;
    maskzdsspec->max_theta=max_theta;
    
    maskzdsspec->ovec=wtk_complex_new_p3(ang_num,nbin,channel);
    maskzdsspec->tmp=(wtk_complex_t *)wtk_malloc(channel*sizeof(wtk_complex_t));

    maskzdsspec->a_ovec=wtk_complex_new_p3((max_theta+1),nbin,channel);
    maskzdsspec->t_nbins=(float *)wtk_malloc(sizeof(float)*nbin);
    maskzdsspec->tdoa=(float **)wtk_malloc(sizeof(float*)*(max_theta+1));
    for(i=0;i<=max_theta;++i){
        maskzdsspec->tdoa[i]=(float *)wtk_malloc(sizeof(float)*channel);
    }

    for(i=0;i<=max_theta;++i){
        theta=i*PI/180;
        td=maskzdsspec->tdoa[i];
        x=cos(theta);
        y=sin(theta);
        z=0;
        for(j=0;j<channel;++j){
            mic=mic_pos[j];
            td[j]=(mic[0]*x+mic[1]*y+mic[2]*z)/sv;
        }
    }
    for(i=0;i<nbin;++i){
        maskzdsspec->t_nbins[i]=t_tmp*i;
    }

    for(i=0;i<=max_theta;++i){
        ovec1=maskzdsspec->a_ovec[i];
        td=maskzdsspec->tdoa[i];
        for(j=0;j<nbin;++j){
            ovec2=ovec1[j];
            t=maskzdsspec->t_nbins[j];
            for(k=0;k<channel;++k){
                tmp=t*td[k];
                ovec2[k].a=cos(tmp)*x_tmp;
                ovec2[k].b=sin(tmp)*x_tmp;
            }
        }
    }


    return maskzdsspec;
}

void wtk_maskzdsspec_reset2(wtk_maskzdsspec_t *maskzdsspec)
{
    wtk_complex_zero_p3(maskzdsspec->ovec,maskzdsspec->ang_num,maskzdsspec->nbin,maskzdsspec->channel);
}

void wtk_maskzdsspec_delete2(wtk_maskzdsspec_t *maskzdsspec)
{
    int i;
    wtk_complex_delete_p3(maskzdsspec->ovec,maskzdsspec->ang_num,maskzdsspec->nbin);
    wtk_complex_delete_p3(maskzdsspec->a_ovec,maskzdsspec->max_theta+1,maskzdsspec->nbin);
    for(i=0;i<=maskzdsspec->max_theta;++i){
        wtk_free(maskzdsspec->tdoa[i]);
    }
    wtk_free(maskzdsspec->tdoa);
    wtk_free(maskzdsspec->t_nbins);
    wtk_free(maskzdsspec->tmp);
    wtk_free(maskzdsspec);
}

void wtk_maskzdsspec_start2(wtk_maskzdsspec_t *maskzdsspec,  float **mic_pos, float sv, int rate, float theta, float phi, int ang_idx, int use_line, float ls_eye, int th_step)
{
    int channel=maskzdsspec->channel;
	int nbin=maskzdsspec->nbin;
    wtk_complex_t **ovec=maskzdsspec->ovec[ang_idx];
    wtk_complex_t ***a_ovec=maskzdsspec->a_ovec;
    wtk_complex_t *a,*b;
    wtk_complex_t *cov,*cov1,*cov2;
    int i,j,k;
    // int theta2;
    wtk_dcomplex_t *tmp_inv;
    wtk_complex_t *tmp=maskzdsspec->tmp;
    float max_th=use_line?180.0:359.0;
    float th;

    cov=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*channel*channel);
    tmp_inv=(wtk_dcomplex_t *)wtk_malloc(channel*(channel+1)*sizeof(wtk_dcomplex_t));
    // theta2=theta;
    for(k=1;k<nbin-1;++k)
    {
        memset(cov,0,sizeof(wtk_complex_t)*channel*channel);
        for(th=0; th<=max_th; th+=th_step)
        {
            a=a_ovec[(int)th][k];
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
            wtk_complex_guass_elimination_p1(cov, a_ovec[(int)theta][k], tmp_inv, channel, tmp);
        // }
        memcpy(ovec[k], tmp, sizeof(wtk_complex_t)*channel);
    }

    wtk_free(cov);
    wtk_free(tmp_inv);
}

float wtk_maskzdsspec_flush_spec_k(wtk_maskzdsspec_t *maskzdsspec, wtk_complex_t *scov,wtk_complex_t *ncov, float prob, int k, int ang_idx)
{
    wtk_complex_t *b,*c;
    float fa,fb,fa2,fb2,f1,f2,f;
    int channel=maskzdsspec->channel;
    wtk_complex_t *ovec=maskzdsspec->ovec[ang_idx][k];
    int i,j;

    f1=f2=0;
    for(i=0;i<channel;++i)
    {
        fa=fb=0;
        fa2=fb2=0;
        for(j=0;j<channel;++j)
        {
            b=ovec+j;
            fa+=scov->a*b->a-scov->b*b->b;
            fb+=scov->a*b->b+scov->b*b->a;
            ++scov;

            fa2+=ncov->a*b->a-ncov->b*b->b;
            fb2+=ncov->a*b->b+ncov->b*b->a;
            ++ncov;
        }

        c=ovec+i;
        f1+=fa*c->a+fb*c->b;
        f2+=fa2*c->a+fb2*c->b;
    }

    f=prob * f1/(f1+f2);

    return f;
}

void wtk_maskzdsspec_flush_spec_k2(wtk_maskzdsspec_t *maskzdsspec, wtk_complex_t *scov,wtk_complex_t *ncov, float prob, int k, int ang_idx, float *spec)
{
    wtk_complex_t *b,*c;
    float fa,fb,fa2,fb2,f1,f2,f;
    int channel=maskzdsspec->channel;
    wtk_complex_t *ovec;
    int i,j,n;
    wtk_complex_t *scov1, *ncov1;

    for(n=0;n<ang_idx;++n,++spec){
        ovec=maskzdsspec->ovec[n][k];
        f1=f2=0;
        scov1=scov;
        ncov1=ncov;
        for(i=0;i<channel;++i)
        {
            fa=fb=0;
            fa2=fb2=0;
            for(j=0;j<channel;++j)
            {
                b=ovec+j;
                fa+=scov1->a*b->a-scov1->b*b->b;
                fb+=scov1->a*b->b+scov1->b*b->a;
                ++scov1;

                fa2+=ncov1->a*b->a-ncov1->b*b->b;
                fb2+=ncov1->a*b->b+ncov1->b*b->a;
                ++ncov1;
            }

            c=ovec+i;
            f1+=fa*c->a+fb*c->b;
            f2+=fa2*c->a+fb2*c->b;
        }

        f=prob * f1/(f1+f2);
        *spec=f;
    }
}

