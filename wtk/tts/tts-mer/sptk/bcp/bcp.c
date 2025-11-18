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
*    Block Copy                                                         *
*                                                                       *
*                                       1988.7  T.Kobayashi             *
*                                       1996.5  K.Koishida              *
*                                                                       *
*       usage:                                                          *
*               bcp [options] [infile] > stdout                         *
*       options:                                                        *
*               -l l     :  number of items contained 1 block   [512]   *
*               -L L     :  number of destination block size    [N/A]   *
*               -n n     :  order of items contained 1 block    [l-1]   *
*               -N N     :  order of destination block size     [N/A]   *
*               -s s     :  start number                        [0]     *
*               -S S     :  start number in destination block   [0]     *
*               -e e     :  end number                          [EOF]   *
*               -f f     :  fill into empty block               [0.0]   *
*               +type    :  data type                           [f]     *
*                           c (char)           C  (unsigned char)       *
*                           s (short)          S  (unsigned short)      *
*                           i (int)            I  (unsigned int)        *
*                           i3 (int, 3byte)    I3 (unsigned int, 3byte) *
*                           l (long)           L  (unsigned long)       *
*                           le (long long)     LE (unsigned long long)  *
*                           f (float)          d  (double)              *
*                           de (long double)   a  (ascii)               *
*       infile:                                                         *   
*               data sequence                                   [stdin] *
*       stdout:                                                         *
*               copied data sequence                                    *
*       notice:                                                         *
*               When both (-L and -N) or (-l and -n) are specified,     *
*               latter argument is adopted.                             *
*                                                                       *
************************************************************************/

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

#include "../SPTK.h"
#include "bcp.h"

/*  Default Values  */
#define START 0
#define END -1
#define DSTART 0
#define ITEM 512
#define DITEM 0
#define FILL 0.0


/*  Command Name  */
typedef union typex {
   char c;
   short s;
   int i;
   int i3;
   long l;
   long long le;
   unsigned char C;
   unsigned short S;
   unsigned int I;
   unsigned int I3;
   unsigned long L;
   unsigned long long LE;
   float f;
   double d;
   long double de;
} typex_u;

static void bcp(long double fl, char type, int sno, int eno, int dsno, int size, int nitems, int dnitems, void *p, size_t pbytes, void *dst);
static int wtk_sptk_bcp_main(int argc, char **argv, void *p, size_t pbytes, void *dst);

void wtk_sptk_bcp(char *cmnd, void *p, int pbytes, void *dst)
{/* 
outlen  (e + 1 - s) * (plen/(n+1))
 */
   argv_t *argvt = wtk_sptk_arg_new(cmnd);
   int argc = argvt->argc;
   char **argv = argvt->argvs;
   wtk_sptk_bcp_main(argc, argv, p, pbytes, dst);
   wtk_sptk_arg_delete(argvt);
}

static int wtk_sptk_bcp_main(int argc, char **argv, void *p, size_t pbytes, void *dst)
{
   char *s, c;
   int sno = START, eno = END, dsno = DSTART, size = sizeof(float), nitems = ITEM, dnitems = DITEM;
   long double fl = FILL;
   char type = 'f';

   // void bcp(FILE * fp);

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
         case 'n':
            nitems = atoi(*++argv) + 1;
            --argc;
            break;
         case 'N':
            dnitems = atoi(*++argv) + 1;
            --argc;
            break;
         case 'b':
            nitems = atoi(*++argv);
            --argc;
            break;
         case 'l':
            nitems = atoi(*++argv);
            --argc;
            break;
         case 'B':
            dnitems = atoi(*++argv);
            --argc;
            break;
         case 'L':
            dnitems = atoi(*++argv);
            --argc;
            break;
         case 's':
            sno = atoi(*++argv);
            --argc;
            break;
         case 'S':
            dsno = atoi(*++argv);
            --argc;
            break;
         case 'e':
            eno = atoi(*++argv) + 1;
            --argc;
            break;
         case 'f':
            fl = atof(*++argv);
            --argc;
            break;
         case 'h':
        //     usage(0);
         default:
        //     fprintf(stderr, "%s : Invalid option '%c'!\n", cmnd, *(*argv + 1));
        //     usage(1);
            break;
         }
      } else if (*s == '+') {
         type = *++s;
         switch (type) {
         case 'a':
            size = 0;
            break;
         case 'c':
            size = sizeof(char);
            break;
         case 's':
            size = sizeof(short);
            break;
         case 'i':
            if (*(s + 1) == '3') {
               size = 3;
               type = 't';
               (*argv)++;
            } else {
               size = sizeof(int);
            }
            break;
         case 'l':
            if (*(s + 1) == 'e') {
               size = sizeof(long long);
               type = 'u';
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
               type = 'T';
               (*argv)++;
            } else {
               size = sizeof(unsigned int);
            }
            break;
         case 'L':
            if (*(s + 1) == 'E') {
               size = sizeof(unsigned long long);
               type = 'U';
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
               type = 'v';
               (*argv)++;
            } else {
               size = sizeof(double);
            }
            break;
         default:
        //     fprintf(stderr, "%s : Invalid option '%c'!\n", cmnd, *(*argv + 1));
        //     usage(1);
            break;
         }
      } 
