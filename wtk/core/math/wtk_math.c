#include "wtk_math.h"


void ifft(float x[], float y[], int n) //数组x存储时域序列的实部，数组y存储时域序列的虚部,n代表N点FFT.
{

	int i, j, k, l, m, n1, n2;
	float c, c1, e, s, s1, t, tr, ti;

	for (j = 1, i = 1; i < 16; i++) //计算 i = log2N；变量j进行每次自乘2，当j=n，即算出i的值.
	{
		m = i;
		j = 2 * j;
		if (j == n) //此时判断j是否与n相等时，j与2乘，所以n输入时应该选为2的次方。
			break;

	}
	n1 = n - 1; //计算蝶形图的输入下标（码位倒读）
	for (j = 0, i = 0; i < n1; i++) {
		if (i < j) {

			tr = x[j];
			ti = y[j];
			x[j] = x[i];
			y[j] = y[i];
			x[i] = tr;
			y[i] = ti;

		}
		k = n / 2;
		while (k < (j + 1)) {

			j = j - k;
			k = k / 2;

		}
		j = j + k;

	}
	n1 = 1; //计算每一级的输出，l为某一级，i为同一级的不同群，使用同一内存（即位运算）
	for (l = 1; l <= m; l++) {

		n1 = 2 * n1;
		n2 = n1 / 2;
		e = PI / n2;
		c = 1.0;
		s = 0.0;
		c1 = cos(e);
		s1 = sin(e);
		for (j = 0; j < n2; j++) {
			for (i = j; i < n; i += n1) {
				k = i + n2;
				tr = c * x[k] - s * y[k];
				ti = c * y[k] + s * x[k];
				x[k] = x[i] - tr;
				y[k] = y[i] - ti;
				x[i] = x[i] + tr;
				y[i] = y[i] + ti;

			}
			t = c;
			c = c * c1 - s * s1;
			s = t * s1 + s * c1;
		}
	}
	for (i = 0; i < n; i++) {
		x[i] /= n;
		y[i] /= n;

	}
}


void wtk_vector_zero_mean_frame(wtk_vector_t* v)
{
	float sum,off;

	sum=0;
	wtk_vector_do_p(v,sum+=,);
	off=sum/wtk_vector_size(v);
	wtk_vector_do_p(v,,-=off);
}

void wtk_vector_pre_emphasise(wtk_vector_t* v,float k)
{
	int i;

	for(i=wtk_vector_size(v);i>=2;--i)
	{
		v[i]-=v[i-1]*k;
	}
	v[1]*=1.0-k;
}

wtk_vector_t* wtk_math_create_ham_window_h(wtk_heap_t *h,int frame_size)
{
	wtk_vector_t *v;
	float a;

	v=wtk_vector_new_h(h,frame_size);
	a=TPI/(frame_size-1);
	wtk_vector_do_i(v,,=0.54-0.46*cos(a*(i-1)));
	return v;
}


void wtk_math_init_hanning_window(float *f,int n)
{
	int i;
	float t1,t2;

	t1=2*PI/(n+1);
	//wtk_debug("n=%d\n",n);
	for(i=0,t2=t1;i<n;++i,t2+=t1)
	{
		//f[i]=0.5*(1-cos(2*PI*(i+1)/(n+1)));
		f[i]=0.5*(1-cos(t2));
		//wtk_debug("v[%d]=%.12f\n",i,sqrt(f[i]));
	}
	//exit(0);
}

float* wtk_math_create_hanning_window(int n)
{
	float *f;

	f=(float*)wtk_malloc(sizeof(float)*n);
	wtk_math_init_hanning_window(f,n);
	return f;
}

float* wtk_math_create_conj_window(int len)
{
   int i;
   float tmp,x;
   int inv;
   float f;
   float *w;

   w=(float*)wtk_calloc(len,sizeof(float));
   f=4.0/len;
   for (i=0;i<len;i++)
   {
	   x=f*i;//(4.0*i)/len;
	   inv=0;
      if (x<1.0)
      {
      } else if (x<2.0)
      {
         x=2.0-x;
         inv=1;
      } else if (x<3.0)
      {
         x=x-2.0;
         inv=1;
      } else {
    	  x=4.0-x;
      }
      x*=1.271903;
      tmp=0.5-0.5*cos(0.5*PI*x);
      tmp*=tmp;
      if (inv)
      {
    	  tmp=1.0-tmp;
      }
      w[i]=sqrt(tmp);
   }
   return w;
}


float* wtk_math_create_hanning_window2(int n)
{
	float *f;
	int i;
	//float a;

	f=(float*)wtk_malloc(sizeof(float)*n);
	//wtk_debug("n=%d\n",n);
	//a=2*PI/(n-1);
	for(i=0;i<n;++i)
	{
		f[i]=0.5*(1-cos(2*PI*(i)/(n-1)));
		//f[i]=0.5*(1-cos(a*i));
		//wtk_debug("v[%d]=%.12f\n",i,sqrt(f[i]));
	}
	return f;
}

