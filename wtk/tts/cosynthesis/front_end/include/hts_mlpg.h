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
/*    mlpg.h: speech parameter generation from pdf sequence          */
/*  ---------------------------------------------------------------  */

/* slightly modified - Tomoki Toda 12/05/08 */
/* parameter generation considering GV has been implemented */

#ifndef __HTS_MLPG_H
#define __HTS_MLPG_H

#ifdef __cplusplus
extern "C" {
#endif
#ifdef __cplusplus
}
#endif

#define WLEFT  0
#define WRIGHT 1

/* DWin: structure for regression window */
typedef class DWin_CLASS {
  public:
    int num;         /* number of static + deltas */
    int **width;     /* width [0..num-1][0(left) 1(right)] */
    double **coef;   /* coefficient [0..num-1][length[0]..length[1]] */
    int max_L;       /* max {maxw[0], maxw[1]} */

    DWin_CLASS(char *);
    ~DWin_CLASS();

#ifdef RESOURSE_FILE_ENCRYPTED
    DWin_CLASS(char *, simp_hash_file_t *);
#endif
} *DWin;

typedef class GVPDF_CLASS {
  public:
    double *mean;
    double *var;
    
    GVPDF_CLASS();
    GVPDF_CLASS(char *, int);
    ~GVPDF_CLASS();
#ifdef RESOURSE_FILE_ENCRYPTED
    GVPDF_CLASS(char *, int , simp_hash_file_t *);
#endif
} *GVPDF;

/* SMatrices: structure for matrices to generate sequence of speech parameter vectors */
typedef class SMatrices_CLASS {
  public:
    double **R;      /* W' U^-1 W  */
    double *r;       /* W' U^-1 mu */
    double *g;       /* for forward substitution */
    double *b;       /* for GV gradient */

    SMatrices_CLASS(const int, const int);
    ~SMatrices_CLASS();
    void clean(const int);
} *SMatrices;

/* PStream: structure for parameter generation setting */
typedef class PStream_CLASS {
  private:
    int vSize;		/* vector size of observation vector (include static and dynamic features) */
    int dim;		/* vector size of static features */
    int T;		/* length */
    int width;		/* width of dynamic window */
    DWin dw;		/* dynamic window */
    SMatrices sm;	/* matrices for parameter generation */
    double *vm;
    double *vv;

    void calc_R_and_r(const int);
    void Cholesky();
    void Cholesky_forward();
    void Cholesky_backward(const int);
    void calc_varstats(double **, const int, double *, double *);
    void calc_varstats(const int, double *, double *);
    void varconv(double **, const int, const double);
    void calc_grad(const int);
    void calc_vargrad(const int, const double, const double);
    void calc_stepNW(const int, const double, const double);
    void mlpg();
    
public:
    double **mseq;   /* sequence of mean vector */
    double **ivseq;  /* sequence of invarsed variance vector */
    double **par;	/* output parameter vector */

    PStream_CLASS(const int, const int, DWin, GVPDF);
    ~PStream_CLASS();
    void mlpgGrad(const int, const double, const double, const double,
		  const int, const int);
    void mlpgGradNW(const int, const double, const double, const double,
		    const int, const int);
    int get_T();
    int get_dim();
} *PStream;

#endif /* __HTS_MLPG_H */
/* -------------------- End of "mlpg.h" -------------------- */
