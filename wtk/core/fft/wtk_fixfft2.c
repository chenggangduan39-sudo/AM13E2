#include "wtk_fixfft2.h" 

wtk_fixfft2_t* wtk_fixfft2_new(int N)
{
# define M_PI		3.14159265358979323846	/* pi */
	wtk_fixfft2_t *fft;
	int i,n;
	double a;

	fft=(wtk_fixfft2_t*)wtk_malloc(sizeof(wtk_fixfft2_t));
	fft->N=N;
	fft->order=(int)ceil(log(N)/log(2.0));
	n=N/4;
	fft->ccc=(wtk_fix2_t*)wtk_calloc(n,sizeof(wtk_fix2_t));
	fft->sss=(wtk_fix2_t*)wtk_calloc(n,sizeof(wtk_fix2_t));
	for(i=0;i<n;++i)
	{
		a = 2 * M_PI * i/N;
		fft->ccc[i] = FLOAT2COS(cos(a));
        fft->sss[i] = FLOAT2COS(sin(a));
		//wtk_debug("v[%d]=%f+%f\n",i,cos(a),sin(a));
	}
	//need more optimize
	fft->iccc=(wtk_fix2_t*)wtk_calloc(fft->order,sizeof(wtk_fix2_t));
	fft->isss=(wtk_fix2_t*)wtk_calloc(fft->order,sizeof(wtk_fix2_t));
	for(i=1,n=i;i<=fft->order;++i,n*=2)
	{
		a=M_PI/n;
		fft->iccc[i-1]=FLOAT2COS(cos(a));
		fft->isss[i-1]=FLOAT2COS(sin(a));
		//wtk_debug("v[%d]=%f+%f\n",i,cos(a),sin(a));
	}
	//fft->tr=(wtk_fix2_t*)wtk_calloc(N,sizeof(wtk_fix2_t));
	fft->ti=(wtk_fix2_t*)wtk_calloc(N,sizeof(wtk_fix2_t));
	return fft;
}

void wtk_fixfft2_delete(wtk_fixfft2_t *fft)
{
	//wtk_free(fft->tr);
	wtk_free(fft->ti);
	wtk_free(fft->iccc);
	wtk_free(fft->isss);
	wtk_free(fft->sss);
	wtk_free(fft->ccc);
	wtk_free(fft);
}



/*
 * libsphinxbase
 */
