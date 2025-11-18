#include "wtk_zdswspec.h"

wtk_zdswspec_t *wtk_zdswspec_new(int nbin,int channel, int ang_num)
{
    wtk_zdswspec_t *zdswspec;

    zdswspec=(wtk_zdswspec_t *)wtk_malloc(sizeof(wtk_zdswspec_t));
    zdswspec->nbin=nbin;
    zdswspec->channel=channel;
    zdswspec->ang_num=ang_num;
    zdswspec->ovec=wtk_complex_new_p3(ang_num,nbin,channel);
    zdswspec->tmp=(wtk_complex_t *)wtk_malloc(channel*sizeof(wtk_complex_t));
    zdswspec->B=wtk_float_new_p2(ang_num,nbin);
    zdswspec->LD=wtk_float_new_p2(ang_num,nbin);

    return zdswspec;
}

void wtk_zdswspec_reset(wtk_zdswspec_t *zdswspec)
{
    wtk_complex_zero_p3(zdswspec->ovec,zdswspec->ang_num,zdswspec->nbin,zdswspec->channel);
}

void wtk_zdswspec_delete(wtk_zdswspec_t *zdswspec)
{
    wtk_complex_delete_p3(zdswspec->ovec,zdswspec->ang_num,zdswspec->nbin);
    wtk_float_delete_p2(zdswspec->B,zdswspec->ang_num);
    wtk_float_delete_p2(zdswspec->LD,zdswspec->ang_num);
    wtk_free(zdswspec->tmp);
    wtk_free(zdswspec);
}


