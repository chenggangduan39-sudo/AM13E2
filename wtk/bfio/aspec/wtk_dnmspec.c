#include "wtk_dnmspec.h"



wtk_dnmspec_t *wtk_dnmspec_new(int nbin,int channel, int ang_num)
{
    wtk_dnmspec_t *dnmspec;

    dnmspec=(wtk_dnmspec_t *)wtk_malloc(sizeof(wtk_dnmspec_t));
    dnmspec->nbin=nbin;
    dnmspec->channel=channel;
    dnmspec->ang_num=ang_num;

    dnmspec->ARA=(wtk_complex_t *)wtk_malloc(channel*sizeof(wtk_complex_t));    
    dnmspec->LA=wtk_complex_new_p3(ang_num,nbin,2*channel);
    dnmspec->A=wtk_complex_new_p3(ang_num,nbin,channel*channel);

    dnmspec->tmp=(wtk_complex_t *)wtk_malloc(channel*channel*sizeof(wtk_complex_t));
  
    dnmspec->eig=wtk_eig_new(channel);
    
    return dnmspec;
}

void wtk_dnmspec_reset(wtk_dnmspec_t *dnmspec)
{
    wtk_complex_zero_p3(dnmspec->LA,dnmspec->ang_num,dnmspec->nbin,2*dnmspec->channel);
    wtk_complex_zero_p3(dnmspec->A,dnmspec->ang_num,dnmspec->nbin,dnmspec->channel*dnmspec->channel);
}

void wtk_dnmspec_delete(wtk_dnmspec_t *dnmspec)
{
    wtk_complex_delete_p3(dnmspec->LA,dnmspec->ang_num,dnmspec->nbin);
    wtk_complex_delete_p3(dnmspec->A,dnmspec->ang_num,dnmspec->nbin);

    wtk_free(dnmspec->ARA);
    wtk_free(dnmspec->tmp);
    wtk_eig_delete(dnmspec->eig);
    wtk_free(dnmspec);
}

