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

/**************************************************************************
*                                                                         *
*    Execute Vector Operations                                            *
*                                                                         *
*                                               1988.6  T.Kobayashi       *
*                                               1996.5  K.Koishida        *
*       usage:                                                            *
*               vopr [ options ] [ [ file1 ] [ infile ]  > stdout         *
*       options:                                                          *
*               -l l     :  length of vector                [1]           *
*               -n n     :  order of vector                 [l-1]         *
*               -i       :  specified file contains a and b [FALSE]       *
*               -a       :  addition        (a + b)         [FALSE]       *
*               -s       :  subtraction     (a - b)         [FALSE]       *
*               -m       :  multiplication  (a * b)         [FALSE]       *
*               -d       :  division        (a / b)         [FALSE]       *
*               -ATAN2   :  atan2           atan2(b,a)      [FALSE]       *
*               -AM      :  arithmetic mean ((a + b) / 2)   [FALSE]       *
*               -GM      :  geometric mean  (sqrt(a * b))   [FALSE]       *
*               -c       :  choose smaller value            [FALSE]       *
*               -f       :  choose larger value             [FALSE]       *
*               -gt      :  decide 'greater than'           [FALSE]       *
*               -ge      :  decide 'greater than or equal'  [FALSE]       *
*               -lt      :  decide 'less than'              [FALSE]       *
*               -le      :  decide 'less than or equal'     [FALSE]       *
*               -eq      :  decide 'equal to'               [FALSE]       *
*               -ne      :  decide 'not equal to'           [FALSE]       *
*        notice:                                                          *
*               When both -l and -n are specified,                        *
*               latter argument is adopted.                               *
*               When -gt, -ge, -le, -lt, -eq or -ne is specified,         *
*               each element of output vec. is 1.0 (true) or 0.0 (false). *
*                                                                         *
***************************************************************************/

/*  Standard C Libraries  */
#include <stdio.h>

#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#ifndef HAVE_STRRCHR
#define strrchr rindex
#endif
#endif

#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#include "../SPTK.h"
#include "vopr.h"

/*  Default Values  */
#define LENG 1
#define INV  FA

static int vopr(int opr, int leng, float *p, int len, float *p2, int len2, float *dst);
static int wtk_sptk_vopr_main(int argc, char **argv, float *p, int len, float *p2, int len2, float *dst);

void wtk_sptk_vopr(char *cmnd, float *p, int len, float *p2, int len2, float *dst)
{
   argv_t *argvt = wtk_sptk_arg_new(cmnd);
   int argc = argvt->argc;
   char **argv = argvt->argvs;
   wtk_sptk_vopr_main(argc, argv, p, len, p2, len2, dst);
   wtk_sptk_arg_delete(argvt);
}

static int wtk_sptk_vopr_main(int argc, char **argv, float *p, int len, float *p2, int len2, float *dst)
{
   Boolean inv = INV;
   int opr = ' ', leng = LENG;
   char *s, c;
   // int vopr(FILE * fp1, FILE * fp2);

   // if ((cmnd = strrchr(argv[0], '/')) == NULL)
   //    cmnd = argv[0];
   // else
   //    cmnd++;

   // while (--argc) 
   for (;argc--;argv++)
   {
      if (*(s = *argv) == '-') {
         c = *++s;
         if (*++s == '\0' && (c == 'l' || c == 'n')) {
            s = *++argv;
            --argc;
         }
         switch (c) {
         case 'l':
            if ((c == 'l') && strncmp("e", s, 1) == 0) {
               opr = 'r';
            } else if ((c == 'l') && strncmp("t", s, 1) == 0) {
               opr = 'R';
            } else {
               leng = atoi(s);
            }
            break;
         case 'n':
            if ((c == 'n') && strncmp("e", s, 1) == 0) {
               opr = 'E';
            } else {
               leng = atoi(s) + 1;
            }
            break;
         case 'i':
            inv = 1 - inv;
            break;
         case 'a':
         case 'd':
         case 'm':
         case 's':
         case 'A':
         case 'c':
         case 'f':
         case 'g':
         case 'G':
         case 'e':
            opr = c;
            if ((c == 'A') && strncmp("M", s, 1) == 0) {
               opr = 'p';
            } else if ((c == 'G') && strncmp("M", s, 1) == 0) {
               opr = 'P';
            } else if ((c == 'g') && strncmp("e", s, 1) == 0) {
               opr = 'q';
            } else if ((c == 'g') && strncmp("t", s, 1) == 0) {
               opr = 'Q';
            }
            break;
        //  case 'h':
        //     usage(0);
        //  default:
        //     fprintf(stderr, "%s : Invalid option '%c'!\n", cmnd, *(*argv + 1));
        //     usage(1);
         }
      } /* else
         infile[nfiles++] = s; */
   }

   /* if (nfiles == 0)
      vopr(stdin, stdin);
   else {
      fp1 = getfp(infile[0], "rb");
      if (nfiles == 1) {
         if (inv)
            vopr(fp1, fp1);
         else
            vopr(stdin, fp1);
      } else {
         fp2 = getfp(infile[1], "rb");
         vopr(fp1, fp2);
      }
   } */
   vopr(opr, leng, p, len, p2, len2, dst);

   return (0);
}

