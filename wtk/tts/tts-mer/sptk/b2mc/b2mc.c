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
*    Transform MLSA Digital Filter Coefficients to Mel Cepstrum         *
*                                                                       *
*                                        1995.12  K.Koishida            *
*                                                                       *
*        usage:                                                         *
*                b2mc [ options ] [ infile ] > stdout                   *
*        options:                                                       *
*                -m m  :  order of mel cepstrum   [25]                  *
*                -a a  :  all-pass constant       [0.35]                *
*        infile:                                                        *
*                MLSA filter coefficients                               *
*                , b(0), b(1), ..., b(M),                               *
*        stdout:                                                        *
*                mel cepstral coefficients                              *
*                , c~(0), c~(1), ..., c~(M),                            *
*        require:                                                       *
*                b2mc()                                                 *
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
#include "b2mc.h"

/*  Default Values  */
#define	ALPHA		0.35
#define ORDER		25

static int wtk_sptk_b2mc_main(int argc, char **argv, float *p, int len, float *dst);

void wtk_sptk_b2mc(char *cmnd, float *p, int len, float *dst)
{
   argv_t *argvt = wtk_sptk_arg_new(cmnd);
   int argc = argvt->argc;
   char **argv = argvt->argvs;
   wtk_sptk_b2mc_main(argc, argv, p, len, dst);
   wtk_sptk_arg_delete(argvt);
}

static int wtk_sptk_b2mc_main(int argc, char **argv, float *p, int len, float *dst)
{
   int m = ORDER, m1;
//    FILE *fp = stdin;
   double a = ALPHA, *x;
   int i, j;

/*    if ((cmnd = strrchr(argv[0], '/')) == NULL)
      cmnd = argv[0]; 
   else
      cmnd++;
   while (--argc) */
   for (;argc--;argv++)
   {
      if (**argv == '-') {
         switch (*(*argv + 1)) {
         case 'a':
            a = atof(*++argv);
            --argc;
            break;
         case 'm':
            m = atoi(*++argv);
            --argc;
            break;
         case 'h':
            // usage(0);
         default:
            // fprintf(stderr, "%s : Invalid option '%c'!\n", cmnd, *(*argv + 1));
            // usage(1);
            break;
         }
      }
      //  else
      //    fp = getfp(*argv, "rb");
   }
   m1 = m + 1;

   x = dgetmem(m1);

   /* while (freadf(x, sizeof(*x), m1, fp) == m1) {
      b2mc(x, x, m, a);
      fwritef(x, sizeof(*x), m1, stdout);
   } */
   i = 0;
   while (i+m1<=len)
   {
      for (j=0; j<m1; j++) {
         x[j] = (double)p[j];
      }
      b2mc(x, x, m, a);
      for (j=0; j<m1; j++) {
         dst[j] = (float)x[j];
      }
      p += m1;
      dst += m1;
      i += m1;
   }
   free(x);
   return (0);
}