void wtk_dnmspec_start(wtk_dnmspec_t *dnmspec, float **mic_pos, float sv, int rate, float theta, float phi, int ang_idx)
{
    int i,j,k,n;
    wtk_complex_t *LL,*LL_inv;
    wtk_complex_t *L1;
    wtk_complex_t *L2;
    int channel=dnmspec->channel;
    int nbin=dnmspec->nbin;
    float f,d,f1,f2,f3,ff;
    int win=(nbin-1)*2;
    wtk_complex_t *fvij;
    double *fv,*b,*c;
    wtk_complex_t *a;
    wtk_dcomplex_t *tmp_inv;
    wtk_complex_t *tmp,*tmp2,*tmp3;
    wtk_complex_t *alpha;
    int ret;
    wtk_complex_t *A1;
    wtk_complex_t *LA_;
    wtk_complex_t *LA1,*LA_tmp,*LA_tmp_inv;
    wtk_complex_t **A=dnmspec->A[ang_idx];
    wtk_complex_t **LA=dnmspec->LA[ang_idx];
    float x,y,z;
	float t;
	float *mic;
	float *tdoa;

    phi*=PI/180;
    theta*=PI/180;
    x=cos(phi)*cos(theta);
    y=cos(phi)*sin(theta);
    z=sin(phi);

    tdoa=(float *)wtk_malloc(channel*sizeof(float));    
    alpha=(wtk_complex_t *)wtk_malloc(channel*sizeof(wtk_complex_t));

    LL=(wtk_complex_t *)wtk_malloc(channel*channel*sizeof(wtk_complex_t));
    LL_inv=(wtk_complex_t *)wtk_malloc(channel*channel*sizeof(wtk_complex_t));
    L1=(wtk_complex_t *)wtk_malloc(channel*channel*sizeof(wtk_complex_t));
    L2=(wtk_complex_t *)wtk_malloc(channel*channel*sizeof(wtk_complex_t));
    tmp=(wtk_complex_t *)wtk_malloc(channel*sizeof(wtk_complex_t));
    a=(wtk_complex_t *)wtk_malloc(channel*channel*sizeof(wtk_complex_t));
    b=wtk_malloc(sizeof(double)*channel*2);
    c=wtk_malloc(sizeof(double)*channel*2);
    fv=wtk_malloc(sizeof(double)*channel*channel*4);
    fvij=(wtk_complex_t *)wtk_malloc(channel*channel*sizeof(wtk_complex_t));
    tmp_inv=(wtk_dcomplex_t *)wtk_malloc(channel*channel*2*sizeof(wtk_dcomplex_t));
    tmp2=(wtk_complex_t *)wtk_malloc(channel*sizeof(wtk_complex_t));
    tmp3=(wtk_complex_t *)wtk_malloc(channel*channel*sizeof(wtk_complex_t));
    LA_=(wtk_complex_t *)wtk_malloc(channel*2*sizeof(wtk_complex_t));
    LA_tmp=(wtk_complex_t *)wtk_malloc(2*2*sizeof(wtk_complex_t));
    LA_tmp_inv=(wtk_complex_t *)wtk_malloc(2*2*sizeof(wtk_complex_t));


    for(i=0;i<channel;++i)
	{
		mic=mic_pos[i];
		tdoa[i]=(mic[0]*x+mic[1]*y+mic[2]*z)/sv;
	}

    for(k=1;k<nbin-1;++k)
    {
		t=2*PI*rate*1.0/win*k;
		for(i=0;i<channel;++i)
		{
			alpha[i].a=cos(t*tdoa[i]);
			alpha[i].b=sin(t*tdoa[i]);
		}

        f=rate*1.0/win*k;
        for(i=0;i<channel;++i)
        {
            LL[i*channel+i].a=1;
            LL[i*channel+i].b=0;
            for(j=i+1;j<channel;++j)
            {
                f1=mic_pos[i][0]-mic_pos[j][0];
                f2=mic_pos[i][1]-mic_pos[j][1];
                f3=mic_pos[i][2]-mic_pos[j][2];
                d=sqrt(f1*f1+f2*f2+f3*f3);
                ff=2*PI*(d/sv)*f;
                LL[i*channel+j].a=sin(ff)/ff;
                LL[j*channel+i].a=LL[i*channel+j].a;
                LL[i*channel+j].b=LL[j*channel+i].b=0;
            }
        }
        // [vac,~]=eig(alpha(:,k)*alpha(:,k)'*inv(LL(:,:,k)));
        ret=wtk_complex_invx4(LL,tmp_inv,channel,LL_inv,1);
        if(ret!=0)
		{
            j=0;
            for(i=0;i<channel;++i)
            {
                LL[j].a+=0.01;
                j+=channel+1;
            }
            wtk_complex_invx4(LL,tmp_inv,channel,LL_inv,1);
		}

        memset(tmp,0,channel*sizeof(wtk_complex_t));
        for(i=0;i<channel;++i)
        {
            for(j=0;j<channel;++j)
            {
                tmp[j].a+=LL_inv[i*channel+j].a*alpha[i].a;
                tmp[j].b+=-LL_inv[i*channel+j].a*alpha[i].b;
            }
        }
        for(i=0;i<channel;++i)
        {
            for(j=0;j<channel;++j)
            {
                a[i*channel+j].a=alpha[i].a*tmp[j].a-alpha[i].b*tmp[j].b;
                a[i*channel+j].b=alpha[i].a*tmp[j].b+alpha[i].b*tmp[j].a;
            }
        }

        // printf("=======%d=======\n",k);
        // wtk_complex_print2(a,channel,channel);

        // if(0)
        // {
        //     wtk_eig_matlab(a,fvij,channel);
        // }else
        {
            wtk_eig_process2_matlab(dnmspec->eig,a,fvij);
        }

        // wtk_complex_print2(fvij,channel,channel);
        // for(i=0;i<channel*2;++i)
        // {
        //     printf("%f ",b[i]);
        // }
        // printf("\n");

        A1=A[k];
        ret=wtk_complex_invx4(fvij,tmp_inv,channel,A1,0);
		if(ret!=0)
		{
            j=0;
            for(i=0;i<channel;++i)
            {
                fvij[j].a+=0.01;
                j+=channel+1;
            }
            wtk_complex_invx4(fvij,tmp_inv,channel,A1,0);
		}

        // wtk_complex_print2(A1,channel,channel);

        //  LA_=[diag(L1(:,:,k)),diag(L2(:,:,k))];
        memset(LA_,0,channel*2*sizeof(wtk_complex_t));
        //diag( A(:,:,k)*alpha(:,k)*alpha(:,k)'*A(:,:,k)');
        memset(tmp,0,channel*sizeof(wtk_complex_t));
        memset(tmp2,0,channel*sizeof(wtk_complex_t));
        for(i=0;i<channel;++i)
        {
            for(j=0;j<channel;++j)
            {
                //alpha(:,k)'*A(:,:,k)'
                tmp[i].a+=alpha[j].a*A1[i*channel+j].a-alpha[j].b*A1[i*channel+j].b;
                tmp[i].b+=-(alpha[j].a*A1[i*channel+j].b+alpha[j].b*A1[i*channel+j].a);

                // A(:,:,k)*alpha(:,k)
                tmp2[i].a+=A1[i*channel+j].a*alpha[j].a-A1[i*channel+j].b*alpha[j].b;
                tmp2[i].b+=A1[i*channel+j].a*alpha[j].b+A1[i*channel+j].b*alpha[j].a;
            }
        }
        for(i=0;i<channel;++i)
        {
            LA_[i*2].a=tmp2[i].a*tmp[i].a-tmp2[i].b*tmp[i].b;
            LA_[i*2].b=0;//tmp2[i].a*tmp[i].b+tmp2[i].b*tmp[i].a;
        }
        // diag(A(:,:,k)*LL(:,:,k)*A(:,:,k)'))
        memset(tmp3,0,sizeof(wtk_complex_t)*channel*channel);
        for(i=0;i<channel;++i)
        {
            for(j=0;j<channel;++j)
            {
                for(n=0;n<channel;++n)
                {
                    tmp3[i*channel+n].a+=A1[i*channel+j].a*LL[j*channel+n].a;//-A1[i*channel+j].b*LL[j*channel+n].b;
                    tmp3[i*channel+n].b+=A1[i*channel+j].b*LL[j*channel+n].a;//+A1[i*channel+j].a*LL[j*channel+n].b;
                }
            }
        }
        for(i=0;i<channel;++i)
        {
            for(j=0;j<channel;++j)
            {
                LA_[i*2+1].a+=tmp3[i*channel+j].a*A1[i*channel+j].a+tmp3[i*channel+j].b*A1[i*channel+j].b;
                // LA_[i*2+1].b+=-tmp3[i*channel+j].a*A1[i*channel+j].b+tmp3[i*channel+j].b*A1[i*channel+j].a;
            }
            LA_[i*2+1].b=0;
        }
        //wtk_complex_print2(LA_,channel,2);
        // LA(:,:,k)=inv(LA_'*LA_)*LA_';
        memset(LA_tmp,0,sizeof(wtk_complex_t)*2*2);
        for(i=0;i<2;++i)
        {
            for(j=0;j<channel;++j)
            {
                for(n=0;n<2;++n)
                {
                    LA_tmp[i*2+n].a+=LA_[j*2+i].a*LA_[j*2+n].a;//+LA_[j*2+i].b*LA_[j*2+n].b;
                    // LA_tmp[i*2+n].b+=LA_[j*2+i].a*LA_[j*2+n].b-LA_[j*2+i].b*LA_[j*2+n].a;
                    LA_tmp[i*2+n].b=0;
                }
            }
        }
        //wtk_complex_print2(LA_tmp,2,2);

        ret=wtk_complex_invx4(LA_tmp,tmp_inv,2,LA_tmp_inv,1);
		if(ret!=0)
		{
            j=0;
            for(i=0;i<2;++i)
            {
                LA_tmp[j].a+=0.01;
                j+=3;
            }
            wtk_complex_invx4(LA_tmp,tmp_inv,2,LA_tmp_inv,1);
		}
        //wtk_complex_print2(LA_tmp_inv,2,2);

        LA1=LA[k];
        memset(LA1,0,sizeof(wtk_complex_t)*2*channel);
        for(i=0;i<2;++i)
        {
            for(j=0;j<2;++j)
            {
                for(n=0;n<channel;++n)
                {
                    LA1[i*channel+n].a+=LA_tmp_inv[i*2+j].a*LA_[n*2+j].a;//+LA_tmp_inv[i*2+j].b*LA_[n*2+j].b;
                    // LA1[i*channel+n].b+=-LA_tmp_inv[i*2+j].a*LA_[n*2+j].b;//+LA_tmp_inv[i*2+j].b*LA_[n*2+j].a;
                    LA1[i*channel+n].b=0;
                }
            }
        }
        // wtk_complex_print2(LA1,2,channel);
    }

	wtk_free(tdoa);
    wtk_free(alpha);
    wtk_free(LL);
    wtk_free(LL_inv);
    wtk_free(L1);
    wtk_free(L2);
    wtk_free(tmp);
    wtk_free(a);
    wtk_free(b);
    wtk_free(c);
    wtk_free(fv);
    wtk_free(fvij);
    wtk_free(tmp_inv);
    wtk_free(tmp2);
    wtk_free(tmp3);
    wtk_free(LA_);
    wtk_free(LA_tmp);
    wtk_free(LA_tmp_inv);
}