void wtk_fixfft2_fft(wtk_fixfft2_t *fft,wtk_fix2_t *x)
{
	int i,j,k;
	int n=fft->N;
	int m=fft->order;
    int n1, n2, n4,n5;
	wtk_fix2_t xt;
	wtk_fix2_t cc, ss, t1, t2;
    int i1, i2, i3, i4;
    wtk_fix2_t *ccc=fft->ccc;
    wtk_fix2_t *sss=fft->sss;

    /* Bit-reverse the input. */
    j = 0;
    n1=n-1;
    for (i = 0; i < n1; ++i)
    {
        if (i < j)
        {
            xt = x[j];
            x[j] = x[i];
            x[i] = xt;
        }
        k = n / 2;
        while (k <= j)
        {
            j -= k;
            k /= 2;
        }
        j += k;
    }
    /* Basic butterflies (2-point FFT, real twiddle factors):
     * x[i]   = x[i] +  1 * x[i+1]
     * x[i+1] = x[i] + -1 * x[i+1]
     */
    for (i = 0; i < n; i += 2)
    {
        xt = x[i];
        x[i] = (xt + x[i + 1]);
        x[i + 1] = (xt - x[i + 1]);
    }

    /* The rest of the butterflies, in stages from 1..m */
    for (k = 1; k < m; ++k)
    {
        n4 = k - 1;
        n2 = k;
        n1 = k + 1;
        /* Stride over each (1 << (k+1)) points */
        for (i = 0; i < n; i += (1 << n1))
        {
            /* Basic butterfly with real twiddle factors:
             * x[i]          = x[i] +  1 * x[i + (1<<k)]
             * x[i + (1<<k)] = x[i] + -1 * x[i + (1<<k)]
             */
            xt = x[i];
            x[i] = (xt + x[i + (1 << n2)]);
            x[i + (1 << n2)] = (xt - x[i + (1 << n2)]);

            /* The other ones with real twiddle factors:
             * x[i + (1<<k) + (1<<(k-1))]
             *   = 0 * x[i + (1<<k-1)] + -1 * x[i + (1<<k) + (1<<k-1)]
             * x[i + (1<<(k-1))]
             *   = 1 * x[i + (1<<k-1)] +  0 * x[i + (1<<k) + (1<<k-1)]
             */
            x[i + (1 << n2) + (1 << n4)] = -x[i + (1 << n2) + (1 << n4)];
            //x[i + (1 << n4)] = x[i + (1 << n4)];

            n5=1<<n4;
            /* Butterflies with complex twiddle factors.
             * There are (1<<k-1) of them.
             */
            for (j = 1; j < n5; ++j)
            {
                i1 = i + j;
                i2 = i + (1 << n2) - j;
                i3 = i + (1 << n2) + j;
                i4 = i + (1 << n2) + (1 << n2) - j;

                /*
                 * cc = real(W[j * n / (1<<(k+1))])
                 * ss = imag(W[j * n / (1<<(k+1))])
                 */
                cc = ccc[j << (m - n1)];
                ss = sss[j << (m - n1)];

                /* There are some symmetry properties which allow us
                 * to get away with only four multiplications here. */
                t1 = COSMUL(x[i3], cc) + COSMUL(x[i4], ss);
                t2 = COSMUL(x[i3], ss) - COSMUL(x[i4], cc);

                x[i4] = (x[i2] - t2);
                x[i3] = (-x[i2] - t2);
                x[i2] = (x[i1] - t1);
                x[i1] = (x[i1] + t1);
            }
        }
    }
    //print_int(x,n);
}

void wtk_fixfft2_test_FFT(float *x,float *y,int n,int sign)
{
	int i,j,k,l,m,n1,n2;
	float c,c1,e,s,s1,t,tr,ti;

	//Calculate i = log2N
	for(j = 1,i = 1; i<16; i++)
	{
		m = i;
		j = 2*j;
		if(j == n)
		{
			break;
		}
	}
	// 计算蝶形图的输入下标（码位倒读）
	n1 = n - 1;
	for(j = 0, i = 0; i < n1; i++)
	{
		if(i<j)
		{
			tr = x[j];
			ti = y[j];
			x[j] = x[i];
			y[j] = y[i];
			x[i] = tr;
			y[i] = ti;
		}
		k = n/2;
		while(k<(j+1))
		{
			j = j - k;
			k = k/2;
		}
		j = j + k;
	}
	// 计算每一级的输出，l为某一级，i为同一级的不同群，使用同一内存（即位运算）
	n1 = 1;
	for(l = 1; l <= m; l++)
	{
		n1 = 2*n1;
		n2 = n1/2;
		e = 3.1415926/n2;
		c = 1.0;
		s = 0.0;
		c1 = cos(e);
		s1 = -sign*sin(e);
		for(j=0; j<n2; j++)
		{
			for(i=j; i<n; i+=n1)
			{
				k = i + n2;
				tr = c*x[k] - s*y[k];
				ti = c*y[k] + s*x[k];
				x[k] = x[i] - tr;
				y[k] = y[i] - ti;
				x[i] = x[i] + tr;
				y[i] = y[i] + ti;
				wtk_debug("%d %d/%d %f/%f\n",k,k,i,tr,ti);
			}
			t = c;
			c = c*c1 - s*s1;
			s = t*s1 + s*c1;
		}
	}
	//如果是求IFFT，再除以N
	if(sign == -1)
	{
		for(i=0; i<n; i++)
		{
			x[i] /= n;
			y[i] /= n;
		}
	}
}

