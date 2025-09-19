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
/*    vocoder.h:  mel-cepstral vocoder                               */
/*               (pulse/noise excitation & MLSA filter)              */
/*  ---------------------------------------------------------------  */

/* slightly modified - Tomoki Toda 12/05/08 */
/* STRAIGHT mixed excitation has been implemented */

#ifndef __HTS_VOCODER_H
#define __HTS_VOCODER_H

#include "hts_sp_sub.h"

#ifdef __cplusplus
extern "C" {
#endif
#ifdef __cplusplus
}
#endif
    
#define RANDMAX 32767 

#define   IPERIOD    1
#define   SEED       1
#define   B0         0x00000001
#define   B28        0x10000000
#define   B31        0x80000000
#define   B31_       0x7fffffff
#define   Z          0x00000000

#ifdef HTS_EMBEDDED
   #define   GAUSS      0
   #define   PADEORDER  4  /* pade order (for MLSA filter) */
   #define   IRLENG     64 /* length of impulse response */
#else
   #define   GAUSS      1
   #define   PADEORDER  5
   #define   IRLENG     96
#endif

/* Vocoder: structure for setting of vocoder */
typedef class Vocoder_CLASS {
  private:
    int order;
    int iprd;
    int pd;
    double alpha;
    double beta;
    double pade[21];
    double *ppade;
    double *pb;
    double *cb;
    double *binc;
    double *d1;
    /* for postfiltering */
    int irleng;  
    double *pfd; 
    double *pfg;
    double *pfmc;
    double *pfcep;
    double *pfir;

    void movem(double *, double *, const int);
    void mc2b(double *);
    void b2mc();
    void freqt();
    void c2ir();
    double b2en();
    void postfilter(double *);
    double mlsafir(const double, double *);
    double mlsadf1(double, double *);
    double mlsadf2(double, double *);
    double mlsadf(double);

  public:
    Vocoder_CLASS(const int, globalP);
    ~Vocoder_CLASS();
    void mlsafilter(double *, double *, int, FILE *);
} *Vocoder;


/* pulse with random phase */
typedef class RPHAPF_CLASS {
  private:
    double gdbw;
    double gdsd;
    double cornf;
    double fs;
    long fftl;
    DVECTOR phstransw;
    DVECTOR fgdsw;
    DVECTOR gdwt;
    DVECTOR gd;
    DVECTOR apf;

    DVECTOR xphstranswin();
    DVECTOR xgrpdlywin();
    DVECTOR xfgrpdlywin();
    DVECTOR xgdweight(double);
    void getgrpdly();
    void gdtorandomapf();
    void getrandomapf();

  public:
    RPHAPF_CLASS(double sampfreq, long fftlen);
    ~RPHAPF_CLASS();
    void randomspec(DVECTOR spc);
} *RPHAPF;


typedef class Synthesis_CLASS {
  private:
    int fprd;
    double fs;
    long len;
    long hlen;
    double f0min;
    double uvf0;
    XBOOL rnd_flag;
    DVECTOR hann;
    DVECTOR psres;
    DVECTOR pulse;
    DVECTOR noise;
    Vocoder vs;
    RPHAPF rphapf;

    void hanning(double *, long);
    void hanning(double *, long, long, long);
    void pshannwin(long);
    void pshannwin(long, long);
    double sigmoid(double, double, double);
    void psres_mix_rndphs(DMATRIX, long, double);
    void waveampcheck(DVECTOR, XBOOL);

  public:
    Synthesis_CLASS(int, globalP);
    ~Synthesis_CLASS();

    DVECTOR synthesis_body(DVECTOR, DMATRIX, DMATRIX, double, FILE *);
} *Synthesis;

#endif /* _HTS_VOCODER_H */
/* -------------------- End of "vocoder.h" -------------------- */