float* wtk_math_create_hanning_window_torch(int n)
{
	float *f;
	int i;
	//float a;

	f=(float*)wtk_malloc(sizeof(float)*n);
	//wtk_debug("n=%d\n",n);
	//a=2*PI/(n-1);
	for(i=0;i<n;++i)
	{
		f[i]=0.5*(1-cos(2*PI*(i)/n));
		//f[i]=0.5*(1-cos(a*i));
		//wtk_debug("v[%d]=%.12f\n",i,sqrt(f[i]));
	}
	return f;
}
wtk_vector_t* wtk_math_create_ham_window(int frame_size)
{
	wtk_vector_t *v;
	float a;

	v=wtk_vector_new(frame_size);
	a=TPI/(frame_size-1);
	wtk_vector_do_i(v,,=0.54-0.46*cos(a*(i-1)));
	return v;
}

wtk_vector_t* wtk_math_create_povey_window(int frame_size)
{
	wtk_vector_t *v;
	float a;

	v=wtk_vector_new(frame_size);
	a=TPI/(frame_size-1);
	//wtk_vector_do_i(v,,=0.54-0.46*cos(a*(i-1)));
	wtk_vector_do_i(v,,=pow(0.5-0.5*cos(a*(i-1)),0.85));
	return v;
}

float* wtk_math_create_bartlett_window(int win)
{
	float *f;
	int i;

	f=(float*)wtk_malloc(win*sizeof(float));
	for(i=0;i<win;++i)
	{
		f[i]=(2.0/(win-1))*((win-1)/2 - fabs((double)(i-(win-1)/2)));
	}
	//print_float(f,10);
	return f;
}

float* wtk_math_create_bartlett_hann_window(int win)
{
	float *f;
	int i;

	f=(float*)wtk_malloc(win*sizeof(float));
	for(i=0;i<win;++i)
	{
		f[i]=0.62 - 0.48*(i*1.0/(win-1)-0.5) - 0.38*cos(2*PI*i*1.0/(win-1));
	}
	//print_float(f,10);
	return f;
}


float* wtk_math_create_blackman_window(int win)
{
	float *f;
	int i;

	f=(float*)wtk_malloc(win*sizeof(float));
	for(i=0;i<win;++i)
	{
		f[i]=0.42 - 0.5*cos(2*PI*i*1.0/(win-1)) + 0.08*cos(4*PI*i*1.0/(win-1));
	}
	//print_float(f,10);
	return f;
}

void wtk_math_init_blackman_window(float *f,int n)
{
	int i;

	for(i=0;i<n;++i)
	{
		f[i]=0.42 - 0.5*cos(2*PI*i*1.0/(n-1)) + 0.08*cos(4*PI*i*1.0/(n-1));
	}
}

/*
 * win=sin((.5:wlen-.5)/wlen*pi).';
 */
float * wtk_math_create_sine_window(int win)
{
	float *f;
	int i;

	f=(float*)wtk_malloc(win*sizeof(float));
	for(i=0;i<win;++i)
	{
		f[i]=sin((0.5+i)*PI/(win));
	}
	//print_float(f,10);
	return f;
}

float* wtk_math_create_float_ham_window(int frame_size)
{
	float *v;
	int i;
	//float a;

	v=(float*)wtk_malloc(frame_size*sizeof(float));
	//a=2*PI/(frame_size-1);
	for(i=0;i<frame_size;++i)
	{
		v[i]=0.54-0.46*cos(2*PI*(i)/(frame_size-1));
		//v[i]=0.54-0.46*cos(a*i);
		//v[i]=0.53836-0.46164*cos(2*PI*(i)/(frame_size-1));
		//v[i]=0.53836-0.46164*cos(a*i);
	}
	return v;
}

void wtk_fft(wtk_vector_t* s, int invert)
{
   int ii,jj,n,nn,limit,m,j,inc,i;
   double wx,wr,wpr,wpi,wi,theta;
   double xre,xri,x;

   n=wtk_vector_size(s);
   nn=n / 2; j = 1;
   for (ii=1;ii<=nn;ii++) {
      i = 2 * ii - 1;
      if (j>i) {
         xre = s[j]; xri = s[j + 1];
         s[j] = s[i];  s[j + 1] = s[i + 1];
         s[i] = xre; s[i + 1] = xri;
      }
      m = n / 2;
      while (m >= 2  && j > m) {
         j -= m; m /= 2;
      }
      j += m;
   };
   limit = 2;
   while (limit < n) {
      inc = 2 * limit; theta = TPI / limit;
      if (invert) theta = -theta;
      x = sin(0.5 * theta);
      wpr = -2.0 * x * x; wpi = sin(theta);
      wr = 1.0; wi = 0.0;
      for (ii=1; ii<=limit/2; ii++) {
         m = 2 * ii - 1;
         for (jj = 0; jj<=(n - m) / inc;jj++) {
            i = m + jj * inc;
            j = i + limit;
            xre = wr * s[j] - wi * s[j + 1];
            xri = wr * s[j + 1] + wi * s[j];
            s[j] = s[i] - xre; s[j + 1] = s[i + 1] - xri;
            s[i] = s[i] + xre; s[i + 1] = s[i + 1] + xri;
         }
         wx = wr;
         wr = wr * wpr - wi * wpi + wr;
         wi = wi * wpr + wx * wpi + wi;
      }
      limit = inc;
   }
   if (invert)
      for (i = 1;i<=n;i++)
         s[i] = s[i] / nn;

}


