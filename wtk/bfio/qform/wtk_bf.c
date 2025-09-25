#include "wtk_bf.h" 


static double bessj0( double x )
/*------------------------------------------------------------*/
/* PURPOSE: Evaluate Bessel function of first kind and order  */
/*          0 at input x                                      */
/*------------------------------------------------------------*/
{
   double ax,z;
   double xx,y,ans,ans1,ans2;

   if ((ax=fabs(x)) < 8.0) {
      y=x*x;
      ans1=57568490574.0+y*(-13362590354.0+y*(651619640.7
         +y*(-11214424.18+y*(77392.33017+y*(-184.9052456)))));
      ans2=57568490411.0+y*(1029532985.0+y*(9494680.718
         +y*(59272.64853+y*(267.8532712+y*1.0))));
      ans=ans1/ans2;
   } else {
      z=8.0/ax;
      y=z*z;
      xx=ax-0.785398164;
      ans1=1.0+y*(-0.1098628627e-2+y*(0.2734510407e-4
         +y*(-0.2073370639e-5+y*0.2093887211e-6)));
      ans2 = -0.1562499995e-1+y*(0.1430488765e-3
         +y*(-0.6911147651e-5+y*(0.7621095161e-6
         -y*0.934935152e-7)));
      ans=sqrt(0.636619772/ax)*(cos(xx)*ans1-z*sin(xx)*ans2);
   }
   return ans;
}

void wtk_bf_init_sdcov(wtk_complex_t **xrnn,float **mic_pos,int channel,int win,float eye,int rate,float speed)
{
	int k,i,j;
	int channelx2=channel*channel;
	float fa,fb;
	float **ds;
	float *m1,*m2;
	wtk_complex_t *a, *rnn;
	int nbin=win/2+1;

	wtk_complex_zero_p2(xrnn,nbin,channelx2);
	fa=rate*2.0*PI*1.0/win;
	ds=wtk_float_new_p2(channel,channel);
	for(i=0;i<channel;++i)
	{
		m1=mic_pos[i];
		for(j=i+1;j<channel;++j)
		{
			m2=mic_pos[j];
			fb=sqrt((m1[0]-m2[0])*(m1[0]-m2[0])+(m1[1]-m2[1])*(m1[1]-m2[1])+(m1[2]-m2[2])*(m1[2]-m2[2]));
			ds[i][j]=fa*fb/speed;
		}
	}
	for(k=0;k<nbin;++k)
	{
		rnn=xrnn[k];
		for(i=0;i<channel;++i)
		{
			a=rnn+i*channel+i;
			a->a=1;
			for(j=i+1;j<channel;++j)
			{
				++a;
				a->a=bessj0(ds[i][j]*k);
				rnn[j*channel+i].a=a->a;
			}
		}
		// wtk_complex_print(rnn,channelx2);
		if(eye!=0)
		{
			j=0;
			for(i=0;i<channel;++i)
			{
				rnn[j].a+=eye;
				j+=channel+1;
			}
		}
	}
	wtk_float_delete_p2(ds,channel);
}

wtk_bf_t* wtk_bf_new(wtk_bf_cfg_t *cfg,int win)
{
	wtk_bf_t *bf;

	bf=(wtk_bf_t*)wtk_malloc(sizeof(wtk_bf_t));
	bf->cfg=cfg;
	bf->channel=cfg->nmic;
	bf->channelx2=bf->channel*bf->channel;
	bf->nbin=win/2+1;
	bf->win=win;
	
	bf->ncov=wtk_complex_new_p2(bf->nbin,bf->channelx2);
	bf->scov=NULL;
	if(bf->cfg->use_sdw_mwf || bf->cfg->use_gev || bf->cfg->use_r1_mwf || bf->cfg->use_vs || bf->cfg->use_mvdr)
	{
		bf->scov=wtk_complex_new_p2(bf->nbin,bf->channelx2);
	}

	bf->ovec=wtk_complex_new_p2(bf->nbin,bf->channel);

	bf->sdcov=wtk_complex_new_p2(bf->nbin,bf->channelx2);
	if(bf->cfg->mic_pos)
	{
		wtk_bf_init_sdcov(bf->sdcov,bf->cfg->mic_pos,bf->channel,win,bf->cfg->sd_eye,bf->cfg->rate,bf->cfg->speed);
	}
	bf->w=wtk_complex_new_p2(bf->nbin,bf->channel);
	bf->fft=(wtk_complex_t*)wtk_calloc(bf->nbin,sizeof(wtk_complex_t));

	bf->tmp_cov=(wtk_complex_t *)wtk_calloc(bf->channelx2,sizeof(wtk_complex_t));
	bf->tmp=(wtk_complex_t *)wtk_calloc(bf->channelx2*2,sizeof(wtk_complex_t));
	bf->tmp2=(wtk_dcomplex_t *)wtk_calloc(bf->channelx2*2,sizeof(wtk_dcomplex_t));

	bf->eigtmp=NULL;
	if(cfg->use_ncov_eig || cfg->use_scov_evd)
	{
		bf->eigtmp=(wtk_complex_t *)wtk_calloc(bf->channel,sizeof(wtk_complex_t));
	}

	bf->vac=NULL;
	bf->val=NULL;
	if(cfg->use_vs)
	{
		bf->vac=(wtk_complex_t *)wtk_calloc(bf->channelx2,sizeof(wtk_complex_t));
		bf->val=(float *)wtk_calloc(bf->channel, sizeof(float));
	}

	bf->corr=bf->acorr=NULL;
	if(cfg->use_post)
	{
		bf->acorr=(float *)wtk_calloc(bf->nbin,sizeof(float));
		bf->corr=(float *)wtk_calloc(bf->nbin,sizeof(float));
	}

	bf->eig=NULL;
	if(bf->cfg->use_vs || cfg->use_eig_ovec || cfg->use_ncov_eig || cfg->use_scov_evd || cfg->use_gev)
	{
		bf->eig=wtk_eig_new(bf->channel);
	}

	bf->pad=(float*)wtk_calloc(win,sizeof(float));
	bf->ths=NULL;
	bf->notify=NULL;
	wtk_bf_reset(bf);
	return bf;
}

void wtk_bf_delete(wtk_bf_t *bf)
{
	wtk_free(bf->pad);
	wtk_complex_delete_p2(bf->w,bf->nbin);
	wtk_complex_delete_p2(bf->ovec,bf->nbin);
	wtk_complex_delete_p2(bf->sdcov,bf->nbin);
	if(bf->acorr)
	{
		wtk_free(bf->acorr);
		wtk_free(bf->corr);
	}
	if(bf->ncov)
	{
		wtk_complex_delete_p2(bf->ncov,bf->nbin);
	}
	if(bf->scov)
	{
		wtk_complex_delete_p2(bf->scov,bf->nbin);
	}
	if(bf->eigtmp)
	{
		wtk_free(bf->eigtmp);
	}
	if(bf->vac)
	{
		wtk_free(bf->vac);
		wtk_free(bf->val);
	}
	if(bf->eig)
	{
		wtk_eig_delete(bf->eig);
	}
	wtk_free(bf->fft);
	wtk_free(bf->tmp);
	wtk_free(bf->tmp2);
	wtk_free(bf->tmp_cov);
	wtk_free(bf);
}

void wtk_bf_reset(wtk_bf_t *bf)
{
	memset(bf->pad,0,bf->win*sizeof(float));
	bf->lst_pos=0;
	wtk_complex_zero_p2(bf->w,bf->nbin,bf->channel);
	wtk_complex_zero_p2(bf->ovec,bf->nbin,bf->channel);
	bf->nframe=0;
	if(bf->acorr)
	{
		memset(bf->acorr,0,sizeof(float)*bf->nbin);
		memset(bf->corr,0,sizeof(float)*bf->nbin);
	}
	if(bf->ncov)
	{
		wtk_complex_zero_p2(bf->ncov,bf->nbin,bf->channelx2);
	}
	if(bf->scov)
	{
		wtk_complex_zero_p2(bf->scov,bf->nbin,bf->channelx2);
	}
}

void wtk_bf_set_notify(wtk_bf_t *bf,void *ths,wtk_bf_notify_f notify)
{
	bf->ths=ths;
	bf->notify=notify;
}


