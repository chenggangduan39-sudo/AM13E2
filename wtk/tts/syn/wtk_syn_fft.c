#include "wtk_syn_fft.h" 
#include "wtk/core/wtk_alloc.h"
#include <math.h>
#ifdef M_PI
#define PI		M_PI
#else
#define PI		3.1415926535897932385
#endif

#define POW2(p) (1 << (int)(p))

/* Operation on FFT */
int nextpow2(long n)
{
    int p;
    long value;

    for (p = 1;; p++)
    {
    	value = (long)POW2(p);
    	if (value >= n) break;
    }
    return p;
}

int wtk_syn_get_j(int j,int k)
{
	//wtk_debug("j=%d k=%d\n",j,k);
	while (j >= k)
	{
		j -= k;
		k /= 2;
		//wtk_debug("j=%d k=%d\n",j,k);
		if(j==0 && k==0)
		{
			break;
		}
	}
	//wtk_debug("j=%d k=%d\n",j,k);
	j += k;
	//wtk_debug("%d=>%d k=%d\n",x,j,k);
	return j;
}

wtk_syn_fft_t* wtk_syn_fft_new(int fftlen)
{
	wtk_syn_fft_t *f;
	int i;
	int n2;

	f=(wtk_syn_fft_t*)wtk_malloc(sizeof(wtk_syn_fft_t));
	f->fftlen=fftlen;
	f->p=nextpow2(f->fftlen);
	f->n=POW2(f->p);
	f->divn=1.0/f->n;
	f->kj=(unsigned short*)wtk_malloc(sizeof(unsigned short)*fftlen);
	n2=f->n>>1;
	for(i=0;i<fftlen;++i)
	{
		f->kj[i]=wtk_syn_get_j(i,n2);
	}
	return f;
}

int wtk_syn_fft_bytes(wtk_syn_fft_t* f)
{
	int bytes;

	bytes=sizeof(unsigned short)*f->fftlen;

	return bytes;
}

void wtk_syn_fft_delete(wtk_syn_fft_t *f)
{
	wtk_free(f->kj);
	wtk_free(f);
}

static float fft_cos[]={
		0,
		-1.000000,
		0.000000,
		0.707107,
		0.923880,
		0.980785,
		0.995185,
		0.998795,
		0.999699,
		0.999925,
};

static float fft_sin[]={
		0,
	-0.000000,
	-1.000000,
	-0.707107,
	-0.382683,
	-0.195090,
	-0.098017,
	-0.049068,
	-0.024541,
	-0.012272,
};



int wtk_syn_fft_process(wtk_syn_fft_t *f,float *xRe, float *xIm, int inv)
{
    int p,n;
    register float *fs,*fe,tf,tf1,*fs2;
    int i, ip, j, m, me, me1;
    float uRe, uIm, vRe, vIm, wRe, wIm, tRe, tIm;


    p=f->p;
    n=f->n;
    //nv2 = n>>1;
    if(inv)
    {
    	fs=xIm;
    	fe=fs+n;
//    	fe=fs+n-8;
//    	while(fs<=fe)
//    	{
//    		*(fs++)=-*fs;
//    		*(fs++)=-*fs;
//    		*(fs++)=-*fs;
//    		*(fs++)=-*fs;
//    		*(fs++)=-*fs;
//    		*(fs++)=-*fs;
//    		*(fs++)=-*fs;
//    		*(fs++)=-*fs;
//    	}
//    	fe+=8;
    	while(fs<fe)
    	{
    		//wtk_debug("fs=%p\n",fs);
    		*(fs)=-*fs;
    		++fs;
    	}
    }

    fs=xRe;fe=xIm;
    // bit reversion
    for (i = 0, j = 0; i < n - 1; ++i,++fs,++fe)
    {
    	//wtk_debug("j=%d i=%d b=%d\n",j,i,j>i);
		if (j > i)
		{
			tf=xRe[j];
			xRe[j]=*fs;
			*fs=tf;

			tf=xIm[j];
			xIm[j]=*fe;
			*fe=tf;
		}
		j=f->kj[j];
    }
    //exit(0);


    //wtk_debug("m=%d p=%d\n",m,p);
    // butterfly numeration
	for (m = 1; m <= p; ++m)
	{
	  me = (long)POW2(m);
	  me1 = me>>1;
	  uRe = 1.0;
	  uIm = 0.0;
	  wRe=fft_cos[m];
	  wIm=fft_sin[m];
	  //wtk_debug("me1=%d/%d\n",me,me1);
	  for (j = 0; j < me1; j++)
	  {
		 for (i = j; i < n; i += me)
		 {
			ip = i + me1;
			tf=xRe[ip];
			tf1=xIm[ip];
			tRe = tf * uRe - tf1 * uIm;
			tIm = tf * uIm + tf1 * uRe;
			xRe[ip] = xRe[i] - tRe;
			xIm[ip] = xIm[i] - tIm;
			xRe[i] += tRe;
			xIm[i] += tIm;
		 }
		 vRe = uRe * wRe - uIm * wIm;
		 vIm = uRe * wIm + uIm * wRe;
		 uRe = vRe;
		 uIm = vIm;
	  }
	}
	//exit(0);

    if (inv)
    {
    	tf=f->divn;
    	tf1=-tf;
    	fs=xRe;
    	fe=fs+n;
    	fs2=xIm;
    	while(fs<fe)
    	{
    		*(fs)=*fs*tf;
    		*(fs2)=*fs2*tf1;
    		++fs;
    		++fs2;
    	}
    }

    return 1;
}


int wtk_syn_fft_process2(wtk_syn_fft_t *f,float *xRe, float *xIm, int inv)
{
    int p;
    long i, ip, j, k, m, me, me1, n, nv2;
    double uRe, uIm, vRe, vIm, wRe, wIm, tRe, tIm;


    p=f->p;
    n=f->n;
    nv2 = n / 2;

    if (inv) for (i = 0; i < n; i++) xIm[i] = -xIm[i];

    // bit reversion
    for (i = 0, j = 0; i < n - 1; i++) {
	if (j > i) {
	    tRe = xRe[j];	tIm = xIm[j];
	    xRe[j] = xRe[i];	xIm[j] = xIm[i];
	    xRe[i] = tRe;	xIm[i] = tIm;
	}
	k = nv2;
	while (j >= k) {
	    j -= k;
	    k /= 2;
	}
	j += k;
    }

    // butterfly numeration
   for (m = 1; m <= p; m++) {
      me = (long)POW2(m);	me1 = me / 2;
      uRe = 1.0;		uIm = 0.0;
      wRe = cos(PI / (double)me1);
      wIm = (-sin(PI / (double)me1));
      for (j = 0; j < me1; j++) {
         for (i = j; i < n; i += me) {
            ip = i + me1;
            tRe = xRe[ip] * uRe - xIm[ip] * uIm;
            tIm = xRe[ip] * uIm + xIm[ip] * uRe;
            xRe[ip] = xRe[i] - tRe;
            xIm[ip] = xIm[i] - tIm;
            xRe[i] += tRe;
            xIm[i] += tIm;
         }
         vRe = uRe * wRe - uIm * wIm;
         vIm = uRe * wIm + uIm * wRe;
         uRe = vRe;
         uIm = vIm;
      }
   }

    if (inv) {
	for (i = 0; i < n; i++) {
	    xRe[i] /= (double)n;
	    xIm[i] /= (double)(-n);
	}
    }

    return 1;
}
