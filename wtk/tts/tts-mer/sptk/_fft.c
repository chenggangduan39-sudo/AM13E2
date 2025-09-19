/* ----------------------------------------------------------------- */
/*             The Speech Signal Processing Toolkit (SPTK)           */
/*             developed by SPTK Working Group                       */
/*             http://sp-tk.sourceforge.net/                         */
/* ----------------------------------------------------------------- */
/*                                                                   */
/*  Copyright (c) 1984-2007  Tokyo Institute of Technology           */
/*                           Interdisciplinary Graduate School of    */
/*                           Science and Engineering                 */
/*                                                                   */
/*                1996-2015  Nagoya Institute of Technology          */
/*                           Department of Computer Science          */
/*                                                                   */
/* All rights reserved.                                              */
/*                                                                   */
/* Redistribution and use in source and binary forms, with or        */
/* without modification, are permitted provided that the following   */
/* conditions are met:                                               */
/*                                                                   */
/* - Redistributions of source code must retain the above copyright  */
/*   notice, this list of conditions and the following disclaimer.   */
/* - Redistributions in binary form must reproduce the above         */
/*   copyright notice, this list of conditions and the following     */
/*   disclaimer in the documentation and/or other materials provided */
/*   with the distribution.                                          */
/* - Neither the name of the SPTK working group nor the names of its */
/*   contributors may be used to endorse or promote products derived */
/*   from this software without specific prior written permission.   */
/*                                                                   */
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND            */
/* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,       */
/* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF          */
/* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE          */
/* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS */
/* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,          */
/* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED   */
/* TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     */
/* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON */
/* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,   */
/* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY    */
/* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE           */
/* POSSIBILITY OF SUCH DAMAGE.                                       */
/* ----------------------------------------------------------------- */

/********************************************************
   $Id: _fft.c,v 1.18 2015/12/14 01:14:13 uratec Exp $               
       NAME:               
                fft - fast fourier transform    
       SYNOPSIS:               
                int   fft(x, y, m);         
                     
                double   x[];   real part      
                double   y[];   imaginary part      
                int      m;     data size      
   
                return : success = 0
                         fault   = -1
       Naohiro Isshiki          Dec.1995    modified   
********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "tts-mer/sptk/SPTK.h"

// static int checkm(const int m)
// {
//    int k;

//    for (k = 4; k <= m; k <<= 1)
//    {
//       if (k == m)
//          return (0);
//    }
//    fprintf(stderr, "fft : m must be a integer of power of 2!\n");

//    return (-1);
// }

static float fft_cos[]={
      0,
      -1.000000,
      0.000000,
      0.707106781186548,
      0.923879532511287,
      0.980785280403230,
      0.995184726672197,
      0.998795456205172,
      0.999698818696204,
      0.999924701839145,
      0.999981175282601,
      0.999995293809576,
      0.999998823451702,
      0.999999705862882,
};

static float fft_sin[]={
      0,
      -0.000000,
      -1.000000,
      -0.707106781186547,
      -0.382683432365090,
      -0.195090322016128,
      -0.098017140329561,
      -0.049067674327418,
      -0.024541228522912,
      -0.012271538285720,
      -0.006135884649154,
      -0.003067956762966,
      -0.001533980186285,
      -0.000766990318743,
};

int fft(double *real, double *imag, int n)
{
   //   complex t,wn;//中间变量
   double xr, xi, yr, yi, vr, vi, vw;
   int i, j, k, m, M;
   int la, lb, lc;

   /*----计算分解的级数M=log2(N)----*/
   for (i = n, M = 1; (i = i / 2) != 1; M++);

   /*----按照倒位序重新排列原信号----*/
   for (i = 1, j = n/2; i <= n - 2; i++)
   {
      if (i < j)
      {
         xr = real[j];
         xi = imag[j];
         real[j] = real[i];
         imag[j] = imag[i];
         real[i] = xr;
         imag[i] = xi;
      }
      k = n / 2;
      while (k<=j)
      {
         j -= k;
         k /= 2;
      }
      j += k;
   }

   /*----FFT算法----*/
   la = 2;
   for (m = 1; m <= M; m++)
   {
      lb = la>>1;
   
      yr = fft_cos[m];
      yi = fft_sin[m];
      // yr = cos(x);
      // yi = -sin(x);
      // if (m>10)
      // {
      //    printf("m: %d  %.15lf %.15lf \n", m, yr, yi);
      // }
      vr = 1.0;
      vi = 0.0;
      /*----碟形运算----*/
      for (i = 0; i < lb; i++)
      {
         for (j = i; j < n; j+=la) //遍历每个分组，分组总数为N/la
         {
            lc = lb + j;
            xr = real[lc]*vr - imag[lc]*vi;
            xi = real[lc]*vi + imag[lc]*vr;
            real[lc] = real[j] - xr;
            imag[lc] = imag[j] - xi;
            real[j] = real[j] + xr;
            imag[j] = imag[j] + xi;
         }
         vw = vr;
         vr = vr*yr - vi*yi;
         vi = vw*yi + vi*yr;
      }
      la=la<<1;
   }
   return 0;
}