void wtk_dnmspec_start2(wtk_dnmspec_t *dnmspec, float **mic_pos, float sv, int rate, float x, float y, float z, int ang_idx)
{
    int i,j,k,n;
    wtk_complex_t *LL,*LL_inv;
    wtk_complex_t *L1;
    wtk_complex_t *L2;
    int channel=dnmspec->channel;
    int nbin=dnmspec->nbin;
    float f,d,f1,f2,f3,ff;
    int win=(nbin-1)*2;
    wtk_complex_t *fvij;
    double *fv,*b,*c;
    wtk_complex_t *a;
    wtk_dcomplex_t *tmp_inv;
    wtk_complex_t *tmp,*tmp2,*tmp3;
    wtk_complex_t *alpha;
    int ret;
    wtk_complex_t *A1;
    wtk_complex_t *LA_;
    wtk_complex_t *LA1,*LA_tmp,*LA_tmp_inv;
    wtk_complex_t **A=dnmspec->A[ang_idx];
    wtk_complex_t **LA=dnmspec->LA[ang_idx];
	float t;
	float *mic;
	float *tdoa;

    tdoa=(float *)wtk_malloc(channel*sizeof(float));    
    alpha=(wtk_complex_t *)wtk_malloc(channel*sizeof(wtk_complex_t));

    LL=(wtk_complex_t *)wtk_malloc(channel*channel*sizeof(wtk_complex_t));
    LL_inv=(wtk_complex_t *)wtk_malloc(channel*channel*sizeof(wtk_complex_t));
    L1=(wtk_complex_t *)wtk_malloc(channel*channel*sizeof(wtk_complex_t));
    L2=(wtk_complex_t *)wtk_malloc(channel*channel*sizeof(wtk_complex_t));
    tmp=(wtk_complex_t *)wtk_malloc(channel*sizeof(wtk_complex_t));
    a=(wtk_complex_t *)wtk_malloc(channel*channel*sizeof(wtk_complex_t));
    b=wtk_malloc(sizeof(double)*channel*2);
    c=wtk_malloc(sizeof(double)*channel*2);
    fv=wtk_malloc(sizeof(double)*channel*channel*4);
    fvij=(wtk_complex_t *)wtk_malloc(channel*channel*sizeof(wtk_complex_t));
    tmp_inv=(wtk_dcomplex_t *)wtk_malloc(channel*channel*2*sizeof(wtk_dcomplex_t));
    tmp2=(wtk_complex_t *)wtk_malloc(channel*sizeof(wtk_complex_t));
    tmp3=(wtk_complex_t *)wtk_malloc(channel*channel*sizeof(wtk_complex_t));
    LA_=(wtk_complex_t *)wtk_malloc(channel*2*sizeof(wtk_complex_t));
    LA_tmp=(wtk_complex_t *)wtk_malloc(2*2*sizeof(wtk_complex_t));
    LA_tmp_inv=(wtk_complex_t *)wtk_malloc(2*2*sizeof(wtk_complex_t));

    for(i=0;i<channel;++i)
    {
        mic=mic_pos[i];
        tdoa[i]=sqrt((mic[0]-x)*(mic[0]-x)+(mic[1]-y)*(mic[1]-y)+(mic[2]-z)*(mic[2]-z))/sv;
        if(i>0)
        {
            tdoa[i]-=tdoa[0];
        }
    }
    tdoa[0]=0;

    for(k=1;k<nbin-1;++k)
    {
		t=2*PI*rate*1.0/win*k;
		for(i=0;i<channel;++i)
		{
			alpha[i].a=cos(t*tdoa[i]);
			alpha[i].b=-sin(t*tdoa[i]);
		}

        f=rate*1.0/win*k;
        for(i=0;i<channel;++i)
        {
            LL[i*channel+i].a=1;
            LL[i*channel+i].b=0;
            for(j=i+1;j<channel;++j)
            {
                f1=mic_pos[i][0]-mic_pos[j][0];
                f2=mic_pos[i][1]-mic_pos[j][1];
                f3=mic_pos[i][2]-mic_pos[j][2];
                d=sqrt(f1*f1+f2*f2+f3*f3);
                ff=2*PI*(d/sv)*f;
                LL[i*channel+j].a=sin(ff)/ff;
                LL[j*channel+i].a=LL[i*channel+j].a;
                LL[i*channel+j].b=LL[j*channel+i].b=0;
            }
        }
        // [vac,~]=eig(alpha(:,k)*alpha(:,k)'*inv(LL(:,:,k)));
        ret=wtk_complex_invx4(LL,tmp_inv,channel,LL_inv,1);
        if(ret!=0)
		{
            j=0;
            for(i=0;i<channel;++i)
            {
                LL[j].a+=0.01;
                j+=channel+1;
            }
            wtk_complex_invx4(LL,tmp_inv,channel,LL_inv,1);
		}

        memset(tmp,0,channel*sizeof(wtk_complex_t));
        for(i=0;i<channel;++i)
        {
            for(j=0;j<channel;++j)
            {
                tmp[j].a+=LL_inv[i*channel+j].a*alpha[i].a;
                tmp[j].b+=-LL_inv[i*channel+j].a*alpha[i].b;
            }
        }
        for(i=0;i<channel;++i)
        {
            for(j=0;j<channel;++j)
            {
                a[i*channel+j].a=alpha[i].a*tmp[j].a-alpha[i].b*tmp[j].b;
                a[i*channel+j].b=alpha[i].a*tmp[j].b+alpha[i].b*tmp[j].a;
            }
        }

        // printf("=======%d=======\n",k);
        // wtk_complex_print2(a,channel,channel);

        // if(0)
        // {
        //     wtk_eig_matlab(a,fvij,channel);
        // }else
        {
            wtk_eig_process2_matlab(dnmspec->eig,a,fvij);
        }

        // wtk_complex_print2(fvij,channel,channel);
        // for(i=0;i<channel*2;++i)
        // {
        //     printf("%f ",b[i]);
        // }
        // printf("\n");

        A1=A[k];
        ret=wtk_complex_invx4(fvij,tmp_inv,channel,A1,0);
		if(ret!=0)
		{
            j=0;
            for(i=0;i<channel;++i)
            {
                fvij[j].a+=0.01;
                j+=channel+1;
            }
            wtk_complex_invx4(fvij,tmp_inv,channel,A1,0);
		}

        // wtk_complex_print2(A1,channel,channel);

        //  LA_=[diag(L1(:,:,k)),diag(L2(:,:,k))];
        memset(LA_,0,channel*2*sizeof(wtk_complex_t));
        //diag( A(:,:,k)*alpha(:,k)*alpha(:,k)'*A(:,:,k)');
        memset(tmp,0,channel*sizeof(wtk_complex_t));
        memset(tmp2,0,channel*sizeof(wtk_complex_t));
        for(i=0;i<channel;++i)
        {
            for(j=0;j<channel;++j)
            {
                //alpha(:,k)'*A(:,:,k)'
                tmp[i].a+=alpha[j].a*A1[i*channel+j].a-alpha[j].b*A1[i*channel+j].b;
                tmp[i].b+=-(alpha[j].a*A1[i*channel+j].b+alpha[j].b*A1[i*channel+j].a);

                // A(:,:,k)*alpha(:,k)
                tmp2[i].a+=A1[i*channel+j].a*alpha[j].a-A1[i*channel+j].b*alpha[j].b;
                tmp2[i].b+=A1[i*channel+j].a*alpha[j].b+A1[i*channel+j].b*alpha[j].a;
            }
        }
        for(i=0;i<channel;++i)
        {
            LA_[i*2].a=tmp2[i].a*tmp[i].a-tmp2[i].b*tmp[i].b;
            LA_[i*2].b=0;//tmp2[i].a*tmp[i].b+tmp2[i].b*tmp[i].a;
        }
        // diag(A(:,:,k)*LL(:,:,k)*A(:,:,k)'))
        memset(tmp3,0,sizeof(wtk_complex_t)*channel*channel);
        for(i=0;i<channel;++i)
        {
            for(j=0;j<channel;++j)
            {
                for(n=0;n<channel;++n)
                {
                    tmp3[i*channel+n].a+=A1[i*channel+j].a*LL[j*channel+n].a;//-A1[i*channel+j].b*LL[j*channel+n].b;
                    tmp3[i*channel+n].b+=A1[i*channel+j].b*LL[j*channel+n].a;//+A1[i*channel+j].a*LL[j*channel+n].b;
                }
            }
        }
        for(i=0;i<channel;++i)
        {
            for(j=0;j<channel;++j)
            {
                LA_[i*2+1].a+=tmp3[i*channel+j].a*A1[i*channel+j].a+tmp3[i*channel+j].b*A1[i*channel+j].b;
                // LA_[i*2+1].b+=-tmp3[i*channel+j].a*A1[i*channel+j].b+tmp3[i*channel+j].b*A1[i*channel+j].a;
            }
            LA_[i*2+1].b=0;
        }
        //wtk_complex_print2(LA_,channel,2);
        // LA(:,:,k)=inv(LA_'*LA_)*LA_';
        memset(LA_tmp,0,sizeof(wtk_complex_t)*2*2);
        for(i=0;i<2;++i)
        {
            for(j=0;j<channel;++j)
            {
                for(n=0;n<2;++n)
                {
                    LA_tmp[i*2+n].a+=LA_[j*2+i].a*LA_[j*2+n].a;//+LA_[j*2+i].b*LA_[j*2+n].b;
                    // LA_tmp[i*2+n].b+=LA_[j*2+i].a*LA_[j*2+n].b-LA_[j*2+i].b*LA_[j*2+n].a;
                    LA_tmp[i*2+n].b=0;
                }
            }
        }
        //wtk_complex_print2(LA_tmp,2,2);

        ret=wtk_complex_invx4(LA_tmp,tmp_inv,2,LA_tmp_inv,1);
		if(ret!=0)
		{
            j=0;
            for(i=0;i<2;++i)
            {
                LA_tmp[j].a+=0.01;
                j+=3;
            }
            wtk_complex_invx4(LA_tmp,tmp_inv,2,LA_tmp_inv,1);
		}
        //wtk_complex_print2(LA_tmp_inv,2,2);

        LA1=LA[k];
        memset(LA1,0,sizeof(wtk_complex_t)*2*channel);
        for(i=0;i<2;++i)
        {
            for(j=0;j<2;++j)
            {
                for(n=0;n<channel;++n)
                {
                    LA1[i*channel+n].a+=LA_tmp_inv[i*2+j].a*LA_[n*2+j].a;//+LA_tmp_inv[i*2+j].b*LA_[n*2+j].b;
                    // LA1[i*channel+n].b+=-LA_tmp_inv[i*2+j].a*LA_[n*2+j].b;//+LA_tmp_inv[i*2+j].b*LA_[n*2+j].a;
                    LA1[i*channel+n].b=0;
                }
            }
        }
        // wtk_complex_print2(LA1,2,channel);
    }

	wtk_free(tdoa);
    wtk_free(alpha);
    wtk_free(LL);
    wtk_free(LL_inv);
    wtk_free(L1);
    wtk_free(L2);
    wtk_free(tmp);
    wtk_free(a);
    wtk_free(b);
    wtk_free(c);
    wtk_free(fv);
    wtk_free(fvij);
    wtk_free(tmp_inv);
    wtk_free(tmp2);
    wtk_free(tmp3);
    wtk_free(LA_);
    wtk_free(LA_tmp);
    wtk_free(LA_tmp_inv);
}