static int vopr(int opr, int leng, float *p, int len, float *p2, int len2, float *dst)
{
   double *a, *b;
   int k;
   int i, i2, j;

   a = dgetmem(leng + leng);
   b = a + leng;

   /* if (fp1 != fp2 && leng > 1) {
      if (freadf(b, sizeof(*b), leng, fp2) != leng)
         return (1);
   } */
   if (p != p2 && leng > 1) 
   {
      if (len2 < leng) {
         printf("%s:%d: p2没有足够的长度或内容\n", __FILE__, __LINE__);
         exit(1);
      }
      for (j=0; j<leng; j++) {
         b[j] = (double)p2[j];
      }
   }

   /* while (freadf(a, sizeof(*a), leng, fp1) == leng)  */
   i = i2 = 0;
   while (i+leng<=len)
   {
      for (j=0; j<leng; j++) {
         a[j] = (double)p[j];
      }
      p += leng;
      i += leng;
      
      /* if (fp1 == fp2 || leng == 1) {
         if (freadf(b, sizeof(*b), leng, fp2) != leng)
            return (1);
      } */
      if (p == p2 || leng == 1)
      {
         i2 += leng;
         if (i2 > len2) {
            printf("%s:%d: p2没有足够的长度或内容\n", __FILE__, __LINE__);
            exit(1);
         }
         for (j=0; j<leng; j++) {
            b[j] = (double)p2[j];
         }
         p2 += leng;
      }
        
      switch (opr) {
      case 'a':
         for (k = 0; k < leng; ++k)
            a[k] += b[k];
         break;
      case 's':
         for (k = 0; k < leng; ++k)
            a[k] -= b[k];
         break;
      case 'm':
         for (k = 0; k < leng; ++k)
            a[k] *= b[k];
         break;
      case 'd':
         for (k = 0; k < leng; ++k)
            a[k] /= b[k];
         break;
      case 'A':
         for (k = 0; k < leng; ++k)
            a[k] = atan2(b[k], a[k]);
         break;
      case 'c':                /* choose smaller one */
         for (k = 0; k < leng; ++k) {
            if (a[k] > b[k]) {
               a[k] = b[k];
            }
         }
         break;
      case 'f':                /* choose larger one */
         for (k = 0; k < leng; ++k) {
            if (a[k] < b[k]) {
               a[k] = b[k];
            }
         }
         break;
      case 'p':                /* arithmetic mean */
         for (k = 0; k < leng; ++k)
            a[k] = (a[k] + b[k]) / 2;
         break;
      case 'P':                /* geometric mean */
         for (k = 0; k < leng; ++k) {
            double tmp = a[k] * b[k];
            if (tmp < 0.0) {
        //        fprintf(stderr, "%s : Can't calculate geometric mean !\n", cmnd);
        //        usage(1);
            }
            a[k] = sqrt(tmp);
         }
         break;
      case 'q':                /* greater than or equal */
         for (k = 0; k < leng; ++k) {
            if (a[k] >= b[k]) {
               a[k] = 1.0;
            } else {
               a[k] = 0.0;
            }
         }
         break;
      case 'Q':                /* greater than */
         for (k = 0; k < leng; ++k) {
            if (a[k] > b[k]) {
               a[k] = 1.0;
            } else {
               a[k] = 0.0;
            }
         }
         break;
      case 'r':                /* less than or equal */
         for (k = 0; k < leng; ++k) {
            if (a[k] <= b[k]) {
               a[k] = 1.0;
            } else {
               a[k] = 0.0;
            }
         }
         break;
      case 'R':                /* less than */
         for (k = 0; k < leng; ++k) {
            if (a[k] < b[k]) {
               a[k] = 1.0;
            } else {
               a[k] = 0.0;
            }
         }
         break;
      case 'e':                /* equal to */
         for (k = 0; k < leng; ++k) {
            if (a[k] == b[k]) {
               a[k] = 1.0;
            } else {
               a[k] = 0.0;
            }
         }
         break;
      case 'E':                /* not equal to */
         for (k = 0; k < leng; ++k) {
            if (a[k] != b[k]) {
               a[k] = 1.0;
            } else {
               a[k] = 0.0;
            }
         }
         break;
      default:
         break;
      }

      // fwritef(a, sizeof(*a), leng, stdout);
      for (j=0; j<leng; j++) {
         dst[j] = (float)a[j];
      }
      dst += leng;
   }
   free(a);
   return (0);
}