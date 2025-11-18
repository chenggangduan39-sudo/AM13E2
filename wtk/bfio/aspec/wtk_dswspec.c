#include "wtk_dswspec.h"

wtk_dswspec_t *wtk_dswspec_new(int nbin,int channel, int ang_num)
{
    wtk_dswspec_t *dswspec;

    dswspec=(wtk_dswspec_t *)wtk_malloc(sizeof(wtk_dswspec_t));
    dswspec->nbin=nbin;
    dswspec->channel=channel;
    dswspec->ang_num=ang_num;
    dswspec->ovec=wtk_complex_new_p3(ang_num,nbin,channel);
    dswspec->tmp=(wtk_complex_t *)wtk_malloc(channel*sizeof(wtk_complex_t));
    dswspec->B=wtk_float_new_p2(ang_num,nbin);

    return dswspec;
}

void wtk_dswspec_reset(wtk_dswspec_t *dswspec)
{
    wtk_complex_zero_p3(dswspec->ovec,dswspec->ang_num,dswspec->nbin,dswspec->channel);
}

void wtk_dswspec_delete(wtk_dswspec_t *dswspec)
{
    wtk_complex_delete_p3(dswspec->ovec,dswspec->ang_num,dswspec->nbin);
    wtk_float_delete_p2(dswspec->B,dswspec->ang_num);
    wtk_free(dswspec->tmp);
    wtk_free(dswspec);
}

void wtk_dswspec_start(wtk_dswspec_t *dswspec,  float **mic_pos, float sv, int rate, float theta, float phi, int ang_idx)
{
    wtk_complex_t *LL, *tmp;
    wtk_dcomplex_t *tmp_inv;
    int channel=dswspec->channel;
    float *B=dswspec->B[ang_idx];
    wtk_complex_t **ovec=dswspec->ovec[ang_idx], *alpha;
    int nbin=dswspec->nbin;
    int k, i, j;
    int win=2*(nbin-1);
    float f, f1, f2, f3, d, ff;

    float x,y,z;
	float t;
	float *mic;
	float *tdoa;

    LL=(wtk_complex_t *)wtk_malloc(channel*channel*sizeof(wtk_complex_t));
    tmp_inv=(wtk_dcomplex_t *)wtk_malloc(channel*channel*2*sizeof(wtk_dcomplex_t));
    tmp=(wtk_complex_t *)wtk_malloc(channel*sizeof(wtk_complex_t));
    tdoa=(float *)wtk_malloc(channel*sizeof(float));

    phi*=PI/180;
    theta*=PI/180;
    x=cos(phi)*cos(theta);
    y=cos(phi)*sin(theta);
    z=sin(phi);

	for(i=0;i<channel;++i)
	{
		mic=mic_pos[i];
		tdoa[i]=(mic[0]*x+mic[1]*y+mic[2]*z)/sv;
	}

    for(k=1;k<nbin-1;++k)
    {
		alpha=ovec[k];
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

        memset(tmp,0,channel*sizeof(wtk_complex_t));
        for(i=0;i<channel;++i)
        {
            for(j=0;j<channel;++j)
            {
                tmp[i].a+=LL[i*channel+j].a*alpha[j].a;
                tmp[i].b+=LL[i*channel+j].a*alpha[j].b;
            }
        }

        B[k]=0;
        for(i=0;i<channel;++i)
        {
            B[k]+=tmp[i].a*alpha[i].a + tmp[i].b*alpha[i].b;
        }
    }

    wtk_free(LL);
    wtk_free(tmp_inv);
    wtk_free(tmp);
    wtk_free(tdoa);
}

