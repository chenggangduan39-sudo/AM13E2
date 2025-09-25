/*  ---------------------------------------------------------------  */
/*           The HMM-Based Speech Synthesis System (HTS)             */
/*                       HTS Working Group                           */
/*                                                                   */
/*                  Department of Computer Science                   */
/*                  Nagoya Institute of Technology                   */
/*                               and                                 */
/*   Interdisciplinary Graduate School of Science and Engineering    */
/*                  Tokyo Institute of Technology                    */
/*                                                                   */
/*                     Copyright (c) 2001-2007                       */
/*                       All Rights Reserved.                        */
/*                                                                   */
/*  Permission is hereby granted, free of charge, to use and         */
/*  distribute this software and its documentation without           */
/*  restriction, including without limitation the rights to use,     */
/*  copy, modify, merge, publish, distribute, sublicense, and/or     */
/*  sell copies of this work, and to permit persons to whom this     */
/*  work is furnished to do so, subject to the following conditions: */
/*                                                                   */
/*    1. The source code must retain the above copyright notice,     */
/*       this list of conditions and the following disclaimer.       */
/*                                                                   */
/*    2. Any modifications to the source code must be clearly        */
/*       marked as such.                                             */
/*                                                                   */
/*    3. Redistributions in binary form must reproduce the above     */
/*       copyright notice, this list of conditions and the           */
/*       following disclaimer in the documentation and/or other      */
/*       materials provided with the distribution.  Otherwise, one   */
/*       must contact the HTS working group.                         */
/*                                                                   */
/*  NAGOYA INSTITUTE OF TECHNOLOGY, TOKYO INSTITUTE OF TECHNOLOGY,   */
/*  HTS WORKING GROUP, AND THE CONTRIBUTORS TO THIS WORK DISCLAIM    */
/*  ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL       */
/*  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT   */
/*  SHALL NAGOYA INSTITUTE OF TECHNOLOGY, TOKYO INSTITUTE OF         */
/*  TECHNOLOGY, HTS WORKING GROUP, NOR THE CONTRIBUTORS BE LIABLE    */
/*  FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY        */
/*  DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,  */
/*  WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTUOUS   */
/*  ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR          */
/*  PERFORMANCE OF THIS SOFTWARE.                                    */
/*                                                                   */
/*  ---------------------------------------------------------------  */

/* functions for speech processing based on codes by Dr. Hideki Banno */
/*                                                                    */
/* coded for implementing STRAIGHT mixed excitation for HTS           */
/* - Tomoki Toda 12/05/08                                             */

#ifndef __HTS_SP_SUB_H
#define __HTS_SP_SUB_H

#ifdef __cplusplus
extern "C" {
#endif
#ifdef __cplusplus
}
#endif


#ifdef NUL
#undef NUL
#endif
#define NUL		'\0'

#ifdef M_PI
#define PI		M_PI
#else
#define PI		3.1415926535897932385
#endif

#ifndef NULL
#define NULL		0
#endif

#define XBOOL int
#define XTRUE 1
#define XFALSE 0
#define ALITTLE_NUMBER 1.0e-10

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define SQUARE(x) ((x) * (x))
#define CSQUARE(xr, xi) ((xr)*(xr)+(xi)*(xi))
#define POW2(p) (1 << (int)(p))
#define ABS(x) ((x) >= 0 ? (x) : -(x))
#define FABS(x) ((x) >= 0.0 ? (x) : -(x))
#define CABS(xr, xi) sqrt((double)(xr)*(double)(xr)+(double)(xi)*(double)(xi))
#define dB(x) (20.0 * log10((double)(x)))
#define dBpow(x) (10.0 * log10((double)(x)))

#define streq(s1, s2) ((s1 != NULL) && (s2 != NULL) && (strcmp((s1), (s2)) == 0) ? 1 : 0)
#define strneq(s1, s2, n) ((s1 != NULL) && (s2 != NULL) && (strncmp((s1), (s2), n) == 0) ? 1 : 0)
#define strveq(s1, s2) ((s1 != NULL) && (s2 != NULL) && (strncmp((s1), (s2), strlen(s2)) == 0) ? 1 : 0)
#define arraysize(array) ((unsigned int)(sizeof(array) / sizeof(array[0])))
#define samesign(a, b) ((a) * (b) >= 0)
#define eqtrue(value) (((value) == XTRUE) ? 1 : 0)
#define strnone(string) (((string) == NULL || *(string) == NUL) ? 1 : 0)
#define strnull strnone

namespace BASIC {
    double spround(double);
    double rem(double, double);
    void cexp(double *, double *);
    void clog(double *, double *);
    double simple_random();
    double simple_gnoise(double);
    double randn();
};

typedef class DVECTOR_CLASS {
 public:
    long length;
    double *data;
    double *imag;

    DVECTOR_CLASS(long);
    DVECTOR_CLASS(long, double);
    DVECTOR_CLASS(long, double *, double *);
    ~DVECTOR_CLASS();

    void dvifree();
    void dvialloc();
    void dvialloc(double);
    void dvrandn();
    double dvmax(long *);
    double dvmin(long *);
} *DVECTOR;

typedef class DMATRIX_CLASS {
 public:
    long row;
    long col;
    double **data;

    DMATRIX_CLASS(long, long);
    ~DMATRIX_CLASS();
} *DMATRIX;

namespace VOPERATE {
    DVECTOR xdvclone(DVECTOR);
    DVECTOR xdvcplx(DVECTOR, DVECTOR);
    void dvoper(DVECTOR, const char *, DVECTOR);
    DVECTOR xdvoper(DVECTOR, char *, DVECTOR);
    void dvscoper(DVECTOR, const char *, double);
    DVECTOR xdvscoper(DVECTOR, char *, double);
    void dvcumsum(DVECTOR);
    DVECTOR xdvcumsum(DVECTOR);
    void dvpaste(DVECTOR, DVECTOR, long, long, int);
    DVECTOR xdvcut(DVECTOR, long, long);
};

namespace FFTOPE {
    int nextpow2(long n);
    int fft(double *xRe, double *xIm, long fftp, int inv);
    void dvfft(DVECTOR x);
    void dvifft(DVECTOR x);
    DVECTOR xdvfft(DVECTOR x);
    DVECTOR xdvifft(DVECTOR x);
    DVECTOR xdvfft(DVECTOR x, long length);
    DVECTOR xdvifft(DVECTOR x, long length);
    void fftturn(double *xRe, double *xIm, long fftp);
    void dvfftturn(DVECTOR x);
    void fftshift(double *xRe, double *xIm, long fftp);
    void dvfftshift(DVECTOR x);
};

class SMP_GAUSSIAN_RND{
	private:
		long x;
		long a;
		long c;
		long m;
		
	public:
		double simple_random();
		double simple_gnoise(double rms);
		SMP_GAUSSIAN_RND();
		~SMP_GAUSSIAN_RND();
	 
};
#endif /* __HTS_SP_SUB_H */
