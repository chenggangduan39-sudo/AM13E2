#include "wtk_cfft.h" 

wtk_cfft_t*  wtk_cfft_new(int len)
{
	wtk_cfft_t *m;

	m=(wtk_cfft_t*)wtk_malloc(sizeof(wtk_cfft_t));
	m->fft=wtk_rfft_new(len/2);
	m->cr=wtk_malloc(sizeof(float)*m->fft->len);
	m->ci=wtk_malloc(sizeof(float)*m->fft->len);
	m->fr=wtk_malloc(sizeof(float)*m->fft->len);
	m->fi=wtk_malloc(sizeof(float)*m->fft->len);
	m->fftd=wtk_malloc(sizeof(wtk_complex_t)*m->fft->len);
	return m;
}

void wtk_cfft_delete(wtk_cfft_t *m)
{
	wtk_rfft_delete(m->fft);
	wtk_free(m->cr);
	wtk_free(m->ci);
	wtk_free(m->fr);
	wtk_free(m->fi);
	wtk_free(m->fftd);
	wtk_free(m);
}

wtk_complex_t* wtk_cfft_fft(wtk_cfft_t *m,wtk_complex_t *x)
{
	return wtk_cfft_fft2(m,x,m->fft->len);
}

wtk_complex_t* wtk_cfft_fft2(wtk_cfft_t *m,wtk_complex_t *frame,int n2)
{
	wtk_rfft_t *fft=m->fft;
	int len=fft->len;
	float *cr=m->cr,*ci=m->ci;
	float *fr=m->fr,*fi=m->fi;
	int i;
	wtk_complex_t *fftd=m->fftd;
	int win=fft->win;

	//wtk_complex_print(frame,m->cfg->len);
	//print_float(hann,len);
	//exit(0);
	//wtk_debug("len=%d\n",len);
	for(i=0;i<n2;++i)
	{
		cr[i]=frame[i].a;
		ci[i]=frame[i].b;
	}
	if(n2<fft->len)
	{
		//wtk_debug("%d/%d\n",n2,fft->len);
		memset(cr+n2,0,(fft->len-n2)*sizeof(float));
		memset(ci+n2,0,(fft->len-n2)*sizeof(float));
	}
	wtk_rfft_process_fft(fft,fr,cr);
	wtk_rfft_process_fft(fft,fi,ci);
	//wtk_rfft_print_fft(fr,fft->len);
	//wtk_rfft_print_fft(fi,fft->len);
	//exit(0);
	fftd[0].a=fr[0];
	fftd[0].b=fi[0];
	for(i=1;i<win;++i)
	{
		//(a+bi) (c+di)  = (a-d)+(b+c)i;
		///ta+=f[n-i];
		//tb+=f[n-i+win];
		fftd[i].a=fr[i]+fi[i+win];//fi[fft->len-i];
		fftd[i].b=-fr[i+win]+fi[i];//fi[fft->len-i+fft->win];
		//wtk_debug("v[%d/%d]=%f+%fj\n",i,j,fftd[j].a,fftd[j].b);
		//exit(0);
	}
	fftd[win].a=fr[win];
	fftd[win].b=fi[win];
	for(i=fft->win+1;i<len;++i)
	{
		//(a+bi) (c+di)  = (a-d)+(b+c)i;
		///ta+=f[n-i];
		//tb+=f[n-i+win];
		fftd[i].a=fr[len-i]-fi[len-i+win];//fi[fft->len-i];
		fftd[i].b=fr[len-i+win]+fi[len-i];//fi[fft->len-i+fft->win];
		//wtk_debug("v[%d/%d]=%f+%fj\n",i,j,fftd[j].a,fftd[j].b);
		//exit(0);
	}
	//wtk_complex_print(fftd,fft->win);
	///exit(0);
	return fftd;
}

//IFFT(X)=(1/N)conj(FFT(conj(X));
wtk_complex_t* wtk_cfft_ifft(wtk_cfft_t *m,wtk_complex_t *f)
{
	wtk_rfft_t *fft=m->fft;
	wtk_complex_t *fftd=m->fftd;
	int i;
	int len=fft->len;
	float r=1.0/fft->len;

	for(i=0;i<len;++i)
	{
		fftd[i].a=f[i].a;
		fftd[i].b=-f[i].b;
	}
	//wtk_complex_print(fftd,m->fft->len);
	//exit(0);
	fftd=wtk_cfft_fft(m,fftd);
	//wtk_complex_print(tmp,fft->len);
	//wtk_complex_print(fftd,m->fft->len);
	//exit(0);
	for(i=0;i<fft->len;++i)
	{
		fftd[i].a*=r;
		fftd[i].b*=-r;
	}
	return fftd;
}


