#include "pitch.h"

static void find_best_pitch(float *xcorr, float *y, int len,
                            int max_pitch, int *best_pitch)
{
   int i, j;
   float Syy=1;
   float best_num[2];
   float best_den[2];


   best_num[0] = -1;
   best_num[1] = -1;
   best_den[0] = 0;
   best_den[1] = 0;
   best_pitch[0] = 0;
   best_pitch[1] = 1;
   for (j=0;j<len;j++)
      Syy += y[j]*y[j];
   for (i=0;i<max_pitch;i++)
   {
      if (xcorr[i]>0)
      {
         float num;
         float xcorr16;
         xcorr16 = xcorr[i];

         xcorr16 *= 1e-12f;
         num = xcorr16*xcorr16;
         if (num*best_den[1] > best_num[1]*Syy)
         {
            if (num*best_den[0] > best_num[0]*Syy)
            {
               best_num[1] = best_num[0];
               best_den[1] = best_den[0];
               best_pitch[1] = best_pitch[0];
               best_num[0] = num;
               best_den[0] = Syy;
               best_pitch[0] = i;
            } else {
               best_num[1] = num;
               best_den[1] = Syy;
               best_pitch[1] = i;
            }
         }
      }
      Syy += y[i+len]*y[i+len] - y[i]*y[i];
      Syy = max(1, Syy);
   }
}

static void celt_fir5(const float *x,
         const float *num,
         float *y,
         int N,
         float *mem)
{
   int i;
   float num0, num1, num2, num3, num4;
   float mem0, mem1, mem2, mem3, mem4;
   num0=num[0];
   num1=num[1];
   num2=num[2];
   num3=num[3];
   num4=num[4];
   mem0=mem[0];
   mem1=mem[1];
   mem2=mem[2];
   mem3=mem[3];
   mem4=mem[4];
   for (i=0;i<N;i++)
   {
      float sum = x[i];
      sum += num0*mem0;
      sum += num1*mem1;
      sum += num2*mem2;
      sum += num3*mem3;
      sum += num4*mem4;
      mem4 = mem3;
      mem3 = mem2;
      mem2 = mem1;
      mem1 = mem0;
      mem0 = x[i];
      y[i] = sum;
   }
   mem[0]=mem0;
   mem[1]=mem1;
   mem[2]=mem2;
   mem[3]=mem3;
   mem[4]=mem4;
}


void pitch_downsample(float *x[], float *x_lp,
      int len, int C)
{
   int i;
   float ac[5];
   float tmp=1.0f;
   float lpc[4], mem[5]={0,0,0,0,0};
   float lpc2[5];
   float c1 = 0.8f;

   for (i=1;i<len>>1;i++)
      x_lp[i] = 0.5f*(0.5f*(x[0][(2*i-1)]+x[0][(2*i+1)])+x[0][2*i]);
   x_lp[0] = 0.5f*(0.5f*(x[0][1])+x[0][0]);
   if (C==2)
   {
      for (i=1;i<len>>1;i++)
         x_lp[i] += 0.5f*(0.5f*(x[1][(2*i-1)]+x[1][(2*i+1)])+x[1][2*i]);
      x_lp[0] += 0.5f*(0.5f*(x[1][1])+x[1][0]);
   }

   _celt_autocorr(x_lp, ac, NULL, 0,
                  4, len>>1);

   /* Noise floor -40 dB */
   ac[0] *= 1.0001f;
   /* Lag windowing */
   for (i=1;i<=4;i++)
   {
      /*ac[i] *= exp(-.5*(2*M_PI*.002*i)*(2*M_PI*.002*i));*/
      ac[i] -= ac[i]*(.008f*i)*(.008f*i);
   }

   _celt_lpc(lpc, ac, 4);
   for (i=0;i<4;i++)
   {
      tmp = .9f*tmp;
      lpc[i] = lpc[i]* tmp;
   }
   /* Add a zero */
   lpc2[0] = lpc[0] + .8f;
   lpc2[1] = lpc[1] + c1*lpc[0];
   lpc2[2] = lpc[2] + c1*lpc[1];
   lpc2[3] = lpc[3] + c1*lpc[2];
   lpc2[4] = c1*lpc[3];
   celt_fir5(x_lp, lpc2, x_lp, len>>1, mem);
}