void wtk_realft (wtk_vector_t* s)
{
   int n, n2, i, i1, i2, i3, i4;
   double xr1, xi1, xr2, xi2, wrs, wis;
   double yr, yi, yr2, yi2, yr0, theta, x;

   n=wtk_vector_size(s) / 2; n2 = n/2;
   theta = PI / n;
   wtk_fft(s, 0);
   //print_float(s+1,10);
   //exit(0);
   x = sin(0.5 * theta);
   yr2 = -2.0 * x * x;
   yi2 = sin(theta); yr = 1.0 + yr2; yi = yi2;
   for (i=2; i<=n2; i++) {
      i1 = i + i - 1;      i2 = i1 + 1;
      i3 = n + n + 3 - i2; i4 = i3 + 1;
      wrs = yr; wis = yi;
      xr1 = (s[i1] + s[i3])/2.0; xi1 = (s[i2] - s[i4])/2.0;
      xr2 = (s[i2] + s[i4])/2.0; xi2 = (s[i3] - s[i1])/2.0;
      s[i1] = xr1 + wrs * xr2 - wis * xi2;
      s[i2] = xi1 + wrs * xi2 + wis * xr2;
      s[i3] = xr1 - wrs * xr2 + wis * xi2;
      s[i4] = -xi1 + wrs * xi2 + wis * xr2;
      yr0 = yr;
      yr = yr * yr2 - yi  * yi2 + yr;
      yi = yi * yr2 + yr0 * yi2 + yi;
   }
   xr1 = s[1];
   s[1] = xr1 + s[2];
   s[2] = 0.0;
}

void wtk_math_do_diff(wtk_vector_t** pv,int window_size,double sigma,int start_pos,int step)
{
	int i,j,k,end=start_pos+step;
	wtk_vector_t *v=pv[window_size];
	wtk_vector_t *p,*n;
	int vs=start_pos-step;
	/*
	double sigma;

	sigma=0;
	for(i=1;i<=window_size;++i)
	{
		sigma+=i*i;
	}
	sigma*=2;
	*/
	for(i=1;i<=window_size;++i)
	{
		p=pv[window_size-i];
		n=pv[window_size+i];
		for(j=start_pos,k=vs;j<end;++j,++k)
		{
			if(i==1)
			{
				v[j]=(n[k]-p[k]);
			}else
			{
				v[j]+=i*(n[k]-p[k]);
			}
			if(i==window_size)
			{
				v[j]/=sigma;
			}
		}
	}
}

/**
 *	dt=(C{t+w}-C{t-w))/2w
 */
void wtk_math_do_simple_diff(wtk_vector_t** pv,int window_size,int start_pos,int step)
{
	int j,k,end=start_pos+step;
	wtk_vector_t *v=pv[window_size];
	wtk_vector_t *p,*n;
	int vs=start_pos-step;
	int dw;

	dw=window_size<<1;
	p=pv[0];
	n=pv[dw];
	for(j=start_pos,k=vs;j<end;++j,++k)
	{
		v[j]=(n[k]-p[k])/dw;
	}
}


int wtk_source_read_vector(wtk_source_t* s,wtk_vector_t* v,int bin)
{
	return wtk_source_read_float(s,v+1,wtk_vector_size(v),bin);
}

int wtk_source_read_vector_little(wtk_source_t* s,wtk_vector_t* v,int bin)
{
	return wtk_source_read_float_little(s,v+1,wtk_vector_size(v),bin);
}

int wtk_source_read_matrix(wtk_source_t *s,wtk_matrix_t *m,int bin)
{
	int i,nrows,ret=0;

	//wtk_debug("row=%d col=%d\n",wtk_matrix_rows(m),wtk_matrix_cols(m));
	nrows=wtk_matrix_rows(m);
	for(i=1;i<=nrows;++i)
	{
		//wtk_debug("col=%d\n",wtk_vector_size(m[i]));
		ret=wtk_source_read_vector(s,m[i],bin);
		if(ret!=0)
		{
			wtk_debug("%d,sm=%d\n",ret,wtk_vector_size(m[i]));
			goto end;
		}
	}
end:
	return ret;
}

int wtk_source_read_matrix_little(wtk_source_t *s,wtk_matrix_t *m,int bin)
{
	int i,nrows,ret=0;

	//wtk_debug("row=%d col=%d\n",wtk_matrix_rows(m),wtk_matrix_cols(m));
	nrows=wtk_matrix_rows(m);
	for(i=1;i<=nrows;++i)
	{
		//wtk_debug("col=%d\n",wtk_vector_size(m[i]));
		ret=wtk_source_read_vector_little(s,m[i],bin);
		if(ret!=0)
		{
			wtk_debug("%d,sm=%d\n",ret,wtk_vector_size(m[i]));
			goto end;
		}
	}
end:
	return ret;
}

