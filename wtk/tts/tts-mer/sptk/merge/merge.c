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
*    Data Merge                                                         *
*                                                                       *
*                                       1986.6  K.Tokuda                *
*                                       1996.5  K.Koishida              *
*                                                                       *
*       usage:                                                          *
*               merge [options] file1 [infile] > stdout                 *
*       options:                                                        *
*               -s s     :  insert point                        [0]     *
*               -l l     :  frame length of input data          [25]    *
*               -n n     :  order of input data                 [l-1]   *
*               -L L     :  frame length of insert data         [10]    *
*               -N N     :  order of insert data                [L-1]   *
*               -o       :  over write mode                     [FALSE] *
*               +type    :  data type                           [f]     *
*                           c (char)           C  (unsigned char)       *
*                           s (short)          S  (unsigned short)      *
*                           i (int)            I  (unsigned int)        *
*                           i3 (int, 3byte)    I3 (unsigned int, 3byte) *
*                           l (long)           L  (unsigned long)       *
*                           le (long long)     LE (unsigned long long)  *
*                           f (float)          d  (double)              *
*                           de (long double)                            *
*       file1:  inserted data   , x(0), x(1), ..., x(l-1)               *
*       file2:  input data      , y(0), y(1), ..., y(n-1)               *
*       stdout:                                                         *
*               x(0), ..., x(s), y(0), ...,y(n-1), x(s+1), ..., x(n-1)  *
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

#include <ctype.h>

#include "../SPTK.h"
#include "merge.h"

/*  Default Values  */
#define START 0
#define LENG1 25
#define LENG2 10
#define WRITE FA

/*  Command Name  */
static int wtk_sptk_merge_main(int argc, char **argv, void *p, size_t len, void *p2, size_t len2, void *dst);


void wtk_sptk_merge(char *cmnd, void *p, size_t len, void *p2, size_t len2, void *dst)
{
   argv_t *argvt = wtk_sptk_arg_new(cmnd);
   int argc = argvt->argc;
   char **argv = argvt->argvs;
   wtk_sptk_merge_main(argc, argv, p, len, p2, len2, dst);
   wtk_sptk_arg_delete(argvt);
}