void wtk_bf_update_ovec4(float theta,float phi,int channel,int nbin,int rate,float speed,float **mic_pos,wtk_complex_t **ovec)
{
	// printf("============%f++++++++++++++++\n",theta);

	float x,y,z;
	float t;//,r;
	float *mic;
	int i,j;
	float *tdoa;
	wtk_complex_t *ovec1;
	float f;

	// if(theta==90.0)
	// {
	// 	x=0;
	// 	y=1;
	// 	z=0;
	// }else if(theta==0.0)
	// {
	// 	x=1;
	// 	y=0;
	// 	z=0;
	// }else if(theta==180.0)
	// {
	// 	x=-1;
	// 	y=0;
	// 	z=0;
	// }else if(theta==270.0)
	// {
	// 	x=0;
	// 	y=-1;
	// 	z=0;
	// }else
	// {
		phi*=PI/180;
		theta*=PI/180;
		x=cos(phi)*cos(theta);
		y=cos(phi)*sin(theta);
		z=sin(phi);
	// }

	tdoa=(float *)wtk_malloc(channel*sizeof(float));
	for(i=0;i<channel;++i)
	{
		mic=mic_pos[i];
		// r=sqrt(mic[0]*mic[0]+mic[1]*mic[1]+mic[2]*mic[2]);
		// t=acos((mic[0]*x+mic[1]*y+mic[2]*z)/r);
		// tdoa[i]=cos(t)*r/speed;
		tdoa[i]=(mic[0]*x+mic[1]*y+mic[2]*z)/speed;
	}
	// print_float(tdoa,channel);

	f=1.0/sqrt(channel);
	for(i=0;i<nbin;++i)
	{
		ovec1=ovec[i];
		t=PI*rate*1.0/(nbin-1)*i;
		for(j=0;j<channel;++j)
		{
			ovec1[j].a=cos(t*tdoa[j])*f;
			ovec1[j].b=sin(t*tdoa[j])*f;
		}
	}
	wtk_free(tdoa);
}

void wtk_bf_update_ovec3(wtk_bf_t *bf,float theta,float phi,wtk_complex_t **ovec)
{
	// printf("============%f  %f++++++++++++++++\n",theta,phi);

	float x,y,z;
	float t;//,r;
	float **mic_pos=bf->cfg->mic_pos;
	float *mic;
	int i,j;
	int channel=bf->channel;
	float *tdoa;
	float speed=bf->cfg->speed;
	int nbin=bf->nbin;
	int win=bf->win;
	wtk_complex_t *ovec1;
	int rate=bf->cfg->rate;
	float f;

	// if(theta==90.0)
	// {
	// 	x=0;
	// 	y=1;
	// 	z=0;
	// }else if(theta==0.0)
	// {
	// 	x=1;
	// 	y=0;
	// 	z=0;
	// }else if(theta==180.0)
	// {
	// 	x=-1;
	// 	y=0;
	// 	z=0;
	// }else if(theta==270.0)
	// {
	// 	x=0;
	// 	y=-1;
	// 	z=0;
	// }else
	{
		phi*=PI/180;
		theta*=PI/180;
		x=cos(phi)*cos(theta);
		y=cos(phi)*sin(theta);
		z=sin(phi);
	}

	tdoa=(float *)wtk_malloc(channel*sizeof(float));
	for(i=0;i<channel;++i)
	{
		mic=mic_pos[i];
		// r=sqrt(mic[0]*mic[0]+mic[1]*mic[1]+mic[2]*mic[2]);
		// t=acos((mic[0]*x+mic[1]*y+mic[2]*z)/r);
		// tdoa[i]=cos(t)*r/speed;
		tdoa[i]=(mic[0]*x+mic[1]*y+mic[2]*z)/speed;
	}
	// print_float(tdoa,channel);

	if(bf->cfg->use_ovec_norm)
	{
		f=1.0/sqrt(channel);
	}else
	{
		f=1.0;
	}
	for(i=0;i<nbin;++i)
	{
		ovec1=ovec[i];
		t=2*PI*rate*1.0/win*i;
		for(j=0;j<channel;++j)
		{
			ovec1[j].a=cos(t*tdoa[j])*f;
			ovec1[j].b=sin(t*tdoa[j])*f;
		}
	}
	wtk_free(tdoa);
}

void wtk_bf_update_ovec(wtk_bf_t *bf,float theta,float phi)
{
	wtk_complex_t **ovec=bf->ovec;
	float x,y,z;
	float t;//,r;
	float **mic_pos=bf->cfg->mic_pos;
	float *mic;
	int i,j;
	int channel=bf->channel;
	float *tdoa;
	float speed=bf->cfg->speed;
	int nbin=bf->nbin;
	int win=bf->win;
	wtk_complex_t *ovec1;
	int rate=bf->cfg->rate;
	float f;

	// if(theta==90.0)
	// {
	// 	x=0;
	// 	y=1;
	// 	z=0;
	// }else if(theta==0.0)
	// {
	// 	x=1;
	// 	y=0;
	// 	z=0;
	// }else if(theta==180.0)
	// {
	// 	x=-1;
	// 	y=0;
	// 	z=0;
	// }else if(theta==270.0)
	// {
	// 	x=0;
	// 	y=-1;
	// 	z=0;
	// }else
	{
		phi*=PI/180;
		theta*=PI/180;
		x=cos(phi)*cos(theta);
		y=cos(phi)*sin(theta);
		z=sin(phi);
	}

	tdoa=(float *)wtk_malloc(channel*sizeof(float));
	for(i=0;i<channel;++i)
	{
		mic=mic_pos[i];
		// r=sqrt(mic[0]*mic[0]+mic[1]*mic[1]+mic[2]*mic[2]);
		// t=acos((mic[0]*x+mic[1]*y+mic[2]*z)/r);
		// tdoa[i]=cos(t)*r/speed;
		tdoa[i]=(mic[0]*x+mic[1]*y+mic[2]*z)/speed;
	}
	// print_float(tdoa,channel);

	if(bf->cfg->use_ovec_norm)
	{
		f=1.0/sqrt(channel);
	}else
	{
		f=1.0;
	}
	for(i=0;i<nbin;++i)
	{
		ovec1=ovec[i];
		t=2*PI*rate*1.0/win*i;
		for(j=0;j<channel;++j)
		{
			ovec1[j].a=cos(t*tdoa[j])*f;
			ovec1[j].b=sin(t*tdoa[j])*f;
		}
	}
	wtk_free(tdoa);
}


void wtk_bf_update_ovec2(wtk_bf_t *bf,float theta,float phi)
{
	wtk_complex_t **ovec=bf->ovec;
	float x,y,z;
	float t;//,r;
	float **mic_pos=bf->cfg->mic_pos;
	float *mic;
	int i,j;
	int channel=bf->channel;
	float *tdoa;
	float speed=bf->cfg->speed;
	int nbin=bf->nbin;
	int win=bf->win;
	wtk_complex_t *ovec1;
	int rate=bf->cfg->rate;
	float r=0.5;
	float f;

	// if(theta==90.0)
	// {
	// 	x=0;
	// 	y=1;
	// 	z=0;
	// }else if(theta==0.0)
	// {
	// 	x=1;
	// 	y=0;
	// 	z=0;
	// }else if(theta==180.0)
	// {
	// 	x=-1;
	// 	y=0;
	// 	z=0;
	// }else if(theta==270.0)
	// {
	// 	x=0;
	// 	y=-1;
	// 	z=0;
	// }else
	{
		phi*=PI/180;
		theta*=PI/180;
		x=cos(phi)*cos(theta);
		y=cos(phi)*sin(theta);
		z=sin(phi);
	}

	tdoa=(float *)wtk_malloc(channel*sizeof(float));
	for(i=0;i<channel;++i)
	{
		mic=mic_pos[i];
		tdoa[i]=(r-sqrt((mic[0]-x)*(mic[0]-x)+(mic[1]-y)*(mic[1]-y)+(mic[2]-z)*(mic[2]-z)))/speed;
	}
	print_float(tdoa,channel);

	if(bf->cfg->use_ovec_norm)
	{
		f=1.0/sqrt(channel);
	}else
	{
		f=1.0;
	}
	for(i=0;i<nbin;++i)
	{
		ovec1=ovec[i];
		t=2*PI*rate*1.0/win*i;
		for(j=0;j<channel;++j)
		{
			ovec1[j].a=cos(t*tdoa[j])*f;
			ovec1[j].b=sin(t*tdoa[j])*f;
		}
	}
	wtk_free(tdoa);
}
#ifndef EPSILON
#define EPSILON 1e-10  // 判断是否为0的阈值
#endif

static int _matrix_inverse(float **a, float **b, int n) {
    int i, j, k;
    float max, temp;

    // 初始化b为单位矩阵
    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            b[i][j] = (i == j) ? 1.0 : 0.0;
        }
    }

    // 高斯-约当消元
    for (k = 0; k < n; k++) {
        // 部分主元选择：找到第k列中绝对值最大的行
        max = fabs(a[k][k]);
        int max_row = k;
        for (i = k + 1; i < n; i++) {
            if (fabs(a[i][k]) > max) {
                max = fabs(a[i][k]);
                max_row = i;
            }
        }

        // 如果最大主元接近0，矩阵不可逆
        if (max < EPSILON) {
            return -1;
        }

        // 交换当前行(k)和最大主元行(max_row)
        if (max_row != k) {
            for (j = 0; j < n; j++) {
                temp = a[k][j];
                a[k][j] = a[max_row][j];
                a[max_row][j] = temp;

                temp = b[k][j];
                b[k][j] = b[max_row][j];
                b[max_row][j] = temp;
            }
        }

        // 归一化主元行
        temp = a[k][k];
        for (j = 0; j < n; j++) {
            a[k][j] /= temp;
            b[k][j] /= temp;
        }

        // 消去其他行的第k列
        for (i = 0; i < n; i++) {
            if (i != k) {
                temp = a[i][k];
                for (j = 0; j < n; j++) {
                    a[i][j] -= a[k][j] * temp;
                    b[i][j] -= b[k][j] * temp;
                }
            }
        }
    }

    return 0;
}