int wtk_source_read_double_vector(wtk_source_t* s,wtk_double_vector_t* v,int bin)
{
	return wtk_source_read_double_bin(s,v+1,wtk_vector_size(v),bin);
}

int wtk_source_read_double_matrix(wtk_source_t *s,wtk_double_matrix_t *m,int bin)
{
	int i,nrows,ret=0;

	//wtk_debug("row=%d col=%d\n",wtk_matrix_rows(m),wtk_matrix_cols(m));
	nrows=wtk_matrix_rows(m);
	for(i=1;i<=nrows;++i)
	{
		//wtk_debug("col=%d\n",wtk_vector_size(m[i]));
		ret=wtk_source_read_double_vector(s,m[i],bin);
		if(ret!=0)
		{
			wtk_debug("%d,sm=%d\n",ret,wtk_vector_size(m[i]));
			goto end;
		}
	}
end:
	return ret;
}

int wtk_source_read_double_vector_little(wtk_source_t* s,wtk_double_vector_t* v,int bin)
{
        return wtk_source_read_double_little(s,v+1,wtk_vector_size(v),bin);
}

int wtk_source_read_double_matrix_little(wtk_source_t *s,wtk_double_matrix_t *m,int bin)
{
        int i,nrows,ret=0;

        //wtk_debug("row=%d col=%d\n",wtk_matrix_rows(m),wtk_matrix_cols(m));
        nrows=wtk_matrix_rows(m);
        for(i=1;i<=nrows;++i)
        {
                //wtk_debug("col=%d\n",wtk_vector_size(m[i]));
                ret=wtk_source_read_double_vector_little(s,m[i],bin);
                if(ret!=0)
                {
                        wtk_debug("%d,sm=%d\n",ret,wtk_vector_size(m[i]));
                        goto end;
                }
        }
end:
        return ret;
}

int wtk_source_read_hlda_bin(wtk_matrix_t **pm,wtk_source_t *s)
{
	wtk_matrix_t *m;
	int r_c[2];
	int ret;
	int i;

	//wtk_debug("read bin\n");
	s->swap=0;
	ret=wtk_source_read_int(s,r_c,2,1);
	if(ret!=0){goto end;}
	//wtk_debug("[%d/%d]\n",r_c[0],r_c[1]);
	//m=wtk_matrix_new(r_c[0],r_c[1]);
	m=wtk_matrix_new2(r_c[0],r_c[1]);
	//wtk_debug("[%d,%d]\n",wtk_matrix_rows(m),wtk_matrix_cols(m));
	for(i=1;i<=r_c[0];++i)
	{
		ret=wtk_source_read_vector(s,m[i],1);
		if(ret!=0){goto end;}
	}
	*pm=m;
	//wtk_matrix_print(m);
	//exit(0);
	ret=0;
end:
	//wtk_debug("ret=%d\n",ret);
	//exit(0);
	return ret;
}

int wtk_source_read_hlda(wtk_source_t *s,wtk_matrix_t **pm)
{
	wtk_strbuf_t *buf;
	int ret;
	int row_col[2];
	wtk_matrix_t *m=0;

	buf=wtk_strbuf_new(64,1);
	while(1)
	{
		ret=wtk_source_read_string(s,buf);
		if(ret!=0){goto end;}
		if(wtk_str_equal_s(buf->data,buf->pos,"<XFORM>"))
		{
			break;
		}
	}
	ret=wtk_source_read_int(s,row_col,2,0);
	if(ret!=0){goto end;}
	m=wtk_matrix_new(row_col[0],row_col[1]);
	ret=wtk_source_read_matrix(s,m,0);
	//wtk_matrix_print(m);
	//exit(0);
end:
	if(ret==0)
	{
		*pm=m;
	}else
	{
		if(m)
		{
			wtk_matrix_delete(m);
		}
	}
	wtk_strbuf_delete(buf);
	return ret;
}

int wtk_hlda_read(wtk_matrix_t **pm,wtk_source_t *s)
{
	return wtk_source_read_hlda(s,pm);
}


void wtk_matrix_multiply_vector(wtk_vector_t *dst,wtk_matrix_t *m,wtk_vector_t *src)
{
	int rows,cols,i,j;
	float vi;
	float *mi;

	rows=wtk_matrix_rows(m);
	cols=wtk_matrix_cols(m);
	for(i=1;i<=rows;++i)
	{
		vi=0;mi=m[i];
		for(j=1;j<=cols;++j)
		{
			vi+=mi[j]*src[j];
		}
		dst[i]=vi;
	}
}


int wtk_floatfix_q(float max,float min,int n)
{
	float f;
	int i;

	n-=1;
	for(i=0;i<n;++i)
	{
		f=pow(2,i);
		//wtk_debug("v[%d]=%f(%f/%f)\n",i,f,cfg->min,cfg->max);
		if(f>max && f>-min)
		{
			//wtk_debug("v[%d]=%f(%f/%f)\n",i,f,cfg->min,cfg->max);
			break;
		}
	}
	return n-i;
}