//       else if (size == 0)
//          fp = getfp(*argv, "rt");
//       else
//          fp = getfp(*argv, "rb");
   }

   if (eno < 0)
      eno = nitems;
   if (sno < 0 || sno >= nitems || sno > eno || eno > nitems || dsno < 0)
      return (1);

   if (dnitems == 0)
      dnitems = eno - sno + dsno;

   bcp( fl, type, sno, eno, dsno, size, nitems, dnitems, p, pbytes, dst);

   return (0);
}

static void bcp(long double fl, char type, int sno, int eno, int dsno, int size, int nitems, int dnitems, void *p, size_t pbytes, void *dst)
{
   char *buf, *lz = NULL, *fz = NULL;
   int ibytes, obytes, offset, nlz, nfz;
   typex_u fillx;
   // void acopy(FILE * fp);
   void filln(char *ptr, int size, int nitem, typex_u fillx);

   switch (type) {
   case 'c':
      fillx.c = (char) fl;
      break;
   case 's':
      fillx.s = (short) fl;
      break;
   case 'i':
      fillx.i = (int) fl;
      break;
   case 't':
      fillx.i3 = (int) fl;
      break;
   case 'l':
      fillx.l = (long) fl;
      break;
   case 'u':
      fillx.le = (long long) fl;
      break;
   case 'C':
      fillx.c = (unsigned char) fl;
      break;
   case 'S':
      fillx.s = (unsigned short) fl;
      break;
   case 'I':
      fillx.i = (unsigned int) fl;
      break;
   case 'T':
      fillx.I3 = (unsigned int) fl;
      break;
   case 'L':
      fillx.l = (unsigned long) fl;
      break;
   case 'U':
      fillx.le = (unsigned long long) fl;
      break;
   case 'f':
      fillx.f = (float) fl;
      break;
   case 'd':
      fillx.d = (double) fl;
      break;
   case 'v':
      fillx.de = (long double) fl;
      break;
   case 'a':
      break;
   }

   /* if (size == 0) {
      acopy(fp);
      return;
   } */
   if (size == 0) {
      printf("%s:%d: 不支持size为0\n", __FILE__, __LINE__);
      exit(1);
   }

   ibytes = size * nitems;
   offset = size * sno;
   obytes = size * (eno - sno);
   nlz = size * dsno;
   nfz = ((nfz = size * dnitems - nlz - obytes) < 0) ? 0 : nfz;
   
   /* printf("size: %d\n", size);
   printf("nitems: %d\n", nitems);
   printf("ibytes: %d\n", ibytes);
   printf("obytes: %d\n", obytes);
   printf("sno: %d\n", sno);
   printf("eno: %d\n", eno);
   printf("dsno: %d\n", dsno);
   printf("nfz: %d\n", nfz);
   printf("nlz: %d\n", nlz); */
   if ((buf = (char *) dgetmem(ibytes + nlz + nfz)) == NULL)
      return;

   if (nlz) {
      lz = buf + ibytes;
      filln(lz, size, nlz, fillx);
   }

   if (nfz) {
      fz = buf + ibytes + nlz;
      filln(fz, size, nfz, fillx);
   }

   // while (freadx(buf, sizeof(*buf), ibytes, fp) == ibytes) {
   //    if (nlz)
   //       fwritex(lz, sizeof(*lz), nlz, stdout);
   //    fwritex(buf + offset, sizeof(*buf), obytes, stdout);

   //    if (nfz)
   //       fwritex(fz, sizeof(*fz), nfz, stdout);
   // }
   int i = 0;
   char *cp = (char*)p;
   char *cdst = (char*)dst;
   while (i+ibytes <= pbytes)
   {
      memcpy(buf, cp, sizeof(*buf)*ibytes);

      if (nlz) {
         memcpy(cdst, lz, sizeof(*lz)*nlz);
         cdst += nlz;
      }

      memcpy(cdst, buf + offset, sizeof(*buf) * obytes);
      cdst += obytes;

      if (nfz) {
         memcpy(cdst, fz, sizeof(*fz)*nfz);
         cdst += nfz;
      }

      cp += ibytes;
      i += ibytes;
   }
   free(buf);
}

/* void acopy(FILE * fp)
{
   char s[512];
   int n, dn;
   int getstr(FILE * fp, char *s);

   for (dn = 0; !feof(fp);) {
      for (n = 0; n < sno; n++)
         if (getstr(fp, s) == 0)
            break;

      for (; n < eno; n++) {
         if (getstr(fp, s) == 0)
            break;
         if (dn++)
            putchar(' ');
         printf("%s", s);
         if (dn == dnitems) {
            putchar('\n');
            dn = 0;
         }
      }
      for (; n < nitems; n++)
         if (getstr(fp, s) == 0)
            break;
   }
} */

int getstr(FILE * fp, char *s)
{
   int c;

   while ((c = getc(fp)) == ' ' || c == 't' || c == '\n' || c == ',');

   if (c == EOF)
      return (0);

   for (*s++ = c;
        (c = getc(fp)) != EOF && c != ' ' && c != '\t' && c != '\n'
        && c != ',';)
      *s++ = c;

   *s = '\0';
   return (1);
}

void filln(char *ptr, int size, int nitem, typex_u fillx)
{
   int i;
   char *c;

   nitem = nitem / size;
   while (nitem--) {
      c = &fillx.c;
      for (i = 1; i <= size; i++) {
         *ptr++ = *c++;
      }
   }
}