//IFFT(X)=(1/N)conj(FFT(conj(X));
wtk_complex_t* wtk_cfft_ifft2(wtk_cfft_t *m,wtk_complex_t *frame,int n2)
{
	wtk_rfft_t *fft=m->fft;
	wtk_complex_t *fftd=m->fftd;
	int i;
	int len=fft->len;
	float r=1.0/fft->len;

	for(i=0;i<n2;++i)
	{
		fftd[i].a=frame[i].a;
		fftd[i].b=-frame[i].b;
	}
	if(n2<len)
	{
		//wtk_debug("%d/%d\n",n2,fft->len);
		memset(fftd+n2,0,(len-n2)*sizeof(wtk_complex_t));
	}
	//wtk_complex_print(fftd,m->fft->len);
	//exit(0);
	fftd=wtk_cfft_fft(m,fftd);
	//wtk_complex_print(tmp,fft->len);
	//wtk_complex_print(fftd,m->fft->len);
	//exit(0);
	for(i=0;i<len;++i)
	{
		fftd[i].a*=r;
		fftd[i].b*=-r;
	}
	//wtk_complex_print(fftd,len);
	return fftd;
}

wtk_complex_t* wtk_cfft_ifft3(wtk_cfft_t *m,wtk_complex_t *frame,int n2,float fx)
{
	wtk_rfft_t *fft=m->fft;
	wtk_complex_t *fftd=m->fftd;
	int i;
	int len=fft->len;
	float r=1.0/fft->len;

	for(i=0;i<n2;++i)
	{
		fftd[i].a=frame[i].a*fx;
		fftd[i].b=-frame[i].b*fx;
	}
	if(n2<len)
	{
		//wtk_debug("%d/%d\n",n2,fft->len);
		memset(fftd+n2,0,(len-n2)*sizeof(wtk_complex_t));
	}
	//wtk_complex_print(fftd,m->fft->len);
	//exit(0);
	fftd=wtk_cfft_fft(m,fftd);
	//wtk_complex_print(tmp,fft->len);
	//wtk_complex_print(fftd,m->fft->len);
	//exit(0);
	for(i=0;i<len;++i)
	{
		fftd[i].a*=r;
		fftd[i].b*=-r;
	}
	//wtk_complex_print(fftd,len);
	return fftd;
}


float wtk_rfft_find_best_value2(float *x1,int len,int margin,float *max_v)
{
	int idx;
	float ft;
	int i;
	int win=len/2;
	float fidx;
	int j=0;

	if(margin>win)
	{
		margin=win;
	}
	//wtk_debug("margin=%d\n",margin);
	idx=0;
	ft=x1[0];
	for(i=1;i<margin;++i)
	{
		if(x1[i]>ft)
		{
			idx=i;
			ft=x1[i];
		}
	}
	//print_float(x1,20);
	//print_float(x1+len-20,20);
	//wtk_debug("idx=%d %e\n", idx,ft);
	for(i=len-margin,j=-margin;i<len;++i,++j)
	{
		if(x1[i]>ft)
		{
			idx=j;
			ft=x1[i];
		}
	}
	//wtk_debug("idx=%d %e\n", idx,ft);
//	wtk_debug("idx=%d %e\n",idx,ft);
//	wtk_debug("margin=%d\n",margin);
//	print_float(x1,10);
//	print_float(x1+rfft->len-10,10);
	//wtk_debug("idx[%d]=%f\n",idx,ft);
	//wtk_debug("idx=%d f=%f len=%d/%d\n",idx,ft,rfft->win,rfft->len);
//	if(len>3)
//	{
//		//寻找中间值
//		if(idx>win)
//		{
//			v[0]=idx-len;
//			v[1]=x1[idx];
//			v[2]=idx-1-len;
//			v[3]=x1[idx-1];
//			if((idx+1)<len)
//			{
//				v[4]=idx+1-len;
//				v[5]=x1[idx+1];
//			}else
//			{
//				v[4]=idx-2-len;
//				v[5]=x1[idx-2];
//			}
//			fidx=wtk_rfft_find_max_value(v[0],v[1],v[2],v[3],v[4],v[5],max_v);
//			//fidx-=rfft->len;
//		}else
//		{
//			v[0]=idx;
//			v[1]=x1[idx];
//			wtk_debug("%f=%e\n",v[0],v[1]);
//			if(idx<=0)
//			{
//				v[2]=idx+2;
//				v[3]=x1[idx+2];
//			}else
//			{
//				v[2]=idx-1;
//				v[3]=x1[idx-1];
//			}
//			v[4]=idx+1;
//			v[5]=x1[idx+1];
//			fidx=wtk_rfft_find_max_value(v[0],v[1],v[2],v[3],v[4],v[5],max_v);
//		}
//		wtk_debug("fidx=%f\n",fidx);
//	}else
	{
	//	wtk_debug("ft=%f\n",ft);
		if(max_v)
		{
			*max_v=ft;//x1[idx];
		}
		fidx=idx;
	}
	return fidx;
}


float wtk_float_sum2(float *a,int n)
{
	int i;
	float f;

	f=0;
	//wtk_debug("n=%d\n",n);
	for(i=0;i<n;++i)
	{
		if(a[i]>=0)
		{
			f+=a[i];
		}else
		{
			f-=a[i];
		}
		//f+=fabs(a[i]);
	}
	//wtk_debug("f=%f nan=%d/%d\n",f,isnan(f),isnan(log(0)));
	return f;
}