float wtk_float_min(float *a,int n)
{
	int i;
	float f;

	f=a[0];
	for(i=1;i<n;++i)
	{
		if(a[i]<f)
		{
			f=a[i];
		}
	}
	return f;
}

float wtk_float_max(float *a,int n)
{
	int i;
	float f;

	f=a[0];
	for(i=1;i<n;++i)
	{
		if(a[i]>f)
		{
			f=a[i];
		}
	}
	return f;
}

int wtk_float_argmax(float *a,int n)
{
	int i;
	float f;
	int ret = 0;

	f=a[0];
	ret = 0;
	for(i=1;i<n;++i)
	{
		if(a[i]>f)
		{
			f=a[i];
			ret = i;
		}
	}
	return ret;
}

float wtk_float_sum(float *a,int n)
{
	int i;
	float f;

	f=0;
	for(i=0;i<n;++i)
	{
		f+=a[i];
	}
	return f;
}

double wtk_double_sum(double *a,int n)
{
	int i;
	double f;

	f=0;
	for(i=0;i<n;++i)
	{
		f+=a[i];
	}
	return f;
}

float wtk_int_sum(int *a,int n)
{
	int i;
	float f;

	f=0;
	for(i=0;i<n;++i)
	{
		f+=a[i];
	}
	return f;
}

float wtk_int_power(int *a,int n)
{
	int i;
	float f;

	f=0;
	for(i=0;i<n;++i)
	{
		f+=a[i]>0?a[i]:-a[i];
	}
	return f;
}

float wtk_short_power(short *a,int n)
{
	int i;
	float f;

	f=0;
	for(i=0;i<n;++i)
	{
		f+=a[i]>0?a[i]:-a[i];
	}
	return f;
}



void wtk_float_mul(float *a,int n,float f)
{
	int i;

	if(f==1.0){return;}
	for(i=0;i<n;++i)
	{
		a[i]*=f;
	}
}

int wtk_short_max(short *pv,int len)
{
	int max=pv[0];
	int i;

	for(i=1;i<len;++i)
	{
		if(pv[i]>max)
		{
			max=pv[i];
		}
	}
	return max;
}

int wtk_short_abs_max(short *pv,int len)
{
	int max=abs(pv[0]);
	int i;
	int m;

	for(i=1;i<len;++i)
	{
		m=abs(pv[i]);
		if(m>max)
		{
			max=m;
		}
	}
	return max;
}

float wtk_short_energy(short *pv,int len)
{
	float t=0;
	int i;

	for(i=0;i<len;++i)
	{
		t+=pv[i]*pv[i];
	}
	return t/len;
}


float wtk_short_abs_sum(short *pv,int len)
{
	float t=0;
	int i;

	for(i=0;i<len;++i)
	{
		if(pv[i]>0)
		{
			t+=pv[i];
		}else
		{
			t-=pv[i];
		}
		//t+=fabs(pv[i]);
	}
	return t;
}

float wtk_short_abs_mean(short *pv,int len)
{
	float t=0;
	int i;

	for(i=0;i<len;++i)
	{
		if(pv[i]>0)
		{
			t+=pv[i];
		}else
		{
			t-=pv[i];
		}
	}
	return t/len;
}

float wtk_float_abs_mean(float *pv,int len)
{
	float t=0;
	int i;

	for(i=0;i<len;++i)
	{
		if(pv[i]>0)
		{
			t+=pv[i];
		}else
		{
			t-=pv[i];
		}
	}
	return t/len;
}

float wtk_int_abs_mean(int *pv,int len)
{
	float t=0;
	int i;

	for(i=0;i<len;++i)
	{
		if(pv[i]>=0)
		{
			t+=pv[i];
		}else
		{
			t-=pv[i];
		}
	}
	return t/len;
}

float wtk_float_mean(float *pv,int len)
{
	float t=0;
	int i;

	for(i=0;i<len;++i)
	{
		t+=pv[i];
	}
	return t/len;
}


float wtk_short_sum(short *pv,int len)
{
	float t=0;
	int i;

	for(i=0;i<len;++i)
	{
		t+=pv[i];
	}
	return t;
}

float wtk_float_energy(float *a,int n)
{
	float f;
	int i;

	f=0;
	for(i=0;i<n;++i)
	{
		f+=a[i]*a[i];
	}
	return f/n;
}

float wtk_float_abs_max(float *a,int n)
{
	int i;
	float f,t;

	//f=fabs(a[0]);
	f=a[0]>0?a[0]:-a[0];
	for(i=1;i<n;++i)
	{
		t=a[i]>0?a[i]:-a[i];
		if(t>f)
		{
			f=t;
		}
	}
	return f;
}

float wtk_float_median(float *a, int n) {
    // 使用冒泡排序对数组进行排序
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (a[j] > a[j + 1]) {
                float temp = a[j];
                a[j] = a[j + 1];
                a[j + 1] = temp;
            }
        }
    }

    if (n % 2 == 0) {
        // 数据个数为偶数，取中间两个数的平均值
        int mid = n / 2;
        return (a[mid - 1] + a[mid]) / 2;
    } else {
        // 数据个数为奇数，取中间位置的数
        return a[n / 2];
    }
}

