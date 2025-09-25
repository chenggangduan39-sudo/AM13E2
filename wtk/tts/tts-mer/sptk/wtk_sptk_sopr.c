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

/*******************************************************************************
*                                                                              *
*    Execute Scalar Operations                                                 *
*                                                                              *
*                                     1990.11 T.Kobayashi                      *
*                                     1996.5  K.Koishida                       *
*                                     2000.5  T.Kobayashi                      *
*                                     2010.6  A.Tamamori                       *
*                                     2010.12 T.Sawada                         *
*                                     2013.3  T.Okada                          *
*       usage:                                                                 *
*               sopr [ options ] [ infile ] > stdout                           *
*       options:                                                               *
*               -a a         :  addition             (in + a)                  *
*               -s s         :  subtraction          (in - s)                  *
*               -m m         :  multiplication       (in * m)                  *
*               -d d         :  division             (in / d)                  *
*               -p p         :  power                (in ^ p)                  *
*               -f f         :  flooring             (in < f -> f)             *
*               -c c         :  ceiling              (in > f -> f)             *
*               -ABS         :  absolute             (abs(in))                 *
*               -INV         :  inverse              (1 / in)                  *
*               -P           :  square               (in * in)                 *
*               -R           :  root                 (sqrt(in))                *
*               -SQRT        :  root                 (sqrt(in))                *
*               -LN          :  logarithm natural    (log(in))                 *
*               -LOG2        :  logarithm to base 2  (log2(in))                *
*               -LOG10       :  logarithm to base 10 (log10(in))               *
*               -LOGX X      :  logarithm to base X  (logX(in))                *
*               -EXP         :  exponential          (exp(in))                 *
*               -POW2        :  power of 2           (2^(in))                  *
*               -POW10       :  power of 10          (10^(in))                 *
*               -POWX X      :  power of X           (X^(in))                  *
*               -FIX         :  round                ((int)in)                 *
*               -UNIT        :  unit step            (u(in))                   *
*               -CLIP        :  clipping             (in * u(in)               *
*               -SIN         :  sin                  (sin(in))                 *
*               -COS         :  cos                  (cos(in))                 *
*               -TAN         :  tan                  (tan(in))                 *
*               -ATAN        :  atan                 (atan(in))                *
*                                                                              *
*               -magic magic :  remove magic number                            *
*               -MAGIC MAGIC :  replace magic number by MAGIC                  *
*                               if -magic option is not given,                 *
*                               return error                                   *
*                               if -magic or -MAGIC option                     *
*                               is given multiple times, return error          *
*               -r mn    :  read from memory register n                        *
*               -w mn    :  write to memory register n                         *
*                                                                              *
*      infile:                                                                 *
*               data sequences (float)                                         *
*      stdout:                                                                 *
*               data sequences after operations                                *
*                                                                              *
*******************************************************************************/

/*  Standard C Libraries  */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#ifndef HAVE_STRRCHR
#define strrchr rindex
#endif
#endif

#include <ctype.h>
#include <math.h>

#include "tts-mer/sptk/SPTK.h"
#include "wtk_sptk_sopr.h"

/*  Command Name  */

/* Default Value  */
#define MEMSIZE 10
#define MAGIC FA
#define REP FA

#define LOG2(x) (log(x) / log(2))

typedef struct
{
   char op[4];
   double d;
   Boolean magic;
   Boolean ifrep;
} operation_t;

/* define */
static void usage(int i)
{
   exit(1);
}
static void wtk_sptk_sopr_main(char *cmnd, int argc, char **argv, float *p, int len);
static int sopr();

/* function */
void wtk_sptk_sopr(char *cmnd, float *p, int len)
{
   argv_t *argvt = wtk_sptk_arg_new(cmnd);
   char **argv = argvt->argvs;
   int argc = argvt->argc;

   wtk_sptk_sopr_main(cmnd, argc, argv, p, len);
   wtk_sptk_arg_delete(argvt);
}