void wtk_bf_update_ovec5(float theta,float phi,int channel,int nbin,int rate,float speed,float **mic_pos,wtk_complex_t **ovec, float sdb_alpha, float Q_eye)
{
	float x,y,z;
	float t;//,r;
	float *mic;
	int i,j,k;
	float *tdoa;
	float *omega;
	float **R;
	float **R_tmp;
	wtk_complex_t *numerator;
	wtk_complex_t denominator;
	wtk_complex_t *tmp;
	wtk_complex_t *ovec1;
	float f;
	int wins = (nbin-1)*2;
	int ret=-1;

	phi*=PI/180;
	theta*=PI/180;
	x=cos(phi)*cos(theta);
	y=cos(phi)*sin(theta);
	z=sin(phi);

	tdoa=(float *)wtk_malloc(channel*sizeof(float));
	omega=(float *)wtk_malloc(nbin*sizeof(float));
	R=wtk_float_new_p2(channel,channel);
	R_tmp=wtk_float_new_p2(channel,channel);
	numerator=(wtk_complex_t *)wtk_malloc(channel*sizeof(wtk_complex_t));
	tmp=(wtk_complex_t *)wtk_malloc(channel*sizeof(wtk_complex_t));
	for(i=0;i<channel;++i)
	{
		mic=mic_pos[i];
		tdoa[i]=(mic[0]*x+mic[1]*y+mic[2]*z)/speed;
	}

	f=1.0/sqrt(channel);
	for(i=0;i<nbin;++i)
	{
		ovec1=ovec[i];
		t=PI*rate*1.0/(nbin-1)*i;
		for(j=0;j<channel;++j)
		{
			ovec1[j].a=cos(t*tdoa[j])*f;
			ovec1[j].b=sin(t*tdoa[j])*f;
		}
	}

	for(i=0;i<nbin;++i)
	{
		omega[i]=2*PI*rate*1.0/wins*i;
		ovec1=ovec[i];
		for(j=0;j<channel;++j)
		{
			for(k=j;k<channel;++k)
			{
				if(j==k){
					R[j][k]=1.0+Q_eye;
				}else{
					t = omega[i]*sqrtf((mic_pos[j][0]-mic_pos[k][0])*(mic_pos[j][0]-mic_pos[k][0])+(mic_pos[j][1]-mic_pos[k][1])*(mic_pos[j][1]-mic_pos[k][1])+(mic_pos[j][2]-mic_pos[k][2])*(mic_pos[j][2]-mic_pos[k][2]))*1.0/speed;
					R[j][k]=sinf(t)/(t+1e-9)*sdb_alpha;
				}
			}
		}
		for(j=0;j<channel;++j)
		{
			for(k=j+1;k<channel;++k)
			{
				R[k][j]=R[j][k];
			}
		}
		ret = _matrix_inverse(R,R_tmp,channel);
		if(ret != 0){
			wtk_debug("error in matrix inverse\n");
		}
		for(j=0;j<channel;++j){
			numerator[j].a = numerator[j].b = 0.0;
			tmp[j].a = tmp[j].b = 0.0;
			for(k=0;k<channel;++k){
				numerator[j].a += R_tmp[j][k]*ovec1[k].a;
				numerator[j].b += R_tmp[j][k]*ovec1[k].b;
				tmp[j].a += R_tmp[j][k]*ovec1[k].a;
				tmp[j].b += -R_tmp[j][k]*ovec1[k].b;
			}
		}
		denominator.a = denominator.b = 0.0;
		for(j=0;j<channel;++j){
			denominator.a += tmp[j].a * ovec1[j].a - tmp[j].b * ovec1[j].b;
			denominator.b += tmp[j].a * ovec1[j].b + tmp[j].b * ovec1[j].a;
		}
		for(j=0;j<channel;++j){
			ovec1[j].a = (numerator[j].a * denominator.a + numerator[j].b * denominator.b) / (denominator.a * denominator.a + denominator.b * denominator.b);
			ovec1[j].b = (numerator[j].b * denominator.a - numerator[j].a * denominator.b) / (denominator.a * denominator.a + denominator.b * denominator.b);
		}
	}


	wtk_free(tdoa);
	wtk_free(omega);
	wtk_float_delete_p2(R,channel);
	wtk_float_delete_p2(R_tmp,channel);
	wtk_free(numerator);
	wtk_free(tmp);
}

//c!=0
void wtk_bf_update_ovec_orth(wtk_bf_t *bf,wtk_complex_t **ovec2,int c)
{
	wtk_complex_t **ovec=bf->ovec;
	int k;
	int channel=bf->channel;
	int nbin=bf->nbin;
	wtk_complex_t *ovectmp,*ovectmp2;

	for(k=0;k<nbin;++k)
	{
		ovectmp=ovec[k];
		ovectmp2=ovec2[k];
		memset(ovectmp,0,channel*sizeof(wtk_complex_t));
		if(ovectmp2[c].a==0.00 && ovectmp2[c].b==0.00)
		{
			wtk_debug("error nbin %d channel %d alpha zero\n",k,c+1);
			exit(0);
		}
		if(ovectmp2[0].a==0.00 && ovectmp2[0].b==0.00)
		{
			wtk_debug("error nbin %d channel 1 alpha zero\n",k);
			exit(0);
		}
		ovectmp[0].a=-ovectmp2[c].a;
		ovectmp[0].b=-ovectmp2[c].b;
		ovectmp[c].a=ovectmp2[0].a;
		ovectmp[c].b=ovectmp2[0].b;
	}
}

//c!=0
void wtk_bf_update_ovec_orth2(wtk_complex_t **ovec, wtk_complex_t **ovec_orth, int channel, int nbin, int c)
{
	int k;
	wtk_complex_t *ovectmp,*ovectmp2;
	// float f,fa,fb;

	for(k=0;k<nbin;++k)
	{
		ovectmp=ovec_orth[k];
		ovectmp2=ovec[k];
		memset(ovectmp,0,channel*sizeof(wtk_complex_t));
		if(ovectmp2[c].a==0.00 && ovectmp2[c].b==0.00)
		{
			wtk_debug("error nbin %d channel %d alpha zero\n",k,c+1);
			exit(0);
		}
		if(ovectmp2[0].a==0.00 && ovectmp2[0].b==0.00)
		{
			wtk_debug("error nbin %d channel 1 alpha zero\n",k);
			exit(0);
		}
		ovectmp[0].a=-ovectmp2[c].a;
		ovectmp[0].b=-ovectmp2[c].b;
		ovectmp[c].a=ovectmp2[0].a;
		ovectmp[c].b=ovectmp2[0].b;
		// fa=ovectmp2[c].a;
		// fb=ovectmp2[c].b;
		// f=1.0/(fa*fa+fb*fb);
		// ovectmp[c].a=-(ovectmp2[0].a*fa+ovectmp2[0].b*fb)*f;
		// ovectmp[c].b=-(-ovectmp2[0].a*fb+ovectmp2[0].b*fa)*f;

		// ovectmp[0].a=1;
		// ovectmp[0].b=0;
	}
}


// static void wtk_eig_matlab(wtk_complex_t *w,wtk_complex_t *v,int c)
// {
// 	int i;
// 	FILE *f;
// 	float a,b;

// 	f=fopen("eig.txt","w");
// 	for(i=0;i<c*c;++i)
// 	{
// 		fprintf(f,"%e %e\n",w[i].a,w[i].b);
// 	}
// 	fclose(f);
// 	system("matlab  -nodesktop -nosplash -nojvm -r 'run ./eigx2.m; quit'");
// 	f=fopen("x.txt","r");
// 	for(i=0;i<c;++i)
// 	{
// 		fscanf(f,"%e %e\n",&a,&b);
// 		v[i].a=a;
// 		v[i].b=b;
//     }
// 	fclose(f);
// }