int wtk_float_median_index(float *a,int n) {
	// 使用冒泡排序对数组进行排序
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (a[j] > a[j + 1]) {
                float temp = a[j];
                a[j] = a[j + 1];
                a[j + 1] = temp;
            }
        }
    }

    if (n % 2 == 0) {
        // 数据个数为偶数，取中间两个数的平均值
        int mid = n / 2;
        return mid;
    } else {
        // 数据个数为奇数，取中间位置的数
        return n / 2;
    }
}

float wtk_float_weighted_median(float *a, float *weight, int n) {
    // 冒泡排序：对 a 数组和对应的 weight 数组按 a 数组元素值从小到大排序
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (a[j] > a[j + 1]) {
                // 交换 a 数组元素
                float temp_a = a[j];
                a[j] = a[j + 1];
                a[j + 1] = temp_a;
                // 同时交换 weight 数组元素，保证权重与数据对应关系不变
                float temp_weight = weight[j];
                weight[j] = weight[j + 1];
                weight[j + 1] = temp_weight;
            }
        }
    }

    // 计算总权重
    float total_weight = 0;
    for (int i = 0; i < n; i++) {
        total_weight += weight[i];
    }

    // 累加权重，寻找加权中位数
    float cumulative_weight = 0;
    for (int i = 0; i < n; i++) {
        cumulative_weight += weight[i];
        if (cumulative_weight >= total_weight / 2) {
            return a[i];
        }
    }

    return -1;
}

float wtk_float_mode(float *a, int n) {
    if (n == 0) {
        return 0; // 处理数组为空的情况
    }

    float mode = a[0];
    int max_count = 0;

    for (int i = 0; i < n; i++) {
        int current_count = 0;
        for (int j = 0; j < n; j++) {
            if (a[i] == a[j]) {
                current_count++;
            }
        }
        if (current_count > max_count) {
            max_count = current_count;
            mode = a[i];
        }
    }

    return mode;
}

int wtk_int_mode(int *a, int n) {
    if (n == 0) {
        return 0; // 处理数组为空的情况
    }

    int mode = a[0];
    int max_count = 0;

    for (int i = 0; i < n; i++) {
        int current_count = 0;
        for (int j = 0; j < n; j++) {
            if (a[i] == a[j]) {
                current_count++;
            }
		}
        if (current_count > max_count) {
            max_count = current_count;
            mode = a[i];
        }
    }

    return mode;
}

int wtk_int_cnt_mode(int *a, int n, int cnt) {
    if (n == 0) {
        return 0; // 处理数组为空的情况
    }

    int mode = a[0];
    int max_count = 0;

    for (int i = 0; i < n; i++) {
        int current_count = 0;
        for (int j = 0; j < n; j++) {
            if (a[i] == a[j]) {
                current_count++;
            }
		}
        if (current_count > max_count) {
            max_count = current_count;
            mode = a[i];
        }
    }

	if(max_count < cnt) {
		return 0;
	}

    return mode;
}

void wtk_float_scale(float *a,register float f,float *c,int n)
{
	float *e,*e2;

	e=c+n;
	e2=c+((n>>3)<<3);
	while(c<e2)
	{
		c[0]=a[0]*f;
		c[1]=a[1]*f;
		c[2]=a[2]*f;
		c[3]=a[3]*f;
		c[4]=a[4]*f;
		c[5]=a[5]*f;
		c[6]=a[6]*f;
		c[7]=a[7]*f;
		c+=8;
		a+=8;
	}
	while(c<e)
	{
		*(c++)=(*(a++))*f;
	}
}

void wtk_short_scale(short *a,register float f,float *c,int n)
{
	float *e,*e2;

	e=c+n;
	e2=c+((n>>3)<<3);
	while(c<e2)
	{
		c[0]=a[0]*f;
		c[1]=a[1]*f;
		c[2]=a[2]*f;
		c[3]=a[3]*f;
		c[4]=a[4]*f;
		c[5]=a[5]*f;
		c[6]=a[6]*f;
		c[7]=a[7]*f;
		c+=8;
		a+=8;
	}
	while(c<e)
	{
		*(c++)=(*(a++))*f;
	}
}

void wtk_short_scale_add(short *a,register float f,float *c,int n)
{
	float *e,*e2;

	e=c+n;
	e2=c+((n>>3)<<3);
	while(c<e2)
	{
		c[0]+=a[0]*f;
		c[1]+=a[1]*f;
		c[2]+=a[2]*f;
		c[3]+=a[3]*f;
		c[4]+=a[4]*f;
		c[5]+=a[5]*f;
		c[6]+=a[6]*f;
		c[7]+=a[7]*f;
		c+=8;
		a+=8;
	}
	while(c<e)
	{
		*(c++)+=(*(a++))*f;
	}
}