void wtk_fixfft2_ifftr(wtk_fixfft2_t *fft,wtk_fix2_t *x,wtk_fix2_t *y)
{
	int n=fft->N;
	int m=fft->order;
	int i,j,k,l,n1,n2;
	wtk_fix2_t c,c1,s,s1,t,tr,ti;

	n1 = n - 1;
	for(j = 0, i = 0; i < n1; i++)
	{
		if(i<j)
		{
			tr = x[j];
			ti = y[j];
			x[j] = x[i];
			y[j] = y[i];
			x[i] = tr;
			y[i] = ti;
		}
		k = n/2;
		while(k<(j+1))
		{
			j = j - k;
			k = k/2;
		}
		j = j + k;
	}
	n1 = 1;
	for(l = 1; l <= m; l++)
	{
		n1 = 2*n1;
		n2 = n1/2;
		c = FLOAT2COS(1.0);
		s = FLOAT2COS(0.0);
		c1=fft->iccc[l-1];
		s1=fft->isss[l-1];
		for(j=0; j<n2; j++)
		{
			for(i=j; i<n; i+=n1)
			{
				k = i + n2;
				tr =COSMUL(c,x[k]) - COSMUL(s,y[k]);
				ti =COSMUL(c,y[k]) + COSMUL(s,x[k]);
				x[k] = x[i] - tr;
				y[k] = y[i] - ti;
				x[i] = x[i] + tr;
				y[i] = y[i] + ti;
				//wtk_debug("%d %d/%d %f/%f\n",k,k,i,tr,ti);
			}
			t = c;
			c = COSMUL(c,c1) - COSMUL(s,s1);
			s = COSMUL(t,s1) + COSMUL(s,c1);
		}
	}
	for(i=0; i<n; i++)
	{
		x[i] /= n;
	}
}

void wtk_fixfft2_ifft(wtk_fixfft2_t *fft,wtk_fix2_t *f,wtk_fix2_t *x)
{
	wtk_fix2_t *tr=x;
	wtk_fix2_t *ti=fft->ti;
	int N=fft->N;
	int n=N>>1;
	int i,j;

	tr[0]=f[0];ti[0]=0;
	for(i=1,j=N-i;i<n;++i,--j)
	{
		tr[i]=f[i];
		ti[i]=f[j];
		tr[j]=f[i];
		ti[j]=-f[j];
	}
	tr[n]=f[n];
	ti[n]=0;
	wtk_fixfft2_ifftr(fft,tr,ti);
}

void wtk_fixfft2_test()
{
#define NX 8
	short fx[NX]={0,1,2,3,4,5,6,7};
	wtk_fix2_t fy[NX];
	wtk_fixfft2_t *fix;
	int shift=12;
	wtk_fix2_t y[NX];

	print_short(fx,NX);
	fix=wtk_fixfft2_new(NX);
	wtk_fixfft2_fft3(fix,fx,fy,shift);
	wtk_fixfft2_print(fy,NX,shift);
	wtk_fixfft2_ifft(fix,fy,y);
	wtk_fix_print(y,NX,shift);
	wtk_fixfft2_delete(fix);
	exit(0);
}


void wtk_fixfft2_fft2(wtk_fixfft2_t *fft,wtk_fix2_t *x,wtk_fix2_t *f)
{
	memcpy(f,x,fft->N*sizeof(wtk_fix2_t));
	wtk_fixfft2_fft(fft,f);
}

void wtk_fixfft2_fft3(wtk_fixfft2_t *fft,short *x,wtk_fix2_t *f,int shift)
{
	int N=fft->N;
	int i;

	for(i=0;i<N;++i)
	{
		f[i]=FLOAT2FIX_ANY(x[i],shift);
		//f[i]=x[i]<<N;
	}
	wtk_fixfft2_fft(fft,f);
}