void wtk_bf_update_scov(wtk_bf_t *bf,wtk_complex_t **scov,int k)
{
	wtk_complex_t **ovec=bf->ovec;
	int i,j;
	int channel=bf->channel;
	wtk_complex_t *tmp=bf->tmp;
	wtk_complex_t *eigtmp=bf->eigtmp;
	float eps=bf->cfg->eig_iter_eps;
	int max_iter=bf->cfg->eig_iter_max_iter;
	wtk_complex_t *a,*b;
	int n,n2;
	wtk_complex_t *eig1,*eig2;
	wtk_complex_t *scov1=scov[k];
	wtk_complex_t *scov2;
	float tr1,tr2,tr;
	// float eye=bf->cfg->eye;

	for(i=0;i<channel;++i)
	{
		n=i*channel+i;
		scov1[n].b=0;
		for(j=i+1;j<channel;++j)
		{
			n=i*channel+j;
			n2=j*channel+i;
			scov1[n2].a=scov1[n].a;
			scov1[n2].b=-scov1[n].b;
		}
	}
	if(bf->cfg->use_eig_ovec)
	{
		// wtk_complex_print2(scov[k],channel,channel);
		if(0)
		{
			wtk_complex_itereig(scov1,ovec[k],tmp,channel,eps,max_iter);
		// }else
		// {
		// 	wtk_eig_matlab(scov1,ovec[k],channel);
		}else
		{
			wtk_eig_process_maxfv(bf->eig,scov1,ovec[k]);
		}
		// wtk_complex_print(ovec[k],channel);
		tr=0;
		for(i=0;i<channel;++i)
		{
			tr+=ovec[k][i].a*ovec[k][i].a+ovec[k][i].b*ovec[k][i].b;
		}
		tr=sqrt(tr);
		for(i=0;i<channel;++i)
		{
			ovec[k][i].a/=tr;
			ovec[k][i].b/=tr;
		}
	}else if(bf->cfg->use_scov_evd)
	{
		scov2=bf->scov[k];
		if(0)
		{
			wtk_complex_itereig(scov1,eigtmp,tmp,channel,eps,max_iter);
		}else
		{
			wtk_eig_process_maxfv(bf->eig,scov1,eigtmp);
		}
		
		for(i=0;i<channel;++i)
		{
			eig1=eigtmp+i;
			for(j=i;j<channel;++j)
			{
				eig2=eigtmp+j;

				a=scov2+i*channel+j;
				a->a=eig1->a*eig2->a+eig1->b*eig2->b;
				a->b=-eig1->a*eig2->b+eig1->b*eig2->a;
			}
		}
		tr1=tr2=0;
		for(i=0;i<channel;++i)
		{
			tr1+=scov1[i*channel+i].a;
			tr2+=scov2[i*channel+i].a;
		}
		if(tr2!=0.0)
		{
			tr=tr1/tr2;
			for(i=0;i<channel;++i)
			{
				for(j=i;j<channel;++j)
				{
					a=scov2+i*channel+j;
					a->a*=tr;
					a->b*=tr;
					if(i!=j)
					{
						b=scov2+j*channel+i;
						b->a=a->a;
						b->b=-a->b;
					}
				}
			}
		}else
		{
			for(i=0;i<channel;++i)
			{
				for(j=i+1;j<channel;++j)
				{
					a=scov2+i*channel+j;

					b=scov2+j*channel+i;
					b->a=a->a;
					b->b=-a->b;
				}
			}
		}
	}else
	{
		memcpy(bf->scov[k],scov1,sizeof(wtk_complex_t)*bf->channelx2);
	}
	// if(eye>0.0)
	// {
	// 	j=0;
	// 	for(i=0;i<channel;++i)
	// 	{
	// 		bf->scov[k][j].a+=eye;
	// 		j+=channel+1;
	// 	}
	// }
}

void wtk_bf_update_ncov(wtk_bf_t *bf,wtk_complex_t **ncov,int k)
{
	wtk_complex_t *ncov1=bf->ncov[k], *ncov2=ncov[k];
	int i,j;
	int channel=bf->channel;
	float eye=bf->cfg->eye;
	// wtk_dcomplex_t *tmp_inv=bf->tmp2;
	// wtk_complex_t *tmp_ncov=bf->tmp_ncov;
	wtk_complex_t *a, *b;
	int n,n2;
	wtk_complex_t *tmp=bf->tmp;
	wtk_complex_t *eig=bf->eigtmp,*eig1,*eig2;
	float tr1,tr2,tr;

	for(i=0;i<channel;++i)
	{
		n=i*channel+i;
		ncov2[n].b=0;
		for(j=i+1;j<channel;++j)
		{
			n=i*channel+j;
			n2=j*channel+i;
			ncov2[n2].a=ncov2[n].a;
			ncov2[n2].b=-ncov2[n].b;
		}
	}
	if(bf->cfg->use_ncov_eig)
	{
		if(0)
		{
			wtk_complex_itereig(ncov[k],eig,tmp,channel,bf->cfg->eig_iter_eps,bf->cfg->eig_iter_max_iter);
		}else
		{
			wtk_eig_process_maxfv(bf->eig,ncov[k],eig);
		}
		
		// wtk_complex_print(eig,channel);
		for(i=0;i<channel;++i)
		{
			eig1=eig+i;
			for(j=i;j<channel;++j)
			{
				eig2=eig+j;

				a=ncov1+i*channel+j;
				a->a=eig1->a*eig2->a+eig1->b*eig2->b;
				a->b=-eig1->a*eig2->b+eig1->b*eig2->a;
				if(i!=j)
				{
					b=ncov1+j*channel+i;
					b->a=a->a;
					b->b=-a->b;
				}
			}
		}
		tr1=tr2=0;
		for(i=0;i<channel;++i)
		{
			tr1+=ncov2[i*channel+i].a;
			tr2+=ncov1[i*channel+i].a;
		}
		if(tr2!=0.0)
		{
			tr=tr1/tr2;
			for(i=0;i<channel;++i)
			{
				for(j=i;j<channel;++j)
				{
					a=ncov1+i*channel+j;
					a->a*=tr;
					a->b*=tr;
					if(i!=j)
					{
						b=ncov1+j*channel+i;
						b->a=a->a;
						b->b=-a->b;
					}
				}
			}
		}else
		{
			for(i=0;i<channel;++i)
			{
				for(j=i+1;j<channel;++j)
				{
					a=ncov1+i*channel+j;

					b=ncov1+j*channel+i;
					b->a=a->a;
					b->b=-a->b;
				}
			}
		}
	}else
	{
		memcpy(ncov1, ncov2, sizeof(wtk_complex_t)*channel*channel);
	}
	if(eye>0.0)
	{
		j=0;
		for(i=0;i<channel;++i)
		{
			ncov1[j].a+=eye;
			j+=channel+1;
		}
	}
}

void wtk_bf_update_mvdr_w(wtk_bf_t *bf,int sd,int k)
{
	int channel=bf->channel;
	int i;
	float fa,f;
	wtk_complex_t **ncov;
	wtk_complex_t *w=bf->w[k];
	wtk_dcomplex_t *tmp2=bf->tmp2;
	wtk_complex_t *rk, *a;

	ncov=sd? bf->sdcov:bf->ncov;
	a=ncov[k];
	rk=bf->ovec[k];
	
	wtk_complex_guass_elimination_p1(a, rk, tmp2, channel, w);

	fa=0;
	a=w;
	for(i=0;i<channel;++i,++rk,++a)
	{
		fa+=rk->a*a->a+rk->b*a->b;
		// fb+=a->a*b->b-a->b*b->a;
	}
	// f=1.0/(fa*fa+fb*fb);
	f=1.0/fa;
	// fa*=f;
	// fb*=f;
	a=w;
	for(i=0;i<channel;++i,++a)
	{
		// a[i].a=(rk2[i].a*fa + rk2[i].b*fb);
		// a[i].b=(-rk2[i].a*fb + rk2[i].b*fa)
		a->a*=f;
		a->b*=f;
	}
}

void wtk_bf_update_mvdr_w_f(wtk_bf_t *bf,int sd,int k)
{
	int channel=bf->channel;
	int i;
	float fa,f;
	wtk_complex_t **ncov;
	wtk_complex_t *w=bf->w[k];
	wtk_complex_t *tmp=bf->tmp;
	wtk_complex_t *rk, *a;

	ncov=sd? bf->sdcov:bf->ncov;
	a=ncov[k];
	rk=bf->ovec[k];
	
	wtk_complex_guass_elimination_p1_f(a, rk, tmp, channel, w);

	fa=0;
	a=w;
	for(i=0;i<channel;++i,++rk,++a)
	{
		fa+=rk->a*a->a+rk->b*a->b;
		// fb+=a->a*b->b-a->b*b->a;
	}
	// f=1.0/(fa*fa+fb*fb);
	f=1.0/fa;
	// fa*=f;
	// fb*=f;
	a=w;
	for(i=0;i<channel;++i,++a)
	{
		// a[i].a=(rk2[i].a*fa + rk2[i].b*fb);
		// a[i].b=(-rk2[i].a*fb + rk2[i].b*fa)
		a->a*=f;
		a->b*=f;
	}
}