void wtk_float_scale_add(float *a,register float f,float *c,int n)
{
	float *e,*e2;

	e=c+n;
	e2=c+((n>>3)<<3);
	while(c<e2)
	{
		c[0]+=a[0]*f;
		c[1]+=a[1]*f;
		c[2]+=a[2]*f;
		c[3]+=a[3]*f;
		c[4]+=a[4]*f;
		c[5]+=a[5]*f;
		c[6]+=a[6]*f;
		c[7]+=a[7]*f;
		c+=8;
		a+=8;
	}
	while(c<e)
	{
		*(c++)+=(*(a++))*f;
	}
}

//a*b=c;
void wtk_float_mult(float *a,float *b,float *c,int n)
{
	float *e,*e2;

	e=c+n;
	e2=c+((n>>3)<<3);
	while(c<e2)
	{
		c[0]=a[0]*b[0];
		c[1]=a[1]*b[1];
		c[2]=a[2]*b[2];
		c[3]=a[3]*b[3];
		c[4]=a[4]*b[4];
		c[5]=a[5]*b[5];
		c[6]=a[6]*b[6];
		c[7]=a[7]*b[7];
		c+=8;
		a+=8;
		b+=8;
	}
	while(c<e)
	{
		*(c++)=(*(a++))*(*(b++));
	}
}

//c+=a*b;
void wtk_float_mult_add(float *a,float *b,float *c,int n)
{
	float *e,*e2;

	e=c+n;
	e2=c+((n>>3)<<3);
	while(c<e2)
	{
		c[0]+=a[0]*b[0];
		c[1]+=a[1]*b[1];
		c[2]+=a[2]*b[2];
		c[3]+=a[3]*b[3];
		c[4]+=a[4]*b[4];
		c[5]+=a[5]*b[5];
		c[6]+=a[6]*b[6];
		c[7]+=a[7]*b[7];
		c+=8;
		a+=8;
		b+=8;
	}
	while(c<e)
	{
		*(c++)+=(*(a++))*(*(b++));
	}
}

void wtk_relu(float* a,float* alpha,float* beta,int len)
{
	float *p,*e,*al,*be;
	al=alpha;be=beta;
	p=a;e=p+len;
	while(p<e)
	{
		//wtk_debug("%f\n",*p)
		if(*p>0)
		{
			*p=(*p)*(*al);
			//wtk_debug(">0 %f\n",*p);
		}else
		{
			*p=abs((int)((*p)*(*be)));
			//wtk_debug("<0 %f\n",*p);
		}
		++p;
		++al;
		++be;
	}
	//exit(0);
}

void wtk_relu2(float* a,int len)
{
	float *p,*e;
	p=a;e=p+len;
	while(p<e)
	{
		//wtk_debug("%f\n",*p)
		if(*p<0)
		{
			*p=0.0;
		}
		++p;
	}
	//exit(0);
}

void wtk_raw_relu(float* a,int len)
{
	float *p,*e;
	p=a;e=p+len;
	while(p<e)
	{
		if(*p<0)
		{
			*p=0;
		}
		++p;
	}
}

void wtk_softmax(float* a,int len)
{
	float max,sum;
	float *p,*e;

	max=wtk_math_max(a,len);
	sum=0;
	p=a;e=p+len;
	while(p<e)
	{
		*p=expf(*p-max);
		sum+=*p;
		++p;
	}
	sum=1.0f/sum;
	p=a;e=p+len;
	while(p<e)
	{
		*p*=sum;
		++p;
	}
}

void wtk_torch_softmax(float* a,int len)
{
	float sum;
	float *p,*e;
	sum=0;
	p=a;e=p+len;
	while(p<e)
	{
		*p=expf(*p);
		sum+=*p;
		++p;
	}
	sum=1.0f/sum;
	p=a;e=p+len;
	while(p<e)
	{
		*p*=sum;
		++p;
	}
}

void wtk_add_log(float *p,int n)
{
	float *pe;

	pe=p+n;
	while(p<pe)
	{
		*p=log(*p);
		++p;
	}
}

void wtk_sigmoid(float *f,int len)
{
	float *p;
	float *e;

	p=f;e=p+len;
	while(p<e)
	{
		*p=1.0/(1.0+expf(-*p));
		++p;
	}
}

void wtk_tanh(float *f,int len)
{
	float *p;
	float *e;
	float inv_expx,expx;
	p=f;e=p+len;
	while(p<e)
	{
		if(*p>0.0)
		{
			inv_expx=expf(-(*p));
			*p=-1.0+2.0/(1.0+inv_expx*inv_expx);
		}else
		{
			expx=expf(*p);
			*p=1.0-2.0/(1.0+expx*expx);
		}
		++p;
	}
}

void wtk_pnorm(float *f,float *f2,int len,int next_len)
{
	int group=len/next_len;
	int i;
	float *p,*p2,*e;
	float sum=0.0,y;

	p=f;e=p+len;
	p2=f2;

	while(p<e)
	{
		for(i=0;i<group;i++)
		{
			sum+=(*p)*(*p);
			++p;
		}
//		wtk_debug("%p\n",p2);
		*p2=pow(sum,0.5);
		sum=0.0;
		++p2;
	}

	sum=0.0;
	p2=f2;e=p2+next_len;
	while(p2<e)
	{
		sum+=*p2*(*p2);
		++p2;
	}
	y=pow(sum/next_len,0.5);
	//wtk_debug("%f\n",y);
	p2=f2;e=p2+next_len;
	if(y>1)
	{
		while(p2<e)
		{
			*p2*=1/y;
			++p2;
		}
	}
}

