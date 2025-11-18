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

/***************************************************************

    $Id: _freqt.c,v 1.18 2015/12/14 01:14:15 uratec Exp $

    Frequency Transformation

        void   freqt(c1, m1, c2, m2, a)

        double   *c1   : minimum phase sequence
        int      m1    : order of minimum phase sequence
        double   *c2   : warped sequence
        int      m2    : order of warped sequence
        double   a     : all-pass constant

***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "tts-mer/sptk/SPTK.h"

#ifdef __ANDROID__
// #if 1
void freqt(double *c1, const int m1, double *c2, const int m2, const double a)
{
   int i, j;
   double b;
   double *d = NULL, *g;
   int size;

   if (d == NULL) {
      size = m2;
      d = dgetmem(size + size + 2);
      g = d + size + 1;
   }

   if (m2 > size) {
      free(d);
      size = m2;
      d = dgetmem(size + size + 2);
      g = d + size + 1;
   }

   b = 1 - a * a;
   fillz(g, sizeof(*g), m2 + 1);

   for (i = -m1; i <= 0; i++) {
      if (0 <= m2)
         g[0] = c1[-i] + a * (d[0] = g[0]);
      if (1 <= m2)
         g[1] = b * d[0] + a * (d[1] = g[1]);

      for (j = 2; j <= m2;++j)
      {
         g[j] = d[j - 1] + a * ((d[j] = g[j]) - g[j - 1]);
         if (g[j]==0) {break;/* 后面的结果注定为0 */}
      }
   }

   sptk_movem(g, c2, sizeof(*g), m2 + 1);
   free(d);
   return;
}

#else
// 1<<23  8388608
#define fix_type long long
#define fix (1<<23)
#define ITOF(x) (float)(x*(1.0/fix))
#define ITOD(x) (double)(x*(1.0/fix))
#define FTOI(x) (int)(x*(fix))
#define FTOL(x) (fix_type)(x*fix)
// #define FTOL(x) (int)(x*(8388608))

void freqt(double *c1, const int m1, double *c2, const int m2, const double a)
{
   int i, j;
   double b;
   double *d = NULL, *g;
   fix_type *ld, *lg, la, lb, tmp0=0, tmp1=0;
   int size;

   if (d == NULL) {
      size = m2;
      // d = dgetmem(size + size + 2);
      // g = d + size + 1;
      g = c2;

      ld = (fix_type*)getmem(size + size + 2, sizeof(*ld));
      lg = ld + size + 1;
   }

   // if (m2 > size) {
   //    free(d);
   //    size = m2;
   //    d = dgetmem(size + size + 2);
   //    g = d + size + 1;

   //    ld = lgetmem(size + size + 2);
   //    lg = ld + size + 1;
   // }

   b = 1 - a * a;
   fillz(g, sizeof(*g), m2 + 1);
   fillz(lg, sizeof(*lg), m2 + 1);

   la = FTOL(a);
   lb = FTOL(b);

   // for (i = -m1; i <= 0; i++) {
   // 改 --i 有逻辑错误
   for (i = m1; i--;) {
      if (0 <= m2)
      {
         // d[0] = ITOD(ld[0]);
         // g[0] = c1[i] + a * (d[0] = g[0]);
         // ld[0]=lg[0];
         tmp0=lg[0];
         lg[0] = FTOL(c1[i]) + (fix_type)(i==m1?la*lg[0]: (fix_type)(la*lg[0])/fix);
         g[0] = ITOD(lg[0]);
      }
         
      if (1 <= m2)
      {
         // ld[1]=lg[1];
         // d[1] = ITOD(ld[1]);
         // g[1] = b * d[0] + a * (d[1] = g[1]);
         // lg[1] = (long)(lb * tmp0*1/(1<<23)) + (long)(la*lg[1]*1/(1<<23));
         tmp1=lg[1];
         lg[1] = (fix_type)(((fix_type)(lb * tmp0) + (fix_type)(la*lg[1]))/fix);
         g[1] = ITOD(lg[1]);
      }
      
      for (j=2; j<=m2; ++j)
      {
         // d[j] = g[j]
         // g[j] = d[j - 1] + a * ((d[j] = g[j]) - g[j - 1]);
         // ld[j] = lg[j];
         // i == m1-3 && printf("%ld ", ld[j]);
         // lg[j] = ld[j - 1] + (long)(la*(lg[j] - lg[j - 1])*1/(1<<23));
         // g[j] = ITOD(lg[j]);
         tmp0=lg[j];
         lg[j] = tmp1 + (fix_type)(la*(tmp0 - lg[j-1])/fix);
         g[j] = ITOD(lg[j]);
         tmp1=tmp0;
         if (lg[j]==0) {break;/* 后面的结果注定为0 */}
      }
   }

   free(ld);
   // free(d);
   return;
}

#endif