void wtk_bf_update_gev_w(wtk_bf_t *bf,int k)
{
	int channel=bf->channel;
	wtk_complex_t *ncov=bf->ncov[k];
	wtk_complex_t *scov=bf->scov[k];
	wtk_complex_t *tmp_cov=bf->tmp_cov;
	wtk_complex_t *tmp=bf->tmp;
	wtk_dcomplex_t *tmp2=bf->tmp2;
	wtk_complex_t *w=bf->w[k];

	wtk_complex_guass_elimination_p2(ncov, scov, tmp2, channel, tmp_cov);
	// wtk_complex_print2(bf->tmp_ncov,channel,channel);
		// wtk_debug("===========\n");
	// wtk_eig_matlab(bf->tmp_ncov,w,channel);
	if(0)
	{
		wtk_complex_itereig(tmp_cov,w,tmp,channel,bf->cfg->eig_iter_eps,bf->cfg->eig_iter_max_iter);
	}else
	{
		wtk_eig_process_maxfv(bf->eig,tmp_cov,w);
	}
	
	// wtk_complex_print(tmp,channel);
	// if(k>2)
	// {
	// 	wf.a=wf.b=0;
	// 	w2=bf->w[k-1];
	// 	for(i=0;i<channel;++i)
	// 	{
	// 		wf.a+=w[i].a*w2[i].a+w[i].b*w2[i].b;
	// 		wf.b+=-w[i].a*w2[i].b+w[i].b*w2[i].a;
	// 	}
	// 	ff=sqrt(wf.a*wf.a+wf.b*wf.b);
	// 	wf2.a=wf.a/ff;
	// 	wf2.b=wf.b/ff;
	// 	for(i=0;i<channel;++i)
	// 	{
	// 		wf.a=w[i].a;
	// 		wf.b=w[i].b;
	// 		w[i].a=wf.a*wf2.a-wf.b*wf2.b;
	// 		w[i].b=wf.a*wf2.b+wf.b*wf2.a;
	// 	}
	// }
}

void wtk_bf_update_sdw_mwf_w(wtk_bf_t *bf,int k)
{
	wtk_complex_t *scov=bf->scov[k];
	wtk_complex_t *ncov=bf->ncov[k];
	wtk_complex_t *a,*b,*c;
	int channel=bf->channel;
	int i,j;
	wtk_complex_t *tmp_cov=bf->tmp_cov;
	wtk_complex_t *tmp=bf->tmp;
	wtk_dcomplex_t *tmp2=bf->tmp2;
	wtk_complex_t *w=bf->w[k];
	float mu=bf->cfg->mu;

	for(i=0;i<channel;++i)
	{
		for(j=i;j<channel;++j)
		{
			a=tmp_cov+i*channel+j;
			b=scov+i*channel+j;
			c=ncov+i*channel+j;
			a->a=b->a+mu*c->a;
			a->b=b->b+mu*c->b;
			if(i!=j)
			{
				b=tmp_cov+j*channel+i;
				b->a=a->a;
				b->b=-a->b;
			}
		}
		tmp[i].a=scov[i].a;
		tmp[i].b=-scov[i].b;
	}
	wtk_complex_guass_elimination_p1(tmp_cov, tmp, tmp2, channel, w);
	if(bf->cfg->use_norm)
	{
		for(i=0;i<channel;++i, ++w)
		{
			w->a/=channel;
			w->b/=channel;
		}
	}
}

void wtk_bf_update_r1_mwf_w(wtk_bf_t *bf,int k)
{
	int i;
	int channel=bf->channel;
	wtk_complex_t *ncov=bf->ncov[k];
	wtk_complex_t *scov=bf->scov[k];
	wtk_complex_t *tmp_cov=bf->tmp_cov, *tmp_cov1;
	wtk_dcomplex_t *tmp2=bf->tmp2;
	wtk_complex_t *w=bf->w[k];
	float ff,fa,fb;
	float mu=bf->cfg->mu;
	int ret;

	ret=wtk_complex_guass_elimination_p2(ncov, scov, tmp2, channel, tmp_cov);
	if(ret!=0)
	{
		return;
	}

	tmp_cov1=tmp_cov;
	fa=mu;
	fb=0;
	for(i=0;i<channel;++i,tmp_cov1+=channel+1)
	{
		fa+=tmp_cov1->a;
		fb+=tmp_cov1->b;
	}
	ff=1.0/(fa*fa+fb*fb);
	fa*=ff;
	fb*=ff;

	for(i=0;i<channel;++i,++w,tmp_cov+=channel)
	{
		w->a=tmp_cov->a*fa+tmp_cov->b*fb;
		w->b=-tmp_cov->a*fb+tmp_cov->b*fa;
	}
	if(bf->cfg->use_norm)
	{
		w=bf->w[k];
		for(i=0;i<channel;++i, ++w)
		{
			w->a/=channel;
			w->b/=channel;
		}
	}

}

void wtk_bf_update_sdw_mwf_w2(wtk_bf_t *bf,int k,int nch)
{
	wtk_complex_t *scov=bf->scov[k];
	wtk_complex_t *ncov=bf->ncov[k];
	wtk_complex_t *a,*b,*c;
	int channel=bf->channel;
	int i,j;
	wtk_complex_t *tmp_cov=bf->tmp_cov;
	wtk_complex_t *tmp=bf->tmp;
	wtk_dcomplex_t *tmp2=bf->tmp2;
	wtk_complex_t *w=bf->w[k];
	float mu=bf->cfg->mu;

	for(i=0;i<channel;++i)
	{
		for(j=i;j<channel;++j)
		{
			a=tmp_cov+i*channel+j;
			b=scov+i*channel+j;
			c=ncov+i*channel+j;
			a->a=b->a+mu*c->a;
			a->b=b->b+mu*c->b;
			if(i!=j)
			{
				b=tmp_cov+j*channel+i;
				b->a=a->a;
				b->b=-a->b;
			}
		}
		tmp[i].a=scov[i+nch*channel].a;
		tmp[i].b=-scov[i+nch*channel].b;
	}
	wtk_complex_guass_elimination_p1(tmp_cov, tmp, tmp2, channel, w);
	if(bf->cfg->use_norm)
	{
		for(i=0;i<channel;++i, ++w)
		{
			w->a/=channel;
			w->b/=channel;
		}
	}
}

void wtk_bf_update_r1_mwf_w2(wtk_bf_t *bf,int k,int nch)
{
	int i;
	int channel=bf->channel;
	wtk_complex_t *ncov=bf->ncov[k];
	wtk_complex_t *scov=bf->scov[k];
	wtk_complex_t *tmp_cov=bf->tmp_cov, *tmp_cov1;
	wtk_dcomplex_t *tmp2=bf->tmp2;
	wtk_complex_t *w=bf->w[k];
	float ff,fa,fb;
	float mu=bf->cfg->mu;
	int ret;

	ret=wtk_complex_guass_elimination_p2(ncov, scov, tmp2, channel, tmp_cov);
	if(ret!=0)
	{
		return;
	}

	tmp_cov1=tmp_cov;
	fa=mu;
	fb=0;
	for(i=0;i<channel;++i,tmp_cov1+=channel+1)
	{
		fa+=tmp_cov1->a;
		fb+=tmp_cov1->b;
	}
	ff=1.0/(fa*fa+fb*fb);
	fa*=ff;
	fb*=ff;

	tmp_cov+=nch;
	for(i=0;i<channel;++i,++w,tmp_cov+=channel)
	{
		w->a=tmp_cov->a*fa+tmp_cov->b*fb;
		w->b=-tmp_cov->a*fb+tmp_cov->b*fa;
	}
	if(bf->cfg->use_norm)
	{
		w=bf->w[k];
		for(i=0;i<channel;++i, ++w)
		{
			w->a/=channel;
			w->b/=channel;
		}
	}

}


void wtk_bf_update_vs_w(wtk_bf_t *bf,int k)
{
	int i,j,q;
	int channel=bf->channel;
	wtk_complex_t *scov=bf->scov[k], *scov1;
	wtk_complex_t *ncov=bf->ncov[k];
	wtk_dcomplex_t *tmp2=bf->tmp2;
	wtk_complex_t *tmp_cov=bf->tmp_cov;
	wtk_complex_t *a,*b;
	wtk_complex_t *vac=bf->vac,*vac1;
	float *val=bf->val;
	wtk_complex_t *w;
	float mu=bf->cfg->mu;
	float ff;
	int qrank=bf->cfg->qrank;

	wtk_complex_guass_elimination_p2(ncov, scov, tmp2, channel, tmp_cov);
	wtk_eig_process_fv2( bf->eig, tmp_cov, vac, val);

	memset(tmp_cov,0,bf->channelx2*sizeof(wtk_complex_t));
	for(q=0; q<channel-qrank; ++q)
	{
		for(i=0; i<channel; ++i,++vac)
		{
			vac1=vac;
			for(j=i; j<channel; ++j,++vac1)
			{
				a=tmp_cov+i*channel+j;
				a->a+=(vac->a*vac1->a+vac->b*vac1->b)/(mu+val[q]);
				a->b+=(-vac->a*vac1->b+vac->b*vac1->a)/(mu+val[q]);
				if(i!=j)
				{
					b=tmp_cov+j*channel+i;
					b->a=a->a;
					b->b=-a->b;
				}
			}
		}
	}

	w=bf->w[k];
	for(i=0;i<channel;++i,++w)
	{
		w->a=w->b=0;
		scov1=scov;
		for(j=0;j<channel;++j,++tmp_cov,++scov1)
		{
			w->a+=tmp_cov->a*scov1->a + tmp_cov->b*scov1->b;
			w->b+=-tmp_cov->a*scov1->b + tmp_cov->b*scov1->a;
		}
	}

	w=bf->w[k];
	ff=0;
	for(i=0;i<channel;++i,++w)
	{
		ff+=w->a*w->a+w->b*w->b;
	}
	ff=1.0/sqrt(ff);

	w=bf->w[k];
	for(i=0;i<channel;++i,++w)
	{
		w->a*=ff;
		w->b*=ff;
	}
}