void wtk_dswspec_start2(wtk_dswspec_t *dswspec,  float **mic_pos, float sv, int rate, float x, float y, float z, int ang_idx)
{
    wtk_complex_t *LL, *tmp;
    wtk_dcomplex_t *tmp_inv;
    int channel=dswspec->channel;
    float *B=dswspec->B[ang_idx];
    wtk_complex_t **ovec=dswspec->ovec[ang_idx], *alpha;
    int nbin=dswspec->nbin;
    int k, i, j;
    int win=2*(nbin-1);
    float f, f1, f2, f3, d, ff;

	float t;
	float *mic;
	float *tdoa;

    LL=(wtk_complex_t *)wtk_malloc(channel*channel*sizeof(wtk_complex_t));
    tmp_inv=(wtk_dcomplex_t *)wtk_malloc(channel*channel*2*sizeof(wtk_dcomplex_t));
    tmp=(wtk_complex_t *)wtk_malloc(channel*sizeof(wtk_complex_t));
    tdoa=(float *)wtk_malloc(channel*sizeof(float));

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
		alpha=ovec[k];
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

        memset(tmp,0,channel*sizeof(wtk_complex_t));
        for(i=0;i<channel;++i)
        {
            for(j=0;j<channel;++j)
            {
                tmp[i].a+=LL[i*channel+j].a*alpha[j].a;
                tmp[i].b+=LL[i*channel+j].a*alpha[j].b;
            }
        }

        B[k]=0;
        for(i=0;i<channel;++i)
        {
            B[k]+=tmp[i].a*alpha[i].a + tmp[i].b*alpha[i].b;
        }
    }

    wtk_free(LL);
    wtk_free(tmp_inv);
    wtk_free(tmp);
    wtk_free(tdoa);
}