void wtk_normalize(float *f,int len)
{
	float *p,*e;
	float y,sum;

	sum=0;
	p=f;e=p+len;
	while(p<e)
	{
		sum+=*p*(*p);
		++p;
	}
	y=pow(sum*1.0/len,0.5);
	p=f;e=p+len;
	if(y>1)
	{
		while(p<e)
		{
			*p*=1/y;
			++p;
		}
	}
}

float wtk_scalar_sigmoid(float a)
{
	if(a>0)
	{
		return 1.0/(1.0+expf(-a));
	}else
	{
		float x=expf(a);
		return x/(x+1.0);
	}
}

float wtk_scalar_tanh(float a)
{
	if(a>0)
	{
		float inv_expa=expf(-a);
		return -1.0+2.0/(1.0+inv_expa*inv_expa);
	}else
	{
		float expa=expf(a);
		return 1.0-2.0/(1.0+expa*expa);
	}
}

float wtk_float_max2(float *a,int n,float add)
{
	int i;
	float f,t;

	f=a[0];
	for(i=1;i<n;++i)
	{
		t=a[i];
		if(t>f)
		{
			f=t;
		}
	}
	f+=add;
	return f;
}

void wtk_float_clamp(float *a,int n,int use_min, float min, int use_max, float max)
{
	int i = 0;
	for(i = 0; i < n; ++i){
		if(use_min && a[i] < min){
			a[i] = min;
		}else if(use_max && a[i] > max){
			a[i] = max;
		}
	}
	return;
}

float* wtk_math_create_win(char *win,int len)
{
	if(strcmp(win,"ham")==0 || strcmp(win,"hamming")==0)
	{
		return wtk_math_create_float_ham_window(len);
	}else if(strcmp(win,"hann")==0 || strcmp(win,"hanning")==0)
	{
		return wtk_math_create_hanning_window2(len);
	}else if(strcmp(win,"hann2")==0)
	{
		return wtk_math_create_hanning_window(len);
	}else if(strcmp(win,"conj")==0)
	{
		return wtk_math_create_conj_window(len);
	}else if(strcmp(win,"sine")==0)
	{
		return wtk_math_create_sine_window(len);
	}
	return  NULL;
}

float wtk_float_std(float *pv, int n)
{
	int i;
	float f;
	float tmp;
	float mean = wtk_float_mean(pv, n);

	f = 0;	
	for (i=0;i<n;++i){
		tmp = pv[i] - mean;
		f += tmp * tmp;
	}
	f /= n;
	return f;
}

int wtk_math_rand_r(unsigned int *seed)
{
#ifdef QTK_RAND_R
	return rand_r(seed);
#else
	// printf("Please confirm that there is no rand_r in stdlib.h\nor else define QTK_RAND_R\n");
	unsigned int next = *seed;
	int result;

	next *= 1103515245;
	next += 12345;
	result = (unsigned int) (next / 65536) % 2048;

	next *= 1103515245;
	next += 12345;
	result <<= 10;
	result ^= (unsigned int) (next / 65536) % 1024;

	next *= 1103515245;
	next += 12345;
	result <<= 10;
	result ^= (unsigned int) (next / 65536) % 1024;

	*seed = next;

	return result;
#endif
}

void wtk_float_add_dither(float *data, int len) {
    unsigned seed = 0;
    int i = 0;
    float dither = 1.0;

    seed = rand() + 27437;
    for (i = 0; i < len; ++i) {
        data[i] +=
            dither *
            sqrt(-2 * log((wtk_math_rand_r(&seed) + 1.0) / (RAND_MAX + 2.0))) *
            cos(2 * M_PI * (wtk_math_rand_r(&seed) + 1.0) / (RAND_MAX + 2.0));
    }
}

float wtk_float_clip(float value, float min, float max) {
    if (value < min) {
        return min;
    } else if (value > max) {
        return max;
    } else {
        return value;
    }
}

float wtk_logaddexp(float x,float y){
	if(x == y){
		return x + QLOGE2;
	}else{
		float tmp = x - y;

		if(tmp > 0){
			return x + log1p(exp(-tmp));
		}else if(tmp <= 0){
			return y + log1p(exp(tmp));
		}else{
			return tmp;
		}
	}
}

int qtk_gcd(int m, int n) {
    if (m == 0 || n == 0) {
        return m == 0 ? (n > 0 ? n : -1) : (m > 0 ? m : -m);
    }
    while (1) {
        m %= n;
        if (m == 0)
            return n > 0 ? n : -n;
        n %= m;
        if (n == 0)
            return m > 0 ? m : -m;
    }
}

int qtk_lcm(int m, int n) {
    int gcd;

    gcd = qtk_gcd(m, n);
    return gcd * (m / gcd) * (n / gcd);
}