void wtk_zdswspec_flush_ovec(wtk_zdswspec_t *zdswspec,  wtk_complex_t *ovec, float **mic_pos, float sv, int rate, 
                                                        int theta2, int phi2, int k, float *tdoa)
{
	float x,y,z;
	float t;
	float *mic;
	int j;
	int channel=zdswspec->channel;
	int win=(zdswspec->nbin-1)*2;
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

void wtk_zdswspec_flush_ovec2(wtk_zdswspec_t *zdswspec,  wtk_complex_t *ovec, float **mic_pos, float sv, int rate, 
                                                        float x,float y,float z, int k, float *tdoa)
{
	float t;
	float *mic;
	int j;
	int channel=zdswspec->channel;
	int win=(zdswspec->nbin-1)*2;
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

void wtk_zdswspec_start(wtk_zdswspec_t *zdswspec,  float **mic_pos, float sv, int rate, float theta, float phi, int ang_idx, int use_line, float ls_eye)
{
    wtk_complex_t *LL;
    float *B=zdswspec->B[ang_idx];
    float *LD=zdswspec->LD[ang_idx];
    float f, f1, f2, f3, d, ff;
    int channel=zdswspec->channel;
	int nbin=zdswspec->nbin;
    wtk_complex_t **ovec=zdswspec->ovec[ang_idx];
    wtk_complex_t *novec,*a,*b;
    wtk_complex_t *cov,*cov1,*cov2;
    int th,i,j,k;
    int theta2;
    wtk_dcomplex_t *tmp_inv;
    wtk_complex_t *tmp=zdswspec->tmp;
    float *tdoa;
    int max_th=use_line?180:359;
    int win=2*(nbin-1);

    tmp_inv=(wtk_dcomplex_t *)wtk_malloc(channel*(channel+1)*sizeof(wtk_dcomplex_t));
    tdoa=(float *)wtk_malloc(channel*sizeof(float));
    novec=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*channel);
    cov=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*channel*channel);
    LL=(wtk_complex_t *)wtk_malloc(channel*channel*sizeof(wtk_complex_t));

    theta2=theta;
    for(k=1;k<nbin-1;++k)
    {
        wtk_zdswspec_flush_ovec(zdswspec, ovec[k],mic_pos,sv,rate,theta2,0,k,tdoa);
        memset(cov,0,sizeof(wtk_complex_t)*channel*channel);
        for(th=0; th<=max_th; ++th)
        {
            wtk_zdswspec_flush_ovec(zdswspec, novec, mic_pos,sv,rate,th,0,k,tdoa);
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
        LD[k]=0;
        for(i=0;i<channel;++i)
        {
            LD[k]+=tmp[i].a*ovec[k][i].a + tmp[i].b*ovec[k][i].b;
        }
        memcpy(ovec[k], tmp, sizeof(wtk_complex_t)*channel);

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

        memset(tmp,0,channel*sizeof(wtk_complex_t));
        for(i=0;i<channel;++i)
        {
            for(j=0;j<channel;++j)
            {
                tmp[i].a+=LL[i*channel+j].a*ovec[k][j].a;
                tmp[i].b+=LL[i*channel+j].a*ovec[k][j].b;
            }
        }

        B[k]=0;
        for(i=0;i<channel;++i)
        {
            B[k]+=tmp[i].a*ovec[k][i].a + tmp[i].b*ovec[k][i].b;
        }
    }

    wtk_free(LL);
    wtk_free(tmp_inv);
    wtk_free(tdoa);
    wtk_free(novec);
    wtk_free(cov);
}

void wtk_zdswspec_start2(wtk_zdswspec_t *zdswspec,  float **mic_pos, float sv, int rate, float x, float y, float z, int ang_idx, int use_line,float ls_eye)
{
    wtk_complex_t *LL;
    float *B=zdswspec->B[ang_idx];
    float *LD=zdswspec->LD[ang_idx];
    float f, f1, f2, f3, d, ff;
    int channel=zdswspec->channel;
	int nbin=zdswspec->nbin;
    wtk_complex_t **ovec=zdswspec->ovec[ang_idx];
    wtk_complex_t *novec,*a,*b;
    wtk_complex_t *cov,*cov1,*cov2;
    int th,i,j,k;
    wtk_dcomplex_t *tmp_inv;
    wtk_complex_t *tmp=zdswspec->tmp;
    float *tdoa;
    int max_th=use_line?180:359;
    int win=2*(nbin-1);

    tmp_inv=(wtk_dcomplex_t *)wtk_malloc(channel*(channel+1)*sizeof(wtk_dcomplex_t));
    tdoa=(float *)wtk_malloc(channel*sizeof(float));
    novec=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*channel);
    cov=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*channel*channel);
    LL=(wtk_complex_t *)wtk_malloc(channel*channel*sizeof(wtk_complex_t));

    for(k=1;k<nbin-1;++k)
    {
        wtk_zdswspec_flush_ovec2(zdswspec, ovec[k],mic_pos,sv,rate,x,y,z,k,tdoa);
        memset(cov,0,sizeof(wtk_complex_t)*channel*channel);
        for(th=0; th<=max_th; ++th)
        {
            wtk_zdswspec_flush_ovec(zdswspec, novec, mic_pos,sv,rate,th,0,k,tdoa);
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
        LD[k]=0;
        for(i=0;i<channel;++i)
        {
            LD[k]+=tmp[i].a*ovec[k][i].a + tmp[i].b*ovec[k][i].b;
        }
        memcpy(ovec[k], tmp, sizeof(wtk_complex_t)*channel);


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
        memset(tmp,0,channel*sizeof(wtk_complex_t));
        for(i=0;i<channel;++i)
        {
            for(j=0;j<channel;++j)
            {
                tmp[i].a+=LL[i*channel+j].a*ovec[k][j].a;
                tmp[i].b+=LL[i*channel+j].a*ovec[k][j].b;
            }
        }
        B[k]=0;
        for(i=0;i<channel;++i)
        {
            B[k]+=tmp[i].a*ovec[k][i].a + tmp[i].b*ovec[k][i].b;
        }
    }

    wtk_free(LL);
    wtk_free(tmp_inv);
    wtk_free(tdoa);
    wtk_free(novec);
    wtk_free(cov);
}