float wtk_dswspec_flush_spec_k(wtk_dswspec_t *dswspec, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx)
{

/////////////////////////////////////////////
#ifdef USE_NEON
    wtk_complex_t *a_c,*b_c;
    float fa,fb,f;
    int channel=dswspec->channel;
    int channelx2=channel*channel;
    wtk_complex_t *ovec=dswspec->ovec[ang_idx][k];
    float B=dswspec->B[ang_idx][k];
    wtk_complex_t *tmp=dswspec->tmp;
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
        for(j=0;j<channel_tmp;j+=4,a_c+=4,b_c+=4)
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
        for(j=channel_tmp;j<channel;++j,++a_c,++b_c,a+=2,b+=2){
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
    for(i=0;i<channel_tmp;i+=4,a_c+=4,b_c+=4){
        a_1 = vld2q_f32(a);
        a+=8;
        b_1 = vld2q_f32(b);
        b+=8;
        ftmp_1 = vmulq_f32(a_1.val[0], b_1.val[0]);
        ftmp_2 = vmulq_f32(a_1.val[1], b_1.val[1]);

        fc_1 = vaddq_f32(ftmp_1, ftmp_2);  // 加法

        fa += vgetq_lane_f32(fc_1, 0) + vgetq_lane_f32(fc_1, 1) + vgetq_lane_f32(fc_1, 2) + vgetq_lane_f32(fc_1, 3);
    }
    for(j=channel_tmp;j<channel;++j,++a_c,++b_c,a+=2,b+=2){
        fa+=a_c->a*b_c->a+a_c->b*b_c->b;
    }

    f=fa/channelx2;
    if(cov_travg-f<1e-6)
    {
        f=f/1e-6;
    }else
    {
        f=f/(cov_travg-f);
    }
    
    spec=((channelx2-B)*f-B)/channelx2;
#else
///////////////////////////////////////////////
    wtk_complex_t *a,*b;
    float fa,fb,f;
    int channel=dswspec->channel;
    int channelx2=channel*channel;
    wtk_complex_t *ovec=dswspec->ovec[ang_idx][k];
    float B=dswspec->B[ang_idx][k];
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
    f=fc/channelx2;
    if(cov_travg-f<1e-6)
    {
        f=f/1e-6;
    }else
    {
        f=f/(cov_travg-f);
    }
    
    spec=((channelx2-B)*f-B)/channelx2;
#endif
/////////////////////////////////////////////
    // wtk_complex_t *a,*b;
    // float fa,fb,f;
    // int channel=dswspec->channel;
    // int channelx2=channel*channel;
    // wtk_complex_t *ovec=dswspec->ovec[ang_idx][k];
    // float B=dswspec->B[ang_idx][k];
    // wtk_complex_t *tmp=dswspec->tmp;
    // int i,j;
    // float spec;
    // a=cov;
    // for(i=0;i<channel;++i)
    // {
    //     fa=fb=0;
    //     for(j=0;j<channel;++j)
    //     {
    //         b=ovec+j;
    //         fa+=a->a*b->a-a->b*b->b;
    //         fb+=a->a*b->b+a->b*b->a;
    //         ++a;
    //     }
    //     tmp[i].a=fa;
    //     tmp[i].b=fb;

    // }
    // // fa=fb=0;
    // fa=0;
    // for(i=0;i<channel;++i)
    // {
    //     a=ovec+i;
    //     b=tmp+i;
    //     fa+=a->a*b->a+a->b*b->b;
    //     // fb+=a->a*b->b-a->b*b->a;
    // }
    // // f=sqrt(fa*fa+fb*fb)/channelx2;
    // f=fa/channelx2;
    // f=f/(cov_travg-f);
    
    // spec=((channelx2-B)*f-B)/channelx2;
///////////////////////////////////////////////
    return spec;
}


void wtk_dswspec_flush_spec_k2(wtk_dswspec_t *dswspec, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx, float *spec)
{

/////////////////////////////////////////////
#ifdef USE_NEON
    wtk_complex_t *a_c,*b_c;
    float fa,fb,f;
    wtk_complex_t *tmp;
    wtk_complex_t *ovec;
    float B;
    int channel=dswspec->channel;
    int channelx2=channel*channel;
    int i,j,n;
    int channel_tmp;

    float32_t *a;
    float32_t *b;

    float32x4_t a_a_1, a_b_1;
    float32x4_t b_a_1, b_b_1;
    float32x4_t c_a_1, c_b_1;
    float32x4_t ftmp_1,ftmp_2, fc_1;

    float32x4x2_t a_1;
    float32x4x2_t b_1;

    channel_tmp = (int)(channel >> 2) << 2;

    for(n=0;n<ang_idx;++n,++spec){
        tmp=dswspec->tmp;
        ovec=dswspec->ovec[n][k];
        B=dswspec->B[n][k];

        a_c = cov;
        a = (float32_t *)cov;  // complex转化为float类型

        for(i=0;i<channel;++i)
        {
            fa=fb=0;
            b_c = ovec;
            b = (float32_t *)ovec;
            for(j=0;j<channel_tmp;j+=4,a_c+=4,b_c+=4)
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
            for(j=channel_tmp;j<channel;++j,++a_c,++b_c,a+=2,b+=2){
                fa+=a_c->a*b_c->a-a_c->b*b_c->b;
                fb+=a_c->a*b_c->b+a_c->b*b_c->a;
            }
            tmp[i].a=fa;
            tmp[i].b=fb;
        }

        fa = 0;
        a_c = ovec;
        a = (float32_t *)ovec;
        b_c = tmp;
        b = (float32_t *)tmp;
        for(i=0;i<channel_tmp;i+=4,a_c+=4,b_c+=4){
            a_1 = vld2q_f32(a);
            a+=8;
            b_1 = vld2q_f32(b);
            b+=8;
            ftmp_1 = vmulq_f32(a_1.val[0], b_1.val[0]);
            ftmp_2 = vmulq_f32(a_1.val[1], b_1.val[1]);

            fc_1 = vaddq_f32(ftmp_1, ftmp_2);  // 加法

            fa += vgetq_lane_f32(fc_1, 0) + vgetq_lane_f32(fc_1, 1) + vgetq_lane_f32(fc_1, 2) + vgetq_lane_f32(fc_1, 3);
        }
        for(j=channel_tmp;j<channel;++j,++a_c,++b_c,a+=2,b+=2){
            fa+=a_c->a*b_c->a+a_c->b*b_c->b;
        }

        f=fa/channelx2;
        if(cov_travg-f<1e-6)
        {
            f=f/1e-6;
        }else
        {
            f=f/(cov_travg-f);
        }
        
        *spec=((channelx2-B)*f-B)/channelx2;
    }
#else
///////////////////////////////////////////////
    wtk_complex_t *a,*b;
    float fa,fb,f;
    int channel=dswspec->channel;
    int channelx2=channel*channel;
    int i,j,n;
    float fc;
    wtk_complex_t *c;
    wtk_complex_t *ovec;
    float B;

    for(n=0;n<ang_idx;++n,++spec){
        ovec=dswspec->ovec[n][k];
        B=dswspec->B[n][k];
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
        f=fc/channelx2;
        if((cov_travg-f)<1e-6)
        {
            f=f/1e-6;
        }else
        {
            f=f/(cov_travg-f);
        }
        
        *spec=((channelx2-B)*f-B)/channelx2;
    }
#endif
}