void wtk_bf_update_w(wtk_bf_t *bf,int k)
{
	if(bf->cfg->use_mvdr)
	{
		wtk_bf_update_mvdr_w(bf,0,k);
	}else if(bf->cfg->use_sdw_mwf)
	{
		wtk_bf_update_sdw_mwf_w(bf,k);
	}else if(bf->cfg->use_r1_mwf)
	{
		wtk_bf_update_r1_mwf_w(bf,k);
	}else if(bf->cfg->use_gev)
	{
		wtk_bf_update_gev_w(bf,k);
	}else if(bf->cfg->use_vs)
	{
		wtk_bf_update_vs_w(bf,k);
	}
}

void wtk_bf_update_w2(wtk_bf_t *bf,int k,int nch)
{
	if(bf->cfg->use_sdw_mwf)
	{
		wtk_bf_update_sdw_mwf_w2(bf,k,nch);
	}else if(bf->cfg->use_r1_mwf)
	{
		wtk_bf_update_r1_mwf_w2(bf,k,nch);
	}
}


void wtk_bf_init_w(wtk_bf_t *bf)
{
	int i;
	int nbin=bf->nbin;
	for(i=0;i<nbin;++i)
	{
		wtk_bf_update_mvdr_w(bf,1,i);
	}
}

void wtk_bf_output_msg3(wtk_bf_t *m,wtk_stft_msg_t *msg,wtk_stft_t *stft,int is_end,float *cohv)
{
	wtk_complex_t **w=m->w;
	int nbin=m->nbin;
	int i,k;
	int channel=m->channel;
	double ta,tb,td,td2;
	wtk_complex_t *output=m->fft;
	float *po=stft->output[0];
	wtk_complex_t *w1;

	float *corr=m->corr;
	float *acorr=m->acorr;
	wtk_complex_t **fft,*fft1;
	float gramma=m->cfg->post_gramma;
	float gramma1=1-gramma;
	float post=1;

	++m->nframe;
	if(m->nframe>20000)
	{
		m->nframe=20000;
	}

	if(msg)
	{
		fft=msg->fft;
		for(k=1;k<nbin-1;++k)
		{
			w1=w[k];
			ta=tb=0;
			for(i=0;i<channel;++i)
			{
				fft1=fft[i]+k;
				ta+=w1[i].a*fft1->a + w1[i].b*fft1->b;
				tb+=w1[i].a*fft1->b - w1[i].b*fft1->a;
			}
			output[k].a=ta;
			output[k].b=tb;
			
			if(acorr)
			{
				td=0;
				for(i=0;i<channel;++i)
				{
					fft1=fft[i]+k;
					td+=fft1->a*fft1->a + fft1->b*fft1->b;
				}
				if(cohv[k]<=0)
				{
					td2=td;
				}else
				{
					td2=0;
				}
				
				
				if(m->nframe==1)
				{
					acorr[k]=td;
					corr[k]=td2;
				}else
				{
					acorr[k]=gramma*acorr[k]+gramma1*td;
					corr[k]=gramma*corr[k]+gramma1*td2;
				}
				post=(corr[k])/(acorr[k]);

				output[k].a*=post;
				output[k].b*=post;
			}
		}
		output[0].a=output[0].b=0;
		output[nbin-1].a=output[nbin-1].b=0;
	}
	//exit(0);
	if(is_end)
	{
		k=wtk_stft_output_ifft2(stft,output,0,m->lst_pos,1,po,m->pad);
	}else
	{
		k=wtk_stft_output_ifft2(stft,output,0,0,0,po,m->pad);
	}
	if(m->notify)
	{
		m->notify(m->ths,po,k,is_end);
	}
}

wtk_complex_t *wtk_bf_output_fft_msg(wtk_bf_t *m,wtk_stft_msg_t *msg)
{
	wtk_complex_t **w=m->w;
	int nbin=m->nbin;
	int i,k,j;
	int channel=m->channel;
	double ta,tb,td,td2;
	wtk_complex_t *output=m->fft;
	wtk_complex_t *w1;

	float *corr=m->corr;
	float *acorr=m->acorr;
	wtk_complex_t **fft,*fft1,*fft2;
	float gramma=m->cfg->post_gramma;
	float gramma1=1-gramma;
	float post=1;
	int nchx=(channel*(channel-1))/2;

	++m->nframe;
	if(m->nframe>20000)
	{
		m->nframe=20000;
	}

	if(msg)
	{
		fft=msg->fft;
		for(k=1;k<nbin-1;++k)
		{
			w1=w[k];
			ta=tb=0;
			for(i=0;i<channel;++i)
			{
				fft1=fft[i]+k;
				ta+=w1[i].a*fft1->a + w1[i].b*fft1->b;
				tb+=w1[i].a*fft1->b - w1[i].b*fft1->a;
			}
			output[k].a=ta;
			output[k].b=tb;
			
			if(acorr)
			{
				td=0;
				for(i=0;i<channel;++i)
				{
					fft1=fft[i]+k;
					td+=fft1->a*fft1->a + fft1->b*fft1->b;
				}
				td2=0;
				for(i=0;i<channel-1;++i)
				{
					fft1=fft[i]+k;
					for(j=i+1;j<channel;++j)
					{
						fft2=fft[j]+k;
						td2+=fft1->a*fft2->a + fft1->b*fft2->b;
					}
				}
				if(m->nframe==1)
				{
					acorr[k]=td;
					corr[k]=td2;
				}else
				{
					acorr[k]=gramma*acorr[k]+gramma1*td;
					corr[k]=gramma*corr[k]+gramma1*td2;
				}
				post=(corr[k]/nchx)/(acorr[k]/channel);

				output[k].a*=post;
				output[k].b*=post;
			}
		}
		output[0].a=output[0].b=0;
		output[nbin-1].a=output[nbin-1].b=0;
	}
	return output;
}


void wtk_bf_output_fft_k(wtk_bf_t *m,wtk_complex_t *fft, wtk_complex_t *out, int k)
{
	wtk_complex_t *w=m->w[k];
	int i;
	int channel=m->channel;
	float ta,tb;

	ta=tb=0;
	for(i=0; i<channel; ++i, ++fft, ++w)
	{
		ta+=w->a*fft->a + w->b*fft->b;
		tb+=w->a*fft->b - w->b*fft->a;
	}
	out->a=ta;
	out->b=tb;
}


wtk_complex_t *wtk_bf_output_fft_msg2_2(wtk_bf_t *m,wtk_stft_msg_t *msg,float cohv)
{
	wtk_complex_t **w=m->w;
	int nbin=m->nbin;
	int i,k;
	int channel=m->channel;
	double ta,tb,td,td2;
	wtk_complex_t *output=m->fft;
	wtk_complex_t *w1;

	float *corr=m->corr;
	float *acorr=m->acorr;
	wtk_complex_t **fft,*fft1;
	float gramma=m->cfg->post_gramma;
	float gramma1=1-gramma;
	float post=1;

	++m->nframe;
	if(m->nframe>20000)
	{
		m->nframe=20000;
	}

	if(msg)
	{
		fft=msg->fft;
		for(k=1;k<nbin-1;++k)
		{
			w1=w[k];
			ta=tb=0;
			for(i=0;i<channel;++i)
			{
				fft1=fft[i]+k;
				ta+=w1[i].a*fft1->a + w1[i].b*fft1->b;
				tb+=w1[i].a*fft1->b - w1[i].b*fft1->a;
			}
			output[k].a=ta;
			output[k].b=tb;
			
			if(acorr)
			{
				td=0;
				for(i=0;i<channel;++i)
				{
					fft1=fft[i]+k;
					td+=(fft1->a*fft1->a + fft1->b*fft1->b);
				}
				if(cohv>0)
				{
					td2=td;
				}else
				{
					td2=0.3*td;
					// td2=0;
					// for(i=0;i<channel-1;++i)
					// {
					// 	fft1=fft[i]+k;
					// 	for(j=i+1;j<channel;++j)
					// 	{
					// 		fft2=fft[j]+k;
					// 		td2+=(fft1->a*fft2->a + fft1->b*fft2->b);
					// 	}
					// }
					// td2*=channel*1.0/(channel*(channel-1)/2);
				}
				
				
				if(m->nframe==1)
				{
					acorr[k]=td;
					corr[k]=td2;
				}else
				{
					acorr[k]=gramma*acorr[k]+gramma1*td;
					corr[k]=gramma*corr[k]+gramma1*td2;
				}
				post=(corr[k])/(acorr[k]);

				output[k].a*=post;
				output[k].b*=post;
			}
		}
		output[0].a=output[0].b=0;
		output[nbin-1].a=output[nbin-1].b=0;
	}
	return output;
}