float wtk_dnmspec_flush_spec_k(wtk_dnmspec_t *dnmspec, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx)
{
    int channel=dnmspec->channel;
    wtk_complex_t *tmp=dnmspec->tmp;
    wtk_complex_t *ARA=dnmspec->ARA;
    int i,j,n;
    float spec;
    wtk_complex_t vs,vd;
    wtk_complex_t *A=dnmspec->A[ang_idx][k];
    wtk_complex_t *LA=dnmspec->LA[ang_idx][k];

    // wtk_complex_print2(LA,2,channel);
    // wtk_complex_print2(A,channel,channel);
    // wtk_complex_print2(cov,channel,channel);

    // ARA=diag(A(:,:,k)*xx*A(:,:,k)');
    memset(tmp,0,sizeof(wtk_complex_t)*channel*channel);
    memset(ARA,0,sizeof(wtk_complex_t)*channel);
    for(i=0;i<channel;++i)
    {
        for(j=0;j<channel;++j)
        {
            for(n=0;n<channel;++n)
            {
                tmp[i*channel+n].a+=A[i*channel+j].a*cov[j*channel+n].a-A[i*channel+j].b*cov[j*channel+n].b;
                tmp[i*channel+n].b+=A[i*channel+j].a*cov[j*channel+n].b+A[i*channel+j].b*cov[j*channel+n].a;
            }
        }
    }
    for(i=0;i<channel;++i)
    {
        for(j=0;j<channel;++j)
        {
            ARA[i].a+=tmp[i*channel+j].a*A[i*channel+j].a+tmp[i*channel+j].b*A[i*channel+j].b;
            // ARA[i].b+=-tmp[i*channel+j].a*A[i*channel+j].b+tmp[i*channel+j].b*A[i*channel+j].a;
        }
        ARA[i].b=0;
    }

    // wtk_complex_print(ARA,channel);

    //vv=LA(:,:,k)*ARA;
    vs.a=vd.a=vs.b=vd.b=0;
    for(j=0;j<channel;++j)
    {
        vs.a+=LA[j].a*ARA[j].a;//-LA[j].b*ARA[j].b;
        // vs.b+=LA[j].a*ARA[j].b;//+LA[j].b*ARA[j].a;
        
        vd.a+=LA[channel+j].a*ARA[j].a;//-LA[channel+j].b*ARA[j].b;
        // vd.b+=LA[channel+j].a*ARA[j].b;//+LA[channel+j].b*ARA[j].a;
    }

    // wtk_debug("%f+i%f  %f+%fi\n",vs.a,vs.b,vd.a,vd.b);

    // spec=sqrt(vs.a*vs.a+vs.b*vs.b)/sqrt(vd.a*vd.a+vd.b*vd.b);
    spec=abs(vs.a/vd.a);

    return spec;
}