static int wtk_sptk_merge_main(int argc, char **argv, void *p, size_t len, void *p2, size_t len2, void *dst)
{
   int start = START, leng1 = LENG1, leng2 = LENG2, i, j, flag = 1;
   size_t size = sizeof(float);
   Boolean write = WRITE;
   char *y, c, *s;
   long double x;

   /* if ((cmnd = strrchr(argv[0], '/')) == NULL)
      cmnd = argv[0];
   else
      cmnd++;
   while (--argc) */
   for (;argc--;argv++)
   {
      if (*(s = *argv) == '-') {
         c = *++s;
         switch (c) {
         case 's':
            start = atoi(*++argv);
            --argc;
            break;
         case 'l':
            leng1 = atoi(*++argv);
            --argc;
            break;
         case 'n':
            leng1 = atoi(*++argv) + 1;
            --argc;
            break;
         case 'L':
            leng2 = atoi(*++argv);
            --argc;
            break;
         case 'N':
            leng2 = atoi(*++argv) + 1;
            --argc;
            break;
         case 'o':
            write = 1 - write;
            break;
        //  case 'h':
        //     usage(0);
        //  default:
        //     fprintf(stderr, "%s : Invalid option '%c'!\n", cmnd, *(*argv + 1));
        //     usage(1);
         }
      } else if (*s == '+') {
         c = *++s;
         switch (c) {
         case 'c':
            size = sizeof(char);
            break;
         case 's':
            size = sizeof(short);
            break;
         case 'i':
            if (*(s + 1) == '3') {
               size = 3;
               (*argv)++;
            } else {
               size = sizeof(int);
            }
            break;
         case 'l':
            if (*(s + 1) == 'e') {
               size = sizeof(long long);
               (*argv)++;
            } else {
               size = sizeof(long);
            }
            break;
         case 'C':
            size = sizeof(unsigned char);
            break;
         case 'S':
            size = sizeof(unsigned short);
            break;
         case 'I':
            if (*(s + 1) == '3') {
               size = 3;
               (*argv)++;
            } else {
               size = sizeof(unsigned int);
            }
            break;
         case 'L':
            if (*(s + 1) == 'E') {
               size = sizeof(unsigned long long);
               (*argv)++;
            } else {
               size = sizeof(unsigned long);
            }
            break;
         case 'f':
            size = sizeof(float);
            break;
         case 'd':
            if (*(s + 1) == 'e') {
               size = sizeof(long double);
               (*argv)++;
            } else {
               size = sizeof(double);
            }
            break;
        //  default:
        //     fprintf(stderr, "%s : Invalid option '%c'!\n", cmnd, *(*argv + 1));
        //     usage(1);
         }
      } /* else if (fp2 == NULL)
         fp2 = getfp(*argv, "rb");
      else
         fp1 = getfp(*argv, "rb"); */
   }

   /* if (fp2 == NULL) {
      fprintf(stderr, "%s : Inserted data must be specified !\n", cmnd);
      usage(1);
   } */

   y = (char *) dgetmem(leng2 * size);

   /* for (;;) {
      for (j = start, i = leng1; j-- && i--;) {
         if (freadx(&x, size, 1, fp1) != 1)
            break;
         fwritex(&x, size, 1, stdout);
      }
      for (j = leng2; j--;)
         if (write) {
            if (freadx(&x, size, 1, fp1) != 1)
               break;
            i--;
         }

      if (freadx(y, size, leng2, fp2) != leng2)
         if (!flag)
            break;

      fwritex(y, size, leng2, stdout);
      flag = 0;
      for (; i-- > 0;) {
         if (freadx(&x, size, 1, fp1) != 1)
            break;
         fwritex(&x, size, 1, stdout);
      }
   } */

   char *cp = (char*)p
      , *cp2 = (char*)p2
      , *cdst = (char*)dst;
   size_t pi = 0, pi2 = 0, dsti = 0;
   while (1)
   {
      for (j = start, i = leng1; j-- && i--;) {
         /* if (freadx(&x, size, 1, fp1) != 1)
            break;
         fwritex(&x, size, 1, stdout); */
         if (pi == len)
            break;
         memcpy(&x, cp, size);
         memcpy(cdst, &x, size);
         cp += size;
         pi += size;
         cdst += size;
         dsti += size;
      }
      for (j = leng2; j--;)
         if (write) {
            /* if (freadx(&x, size, 1, fp1) != 1)
               break; */
            if ( pi == len)
               break;
            memcpy(&x, cp, size);
            cp += size;
            pi += size;
            i--;
         }

      /* if (freadx(y, size, leng2, fp2) != leng2)
         if (!flag)
            break; */
      size_t slen = size*leng2;
      if (pi2 + slen > len2) {
         if (!flag)
            break;
      } else {
         memcpy(y, cp2, slen);
         cp2 += slen;
         pi2 += slen;
      }

      /* fwritex(y, size, leng2, stdout);
      flag = 0;
      for (; i-- > 0;) {
         if (freadx(&x, size, 1, fp1) != 1)
            break;
         fwritex(&x, size, 1, stdout);
      } */

      memcpy(cdst, y, slen);
      dsti += slen;
      cdst += slen;
      flag = 0;
      for (; i-- > 0;) {
         /* if (freadx(&x, size, 1, fp1) != 1)
            break;
         
         fwritex(&x, size, 1, stdout); */
         if (pi == len)
            break;
         memcpy(&x, cp, size);
         memcpy(cdst, &x, size);
         cp += size;
         cdst += size;
         dsti += size;
         pi += size;
      }
   }
   /* if ((fgetc(fp1) == EOF) && (fgetc(fp2) == EOF)) {
      if (feof(fp1) && feof(fp2))
         return (0);
   } */
   free(y);
   return (1);
}