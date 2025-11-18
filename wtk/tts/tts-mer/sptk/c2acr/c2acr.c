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
*    Transform Cepstrum to Autocorrelation                              *
*                                                                       *
*                                       1986.9  K.Tokuda                *
*                                       1996.1  K.Koishida              *
*                                                                       *
*       usage:                                                          *
*               c2acr [ options ] [ infile ] > stdout                   *
*       options:                                                        *
*               -m m     :  order of cepstrum          [25]             *
*               -M M     :  order of autocorrelation   [25]             *
*               -l l     :  FFT length                 [256]            *
*       infile:                                                         *
*               cepstral coefficients                                   *
*                   , c(0), c(1), ..., c(M),                            *
*       stdout:                                                         *
*               autocorrelation coefficeints                            *
*                   , r(0), r(1), ..., r(N),                            *
*       require:                                                        *
*               c2acr()                                                 *
*                                                                       *
************************************************************************/

/*  Standard C Libraries  */
#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#ifndef HAVE_STRRCHR
#define strrchr rindex
#endif
#endif

#include "../SPTK.h"
#include "c2acr.h"

/*  Default Values  */
#define ORDERC 25
#define ORDERR 25
#define FLENG 256

/*  Command Name  */
static int wtk_sptk_c2acr_main( double *sintbl, int argc, char **argv, float *p, int len, float *dst);

void wtk_sptk_c2acr(double *sintbl, char *cmnd, float *p, int len, float *dst)
{
   argv_t *argvt = wtk_sptk_arg_new(cmnd);
   int argc = argvt->argc;
   char **argv = argvt->argvs;
   wtk_sptk_c2acr_main( sintbl, argc, argv, p, len, dst);
   wtk_sptk_arg_delete(argvt);
}

static int wtk_sptk_c2acr_main(double *sintbl, int argc, char **argv, float *p, int len, float *dst)
{
   int m = ORDERC, n = ORDERR, l = FLENG, i, j;
//    FILE *fp = stdin;
   double *c, *r;

//    if ((cmnd = strrchr(argv[0], '/')) == NULL)
//       cmnd = argv[0];
//    else
//       cmnd++;

//    while (--argc)
   for (;argc--;argv++)
   {
      if (**argv == '-') {
         switch (*(*argv + 1)) {
         case 'm':
            m = atoi(*++argv);
            --argc;
            break;
         case 'M':
            n = atoi(*++argv);
            --argc;
            break;
         case 'l':
            l = atoi(*++argv);
            --argc;
            break;
         case 'h':
            // usage(0);
         default:
            // fprintf(stderr, "%s : Invalid option '%c'!\n", cmnd, *(*argv + 1));
            // usage(1);
            break;
         }
      } /* else
         fp = getfp(*argv, "rb"); */
   }

   c = dgetmem(m + n + 2);
   r = c + m + 1;

   // while (freadf(c, sizeof(*c), m + 1, fp) == m + 1) {
   //    c2acr(c, m, r, n, l);
   //    fwritef(r, sizeof(*r), n + 1, stdout);
   // }
   if (p == dst)
   {
      printf("%s:%d:    ", __FUNCTION__, __LINE__);
      printf("%p:%p:    \n", p, dst);
      printf("Input and output memory are the same:  p == dst\n");
      exit(1);
   }

   i=0;
   int m1=m+1, n1=n+1;
   double *cache;
   cache = dgetmem(l*2);

   while (i+m1<=len)
   {
      for (j=0;j<m1;j++) {
         c[j] = (double)p[j];
      }
      // c2acr(c, m, r, n, l);
      sptk_c2acr( sintbl, cache, c, m, r, n, l);
      for (j=0;j<n1;j++) {
         dst[j] = (float)r[j];
      }
      p += m1;
      i += m1;
      dst += n1;
   }
   free(c);
   free(cache);
   return (0);
}