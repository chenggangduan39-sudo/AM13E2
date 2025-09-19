#include "celt_lpc.h"

void _celt_lpc(
      float       *_lpc, /* out: [0...p-1] LPC coefficients      */
const float *ac,  /* in:  [0...p] autocorrelation values  */
int          p
)
{
   int i, j;
   float r;
   float error = ac[0];
   float *lpc = _lpc;

   memset(lpc, 0, p*sizeof(float));
   if (ac[0] != 0)
   {
      for (i = 0; i < p; i++) {
         /* Sum up this iteration's reflection coefficient */
         float rr = 0;
         for (j = 0; j < i; j++)
            rr += lpc[j]*ac[i - j];
         rr += ac[i + 1];
         r = -rr/error;
         /*  Update LPC coefficients and total error */
         lpc[i] = r;
         for (j = 0; j < (i+1)>>1; j++)
         {
            float tmp1, tmp2;
            tmp1 = lpc[j];
            tmp2 = lpc[i-1-j];
            lpc[j]     = tmp1 + r*tmp2;
            lpc[i-1-j] = tmp2 + r*tmp1;
         }

         error = error - r*r*error;
         /* Bail out once we get 30 dB gain */
         if (error<.001f*ac[0])
            break;
      }
   }
}


void celt_fir(
         const float *x,
         const float *num,
         float *y,
         int N,
         int ord_n)
{
   int i,j;
   float rnum[64];
   assert(ord_n < sizeof(rnum) / sizeof(rnum[0]));
   for(i=0;i<ord_n;i++)
      rnum[i] = num[ord_n-i-1];
   for (i=0;i<N-3;i+=4)
   {
      float sum[4];
      sum[0] = x[i];
      sum[1] = x[i+1];
      sum[2] = x[i+2];
      sum[3] = x[i+3];
      xcorr_kernel(rnum, x+i-ord_n, sum, ord_n);
      y[i  ] = sum[0];
      y[i+1] = sum[1];
      y[i+2] = sum[2];
      y[i+3] = sum[3];
   }
   for (;i<N;i++)
   {
      float sum = x[i];
      for (j=0;j<ord_n;j++)
         sum += rnum[j]*x[i+j-ord_n];
      y[i] = sum;
   }
}

void celt_iir(const float *_x,
         const float *den,
         float *_y,
         int N,
         int ord_n,
         float *mem)
{
   int i,j;
   assert((ord_n&3)==0);
   float rden[64];
   float y[128];
   assert(ord_n < (sizeof(rden) / sizeof(rden[0])));
   assert(N + ord_n < (sizeof(y)  / sizeof(y[0])));
   for(i=0;i<ord_n;i++)
      rden[i] = den[ord_n-i-1];
   for(i=0;i<ord_n;i++)
      y[i] = -mem[ord_n-i-1];
   for(;i<N+ord_n;i++)
      y[i]=0;
   for (i=0;i<N-3;i+=4)
   {
      /* Unroll by 4 as if it were an FIR filter */
      float sum[4];
      sum[0]=_x[i];
      sum[1]=_x[i+1];
      sum[2]=_x[i+2];
      sum[3]=_x[i+3];
      xcorr_kernel(rden, y+i, sum, ord_n);

      /* Patch up the result to compensate for the fact that this is an IIR */
      y[i+ord_n  ] = -sum[0];
      _y[i  ] = sum[0];
      sum[1] += y[i+ord_n]*den[0];
      y[i+ord_n+1] = -sum[1];
      _y[i+1] = sum[1];
      sum[2] += y[i+ord_n+1]* den[0];
      sum[2] += y[i+ord_n  ]* den[1];
      y[i+ord_n+2] = -sum[2];
      _y[i+2] = sum[2];

      sum[3] += y[i+ord_n+2]* den[0];
      sum[3] += y[i+ord_n+1]* den[1];
      sum[3] += y[i+ord_n  ]* den[2];
      y[i+ord_n+3] = -sum[3];
      _y[i+3] = sum[3];
   }
   for (;i<N;i++)
   {
      float sum = _x[i];
      for (j=0;j<ord_n;j++)
         sum -= rden[j]*y[i+j];
      y[i+ord_n] = sum;
      _y[i] = sum;
   }
   for(i=0;i<ord_n;i++)
      mem[i] = _y[N-i-1];
}

int _celt_autocorr(
                   const float *x,   /*  in: [0...n-1] samples x   */
                   float       *ac,  /* out: [0...lag-1] ac values */
                   const float       *window,
                   int          overlap,
                   int          lag,
                   int          n)
{
   float d;
   int i, k;
   int fastN=n-lag;
   int shift;
   const float *xptr;
   float xx[2048];
   assert(n>0 && n<sizeof(xx)/sizeof(xx[0]));
   assert(overlap>=0);
   if (overlap == 0)
   {
      xptr = x;
   } else {
      for (i=0;i<n;i++)
         xx[i] = x[i];
      for (i=0;i<overlap;i++)
      {
         xx[i] = x[i]*window[i];
         xx[n-i-1] = x[n-i-1]*window[i];
      }
      xptr = xx;
   }
   shift=0;

   celt_pitch_xcorr(xptr, xptr, ac, fastN, lag+1);
   for (k=0;k<=lag;k++)
   {
      for (i = k+fastN, d = 0; i < n; i++)
         d += xptr[i]*xptr[i-k];
      ac[k] += d;
   }

   return shift;
}