int fft_float(float *real, float *imag, int n)
{
   //   complex t,wn;//中间变量
   float xr, xi, yr, yi, vr, vi, vw;
   int i, j, k, m, M;
   int la, lb, lc;

   /*----计算分解的级数M=log2(N)----*/
   for (i = n, M = 1; (i = i >> 1) != 1; M++);

   /*----按照倒位序重新排列原信号----*/
   for (i = 1, j = n/2; i <= n - 2; ++i)
   {
      if (i < j)
      {
         xr = real[j];
         xi = imag[j];
         real[j] = real[i];
         imag[j] = imag[i];
         real[i] = xr;
         imag[i] = xi;
      }
      k = n / 2;
      while (k<=j)
      {
         j -= k;
         k /= 2;
      }
      j += k;
   }

   /*----FFT算法----*/
   la = 2;
   for (m = 1; m <= M; ++m)
   {
      lb = la>>1;
   
      yr = fft_cos[m];
      yi = fft_sin[m];
      // yr = cos(x);
      // yi = -sin(x);
      // if (m>10)
      // {
      //    printf("m: %d  %.15lf %.15lf \n", m, yr, yi);
      // }
      vr = 1.0;
      vi = 0.0;
      /*----碟形运算----*/
      for (i = 0; i < lb; ++i)
      {
         for (j = i; j < n; j+=la) //遍历每个分组，分组总数为N/la
         {
            lc = lb + j;
            xr = real[lc]*vr - imag[lc]*vi;
            xi = real[lc]*vi + imag[lc]*vr;
            real[lc] = real[j] - xr;
            imag[lc] = imag[j] - xi;
            real[j] = real[j] + xr;
            imag[j] = imag[j] + xi;
         }
         vw = vr;
         vr = vr*yr - vi*yi;
         vi = vw*yi + vi*yr;
      }
      la=la<<1;
   }
   return 0;
}

static void _rfft( wtk_rfft_t *rf, float *cache, float *x, float *y)
{/* 默认虚部为0 */
   int m=rf->len
     , win=rf->win
     , i;
   float *fin = x;
   float *f = cache + m;

   wtk_rfft_process_fft(rf, f, fin);
   // wtk_rfft_print_fft(fout, m);
   x[0] = f[0];
   y[0] = 0;
   for (i=1; i<win; ++i)
   {
      x[i]=f[i];
      y[i]=-f[i+win];
   }
   x[win]=f[win];
   y[win]=0;
   for(i=win+1; i<m; ++i)
   {
      x[i]=f[m-i];
      y[i]=f[m-i+win];
   }
   // exit(1);
}

static void _rfft_double( wtk_rfft_t *rf, float *cache, double *x, double *y)
{/* 默认虚部为0 */
   int m=rf->len
     , win=rf->win
     , i;
   float *fin = cache;
   float *f = cache + m;
   for (i=0; i<m; ++i)
   {
      fin[i] = (float)x[i];
   }
   wtk_rfft_process_fft(rf, f, fin);
   // wtk_rfft_print_fft(fout, m);
   x[0] = f[0];
   y[0] = 0;
   for (i=1; i<win; ++i)
   {
      x[i]=f[i];
      y[i]=-f[i+win];
   }
   x[win]=f[win];
   y[win]=0;
   for(i=win+1; i<m; ++i)
   {
      x[i]=f[m-i];
      y[i]=f[m-i+win];
   }
   // exit(1);
}

int sptk_fft( double *x, double *y, int m)
{ /* x: 实部 y: 虚部 */
   fft(x, y, m);
   return 0;
};

int sptk_rfft( wtk_rfft_t *rf, float *cache, double *x, double *y)
{/* y: 0 ; cache_len=2*m*/
   _rfft_double( rf, cache, x, y);
   return 0;
}

int sptk_rfft_float( wtk_rfft_t *rf, float *cache, float *x, float *y)
{/* y: 0 ; cache_len=2*m*/
   _rfft( rf, cache, x, y);
   return 0;
}


int sptk_ifft_float(float *x, float *y, const int m)
{
   int i;
   if (fft_float(y, x, m) == -1)
      return (-1);
   
   for (i = m; --i >= 0; ++x, ++y) {
      *x /= m;
      *y /= m;
   }

   return (0);
}

int sptk_rfft_ifft_float(wtk_rfft_t *rf, float *cache, float *x, float *y)
{/* 
x: real
y: imag
只能得到实部的值 
*/
   float *f;
   int i
     , m = rf->len
     , win = rf->win;
   f = cache;

   f[0] = x[0];
   for (i=1; i<win; ++i)
   {
      f[i] = x[i];
      f[i+win] = -y[i];
   }
   f[win] = x[win];
   for(i=win+1; i<m; ++i)
   {
      f[m-i] = x[i];
      f[m-i+win] = y[i];
   }
   wtk_rfft_process_ifft(rf, f, x);
   return (0);
}