void celt_pitch_xcorr(const float *_x, const float *_y,
      float *xcorr, int len, int max_pitch)
{
    /* Unrolled version of the pitch correlation -- runs faster on x86 and ARM */
   int i;
   /*The EDSP version requires that max_pitch is at least 1, and that _x is
      32-bit aligned.
     Since it's hard to put asserts in assembly, put them here.*/

   assert(max_pitch>0);
   assert((((unsigned char *)_x-(unsigned char *)NULL)&3)==0);
   for (i=0;i<max_pitch-3;i+=4)
   {
      float sum[4]={0,0,0,0};
      xcorr_kernel(_x, _y+i, sum, len);
      xcorr[i]=sum[0];
      xcorr[i+1]=sum[1];
      xcorr[i+2]=sum[2];
      xcorr[i+3]=sum[3];

   }
   /* In case max_pitch isn't a multiple of 4, do non-unrolled version. */
   for (;i<max_pitch;i++)
   {
      float sum;
      sum = celt_inner_prod(_x, _y+i, len);
      xcorr[i] = sum;

   }
}

void pitch_search(const float *x_lp, float *y,
                  int len, int max_pitch, int *pitch)
{
   int i, j;
   int lag;
   int best_pitch[2]={0,0};

   int offset;

   assert(len>0);
   assert(max_pitch>0);
   lag = len+max_pitch;

   float x_lp4[1024];
   float y_lp4[1024];
   float xcorr[512];

   assert((len >> 2) < (sizeof(x_lp4) / sizeof(x_lp4[0])));
   assert((lag >> 2) < (sizeof(y_lp4) / sizeof(y_lp4[0])));
   assert((max_pitch >> 1) < (sizeof(xcorr) / sizeof(xcorr[0])));

   /* Downsample by 2 again */
   for (j=0;j<len>>2;j++)
      x_lp4[j] = x_lp[2*j];
   for (j=0;j<lag>>2;j++)
      y_lp4[j] = y[2*j];

   /* Coarse search with 4x decimation */

   celt_pitch_xcorr(x_lp4, y_lp4, xcorr, len>>2, max_pitch>>2);

   find_best_pitch(xcorr, y_lp4, len>>2, max_pitch>>2, best_pitch);

   /* Finer search with 2x decimation */
   for (i=0;i<max_pitch>>1;i++)
   {
      float sum;
      xcorr[i] = 0;
      if (abs(i-2*best_pitch[0])>2 && abs(i-2*best_pitch[1])>2)
         continue;
      sum = celt_inner_prod(x_lp, y+i, len>>1);
      xcorr[i] = max(-1, sum);
   }
   find_best_pitch(xcorr, y, len>>1, max_pitch>>1, best_pitch);

   /* Refine by pseudo-interpolation */
   if (best_pitch[0]>0 && best_pitch[0]<(max_pitch>>1)-1)
   {
      float a, b, c;
      a = xcorr[best_pitch[0]-1];
      b = xcorr[best_pitch[0]];
      c = xcorr[best_pitch[0]+1];
      if ((c-a) > .7f*(b-a))
         offset = 1;
      else if ((a-c) > .7f*(b-c))
         offset = -1;
      else
         offset = 0;
   } else {
      offset = 0;
   }
   *pitch = 2*best_pitch[0]-offset;
}

void pitch_search2(const float *x_lp, float *y,
                  int len, int max_pitch, int *pitch,  float *pitch_corr)
{
   int i, j;
   int lag;
   int best_pitch[2]={0,0};

   int offset;

   assert(len>0);
   assert(max_pitch>0);
   lag = len+max_pitch;

   float x_lp4[1024];
   float y_lp4[1024];
   float xcorr[512];

   assert((len >> 2) < (sizeof(x_lp4) / sizeof(x_lp4[0])));
   assert((lag >> 2) < (sizeof(y_lp4) / sizeof(y_lp4[0])));
   assert((max_pitch >> 1) < (sizeof(xcorr) / sizeof(xcorr[0])));

   /* Downsample by 2 again */
   for (j=0;j<len>>2;j++)
      x_lp4[j] = x_lp[2*j];
   for (j=0;j<lag>>2;j++)
      y_lp4[j] = y[2*j];

   /* Coarse search with 4x decimation */

   celt_pitch_xcorr(x_lp4, y_lp4, xcorr, len>>2, max_pitch>>2);

   find_best_pitch(xcorr, y_lp4, len>>2, max_pitch>>2, best_pitch);

   /* Finer search with 2x decimation */
   for (i=0;i<max_pitch>>1;i++)
   {
      float sum;
      xcorr[i] = 0;
      if (abs(i-2*best_pitch[0])>2 && abs(i-2*best_pitch[1])>2)
         continue;
      sum = celt_inner_prod(x_lp, y+i, len>>1);
      xcorr[i] = max(-1, sum);
   }
   find_best_pitch(xcorr, y, len>>1, max_pitch>>1, best_pitch);

   /* Refine by pseudo-interpolation */
   if (best_pitch[0]>0 && best_pitch[0]<(max_pitch>>1)-1)
   {
      float a, b, c;
      a = xcorr[best_pitch[0]-1];
      b = xcorr[best_pitch[0]];
      c = xcorr[best_pitch[0]+1];
      if ((c-a) > .7f*(b-a))
         offset = 1;
      else if ((a-c) > .7f*(b-c))
         offset = -1;
      else
         offset = 0;
   } else {
      offset = 0;
   }
   *pitch = 2*best_pitch[0]-offset;
   *pitch_corr = xcorr[best_pitch[0]];
}