float wtk_float_sum22(float *a,int n)
{
	int i;
	float f;

	f=0;
	for(i=0;i<n;++i)
	{
		f+=a[i]*a[i];
	}
	return f;
}

float wtk_short_sum2(short *a,int n)
{
	int i;
	float f;

	f=0;
	for(i=0;i<n;++i)
	{
		f+=abs(a[i]);
	}
	f/=32768.0;
	return f;
}

int wtk_short_cuts(short *a,int n)
{
	int i;
	int v;
	int cut=0;
	int vx=0;

	for(i=0;i<n;++i)
	{
		v=abs(a[i]);
		if(v>=32600)
		{
			++cut;
		}
		if(v>vx)
		{
			vx=v;
		}
	}
	//wtk_debug("vx=%d\n",vx);
	return cut;
}


int wtk_float_find_max_idx(float *pv,int n,int step,float *pf)
{
	int idx;
	int i;
	float f;
	float maxf;

	maxf=-1;
	idx=-1;
	for(i=0;i<n-step;++i)
	{
		f=wtk_float_sum2(pv+i,step);
		if(f>maxf)
		{
			idx=i;
			maxf=f;
		}
	}
	*pf=maxf;
	return idx;
}


//fft -ref_fft
void wtk_xcorr_freq_mult2(float *fft1,float *fft2,float *xy,int fft_size)
{
	int i,j;
	//float t;
	//int nx;
	//float x1;//,x2;

	//nx=fft_size<<1;
	xy[0]=fft1[0]*fft2[0];
	//t=abs(x1);
	//if(t==0){t=1;}
	//xy[0]=x1;///(t*nx);
	for(i=1,j=i+fft_size;i<fft_size;++i,++j)
	{
		//do the multiplication: (a+jb)(c+jd)* = (a+jb)*(c-jd)=(ac+bd)+j(bc-ad);
		xy[i]=fft1[i]*fft2[i]+fft1[i+fft_size]*fft2[i+fft_size];
		xy[j]=fft1[i+fft_size]*fft2[i]-fft1[i]*fft2[i+fft_size];
		//t=sqrt(x1*x1+x2*x2);
		//if(t==0){t=1;}
		//t=1.0/(t*nx);
		//xy[i]=x1;//*t;///(t*nx);
		//xy[i+fft_size]=x2*t;///(t*nx);
	}
	xy[fft_size]=fft1[fft_size]*fft2[fft_size];
	//t=abs(x1);
	//if(t==0){t=1;}
	//xy[fft_size]=x1;///(t*nx);
}


int wtk_rfft_xcorr_float2(float *mic,int mic_len,float *sp,int sp_len,float *pt)
{
	int len;
	wtk_rfft_t *rfft;
	float *x1;
	float *x2;
	float *x3;
	int i,j;
	//float ft;
	//float x;
	int margin;
	float t;
	int idx;

//	wave_write_file_float("frame.wav",48000,mic,mic_len);
//	wave_write_file_float("sp.wav",48000,sp,sp_len);
	//wtk_debug("mic=%d sp=%d\n",mic_len,sp_len);
	len=max(mic_len,sp_len);

	//wtk_debug("len=%d\n",len);
	margin=len/4;
	//margin=len;
	rfft=wtk_rfft_new(len);
	//wtk_debug("len=%d/%d\n",len,rfft->len);
	x1=(float*)wtk_calloc(rfft->len,sizeof(float));
	x2=(float*)wtk_calloc(rfft->len,sizeof(float));
	x3=(float*)wtk_calloc(rfft->len,sizeof(float));
	memcpy(x1,mic,len*sizeof(float));
	memcpy(x2,sp,len*sizeof(float));
	wtk_rfft_process_fft(rfft,x3,x1);//x3 mic fft;
	wtk_rfft_process_fft(rfft,x1,x2);//x1 sp fft;
	//print_float(x3,rfft->len);
	//exit(0);
	wtk_xcorr_freq_mult2(x3,x1,x2,rfft->win);

	//print_float(x2,rfft->len);
	//exit(0);


	wtk_rfft_process_ifft(rfft,x2,x1);

	//print_float(x1+rfft->len-800,800);//rfft->len);
	//exit(0);
	margin=len/2;
	idx=0;
	t=x1[0];
	for(i=1;i<margin;++i)
	{
		if(x1[i]>t)
		{
			idx=i;
			t=x1[i];
		}
	}
	//wtk_debug("idx=%d %e\n", idx,t);
	for(i=rfft->len-margin,j=-margin;i<rfft->len;++i,++j)
	{
		if(x1[i]>t)
		{
			idx=j;
			t=x1[i];
		}
	}
	//wtk_debug("idx=%d %e\n", idx,t);
	*pt=t;
	//wtk_debug("idx=%f f=%f len=%d/%d\n",fidx,ft,rfft->win,rfft->len);
	wtk_free(x1);
	wtk_free(x2);
	wtk_free(x3);
	wtk_rfft_delete(rfft);
	return idx;
}