void wtk_fixfft2_print(wtk_fix2_t *fft,int N,int shift)
{
	int n=N/2;
	int i;
	float ta,tb;

	ta=tb=0;
	wtk_debug("=================== FFT ======================\n");
	if(shift>0)
	{
		if(0)
		{
			wtk_debug("v[%d]=%f+%fi\n",0,FIX2FLOAT(fft[0]),0.0);
			for(i=1;i<n;++i)
			{
				wtk_debug("v[%d]=%f+%fi\n",i,FIX2FLOAT(fft[i]),FIX2FLOAT(fft[N-i]));
			}
			wtk_debug("v[%d]=%f+%fi\n",n,FIX2FLOAT(fft[n]),0.0);
		}else
		{
			ta+=FIX2FLOAT_ANY(fft[0],shift);
			//ta=fft[0];
			//wtk_debug("v[%d]=%d+%di %f+%fi\n",0,fft[0],0,FIX2FLOAT_ANY(fft[0],shift),0.0);
			wtk_debug("v[%d]=%f+%fi\n",0,FIX2FLOAT_ANY(fft[0],shift),0.0);
			for(i=1;i<n;++i)
			{
//				if(i>5)
//				{
//					break;
//				}
				ta+=FIX2FLOAT_ANY(fft[i],shift);
				tb+=FIX2FLOAT_ANY(fft[N-i],shift);
				//ta+=fft[0];
				//tb+=fft[N-1];
				//wtk_debug("v[%d]=%d+%di %f+%fi\n",i,fft[i],fft[N-i],FIX2FLOAT_ANY(fft[i],shift),FIX2FLOAT_ANY(fft[N-i],shift));
				wtk_debug("v[%d]=%f+%fi\n",i,FIX2FLOAT_ANY(fft[i],shift),FIX2FLOAT_ANY(fft[N-i],shift));
				//exit(0);
			}
			ta+=FIX2FLOAT_ANY(fft[n],shift);
			//ta+=fft[n];
			//wtk_debug("v[%d]=%d+%di %f+%fi\n",n,fft[n],0,FIX2FLOAT_ANY(fft[n],shift),0.0);
			wtk_debug("v[%d]=%f+%fi\n",n,FIX2FLOAT_ANY(fft[n],shift),0.0);
			wtk_debug("tot: %f+%fi\n",ta,tb);
			//wtk_debug("tot: %f+%fi\n",FIX2FLOAT_ANY(ta,shift),FIX2FLOAT_ANY(tb,shift));
		}
	}else
	{
		shift=-shift;
		ta+=FIX2FLOAT_ANY2(fft[0],shift);
		wtk_debug("v[%d]=%f+%fi\n",0,FIX2FLOAT_ANY2(fft[0],shift),0.0);
		for(i=1;i<n;++i)
		{
			ta+=FIX2FLOAT_ANY2(fft[0],shift);
			tb+=FIX2FLOAT_ANY2(fft[N-i],shift);
			wtk_debug("v[%d]=%f+%fi\n",i,FIX2FLOAT_ANY2(fft[i],shift),FIX2FLOAT_ANY2(fft[N-i],shift));
		}
		ta+=FIX2FLOAT_ANY2(fft[n],shift);
		wtk_debug("v[%d]=%f+%fi\n",n,FIX2FLOAT_ANY2(fft[n],shift),0.0);
		wtk_debug("tot: %f+%fi\n",ta,tb);
	}
}

void wtk_fixfft2_to_rfft(wtk_fix2_t *ifft,float *fft,int shift,int N)
{
	int n=N/2;
	int i;

	fft[0]=FIX2FLOAT_ANY(ifft[0],shift);
	for(i=1;i<n;++i)
	{
		fft[i]=FIX2FLOAT_ANY(ifft[i],shift);
		fft[i+n]=-FIX2FLOAT_ANY(ifft[N-i],shift);
	}
	fft[n]=FIX2FLOAT_ANY(ifft[n],shift);
}


/**
 *  shift1=shifte/2;
	shift2=shifte-shift1;
	ax1=1<<shift1;
	ax2=1<<shift2;
	ax3=1<<shifte;

	c(shift)=a(shift)*b(shiftw);
 */
wtk_fix2_t wtk_fix2_mulx(wtk_fix2_t a,wtk_fix2_t b,int a1,int a2,int a3)
{
	wtk_fix2_t tmp;

	if(a<-a1 || a>a1)
	{
		if(b<-a2 || b>a2)
		{
			tmp=(a/a1)*(b/a2);
		}else
		{
			tmp=((a/a1)*b)/a2;
		}
	}else
	{
		if(b<-a2 || b>a2)
		{
			tmp=(a*(b/a2))/a1;
		}else
		{
			tmp=(a*b)/a3;
		}
	}
	return tmp;
}
