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
*    Frequency Transformation                                           *
*                                                                       *
*                                       1988.2  K.Tokuda                *
*                                       1996.1  K.Koishida              *
*                                                                       *
*       usage:                                                          *
*               freqt [ options ] [ infile ] > stdout                   *
*       options:                                                        *
*               -m m     :  order of minimum phase sequence     [25]    *
*               -M M     :  order of warped sequence            [25]    *
*               -a a     :  all-pass constant of input sequence [0]     *
*               -A A     :  all-pass constant of output sequence[0.35]  *
*       infile:                                                         *
*               minimum phase sequence                                  *
*                   , c(0), c(1), ..., c(M),                            *
*       stdout:                                                         *
*               warped sequence                                         *
*                   , c~(0), c~(1), ..., c~(N),                         *
*       require:                                                        *
*               freqt()                                                 *
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
#include "freqt.h"

/*  Default Values  */
#define ORDERC1 25
#define ORDERC2 25
#define ALPHA1 0.0
#define ALPHA2 0.35

/*  Command Name  */
static int wtk_sptk_freqt_main(int argc, char **argv, float *p, int len, float *dst);

void wtk_sptk_freqt(char *cmnd, float *p, int len, float *dst)
{
   argv_t *argvt = wtk_sptk_arg_new(cmnd);
   int argc = argvt->argc;
   char **argv = argvt->argvs;
   wtk_sptk_freqt_main(argc, argv, p, len, dst);
   wtk_sptk_arg_delete(argvt);
}

static int wtk_sptk_freqt_main(int argc, char **argv, float *p, int len, float *dst)
{
   int m1 = ORDERC1, m2 = ORDERC2, i, j, m, n;
   double *c1, *c2, a1 = ALPHA1, a2 = ALPHA2, a;

   /* if ((cmnd = strrchr(argv[0], '/')) == NULL)
      cmnd = argv[0];
   else
      cmnd++; */

   /* while (--argc) */
   for (;argc--;argv++)
   {
      /* printf("argv %s\n", *argv); */
      if (**argv == '-') {
         switch (*(*argv + 1)) {
         case 'm':
            m1 = atoi(*++argv);
            --argc;
            break;
         case 'M':
            m2 = atoi(*++argv);
            --argc;
            break;
         case 'a':
            a1 = atof(*++argv);
            --argc;
            break;
         case 'A':
            a2 = atof(*++argv);
            --argc;
            break;
         case 'h':
        //     usage(0);
         default:
        //     fprintf(stderr, "%s : Invalid option '%c'!\n", cmnd, *(*argv + 1));
        //     usage(1);
            break;
         }
      } /* else
         fp = getfp(*argv, "rb"); */
   }

   c1 = dgetmem(m1 + m2 + 2);
   c2 = c1 + m1 + 1;

   a = (a2 - a1) / (1 - a1 * a2);

   // while (freadf(c1, sizeof(*c1), m1 + 1, fp) == m1 + 1) {
   //    freqt(c1, m1, c2, m2, a);
   //    fwritef(c2, sizeof(*c2), m2 + 1, stdout);
   // }
   if (p == dst)
   {
      printf("%s:%d:    ", __FUNCTION__, __LINE__);
      printf("Input and output memory are the same:  p == dst\n");
      exit(1);
   }

   i = 0;
   m = m1+1;
   n = m2+1;
   while (i+m<=len)
   {
      for (j=0; j<m; j++) {
         c1[j] = (double)p[j];
      }
      freqt(c1, m1, c2, m2, a);
      for (j=0; j<n; j++) {
         dst[j] = (float)c2[j];
      }
      p += m;
      i += m;
      dst += n;
   }
   free(c1);
   return 0;
}