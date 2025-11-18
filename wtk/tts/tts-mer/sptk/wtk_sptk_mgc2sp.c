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

/************************************************************************
*                                                                       *
*    Transform Mel-Generalize Cepstrum to Spectrum                      *
*                                                                       *
*                                       1996.4  K.Koishida              *
*                                       2010.5  A.Tamamori              *
*                                                                       *
*       usage:                                                          *
*               mgc2sp [ options ] [ infile ] > stdout                  *
*       options:                                                        *
*               -a  a    :  alpha                               [0]     *
*               -g  g    :  gamma                               [0]     *
*               -c c     :  gamma = -1 / (int) c                        *
*               -m  m    :  order of mel-generalized cepstrum   [25]    *
*               -n       :  regard input as normalized cepstrum [FALSE] *
*               -u       :  regard input as multiplied by gamma [FALSE] *
*               -l  l    :  FFT length                          [256]   *
*               -p       :  output phase                        [FALSE] *
*               -o  o    :  output format (see stdout)          [0]     *
*                               0 (20 * log|H(z)|)                      *
*                               1 (ln|H(z)|)                            *
*                               2 (|H(z)|)                              *
*                               3 (|H(z)|^2)                            *
*                           -p option is specified                      *
*                               0 (arg|H(z)| / pi       [pi rad])       *
*                               1 (arg|H(z)|            [rad])          *
*                               2 (arg|H(z)| * 180 / pi [deg])          *
*      infile:                                                          *
*               mel-generalized cepstrum                                *
*                       , c(0), c(1), ..., c(m),                        *
*      stdout:                                                          *
*               spectrum                                                *
*                       , s(0), s(1), ..., s(L/2),                      *
*      notice:                                                          *
*               value of c must be c>=1                                 *
*      require:                                                         *
*               mgc2sp()                                                *
*                                                                       *
************************************************************************/

/*  Standard C Libraries  */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <math.h>

#include "tts-mer/sptk/SPTK.h"

/*  Default Values  */
#define ORDER  25
#define ALPHA  0.0
#define GAMMA  0.0
#define NORM  FA
#define LENG  256
#define OTYPE  0
#define PHASE  FA
#define MULG  FA

/*  Command Name  */
static void wtk_sptk_mgc2sp_main( double *sintbl, int argc, char **argv, float *p, int len, float *dst);

void wtk_sptk_mgc2sp(double *sintbl, char *cmnd, float *p, int len, float *dst)
{
   argv_t *argvt = wtk_sptk_arg_new(cmnd);
   char **argv = argvt->argvs;
   int argc = argvt->argc;
   wtk_sptk_mgc2sp_main(sintbl, argc, argv, p, len, dst);
   wtk_sptk_arg_delete(argvt);
}

static void wtk_sptk_mgc2sp_main( double *sintbl, int argc, char **argv, float *p, int len, float *dst)
{
   int m = ORDER, l = LENG, otype = OTYPE, no, i;
   double alpha = ALPHA, gamma = GAMMA, *c, *x, *y, logk;
   Boolean norm = NORM, phase = PHASE, mulg = MULG;
//    FILE *fp = stdin;
   int k, h;

   // while (--argc)
   for (;argc--;argv++)
   {
      if (**argv == '-') {
         switch (*(*argv + 1)) {
         case 'm':
            m = atoi(*++argv);
            --argc;
            break;
         case 'a':
            alpha = atof(*++argv);
            --argc;
            break;
         case 'g':
            gamma = atof(*++argv);
            --argc;
            break;
         case 'c':
            gamma = atoi(*++argv);
            --argc;
        //     if (gamma < 1)
        //        fprintf(stderr, "%s : value of c must be c>=1!\n", cmnd);
            gamma = -1.0 / gamma;
            break;
         case 'n':
            norm = 1 - norm;
            break;
         case 'u':
            mulg = 1 - mulg;
            break;
         case 'l':
            l = atoi(*++argv);
            --argc;
            break;
         case 'o':
            otype = atoi(*++argv);
            --argc;
            break;
         case 'p':
            phase = 1 - phase;
            break;
        //  case 'h':
        //     usage(0);
        //  default:
        //     fprintf(stderr, "%s : Invalid option '%c'!\n", cmnd, *(*argv + 1));
        //     usage(1);
         }
      }
      /*  else
         fp = getfp(*argv, "rb"); */
   }

   x = dgetmem(l + l + m + 1);
   y = x + l;
   c = y + l;

   no = l / 2 + 1;
   logk = 20.0 / log(10.0);
   
   double *cache;
   cache = dgetmem(l*2);
//    sintbl = getsintbl(l);
   
   // while (freadf(c, sizeof(*c), m + 1, fp) == m + 1) {
   for (k=m+1; k<=len; p+=m+1, k+=m+1)
   {
      for (h=0; h<m+1; h++)
      {
         c[h] = p[h];
      }
      if (norm)
         ignorm(c, c, m, gamma);
      else if (mulg) {
         if (gamma == 0) {
        //     fprintf(stderr,
        //             "%s : gamma for input mgc coefficients should not equal to 0 if you specify -u option!\n",
        //             cmnd);
        //     usage(1);
         }
         c[0] = (c[0] - 1.0) / gamma;
      }

      if (mulg) {
        //  if (gamma == 0) {
        //     fprintf(stderr,
        //             "%s : gamma for input mgc coefficients should not equal to 0 if you specify -u option!\n",
        //             cmnd);
        //     usage(1);
        //  }
         for (i = m; i > 0; i--)
            c[i] /= gamma;
      }

//       mgc2sp(c, m, alpha, gamma, x, y, l);
      sptk_mgc2sp( sintbl, cache, c, m, alpha, gamma, x, y, l);

      if (phase)
         switch (otype) {
         case 1:
            for (i = no; i--;)
               x[i] = y[i];
            break;
         case 2:
            for (i = no; i--;)
               x[i] = y[i] * 180 / PI;
            break;
         default:
            for (i = no; i--;)
               x[i] = y[i] / PI;
            break;
      } else
         switch (otype) {
         case 1:
            break;
         case 2:
            for (i = no; i--;)
               x[i] = exp(x[i]);
            break;
         case 3:
            for (i = no; i--;)
               x[i] = exp(2 * x[i]);
            break;
         default:
            for (i = no; i--;)
               x[i] *= logk;
            break;
         }
      
      for (h=0; h<no; h++)
      {
         dst[h] = x[h];
      }
      dst += no;
      // fwritef(x, sizeof(*x), no, stdout);
   }
   free(x);
//    free(sintbl);
   free(cache);
}