static void wtk_sptk_sopr_main(char *cmnd, int argc, char **argv, float *p, int len)
{
   int magic_count = 0, rep_count = 0;
   char *s, c;
//    char *infile = NULL;
   int nopr = 0;
   int mopr = 0;
   int ropr = 0;

   operation_t *optbl = (operation_t *)calloc(sizeof(operation_t), argc);

   for (; argc--; argv++)
   {
      if (*(s = *argv) == '-')
      {
         c = *++s;
         if (islower(c) && *++s == '\0')
         {
            s = *++argv;
            if ((s == NULL) && (c != 'h'))
            {
        //        fprintf(stderr,
        //                "%s : numerical argument is also needed !\n", cmnd);
        //        usage(1);
            }
            --argc;
         }
         // printf("argv: %s %d\n", *argv, argc);

         switch (c)
         {
         case 'a':
         case 'c':
         case 'd':
         case 'f':
         case 'p':
         case 'P':
         case 'm':
         case 's':
         case 'r':
         case 'w':
         case 'M':
         case 'L':
            if (c == 'P')
            {
               if (strncmp("POWX\0", s, 5) == 0)
               {
                  strncpy(optbl[nopr].op, "POWX", 4);
                  s = *++argv;
                  if (s == NULL)
                  {
                     fprintf(stderr,
                             "%s : numerical argument is also needed !\n",
                             cmnd);
                     usage(1);
                  }
                  --argc;
               }
               else
               {
                  strncpy(optbl[nopr].op, s, 4);
               }
            }
            if (c == 'L')
            {
               if (strncmp("LOGX\0", s, 5) == 0)
               {
                  strncpy(optbl[nopr].op, "LOGX", 4);
                  s = *++argv;
                  if (s == NULL)
                  {
                     fprintf(stderr,
                             "%s : numerical argument is also needed !\n",
                             cmnd);
                     usage(1);
                  }
                  if (atof(s) <= 0)
                  {
                     fprintf(stderr,
                             "%s : base of a logarithm must be positive number !\n",
                             cmnd);
                     usage(1);
                  }
                  --argc;
               }
               else
               {
                  strncpy(optbl[nopr].op, s, 4);
               }
            }
            if ((c == 'm') && strncmp("agic", s, 4) == 0)
            {
               if (magic_count > 0)
               {
                  fprintf(stderr,
                          "%s : Cannot specify -magic option multiple times!\n",
                          cmnd);
                  usage(1);
               }
               optbl[nopr].magic = 1 - MAGIC;
               mopr = nopr;
               magic_count++;
               s = *++argv;
               if (s == NULL)
               { /* No magic number */
                  fprintf(stderr,
                          "%s : -magic option need magic number !\n", cmnd);
                  usage(1);
               }
               --argc;
            }
            if (c == 'M')
            {
               if (rep_count > 0)
               {
                  fprintf(stderr,
                          "%s : Cannot specify -MAGIC option multiple times!\n",
                          cmnd);
                  usage(1);
               }
               if (!optbl[mopr].magic)
               {
                  fprintf(stderr,
                          "%s : Cannot find -magic option befor -MAGIC option!\n",
                          cmnd);
                  usage(1);
               }
               else
               {
                  optbl[nopr].magic = 1 - MAGIC;
                  optbl[nopr].ifrep = 1 - REP;
                  rep_count++;
                  s = *++argv;
                  if (s == NULL)
                  { /* No magic number */
                     fprintf(stderr,
                             "%s : -MAGIC option need magic number !\n", cmnd);
                     usage(1);
                  }
                  --argc;
               }
            }
            if (strncmp("dB", s, 2) == 0)
               optbl[nopr].d = 20 / log(10.0);
            else if (strncmp("cent", s, 4) == 0)
               optbl[nopr].d = 1200 / log(2.0);
            else if (strncmp("octave", s, 6) == 0)
               optbl[nopr].d = 1.0 / log(2.0);
            else if (strncmp("semitone", s, 8) == 0)
               optbl[nopr].d = 12.0 / log(2.0);
            else if (strncmp("pi", s, 2) == 0)
               optbl[nopr].d = PI;
            else if (strncmp("ln", s, 2) == 0)
               optbl[nopr].d = log(atof(s + 2));
            else if (strncmp("exp", s, 3) == 0)
               optbl[nopr].d = exp(atof(s + 3));
            else if (strncmp("sqrt", s, 4) == 0)
               optbl[nopr].d = sqrt(atof(s + 4));
            else if (*s == 'm')
            {
               s = *(argv + 1);
               if (s == NULL)
               {
                  fprintf(stderr,
                          "%s : next operation must be specified !\n", cmnd);
                  usage(1);
               }
               optbl[nopr].d = atoi(s + 1);
               if (c == 'a')
                  c = '+';
               else if (c == 'd')
                  c = '/';
               else if (c == 'm')
                  c = '*';
               else if (c == 's')
                  c = '-';
               else if (c == 'p')
                  c = '^';
            }
            else
            {
               if (isdigit(*s) == 0 && strncmp(optbl[nopr].op, s, 4) != 0)
               { /* Check the value is correct */
                  if ((*s) != '+' && (*s) != '-')
                  {
                     fprintf(stderr,
                             "%s : %s option need numerical number !\n", cmnd,
                             *(argv - 1));
                     usage(1);
                  }
                  else if (isdigit(*(s + 1)) == 0)
                  {
                     fprintf(stderr,
                             "%s : %s option need numerical number !\n", cmnd,
                             *(argv - 1));
                     usage(1);
                  }
               }
               optbl[nopr].d = atof(s);
            }
         case 'A':
         case 'C':
         case 'E':
         case 'F':
         case 'I':
         case 'R':
         case 'S':
         case 'T':
         case 'U':
            if ((c == 'A') || (c == 'C') || (c == 'S'))
               strncpy(optbl[nopr].op, s, 4);
            else
               optbl[nopr].op[0] = c;
            ++nopr;
            break;
         case 'h':
            usage(0);
         default:
            fprintf(stderr, "%s : Invalid option '%c'!\n", cmnd, c);
            usage(1);
         }
      }
//       else
        //  infile = s;
   }

   sopr(p, len, optbl, nopr, mopr, ropr);

   free(optbl);
}