float wtk_zdswspec_flush_spec_k(wtk_zdswspec_t *zdswspec, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx)
{
#ifdef USE_NEON
    wtk_complex_t *a_c,*b_c;
    float fa,fb,f;
    int channel=zdswspec->channel;
    wtk_complex_t *ovec=zdswspec->ovec[ang_idx][k];
    float B=zdswspec->B[ang_idx][k];
    float LD=zdswspec->LD[ang_idx][k];
    wtk_complex_t *tmp=zdswspec->tmp;
    int i,j;
    float spec;
    int channel_tmp;

    channel_tmp = (int)(channel >> 2) << 2;

    a_c = cov;
    float32_t *a = (float32_t *)cov;  // complex转化为float类型
    float32_t *b;

    float32x4_t a_a_1, a_b_1;
    float32x4_t b_a_1, b_b_1;
    float32x4_t c_a_1, c_b_1;
    float32x4_t ftmp_1,ftmp_2, fc_1;

    float32x4x2_t a_1;
    float32x4x2_t b_1;


    for(i=0;i<channel;++i)
    {
        fa=fb=0;
        b_c = ovec;
        b = (float32_t *)ovec;
        for(j=0;j<channel_tmp;j+=4, a_c+=4, b_c+=4)
        {
            a_1 = vld2q_f32(a);  // float32_t转化为floatx4x2_t，每次读取4个复数，即4*2矩阵
            a+=8;  // 因为float32_t实部虚部交错，每次读取4个复数指针偏移4*2
            b_1 = vld2q_f32(b);
            b+=8;

            a_a_1 = vmulq_f32(a_1.val[0], b_1.val[0]);  // 分别取出实部虚部做乘法
            a_b_1 = vmulq_f32(a_1.val[1], b_1.val[1]);
            b_a_1 = vmulq_f32(a_1.val[0], b_1.val[1]);
            b_b_1 = vmulq_f32(a_1.val[1], b_1.val[0]);

            c_a_1 = vsubq_f32(a_a_1, a_b_1);  // 减法
            c_b_1 = vaddq_f32(b_a_1, b_b_1);  // 加法
            fa += vgetq_lane_f32(c_a_1, 0) + vgetq_lane_f32(c_a_1, 1) + vgetq_lane_f32(c_a_1, 2) + vgetq_lane_f32(c_a_1, 3);  // 将计算完成的floatx4_t类型4个数据按索引分别取出
            fb += vgetq_lane_f32(c_b_1, 0) + vgetq_lane_f32(c_b_1, 1) + vgetq_lane_f32(c_b_1, 2) + vgetq_lane_f32(c_b_1, 3);
        }
        for(j=channel_tmp;j<channel;++j, ++a_c, ++b_c){
            fa+=a_c->a*b_c->a-a_c->b*b_c->b;
            fb+=a_c->a*b_c->b+a_c->b*b_c->a;
        }
        tmp[i].a=fa;
        tmp[i].b=fb;
    }

    fa = 0;
    a_c = ovec;
    b_c = tmp;
    a = (float32_t *)ovec;
    b = (float32_t *)tmp;
    for(i=0;i<channel_tmp;i+=4, a_c+=4, b_c+=4){
        a_1 = vld2q_f32(a);
        a+=8;
        b_1 = vld2q_f32(b);
        b+=8;
        ftmp_1 = vmulq_f32(a_1.val[0], b_1.val[0]);
        ftmp_2 = vmulq_f32(a_1.val[1], b_1.val[1]);

        fc_1 = vaddq_f32(ftmp_1, ftmp_2);  // 加法

        fa += vgetq_lane_f32(fc_1, 0) + vgetq_lane_f32(fc_1, 1) + vgetq_lane_f32(fc_1, 2) + vgetq_lane_f32(fc_1, 3);
    }
    for(j=channel_tmp;j<channel;++j, ++a_c, ++b_c){
        fa+=a_c->a*b_c->a+a_c->b*b_c->b;
    }

    f=fa;
    if(cov_travg-f<1e-6)
    {
        f=f/1e-6;
    }else
    {
        f=f/(cov_travg-f);
    }
    
    spec=(f-f*B-B)/(LD-f+LD*f);
#else
    wtk_complex_t *a,*b;
    float fa,fb,f;
    int channel=zdswspec->channel;
    wtk_complex_t *ovec=zdswspec->ovec[ang_idx][k];
    float B=zdswspec->B[ang_idx][k];
    float LD=zdswspec->LD[ang_idx][k];
    int i,j;
    float spec;
    float fc;
    wtk_complex_t *c;

    c=ovec;
    a=cov;
    fc=0;
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
        fc+=c->a*fa+c->b*fb;
        ++c;
    }
    f=fc;
    if((cov_travg-f)<1e-6)
    {
        f=f/1e-6;
    }else
    {
        f=f/(cov_travg-f);
    }
    spec=(f-f*B-B)/(LD-f+LD*f);
    // spec=f;
#endif
    return spec;
}