static float compute_pitch_gain(float xy, float xx, float yy)
{
   return xy/sqrt(1+xx*yy);
}

static const int second_check[16] = {0, 0, 3, 2, 3, 2, 5, 2, 3, 2, 3, 2, 5, 2, 3, 2};
float remove_doubling(float *x, int maxperiod, int minperiod,
      int N, int *T0_, int prev_period, float prev_gain)
{
   int k, i, T, T0;
   float g, g0;
   float pg;
   float xy,xx,yy,xy2;
   float xcorr[3];
   float best_xy, best_yy;
   int offset;
   int minperiod0;

   minperiod0 = minperiod;
   maxperiod /= 2;
   minperiod /= 2;
   *T0_ /= 2;
   prev_period /= 2;
   N /= 2;
   x += maxperiod;
   if (*T0_>=maxperiod)
      *T0_=maxperiod-1;

   T = T0 = *T0_;
   float yy_lookup[1024];
   assert((maxperiod + 1 ) <  (sizeof(yy_lookup) / sizeof(yy_lookup[0])));
   dual_inner_prod(x, x, x-T0, N, &xx, &xy);
   yy_lookup[0] = xx;
   yy=xx;
   for (i=1;i<=maxperiod;i++)
   {
      yy = yy+x[-i]*x[-i]-x[N-i]*x[N-i];
      yy_lookup[i] = max(0, yy);
   }
   yy = yy_lookup[T0];
   best_xy = xy;
   best_yy = yy;
   g = g0 = compute_pitch_gain(xy, xx, yy);
   /* Look for any pitch at T/k */
   for (k=2;k<=15;k++)
   {
      int T1, T1b;
      float g1;
      float cont=0;
      float thresh;
      T1 = (2*T0+k)/(2*k);
      if (T1 < minperiod)
         break;
      /* Look for another strong correlation at T1b */
      if (k==2)
      {
         if (T1+T0>maxperiod)
            T1b = T0;
         else
            T1b = T0+T1;
      } else
      {
         T1b = (2*second_check[k]*T0+k)/(2*k);
      }
      dual_inner_prod(x, &x[-T1], &x[-T1b], N, &xy, &xy2);
      xy = 0.5f*(xy + xy2);
      yy = 0.5f*(yy_lookup[T1] + yy_lookup[T1b]);
      g1 = compute_pitch_gain(xy, xx, yy);
      if (abs(T1-prev_period)<=1)
         cont = prev_gain;
      else if (abs(T1-prev_period)<=2 && 5*k*k < T0)
         cont = 0.5f*(prev_gain);
      else
         cont = 0;
      thresh = max(.3f, .7f*g0-cont);
      /* Bias against very high pitch (very short period) to avoid false-positives
         due to short-term correlation */
      if (T1<3*minperiod)
         thresh = max(.4f, .85f*g0-cont);
      else if (T1<2*minperiod)
         thresh = max(.5f, .9f*g0-cont);
      if (g1 > thresh)
      {
         best_xy = xy;
         best_yy = yy;
         T = T1;
         g = g1;
      }
   }
   best_xy = max(0, best_xy);
   if (best_yy <= best_xy)
      pg = 1.0f;
   else
      pg = best_xy/(best_yy+1);

   for (k=0;k<3;k++)
      xcorr[k] = celt_inner_prod(x, x-(T+k-1), N);
   if ((xcorr[2]-xcorr[0]) > .7f*(xcorr[1]-xcorr[0]))
      offset = 1;
   else if ((xcorr[0]-xcorr[2]) > .7f*(xcorr[1]-xcorr[2]))
      offset = -1;
   else
      offset = 0;
   if (pg > g)
      pg = g;
   *T0_ = 2*T+offset;

   if (*T0_<minperiod0)
      *T0_=minperiod0;
   return pg;
}