static int sopr(float *p, int len, operation_t *optbl, int nopr, int mopr, int ropr)
{
   double x, y;
   int k, i, j;
   Boolean skipflg = FA;
   double mem[MEMSIZE];

   for (j = 0; j < len; p++, j++)
   // while (freadf(&x, sizeof(x), 1, fp) == 1)
   {
      x = *p;
      for (k = 0; k < MEMSIZE; ++k)
         mem[k] = 0;
      for (k = 0; k < nopr; ++k)
      {
         y = optbl[k].d;
         if (optbl[k].magic)
         { /* -magic or -MAGIC */
            if (optbl[k].ifrep)
            { /* -MAGIC */
               if (x == optbl[mopr].d)
               {         /* still remains magic number */
                  x = y; /* substitute by new magic number */
                  skipflg = FA;
               }
            }
            else if (x == y)
            { /* -magic */
               skipflg = TR;
            }
         }
         else if (skipflg == FA)
         {
            switch (optbl[k].op[0])
            {
            case 'r':
               x = mem[(int)y];
               break;
            case 'w':
               mem[(int)y] = x;
               break;
            case '+':
               x += mem[(int)y];
               break;
            case '-':
               x -= mem[(int)y];
               break;
            case '*':
               x *= mem[(int)y];
               break;
            case '/':
               x /= mem[(int)y];
               break;
            case '^':
               x = pow(x, mem[(int)y]);
               break;
            case 'a':
               x += y;
               break;
            case 's':
               x -= y;
               break;
            case 'm':
               x *= y;
               break;
            case 'd':
               x /= y;
               break;
            case 'p':
               x = pow(x, y);
               break;
            case 'f':
               x = (x < y) ? y : x;
               break;
            case 'c':
               x = (x > y) ? y : x;
               break;
            case 'A':
               if (optbl[k].op[1] == 'T')
                  x = atan(x);
               else if (x < 0)
                  x = -x;
               break;
            case 'C':
               if (optbl[k].op[1] == 'L')
               {
                  if (x < 0)
                     x = 0;
               }
               else
                  x = cos(x);
               break;
            case 'I':
               x = 1 / x;
               break;
            case 'P':
               if (optbl[k].op[1] == 'O' && optbl[k].op[3] == '1')
                  x = pow(10.0, x);
               else if (optbl[k].op[1] == 'O' && optbl[k].op[3] == '2')
                  x = pow(2.0, x);
               else if (optbl[k].op[1] == 'O' && optbl[k].op[3] == 'X')
                  x = pow(y, x);
               else
                  x *= x;
               break;
            case 'R':
               x = sqrt(x);
               break;
            case 'S':
               if (optbl[k].op[1] == 'Q')
                  x = sqrt(x);
               else
                  x = sin(x);
               break;
            case 'E':
               x = exp(x);
               break;
            case 'L':
               if (x <= 0)
                  fprintf(stderr, "WARNING: LOG of zero or negative value !\n");
               if (optbl[k].op[3] == 'X')
                  x = log(x) / log(y);
               else if (optbl[k].op[3] == '1')
                  x = log10(x);
               else if (optbl[k].op[3] == '2')
                  x = LOG2(x);
               else
                  x = log(x);
               break;
            case 'F':
               if (x < 0)
                  i = x - 0.5;
               else
                  i = x + 0.5;
               x = i;
               break;
            case 'T':
               x = tan(x);
               break;
            case 'U':
               if (x < 0)
                  x = 0;
               else
                  x = 1;
            case 'M':

            default:
               break;
            }
         }
      }
      if (skipflg == FA)
      {
         *p = x;
      }
      skipflg = FA;
   }
   return (0);
}