wtk_complex_t *wtk_bf_output_fft_msg2(wtk_bf_t *m,wtk_stft_msg_t *msg,float *cohv)
{
	wtk_complex_t **w=m->w;
	int nbin=m->nbin;
	int i,k;
	int channel=m->channel;
	double ta,tb,td,td2;
	wtk_complex_t *output=m->fft;
	wtk_complex_t *w1;

	float *corr=m->corr;
	float *acorr=m->acorr;
	wtk_complex_t **fft,*fft1;
	float gramma=m->cfg->post_gramma;
	float gramma1=1-gramma;
	float post=1;

	++m->nframe;
	if(m->nframe>20000)
	{
		m->nframe=20000;
	}

	if(msg)
	{
		fft=msg->fft;
		for(k=1;k<nbin-1;++k)
		{
			w1=w[k];
			ta=tb=0;
			for(i=0;i<channel;++i)
			{
				fft1=fft[i]+k;
				ta+=w1[i].a*fft1->a + w1[i].b*fft1->b;
				tb+=w1[i].a*fft1->b - w1[i].b*fft1->a;
			}
			output[k].a=ta;
			output[k].b=tb;
			
			if(acorr)
			{
				td=0;
				for(i=0;i<channel;++i)
				{
					fft1=fft[i]+k;
					td+=sqrt(fft1->a*fft1->a + fft1->b*fft1->b);
				}
				td=td/channel;
				td2=cohv[k]*td;
				// if(cohv[k]>0)
				// {
				// 	td2=td;
				// }else
				// {
				// 	td2=0;
				// }
				
				if(m->nframe==1)
				{
					acorr[k]=td;
					corr[k]=td2;
				}else
				{
					acorr[k]=gramma*acorr[k]+gramma1*td;
					corr[k]=gramma*corr[k]+gramma1*td2;
				}
				post=(corr[k])/(acorr[k]);

				output[k].a*=post;
				output[k].b*=post;
			}
		}
		output[0].a=output[0].b=0;
		output[nbin-1].a=output[nbin-1].b=0;
	}
	return output;
}

wtk_complex_t *wtk_bf_output_fft_msg3(wtk_bf_t *m,wtk_stft_msg_t *msg,float *cohv)
{
	wtk_complex_t **w=m->w;
	int nbin=m->nbin;
	int i,k;
	int channel=m->channel;
	double ta,tb,td,td2;
	wtk_complex_t *output=m->fft;
	wtk_complex_t *w1;

	float *corr=m->corr;
	float *acorr=m->acorr;
	wtk_complex_t **fft,*fft1;
	float gramma=m->cfg->post_gramma;
	float gramma1=1-gramma;
	float post=1;

	++m->nframe;
	if(m->nframe>20000)
	{
		m->nframe=20000;
	}

	if(msg)
	{
		fft=msg->fft;
		for(k=1;k<nbin-1;++k)
		{
			w1=w[k];
			ta=tb=0;
			for(i=0;i<channel;++i)
			{
				fft1=fft[i]+k;
				ta+=w1[i].a*fft1->a + w1[i].b*fft1->b;
				tb+=w1[i].a*fft1->b - w1[i].b*fft1->a;
			}
			output[k].a=ta;
			output[k].b=tb;
			
			if(acorr)
			{
				td=0;
				for(i=0;i<channel;++i)
				{
					fft1=fft[i]+k;
					td+=fft1->a*fft1->a + fft1->b*fft1->b;
				}
				td2=0;
				if(cohv[k]<0)
				{
					td2=td;
				}
				
				if(m->nframe==1)
				{
					acorr[k]=td;
					corr[k]=td2;
				}else
				{
					acorr[k]=gramma*acorr[k]+gramma1*td;
					corr[k]=gramma*corr[k]+gramma1*td2;
				}
				post=(corr[k])/(acorr[k]);

				output[k].a*=post;
				output[k].b*=post;
			}
		}
		output[0].a=output[0].b=0;
		output[nbin-1].a=output[nbin-1].b=0;
	}
	return output;
}

void wtk_bf_output_msg2(wtk_bf_t *m,wtk_stft_msg_t *msg,wtk_stft_t *stft,int is_end,float *cohv)
{
	wtk_complex_t **w=m->w;
	int nbin=m->nbin;
	int i,k;
	int channel=m->channel;
	double ta,tb,td,td2;
	wtk_complex_t *output=m->fft;
	float *po=stft->output[0];
	wtk_complex_t *w1;

	float *corr=m->corr;
	float *acorr=m->acorr;
	wtk_complex_t **fft,*fft1;
	float gramma=m->cfg->post_gramma;
	float gramma1=1-gramma;
	float post=1;

	++m->nframe;
	if(m->nframe>20000)
	{
		m->nframe=20000;
	}

	if(msg)
	{
		fft=msg->fft;
		for(k=1;k<nbin-1;++k)
		{
			w1=w[k];
			ta=tb=0;
			for(i=0;i<channel;++i)
			{
				fft1=fft[i]+k;
				ta+=w1[i].a*fft1->a + w1[i].b*fft1->b;
				tb+=w1[i].a*fft1->b - w1[i].b*fft1->a;
			}
			output[k].a=ta;
			output[k].b=tb;
			
			if(acorr)
			{
				td=0;
				for(i=0;i<channel;++i)
				{
					fft1=fft[i]+k;
					td+=fft1->a*fft1->a + fft1->b*fft1->b;
				}
				if(cohv[k]>0)
				{
					td2=td;
				}else
				{
					td2=0;
				}
				
				if(m->nframe==1)
				{
					acorr[k]=td;
					corr[k]=td2;
				}else
				{
					acorr[k]=gramma*acorr[k]+gramma1*td;
					corr[k]=gramma*corr[k]+gramma1*td2;
				}
				post=(corr[k])/(acorr[k]);

				output[k].a*=post;
				output[k].b*=post;
			}
		}
		output[0].a=output[0].b=0;
		output[nbin-1].a=output[nbin-1].b=0;
	}
	//exit(0);
	if(is_end)
	{
		k=wtk_stft_output_ifft2(stft,output,0,m->lst_pos,1,po,m->pad);
	}else
	{
		k=wtk_stft_output_ifft2(stft,output,0,0,0,po,m->pad);
	}
	if(m->notify)
	{
		m->notify(m->ths,po,k,is_end);
	}
}

void wtk_bf_output_msg(wtk_bf_t *m,wtk_stft_msg_t *msg,wtk_stft_t *stft,int is_end)
{
	wtk_complex_t **w=m->w;
	int nbin=m->nbin;
	int i,j,k;
	int channel=m->channel;
	double ta,tb,td,td2;
	wtk_complex_t *output=m->fft;
	float *po=stft->output[0];
	wtk_complex_t *w1;

	float *corr=m->corr;
	float *acorr=m->acorr;
	wtk_complex_t **fft,*fft1,*fft2;
	float gramma=m->cfg->post_gramma;
	float gramma1=1-gramma;
	int nchx=(channel*(channel-1))/2;
	float post=1;

	++m->nframe;
	if(m->nframe>20000)
	{
		m->nframe=20000;
	}
	
	if(msg)
	{
		fft=msg->fft;
		for(k=1;k<nbin-1;++k)
		{
			w1=w[k];
			ta=tb=0;
			for(i=0;i<channel;++i)
			{
				fft1=fft[i]+k;
				ta+=w1[i].a*fft1->a + w1[i].b*fft1->b;
				tb+=w1[i].a*fft1->b - w1[i].b*fft1->a;
			}
			output[k].a=ta;
			output[k].b=tb;
			
			if(acorr)
			{
				td=0;
				for(i=0;i<channel;++i)
				{
					fft1=fft[i]+k;
					td+=fft1->a*fft1->a + fft1->b*fft1->b;
				}
				td2=0;
				for(i=0;i<channel-1;++i)
				{
					fft1=fft[i]+k;
					for(j=i+1;j<channel;++j)
					{
						fft2=fft[j]+k;
						td2+=fft1->a*fft2->a + fft1->b*fft2->b;
					}
				}
				if(m->nframe==1)
				{
					acorr[k]=td;
					corr[k]=td2;
				}else
				{
					acorr[k]=gramma*acorr[k]+gramma1*td;
					corr[k]=gramma*corr[k]+gramma1*td2;
				}
				post=(corr[k]/nchx)/(acorr[k]/channel);

				output[k].a*=post;
				output[k].b*=post;
			}
		}
		output[0].a=output[0].b=0;
		output[nbin-1].a=output[nbin-1].b=0;
	}
	//exit(0);
	if(is_end)
	{
		k=wtk_stft_output_ifft2(stft,output,0,m->lst_pos,1,po,m->pad);
	}else
	{
		k=wtk_stft_output_ifft2(stft,output,0,0,0,po,m->pad);
	}
	if(m->notify)
	{
		m->notify(m->ths,po,k,is_end);
	}
}





wtk_complex_t *wtk_bf_output_fft2_msg(wtk_bf_t *m,wtk_stft2_msg_t *msg)
{
	wtk_complex_t **w=m->w;
	int nbin=m->nbin;
	int i,k,j;
	int channel=m->channel;
	double ta,tb,td,td2;
	wtk_complex_t *output=m->fft,*out2;
	wtk_complex_t *w1;

	float *corr=m->corr;
	float *acorr=m->acorr;
	wtk_complex_t **fft,*fft1,*fft2;
	float gramma=m->cfg->post_gramma;
	float gramma1=1-gramma;
	float post=1;
	int nchx=(channel*(channel-1))/2;

	++m->nframe;
	if(m->nframe>20000)
	{
		m->nframe=20000;
	}

	if(msg)
	{
		fft=msg->fft;
		out2=output+1;
		for(k=1;k<nbin-1;++k,++out2)
		{
			w1=w[k];
			fft1=fft[k];
			ta=tb=0;
			for(i=0;i<channel;++i,++fft1,++w1)
			{
				ta+=w1->a*fft1->a + w1->b*fft1->b;
				tb+=w1->a*fft1->b - w1->b*fft1->a;
			}
			out2->a=ta;
			out2->b=tb;
			
			if(acorr)
			{
				td=0;
				fft1=fft[k];
				for(i=0;i<channel;++i,++fft1)
				{
					td+=fft1->a*fft1->a + fft1->b*fft1->b;
				}
				td2=0;
				fft1=fft[k];
				for(i=0;i<channel-1;++i,++fft1)
				{
					fft2=fft1+1;
					for(j=i+1;j<channel;++j,++fft2)
					{
						td2+=fft1->a*fft2->a + fft1->b*fft2->b;
					}
				}
				if(m->nframe==1)
				{
					acorr[k]=td;
					corr[k]=td2;
				}else
				{
					acorr[k]=gramma*acorr[k]+gramma1*td;
					corr[k]=gramma*corr[k]+gramma1*td2;
				}
				post=(corr[k]/nchx)/(acorr[k]/channel);

				out2->a*=post;
				out2->b*=post;
			}
		}
		output[0].a=output[0].b=0;
		output[nbin-1].a=output[nbin-1].b=0;
	}
	return output;
}

wtk_complex_t *wtk_bf_output_fft2_msg2(wtk_bf_t *m,wtk_stft2_msg_t *msg,float *cohv)
{
	wtk_complex_t **w=m->w;
	int nbin=m->nbin;
	int i,k;
	int channel=m->channel;
	double ta,tb,td,td2;
	wtk_complex_t *output=m->fft, *out2;
	wtk_complex_t *w1;

	float *corr=m->corr;
	float *acorr=m->acorr;
	wtk_complex_t **fft,*fft1;
	float gramma=m->cfg->post_gramma;
	float gramma1=1-gramma;
	float post=1;

	++m->nframe;
	if(m->nframe>20000)
	{
		m->nframe=20000;
	}

	if(msg)
	{
		fft=msg->fft;
		out2=output+1;
		for(k=1;k<nbin-1;++k,++out2)
		{
			w1=w[k];
			fft1=fft[k];

			ta=tb=0;
			for(i=0;i<channel;++i,++fft1,++w1)
			{
				ta+=w1->a*fft1->a + w1->b*fft1->b;
				tb+=w1->a*fft1->b - w1->b*fft1->a;
			}
			out2->a=ta;
			out2->b=tb;
			
			if(acorr)
			{
				td=0;
				fft1=fft[k];
				for(i=0;i<channel;++i,++fft1)
				{
					td+=sqrt(fft1->a*fft1->a + fft1->b*fft1->b);
				}
				td=td/channel;
				td2=cohv[k]*td;
				// if(cohv[k]>0)
				// {
				// 	td2=td;
				// }else
				// {
				// 	td2=0;
				// }
				
				if(m->nframe==1)
				{
					acorr[k]=td;
					corr[k]=td2;
				}else
				{
					acorr[k]=gramma*acorr[k]+gramma1*td;
					corr[k]=gramma*corr[k]+gramma1*td2;
				}
				post=(corr[k])/(acorr[k]);

				out2->a*=post;
				out2->b*=post;
			}
		}
		output[0].a=output[0].b=0;
		output[nbin-1].a=output[nbin-1].b=0;
	}
	return output;
}

wtk_complex_t *wtk_bf_output_fft2(wtk_bf_t *m,wtk_complex_t **fft,float *cohv)
{
	wtk_complex_t **w=m->w;
	int nbin=m->nbin;
	int i,k;
	int channel=m->channel;
	double ta,tb,td,td2;
	wtk_complex_t *output=m->fft, *out2;
	wtk_complex_t *w1;

	float *corr=m->corr;
	float *acorr=m->acorr;
	wtk_complex_t *fft1;
	float gramma=m->cfg->post_gramma;
	float gramma1=1-gramma;
	float post=1;

	++m->nframe;
	if(m->nframe>20000)
	{
		m->nframe=20000;
	}

	out2=output+1;
	for(k=1;k<nbin-1;++k,++out2)
	{
		w1=w[k];
		fft1=fft[k];

		ta=tb=0;
		for(i=0;i<channel;++i,++fft1,++w1)
		{
			ta+=w1->a*fft1->a + w1->b*fft1->b;
			tb+=w1->a*fft1->b - w1->b*fft1->a;
		}
		out2->a=ta;
		out2->b=tb;
		
		if(acorr)
		{
			td=0;
			fft1=fft[k];
			for(i=0;i<channel;++i,++fft1)
			{
				td+=sqrt(fft1->a*fft1->a + fft1->b*fft1->b);
			}
			td=td/channel;
			td2=cohv[k]*td;
			// if(cohv[k]>0)
			// {
			// 	td2=td;
			// }else
			// {
			// 	td2=0;
			// }
			
			if(m->nframe==1)
			{
				acorr[k]=td;
				corr[k]=td2;
			}else
			{
				acorr[k]=gramma*acorr[k]+gramma1*td;
				corr[k]=gramma*corr[k]+gramma1*td2;
			}
			post=(corr[k])/(acorr[k]);

			out2->a*=post;
			out2->b*=post;
		}
	}
	output[0].a=output[0].b=0;
	output[nbin-1].a=output[nbin-1].b=0;
	return output;
}

//Schmidt_orthogonal
void wtk_bf_orth_ovec(wtk_complex_t ***a,int m,int nbin,int channel, int need_norm)
{
	int i,j,k,c;
	wtk_complex_t *atmp,*btmp,*btmp2;
	wtk_complex_t ***b;
	wtk_complex_t alpha;
	float f,fa,fb,fa2,fb2;

	b=wtk_complex_new_p3(m, nbin, channel);

	for(k=0; k<nbin; ++k)
	{
		memcpy(b[0][k], a[0][k], sizeof(wtk_complex_t)*channel);
		for(i=1; i<channel; ++i)
		{
			btmp=b[i][k];
			atmp=a[i][k];

			for(j=0; j<i; ++j)
			{
				btmp2=b[j][k];

				fa=fb=fa2=fb2=0;
				for(c=0; c<channel; ++c)
				{
					fa+=atmp[c].a*btmp2[c].a-atmp[c].b*btmp2[c].b;
					fb+=atmp[c].a*btmp2[c].b+atmp[c].b*btmp2[c].a;

					fa2+=btmp2[c].a*btmp2[c].a-btmp2[c].b*btmp2[c].b;
					fb2+=btmp2[c].a*btmp2[c].b+btmp2[c].b*btmp2[c].a;
				}
				f=1.0/(fa2*fa2+fb2*fb2);
				alpha.a=(fa*fa2+fb*fb2)*f;
				alpha.b=(-fa*fb2+fb*fa2)*f;

				for(c=0; c<channel; ++c)
				{
					btmp[c].a-=alpha.a*btmp2[c].a-alpha.b*btmp2[c].b;
					btmp[c].b-=alpha.a*btmp2[c].b+alpha.b*btmp2[c].a;
				}
			}

			for(c=0; c<channel; ++c)
			{
				btmp[c].a+=atmp[c].a;
				btmp[c].b+=atmp[c].b;
			}
		}
	}

	if(need_norm)
	{
		for(i=0; i<channel; ++i)
		{
			for(k=0; k<nbin; ++k)
			{
				btmp=b[i][k];
				f=0;
				for(c=0; c<channel; ++c, ++btmp)
				{
					f+=btmp->a*btmp->a+btmp->b*btmp->b;
				}
				f=1.0/sqrt(f);

				btmp=b[i][k];
				for(c=0; c<channel; ++c, ++btmp)
				{
					btmp->a*=f;
					btmp->b*=f;
				}
			}
		}
	}

	wtk_complex_cpy_p3(a, b, m, nbin, channel);
	wtk_complex_delete_p3(b, m, nbin);
}

